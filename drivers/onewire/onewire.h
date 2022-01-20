

#ifndef MLX_ONEWIRE_H_
#define MLX_ONEWIRE_H_

#include <zephyr.h>
#include <device.h>
#include <stdio.h>


typedef struct {
    uint8_t id[8];

} mlx_ow_node_t;



bool mlx_ow_test(const struct device *dev);
size_t mlx_ow_findAll(const struct device *dev, mlx_ow_node_t **sensors, size_t len);

bool mlx_DS18B20_convert(const struct device *dev, uint8_t ROM[8]);
int16_t mlx_DS18B20_read(const struct device *dev, uint8_t ROM[8]);

#endif /* MLX_ONEWIRE_H_ */