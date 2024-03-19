/*
*   support ios iap2 protocol
*
*
*/
#include "iap2.h"
#include "tr_usound.h"

static iap2 *iap2_dev;
u8_t *cache_buf =NULL;

int link_dev(void);
#define TIMER_MAX_TIME  0xfffffffful
#define CONFIG_I2C_GPIO_1_NAME "I2C_1"

static u32_t timestamp_ticks_get(void)
{
	return *(volatile u32_t *)0xc0120108;
}

unsigned int timestamp_get_us(void)
{
    unsigned int ts,us;

    ts = TIMER_MAX_TIME - timestamp_ticks_get();

    us = (unsigned long long)ts;

    return us;
}

static void time_delay_ticks(u32_t ticks)
{
    u32_t start = timestamp_get_us();

    while(start - timestamp_get_us() < ticks);
}

void iap2_bulk_out_cbk(void)
{
	if(iap2_dev){
		printk("bulk out\n");
		k_sem_give(&iap2_dev->read_sem);
	}
}
void iap2_bulk_in_cbk(void)
{
	if(iap2_dev){
		printk("bulk in\n");
		k_sem_give(&iap2_dev->write_sem);
	}
}
u8_t checksum_calculation(u8_t *buffer, u16_t start, u16_t len)
{
	u16_t i;
	u8_t sum = 0;

	for(i=start;i<(start+len);i++){
		sum += buffer[i];
	}

	return (u8_t)(0x100 - sum); /*2's complement'*/
}

int iap2_session_control_link(u8_t *buf,  u16_t sessionID, u16_t pktlen)
{
	if(!buf)
	{
		printk("buf null\n");
		return -1;
	}
	struct iap2_session_link_packet *link = (struct iap2_session_link_packet *)buf;

	link->startofPacket = 0xff50;
	link->SessionID = sessionID;
	link->PacketLength = pktlen;
	link->PacketSeqNum =1;
	//link->Payloadcrc =checksum_calculation(buf, 0, sizeof(iap2_session_link_packet));

	return 0;
}
static int iap2_bulk_read(u8_t *buf, u16_t len)
{
	int ret = 0;
	u32_t time = k_uptime_get_32();
	u32_t read;

retry:
	//read authentication req
	ret = usb_read(IAP2_CONTROL_EP_BULK_OUT, iap2_dev->buffer, len, &read);
	if(ret<0 || read <=0){
		printk("get req message fail\n");
		if(k_uptime_get_32() - time >= 10000)
			return -1;
		os_sleep(20);
		goto retry;
	}
	if(read > 0){
		printk("rev %x suc\n",read);
		ret = read;
	}

	return ret;
}

int iap2_control_auth_send(void)
{
	//iap2_session_control iap2_session_control_packet;

	return 0;
}

int iap2_control_seesion_send(void *data, u32_t len)
{
	iap2_session_control iap2_session_cntrol_packet;

	if(!data){
		printk("data null\n");
		return -1;
	}

	iap2_session_control_link((u8_t *)&iap2_session_cntrol_packet, 0x0100, sizeof(iap2_session_control));

	return 0;
}
int iap2_handle_reg(void *handle)
{
	if(!handle)
		return -1;

	return 0;
}
void iap2_handle_unreg(void)
{
	/*if(handle)
		handle = NULL;
		*/
}
void iap2_session_handle(u8_t bmquest, u8_t bmquest_type, u8_t value, u8_t len)
{
	switch(bmquest){
		case IAP2_SESSION_TYPE_CONTROL:
			//iap2_session_control_handl(buf, len);
			break;

		case IAP2_SESSION_TYPE_EXT_ACC_PRO:
			//iap2_session_ext_handle(buf, len);
			break;

		case IAP2_SESSION_TYPE_FILE_TRANS:
			break;

		default:
			break;
	}
}
int iap2_read_cap(u8_t reg_addr, u16_t len, u8_t *buf)
{
	u16_t retry = 0;
	int ret = -1;

	if(iap2_dev->act_i2c){
reWrite:
		ret = i2c_write(iap2_dev->act_i2c, &reg_addr, 0, (reg_addr<<8) |CP_I2C_WR);
		if(ret && (retry++) <=20){
			time_delay_ticks(500);
			goto reWrite;
		}
		else if(retry > 20){
			goto errExit1;
		}else
			retry = 0;
reRead:
		ret = i2c_read(iap2_dev->act_i2c, buf, len, CP_I2C_RR);
		if( !ret){
			return ret;
		}else if(retry <= 30){
			//printk("i2c read error\n");
			time_delay_ticks(500);
			goto reRead;
		}else
			goto errExit1;

	}

errExit1:
	ret = -1;
	return ret;
}
#if 0
static int iap2_write_cap(u8_t reg_addr, u16_t len, u8_t *buf)
{
	u16_t retry = 0;
	int ret = -1;

	if(iap2_dev->act_i2c){
reWrite1:
		ret = i2c_write(iap2_dev->act_i2c, buf, len, (reg_addr<<8) |CP_I2C_WR);
		if(ret && (retry++) <=20){
			time_delay_ticks(500);
			goto reWrite1;
		}
		else if(retry > 20){
			goto errExit2;
		}else if(!ret)
			return 0;
	}
errExit2:
	ret = -1;
	return ret;
}
#endif
static int authen_send(u8_t *buf, u16_t len)
{
	int ret;
	u32_t write_len;

	if(len>512){
 		u16_t write = 0;
		u16_t total = 0;
		while(1){
			if(len>= 64)
				write = 64;
			else
				write = len;
			k_sem_take(&iap2_dev->write_sem, K_FOREVER);
			//print_buffer(buf+total, 1, write, 16, -1);
			ret = usb_write(IAP2_CONTROL_EP_BULK_IN, buf+total, write,&write_len);
			if(ret<0){
				continue;
			}

			total += write;
			len -= write;
			if(!len){
				break;
			}
 			if(ret){
				printk("write fail\n");
				return -1;
			}
		}
	}
	return 0;
 }
#if 0
static int write_challenge(u8_t *buf, u16_t len)
{
	u8_t reg;
	u16_t data;
	u8_t val;
	u16_t length = 0;
	//struct iap2_session_link_packet link;
	//struct Iap2_message_str message;

	//write chal data len
	reg =CP_CHALLENGE_DATA_LEN;
	data = len;
	iap2_write_cap(reg, 2, (u8_t *)&data);

	//write challenge data
	reg =CP_CHALLENGE_DATA;
	iap2_write_cap(reg, len, buf);

	//write auth control
	reg =CP_AUTHENTICATION_CONTROL_STATUES;
	val = 0x01;
	iap2_write_cap(reg, 1, &val);

	//read auth control and result
	iap2_read_cap(reg, 1, &val);
	printk("read auth control %x\n",val);

	//read challenge response data len
	reg =CP_CHALLENGE_RESPONSE_DATA_LEN;
	iap2_read_cap(reg, 2, (u8_t *)&data);
	length = sys_cpu_to_le16(data);
	printk("response len %x\n",length);

	//read response data
	reg = CP_CHALLENGE_RESPONSE_DATA;
	iap2_read_cap(reg, length, iap2_dev->buffer);
	print_buffer(iap2_dev->buffer, 1, length, 16, -1);

	authen_send(iap2_dev->buffer, length);

	return 0;
}
#endif
static int start_authen(void)
{
	int ret;
	struct iap2_session_link_packet link;
	struct Iap2_message_str message;
	struct iap2_session_link_packet *packet;
	struct Iap2_message_str *pmessage;

	if(iap2_dev->act_i2c){
		u8_t reg;
		//first read certification data len 2 btyes
		reg =CP_ACCESSORY_CERTIFICATION_DATA_LEN;
		ret = iap2_read_cap(reg, 2,iap2_dev->buffer);
		if(ret){
			printk("read %x fail\n",reg);
			return -1;
		}else{
			u16_t cer_len = sys_cpu_to_le16(*(u16_t *)iap2_dev->buffer);
			printk("auth cer len %x\n",cer_len);
			//request cer
			cache_buf = mem_malloc(700);
			if(!cache_buf)
			{
				printk("malloc authen buf fail\n");
				return -1;
			}
			//secondry read certification data 608 bytes
			u16_t len =IAP2_LINK_PKT_LEN + sizeof(struct Iap2_message_str) + cer_len -3;
			link.startofPacket = sys_cpu_to_le16(0xff5a);
			link.PacketLength = sys_cpu_to_le16(len);
			link.controlByte =LINK_CONTROL_ACK;
			link.PacketSeqNum = iap2_dev->ack_seq+1;
			link.PacketAckNum = iap2_dev->sync_seq;
			link.SessionID = iap2_dev->session_id;
			link.headercrc = checksum_calculation((u8_t *)&link, 0, 8);
			len = sizeof(struct Iap2_message_str) + cer_len -4;
			message.sofMessage = sys_cpu_to_le16(0x4040);
			message.messageLen =sys_cpu_to_le16(len);
			message.messageId = sys_cpu_to_le16(SESSION_ID_AUTHENTICATION_CERTIFICATE);
			message.ParamLen = sys_cpu_to_le16(cer_len);
			message.ParamID =sys_cpu_to_le16(0x0);
			len = IAP2_LINK_PKT_LEN;
			memcpy(cache_buf, &link, len);
			memcpy(cache_buf+len, &message, sizeof(struct Iap2_message_str) -4);
			len =IAP2_LINK_PKT_LEN + sizeof(struct Iap2_message_str) + cer_len -4;
			reg =CP_ACCESSORY_CERTIFICATION_DATA_1;
			ret = iap2_read_cap(reg, cer_len,cache_buf+IAP2_LINK_PKT_LEN+sizeof(struct Iap2_message_str) -4);
			if(ret){
				printk("read authen data fail %x\n",reg);
				return -1;
			}else{
				*(cache_buf+len) = checksum_calculation(cache_buf, IAP2_LINK_PKT_LEN, len - IAP2_LINK_PKT_LEN);
				len += 1;
				print_buffer(cache_buf, 1, len, 16, -1);
				u16_t mes_id = 0;
				authen_send(cache_buf, len);
				while(1){
					//read certifacation response
					k_sem_take(&iap2_dev->read_sem,K_FOREVER);
					ret = iap2_bulk_read(iap2_dev->buffer, 120);
					if(ret > 0){
						u8_t retry = 0;
						print_buffer(iap2_dev->buffer, 1, ret, 16, -1);
						packet =(struct iap2_session_link_packet *)iap2_dev->buffer;
						pmessage =(struct Iap2_message_str *)(iap2_dev->buffer +IAP2_LINK_PKT_LEN);
						printk("rev %x id\n",sys_cpu_to_le16(pmessage->messageId));
						mes_id =sys_cpu_to_le16(pmessage->messageId);
						if(mes_id == SESSION_ID_AUTH_FAIL){
							reg =CP_DEVICE_ID;
							iap2_read_cap(reg, 4, iap2_dev->buffer);
							printk("device id %x\n",*(u32_t *)iap2_dev->buffer);

							reg =CP_DEV_CERTIFICATION_SERIAL_NUM;
							iap2_read_cap(reg, 32, iap2_dev->buffer);
							printk("cap err code %s\n",iap2_dev->buffer);
						}
						#if 0
						if( mes_id !=SESSION_ID_REQUEST_AUTH_CHALLENG_RESP)
						{
							authen_send(auther_buf, len);
							continue;
						}else if(mes_id == SESSION_ID_REQUEST_AUTH_CHALLENG_RESP){
							//write challenge data
							len =sys_cpu_to_le16(pmessage->messageLen);
							write_challenge(iap2_dev->buffer+IAP2_LINK_PKT_LEN+sizeof(struct Iap2_message_str) -4, len);
							break;
						}
						else if(mes_id == SESSION_ID_AUTH_FAIL){
							reg =CP_DEVICE_ID;
							iap2_read_cap(reg, 4, iap2_dev->buffer);
							printk("device id %x\n",*(u32_t *)iap2_dev->buffer);

							reg =CP_ERROR_CODE;
							iap2_read_cap(reg, 1, iap2_dev->buffer);
							printk("cap err code %x\n",*iap2_dev->buffer);
						}
						#endif
						if(++retry >= 4)
							break;
					}
				}
			}
		}
	}

	return 0;
}
static int iap2_fill_component(u8_t * p)
{
	u16_t length =0;
	u16_t len = 0;
	struct lap2_message_param_str *param;
	u8_t *q = NULL;

	param = (struct lap2_message_param_str *)p;
	//device name
	param->id = 0x0;
	len = 5+strlen(IAP2_DEVICE_NAME);
	param->len = sys_cpu_to_le16(len);
	memcpy(param->data, IAP2_DEVICE_NAME,strlen(IAP2_DEVICE_NAME));
	*(p+len) = '\0';
	length = len;
	p += len;
	param = (struct lap2_message_param_str *)p;
	//mode id
	param->id = sys_cpu_to_le16(0x0001);
	len = 5+ strlen(IAP2_MODEID);
	param->len = sys_cpu_to_le16(len);
	memcpy(param->data, IAP2_MODEID, strlen(IAP2_MODEID));
	*(p+len) = '\0';
	length += len;
	p += len;
	param = (struct lap2_message_param_str *)p;
	//manufatory
	param->id = sys_cpu_to_le16(0x0002);
	len = 5+strlen(IAP2_MANUFATORY);
	param->len = sys_cpu_to_le16(len);
	memcpy(param->data, IAP2_MANUFATORY, strlen(IAP2_MANUFATORY));
	*(p+len) ='\0';
	length += len;
	p += len;
	param = (struct lap2_message_param_str *)p;
	//serial name
	param->id = sys_cpu_to_le16(0x0003);
	len = 5+ strlen(IAP2_SERIAL_NAME);
	param->len = sys_cpu_to_le16(len);
	memcpy(param->data, IAP2_SERIAL_NAME, strlen(IAP2_SERIAL_NAME));
	*(p+len) ='\0';
	length += len;
	p += len;
	param = (struct lap2_message_param_str *)p;
	//firmware version
	param->id = sys_cpu_to_le16(0x0004);
	len = 5+ strlen(IAP2_FIRMWAREVERSION);
	param->len = sys_cpu_to_le16(len);
	memcpy(param->data, IAP2_FIRMWAREVERSION, strlen(IAP2_FIRMWAREVERSION));
	*(p+len) ='\0';
	length += len;
	p += len;
	param = (struct lap2_message_param_str *)p;
	//hardware version
	param->id = sys_cpu_to_le16(0x0005);
	len = 5+strlen(IAP2_HARDWAREVERSION);
	param->len = sys_cpu_to_le16(len);
	memcpy(param->data, IAP2_HARDWAREVERSION, strlen(IAP2_HARDWAREVERSION));
	*(p+len) = '\0';
	length += len;
	p += len;
	//message id rev and send none
	//power to device none
	//power drawn from device
	param = (struct lap2_message_param_str *)p;
	param->id = sys_cpu_to_le16(0x0009);
	param->len = sys_cpu_to_le16(0x06);
	*(u16_t *)(param->data) = sys_cpu_to_le16(0x02);
	length += 6;
	p += 6;
	param = (struct lap2_message_param_str *)p;
	//support language and current language
	param->id = sys_cpu_to_le16(12);
	param->len =sys_cpu_to_le16(0x07);
	param->data = "en";
	*(p + 0x07) = '\0';
	length += 0x07;
	p += 7;
	param = (struct lap2_message_param_str *)p;
	param->id = sys_cpu_to_le16(13);
	param->len =sys_cpu_to_le16(0x07);
	param->data = "en";
	*(p + 0x07) = '\0';
	length += 0x07;
	p += 7;
	param = (struct lap2_message_param_str *)p;
	//uhost component
	param->id = sys_cpu_to_le16(0x0010);
	q = p;
	len = 0;
	p += 4;
	//uhost component id
	param = (struct lap2_message_param_str *)p;
	param->id = 0x0;
	param->len = sys_cpu_to_le16(0x0006);
	*(u16_t *)param->data = 0x0;
	len += 6;
	p += 6;
	param = (struct lap2_message_param_str *)p;
	param->id = sys_cpu_to_le16(0x0001);
	param->len = sys_cpu_to_le16(5+strlen(IAP2_UHOST_COMPONENT));
	memcpy(param->data, IAP2_UHOST_COMPONENT, strlen(IAP2_UHOST_COMPONENT));
	*(p+5+strlen(IAP2_UHOST_COMPONENT)) = '\0';
	len += 5+strlen(IAP2_UHOST_COMPONENT);
	p+=5+strlen(IAP2_UHOST_COMPONENT);
	param = (struct lap2_message_param_str *)p;
	param->id = sys_cpu_to_le16(0x0002);
	param->len = sys_cpu_to_le16(0x0004);
	len += 4;
	p += 4;
	*(u16_t *)q = sys_cpu_to_le16(len);
	length += len;

	//uhost hid component
	param = (struct lap2_message_param_str *)p;
	q = p;
	len = 0;
	param->id =sys_cpu_to_le16(23);
	p += 4;
	param = (struct lap2_message_param_str *)p;
	param->id = 0x0;
	param->len = sys_cpu_to_le16(0x0006);
	*(u16_t *)param->data = sys_cpu_to_le16(0x0001);
	len += 6;
	p += 6;
	param = (struct lap2_message_param_str *)p;
	param->id = sys_cpu_to_le16(0x0001);
	param->len = sys_cpu_to_le16(5+strlen(IAP2_HID_COMPONENT));
	memcpy(param->data, IAP2_HID_COMPONENT, strlen(IAP2_HID_COMPONENT));
	len += 5+strlen(IAP2_HID_COMPONENT);
	p +=  5+strlen(IAP2_HID_COMPONENT);
	param = (struct lap2_message_param_str *)p;
	param->id = sys_cpu_to_le16(0x0002);
	param->len = sys_cpu_to_le16(0x05);
	*(u8_t *)param->data = 0x1;
	len += 5;
	p += 5;
	param = (struct lap2_message_param_str *)p;
	param->id = sys_cpu_to_le16(0x0003);
	param->len = sys_cpu_to_le16(0x0006);
	*(u16_t *)param->data = 0x0;
	len += 6;
	p+= 6;
	param = (struct lap2_message_param_str *)p;
	param->id = sys_cpu_to_le16(0x0004);
	param->len = sys_cpu_to_le16(0x0006);
	*(u16_t *)param->data = 0x1;
	len += 6;
	*(u16_t *)q = sys_cpu_to_le16(len);
	length += len;

	return length;
}
static int iap2_identifer()
{
	u16_t len = 0;
	int ret = 0;

	u8_t *p = cache_buf +IAP2_LINK_PKT_LEN + 6;
	struct iap2_session_link_packet link;
	struct Iap2_message_str message;

	len = iap2_fill_component(p);

	link.startofPacket = sys_cpu_to_le16(0xff5a);
	link.PacketLength = sys_cpu_to_le16(len+IAP2_LINK_PKT_LEN +sizeof(struct Iap2_message_str) -4);
	link.controlByte =LINK_CONTROL_ACK;
	link.PacketSeqNum = iap2_dev->ack_seq+1;
	link.PacketAckNum = iap2_dev->sync_seq;
	link.SessionID = iap2_dev->session_id;
	link.headercrc = checksum_calculation((u8_t *)&link, 0, 8);
	message.sofMessage = sys_cpu_to_le16(0x4040);
	message.messageLen =sys_cpu_to_le16(len +sizeof(struct Iap2_message_str) -4);
	message.messageId = sys_cpu_to_le16(SESSION_ID_IDENTIFICATION_INFO);
	message.ParamLen = sys_cpu_to_le16(len);
	message.ParamID =sys_cpu_to_le16(0x0);

	memcpy(cache_buf, (u8_t *)&link, IAP2_LINK_PKT_LEN);
	memcpy(cache_buf +IAP2_LINK_PKT_LEN, (u8_t *)&message, sizeof(struct Iap2_message_str) -4);
	*(u8_t *)(cache_buf + len + IAP2_LINK_PKT_LEN +sizeof(struct Iap2_message_str) -4)  = checksum_calculation(cache_buf+IAP2_LINK_PKT_LEN, 0, len +sizeof(struct Iap2_message_str) -4 );
	print_buffer(cache_buf, 1, 700, 16, -1);
	authen_send(cache_buf, IAP2_LINK_PKT_LEN + sizeof(struct Iap2_message_str) -4);

	return ret;
}
static int start_identifaction()
{
	int ret = 0;
	u32_t max_len = IAP2_CACHE_LEN;
	struct iap2_session_link_packet *packet;
	struct Iap2_message_str *message;
	u16_t id;

	k_sem_take(&iap2_dev->read_sem,K_FOREVER);
	ret = iap2_bulk_read(iap2_dev->buffer, max_len);
	if(ret > 0){
		print_buffer(iap2_dev->buffer, 1, ret, 16, -1);
		packet =(struct iap2_session_link_packet *)iap2_dev->buffer;
		printk("rev %x data\n",sys_cpu_to_le16(packet->PacketLength));
		message =(struct Iap2_message_str *)(iap2_dev->buffer +IAP2_LINK_PKT_LEN);
		printk("rev %x id\n",sys_cpu_to_le16(message->messageId));
		id =sys_cpu_to_le16(message->messageId);
		//store the num sync req;
		iap2_dev->sync_seq = packet->PacketSeqNum;
		iap2_dev->ack_seq = packet->PacketAckNum;
		iap2_dev->session_id = packet->SessionID;
		if(id == SESSION_ID_START_IDENTIFICATION){
			printk("start send identification\n");
			iap2_identifer();
		}
	}else
		return -1;

	return ret;
}
int detect_identif_dev(void)
{
	int ret = 0;
	u32_t read;
	//u32_t write_len;
	u32_t max_len = IAP2_CACHE_LEN;
	struct iap2_session_link_packet *packet;
	struct Iap2_message_str *message;
	u32_t time;
	u16_t id;

	time = k_uptime_get_32();
	k_sem_take(&iap2_dev->read_sem,K_FOREVER);
retry:
	//read authentication req
	ret = usb_read(IAP2_CONTROL_EP_BULK_OUT, iap2_dev->buffer, max_len, &read);
	if(ret<0 || read <=0){
		printk("get req message fail\n");
		if(k_uptime_get_32() - time >= 10000)
			return -1;
		os_sleep(20);
		goto retry;
	}
	if(read > 0){
		print_buffer(iap2_dev->buffer, 1, read, 16, -1);
		packet =(struct iap2_session_link_packet *)iap2_dev->buffer;
		u8_t check =checksum_calculation(iap2_dev->buffer, IAP2_LINK_PKT_LEN,sys_cpu_to_le16(packet->PacketLength) - IAP2_LINK_PKT_LEN -1 );
		printk("rev %x data %x\n",sys_cpu_to_le16(packet->PacketLength),check);
		message =(struct Iap2_message_str *)(iap2_dev->buffer +IAP2_LINK_PKT_LEN);
		printk("rev %x id\n",sys_cpu_to_le16(message->messageId));
		id =sys_cpu_to_le16(message->messageId);
		//store the num sync req;
		iap2_dev->sync_seq = packet->PacketSeqNum;
		iap2_dev->ack_seq = packet->PacketAckNum;
		iap2_dev->session_id = packet->SessionID;
		if(id !=SESSION_ID_REQUESTAUTHCERTIFICATE){
			printk("no req for auth\n");
		}else{
			start_authen();
			start_identifaction();
		}
	}
	//start identifaction

	return 0;
}
#if 0
static void iap2_session_sync(void)
{
	return;
}
#endif
static void iap2_link_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	return;
	switch(iap2_dev->status){
		case INIT:/*
			if(k_uptime_get_32() - iap2_dev->link_dete_time >= 1000){
				ret = link_dev();
				if(!ret)
					iap2_dev->status = LINK_ESTABLISH;
				else
					iap2_dev->link_dete_time = k_uptime_get_32();
			}
			break;
			*/
		case LINK_ESTABLISH:
		case SYNC:
			link_dev();
			break;

		/*case DEETCET_DEV:
			ret = detect_identif_dev();
			break;
		*/
		default:
			break;
	}

}

int link_dev(void)
{
	u32_t write_len = 0;
	u32_t read_len = 0;
	u32_t ret;
	u32_t time;

	//u16_t head = 0xff55;
	u8_t packet[6] = {0xff, 0x55, 0x02, 0x00, 0xee, 0x10};

	print_buffer(packet, 1, sizeof(packet), 16, -1);
	//usb_write(IAP2_CONTROL_EP_INTE_IN, (u8_t *)&head, 2, &write_len);
	usb_write(IAP2_CONTROL_EP_BULK_IN, packet, 6, &write_len);

	time = k_uptime_get_32();
	k_sem_take(&iap2_dev->read_sem,K_FOREVER);
retry:
	ret = usb_read(IAP2_CONTROL_EP_BULK_OUT, iap2_dev->buffer, 6, &read_len);
	if(read_len == 6)
	{
		print_buffer(iap2_dev->buffer, 1, read_len, 16, -1);
		if(memcmp(iap2_dev->buffer, packet, 6)==0)
			printk("receive iap2 device detect ack\n");
		iap2_dev->status = LINK_ESTABLISH;
	}
	else{
		//return -1;
		if(k_uptime_get_32() - time >= 5000)
			return -1;
		os_sleep(10);
		goto retry;
	}

	return 0;
}

static int iap2_link_sync_send(void)
{
	u8_t ret;
	u32_t write_len = 0;

	struct iap2_link_sync_packet link;
	link.link_packet.startofPacket = sys_cpu_to_le16(0xff5a);
	link.link_packet.PacketLength = sys_cpu_to_le16(sizeof(struct iap2_link_sync_packet));
	link.link_packet.controlByte =LINK_CONTROL_SYN;
	link.link_packet.PacketSeqNum = 0x2b;
	link.link_packet.PacketAckNum = 0x0;
	link.link_packet.SessionID = 0x0;
	link.link_packet.headercrc = checksum_calculation((u8_t *)&link.link_packet, 0, 8);

	link.link_sync_payload.link_version = 0x1;
	link.link_sync_payload.maxnumoutstandingpacket = 0x04;
	link.link_sync_payload.maxnumreceviedpacketlen =sys_cpu_to_le16( 0x0200);
	link.link_sync_payload.retransmit_timeout = sys_cpu_to_le16(0x040b);
	link.link_sync_payload.Cumulativ_ack_Timeout = sys_cpu_to_le16(0x0017);
	link.link_sync_payload.maxnumretransmit = 0x03;
	link.link_sync_payload.maxcumulativackNum = 0x03;
	link.link_sync_payload.sessionID1 = SESSION_SYNC_ID_1;
	link.link_sync_payload.sessionType1 =IAP2_SESSION_TYPE_CONTROL;
	link.link_sync_payload.session_ver1 = 0x1;
	link.link_sync_payload.sessionID2 = SESSION_SYNC_ID_2;
	link.link_sync_payload.sessionType2 = IAP2_SESSION_TYPE_EXT_ACC_PRO;
	link.link_sync_payload.session_ver2 = 0x1;
	link.link_sync_payload.payload_crc = checksum_calculation((u8_t *)&link.link_sync_payload, 0, sizeof(link.link_sync_payload)-1);

	print_buffer(&link, 1, sizeof(link), 16, -1);
	usb_write(IAP2_CONTROL_EP_BULK_IN, (u8_t *)&link, sizeof(struct iap2_link_sync_packet), &write_len);

	iap2_dev->sync_seq = link.link_packet.PacketSeqNum;
	ret = link.link_packet.PacketAckNum;
	return ret;
}

static void iap2_link_ack_send(u8_t reqnum)
{
	u32_t write_len = 0;
	//u32_t read;
	//u32_t time;
	//int ret;
	struct iap2_session_link_packet link;
	u8_t len =sizeof(struct iap2_session_link_packet);

	link.startofPacket = sys_cpu_to_le16(0xff5a);
	link.PacketLength = sys_cpu_to_le16(len);
	link.controlByte =LINK_CONTROL_ACK;
	link.PacketSeqNum = 0x2b;
	link.PacketAckNum = reqnum;
	link.SessionID = 0x0;
	link.headercrc = checksum_calculation((u8_t *)&link, 0, 8);

	print_buffer(&link, 1, sizeof(link), 16, -1);
	usb_write(IAP2_CONTROL_EP_BULK_IN, (u8_t *)&link, len,&write_len );
/*
	time = k_uptime_get_32();
	k_sem_take(&iap2_dev->read_sem,K_FOREVER);
retry:
	ret = usb_read(IAP2_CONTROL_EP_BULK_OUT, iap2_dev->buffer, 16, &read);
	if(ret<0 || read <=0){
		if(k_uptime_get_32() - time >= 5000)
			return;
		os_sleep(20);
		goto retry;
	}
*/
	return;
}
static int iap2_link_ack(void)
{
	u32_t read = 0;
	//int retry = 10;
	int ret = -1;
	u8_t send_seq = 0;
	u32_t time;

	struct iap2_session_link_packet *link;

	send_seq = iap2_link_sync_send();

	time = k_uptime_get_32();
	k_sem_take(&iap2_dev->read_sem,K_FOREVER);
retry:
	ret = usb_read(IAP2_CONTROL_EP_BULK_OUT, iap2_dev->buffer, sizeof(struct iap2_link_sync_packet), &read);
	if(ret<0 || read < sizeof(struct iap2_link_sync_packet)){
		if(k_uptime_get_32() - time >= 5000)
			return ret;
		os_sleep(500);
		goto retry;
	}else{
		link = (struct iap2_session_link_packet *)iap2_dev->buffer;
		print_buffer(iap2_dev->buffer, 1, read, 16, -1);
		printk("seq %x ack %x\n",link->PacketSeqNum,link->PacketAckNum);
		if(link->PacketSeqNum >0){
			iap2_dev->ack_seq = link->PacketSeqNum;
			iap2_link_ack_send(link->PacketSeqNum);
			iap2_dev->status = DEETCET_DEV;
			ret = 0;
		}
	}

	return ret;
}

int iap2_link_establish(void)
{
	return iap2_link_ack();
}

static int iap2_hid(void)
{
	//hid control message


	return 0;
}

static void iap2_rev_thread_handle(void * arg1, void *arg2, void *arg3)
{
	u32_t read_len = 0;
	int ret = 0;
	struct iap2_session_link_packet *link_packet;
	//iap2_session_control *ctl_session;

	os_sleep(1000);
	if(iap2_dev->status == INIT){
		ret = link_dev();
		if(!ret){
			iap2_dev->status =LINK_ESTABLISH;
			ret = iap2_link_establish();
			if(ret){
				printk("%s %d:run here.\n",__func__,__LINE__);
				return;
			}
		}
	}
	if(iap2_dev->status == DEETCET_DEV){
		printk("enter authentication\n");
		ret = detect_identif_dev();
		if(!ret)
			iap2_dev->status = IDENTIF;
	}
	//send hid report
	if(iap2_dev->status == IDENTIF){
		printk("send hid report\n");
		ret = iap2_hid();
	}
	while(1){
		os_sleep(1000);
	}

	while(!iap2_dev->iap2_thread_exit){
		k_sem_take(&iap2_dev->read_sem,K_FOREVER);
		usb_read(IAP2_CONTROL_EP_BULK_OUT, iap2_dev->buffer, 64, &read_len);

		link_packet = (struct iap2_session_link_packet *)iap2_dev->buffer;
		print_buffer(iap2_dev->buffer, 1, sizeof(iap2_dev->buffer), 16, -1);
		if(!link_packet)
			continue;
		if(link_packet->SessionID == 0x10 && link_packet->startofPacket == 0xff5a){
			iap2_dev->status =LINK_ESTABLISH;
			continue;
		}
		switch (link_packet->SessionID){
			case 0x1:  //request auth

				break;
			case 0x2:	//request uac

				break;
			default:
				break;
		}
		os_sleep(20);
	}

	iap2_dev->iap2_thread_exit = 2;
}

int iap2_protocol_init()
{
	int ret;
	//if(iap2_dev)
	//	return -1;

	iap2_dev = mem_malloc(sizeof(iap2));
	if(!iap2_dev){
		printk("iap2 dev malloc fail\n");
		return -1;
	}

	iap2_dev->handle = NULL;
	iap2_dev->buffer = mem_malloc(IAP2_CACHE_LEN);
	if(!iap2_dev->buffer){
		printk("malloc fail");
		return -1;
	}

	iap2_dev->status = INIT;

	thread_timer_init(&iap2_dev->link_timer_handle, iap2_link_timer_handle, NULL);
	thread_timer_start(&iap2_dev->link_timer_handle,20,100);
	iap2_dev->link_dete_time = 0;

	iap2_dev->iap2_thread_stack = mem_malloc(1024);
	if(!iap2_dev->iap2_thread_stack){
		printk("malloc iap2 thread stack fail\n");
		return -1;
	}
	os_thread_create(iap2_dev->iap2_thread_stack, 1024, iap2_rev_thread_handle, 0, 0, 0, 5, 0, OS_NO_WAIT);
	iap2_dev->iap2_thread_exit = 0;

	k_sem_init(&iap2_dev->read_sem, 0, 1);
	k_sem_init(&iap2_dev->write_sem, 0, 1);

	iap2_dev->act_i2c = device_get_binding(CONFIG_I2C_GPIO_1_NAME);
	if(!iap2_dev->act_i2c)
	{
		printk("get i2c1 fail\n");
		return -1;
	}else{
		union dev_config config;
		config.bits.use_10_bit_addr = 0;
		config.bits.is_master_device = 1;
		config.bits.speed =I2C_SPEED_FAST;
		//config.bits.speed =I2C_SPEED_STANDARD;
		ret = i2c_configure(iap2_dev->act_i2c, *(u32_t *)&config);
		if(ret){
			printk("configure i2c fail\n");
			return -1;
		}else{
			printk("configure i2c success\n");
		}
	}

	return 0;
}
void iap2_protocol_deinit()
{
	if(!iap2_dev)
        return;

	iap2_dev->iap2_thread_exit = 1;
	while(iap2_dev->iap2_thread_exit != 2){
		os_sleep(500);
	}

	if(iap2_dev->iap2_thread_stack){
		mem_free(iap2_dev->iap2_thread_stack);
		iap2_dev->iap2_thread_stack = NULL;
	}

	if(cache_buf){
		mem_free(cache_buf);
		cache_buf = NULL;
	}

	if(iap2_dev->buffer){
		mem_free(iap2_dev->buffer);
		iap2_dev->buffer = NULL;
	}

	if(iap2_dev){
		mem_free(iap2_dev);
		iap2_dev = NULL;
	}

}
extern int usb_bulk_transfer(u8_t *buf, u32_t len, u8_t mode, u16_t timeout);

int iap2_detect_enable(void)
{
	int ret = 0;
	u8_t data[16] = {0};
	u8_t iap2_init[16] = {0xff, 0x55, 0x02, 0x00, 0xee, 0x10};
	u32_t read_bytes =0;
	//u8_t i = 0;

	usb_write(0x80,data,6,&read_bytes);
	//usb_bulk_transfer(iap2_init, 6, 1,50);

	//for(i=0;i<3;i++){
	while(1){
		ret = usb_read(0x0, data, 9, &read_bytes);
		//ret =usb_bulk_transfer(data, 6, 0, 50);
		if(ret<=0){
			//printk("no iap2 connect %d\n",read_bytes);
			printk("no iap2 connect\n");
			//if(read_bytes == 6)
			{
				printk("rev %d\n",read_bytes);
				if(!memcmp(data, iap2_init, 16)){
					ret = 1;
					printk("link estab\n");
					break;
				}
			}
			os_sleep(1000);
			usb_write(0x80,data,6,&read_bytes);
			//usb_bulk_transfer(iap2_init, 6, 1,50);
			continue;
		}else{
			printk("rev %d\n",read_bytes);
			if(!memcmp(data, iap2_init, 16)){
				ret = 1;
				break;
			}
		}
	}

	return ret;
}
