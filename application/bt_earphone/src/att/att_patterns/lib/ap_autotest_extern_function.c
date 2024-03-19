/*
 * Copyright (c) 2021 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: wuyufan<wuyufan@actions-semi.com>
 *
 * Change log:
 *	2021/2/17: Created by wuyufan.
 */

#include "att_pattern_test.h"

struct att_env_var *self;

extern void uart_puts(char *s, unsigned int len);


//new
#if 0

void printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	self->api->vprintk(fmt, ap);
	va_end(ap);

	return ;
}

#else

#define _SIGN     1  /*  has sign character */
#define _ZEROPAD  2  /*  has 0 character prefix */
#define _LARGE    4  /*  use large case */


#define _putc(_str, _end, _ch)  \
	do {                        \
		if (_str < _end) {      \
		*_str = _ch;            \
		_str++;                 \
		}                       \
	}                           \
	while (0)

/**
 * \brief               output variable parameters to a size limit character buffer
 *
 * \param buf           output buffer address
 * \param size          output buffer size
 * \param linesep       windows line end \r\n, or unix line end \n
 * \param fmt           format string
 * \param args          variable parameters
 *
 * \return              output character count, it must be NOT more than param size
 */
int att_vsnprintf(char *buf, size_t size, unsigned int linesep, const char *fmt,
		va_list args)
{
	char *str = buf;
	char *end = buf + size - 1;

	if (end < buf - 1)
		end = (char *)-1;

	for (; *fmt != '\0'; fmt++) {
		uint32_t flags;
		int width;
		int precision;

		uint32_t number;
		uint32_t base;

		char num_str[16];
		int num_len;
		int sign;

		if (*fmt != '%') {
			if (linesep == LINESEP_FORMAT_WINDOWS) {
				/* windows must put \r before \n */
				if (*fmt == '\n')
					_putc(str, end, '\r');
			}
			_putc(str, end, *fmt);
			continue;
		}

		fmt++;

		flags = 0;
		width = 0;
		precision = 0;
		base = 10;

		if (*fmt == '0') {
			flags |= _ZEROPAD;
			fmt++;
		}

		while (isdigit(*fmt)) {
			width = (width * 10) + (*fmt - '0');
			fmt++;
		}

		if (*fmt == '.') {
			fmt++;

			while (isdigit(*fmt)) {
				precision = (precision * 10) + (*fmt - '0');
				fmt++;
			}

			if (width < precision)
				width = precision;

			flags |= _ZEROPAD;
		}

		switch (*fmt) {
		case 'c':
		{
			unsigned char ch = (unsigned char)va_arg(args, int);

			_putc(str, end, ch);
			continue;
		}

		case 's':
		{
			char *s = va_arg(args, char *);

			if (s == NULL) {
				char *rodata_s = "<NULL>";

				s = rodata_s;
			}

			while (*s != '\0') {
				if (linesep == LINESEP_FORMAT_WINDOWS) {
					if ((*s == '\r')
							&& (*(s + 1) != '\n')) {
						/* windows must put \r before \n */
						_putc(str, end, '\r');
						_putc(str, end, '\n');
						s++;
					} else {
						_putc(str, end, *s);
						s++;
					}
				} else {
					_putc(str, end, *s);
					s++;
				}
			}

			continue;
		}

		case 'X':
			flags |= _LARGE;

		case 'x':
		case 'p':
			base = 16;
			break;

			/* integer number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;

		case 'd':
		case 'i':
			flags |= _SIGN;

		case 'u':
			break;

		case '%':
			_putc(str, end, '%');
			continue;

		default:
			continue;
		}

		number = va_arg(args, uint32_t);

		sign = 0;
		num_len = 0;

		if (flags & _SIGN) {
			if ((int)number < 0) {
				number = -(int)number;

				sign = '-';
				width -= 1;
			}
		}

		if (number == 0) {
			num_str[num_len++] = '0';
		} else {
			const char *digits = "0123456789abcdef";

			while (number != 0) {
				char ch = digits[number % base];

				if (flags & _LARGE)
					ch = toupper(ch);

				num_str[num_len++] = ch;
				number /= base;
			}
		}

		width -= num_len;

		if (!(flags & _ZEROPAD)) {
			while (width-- > 0)
				_putc(str, end, ' ');
		}

		if (sign != 0)
			_putc(str, end, sign);

		if (flags & _ZEROPAD) {
			while (width-- > 0)
				_putc(str, end, '0');
		}

		while (num_len-- > 0)
			_putc(str, end, num_str[num_len]);
	}

	if (str <= end)
		*str = '\0';

	else if (size > 0)
		*end = '\0';

	return (str - buf);
}

int att_printf(const char *fmt, ...)
{
	int output_len;
	char debug_buf[DEBUG_BUF_SIZE];
	va_list args;

	va_start(args, fmt);
	output_len = att_vsnprintf(debug_buf, DEBUG_BUF_SIZE,
			LINESEP_FORMAT_WINDOWS, fmt, args);
	va_end(args);

	uart_puts(debug_buf, output_len);

	return output_len;
}

#endif


#ifdef __UVISION_VERSION
int  $Sub$$__2printf(const char *fmt, ...)
#else
int printf(const char *fmt, ...)
#endif
{
	va_list ap;

	va_start(ap, fmt);
	self->api->vprintk(fmt, ap);
	va_end(ap);

	return 0;
}


int property_get(const char *key, char *value, int value_len)
{
	return self->api->property_get(key, value, value_len);
}

int property_set(const char *key, char *value, int value_len)
{
	return self->api->property_set(key, value, value_len);
}

int property_set_factory(const char *key, char *value, int value_len)
{
	return self->api->property_set_factory(key, value, value_len);
}

int stub_write_packet(uint16_t opcode, uint32_t op_para, uint8_t *data_buffer, uint32_t data_len)
{
    return self->api->stub_write_packet(opcode, op_para, data_buffer, data_len);
}

int stub_read_packet(uint16_t opcode, uint32_t op_para, uint8_t *data_buffer, uint32_t data_len)
{
    return self->api->stub_read_packet(opcode, op_para, data_buffer, data_len);
}

int stub_status_inquiry(void)
{
    return self->api->stub_status_inquiry();
}

int read_atf_sub_file(u8_t *dst_addr, u32_t dst_buffer_len, const u8_t *sub_file_name, s32_t offset, s32_t read_len, atf_dir_t *sub_atf_dir)
{
	return self->api->read_atf_sub_file(dst_addr, dst_buffer_len, sub_file_name, offset, read_len, sub_atf_dir);
}

void *malloc(size_t size)
{
	return self->api->malloc(size);
}

void free(void *ptr)
{
	self->api->free(ptr);
}

void k_sleep(s32_t duration)
{
	self->api->k_sleep(duration);
}

u32_t k_uptime_get(void)
{
	return self->api->k_uptime_get();
}

void udelay(u32_t delay_val)
{
	self->api->udelay(delay_val);
}

void mdelay(unsigned int msec)
{
    msec <<= 1;
	while (msec-- != 0){
		udelay(500);
    }
}