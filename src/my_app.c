#include <my_app.h>

/*
 * Button to Client Model Assignments
 */

static struct bt_mesh_model *mod_cli_sw[] = {
		&root_models[4],
		&secondary_0_models[1],
		&secondary_1_models[1],
		&secondary_2_models[1],
};

/*
 * LED to Server Model Assigmnents
 */

static struct bt_mesh_model *mod_srv_sw[] = {
		&root_models[3],
		&secondary_0_models[0],
		&secondary_1_models[0],
		&secondary_2_models[0],
};

/*
 * Map GPIO pins to button number
 * Change to select different GPIO input pins
 */

uint8_t pin_to_sw(uint32_t pin_pos)
{
	switch (pin_pos) {
	case BIT(SW0_GPIO_PIN): return 0;
	case BIT(SW1_GPIO_PIN): return 1;
	case BIT(SW2_GPIO_PIN): return 2;
	case BIT(SW3_GPIO_PIN): return 3;
	}

	SYS_LOG_ERR("No match for GPIO pin 0x%08x", pin_pos);
	return 0;
}

void button_pressed(struct device *dev, struct gpio_callback *cb,
			   uint32_t pin_pos)
{
	/*
	 * One button press within a 1 second interval sends an on message
	 * More than one button press sends an off message
	 */

	time = k_uptime_get_32();

	/* debounce the switch */
	if (time < last_time + BUTTON_DEBOUNCE_DELAY_MS) {
		last_time = time;
		return;
	}

	if (button_press_cnt == 0) {
		k_timer_start(&sw.button_timer, K_SECONDS(1), 0);
	}

	SYS_LOG_INF("button_press_cnt 0x%02x", button_press_cnt);
	button_press_cnt++;

	/* The variable pin_pos is the pin position in the GPIO register,
	 * not the pin number. It's assumed that only one bit is set.
	 */

	sw.sw_num = pin_to_sw(pin_pos);
	last_time = time;
}

/*
 * Button Count Timer Worker
 */

void button_cnt_timer(struct k_timer *work)
{
	struct sw *button_sw = CONTAINER_OF(work, struct sw, button_timer);

	button_sw->onoff_state = button_press_cnt == 1 ? 1 : 0;
	SYS_LOG_INF("button_press_cnt 0x%02x onoff_state 0x%02x",
			button_press_cnt, button_sw->onoff_state);
	button_press_cnt = 0;
	k_work_submit(&sw.button_work);
}

/*
 * Button Pressed Worker Task
 */

void button_pressed_worker(struct k_work *work)
{
	struct bt_mesh_model *mod_cli, *mod_srv;
	struct bt_mesh_model_pub *pub_cli, *pub_srv;
	struct sw *sw = CONTAINER_OF(work, struct sw, button_work);
	int err;
	u8_t sw_idx = sw->sw_num;

	mod_cli = mod_cli_sw[sw_idx];
	pub_cli = mod_cli->pub;

	mod_srv = mod_srv_sw[sw_idx];
	pub_srv = mod_srv->pub;

	/* If unprovisioned, just call the set function.
	 * The intent is to have switch-like behavior
	 * prior to provisioning. Once provisioned,
	 * the button and its corresponding led are no longer
	 * associated and act independently. So, if a button is to
	 * control its associated led after provisioning, the button
	 * must be configured to either publish to the led's unicast
	 * address or a group to which the led is subscribed.
	 */

	if (primary_addr == BT_MESH_ADDR_UNASSIGNED) {
		NET_BUF_SIMPLE_DEFINE(msg, 1);
		struct bt_mesh_msg_ctx ctx = {
			.addr = sw_idx + primary_addr,
		};

		/* This is a dummy message sufficient
		 * for the led server
		 */

		net_buf_simple_add_u8(&msg, sw->onoff_state);
		gen_onoff_set_unack(mod_srv, &ctx, &msg);
		return;
	}

	if (pub_cli->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	SYS_LOG_INF("publish to 0x%04x onoff 0x%04x sw_idx 0x%04x",
		    pub_cli->addr, sw->onoff_state, sw_idx);
	bt_mesh_model_msg_init(pub_cli->msg,
			       BT_MESH_MODEL_OP_GEN_ONOFF_SET);
	net_buf_simple_add_u8(pub_cli->msg, sw->onoff_state);
	net_buf_simple_add_u8(pub_cli->msg, trans_id++);
	err = bt_mesh_model_publish(mod_cli);
	if (err) {
		SYS_LOG_ERR("bt_mesh_model_publish err %d", err);
	}
}

void init_led(u8_t dev, const char *port, u32_t pin_num)
{
	onoff_state[dev].led_device = device_get_binding(port);
	gpio_pin_configure(onoff_state[dev].led_device,
			   pin_num, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(onoff_state[dev].led_device, pin_num, 1);
}

/*
 * Log functions to identify output with the node
 */

void log_cbuf_put(const char *format, ...)
{
	va_list args;
	char buf[250];

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	printk("[%04x] %s", primary_addr, buf);
}

void my_app_init()
{
	/*
	 * Initialize the logger hook
	 */
	syslog_hook_install(log_cbuf_put);

	SYS_LOG_INF("Initializing...");

	/* Initialize the button debouncer */
	last_time = k_uptime_get_32();

	/* Initialize button worker task*/
	k_work_init(&sw.button_work, button_pressed_worker);

	/* Initialize button count timer */
	k_timer_init(&sw.button_timer, button_cnt_timer, NULL);

	sw_device = device_get_binding(SW0_GPIO_NAME);
	gpio_pin_configure(sw_device, SW0_GPIO_PIN,
			  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
	gpio_pin_configure(sw_device, SW1_GPIO_PIN,
			  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
	gpio_pin_configure(sw_device, SW2_GPIO_PIN,
			  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
	gpio_pin_configure(sw_device, SW3_GPIO_PIN,
			  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
	gpio_init_callback(&button_cb, button_pressed,
			   BIT(SW0_GPIO_PIN) | BIT(SW1_GPIO_PIN) |
			   BIT(SW2_GPIO_PIN) | BIT(SW3_GPIO_PIN));
	gpio_add_callback(sw_device, &button_cb);
	gpio_pin_enable_callback(sw_device, SW0_GPIO_PIN);
	gpio_pin_enable_callback(sw_device, SW1_GPIO_PIN);
	gpio_pin_enable_callback(sw_device, SW2_GPIO_PIN);
	gpio_pin_enable_callback(sw_device, SW3_GPIO_PIN);

	/* Initialize LED's */
	init_led(0, LED0_GPIO_PORT, LED0_GPIO_PIN);
	init_led(1, LED1_GPIO_PORT, LED1_GPIO_PIN);
	init_led(2, LED2_GPIO_PORT, LED2_GPIO_PIN);
	init_led(3, LED3_GPIO_PORT, LED3_GPIO_PIN);
}
