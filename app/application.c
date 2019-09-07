#include <application.h>

#include "qrcodegen.h"

#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)


// LED instance
bc_led_t led;

// GFX instance
bc_gfx_t *gfx;

// LCD buttons instance
bc_button_t button_left;
bc_button_t button_right;

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

static const bc_radio_sub_t subs[] = {
    {"qr/-/chng/code", BC_RADIO_SUB_PT_STRING, bc_change_qr_value, NULL},
    {"update/-/system/info", BC_RADIO_SUB_PT_STRING, bc_get_system_info, NULL},
    {"update/-/network/info", BC_RADIO_SUB_PT_STRING, bc_get_network_info, NULL}

};

int display_page_index = 0;

bc_tmp112_t temp;

void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    float voltage;
    int percentage;

    if (bc_module_battery_get_voltage(&voltage))
    {
        bc_radio_pub_battery(&voltage);
    }


    if (bc_module_battery_get_charge_level(&percentage))
    {
        bc_radio_pub_string("%d", percentage);
    }
}

void tmp112_event_handler(bc_tmp112_t *self, bc_tmp112_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_TMP112_EVENT_UPDATE)
    {
        float temperature = 0.0;
        bc_tmp112_get_temperature_celsius(&temp, &temperature);
        bc_radio_pub_temperature(BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE, &temperature);
    }
}

void bc_get_system_info(uint64_t *id, const char *topic, void *value, void *param)
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

    bc_scheduler_plan_now(0);

    
}

void bc_get_network_info(uint64_t *id, const char *topic, void *value, void *param)
{
    char *token[3];
    const char s[2] = ";";


    token[0] = strtok(value, s);
    token[1] = strtok(NULL, s);
    token[2] = strtok(NULL, s);

    strncpy(ipAddress, token[0], sizeof(ipAddress));
    strncpy(subnet, token[1], sizeof(subnet));
    strncpy(devicesConnected, token[2], sizeof(devicesConnected));

    bc_scheduler_plan_now(0);

    
}

void bc_change_qr_value(uint64_t *id, const char *topic, void *value, void *param)
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
    bc_log_debug("%s", qr_code);

    bc_eeprom_write(0, qr_code, sizeof(qr_code));
    get_qr_data();

    qrcode_project(qr_code);

    bc_scheduler_plan_now(500);

}

static void print_qr(const uint8_t qrcode[]) 
{
    get_qr_data();
    bc_gfx_clear(gfx);

    bc_gfx_draw_string(gfx, 2, 0, "Scan for Wi-Fi connection: ", true);
    bc_gfx_draw_string(gfx, 2, 10, ssid, true);

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

            bc_gfx_draw_fill_rectangle(gfx, x1, y1, x2, y2, qrcodegen_getModule(qrcode, x, y));
		}
		//fputs("\n", stdout);
	}
	//fputs("\n", stdout);
    bc_gfx_update(gfx);
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

void lcd_event_handler(bc_module_lcd_event_t event, void *event_param)
{ 
    if(event == BC_MODULE_LCD_EVENT_LEFT_CLICK) 
    {
        display_page_index--;
        if(display_page_index < 0)
        {
            display_page_index = numberOfPages;
        }
    }
    else if(event == BC_MODULE_LCD_EVENT_RIGHT_CLICK) 
    {
        display_page_index++;
        if(display_page_index > numberOfPages)
        {
            display_page_index = 0;
        }
    }
    else if(event == BC_MODULE_LCD_EVENT_BOTH_HOLD)
    {
        switch (display_page_index)
        {
        case 0:
            bc_radio_pub_bool("get/system/info", true);
            break;
        case 1:
            bc_radio_pub_bool("get/network/info", true);
            break;
        
        case 4:
            bc_radio_pub_bool("reboot/-/device", true);
            break;
        default: 
            bc_radio_pub_bool("get/qr/info", true);
            break;
        }
    }

    bc_scheduler_plan_now(0);


}

void qrcode_project(char *text)
{
    bc_system_pll_enable();

	// Make and print the QR Code symbol
	uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
	bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_LOW,	qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

	if (ok)
    {
		print_qr(qrcode);
    }

    bc_system_pll_disable();
}

void lcd_page_system()
{
    bc_gfx_clear(gfx);
    bc_gfx_printf(gfx, 0, 0, true, "System info:");
    bc_gfx_printf(gfx, 0, 5, true, "------------------");
    bc_gfx_printf(gfx, 0, 15, true, "Hostname:");
    bc_gfx_printf(gfx, 0, 25, true, "%s", hostname);
    bc_gfx_printf(gfx, 0, 40, true, "Type:");
    bc_gfx_printf(gfx, 0, 50, true, "%s", type);
    bc_gfx_printf(gfx, 0, 65, true, "Free memory:");
    bc_gfx_printf(gfx, 0, 75, true, "%s", memory);
    bc_gfx_printf(gfx, 0, 90, true, "Uptime:");
    bc_gfx_printf(gfx, 0, 100, true, "%s", uptime);


}

void lcd_page_network()
{
    bc_gfx_clear(gfx);
    bc_gfx_printf(gfx, 0, 0, true, "Network info(LAN):");
    bc_gfx_printf(gfx, 0, 5, true, "------------------");
    bc_gfx_printf(gfx, 0, 15, true, "IP address:");
    bc_gfx_printf(gfx, 0, 25, true, "%s", ipAddress);
    bc_gfx_printf(gfx, 0, 40, true, "Subnet:");
    bc_gfx_printf(gfx, 0, 50, true, "%s", subnet);
    bc_gfx_printf(gfx, 0, 90, true, "Connected devices:");
    bc_gfx_printf(gfx, 0, 100, true, "%s devices", devicesConnected);


}

void lcd_page_wifi_data()
{
    bc_gfx_clear(gfx);
    bc_gfx_printf(gfx, 0, 0, true, "Wi-Fi information:");
    bc_gfx_printf(gfx, 0, 5, true, "------------------");
    bc_gfx_printf(gfx, 0, 15, true, "SSID:");
    bc_gfx_printf(gfx, 0, 25, true, "%s", ssid);
    bc_gfx_printf(gfx, 0, 45, true, "Password:");
    bc_gfx_printf(gfx, 0, 55, true, "%s", password);

}

void lcd_reboot_page()
{
    bc_gfx_clear(gfx);
    bc_gfx_printf(gfx, 15, 30, true, "Reboot?");

}

void application_init(void)
{

    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);

    bc_module_lcd_init();
    gfx = bc_module_lcd_get_gfx();
    bc_module_lcd_set_event_handler(lcd_event_handler, NULL);
    bc_gfx_set_font(gfx, &bc_font_ubuntu_13);
    bc_module_lcd_set_button_hold_time(300);

    
    // initialize TMP112 sensor
    bc_tmp112_init(&temp, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);

    // set measurement handler (call "tmp112_event_handler()" after measurement)
    bc_tmp112_set_event_handler(&temp, tmp112_event_handler, NULL);

    // automatically measure the temperature every 15 minutes
    bc_tmp112_set_update_interval(&temp, 5 * 60 * 1000);



    // initialize LCD and load from eeprom
    bc_eeprom_read(0, qr_code, sizeof(qr_code));

    if(strstr(qr_code, "WIFI:S:") == NULL)
    {
        strncpy(qr_code, "WIFI:S:test;T:test;P:test;;", sizeof(qr_code));
    }



    // Initialze battery module
    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);
    bc_radio_set_rx_timeout_for_sleeping_node(800);
    bc_radio_set_subs((bc_radio_sub_t *) subs, sizeof(subs)/sizeof(bc_radio_sub_t));

    bc_radio_pairing_request("turris-mon", VERSION);

    bc_log_debug("jedu");


}

void application_task(void)
{
    if(!bc_module_lcd_is_ready())
    {
        bc_scheduler_plan_current_from_now(10);    
    }
    else
    {
        bc_gfx_set_font(gfx, &bc_font_ubuntu_13);

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
            bc_gfx_set_font(gfx, &bc_font_ubuntu_24);
            lcd_reboot_page();
        }

        bc_gfx_update(gfx);

    }
}