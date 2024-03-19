#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <math.h>

#define MUL32_32_RS(a,b,c)		(int)(((long long)(a)*(long long)(b))>>(c)) //result 32bit

#define COEF_Q_BITS	(23) //
#define SHIFT_LQ    (23)
#define FIXED_ONE_32		0x1FFFFFFF
#define FIXED_ONE_EXP_32	29


typedef int		I32;
typedef short	I16;
typedef unsigned int	U32;
typedef unsigned short	U16;


static int my_exp(int a)
{
	int exp = 0;
	if(a == 0)
	{
		return 31;
	}
	else if(a > 0)
	{
		while(a > 0)
		{
			exp++;
			a <<= 1;
		}
	}
	else
	{
		while(a < 0)
		{
			exp++;
			a <<= 1;
		}
	}
	exp--;
	return exp;
}

static I32 ACT_pow10_int32(I32 arg_in, I16 arg_exp_in, I16 *arg_exp_out)
{
	I32 pow10_res_fractional;
	I32 arg, arg_exp;
	I32 arg_integer, arg_fraction;
	I32 a1, a2, a3, a4, a5, a6, a7, a_l2_10;
	I32 order;
	arg=arg_in;
	arg_exp=(I16)arg_exp_in;
	a_l2_10= 0x35269e0f;
	arg = ((long long)arg*a_l2_10)>>(FIXED_ONE_EXP_32+3);
	arg_exp = arg_exp + 2;
	a1= 0x162e42fe;
	a2= 0x07afef7f;
	a3= 0x0e35846b;
	a4= 0x13b21c5e;
	a5= 0x15d85300;
	a6= 0x14826737;
	a7= 0x1024ab8c;
	arg_fraction=arg;
	arg_integer = 0;
	if(arg_exp>0)
	{
		arg_integer = (long long)((long long)arg<<arg_exp)>>29;
		arg_fraction= (long long)((long long)arg<<arg_exp)& FIXED_ONE_32;
	}
	if(arg_exp<0)
	{
		arg_fraction = arg >>(-arg_exp);
	}
	pow10_res_fractional = ((long long)arg_fraction * a7)   >>(FIXED_ONE_EXP_32+3);
	pow10_res_fractional += a6;
	pow10_res_fractional = ((long long)pow10_res_fractional*arg_fraction)>>(FIXED_ONE_EXP_32+3);
	pow10_res_fractional += a5;
	pow10_res_fractional = ((long long)pow10_res_fractional*arg_fraction)>>(FIXED_ONE_EXP_32+3);
	pow10_res_fractional += a4;
	pow10_res_fractional = ((long long)pow10_res_fractional*arg_fraction)>>(FIXED_ONE_EXP_32+3);
	pow10_res_fractional += a3;
	pow10_res_fractional = ((long long)pow10_res_fractional*arg_fraction)>>(FIXED_ONE_EXP_32+3);
	pow10_res_fractional += a2;
	pow10_res_fractional = ((long long)pow10_res_fractional*arg_fraction)>>(FIXED_ONE_EXP_32);
	pow10_res_fractional += a1;
	pow10_res_fractional = ((long long)pow10_res_fractional*arg_fraction)>>(FIXED_ONE_EXP_32);
	pow10_res_fractional += FIXED_ONE_32;
	order = 0;
	if (pow10_res_fractional < 0)
	{
		order = 31 - my_exp(-pow10_res_fractional);		// quantity significant bit
	}
	if (pow10_res_fractional > 0)
	{
		order = 31 - my_exp(pow10_res_fractional);	   // quantity significant bit
	}
	if (order != 0)
	{
		pow10_res_fractional = pow10_res_fractional<<(31-order);
		arg_integer = arg_integer+order-29;
	}
	*arg_exp_out = (I16)arg_integer;
	return (pow10_res_fractional);
}

static int actPOW10(int in, int inQ, int outQ) {
	int man_in, man_out;
	short exp, exp_in, exp_out;
	int ret;

	exp = my_exp(in);
	man_in = in << (exp);
	exp_in = 31 - exp - inQ;
	man_out = ACT_pow10_int32(man_in, exp_in, &exp_out);
	ret = man_out >> (31 - outQ - exp_out);
	return ret;
}



void anc_gain_convert(int gain, int *buf)
{
	//20log(coef) =gain, coef*b0,b1,b2
	int off = 2;
	int b;
	int coef;
	int tmp;
	//gain:【-200，200】  0.1

    if(gain == 0){
        return;
    }

	coef = actPOW10((gain << SHIFT_LQ) / (200), SHIFT_LQ, COEF_Q_BITS);
	b = *(buf+off);
	tmp = MUL32_32_RS(coef, b, COEF_Q_BITS);
	*(buf + off) = tmp;

	b = *(buf + off+1);
	tmp = MUL32_32_RS(coef, b, COEF_Q_BITS);
	*(buf + off+1) = tmp;

	b = *(buf + off+2);
	tmp = MUL32_32_RS(coef, b, COEF_Q_BITS);
	*(buf + off+2) = tmp;
}



// int main(void) {

// 	anc_param_convert(20,0);

// }
