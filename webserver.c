#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "tusb_lwip_glue.h"

#define LED_PIN 16
#define NUM_PIXELS 1

static PIO pio;
static uint sm;
static uint offset;

static uint32_t pixel_colors[NUM_PIXELS] = {0};

static const char *cgi_update_led(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    uint8_t red = 0, green = 0, blue = 0, brightness = 0;
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "red") == 0) {
            red = atoi(pcValue[i]);
        } else if (strcmp(pcParam[i], "green") == 0) {
            green = atoi(pcValue[i]);
        } else if (strcmp(pcParam[i], "blue") == 0) {
            blue = atoi(pcValue[i]);
        } else if (strcmp(pcParam[i], "brightness") == 0) {
            brightness = atoi(pcValue[i]);
        }
    }

    uint32_t color = ((uint32_t)(red) << 16) | ((uint32_t)(green) << 8) | (uint32_t)(blue);
    uint32_t adjusted_brightness = (brightness * 255) / 100;
    color = (color & 0x00FFFFFF) | (adjusted_brightness << 24);

    for (int i = 0; i < NUM_PIXELS; i++) {
        pixel_colors[i] = color;
    }

    return "/index.html";
}

static const tCGI cgi_handlers[] = {
    {
        "/update_led",
        cgi_update_led
    }
};

void ws2812_init()
{
    pio = pio0;
    sm = 0;
    offset = pio_add_program(pio, &ws2812_program);

    pio_gpio_init(pio, LED_PIN);
    pio_sm_config config = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&config, LED_PIN);
    sm_config_set_out_shift(&config, false, true, 0);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_RX);

    pio_sm_init(pio, sm, offset, &config);
    pio_sm_set_enabled(pio, sm, true);
}

void ws2812_update()
{
    for (int i = 0; i < NUM_PIXELS; i++) {
        pio_sm_put_blocking(pio, sm, pixel_colors[i]);
    }
}

int main()
{
    stdio_init_all();

    // Initialize tinyusb, lwip, dhcpd, and httpd
    init_lwip();
    wait_for_netif_is_up();
    dhcpd_init();
    httpd_init();
    http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));

    // Initialize WS2812 LED
    ws2812_init();

    while (true) {
        tud_task();
        service_traffic();

        // Update WS2812 LED
        ws2812_update();
    }

    return 0;
}
``
