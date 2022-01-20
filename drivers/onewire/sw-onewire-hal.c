

#include <zephyr.h>
#include <device.h>

#include <drivers/gpio.h>
#include <drivers/onewire.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(swow, LOG_LEVEL_INF);

#define DT_DRV_COMPAT sw_one_wire

#define OW_LOW                 0
#define OW_HIGH                1

#define OW_INIT_PULSE_PULLDOWN      480
#define OW_INIT_PULSE_SAMPLE        70
#define OW_INIT_PULSE_PULLUP        410       

#define OW_WRITE_PULLDOWN_1	        6
#define OW_WRITE_PULLUP_1		    64

#define	OW_WRITE_PULLDOWN_0     	60
#define OW_WRITE_PULLUP_0		    10

#define OW_READ_PULLDOWN            6
#define OW_READ_SAMPLE              9
#define OW_READ_PULLUP              55


BUILD_ASSERT(
    DT_HAS_COMPAT_STATUS_OKAY(sw_one_wire),
    "No One-Wire Bus specified in DT"
);


struct swow_gpio_t {
    const struct device *port;
    const char* label;
    gpio_pin_t pin;
    gpio_flags_t flags;
};


struct swow_cfg_t {
    struct gpio_dt_spec pin_rd;
    struct gpio_dt_spec pin_wr;
};


static int swow_init(const struct device *dev);


static int swow_config(const struct device *dev) {
    return swow_init(dev);;
}


static bool swow_bus_init(const struct device *dev) {

    const struct swow_cfg_t *cfg = dev->config;
    //const struct device *wr = device_get_binding(cfg->pin_wr.label);
   // const struct device *rd = device_get_binding(cfg->pin_rd.label);

    gpio_pin_set(cfg->pin_wr.port, cfg->pin_wr.pin, OW_LOW);
    k_busy_wait(OW_INIT_PULSE_PULLDOWN);

    gpio_pin_set(cfg->pin_wr.port, cfg->pin_wr.pin, OW_HIGH);  
    k_busy_wait(OW_INIT_PULSE_SAMPLE);

    int pr = gpio_pin_get(cfg->pin_rd.port, cfg->pin_rd.pin);

    k_busy_wait(OW_INIT_PULSE_PULLUP);

    return (pr == 0 ? true : false);
}


static void swow_write_bit(const struct device *dev, bool bit){

	uint16_t wait_low = ( bit ? OW_WRITE_PULLDOWN_1 : OW_WRITE_PULLDOWN_0);
	uint16_t wait_high = ( bit ? OW_WRITE_PULLUP_1 : OW_WRITE_PULLUP_0);

    const struct swow_cfg_t *cfg = dev->config;
    //const struct device *wr = device_get_binding(cfg->pin_wr.label);

	gpio_pin_set(cfg->pin_wr.port, cfg->pin_wr.pin, OW_LOW);
	k_busy_wait(wait_low);

	gpio_pin_set(cfg->pin_wr.port, cfg->pin_wr.pin, OW_HIGH);
	k_busy_wait(wait_high);

}


static int swow_read_bit(const struct device *dev) {

    const struct swow_cfg_t *cfg = dev->config;
    //const struct device *wr = device_get_binding(cfg->pin_wr.label);
    //const struct device *rd = device_get_binding(cfg->pin_rd.label);

//    LOG_WRN("LBL: %s", cfg->pin_rd.label);

    gpio_pin_set(cfg->pin_wr.port, cfg->pin_wr.pin, OW_LOW);
    k_busy_wait(OW_READ_PULLDOWN);

    gpio_pin_set(cfg->pin_wr.port, cfg->pin_wr.pin, OW_HIGH);  
    k_busy_wait(OW_READ_SAMPLE);

    int pr = gpio_pin_get(cfg->pin_rd.port, cfg->pin_rd.pin);

    k_busy_wait(OW_READ_PULLUP);


    return pr;
}


static int swow_init(const struct device *dev) {

    LOG_INF("Init Called");

    const struct swow_cfg_t *cfg = dev->config;

    //struct swow_gpio_t wr = cfg->pin_wr;
    //wr.port = device_get_binding(wr.label);
    
    struct gpio_dt_spec wr = cfg->pin_wr;
    int ret = gpio_pin_configure(wr.port, wr.pin, wr.dt_flags | GPIO_OUTPUT_ACTIVE);
    gpio_pin_set(wr.port, wr.pin, OW_HIGH);

    struct gpio_dt_spec rd = cfg->pin_rd;       
    ret += gpio_pin_configure(rd.port, rd.pin, rd.dt_flags | GPIO_INPUT);
    //gpio_pin_set(rd.port, rd.pin, OW_HIGH);

    return ret;
}


static void swow_write(const struct device *dev, uint8_t *buf, size_t len) {

    for (size_t n = 0; n < len; n++) {
        for (uint8_t bp = 0x01; bp; bp <<= 1) {
            swow_write_bit(dev, (bool) (buf[n] & bp));
        }
    } 

}


static void swow_read(const struct device *dev, uint8_t *buf, size_t len) {

    int c;

    for (size_t n = 0; n < len; n++) {
        buf[n] = 0x00;

        for (uint8_t bp = 0x01; bp; bp <<= 1) {
            c = swow_read_bit(dev);
            
            if(c) {
                buf[n] |= bp;
            }
        }
    }

}



static const struct ow_driver_api swow_api = {
    .config = swow_config,
    .bus_init = swow_bus_init,
    .bus_write_bit = swow_write_bit,
    .bus_read_bit = swow_read_bit,
    .bus_write = swow_write,
    .bus_read = swow_read
};


static int swow_pm_ctrl(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;
    const struct swow_cfg_t *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
        
        pm_device_busy_set(cfg->pin_wr.port);
        pm_device_busy_set(cfg->pin_rd.port);
		ret = swow_init(dev);
        
		break;
	case PM_DEVICE_ACTION_SUSPEND:
    /*    __fallthrough;
    case PM_DEVICE_ACTION_TURN_OFF:
        __fallthrough;
    case PM_DEVICE_ACTION_LOW_POWER:
*/
        LOG_INF("Setting Pins LOW");
        gpio_pin_set_raw(cfg->pin_wr.port, cfg->pin_wr.pin, 0);
        gpio_pin_set_raw(cfg->pin_rd.port, cfg->pin_rd.pin, 0);

        pm_device_busy_clear(cfg->pin_wr.port);
        pm_device_busy_clear(cfg->pin_rd.port);

        //gpio_pin_configure(cfg->pin_wr.port, cfg->pin_wr.pin, GPIO_DISCONNECTED);
        ///gpio_pin_configure(cfg->pin_rd.port, cfg->pin_rd.pin, GPIO_DISCONNECTED);

		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}



#define SWOW_PIN_CFG(inst, name) \
    {   \
        .label = DT_INST_GPIO_LABEL(inst, name##_gpios),    \
        .pin = DT_INST_GPIO_PIN(inst, name##_gpios),   \
        .flags = DT_INST_GPIO_FLAGS(inst, name##_gpios)   \
    } \


#define SWOW_DEFINE(inst) \
    static struct swow_cfg_t swow_cfg_##inst = \
    { \
        .pin_wr = GPIO_DT_SPEC_INST_GET_BY_IDX(inst, wr_gpios, 0), \
        .pin_rd = GPIO_DT_SPEC_INST_GET_BY_IDX(inst, rd_gpios, 0)     \
    }; \
    DEVICE_DT_INST_DEFINE(  \
        inst,   \
        swow_init,  \
        NULL, NULL, \
        &swow_cfg_##inst,  \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,    \
        &swow_api   \
    );  


DT_INST_FOREACH_STATUS_OKAY(SWOW_DEFINE);