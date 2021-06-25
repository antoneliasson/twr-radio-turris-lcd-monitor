#include <application.h>

#include "qrcodegen.h"

#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)


// LED instance
twr_led_t led;

// GFX instance
twr_gfx_t *gfx;

// LCD buttons instance
twr_button_t button_left;
twr_button_t button_right;

int numberOfPages = 4;

// QR code variables
char qr_code[150];
char ssid[32];
char password[64];
char encryption[32];

char hostname[20];
char type[50];
char memory[50];
char uptime[30];

char ipAddress[40];
char subnet[40];
char devicesConnected[5];

static const twr_radio_sub_t subs[] = {
    {"qr/-/chng/code", TWR_RADIO_SUB_PT_STRING, twr_change_qr_value, NULL},
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
    int percentage;

    if (twr_module_battery_get_voltage(&voltage))
    {
        twr_radio_pub_battery(&voltage);
    }


    if (twr_module_battery_get_charge_level(&percentage))
    {
        twr_radio_pub_string("%d", percentage);
    }
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
    char *token[4];
    const char s[2] = ";";


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
    char *token[3];
    const char s[2] = ";";


    token[0] = strtok(value, s);
    token[1] = strtok(NULL, s);
    token[2] = strtok(NULL, s);

    strncpy(ipAddress, token[0], sizeof(ipAddress));
    strncpy(subnet, token[1], sizeof(subnet));
    strncpy(devicesConnected, token[2], sizeof(devicesConnected));

    twr_scheduler_plan_now(0);


}

void twr_change_qr_value(uint64_t *id, const char *topic, void *value, void *param)
{
    char *token[3];
    const char s[2] = ";";
    char psk[6] = "psk2";

    token[0] = strtok(value, s);
    token[1] = strtok(NULL, s);
    token[2] = strtok(NULL, s);

    strncpy(ssid, token[0], sizeof(ssid));
    strncpy(encryption, token[1], sizeof(encryption));
    strncpy(password, token[2], sizeof(password));

    strcpy(qr_code, "");


    if(strcmp(encryption, psk) == 0)
    {
        strncpy(encryption, "WPA2", sizeof(encryption));
    }

    strncat(qr_code, "WIFI:S:", sizeof(qr_code));
    strncat(qr_code, ssid, sizeof(qr_code));
    strncat(qr_code, ";T:", sizeof(qr_code));
    strncat(qr_code, encryption, sizeof(qr_code));
    strncat(qr_code, ";P:", sizeof(qr_code));
    strncat(qr_code, password, sizeof(qr_code));
    strncat(qr_code, ";;", sizeof(qr_code));
    twr_log_debug("%s", qr_code);

    twr_eeprom_write(0, qr_code, sizeof(qr_code));
    get_qr_data();

    qrcode_project(qr_code);

    twr_scheduler_plan_now(500);

}

static void print_qr(const uint8_t qrcode[])
{
    get_qr_data();
    twr_gfx_clear(gfx);

    twr_gfx_draw_string(gfx, 2, 0, "Scan for Wi-Fi connection: ", true);
    twr_gfx_draw_string(gfx, 2, 10, ssid, true);

    uint32_t offset_x = 8;
    uint32_t offset_y = 32;
    uint32_t box_size = 3;
	int size = qrcodegen_getSize(qrcode);
	int border = 2;
	for (int y = -border; y < size + border; y++) {
		for (int x = -border; x < size + border; x++) {
			//fputs((qrcodegen_getModule(qrcode, x, y) ? "##" : "  "), stdout);
            uint32_t x1 = offset_x + x * box_size;
            uint32_t y1 = offset_y + y * box_size;
            uint32_t x2 = x1 + box_size;
            uint32_t y2 = y1 + box_size;

            twr_gfx_draw_fill_rectangle(gfx, x1, y1, x2, y2, qrcodegen_getModule(qrcode, x, y));
		}
		//fputs("\n", stdout);
	}
	//fputs("\n", stdout);
    twr_gfx_update(gfx);
}

void get_qr_data()
{
    for(int i = 7; qr_code[i] != ';'; i++)
    {
        ssid[i - 7] = qr_code[i];
    }


    int i = 0;
    int semicolons = 0;
    bool password_found = false;
    do
    {
        i++;
        if(qr_code[i] == ';')
        {
            semicolons++;
            if(semicolons == 2)
            {
                password_found = true;
            }
        }
    }
    while(!password_found);

    i += 3;


    int start_i = i;

    for(; qr_code[i] != ';'; i++)
    {
        password[i - start_i] = qr_code[i];
    }
}

void lcd_event_handler(twr_module_lcd_event_t event, void *event_param)
{
    if(event == TWR_MODULE_LCD_EVENT_LEFT_CLICK)
    {
        display_page_index--;
        if(display_page_index < 0)
        {
            display_page_index = numberOfPages;
        }
    }
    else if(event == TWR_MODULE_LCD_EVENT_RIGHT_CLICK)
    {
        display_page_index++;
        if(display_page_index > numberOfPages)
        {
            display_page_index = 0;
        }
    }
    else if(event == TWR_MODULE_LCD_EVENT_BOTH_HOLD)
    {
        switch (display_page_index)
        {
        case 0:
            twr_radio_pub_bool("get/system/info", true);
            break;
        case 1:
            twr_radio_pub_bool("get/network/info", true);
            break;

        case 4:
            twr_radio_pub_bool("reboot/-/device", true);
            break;
        default:
            twr_radio_pub_bool("get/qr/info", true);
            break;
        }
    }

    twr_scheduler_plan_now(0);


}

void qrcode_project(char *text)
{
    twr_system_pll_enable();

	// Make and print the QR Code symbol
	uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
	bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_LOW,	qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

	if (ok)
    {
		print_qr(qrcode);
    }

    twr_system_pll_disable();
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

void lcd_page_wifi_data()
{
    twr_gfx_clear(gfx);
    twr_gfx_printf(gfx, 0, 0, true, "Wi-Fi information:");
    twr_gfx_printf(gfx, 0, 5, true, "------------------");
    twr_gfx_printf(gfx, 0, 15, true, "SSID:");
    twr_gfx_printf(gfx, 0, 25, true, "%s", ssid);
    twr_gfx_printf(gfx, 0, 45, true, "Password:");
    twr_gfx_printf(gfx, 0, 55, true, "%s", password);

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
    twr_module_lcd_set_event_handler(lcd_event_handler, NULL);
    twr_gfx_set_font(gfx, &twr_font_ubuntu_13);
    twr_module_lcd_set_button_hold_time(300);


    // initialize TMP112 sensor
    twr_tmp112_init(&temp, TWR_I2C_I2C0, TWR_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);

    // set measurement handler (call "tmp112_event_handler()" after measurement)
    twr_tmp112_set_event_handler(&temp, tmp112_event_handler, NULL);

    // automatically measure the temperature every 15 minutes
    twr_tmp112_set_update_interval(&temp, 5 * 60 * 1000);



    // initialize LCD and load from eeprom
    twr_eeprom_read(0, qr_code, sizeof(qr_code));

    if(strstr(qr_code, "WIFI:S:") == NULL)
    {
        strncpy(qr_code, "WIFI:S:test;T:test;P:test;;", sizeof(qr_code));
    }



    // Initialze battery module
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);
    twr_radio_set_rx_timeout_for_sleeping_node(800);
    twr_radio_set_subs((twr_radio_sub_t *) subs, sizeof(subs)/sizeof(twr_radio_sub_t));

    twr_radio_pairing_request("turris-mon", VERSION);

    twr_log_debug("jedu");


}

void application_task(void)
{
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
            qrcode_project(qr_code);
        }
        else if(display_page_index == 3)
        {
            lcd_page_wifi_data();
        }
        else if(display_page_index == 4)
        {
            twr_gfx_set_font(gfx, &twr_font_ubuntu_24);
            lcd_reboot_page();
        }

        twr_gfx_update(gfx);

    }
}
