#ifndef MY_APP_H__
#define MY_APP_H__

#include <misc/printk.h>
#include <misc/byteorder.h>
#include <nrf.h>
#include <device.h>
#include <stdio.h>

#include <my_mesh.h>

uint8_t pin_to_sw(uint32_t pin_pos);
void button_pressed(struct device *dev, struct gpio_callback *cb,
                        uint32_t pin_pos);
void button_cnt_timer(struct k_timer *work);
void button_pressed_worker(struct k_work *work);
void init_led(u8_t dev, const char *port, u32_t pin_num);
void log_cbuf_put(const char *format, ...);
void my_app_init();

struct device *sw_device;

struct sw {
	u8_t sw_num;
	u8_t onoff_state;
	struct k_work button_work;
	struct k_timer button_timer;
};

u32_t time, last_time;
u8_t button_press_cnt;
struct sw sw;

struct gpio_callback button_cb;

#define BUTTON_DEBOUNCE_DELAY_MS 250

#endif //MY_APP_H__
