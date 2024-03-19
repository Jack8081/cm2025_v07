/**
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <drivers/dsp.h>
#include <soc_dsp.h>
#include "dsp_inner.h"
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

struct dsp_message pagemiss_message;
struct device *pagemiss_dev = NULL;


int dsp_acts_register_message_handler(struct device *dev, dsp_message_handler handler)
{
    struct dsp_acts_data *dsp_data = dev->data;
    unsigned int irq_key;

    irq_key = irq_lock();
    dsp_data->msg_handler = handler;
    irq_unlock(irq_key);

    return 0;
}

int dsp_acts_unregister_message_handler(struct device *dev)
{
    struct dsp_acts_data *dsp_data = dev->data;
    unsigned int irq_key;

    irq_key = irq_lock();
    dsp_data->msg_handler = NULL;
    irq_unlock(irq_key);

    return 0;
}
int dsp_acts_send_message_response(struct device *dev, struct dsp_message *message, int res)
{
    const struct dsp_acts_config *dsp_cfg = dev->config;
    //struct dsp_acts_data *dsp_data = dev->data;
    struct dsp_protocol_mailbox *mailbox = dsp_cfg->cpu_mailbox;
    uint32_t status = MSG_STATUS(mailbox->msg);

    /* FIXME: force set result fail */
    if (res < 0)
        message->result = DSP_FAIL;

    /* confirm message received */
    status &= ~(MSG_FLAG_BUSY | MSG_FLAG_DONE | MSG_FLAG_RPLY | MSG_FLAG_FAIL);
    status |= MSG_FLAG_ACK;
    switch (message->result) {
    case DSP_REPLY:
        /* always copy back result */
        mailbox->param1 = message->param1;
        mailbox->param2 = message->param2;
        status |= MSG_FLAG_RPLY;
    case DSP_DONE:
        status |= MSG_FLAG_DONE;
        break;
    case DSP_FAIL:
        status |= MSG_FLAG_FAIL;
        break;
    default:
        break;
    }
    mailbox->msg = MAILBOX_MSG(message->id, status);
    //if (message->id != DSP_MSG_REQUEST_FREQADJ) {
    //    printk("dsp_acts_send_message_response id %d status %x  \n",message->id, status);
    //}
    return res;

}

static void dsp_page_miss_handle(struct k_work *work)
{
    int res = dsp_acts_handle_image_pagemiss(pagemiss_dev, pagemiss_message.param1);

    dsp_acts_send_message_response(pagemiss_dev, &pagemiss_message, res);
}

K_WORK_DEFINE(dsp_page_miss_work, dsp_page_miss_handle);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
extern int dsp_acts_freqadj_set(struct device *dev, int flag);
#endif

/* This function called in irq context */
int dsp_acts_recv_message(struct device *dev)
{
    const struct dsp_acts_config *dsp_cfg = dev->config;
    struct dsp_acts_data *dsp_data = dev->data;
    struct dsp_protocol_mailbox *mailbox = dsp_cfg->cpu_mailbox;
    struct dsp_message message = {
        .id = MSG_ID(mailbox->msg),
        .result = DSP_DONE,
        .owner = mailbox->owner,
        .param1 = mailbox->param1,
        .param2 = mailbox->param2,
    };
    uint32_t status = MSG_STATUS(mailbox->msg);
    int res = 0;

    if (dsp_data->pm_status == DSP_STATUS_POWEROFF){
        printk("%s: deaded\n", __func__);
        return -ENODEV;
    }

    if (!(status & MSG_FLAG_BUSY)) {
        printk("%s: busy flag of msg (%u:%u) not set\n", __func__,
               message.owner, message.id);
        return -EINVAL;
    }

    if (status & MSG_FLAG_ACK) {
        printk("%s: ack of msg (%u:%u) already set\n", __func__,
               message.owner, message.id);
        return -EINVAL;
    }
#if 0
    printk("msg_id %d param %x %x\n", message.id, message.param1, message.param2);
#endif
    switch (message.id) {
    case DSP_MSG_REQUEST_BOOTARGS:
        message.param1 = mcu_to_dsp_address((uint32_t)&dsp_data->bootargs, DATA_ADDR);
        message.result = DSP_REPLY;
        break;
    case DSP_MSG_REQUEST_FREQADJ:
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
#ifdef DSP_DYNAMIC_FREQ_ADJ_ENABLE
        res = dsp_acts_freqadj_set(dev, message.param1);
#endif
#endif
        break;
    case DSP_MSG_STATE_CHANGED:
        switch (message.param1) {
        case DSP_TASK_STARTED:
            printk("%s: started\n", __func__);
            if (message.param2 == DSP_NEED_SYNC_CLOCK) {
                dsp_data->need_sync_clock = 'C';
                message.param1 = DSP_NEED_SYNC_CLOCK;
                message.param2 = 0;
                message.result = DSP_REPLY;
            }
            break;
        case DSP_TASK_SUSPENDED:
            printk("%s: suspended\n", __func__);
            break;
        case DSP_TASK_RESUMED:
            printk("%s: resumed\n", __func__);
            break;
        default:
            break;
        }

        k_sem_give(&dsp_data->msg_sem);
        break;
    case DSP_MSG_PAGE_MISS:
#ifdef CONFIG_LOAD_IMAGE_FROM_FS
        pagemiss_dev = dev;
        memcpy(&pagemiss_message, &message, sizeof(struct dsp_message));
        k_work_submit(&dsp_page_miss_work);
        return res;
#else
        res = dsp_acts_handle_image_pagemiss(dev, message.param1);
#ifdef CONFIG_SOFT_BREAKPOINT_DUMP_MEMORY
        if (res == -EACCES) {
            printk("pagemiss breakpoint\n");
            return res;
        }
#endif
        break;
#endif
    case DSP_MSG_PAGE_FLUSH:
        res = dsp_acts_handle_image_pageflush(dev, message.param1);
        break;

    case DSP_MSG_NULL:
        break;
    case DSP_MSG_KICK:
    default:
        if (dsp_data->msg_handler) {
            res = dsp_data->msg_handler(&message);
        } else {
            printk("%s: unexpected msg %u\n", __func__, message.id);
            res = -ENOMSG;
        }
        break;
    }

    res = dsp_acts_send_message_response(dev, &message, res);

    return res;
}

static int wait_ack_timeout(struct dsp_protocol_mailbox *mailbox, int usec_to_wait)
{
    do {
        uint32_t status = MSG_STATUS(mailbox->msg);
        if ((status & MSG_FLAG_ACK) || (usec_to_wait-- <= 0))
            break;

        k_busy_wait(1);
    } while (1);

    return usec_to_wait;
}

int dsp_acts_send_message(struct device *dev, struct dsp_message *message)
{
    struct dsp_acts_data *dsp_data = dev->data;
    const struct dsp_acts_config *dsp_cfg = dev->config;
    struct dsp_protocol_mailbox *mailbox = dsp_cfg->dsp_mailbox;
    uint32_t status = MSG_STATUS(mailbox->msg);

    if (dsp_data->pm_status == DSP_STATUS_POWEROFF)
        return -EFAULT;

    if (!(status & MSG_FLAG_ACK)) {
        printk("%s: ack of msg (%u:%u) not yet set by dsp\n", __func__,
                mailbox->owner, MSG_ID(mailbox->msg));
        return -EBUSY;
    }

    if (status & MSG_FLAG_BUSY) {
        printk("%s: busy flag of msg (%u:%u) not yet cleared by dsp\n",
                __func__, mailbox->owner, MSG_ID(mailbox->msg));
        return -EBUSY;
    }

    if (k_is_in_isr()) {
        printk("%s: send msg (%u:%u) in isr\n", __func__,
               message->owner, message->id);
    } else {
        k_mutex_lock(&dsp_data->msg_mutex, K_FOREVER);
    }

    mailbox->msg = MAILBOX_MSG(message->id, MSG_FLAG_BUSY);
    mailbox->owner = message->owner;
    mailbox->param1 = message->param1;
    mailbox->param2 = message->param2;

    /* trigger irq to dsp */
    mcu_trigger_irq_to_dsp();

    /* Wait for the ack bit: 1ms timeout (1us * 1000) */
    wait_ack_timeout(mailbox, 1000);

    /* de-trigger irq to dsp */
    mcu_untrigger_irq_to_dsp();

    status = MSG_STATUS(mailbox->msg);
    if (!(status & MSG_FLAG_ACK)) {
        printk("%s: ack of msg %u wait timeout\n", __func__, message->id);
        message->result = DSP_NOACK;
        return -ETIMEDOUT;
    } else if (status & MSG_FLAG_FAIL) {
        message->result = DSP_FAIL;
        return -ENOTSUP;
    } else if (status & MSG_FLAG_RPLY) {
        message->param1 = mailbox->param1;
        message->param2 = mailbox->param2;
        message->result = DSP_REPLY;
    } else if (status & MSG_FLAG_DONE) {
        message->result = DSP_DONE;
    } else {
        message->result = DSP_INPROGRESS;
    }

    if (!k_is_in_isr())
        k_mutex_unlock(&dsp_data->msg_mutex);

    return 0;
}

