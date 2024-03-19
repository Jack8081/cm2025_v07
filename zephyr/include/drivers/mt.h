#ifndef _DRIVER_MAGIC_TUNE_H_
#define _DRIVER_MAGIC_TUNE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MAGIC_TUNE_CM001X1
#define CONFIG_MT_DEV_NAME  "mt_cm001x1"
#else 
#define CONFIG_MT_DEV_NAME  "mt_null"
#endif

#if IS_ENABLED(CONFIG_I2C_0)
#define CONFIG_MT_I2C_NAME  CONFIG_I2C_0_NAME 
#else
#define CONFIG_MT_I2C_NAME  CONFIG_I2C_1_NAME
#endif

/**
 * Definitions for ioctl type
*/
typedef enum 
{
    MT_IOW_BLUK_0,
    MT_IOW_BLUK_1,
    MT_IOR_BLUK_0,
} mt_ioctl_type_t;

struct mt_ioctl_info
{
    uint8_t     type;       /* refer to @mt_ioctl_type_t */
    uint8_t     size;
    uint8_t     *pbulk;
};

struct mt_device_data 
{
    struct device          *i2c_dev;
    //struct mt_driver_state  state;
};

typedef int (*mt_api_init)(const struct device *dev);
typedef int (*mt_api_ioctl)(const struct device *dev, struct mt_ioctl_info *p_mt_info);

struct mt_driver_api
{
    mt_api_init     api_init;
    mt_api_ioctl    api_ioctl;
};

/* MT APIS */
int         mt_init(const struct device *dev);
int         mt_ioctl(const struct device *dev, struct mt_ioctl_info *p_mt_info);

#ifdef __cplusplus
}
#endif

#endif
