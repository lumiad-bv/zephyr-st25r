/* SPDX-License-Identifier: Apache-2.0 */

#ifndef ZEPHYR_ST25R_TRIGGER_H_
#define ZEPHYR_ST25R_TRIGGER_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Callback invoked directly from the ST25R GPIO ISR.
 *
 * This callback must remain ISR-safe and return quickly. It must not access the
 * ST25R over SPI. Its only purpose is to signal an application-owned deferred
 * execution mechanism. That mechanism must later call
 * st25r_process_deferred_interrupt() from a thread.
 */
typedef void (*st25r_deferred_trigger_handler_t)(const struct device *dev,
                                                  void *context);

/** Install the ISR callback and enable GPIO, or pass NULL to disable it. */
int st25r_set_deferred_trigger_handler(
    const struct device *dev,
    st25r_deferred_trigger_handler_t handler,
    void *context);

/**
 * Thread-context counterpart of st25r_deferred_trigger_handler_t.
 *
 * This performs the SPI interrupt-register drain and re-enables the GPIO
 * interrupt if a deferred handler remains installed. Never call it from ISR
 * context.
 */
int st25r_process_deferred_interrupt(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ST25R_TRIGGER_H_ */
