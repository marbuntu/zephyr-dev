



#ifndef MLX_UART_OW_HAL_H_
#define MLX_UART_OW_HAL_H_

#include <zephyr.h>
#include <device.h>
#include <stdio.h>
#include <stdint.h>



bool uow_com_init(const struct device *dev);

uint8_t uow_hal_read_bit(const struct device *dev);
void uow_hal_write_bit(const struct device *dev, int bit);

void uow_hal_flush(const struct device *dev);
void uow_hal_transfer_single(const struct device *dev, char *c);

void uow_hal_read(const struct device *dev, uint8_t *buf, size_t len);
void uow_hal_write(const struct device *dev, uint8_t *buf, size_t len);



#endif /* MLX_UART_OW_HAL_H_ */