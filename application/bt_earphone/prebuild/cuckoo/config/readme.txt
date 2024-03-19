cfg_mic.bin     
	mic准入工具生成的mic软件增益，可以通过config工具通话调节->声学参数1 导入，算法通过mic准入工具测试之后会有这个文件输出，
	然后再导入到固件里面来。
	
alcfg.bin 
    算法前后处理音效参数，和config工具“通话调节”栏的aec等参数，包含了normal的音效参数
	包含了config工具链通话调节和音效参数的默认配置参数
	可以通过config工具通话调节->声学参数2导入

ffv16.bin       声学参数3：双mic降噪参数：ffv16.bin，算法部门提供，可以通过config工具通话调节->声学参数3 导入
polar.bin       声学参数4：双mic降噪参数：polar.bin，算法部门提供，可以通过config工具通话调节->声学参数4 导入
	
ainr1.bin      
	AI降噪模型库，算法部门提供，可以通过config工具通话调节->声学参数5导入	
	
extcfg.bin
    扩展音效参数bin，config工具中要使能 音频->使能外部音效 为是，用户在音效调节一栏中可以增加额外的音效
	这个bin文件会包含用户设置的所有音效类型的参数，应用进入到某个场景可以选择使用其中的一个音效参数。
	
usrcfg.bin      基于默认的配置用户修改的所有的非算法配置参数所在的bin

configal.bin    config工具“音效调节”和“通话调节”栏对应xml转换成的bin文件
defcfg.bin      config.txt.c转换生成的默认配置


系统配置文件加载是在函数int system_app_config_init(void)中加载
     /* 需要先加载默认配置文件
     * 文件存放于 ROM 中时可以不使用缓存
     */
    app_config_load("defcfg.bin", NO, NO);

    /* 然后再加载用户配置文件
     */
    app_config_load("usrcfg.bin", NO, NO);

    /* 加载算法音效配置文件
     */
    app_config_load("alcfg.bin", NO, NO);

    /* 最后加载拓展配置文件（自定义音效）
     */
    app_config_load("extcfg.bin", NO, NO);

应用程序通过app_config_read 去读取配置时会在这些文件中去找不同的配置值。




