


#ifndef ZEPHYR_INCLUDE_DRIVER_ONEWIRE_H_
#define ZEPHYR_INCLUDE_DRIVER_ONEWIRE_H_

#include <zephyr/types.h>
#include <device.h>



struct ow_driver_api {
    int (*config)(const struct device *dev);
    bool (*bus_init)(const struct device *dev);
    void (*bus_write_bit)(const struct device *dev, bool bit);
    int (*bus_read_bit)(const struct device *dev);
    void (*bus_write)(const struct device *dev, uint8_t *buf, size_t len);
    void (*bus_read)(const struct device *dev, uint8_t *buf, size_t len);
};

static inline int onewire_config(const struct device *dev) {
    const struct ow_driver_api *api = (const struct ow_driver_api *) dev->api;
    return api->config(dev);
}

static inline bool onewire_bus_init(const struct device *dev) {
    const struct ow_driver_api *api = (const struct ow_driver_api *) dev->api;
    return api->bus_init(dev);
}

static inline void onewire_write(const struct device *dev, uint8_t *buf, size_t len) {
    const struct ow_driver_api *api = (const struct ow_driver_api *) dev->api;
    api->bus_write(dev, buf, len);
}

static inline void onewire_read(const struct device *dev, uint8_t *buf, size_t len) {
    const struct ow_driver_api *api = (const struct ow_driver_api *) dev->api;
    api->bus_read(dev, buf, len);
}

static inline void onewire_write_bit(const struct device *dev, bool bit) {
    const struct ow_driver_api *api = (const struct ow_driver_api *) dev->api;
    api->bus_write_bit(dev, bit);
}

static inline int onewire_read_bit(const struct device *dev) {
    const struct ow_driver_api *api = (const struct ow_driver_api *) dev->api;
    return api->bus_read_bit(dev);
}

static inline int onewire_send(const struct device *dev, uint8_t *data, size_t len) {

    return 0;
}




#endif /* ZEPHYR_INCLUDE_DRIVER_ONEWIRE_H_ */
