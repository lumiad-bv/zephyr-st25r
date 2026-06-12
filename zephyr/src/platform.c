// Copyright © 2023 Vouch.io LLC

#include "platform.h"

#include <stdarg.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ST25R, CONFIG_ST25R_LOG_LEVEL);

K_MUTEX_DEFINE(platform_st25r_comm_mutex);
K_MUTEX_DEFINE(platform_st25r_irq_status_mutex);

void platform_st25r_protect_comm(void)
{
    k_mutex_lock(&platform_st25r_comm_mutex, K_FOREVER);
}

void platform_st25r_unprotect_comm(void)
{
    k_mutex_unlock(&platform_st25r_comm_mutex);
}

void platform_st25r_protect_irq_status(void)
{
    k_mutex_lock(&platform_st25r_irq_status_mutex, K_FOREVER);
}

void platform_st25r_unprotect_irq_status(void)
{
    k_mutex_unlock(&platform_st25r_irq_status_mutex);
}

void platform_st25r_global_error(const char *file, long line)
{
    LOG_ERR("Error at %s:%ld", file, line);
}

void platform_st25r_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    /* Use a fixed buffer for deferred-safe logging */
    char buf[128];
    vsnprintk(buf, sizeof(buf), fmt, args);
    va_end(args);
    LOG_INF("%s", buf);
}
