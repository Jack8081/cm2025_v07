
#define DEBUG_CPU_GPIO_SHIFT		0
#define DEBUG_CPU_GPIO_NUM			16
#define DEBUG_CPU_GPIO_MASK			(((1 << DEBUG_CPU_GPIO_NUM) - 1) << DEBUG_CPU_GPIO_SHIFT)

#define DEBUG_BT_GPIO_SHIFT			(DEBUG_CPU_GPIO_NUM)
#define DEBUG_BT_GPIO_NUM			16
#define DEBUG_BT_GPIO_MASK			(((1 << DEBUG_BT_GPIO_NUM) - 1) << DEBUG_BT_GPIO_SHIFT)

//0~15分配给cpu和dsp
#define DEBUG_CPU_GPIO_PCM_IN		0
#define DEBUG_CPU_GPIO_PCM_OUT		1
#define DEBUG_CPU_GPIO_PCM_OUT_LOST	2

#define DEBUG_DSP_GPIO_DECODE		12
#define DEBUG_DSP_GPIO_ENCODE		13
#define DEBUG_DSP_GPIO_PRE_DEAL		14
#define DEBUG_DSP_GPIO_POST_DEAL	15

//16~31分配给bt
#define DEBUG_BT_GPIO_PREPARE_DATA	16
#define DEBUG_BT_GPIO_RECIEV_DATA	17
#define DEBUG_BT_GPIO_TX_TIME		18
#define DEBUG_BT_GPIO_RX_TIME		19
#define DEBUG_BT_GPIO_APS_FOR_APP	20


void debug_gpio_high(int index);
void debug_gpio_low(int index);
void debug_gpio_display(int index, unsigned int val, int bits, int base_status);
void debug_gpio_enable(bool enable, int bt_mask, unsigned int valid_debug_signal, unsigned char *debug_pin);

