/* SPDX-License-Identifier: Apache-2.0 */

#ifndef RFAL_PLATFORM_H
#define RFAL_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>

gpio_pin_t platform_st25r_int_pin(void);
const struct device *platform_st25r_int_port(void);

#if CONFIG_RFAL_USE_I2C
#define RFAL_USE_I2C
#endif

#define ST25R_INT_PIN            platform_st25r_int_pin()
#define ST25R_INT_PORT           platform_st25r_int_port()

void platform_st25r_protect_comm(void);
void platform_st25r_unprotect_comm(void);
void platform_st25r_protect_irq_status(void);
void platform_st25r_unprotect_irq_status(void);
#define platformProtectST25RComm()          platform_st25r_protect_comm()
#define platformUnprotectST25RComm()        platform_st25r_unprotect_comm()

#define platformProtectST25RIrqStatus()     platform_st25r_protect_irq_status()
#define platformUnprotectST25RIrqStatus()   platform_st25r_unprotect_irq_status()

#define platformProtectWorker()
#define platformUnprotectWorker()

#define platformIrqST25RSetCallback(cb)
#define platformIrqST25RPinInitialize()

#define platformLedsInitialize()
#define platformLedOff(port, pin)
#define platformLedOn(port, pin)
#define platformLedToggle(port, pin)
#define platformLedToogle(port, pin)        platformLedToggle((port), (pin))

#define platformGpioSet(port, pin)          gpio_pin_set((port), (pin), 1)
#define platformGpioClear(port, pin)        gpio_pin_set((port), (pin), 0)
#define platformGpioToggle(port, pin)       gpio_pin_toggle((port), (pin))
#define platformGpioToogle(port, pin)       platformGpioToggle((port), (pin))
#define platformGpioIsHigh(port, pin)       gpio_pin_get((port), (pin))
#define platformGpioIsLow(port, pin)        (!platformGpioIsHigh((port), (pin)))

static inline uint32_t platform_st25r_timer_create(uint16_t timeout_ms)
{
    return k_uptime_get_32() + timeout_ms;
}

static inline bool platform_st25r_timer_is_expired(uint32_t timer)
{
    return (int32_t)(k_uptime_get_32() - timer) >= 0;
}

static inline uint32_t platform_st25r_timer_get_remaining(uint32_t timer)
{
    int32_t remaining = (int32_t)(timer - k_uptime_get_32());

    return (remaining > 0) ? (uint32_t)remaining : 0U;
}

#define platformTimerCreate(t)              platform_st25r_timer_create(t)
#define platformTimerIsExpired(timer)       platform_st25r_timer_is_expired(timer)
#define platformTimerGetRemaining(timer)    platform_st25r_timer_get_remaining(timer)
#define platformTimerDestroy(timer)
#define platformDelay(t)                    k_sleep(K_MSEC(t))
#define platformGetSysTick()                k_uptime_get_32()

#define platformAssert(exp)                 __ASSERT_NO_MSG(exp)

void platform_st25r_global_error(const char *file, long line);
#define platformErrorHandle()               platform_st25r_global_error(__FILE__, __LINE__)

void platform_st25r_spi_select(void);
void platform_st25r_spi_deselect(void);
void platform_st25r_spi_transceive(const uint8_t *txBuf, uint8_t *rxBuf, uint16_t len);
#define platformSpiSelect()                 platform_st25r_spi_select()
#define platformSpiDeselect()               platform_st25r_spi_deselect()
#define platformSpiTxRx(txBuf, rxBuf, len)  platform_st25r_spi_transceive((txBuf), (rxBuf), (len))

void platform_st25r_i2c_send(uint16_t addr, uint8_t *txBuf, uint16_t len, bool last, bool txOnly);
void platform_st25r_i2c_recv(uint16_t addr, uint8_t *rxBuf, uint16_t len);
#define platformI2CTx(txBuf, len, last, txOnly) \
    platform_st25r_i2c_send((uint16_t)0xA0, (uint8_t *)(txBuf), (len), (last), (txOnly))
#define platformI2CRx(rxBuf, len) \
    platform_st25r_i2c_recv((uint16_t)0xA0, (rxBuf), (len))
#define platformI2CStart()
#define platformI2CStop()
#define platformI2CRepeatStart()
#define platformI2CSlaveAddrWR(add)
#define platformI2CSlaveAddrRD(add)

void platform_st25r_log(const char *fmt, ...);
#define platformLog(...)                    platform_st25r_log(__VA_ARGS__)

#define RFAL_FEATURE_LISTEN_MODE            IS_ENABLED(CONFIG_RFAL_FEATURE_LISTEN_MODE)
#define RFAL_FEATURE_WAKEUP_MODE            IS_ENABLED(CONFIG_RFAL_FEATURE_WAKEUP_MODE)
#define RFAL_FEATURE_LOWPOWER_MODE          IS_ENABLED(CONFIG_RFAL_FEATURE_LOWPOWER_MODE)
#define RFAL_FEATURE_NFCA                   IS_ENABLED(CONFIG_RFAL_FEATURE_NFCA)
#define RFAL_FEATURE_NFCB                   IS_ENABLED(CONFIG_RFAL_FEATURE_NFCB)
#define RFAL_FEATURE_NFCF                   IS_ENABLED(CONFIG_RFAL_FEATURE_NFCF)
#define RFAL_FEATURE_NFCV                   IS_ENABLED(CONFIG_RFAL_FEATURE_NFCV)
#define RFAL_FEATURE_T1T                    IS_ENABLED(CONFIG_RFAL_FEATURE_T1T)
#define RFAL_FEATURE_T2T                    IS_ENABLED(CONFIG_RFAL_FEATURE_T2T)
#define RFAL_FEATURE_T4T                    IS_ENABLED(CONFIG_RFAL_FEATURE_T4T)
#define RFAL_FEATURE_ST25TB                 IS_ENABLED(CONFIG_RFAL_FEATURE_ST25TB)
#define RFAL_FEATURE_ST25xV                 IS_ENABLED(CONFIG_RFAL_FEATURE_ST25xV)
#define RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG  IS_ENABLED(CONFIG_RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG)
#define RFAL_FEATURE_DPO                    IS_ENABLED(CONFIG_RFAL_FEATURE_DPO)
#define RFAL_FEATURE_DLMA                   IS_ENABLED(CONFIG_RFAL_FEATURE_DLMA)
#define RFAL_FEATURE_ISO_DEP                IS_ENABLED(CONFIG_RFAL_FEATURE_ISO_DEP)
#define RFAL_FEATURE_ISO_DEP_POLL           IS_ENABLED(CONFIG_RFAL_FEATURE_ISO_DEP_POLL)
#define RFAL_FEATURE_ISO_DEP_LISTEN         IS_ENABLED(CONFIG_RFAL_FEATURE_ISO_DEP_LISTEN)
#define RFAL_FEATURE_NFC_DEP                IS_ENABLED(CONFIG_RFAL_FEATURE_NFC_DEP)

#define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN CONFIG_RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN
#define RFAL_FEATURE_NFC_DEP_BLOCK_MAX_LEN  CONFIG_RFAL_FEATURE_NFC_DEP_BLOCK_MAX_LEN
#define RFAL_FEATURE_NFC_RF_BUF_LEN         CONFIG_RFAL_FEATURE_NFC_RF_BUF_LEN
#define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN   CONFIG_RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN
#define RFAL_FEATURE_NFC_DEP_PDU_MAX_LEN    CONFIG_RFAL_FEATURE_NFC_DEP_PDU_MAX_LEN

#if IS_ENABLED(CONFIG_RFAL_ANALOG_CONFIG_CUSTOM)
#define RFAL_ANALOG_CONFIG_CUSTOM
#endif

#ifdef __cplusplus
}
#endif

#endif /* RFAL_PLATFORM_H */
