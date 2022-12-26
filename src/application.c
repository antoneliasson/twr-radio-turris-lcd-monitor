#include <application.h>

#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)


// LED instance
twr_led_t led;

// GFX instance
twr_gfx_t *gfx;

int numberOfPages = 3;

char hostname[20];
char type[50];
char memory[50];
char uptime[30];

char ipAddress[40];
char subnet[40];
char devicesConnected[5];

static const twr_radio_sub_t subs[] = {
    {"update/-/system/info", TWR_RADIO_SUB_PT_STRING, twr_get_system_info, NULL},
    {"update/-/network/info", TWR_RADIO_SUB_PT_STRING, twr_get_network_info, NULL}
};

int display_page_index = 0;

twr_tmp112_t temp;

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    float voltage;
    // int percentage;

    if (twr_module_battery_get_voltage(&voltage))
    {
        twr_radio_pub_battery(&voltage);
    }


    // if (twr_module_battery_get_charge_level(&percentage))
    // {
    //     twr_radio_pub_string("%d", percentage);
    // }
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == TWR_TMP112_EVENT_UPDATE)
    {
        float temperature = 0.0;
        twr_tmp112_get_temperature_celsius(&temp, &temperature);
        twr_radio_pub_temperature(TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE, &temperature);
    }
}

void twr_get_system_info(uint64_t *id, const char *topic, void *value, void *param)
{
    (void) id;
    (void) topic;
    (void) param;
    char *token[4];
    const char s[2] = ";";

    twr_log_info("%s=%s", topic, (char *)value);

    twr_log_debug(__func__);
    token[0] = strtok(value, s);
    token[1] = strtok(NULL, s);
    token[2] = strtok(NULL, s);
    token[3] = strtok(NULL, s);

    strncpy(hostname, token[0], sizeof(hostname));
    strncpy(uptime, token[1], sizeof(uptime));
    strncpy(memory, token[2], sizeof(memory));
    strncpy(type, token[3], sizeof(type));

    twr_scheduler_plan_now(0);


}

void twr_get_network_info(uint64_t *id, const char *topic, void *value, void *param)
{
    (void) id;
    (void) topic;
    (void) param;
    char *token[3];
    const char s[2] = ";";

    twr_log_debug("%s at %llu", __func__, twr_tick_get());
    token[0] = strtok(value, s);
    token[1] = strtok(NULL, s);
    token[2] = strtok(NULL, s);

    strncpy(ipAddress, token[0], sizeof(ipAddress));
    strncpy(subnet, token[1], sizeof(subnet));
    strncpy(devicesConnected, token[2], sizeof(devicesConnected));

    twr_scheduler_plan_now(0);


}

void encoder_event_handler(twr_module_encoder_event_t event, void *event_param)
{
    (void)event_param;
    bool t = true;

    switch (event)
    {
    case TWR_MODULE_ENCODER_EVENT_ROTATION:
        if (twr_module_encoder_get_increment() < 0) {
            display_page_index--;
            if(display_page_index < 0)
            {
                display_page_index = numberOfPages - 1;
            }
        } else {
            display_page_index++;
            if(display_page_index == numberOfPages)
            {
                display_page_index = 0;
            }
        }
        break;
    case TWR_MODULE_ENCODER_EVENT_CLICK:
        switch (display_page_index)
        {
        case 0:
            twr_log_debug("encoder get system info");
            twr_radio_pub_bool("get/system/info", &t);
            break;
        case 1:
            twr_log_debug("encoder get network info: %llu", twr_tick_get());
            twr_radio_pub_bool("get/network/info", &t);
            break;
        case 2:
/*             twr_radio_pub_bool("reboot/-/device", &t); */
            break;
        default:
            break;
        }
        break;
    case TWR_MODULE_ENCODER_EVENT_PRESS:
    case TWR_MODULE_ENCODER_EVENT_HOLD:
    case TWR_MODULE_ENCODER_EVENT_RELEASE:
    case TWR_MODULE_ENCODER_EVENT_ERROR:
    default:
        return;
    }
    twr_scheduler_plan_now(0);
}

void lcd_page_system()
{
    twr_gfx_clear(gfx);
    twr_gfx_printf(gfx, 0, 0, true, "System info:");
    twr_gfx_printf(gfx, 0, 5, true, "------------------");
    twr_gfx_printf(gfx, 0, 15, true, "Hostname:");
    twr_gfx_printf(gfx, 0, 25, true, "%s", hostname);
    twr_gfx_printf(gfx, 0, 40, true, "Type:");
    twr_gfx_printf(gfx, 0, 50, true, "%s", type);
    twr_gfx_printf(gfx, 0, 65, true, "Free memory:");
    twr_gfx_printf(gfx, 0, 75, true, "%s", memory);
    twr_gfx_printf(gfx, 0, 90, true, "Uptime:");
    twr_gfx_printf(gfx, 0, 100, true, "%s", uptime);


}

void lcd_page_network()
{
    twr_gfx_clear(gfx);
    twr_gfx_printf(gfx, 0, 0, true, "Network info(LAN):");
    twr_gfx_printf(gfx, 0, 5, true, "------------------");
    twr_gfx_printf(gfx, 0, 15, true, "IP address:");
    twr_gfx_printf(gfx, 0, 25, true, "%s", ipAddress);
    twr_gfx_printf(gfx, 0, 40, true, "Subnet:");
    twr_gfx_printf(gfx, 0, 50, true, "%s", subnet);
    twr_gfx_printf(gfx, 0, 90, true, "Connected devices:");
    twr_gfx_printf(gfx, 0, 100, true, "%s devices", devicesConnected);


}

void lcd_reboot_page()
{
    twr_gfx_clear(gfx);
    twr_gfx_printf(gfx, 15, 30, true, "Reboot?");

}

void application_init(void)
{

    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);

    twr_module_lcd_init();
    gfx = twr_module_lcd_get_gfx();
    twr_gfx_set_font(gfx, &twr_font_ubuntu_13);
    //twr_module_lcd_set_button_hold_time(300);

    twr_module_encoder_init();
    twr_module_encoder_set_event_handler(encoder_event_handler, NULL);


    // initialize TMP112 sensor
    twr_tmp112_init(&temp, TWR_I2C_I2C0, TWR_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);

    // set measurement handler (call "tmp112_event_handler()" after measurement)
    twr_tmp112_set_event_handler(&temp, tmp112_event_handler, NULL);

    // automatically measure the temperature every 15 minutes
    twr_tmp112_set_update_interval(&temp, 5 * 60 * 1000);

    // Initialze battery module
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    //twr_radio_init(TWR_RADIO_MODE_NODE_LISTENING);
    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);
    twr_radio_set_rx_timeout_for_sleeping_node(400);
    twr_radio_set_subs((twr_radio_sub_t *) subs, sizeof(subs)/sizeof(twr_radio_sub_t));

    twr_radio_pairing_request("turris-mon", VERSION);

    twr_log_debug("jedu");


}

void application_task(void)
{
    twr_log_debug(__func__);
    if(!twr_module_lcd_is_ready())
    {
        twr_scheduler_plan_current_from_now(10);
    }
    else
    {
        twr_gfx_set_font(gfx, &twr_font_ubuntu_13);

        if(display_page_index == 0)
        {
            lcd_page_system();
        }
        else if(display_page_index == 1)
        {
            lcd_page_network();
        }
        else if(display_page_index == 2)
        {
            lcd_reboot_page();
        }

        twr_gfx_update(gfx);

    }
}
