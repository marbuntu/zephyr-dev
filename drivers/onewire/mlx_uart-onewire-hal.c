


#include "mlx_uart-onewire-hal.h"
#include "mlx_onewire-def.h"

#include <zephyr.h>
#include <device.h>
#include <kernel.h>
#include <drivers/uart.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(uow_hal, LOG_LEVEL_INF);




void uow_hal_callback(const struct device *dev, struct uart_event *evt, void *user_data) {

    LOG_INF("Callback triggd");

}


void uow_hal_flush(const struct device *dev) {

    uint8_t c;
    while (uart_fifo_read(dev, &c, 1) > 0) {
        continue;
    }

}


void uow_hal_transfer_single(const struct device *dev, char *c) {

    uart_poll_out(dev, *c);
    while(uart_poll_in(dev, c) != 0);

}


uint8_t uow_hal_read_bit(const struct device *dev) {
    char c = UOW_READ;
    uow_hal_transfer_single(dev, &c);

    return ((c >= 0x0f) && (c < 0xff) ? 0x00 : 0x01);
}


void uow_hal_read(const struct device *dev, uint8_t *buf, size_t len) {
    
    char c;

    for (size_t n = 0; n < len; n++) {
        buf[n] = 0x00;

        for (uint8_t bp = 0x01; bp; bp <<= 1) {
            c = UOW_READ;
            uow_hal_transfer_single(dev, &c);

            if(c == UOW_READ) {
                buf[n] |= bp;
            }
        }
    }
}


void uow_hal_write(const struct device *dev, uint8_t *buf, size_t len) {

    char c;

    for (size_t n = 0; n < len; n++) {
        for (uint8_t bp = 0x01; bp; bp <<= 1) {
            if((buf[n] & bp)) {
                c = UOW_WRITE_1;
            }
            else {
                c = UOW_WRITE_0;
            }

            uow_hal_transfer_single(dev, &c);
        }
    }
}

void uow_hal_write_bit(const struct device *dev, int bit) {

    char c;

    if (bit == 0) {
        c = UOW_WRITE_0;
    }
    else {
        c = UOW_WRITE_1;
    }

    uow_hal_transfer_single(dev, &c);

}

bool uow_com_init(const struct device *dev) {

    struct uart_config cfg;

    /* Read Current Config */
    uart_config_get(dev, &cfg);
    cfg.baudrate = 9600;
    uart_configure(dev, &cfg);

    /* Init Bus */
    char c = UOW_RESET_BYTE;
    uow_hal_transfer_single(dev, &c);

    /* Reset to higher Speed */
    cfg.baudrate = 115200;
    uart_configure(dev, &cfg);

    return (c != UOW_RESET_BYTE ? true : false);
}