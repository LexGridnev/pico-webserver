#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"

#include "tusb_lwip_glue.h"

#include <stdio.h>

#define LED_PIN     25
#define MIN_FREQ    1
#define MAX_FREQ    500

// LED control variables
bool led_state = false;
bool led_manual_control = false;
uint32_t freq_ms = 500;  // Default frequency in milliseconds

// let our webserver do some dynamic handling
static const char *cgi_toggle_led(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    led_state = !led_state;
    gpio_put(LED_PIN, led_state);
    led_manual_control = led_state;
    return "/index.html";
}

static const char *cgi_reset_usb_boot(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    reset_usb_boot(0, 0);
    return "/index.html";
}

static const char *cgi_set_frequency(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    char *endptr;
    long value = strtol(pcValue[0], &endptr, 10);
    if (endptr == pcValue[0] || *endptr != '\0' || value < MIN_FREQ || value > MAX_FREQ) {
        return "/index.html";  // Invalid frequency, redirect back to index
    }
    freq_ms = (uint32_t)(1000 / value);
    return "/index.html";
}

static const tCGI cgi_handlers[] = {
    {
        "/toggle_led",
        cgi_toggle_led
    },
    {
        "/reset_usb_boot",
        cgi_reset_usb_boot
    },
    {
        "/set_frequency",
        cgi_set_frequency
    }
};

int main()
{
    // Initialize tinyusb, lwip, dhcpd, and httpd
    init_lwip();
    wait_for_netif_is_up();
    dhcpd_init();
    httpd_init();
    http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));

    // For toggle_led
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    uint32_t last_toggle_time = 0;

    while (true)
    {
        tud_task();
        service_traffic();

        // Strobe the LED at the desired frequency if manual control is enabled
        if (led_manual_control)
        {
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            if (current_time - last_toggle_time >= freq_ms)
            {
                last_toggle_time = current_time;
                led_state = !led_state;
                gpio_put(LED_PIN, led_state);
            }
        }
    }

    return 0;
}
