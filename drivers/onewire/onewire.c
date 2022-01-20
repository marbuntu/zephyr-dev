


#include "mlx_onewire.h"
#include "mlx_onewire-def.h"
#include "mlx_uart-onewire-hal.h"


#include <logging/log.h>
LOG_MODULE_REGISTER(mlx_onewire, LOG_LEVEL_DBG);


typedef struct _uow_searchConfig_t {
    uint8_t id;
    uint8_t cmpl;
    uint8_t bit_no;
    uint8_t lastDeviceFlag;
    uint8_t lastDiscrepancy;
    uint8_t lastFamilyDiscrepancy;
    uint8_t lastZero;
    uint8_t searchDirection;
    uint8_t ROM_NO[8];
    uint8_t ROM_CRC;
} mlx_ow_searchConfig_t;


static unsigned char dscrc_table[] = {
        0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
       35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
       70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
       17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
       50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
       87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
unsigned char docrc8(unsigned char value, char seed)
{
   return  dscrc_table[seed ^ value];
}

int mlx_ow_search(const struct device *dev, mlx_ow_searchConfig_t *cfg) {

    cfg->bit_no = 1;
    cfg->lastZero = 0;
    cfg->ROM_CRC = 0x00;

    int rom_byte_no = 0;
    uint8_t rom_byte_mask = 1;
    int search_result = 0;

    uint8_t search_cmd = UOW_REG_FIND_NORMAL;

    //uint8_t b_id[100] = { 0 };
    //uint8_t b_cp[100] = { 0 };
    //size_t bptr = 0;

    // Proceed if not last Device
    if (!cfg->lastDeviceFlag) {

        if (!uow_com_init(dev)) {
            cfg->lastDeviceFlag = false;
            cfg->lastDiscrepancy = 0;
            cfg->lastFamilyDiscrepancy = 0;

            LOG_ERR("No reaction on Init");

            return false;
        }

        // Send Search Command
        uow_hal_write(dev, &search_cmd, 1);

        do {

            cfg->id = uow_hal_read_bit(dev);
            cfg->cmpl = uow_hal_read_bit(dev);

            //b_id[bptr] = cfg->id;
            //b_cp[bptr] = cfg->cmpl;
            //bptr++;

            if ((cfg->id == 1) && (cfg->cmpl == 1)) {
                LOG_ERR("Fail during search (ByteNo: %d, Mask: %02x)", rom_byte_no, rom_byte_mask);
                break;
            } 
            else {

                if (cfg->id != cfg->cmpl) {     // All Nodes habe 0 or 1 
                    cfg->searchDirection = cfg->id;
                    
                }
                else { // Discrepancy

                    if (cfg->bit_no < cfg->lastDiscrepancy) {
                        cfg->searchDirection = ((cfg->ROM_NO[rom_byte_no] & rom_byte_mask) > 0);
                    }
                    else {
                        cfg->searchDirection = (cfg->bit_no == cfg->lastDiscrepancy);
                    }

                    if (cfg->searchDirection == 0) {
                        cfg->lastZero = cfg->bit_no;

                        if (cfg->lastZero < 9) {
                            cfg->lastFamilyDiscrepancy = cfg->lastZero;
                        }
                    }
                }

                // Set or Clear Bit
                if (cfg->searchDirection == 1) {
                    cfg->ROM_NO[rom_byte_no] |= rom_byte_mask;
                }   
                else {
                    cfg->ROM_NO[rom_byte_no] &= ~rom_byte_mask;
                }

                k_sleep(K_MSEC(1));
                uow_hal_write_bit(dev, cfg->searchDirection);
                

                cfg->bit_no++;
                rom_byte_mask <<= 1;

                if (rom_byte_mask == 0) {
                    cfg->ROM_CRC = docrc8(cfg->ROM_NO[rom_byte_no], cfg->ROM_CRC);
                    rom_byte_no++;
                    rom_byte_mask = 1;
                }
            }
        } while (rom_byte_no < 8);
    
        // Add CRC check!
        if (!((cfg->bit_no < 65) || (cfg->ROM_CRC != 0))) {
            cfg->lastDiscrepancy = cfg->lastZero;

            if (cfg->lastDiscrepancy == 0) {
                cfg->lastDeviceFlag = true;
            }

            search_result = true;
        }
    }

    if (!search_result || !cfg->ROM_NO[0]) {
        cfg->lastDiscrepancy = 0;
        cfg->lastDeviceFlag = false;
        cfg->lastFamilyDiscrepancy = 0;
        search_result = false;
    }

    //LOG_HEXDUMP_DBG(b_id, bptr, "ID bits: ");
    //LOG_HEXDUMP_DBG(b_cp, bptr, "CP bits: ");

    return search_result;
}


int mlx_ow_first(const struct device *dev, mlx_ow_searchConfig_t *cfg) {

    cfg->lastDiscrepancy = 0;
    cfg->lastDeviceFlag = false;
    cfg->lastFamilyDiscrepancy = 0;

    return mlx_ow_search(dev, cfg);
}

int mlx_ow_next(const struct device *dev, mlx_ow_searchConfig_t *cfg) {

    return mlx_ow_search(dev, cfg);

}


size_t mlx_ow_findAll(const struct device *dev, mlx_ow_node_t **sensors, size_t len) {

    mlx_ow_searchConfig_t scfg;
    int ret = mlx_ow_first(dev, &scfg);

    if (ret == false) {
        LOG_ERR("Search failed");
        return 0;
    }

    size_t cnt = 1;

    sensors[0] = k_malloc(sizeof(mlx_ow_node_t));
    memcpy(sensors[0]->id, scfg.ROM_NO, 8);

    LOG_HEXDUMP_DBG(scfg.ROM_NO, sizeof(scfg.ROM_NO), "First: ");
    LOG_DBG("CRC: %02x", scfg.ROM_CRC);

    while(!scfg.lastDeviceFlag) {
        mlx_ow_next(dev, &scfg);
        LOG_HEXDUMP_DBG(scfg.ROM_NO, sizeof(scfg.ROM_NO), "Next: ");
        LOG_DBG("CRC: %02x", scfg.ROM_CRC);

        if (ret == false) {
            LOG_ERR("Next Search failed");
            break;
        }

        /*
        if (cnt < len) {
            sensors[cnt] = k_malloc(sizeof(mlx_ow_node_t));
            memcpy(sensors[cnt]->id, scfg.ROM_NO, 8);
        }
        */

        cnt++;
    }

    return cnt;
}

bool mlx_ow_test(const struct device *dev) {
    return uow_com_init(dev);
}

bool mlx_DS18B20_convert(const struct device *dev, uint8_t ROM[8]) {

    char cmd = UOW_REG_MATCH_ROM;

    if (!uow_com_init(dev)) {
        LOG_INF("No Node response!");
        return false;
    }

    uow_hal_write(dev, &cmd, 1);    // Send Match ROM Command
    uow_hal_write(dev, ROM, 8);     // Send ROM 

    cmd = UOW_REG_CONVERT;
    uow_hal_write(dev, &cmd, 1);    // Send Convert Command


    return true;
}


int16_t mlx_DS18B20_read(const struct device *dev, uint8_t ROM[8]) {

    char cmd = UOW_REG_MATCH_ROM;
    uint8_t pad[9] = { 0 };

    if (!uow_com_init(dev)) {
        LOG_INF("No Node response!");
        return false;
    }

    uow_hal_write(dev, &cmd, 1);
    uow_hal_write(dev, ROM, 8);
    
    cmd = UOW_REG_READ_SCRATCHPAD;
    uow_hal_write(dev, &cmd, 1);
    
    uow_hal_read(dev, pad, 9);

    //LOG_HEXDUMP_INF(pad, 9, "PAD: ");

    uint8_t bp = (1 << 3);
    uint8_t sb = 0;

    do {
        sb += (pad[1] & bp ? 1 : 0);    // Check if at least 3 sign bis are set
        bp <<= 1;
    } while (bp > 0);


    int sign = (sb >= 3 ? -1 : 1);      // Set sign accordingly

    float temp =  (sign * (((pad[1] & 0x07) << 8) + pad[0])) / 16.0;

    if ((temp < -60) || (temp > 130)) {
        return -999;
    }

    return ((int16_t) (temp * 100));
}