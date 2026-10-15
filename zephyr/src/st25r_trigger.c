// Copyright © 2023 Vouch.io LLC

#define DT_DRV_COMPAT st_st25r

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "platform.h"
#if defined(CONFIG_ST25R500)
#include "st25r500_irq.h"
#define st25r_chip_isr()	st25r500Isr()
#else
#include "st25r3916_irq.h"
#define st25r_chip_isr()	st25r3916Isr()
#endif

#include "st25r.h"

LOG_MODULE_DECLARE(ST25R, CONFIG_ST25R_LOG_LEVEL);

#if defined(CONFIG_ST25R_TRIGGER_OWN_THREAD) || \
	defined(CONFIG_ST25R_TRIGGER_GLOBAL_THREAD) || \
	defined(CONFIG_ST25R_TRIGGER_DEFERRED)
/* Re-arm the edge interrupt. The ST25R holds IRQ active until its interrupt
 * registers are read, so an IRQ raised while the pin was disarmed produces no
 * edge. Returns 1, with the pin disarmed again, when the line is already
 * active and must be drained once more; 0 when armed and idle.
 */
static int st25r_rearm_interrupt(const struct st25r_device_config *cfg)
{
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	if (gpio_pin_get_dt(&cfg->int_gpio) <= 0) {
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

	return (ret < 0) ? ret : 1;
}
#endif

#if defined(CONFIG_ST25R_TRIGGER_OWN_THREAD) || \
	defined(CONFIG_ST25R_TRIGGER_GLOBAL_THREAD)
static void st25r_handle_interrupt(const struct device *dev)
{
	const struct st25r_device_config *cfg = dev->config;

	do {
		st25r_chip_isr();
	} while (st25r_rearm_interrupt(cfg) > 0);
}
#endif

#ifdef CONFIG_ST25R_TRIGGER_DEFERRED
int st25r_set_deferred_trigger_handler(
	const struct device *dev,
	st25r_deferred_trigger_handler_t handler,
	void *context)
{
	struct st25r_data *st25r;
	const struct st25r_device_config *cfg;
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}

	st25r = dev->data;
	cfg = dev->config;

	if (handler == NULL) {
		ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
						      GPIO_INT_DISABLE);
		st25r->deferred_handler = NULL;
		st25r->deferred_context = NULL;
		return ret;
	}

	st25r->deferred_context = context;
	st25r->deferred_handler = handler;
	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		st25r->deferred_handler = NULL;
		st25r->deferred_context = NULL;
	}

	return ret;
}

int st25r_process_deferred_interrupt(const struct device *dev)
{
	struct st25r_data *st25r;
	const struct st25r_device_config *cfg;
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(!k_is_in_isr());

	st25r = dev->data;
	cfg = dev->config;

	do {
		st25r_chip_isr();

		if (st25r->deferred_handler == NULL) {
			return 0;
		}

		ret = st25r_rearm_interrupt(cfg);
	} while (ret > 0);

	return ret;
}
#endif /* CONFIG_ST25R_TRIGGER_DEFERRED */

static void st25r_gpio_callback(const struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct st25r_data *st25r =
		CONTAINER_OF(cb, struct st25r_data, gpio_cb);
	const struct st25r_device_config *cfg = st25r->dev->config;

	if ((pins & BIT(cfg->int_gpio.pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_ST25R_TRIGGER_OWN_THREAD)
	k_sem_give(&st25r->gpio_sem);
#elif defined(CONFIG_ST25R_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&st25r->work);
#elif defined(CONFIG_ST25R_TRIGGER_DEFERRED)
	/* ISR side of the deferred contract: only notify the owner. The owner
	 * schedules st25r_process_deferred_interrupt() from a thread.
	 */
	if (st25r->deferred_handler != NULL) {
		st25r->deferred_handler(st25r->dev,
					st25r->deferred_context);
	}
#endif /* CONFIG_ST25R_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_ST25R_TRIGGER_OWN_THREAD
static void st25r_thread(struct st25r_data *st25r)
{
	while (1) {
		k_sem_take(&st25r->gpio_sem, K_FOREVER);
		st25r_handle_interrupt(st25r->dev);
	}
}
#endif /* CONFIG_ST25R_TRIGGER_OWN_THREAD */

#ifdef CONFIG_ST25R_TRIGGER_GLOBAL_THREAD
static void st25r_work_cb(struct k_work *work)
{
	struct st25r_data *st25r =
		CONTAINER_OF(work, struct st25r_data, work);

	st25r_handle_interrupt(st25r->dev);
}
#endif /* CONFIG_ST25R_TRIGGER_GLOBAL_THREAD */

static gpio_pin_t s_int_pin;
static const struct device *s_int_port;

gpio_pin_t platform_st25r_int_pin(void)
{
    return s_int_pin;
}

const struct device *platform_st25r_int_port(void)
{
    return s_int_port;
}

int st25r_init_interrupt(const struct device *dev)
{
	struct st25r_data *st25r = dev->data;
	const struct st25r_device_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->int_gpio.port)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->int_gpio.port->name);
		return -ENODEV;
	}

	s_int_pin = cfg->int_gpio.pin;
	s_int_port = cfg->int_gpio.port;

	st25r->dev = dev;

#if defined(CONFIG_ST25R_TRIGGER_OWN_THREAD)
	k_sem_init(&st25r->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&st25r->thread, st25r->thread_stack,
                    CONFIG_ST25R_THREAD_STACK_SIZE,
		       (k_thread_entry_t)st25r_thread, st25r,
		       0, NULL, K_PRIO_PREEMPT(CONFIG_ST25R_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_ST25R_TRIGGER_GLOBAL_THREAD)
	st25r->work.handler = st25r_work_cb;
#elif defined(CONFIG_ST25R_TRIGGER_DEFERRED)
	st25r->deferred_handler = NULL;
	st25r->deferred_context = NULL;
#endif /* CONFIG_ST25R_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&st25r->gpio_cb,
			   st25r_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	if (gpio_add_callback(cfg->int_gpio.port, &st25r->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback");
		return -EIO;
	}

#if defined(CONFIG_ST25R_TRIGGER_DEFERRED)
	/* The integrating module enables the GPIO only after both sides of the
	 * deferred callback contract are ready.
	 */
	return 0;
#else
	return gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					       GPIO_INT_EDGE_TO_ACTIVE);
#endif
}
