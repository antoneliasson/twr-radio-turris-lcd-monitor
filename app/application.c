#include <application.h>

// LED instance
bc_led_t led;

char m_battery_str[16] = "";
char m_devices_name[3][20] = {{"-1. Device-"}, {"-2. Device"}, {"-3. Device-"}};
char m_devices_state[3][4];
char m_devices_voltage[3][6];

bc_button_t button_left;
bc_button_t button_right;

// Button instance
bc_button_t button;
bc_scheduler_task_id_t updateTask;

void menu_main_callback(Menu *menu, MenuItem *item);
void menu_device_callback(Menu *menu, MenuItem *item);
void update_values(uint64_t *id, const char *topic, void *value, void *param);
static void update_task(void* param);

Menu menu_device_1;
Menu menu_device_2;
Menu menu_device_3;

MenuItem m_item_device_1 = {{m_devices_name[0]}, &menu_device_1,  MENU_PARAMETER_IS_STRING, (void*)&m_devices_state[0]};
MenuItem m_item_device_2 = {{m_devices_name[1]}, &menu_device_2,  MENU_PARAMETER_IS_STRING , (void*)&m_devices_state[1]};
MenuItem m_item_device_3 = {{m_devices_name[2]}, &menu_device_3,  MENU_PARAMETER_IS_STRING , (void*)&m_devices_state[2]};
MenuItem m_item_update = {{"Update"}, NULL, NULL, NULL};
MenuItem m_item_battery = {{"Battery"}, NULL, MENU_PARAMETER_IS_STRING, (void*)&m_battery_str};


MenuItem m_item_voltage = {{"Voltage"}, NULL,  MENU_PARAMETER_IS_STRING, m_devices_voltage[0]};
MenuItem m_item_state = {{"State"}, NULL,  MENU_PARAMETER_IS_STRING, m_devices_state[0]};



Menu menu_main = {
    {"Main menu"},
    .items = {&m_item_device_1, &m_item_device_2, &m_item_device_3, &m_item_update, &m_item_battery, 0},
    .refresh = 200,
    .callback = menu_main_callback
};
Menu menu_device_1 = {
    {m_devices_name[0]},
    .items = {&m_item_voltage, &m_item_state, 0},
    .refresh = 200,
    .callback = menu_device_callback
};
Menu menu_device_2 = {
    {m_devices_name[1]},
    .items = {&m_item_voltage, &m_item_state, 0},
    .refresh = 200,
    .callback = menu_device_callback
};
Menu menu_device_3 = {
    {m_devices_name[2]},
    .items = {&m_item_voltage, &m_item_state, 0},
    .refresh = 200,
    .callback = menu_device_callback
};

static const bc_radio_sub_t subs[] = {
    {"submit/-/netio/data", BC_RADIO_SUB_PT_STRING, update_values, NULL},
};

void update_values(uint64_t *id, const char *topic, void *value, void *param)
{
    bc_log_debug("updatuju");
    const char s[2] = ";";
    char *token[4];
    bc_led_blink(&led, 2);

    for(int i = 0; i < 3; i++)
    {
        if(i == 0)
        {
            token[0] = strtok(value, s);
        }
        else 
        {
            token[0] = strtok(NULL, s);
        }
        token[1] = strtok(NULL, s);
        token[2] = strtok(NULL, s); 

        if(!strncmp(token[0], "-1", sizeof(token[0])))
        {
            return;
        }
        else
        {
            bc_log_debug("%s", token[0]);
            strncpy(m_devices_name[i], token[0], sizeof(m_devices_name[i]));
        }
        
        if(!strncmp(token[1], "-1", sizeof(token[0])))
        {
            return;
        }
        else
        {
            bc_log_debug("%s", token[1]);
            strncpy(m_devices_voltage[i], token[1], sizeof(m_devices_voltage[i]));
        }

        if(!strncmp(token[2], "-1", sizeof(token[0])))
        {
            return;
        }
        else
        {
            bc_log_debug("%s", token[2]);
            strncpy(m_devices_state[i], token[2], sizeof(m_devices_state[i]));
        }   
    }
}

void menu_main_callback(Menu *menu, MenuItem *item)
{
    if(item == &m_item_update)
    {
        bc_radio_pub_bool("request/update", true);
    }
    else if(item == &m_item_battery)
    {

    }
    else
    {
        m_item_state.parameter = m_devices_state[menu_main.selectedIndex];
        m_item_voltage.parameter = m_devices_voltage[menu_main.selectedIndex];
    }
}

void menu_device_callback(Menu *menu, MenuItem *item)
{
    if(item == &m_item_state)
    {
        if(menu == &menu_device_1)
        {
            bc_radio_pub_bool("device/1/toggle", true);
        }
        else if(menu == &menu_device_2)
        {
            bc_radio_pub_bool("device/2/toggle", true);
        }
        else if(menu == &menu_device_3)
        {
            bc_radio_pub_bool("device/3/toggle", true);
        }
        bc_scheduler_plan_relative(updateTask, 1025);
    }
}

int app_state = 0;



void lcdBufferString(char *str, int x, int y)
{
    bc_module_lcd_draw_string(x, y, str, 1);
}

void lcdBufferNumber(int number, int x, int y)
{
    char str[16];
    snprintf(str, sizeof(str), "%d", number);
    bc_module_lcd_draw_string(x, y, str, 1);
}

void lcd_event_handler(bc_module_lcd_event_t event, void *event_param)
{
    bc_scheduler_plan_now(0);

    if (app_state == 0)
    {
        if (event == BC_MODULE_LCD_EVENT_LEFT_CLICK)
        {
            menu2_event(&menu_main, BTN_UP);
        }
        if (event == BC_MODULE_LCD_EVENT_LEFT_HOLD)
        {
            menu2_event(&menu_main, BTN_LEFT);
        }
        else if (event == BC_MODULE_LCD_EVENT_RIGHT_CLICK)
        {
            menu2_event(&menu_main, BTN_DOWN);
        }
        else if (event == BC_MODULE_LCD_EVENT_RIGHT_HOLD)
        {
            menu2_event(&menu_main, BTN_ENTER);
        }
    }
}

void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    (void) event_param;

    float voltage;

    if (event == BC_MODULE_BATTERY_EVENT_UPDATE)
    {
        if (bc_module_battery_get_voltage(&voltage))
        {
            snprintf(m_battery_str, sizeof(m_battery_str), "%02.1f V", voltage);
        }
    }
}

static void update_task(void* param) 
{
    (void) param;
    bc_radio_pub_bool("request/update", true);

}


void application_init(void)
{
    bc_log_init(BC_LOG_LEVEL_DEBUG, BC_LOG_TIMESTAMP_ABS);
    
    // Init LCD
    bc_module_lcd_init();
    bc_module_lcd_set_font(&bc_font_ubuntu_13);
    bc_module_lcd_update();

    // Initialize battery
    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(5 * 60 * 1000);

    // Initialize logging
    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_blink(&led, 3);

    bc_module_lcd_set_event_handler(lcd_event_handler, NULL);

    bc_button_set_hold_time(&button_left, 300);
    bc_button_set_hold_time(&button_right, 300);

    bc_button_set_debounce_time(&button_left, 30);
    bc_button_set_debounce_time(&button_left, 30);
    
    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);
    bc_radio_set_rx_timeout_for_sleeping_node(250);
    bc_radio_set_subs((bc_radio_sub_t *) subs, sizeof(subs)/sizeof(bc_radio_sub_t));

    bc_radio_pairing_request("netio-lcd-monitor", VERSION);

    updateTask = bc_scheduler_register(update_task, NULL, 1000);

    menu2_init(&menu_main);
    menu2_init(&menu_device_1);
    menu2_init(&menu_device_2);
    menu2_init(&menu_device_3);

        
}

void application_task(void)
{
    if (!bc_module_lcd_is_ready())
    {
        bc_scheduler_plan_current_relative(20);
        return;
    }

    bc_system_pll_enable();

    menu2_draw(&menu_main);

    bc_system_pll_disable();

    bc_scheduler_plan_current_relative(200);

}