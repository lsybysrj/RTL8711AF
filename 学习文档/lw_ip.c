802.11标准：http://baike.baidu.com/link?url=5Tae-tqq7Z6k-MJwXUwnq7L02uQaYkO5Ns76iIRUBJ08-H927pF95RzXsv7m-Wz-MxBRAvuWvOD3G-sVzKIsDK
================================================================================
LW/IP是瑞典计算机科学院（SICS）的Adam Dunkels开发的一个小型的开源的TCP/IP协议栈
说明：是一个轻型的（Light Weight）IP协议
LW/IP协议栈原理：http://blog.sina.com.cn/s/blog_62a85b950101am53.html
================================================================================
简介：
	1、LWIP是Light Weight（轻型）IP协议，有无操作系统的支持都可以运行。LWIP实现的重点是在保持TCP协议主要功能的基础上减少
	对RAM的占用，一般只要几百字节的RAM和40K左右的ROM就可以运行，这使LWIP协议栈适合在低端的嵌入式系统中使用
	2、LWIP协议栈主要关注的是怎样减少对内存的使用和代码的大小，这样就可以使LWIP适用于资源有限的小平台，例如嵌入式系统。为了
	简化处理过程和内存要求，LWIP对API进行了裁剪，可以不需要复制一些数据
----------------------------------------------------------------------------------->
模式分类
	1、RAW API
		1、把协议栈和应用程序放到一个进程里面，该接口基于函数回调技术，使用该接口的应用程序不用进行连续操作。但这会使应用程序
		编写难度加大且代码不易理解。
		2、为了接收数据，应用程序会向协议栈注册一个回调函数，该回调函数与特定的连接相关联，当该关联的连接到达一个信息包，该
		回调函数就会被协议栈调用。
		3、优缺点
			1、优点：应用程序和协议栈驻留在一个进程中，那么发送和接收数据不需要进行进程切换。
			2、缺点：应用程序不能使自己长期陷入运算中，这样会导致通讯新能下降，原因是TCP/IP处理与连续运算不能并行发生，这个缺
			可以通过把应用程序分为两部分来克服，一部分负责通讯，一部分负责处理运算。
	2、lwip API
		1、lwip把接收和处理放在一个线程里面（缺点处理流程稍微延迟，接收就会被阻塞，直接造成频繁丢包、响应不及时等亚种问题），
		因此，接收和处理必须分开
		2、处理丢包现象：网络驱动中，接收部分以任务的形式创建，数据包到达后，去掉以太网包头得到IP包，然后直接调用tcpip_input()
		函数将其投到mbox邮箱，投递结束，接收任务继续下一个数据包的接收，而被投递的IP包将由TCPIP线程继续处理。这样使得某个ip
		包处理时间过长也不会造成频繁的丢包现象发生。
	3、BSD API
		BSD API提供了基于open-read-write-close模型的UNIX标准API，最大特点就是增加可可移植性，但在嵌入式系统中效率比较低，占用资源
		多。对于嵌入式是不能容忍的
--------------------------------------------------------------------------------->
特性：
	1、支持网络接口下的ip转发
	2、支持ICMP协议
	3、包括实验性扩展的UDP（用户数据报协议）
	4、包括阻塞控制、RTT估算、快速恢复和快速转发的TCP（传输控制协议）
	5、提供专门的内部回调接口（Raw API），用于提高应用程序性能
	6、可选择的Berkeley接口API（在多线程情况下使用）
	7、在最新版本中支持APP
	8、新版中增加了IP fragment的支持
	9、支持DHCP协议
--------------------------------------------------------------------------------->
动态内存管理：
	LWIP协议栈使用动态内存堆分配策略，动态内存堆分配策略是在一个事先定义好大小的内存
	块中进行管理，内存的分配策略是采取最快合适（First Fit）方式，只要找到一个比请求
	的内存块大的空闲块，就从中切出合适的块，并把剩余的部分返回到内存堆中。分配的内存
	块有最小大小的额限制，一般是12字节，前几个字节存放内存分配器管理用的私有数据，该
	数据不能被用户程序修改，否则会导致致命问题，内存释放的过程是相反的，但分配器会查看
	该节点前后相邻的内存块是否空闲，如果空闲则合并成一个大的内存空闲块。
	缺点：频繁的动态分配和释放，会导致严重的内存碎片，导致分配失败
	推荐：分配-->释放-->分配-->释放
	1、mem_init()
		内存堆的初始化函数，主要是告知内存堆的起止地址，以及初始化空闲列表，有lwip初始
		化自己时调用，该接口为内部私有化接口，不对用户开放
	2、mem_malloc()
		1、配内存，参数（总共需要的字节数），返回最新分配内存的指针，失败，返回NULL，
		分配的内存受到内存对齐的影响，可能会比申请的大。返回的块没有初始化，需要马上用
		有效数据或0来初始化这块内存。
		2、分配和释放不能在中断函数里面进行，内存堆是全局变量，因此内存的申请、释放操作
		做了线程安全保护，如果有多个线程在同时进行内存的申请和释放，那么可能会因为信号量
		的等待而导致申请时间较长
	3、mem_calloc()
		对mem_malloc()的简单包装，参数(元素的数目和每个元素的大小，两个参数的乘积就是所
		要分配内存的大小)，会把分配的内存清零(memset())。
FreeRTOS使用了动态堆分配和动态内存池两种办法协作，采用了下面的结构体
	struct pbuf {
		struct pbuf *next;
		void *payload;
		u16_t tot_len;
		u16_t len;
		 
		u8_t  type;
		u8_t flags;
		u16_t ref;
	};
	1、一个数据包可能需要多个这样的结构体形成的链表，tot_len是当前len与next指向的tot_len的
	和，最后一个tot_len是和len相等，第一个tot_len包含了整个数据包的长度，
	2、ref表示一个pbuf，引用的次数，只有ref的值为1才能删除成功
	3、pbuf的类型：PBUF_RAM、BUF_ROM、BUF_REF、PBUF_POOL
		PBUF_RAM：通过内存堆分配得到，在协议栈中用的最多
		PBUF_POOL：主要通过内存池分配得到，这种类型可以在极短时间内分配得到，在接收数据包
					时LWIP一般采用这种方式封装数据，分配内存时，协议栈会在内存池中分配适当
					的内存池个数以满足需要申请的大小
					memp_malloc(MEMP_PBUF_POOL);
		PBUF_ROM与PBUF_REF：只申请结构体头，而不申请数据区的空间
pbuf的释放：（pub_free()函数）
	1、只有pbuf-->ref=1时才能被释放，处在链表中间的节点是不能被删除的，能被删除的肯定是链表首
	节点
	2、PBUF_POOL类型和PBUF_ROM类型、PBUF_REF类型需要通过memp_free()函数删除，PBUF_RAM类型需要
	通过mem_free()函数删除，原因不解释。
--------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------
1、LWIP网络接口结构
	1、LWIP从逻辑上分为四层：链路层、网络层、传输层和应用层，各层之间或多或少可以
	交叉存取，减少了拷贝的开销
	2、LWIP中采用netif网络结构体来描述一个硬件网络接口
		struct netif{
			struct netif *next;			//指向下一个netif结构的指针
			struct ip_addr ip_addr;		//ip地址相关配置
			struct ip_addr netmask;
			struct ip_addr gw;
			
			err_t (* input)(struct pbuf *p,struct netif *imp); //调用这个函数
				可以从网卡上取得一个数据包
			err_t (* output)(struct netif *netif,struct pbuf *p,struct ip_addr *ipaddr);
			//IP层调用这个函数可以向网卡发送一个数据包
			err_t (* linkoutput)(struct netif *netif, struct pbuf *p);  
			// ARP模块调用这个函数向网卡发送一个数据包
			
			void *state;   // 用户可以独立发挥该指针，用于指向用户关心的网卡信息
			u8_t hwaddr_len; // 硬件地址长度，对于以太网就是MAC地址长度，为6各字节
			u8_t hwaddr[NETIF_MAX_HWADDR_LEN];   //MAC地址
			u16_t mtu;   // 一次可以传送的最大字节数，对于以太网一般设为1500
			u8_t flags;   // 网卡状态信息标志位			 
			char name[2]; // 网络接口使用的设备驱动类型的种类
			u8_t num;    // 用来标示使用同种驱动类型的不同网络接口
		}
		next字段指向下一个netif结构体的指针，我们一个产品可能有多个网卡芯片，LWIP
		会将所有网卡芯片的结构体链成一个链表进行管理，有一个netif_list的全局变量指向
		链表头，还有一个netif_default全局变量指向缺省的网络接口结构
	3、以太网数据接收
	LWIP中实现了接收一个数据包和发送一个数据包函数的框架，这两个函数分别是low_level_input
	和low_level_output，用户需要使用实际网卡驱动程序完成这两个函数。在第一篇中讲过，一个
	典型的LWIP应用系统包括这样的三个进程：首先是上层应用程序进程，然后是LWIP协议栈进程，
	最后是底层硬件数据包接收进程。
	接收数据函数：void  ethernetif_input(void *arg)   
					//创建该进程时，要将某个网络接口结构的netif结构指	
2、ARP表
	1、ARP协议的核心是ARP缓冲表，ARP的实质就是对缓冲表的建立、更新、查询，ARP的缓冲表是由一个个
	的缓冲表项(entry)组成的，LWIP中描述缓冲表项的数据结构叫etharp_entry
	struct etharp_entry{
		#if ARP_QUEUEING
			struct etharp_q_entry *q;      // 数据包缓冲队列指针
		#endif
		struct ip_addr ipaddr;         // 目标IP地址
		struct eth_addr ethaddr;       //  MAC地址
		enum etharp_state state;       // 描述该entry的状态
		u8_t ctime;                 // 描述该entry的时间信息
		struct netif *netif;            // 相应网络接口信息
	}
	2、LWIP内核通过数组的方式来创建ARP缓存表
		如：static struct etharp_entry arp_table[ARP_TABLE_SIZE];
	3、state：标识缓存表项的状态，使用了枚举
		enum etharp_state{
			ETHARP_STATE_EMPTY = 0,
			ETHARP_STATE_PENDING,
			ETHARP_STATE_STABLE
		}
	4、arp数据类型
	struct etharp_hdr {
		PACK_STRUCT_FIELD(struct eth_hdr ethhdr);    // 14字节的以太网数据报头
		PACK_STRUCT_FIELD(u16_t hwtype);          // 2字节的硬件类型
		PACK_STRUCT_FIELD(u16_t proto);            // 2字节的协议类型
		PACK_STRUCT_FIELD(u16_t _hwlen_protolen);  // 两个1字节的长度字段
		PACK_STRUCT_FIELD(u16_t opcode);          // 2字节的操作字段op
		PACK_STRUCT_FIELD(struct eth_addr shwaddr);  // 6字节源MAC地址
		PACK_STRUCT_FIELD(struct ip_addr2 sipaddr);   // 4字节源IP地址
		PACK_STRUCT_FIELD(struct eth_addr dhwaddr);  // 6字节目的MAC地址
		PACK_STRUCT_FIELD(struct ip_addr2 dipaddr);   // 4字节目的IP地址
	}PACK_STRUCT_STRUCT;
	PACK_STRUCT_FIELD()是防止编译器字对齐的宏定义
	5、ARP表查询
		1、arp欺骗：伪造mac
		2、static s8_t find_entry(struct ip_addr *ipaddr, u8_t flags)
			输入一个ip地址，返回该IP地址对应的ARP缓存表项索引，功能是寻找一个
			匹配的arp表项或者创建一个新的arp表项并且返回该表项的索引号
			1、ipaddr为非空，函数返回一个处于pending或stable的索引表项，若没
			有匹配到该表项，则函数返回一个empty表项，该表项的IP字段被设置为ipaddr
			这种情况下，find_entry函数返回后，需要将表项从empty改为pending，如果
			ipaddr为空值，同样返回一个empty的表项
		3、etharp_query()
			向给定的ip地址发送一个数据包或者发送一个ARP请求
			err_t etharp_query(struct netif *netif, struct ip_addr *ipaddr, struct pbuf *q)
	6、arp缓存表更新
		static err_t update_arp_entry(struct netif *netif, struct ip_addr *ipaddr, struct eth_addr *ethaddr, u8_t flags)
		以太网的帧类型可以是:IP、ARP、PPPOE、wlan等
		ethernet_input用于接收底层硬件传来的数据包，如果是ip包，调用
		etharp_ip_input，如果是ARP包调用etharp_arp_input
		etharp_output：接收IP层的要发送的数据包，并将数据发送出去
3、IP层
	关键点：信息包的接收、分片数据包重组、信息包的发送
	LWIP描述ip数据报头，使用结构体叫ip_hdr：
	struct ip_hdr {
		PACK_STRUCT_FIELD(u16_t _v_hl_tos);  // 前三个字段：版本号、首部长度、服务类型
		PACK_STRUCT_FIELD(u16_t _len);  // 总长度
		PACK_STRUCT_FIELD(u16_t _id);   // 标识字段
		PACK_STRUCT_FIELD(u16_t _offset); // 3位标志和13位片偏移字段
		#define IP_RF 0x8000        //
		#define IP_DF 0x4000        // 不分组标识位掩码
		#define IP_MF 0x2000        // 后续有分组到来标识位掩码
		#define IP_OFFMASK 0x1fff  // 获取13位片偏移字段的掩码
		PACK_STRUCT_FIELD(u16_t _ttl_proto);  // TTL字段和协议字段
		PACK_STRUCT_FIELD(u16_t _chksum);   // 首部校验和字段
		PACK_STRUCT_FIELD(struct ip_addr src);  // 源IP地址
		PACK_STRUCT_FIELD(struct ip_addr dest);  // 目的IP地址
	} PACK_STRUCT_STRUCT;
	LWIP不允许IP数据包头被封装在不同的pbuf里面
	inet_chksum()：完成ip头部校验和
	1、LWIP中ip分片
		用到的数据结构ip_reassdata
		struct ip_reassdata {
			struct ip_reassdata *next;    // 用于构建单向链表的指针
			struct pbuf *p;    // 该数据报的数据链表
			struct ip_hdr iphdr;  // 该数据报的IP报头
			u16_t datagram_len;  // 已经收到的数据报长度
			u8_t flags;  // 是否收到最后一个分片包
			u8_t timer;  // 设置超时间隔
		};
		ip_reass()：数据包重组函数
		ip_reass_chain_frag_into_datagram_and_validate()：对分片数据包（PBUF）进行插入
		ip_forward()：ip数据包转发
			参数：
				要转发的数据包指针
				要转发的数据包的IP报头指针
				收到该数据包的的网络接口数据结构netif指针。
			首先调用ip_route()找到转发该数据包应该使用的网络接口
			
=========================================================================================
***************汇编****************
1、协处理器，ARM处理器最多支持16个协处理器，协处理器指令分为三类
	1、ARM处理器用于初始化协处理器的数据指令CDP
	2、协处理器寄存器与内存单元之间的数据传送指令LDC与STC
	3、ARM处理器寄存器与协处理器寄存器之间的数据传送指令MCR与MRC
2、ARM中有两条异常中断指令
	1、SWI（软中断）：实现在用户态的系统调用
	2、BKPT：在ARM v5之后引入的，主要用来产生软件断点，用于程序调试
3、ARM汇编程序中的语句可以由ARM指令、Thumb指令、伪操作和宏指令组成
	格式：{lable} {instruction | directive | pseudo-instruction} {;comment}
4、ARM中的伪指令
	1、符号定义伪指令（不分配内存单元）
		定义全局变量：GBLA、GBLL、GBLS
		定义局部变量：LCLA、LCLL、LCLS
		对变量赋值：SETA、SETL、SETS
		为通用寄存器列表定义名称的伪操作：RLIST
	2、数据定义伪指令（分配内存单元）
		DCB：分配一段字节类型的内存单元（-）
		DCW/DCWU：分配一段半字类型的内存单元
		DCD/DCDU：分配一段字类型的内存单元（&）
		DCI：分配一段字类型的内存单元并初始化
		SPACE：分配一块内存单元，并初始化为0（%）
		MAP：定义一个结构化内存表的首地址（^）
		FIFLD：用于定义一个结构化内存表中的数据域（#）
		LTORG：声明一个数据缓冲池的开始
	3、汇编控制伪指令
		IF,ELSE,ENDIF
		WHILE,WEND
		MACRO,MEND,MEXIT
	4、信息报告伪指令
	5、宏定义伪操作
	6、其他伪操作
		CODE16，CODE32：高速编译器后面应该使用的指令集
		EQU：宏定义
			count EQU 0x2000
		ALIGN：对齐方式（必须为2的整数次幂）
=========================================================================================
嵌入式系统分类：
	1、应用于面向控制、工业应用等实时性要求很高的场合（Vxworks、Nucleus等）
	2、面向消费电子（Plam OS、Windows CE、Symbian、Android等）
实时操作系统可以分为抢占和非抢占两大类
μC/OS-II可以管理多达64个任务，每隔任务相当于一个线程，整个操作系统看成一个进程
=========================================================================================
网卡相关函数（ethernetif.c）
	static void low_level_init(struct netif *netif)
		功能：网卡初始化，完成网卡复位及参数初始化
	static err_t low_level_output(struct netif *netif,struct pbuf *p)
		功能：网卡数据包发送函数，将内核结构pbuf猫叔的数据包发送出去
	static struct pbuf * low_level_input(struct netif * netif)
		功能：网卡数据包接收函数，必须将接收的数据包封装成pbuf的形式
	static void ethernetif_input(struct netif * netif)
		功能：调用网卡的数据包接收函数low_level_input从网卡处读取一个数据包，然后解析该数据包的
			  类型（ARP或IP包），最后将数据包交给上层
	err_t ethernetif_init(struct netif * netif)
		功能：上层在管理网络接口结构netif会自动调用，最终调用low_level_init完成网卡初始化

LWIP动态内存管理：（lwip为用户提供了两种最基本的内存你关了机制：动态内存池和动态内存堆）
	1、内存分配函数类型
		1、从高地址的空闲块中进行分配，不去理会已经分配给用户的内存区域是否已经释放，
		当分配无法进行时，系统采取检查以前分配给用户的空间是否已经释放，同时系统将
		已经释放的空间重新组织成一个大的可用空闲块，以满足用户的内存分配请求
		2、一旦用户运行结束并释放内存空间，系统便将该空间标志为空闲，每当有用户提出
		空间分配申请时，系统将以此遍历整个系统中的空闲块，找出一个系统认为可行的空
		闲块给用户（常见）
	2、常见内存分配策略
		1、系统规定用户在申请空间时，申请大小必须为指定的值（如4,8,16等）
		   系统初始化时事先在内存中初始化相应的空闲内存块空间，分配和释放就是简单的
		   链表fetch和add，不需要查找（lwip实现了内存池分配策略）
		2、存储紧缩操作
		3、可变长的内存分配（类似哈希链表）
	3、与内存池相关的函数
		memp_init():内存池初始化函数，在内核初始化时必须调用
		memp_malloc():通常被内核调用
		memp_free()
	4、与内存堆相关的3个函数
		mem_init()
		mem_malloc()
		mem_free()
		plug_holes():内存堆节点合并有关
		
网络接口层：
重点：
	网络接口管理的作用
	网络接口结构netif
	回环接口的概念及作用
	基于回环接口的实验程
1、概念
	1、所有的网络接口结构形成一个netif_list链表，每隔接口的结构注册了对应的操作函数
2、网络接口结构（netif.c和netif.h）

=====================================================================
国际协议IP
=====================================================================
重点：
	1、ip地址的分类，特殊ip地址
	2、子网划分、子网掩码、NAT等概念
	3、ip层数据报结构以及数据输入处理
	4、IP层数据报的发送及分片操作
	5、IP分片数据的重载过程
1、概述
	1、ip地址分类（A B C D E F）
		A类地址：0开头，网络号占8位
		B类地址：10开头，网络号占16位，主机号占16位
		C类地址：110开头，网络号占24位，主机号占8位
		D类地址：1110开头，是多播地址（224.xxx.xxx.xxx）
		E类地址：11111开头，保留未用
	2、判断一个网络地址是否为D类多播地址
		#define ip_addr_ismulticast(addr1) \
		(((addr1)->addr & PP_HTONL(0xf0000000UL)) == PP_HTONL(0xe0000000UL))
	   判断一个网络地址是否为广播地址
	   #define ip_addr_isbroadcast(ipaddr, netif) \
	   ip4_addr_isbroadcast((ipaddr)->addr, (netif))
2、数据报
	1、ip层的数据报官方名字：IP数据报（或IP分组）
	2、由两部分组成：ip首部和数据
	   首部长度：20~60字节，可以包含长达40字节的选项字段
	3、描述首部的数据结构
		PACK_STRUCT_BEGIN //禁止编译器自动对齐
		struct ip_hdr {
		  /* version / header length */
		  PACK_STRUCT_FIELD(u8_t _v_hl);
		  /* type of service */
		  PACK_STRUCT_FIELD(u8_t _tos);
		  /* total length */
		  PACK_STRUCT_FIELD(u16_t _len);
		  /* identification */
		  PACK_STRUCT_FIELD(u16_t _id);
		  /* fragment offset field */
		  PACK_STRUCT_FIELD(u16_t _offset);
		  #define IP_RF 0x8000U        /* reserved fragment flag */
		  #define IP_DF 0x4000U        /* dont fragment flag */
		  #define IP_MF 0x2000U        /* more fragments flag */
		  #define IP_OFFMASK 0x1fffU   /* mask for fragmenting bits */
		  /* time to live */
		  PACK_STRUCT_FIELD(u8_t _ttl);
		  /* protocol*/
		  PACK_STRUCT_FIELD(u8_t _proto);
		  /* checksum */
		  PACK_STRUCT_FIELD(u16_t _chksum);
		  /* source and destination IP addresses */
		  PACK_STRUCT_FIELD(ip_addr_p_t src);
		  PACK_STRUCT_FIELD(ip_addr_p_t dest); 
		} PACK_STRUCT_STRUCT;
	
===================================================================
用户编程接口
===================================================================
重点：
	1、协议栈定时机制、定时事件的执行、协议栈内核进程
	2、协议栈消息机制
	3、协议栈编程接口，即sequential API的实现、编程函数与编程示例
	4、套接字接口，即socket API的实现、编程函数与编程示例
1、定时事件
	1、定时结构
		1、LWIP的定时完全是基于软件方式来模拟的，这种模拟方式基于操作系统
		提供的邮箱和信号量机制
	2、定时链表
		LWIP通过数据结构sys_timeo来记录一个定时事件，内核中所有的定时
		事件对应的sys_timeo结构会被顺序的组织在系统定时链表上
			struct sys_timeo {
			  struct sys_timeo *next; //指向下一个定时事件的指针
			  u32_t time;			//当前定时事件需要等待的事件
			  sys_timeout_handler h;//指向定时函数，超时后该函数会被系统回调执行
			  void *arg;			//传给定时函数的参数
			#if LWIP_DEBUG_TIMERNAMES
			  const char* handler_name;
			#endif /* LWIP_DEBUG_TIMERNAMES */
			};
			//定时链表首部结构
			//Realtek add 
			struct sys_timeouts {
			  struct sys_timeo *next;  //指向第一个定时结构的指针
			};
		
			向内核注册一个定时事件
			void sys_timeout(u32_t msecs, 
							sys_timeout_handler handler,
							void *arg)
			@1：事件定时时间
			@2：处理事件函数
			@3：向handler传递的参数
			
			void sys_untimeout(sys_timeout_handler handler, void *arg)
			从定时链表中删除一个定时事件
	3、内核进程
		在协议栈初始化函数tcpip_init()中，内核集成被创建
		void tcpip_init(tcpip_init_done_fn initfunc, void *arg);
		static void tcpip_thread(void *arg)
		//内核进程会一直阻塞在一个邮箱上等待消息处理
		void sys_timeouts_mbox_fetch(sys_mbox_t *mbox, void **msg)
	4、处理定时事件
2、消息机制
	1、系统消息是通过结构tcpip_msg来描述的
	2、协议栈的API由两部分组成
		1、作为用户编程接口函数提供给用户，这些函数在用户进程中执行
		2、驻留在协议栈内核进程中
		上面两部分通过进程间通信机制（IPC）实现通信和同步，
		共同为应用程序提供服务
	3、被用到的进程间通信机制包括三种
		1、邮箱：例如内核邮箱mbox、连接上接收数据的邮箱recvmbox
		2、信号量：例如op_completed,用于两部分API的同步
		3、共享内存，例如内核消息结构tcpip_msg、API消息内容api_msg等
	4、函数
		err_t tcpip_apimsg(struct api_msg *apimsg)
			功能：向内核投递消息
			参数：记录 API消息的具体内容
		sys_mbox_post(&mbox, &msg);
			功能：投递消息
		sys_arch_sem_wait(&apimsg->msg.conn->op_completed, 0);
			功能：等待消息处理完毕
3、协议栈接口
	1、用户数据缓冲netbuf
		1、文件netbuf.c和netbuf.h包含了所有与用户数据缓冲netbuf相关的结构和函数
		2、netbuf是基于pbuf实现的
			struct netbuf {
			  struct pbuf *p, *ptr;
			  ip_addr_t addr;
			  u16_t port;
			#if LWIP_NETBUF_RECVINFO || LWIP_CHECKSUM_ON_COPY
			#if LWIP_CHECKSUM_ON_COPY
			  u8_t flags;
			#endif /* LWIP_CHECKSUM_ON_COPY */
			  u16_t toport_chksum;
			#if LWIP_NETBUF_RECVINFO
			  ip_addr_t toaddr;
			#endif /* LWIP_NETBUF_RECVINFO */
			#endif /* LWIP_NETBUF_RECVINFO || LWIP_CHECKSUM_ON_COPY */
			};
			1、字段p指向pbuf链表，真正保存数据
			2、ptr也指向pbuf链表
			3、p一直指向链表中的第一个pbuf，ptr可能指向链表中的其它位置，源文档把
			它描述为fragment pointer，与他密切相关的函数netbuf_next,netbuf_first
			4、netbuf_fromaddr和netbuf_fromport分别用于返回netbuf结构中的ip地址和端口号
	2、数据缓冲操作
		1、netbuf是应用程序描述代发数据和已接收数据的基本结构
		2、数据缓冲操作函数
			struct netbuf *netbuf_new(void)
				功能：申请一个新的netbuf空间（部分配数据空间即不指向pbuf），真正的
				数据存储区域需要通过调用函数netbuf_alloc来分配
			void netbuf_delete(struct netbuf *buf)
				功能：释放一个netbuf结构空间
			void * netbuf_alloc(struct netbuf *buf, u16_t size)
				功能：为netbuf分配size大小的结构空间
			void netbuf_free(struct netbuf *buf)
				功能：释放netbuf结构指向的数据pbuf
			err_t netbuf_ref(struct netbuf *buf, const void *dataptr, u16_t size)
				功能：和netbuf_alloc相似，区别是不会分配数据空间，将pbuf的payload
				指针指向数据地址dataptr
			void netbuf_chain(struct netbuf *head, struct netbuf *tail)
				功能：将tail中的pbuf连接到head中的pbuf，并删除tail
			err_t netbuf_data(struct netbuf *buf, void **dataptr, u16_t *len)
				功能：将netbuf结构中ptr记录的pbuf数据起始地址回填dataptr，并将
				该pbuf中的数据长度填入到len中
			s8_t netbuf_next(struct netbuf *buf)
				功能：将netbuf结构中的ptr指向pbuf链表中的下一个pbuf结构
			void netbuf_first(struct netbuf *buf)
				功能：将netbuf结构的ptr指针指向第一个pbuf，即p字段指向的pbuf
	3、连接结构netconn
		/** Protocol family and type of the netconn */
		//描述连接类型
		enum netconn_type {
		  NETCONN_INVALID    = 0,
		  /* NETCONN_TCP Group */
		  NETCONN_TCP        = 0x10,
		  /* NETCONN_UDP Group */
		  NETCONN_UDP        = 0x20,
		  NETCONN_UDPLITE    = 0x21,
		  NETCONN_UDPNOCHKSUM= 0x22,
		  /* NETCONN_RAW Group */
		  NETCONN_RAW        = 0x40
		};
		/** Current state of the netconn. Non-TCP netconns are always
		 * in state NETCONN_NONE! */
		//描述连接状态，主要在tcp连接中使用
		enum netconn_state {
		  NETCONN_NONE,
		  NETCONN_WRITE,
		  NETCONN_LISTEN,
		  NETCONN_CONNECT,
		  NETCONN_CLOSE
		};
		//函数指针类型，回调函数
		/** A callback prototype to inform about events for a netconn */
		typedef void (* netconn_callback)(struct netconn *, enum netconn_evt, u16_t len);
		//连接结构netconn
		/** A netconn descriptor */
		struct netconn {
		  /** type of the netconn (TCP, UDP or RAW) */
		  enum netconn_type type;
		  /** current state of the netconn */
		  enum netconn_state state;
		  /** the lwIP internal protocol control block */
		  union {
			struct ip_pcb  *ip;
			struct tcp_pcb *tcp;
			struct udp_pcb *udp;
			struct raw_pcb *raw;
		  } pcb;
		  /** the last error this netconn had */
		  err_t last_err;
		  /** sem that is used to synchroneously execute functions in the core context */
		  sys_sem_t op_completed;
		  /** mbox where received packets are stored until they are fetched
			  by the netconn application thread (can grow quite big) */
		  sys_mbox_t recvmbox;
		#if LWIP_TCP
		  /** mbox where new connections are stored until processed
			  by the application thread */
		  sys_mbox_t acceptmbox; //内核会将所有新建立的tcp连接结构netconn投递到
		  //邮箱
		#endif /* LWIP_TCP */
		  /** only used for socket layer */
		#if LWIP_SOCKET
		  int socket;
		#endif /* LWIP_SOCKET */
		#if LWIP_SO_SNDTIMEO
		  /** timeout to wait for sending data (which means enqueueing data for sending
			  in internal buffers) */
		  s32_t send_timeout;
		#endif /* LWIP_SO_RCVTIMEO */
		#if LWIP_SO_RCVTIMEO
		  /** timeout to wait for new data to be received
			  (or connections to arrive for listening netconns) */
		  int recv_timeout;
		#endif /* LWIP_SO_RCVTIMEO */
		#if LWIP_SO_RCVBUF
		  /** maximum amount of bytes queued in recvmbox
			  not used for TCP: adjust TCP_WND instead! */
		  int recv_bufsize;
		  /** number of bytes currently in recvmbox to be received,
			  tested against recv_bufsize to limit bytes on recvmbox
			  for UDP and RAW, used for FIONREAD */
		  s16_t recv_avail;//数据邮箱recvmbox中已经缓冲的数据长度
		#endif /* LWIP_SO_RCVBUF */
		  /** flags holding more netconn-internal state, see NETCONN_FLAG_* defines */
		  u8_t flags;
		#if LWIP_TCP
		  /** TCP: when data passed to netconn_write doesn't fit into the send buffer,
			  this temporarily stores how much is already sent. */
		  size_t write_offset;
		  //当调用netconn_write发送数据但缓冲不足时
		  //数据会被暂时封装在current_msg中，等待下一次发送，write_offset记录下一次发送
		  //时的索引，TCP在周期性处理函数poll中会处理，或者当tcp在该连接上
		  //成功发送数据后，内核会再次尝试发送字儿写未发送数据
		  /** TCP: when data passed to netconn_write doesn't fit into the send buffer,
			  this temporarily stores the message.
			  Also used during connect and close. */
		  struct api_msg_msg *current_msg;
		#endif /* LWIP_TCP */
		  /** A callback function that is informed about events for this netconn */
		  netconn_callback callback;
		  //连接相关的回调函数，实现socket API时使用到
		};
	4、协议栈回调函数接口
	5、协议栈API函数
		1、头文件在api.h中，函数实现在api_lib.c中
		2、函数实现
			struct netconn* netconn_new_with_proto_and_callback(enum netconn_type t, u8_t proto, netconn_callback callback)
				功能：函数本质是一个宏，为新连接申请一个连接结构netconn空间
				参数：常用值NETCONN_TCP和NETCONN_UDP
				备注：不会创建任何连接，只是初始化netconn的相关字段
			err_t netconn_delete(struct netconn *conn)
				功能：删除一个连接结构netconn
			err_t netconn_getaddr(struct netconn *conn, ip_addr_t *addr, u16_t *port, u8_t local)
				功能：获得一个连接结构netconn中的预案IP地址和源端口号
				或目标ip地址和目标端口号
			err_t netconn_bind(struct netconn *conn, ip_addr_t *addr, u16_t port)
				功能：将一个连接结构与本地IP地址addr（IP_ADDR_ANY代表任何一个网路接口的ip地址）
				和端口号port绑定，作为服务器端程序，执行这一步是必须的
			err_t netconn_connect(struct netconn *conn, ip_addr_t *addr, u16_t port)
				功能：连接服务器，将连接结构与目标IP地址和目的端口号PORT
				进行绑定，当作为TCP客户端程序时，调用该函数会导致连接握手过程产生
			err_t netconn_disconnect(struct netconn *conn)
				功能：与服务器断开连接
			err_t netconn_listen_with_backlog(struct netconn *conn, u8_t backlog)
				功能：只在TCP服务器程序中使用，将连接结构netconn设置为监听状态，
				为了接受新连接，do_listen会创建邮箱acceptmbox，并向TCP控制块中
				注册回调函数accept_function,当TCP控制块上有新连接出现时，回调
				函数被执行，它会向连接的acceptmbox邮箱中发送消息，以此通知应用程
				序新连接的到来
			err_t netconn_accept(struct netconn *conn, struct netconn **new_conn)
				功能：用于TCP服务器的函数，服务器调用此程序可以从acceptmbox
				邮箱中获得一个新建立的连接，若邮箱为空，则一直阻塞，直到
				新连接的到来，服务器端调用该函数必须先调用netconn_listen
				将连接状态设置为监听状态
				返回值：返回新连接netconn结构的地址
			err_t   netconn_recv(struct netconn *conn, struct netbuf **new_buf);
				功能：从连接的recvmbox邮箱中接收数据包，可用于TCP连接，也
				可用于UDP连接，函数会一直阻塞，直到邮箱中获得数据消息
				如果从邮箱中获得一条空消息，表示对方已经关闭当前连接
				应用程序也应该关闭这个无效连接
			err_t   netconn_send(struct netconn *conn, struct netbuf *buf);
				功能：在已建立的UDP连接上发送数据
			err_t	netconn_write(struct netconn *conn,const void * dataptr,size_t size,u8_t apiflags)
				功能：在tcp连接上发送数据，dataptr和size分别指出了带发送
				数据的起始地址和长度，不要求用户将数据封装在netbuf中，对于
				数据长度也没有限制，内核直接处理这些数据，将它们封装在pbuf中，
				并挂载到TCP的发送队列中
			err_t   netconn_close(struct netconn *conn);
				功能：关闭一个tcp连接
	
	
	
	
	
	
	
	