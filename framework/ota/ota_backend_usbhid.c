
#include <kernel.h>
#include <fs/fs.h>
#include <stream.h>
#include <string.h>
#include <mem_manager.h>
//#include <fs_manager.h>
//#include <logging/sys_log.h>
#include <ota_backend.h>
#include <ota_backend_usbhid.h>

#ifdef CONFIG_BT_TRANSCEIVER
#ifndef CONFIG_HID_INTERRUPT_IN_EP_MPS
#define CONFIG_HID_INTERRUPT_IN_EP_MPS 64
#endif
#endif

#ifdef CONFIG_SPPBLE_DATA_CHAN
static u8_t usbhid_update_mode;
#endif
struct ota_backend_usbhid{
    struct ota_backend backend;
    io_stream_t stream;
    int stream_opened;
    char *data_buf;
    //struct k_sem_ uart_up_sem;
};
extern int usb_hid_exit_func(void);
extern int usb_hid_ep_read(u8_t *data, u32_t data_len, u32_t timeout);

extern int ota_upgrade_hid_read(int offset, void *buf, int size);
extern int ota_upgrade_hid_read_remote(int offset, void *buf, int size);
extern int usb_hid_ep_write(const u8_t *data, u32_t data_len, u32_t timeout);
#ifdef CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND
extern int tr_bt_manager_ble_send_data(uint8_t *data, uint16_t len);
extern int hid_trans_flag;
#endif

#ifdef CONFIG_APP_TRANSMITTER
int ota_backend_usbhid_read_wait_host(bool flag)
{
    u8_t buf[64] = {0};

    buf[0] = 'U';
    buf[1] = 0x60;

    if(flag)
    {
        buf[2] = 0xff;
    }
    else
    {
        buf[2] = 0x00;
    }
#ifdef  CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND
    if(hid_trans_flag)
    {
        return tr_bt_manager_ble_send_data(buf, 3);
    }
#endif

    return usb_hid_ep_write(buf, CONFIG_HID_INTERRUPT_IN_EP_MPS, 5000);
}

static int ota_backend_usbhid_read_host(struct ota_backend *backend, int offset, void *buf, int size)
{
    //printk("offset %x size %x\n",offset,size);

#ifdef  CONFIG_BT_TRANS_BLEDATA_SERVER_FAREND
    if(hid_trans_flag)
    {
        ota_upgrade_hid_read(offset, buf, size);
    }
    else
#endif
    {
        ota_backend_usbhid_read_wait_host(false);
        ota_upgrade_hid_read(offset, buf, size);
        ota_backend_usbhid_read_wait_host(true);
    }
    return 0;
}
#endif

int ota_backend_usbhid_read(struct ota_backend *backend, int offset, unsigned char *buf, int size)
{
    int ret = 0;
#ifdef CONFIG_SPPBLE_DATA_CHAN
    if(usbhid_update_mode == 0)
#endif
#ifdef CONFIG_APP_TRANSMITTER
        ret = ota_backend_usbhid_read_host(backend, offset, buf, size);
#else
        ret = 0;
#endif
#ifdef CONFIG_SPPBLE_DATA_CHAN
    else if(usbhid_update_mode == 1)
    {
        extern int ota_upgrade_hid_remote_read(int offset, void *buf, int size);
        ret = ota_upgrade_hid_remote_read(offset, buf, size);
    }
#else
    ;
#endif

    return ret;
}

int ota_backend_usbhid_open(struct ota_backend *backend)
{
    return 0;
}

int ota_backend_usbhid_close(struct ota_backend *backend)
{
    return 0;
}

void ota_backend_usbhid_exit(struct ota_backend *backend)
{
    struct ota_backend_usbhid *backend_usbhid = CONTAINER_OF(backend, 
            struct ota_backend_usbhid, backend);

    if(backend_usbhid->data_buf)
        mem_free(backend_usbhid->data_buf);

    if(backend_usbhid)
        mem_free(backend_usbhid);

    return;
}

struct ota_backend_api ota_backend_api_usbhid = {
    .init = (void *)ota_backend_usbhid_init,
    .exit = ota_backend_usbhid_exit,
    .open = ota_backend_usbhid_open,
    .close = ota_backend_usbhid_close,
    .read = ota_backend_usbhid_read,
};

struct ota_backend *ota_backend_usbhid_init(ota_backend_notify_cb_t cb,
        struct ota_backend_usbhid_init_param *param)
{
    struct ota_backend_usbhid *backend_usbhid;

    backend_usbhid = mem_malloc(sizeof(struct ota_backend_usbhid));
    if(!backend_usbhid)
    {
        SYS_LOG_ERR("malloc failed");
        return NULL;
    }
    memset(backend_usbhid, 0 , sizeof(struct ota_backend_usbhid));

    if(*param->mode == 0){
        backend_usbhid->data_buf = mem_malloc(64);
        if(!backend_usbhid->data_buf)
        {
            SYS_LOG_ERR("buf malloc fail");
            return NULL;
        }
#ifdef CONFIG_SPPBLE_DATA_CHAN
        usbhid_update_mode = 0;
#endif
    }
    else if(*param->mode == 1)
    {
#ifdef CONFIG_SPPBLE_DATA_CHAN
        usbhid_update_mode = 1;
#endif
    }

    ota_backend_init(&backend_usbhid->backend, OTA_BACKEND_TYPE_USBHID, &ota_backend_api_usbhid, cb);

    return &backend_usbhid->backend;
}
