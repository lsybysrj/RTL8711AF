CMSIS-DAP：http://blog.sina.com.cn/s/blog_4680937f0101ss1z.html
	crotex micorcontroller software interface standard
	crotex微处理器软件接口标准
	用于调试的，主要用在keil/IAR的IDE里面使用，没有单独的应用程序，仅适合crotex-M/A7
SDIO:安全数字输入输出接口
NFC：http://baike.so.com/doc/4731200-4946094.html
	1、其基础是RFID（near field communication：近场通信），短距离高频的无线电技术
	2、NFC采用主动和被动两种读取模式
	3、工作频率为13.56MHZ
PMU：电源管理单元
DMA：直接内存读取
UART：通用异步收发传输器
RTK：实时差分
I2S：（Inter-IC Sound）总线，又称集成电路内置音频总线，是飞利浦公司为数字音频设备之间的音频
		数据传输而定制的一种总线标准，该总线专门负责音频设备之间的数据传输，广泛应用于多媒体
		系统，标准的I2S总线电缆由3根串行导线组成（1、时分多路复用-TDM 2、字选择线 3、时钟线）
SRAM：Static RAM，静态随机存储器，它是一种静止存取功能的内存，不需要刷新电路及能保存它内部存
	储的数据（在此的代码不能被执行，在工程建立之后会生成.map文件用来检查内存）
DRAM：动态随机存储器，每隔一段时间需要刷新充电一次，否则内部数据会丢失，SRAM性能高，个头大		
SDRAM：同步动态随机存储器（仅RTL8195AM和RTL88711AM支持2	MBSDRAM，要使用SDRAM在IAR
		中直接将代码拖至SDRAM，重新编译工程）		
==============================================================
1、资料不断跟新中
	链接：http://pan.baidu.com/s/1eQI26K6 密码：3gg9
2、通用描述
	1、高整合的支持802.11无线网络控制的低功耗的单片机
	2、联合了ARM-crotex-M3（MCU）、无线MAC、1T1R的无线基带、RF
	3、整合了内部内存去完成wifi协议功能，嵌入的内存也提供简单应用的开发
3、特征
	1、一般特征
		package QFN48（6x6mm）
		集成CMOS MAC，基带物理层协议、RF在单片机上去支持802.11b/g/n，兼容无线
		实现了802.11为2.4GHZ波段的解决方案
		使用20MHZ带宽支持72.2Mbps的接收和发送速率
		使用40MHZ带宽支持150Mbps的接收和发送速率
		兼容802.11无线协议
		工作在802.11n模式兼容802.11协议
	2、标准特征
		兼容802.11b/g/n无线协议
		802.11e Qos增强
		802.11i（WPA,WPA2）,开放、共享一对key认证服务
		WIFI WPS支持
		直接支持WIFI
		轻量级TCP/IP协议
	3、无线MAC特征
		聚集帧提高MAC效率
		物理层以欺骗的形式增强向下的兼容性
		电源存储机制
	4、无线物理层特征
		802.11n OFDM
		一路发送和一路接收
		20MHZ和40MHZ带宽传输
		短间隔的监视
		DSSS with DBPSK and DQPSK,CCK modulation with long ang short preamble
		最大数据传输速率54Mbps（802.11g）、150Mbps（802.11n）
		交直流切换
		增强的快速接收器控制
	5、外围接口
		SDIO
		最大数字2：高速UART接口的传输速率至4MHZ
		1：UART以标准速率支持
		最大数字3：I2C接口
		I2S以8/16/24/32/48/96/44.1/88.2KHz采样速率
		最大数字2PCM以8/16KHz采样
		最大数字2SPI支持板载速率至41.5MHz
		支持4路PWM去配置循环任务的周期（0~100%）
		支持4路外部定时器触发事件，去配置周期性的低电模式
		支持21路GPIO引脚
4、硬件
	1、硬件功能
		1MB flash
		1MB ROM
		CPU：4/10/20/41/83/166MHz
		PMU:3.3V输入，1.2v输出
		12 channel GDMA
	4、内存分布
		0x0000_0000~0x000F_FFFF:ROM(1M)
		0x1000_0000~0x1006_FFFF:SRAM(BD SRAM and Buffer SRAM share 484k)
		0x1FFF_0000~0X1FFF_FFFF:TCM(高耦合内存) SRAM
		
		
		
		
		
		
		
		
		
		
		
		