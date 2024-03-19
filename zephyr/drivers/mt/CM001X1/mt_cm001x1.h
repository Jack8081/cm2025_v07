#ifndef _DRIVER_MT_CM001X1_H_
#define _DRIVER_MT_CM001X1_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Em io valid bytes number
 */
#define MT_IO_BYTES_NUM      (8)

int         mt_cm001x1_init(const struct device *mt_dev);
int         mt_cm001x1_ioctl(const struct device *mt_dev, struct mt_ioctl_info *p_mt_info);

#ifdef __cplusplus
}
#endif

#endif
