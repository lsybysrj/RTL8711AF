/*
    FreeRTOS V6.0.4 - Copyright (C) 2010 Real Time Engineers Ltd.

    ***************************************************************************
    *                                                                         *
    * If you are:                                                             *
    *                                                                         *
    *    + New to FreeRTOS,                                                   *
    *    + Wanting to learn FreeRTOS or multitasking in general quickly       *
    *    + Looking for basic training,                                        *
    *    + Wanting to improve your FreeRTOS skills and productivity           *
    *                                                                         *
    * then take a look at the FreeRTOS eBook                                  *
    *                                                                         *
    *        "Using the FreeRTOS Real Time Kernel - a Practical Guide"        *
    *                  http://www.FreeRTOS.org/Documentation                  *
    *                                                                         *
    * A pdf reference manual is also available.  Both are usually delivered   *
    * to your inbox within 20 minutes to two hours when purchased between 8am *
    * and 8pm GMT (although please allow up to 24 hours in case of            *
    * exceptional circumstances).  Thank you for your support!                *
    *                                                                         *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public 
    License and the FreeRTOS license exception along with FreeRTOS; if not it 
    can be viewed here: http://www.freertos.org/a00114.html and also obtained 
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/*
    Implements a simplistic WEB server.  Every time a connection is made and
    data is received a dynamic page that shows the current TCP/IP statistics
    is generated and returned.  The connection is then closed.

    This file was adapted from a FreeRTOS lwIP slip demo supplied by a third
    party.
*/

/* ------------------------ System includes ------------------------------- */


/* ------------------------ FreeRTOS includes ----------------------------- */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* ------------------------ lwIP includes --------------------------------- */
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "netif/loopif.h"
#include <lwip_netconf.h>
/* ------------------------ Project includes ------------------------------ */
#include <string.h>
#include "main.h"

#include "webserver.h"
#include "wlan_intf.h"
#include "public.h"


#define CONFIG_READ_FLASH	1


#ifdef CONFIG_READ_FLASH

#ifndef CONFIG_PLATFORM_8195A

#include <flash/stm32_flash.h>
#if defined(STM32F2XX)
#include <stm32f2xx_flash.h>
#elif defined(STM32F4XX)	 
#include <stm32f4xx_flash.h>
#elif defined(STM32f1xx)	 
#include <stm32f10x_flash.h>
#endif

#else
#include "flash_api.h"
#define DATA_SECTOR     (0x000FE000)
#define BACKUP_SECTOR	(0x00008000)
#define NET_INFO_ADD     (0x00090000)//0x000AF000
#define LOGIN_INFO_ADD     (0x00093000)//0x000DF000

#endif
#endif
/* ------------------------ Defines --------------------------------------- */
/* The size of the buffer in which the dynamic WEB page is created. */
#define webMAX_PAGE_SIZE       ( 15160 ) /*FSL: buffer containing array*/
#define LOCAL_BUF_SIZE        800
//#define LOCAL_BUF_SIZE        20000
#define AP_SETTING_ADDR			0x000FE000;
/* Standard GET response. */
#define webHTTP_OK  "HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"

/* The port on which we listen. */
#define webHTTP_PORT            ( 80 )

/* Delay on close error. */
#define webSHORT_DELAY          ( 10 )

/*Configuration of Web Server*/
//#define CONFIG_READ_FLASH 0
#define CONFIG_READ_FLASH 1

struct netconn *pxNewConnection;
unsigned char psk_essid1[NET_IF_NUM][36];
unsigned char psk_passphrase1[NET_IF_NUM][65];
unsigned char wifi_state = 0;
u8_t bSTAChanged = 0;
u8_t bChanged = 0;
web_set_msg_t web_set_msg;
unsigned char is_Init = 0;
extern struct netif xnetif[];			//LWIP netif
extern rtw_network_info_t wifi;
login_set_msg_t login_set_msg;
login_set_msg_t *login_msg;
uint8_t login_data[sizeof(login_set_msg_t)];
portCHAR cDynamicPage[webMAX_PAGE_SIZE];

#define logePage_HTML "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\
<html xmlns=\"http://www.w3.org/1999/xhtml\">\
<head>\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\" />\
<title>登录</title>\
<style type=\"text/css\">\
#back{width:100%; height:100%; background:#000;  position:absolute;top:0;left:0; z-index:9999;\
filter:alpha(opacity=50); -moz-opacity:0.5; -khtml-opacity:0.5; opacity:0.5; display:none;}\
#load{width:314px; height:160px; background:#048BCB; padding:1px; position:absolute; margin:-157px 0 0 -157px; left:50%; top:50%; z-index:99991; -moz-border-radius:5px; -webkit-border-radius:5px; border-radius:5px; display:none;}\
#load h2{color:#FFF; text-align:center; height:37px; line-height:37px; font-weight:normal; font-size:14px; font-family:\"宋体\";}\
.loadone{width:314px; height:123px; background:#FFF; -moz-border-radius:0 5px 5px 0; -webkit-border-radius:0  0 5px 5px; border:0 0 5px 5px;}\
.loadone img{width:100px; height:100px; margin:0 auto; display:block; text-align:center;}\
\
#load div{  text-align:center; padding:10px 0px;}\
#load div#closeDiv{  position:absolute; top:0px; right:0px; color:white; background-color:red; cursor:pointer; font-weight:bold; font-size:20px; width:30px; height:15px; line-height:15px; -moz-border-radius:0px 5px 0px 0px; -webkit-border-radius:0px 5px 0px 0px; border-radius:0px 5px 0px 0px;}\
div span{ display:inline-block; width:70px; text-align:right;}\
</style>\
<script type=\"text/javascript\">\
        function showLoading() {\
            var oBack = document.getElementById('back');\
            var oLoad = document.getElementById('load');\
            oBack.style.display = oLoad.style.display = 'block';\
			document.getElementById('hd_Show').value=\"1\";\
        }\
</script>\
</head>\
<body onload=\"showLoading();\">\
<form action=\"\"  method=\"post\" >\
  <input type=\"hidden\" value=\"0\" id=\"hd_Show\" />\
  <div id=\"back\"> </div>\
  <div id=\"load\">\
    <h2> 登  陆 </h2>\
    <div class=\"loadone\">\
      <div><span>用户名：</span>\
        <input type=\"text\" name=\"username\" id=\"username\" />\
      </div>\
      <div><span>密码：</span>\
        <input type=\"password\" name=\"password\" id=\"password\" />\
      </div>\
      <div>\
        <input type=\"submit\" value=\"登陆\" />\
      </div>\
    </div>\
  </div>\
</form>\
</body>\
</html>"

#define webHTML_alert "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\
<html xmlns=\"http://www.w3.org/1999/xhtml\">\
账号或密码错误，请核对后重新输入！"

#define webHTML_TEST "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\
<html xmlns=\"http://www.w3.org/1999/xhtml\">\
<head>\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\" />\
<title>光通国际无线路由设置</title>\
<SCRIPT language=Javascript src=\"<% getLangInfo(\"lang\");%>\"></SCRIPT>\
<script type=\"text/javascript\">\
\
var sel = 1;\
var height = 0;\
\
function opt_sel(v)\
{\
document.getElementById(\"op_\"+sel).className = \"opt_no\";\
document.getElementById(\"op_\"+v).className = \"opt_sel\";\
sel = v;\
}\
\
function init_main_page()\
{\
var f = document.form_net_setting;\
f.wifi_mode.options.selectedIndex = 0;\
f.ap_setting_ssid.value = \"QWE\";\
f.ap_setting_wpakey.value = \"11111111\";\
f.ap_setting_freq.options.selectedIndex = 5;\
f.sta_setting_ssid.value = \"GTI\";\
f.sta_setting_auth_sel.options.selectedIndex = 0;\
f.wan_setting_dhcp.options.selectedIndex = 0;\
f.wan_setting_ip.value = \"192.168.1.100\";\
f.wan_setting_msk.value = \"255.255.255.0\";\
f.wan_setting_gw.value = \"192.168.1.1\";\
f.wan_setting_dns.value = \"8.8.4.4\";\
f.net_setting_port.value = \"5001\";\
f.net_setting_ip.value = \"192.168.1.1\";\
f.ap_setting_auth.options.selectedIndex = 0;\
f.set_passname.value = \"admin\";\
f.set_pwd.value = \"admin\";\
auth_change1();\
auth_change();\
auth_change2();\
dhcp(2);\
internetSet();\
\
}\
\
function wpa_key_check_changed1()\
{\
if (wpa_key_show == \"1\") {\
if (window.ActiveXObject) {\
document.all('sta_setting_wpakey').outerHTML = \"<input type=password maxlength=64 size=32 name=sta_setting_wpakey id=wep_key_text class='text' value='\" + document.all('sta_setting_wpakey').value + \"'>\";\
}\
else {\
document.getElementById('wpa_key_text1').type = \"password\";\
}\
wpa_key_show = \"0\";\
}\
else {\
if (window.ActiveXObject) {\
document.all('sta_setting_wpakey').outerHTML = \"<input type=text maxlength=64 size=32 name=sta_setting_wpakey id=wep_key_text class='text' value='\" + document.all('sta_setting_wpakey').value + \"'>\";\
}\
else {\
document.getElementById('wpa_key_text1').type = \"text\";\
}\
wpa_key_show = \"1\";\
}\
}\
\
function enableCon(c, v)\
{\
var con = document.getElementById(c);\
if (con != null) {\
if (v == 1) {\
con.disabled = \"disabled\";\
}\
else {\
con.disabled = \"\";\
}\
}\
}\
\
function show(v)\
{\
var c = document.getElementById(v);\
if (c != null) {\
c.style.visibility = \"visible\";\
c.style.display = \"\";\
}\
}\
\
function hide(v)\
{\
var c = document.getElementById(v);\
if (c != null) {\
c.style.visibility = \"hidden\";\
c.style.display = \"none\";\
}\
}\
\
function dhcp(v)\
{\
var c;\
if (v == 2) {\
c = document.getElementById(\"div_3_dhcp\");\
if (c != null) {\
if (c.value == \"DHCP\") {\
enableCon(\"div_3_ip\", 1);\
enableCon(\"div_3_sub\", 1);\
enableCon(\"div_3_gate\", 1);\
enableCon(\"div_3_dns\", 1);\
}\
else {\
enableCon(\"div_3_ip\", 2);\
enableCon(\"div_3_sub\", 2);\
enableCon(\"div_3_gate\", 2);\
enableCon(\"div_3_dns\", 2);\
}\
}\
}\
}\
\
function auth_change()\
{\
var v = document.form_net_setting.sta_setting_auth_sel.options.selectedIndex;\
if (v == 0) {\
hide(\"pass_show\");\
}\
else {\
show(\"pass_show\");\
}\
}\
var wpa_key_show= \"0\";\
function wpa_key_check_changed()\
{\
if (wpa_key_show == \"1\")\
{\
if (window.ActiveXObject)\
{\
document.all('ap_setting_wpakey').outerHTML=\"<input type=password maxlength=64 size=32 name=ap_setting_wpakey id=wep_key_text class='text' value='\"+document.all('ap_setting_wpakey').value+\"'>\";\
}\
else\
{\
document.getElementById('wpa_key_text').type = \"password\";\
}\
wpa_key_show=\"0\";\
}\
else\
{\
if (window.ActiveXObject)\
{\
document.all('ap_setting_wpakey').outerHTML=\"<input type=text maxlength=64 size=32 name=ap_setting_wpakey id=wep_key_text class='text' value='\"+document.all('ap_setting_wpakey').value+\"'>\";\
}\
else\
{\
document.getElementById('wpa_key_text').type = \"text\";\
}\
wpa_key_show=\"1\";\
}\
}\
\
function hide(v)\
{\
var c = document.getElementById(v);\
if(c != null)\
{\
c.style.visibility = \"hidden\";\
c.style.display = \"none\";\
}\
}\
function show(v)\
{\
var c = document.getElementById(v);\
if(c != null)\
{\
c.style.visibility = \"visible\";\
c.style.display = \"\";\
}\
}\
function auth_change1()\
{\
var v = document.form_net_setting.wifi_mode.options.selectedIndex;\
if (v== 0)\
{\
hide(\"sta_show\");\
show(\"ap_show\");\
}\
else if(v == 1)\
{\
hide(\"ap_show\");\
show(\"sta_show\");\
}\
else\
{\
show(\"ap_show\");\
show(\"sta_show\");\
}\
}\
function auth_change2()\
{\
var v = document.form_net_setting.ap_setting_auth.options.selectedIndex;\
if (v== 0)\
{\
hide(\"ap_pass_show\");\
}\
else if(v == 1)\
{\
show(\"ap_pass_show\");\
}\
\
}\
\
function enableCon(c,v)\
{\
var con = document.getElementById(c);\
if(con  != null)\
{\
if( v == 0 ) con.disabled = \"disabled\";\
if (v == 1) con.disabled = \"\";\
}\
}\
\
function internetSet()\
{\
var pro = document.getElementById(\"div_5_3_tcp\");\
if(pro != null)\
{\
if(pro.options.selectedIndex == 0)\
{\
enableCon(\"div_5_3_port\", 0);\
enableCon(\"div_5_3_add\",0);\
}\
else if ( pro.options.selectedIndex == 1 ) \
{\
enableCon(\"div_5_3_port\", 0);\
enableCon(\"div_5_3_add\", 0);\
}\
else if (pro.options.selectedIndex == 2 ) \
{\
enableCon(\"div_5_3_port\", 0 );\
enableCon(\"div_5_3_add\", 0 );\
\
}\
\
}\
}\
function init_uart_setting()\
{\
var f=document.form_uart_setting;\
f.uart_setting_baud.value=uart_setting_baud;\
f.uart_setting_data.value=uart_setting_data;\
f.uart_setting_parity.value=uart_setting_parity;\
f.uart_setting_stop.value=uart_setting_stop;\
f.uart_setting_fc.value=uart_setting_fc;\
}\
\
function enableCon(c,v)\
{\
var con = document.getElementById(c);\
if(con  != null)\
{\
if(v == 1)\
{\
con.disabled = \"disabled\";\
}\
else\
{\
con.disabled = \"\";\
}\
}\
}\
\
\
function uart_setting_apply()\
{\
var f = document.form_net_setting;\
var v = document.form_net_setting.wifi_mode.options.selectedIndex;\
var c = document.form_net_setting.sta_setting_auth_sel.options.selectedIndex;\
if(v == 0 || v == 2)\
{\
if (f.ap_setting_ssid.value == \"\") {\
alert(\"AP_SSID为空！\");\
return;\
}\
if (f.ap_setting_ssid.value.length > 32) {\
alert(\"SSID的字符必须小于32个！\");\
return;\
}\
if ((f.ap_setting_wpakey.value.length < 8) || (f.ap_setting_wpakey.value.length > 64)) {\
alert(\"密码长度必须大于8个字符小于64个字符！\");\
return;\
}\
}\
if(v == 1 || v == 2)\
{\
if(f.sta_setting_ssid.value == \"\"){\
alert(\"STA_SSID为空！\");\
return;\
}\
if (f.sta_setting_ssid.value.length > 32) {\
alert(\"SSID的字符必须小于32个！\");\
return;\
}\
if ((f.sta_setting_wpakey.value == \"\") && (c != 0)) {\
alert(\"STA密码为空！\");\
return;\
}\
}\
if(parseInt(f.net_setting_port.value)>=65535)\
{\
alert(\"端口数值必须小于65535！\");\
return;\
}\
if (!f.net_setting_ip.value) {\
alert(\"IP不能为空！\");\
return;\
}\
\
\
f.submit();\
}\
</script>\
\
<style>\
.opt_sel\
{\
height:30px;\
background-color:#006699;\
padding-left:25px;\
color:White;\
cursor:pointer;\
}\
.opt_no\
{\
height:30px;\
padding-left:25px;\
cursor:pointer;\
color:#757475;\
}\
\
.opt_cn_big\
{\
padding-top:4px;\
font-size:20px;\
font-weight:bold;\
}\
\
.cl\
{\
clear:left;\
}\
\
.div_c\
{\
margin-left:50px;\
margin-right:50px;\
margin-top:50px;\
margin-bottom:50px;\
}\
.lab_5\
{\
font-size:15px;\
color:blue;\
margin-left:-10px;\
}\
.cu_b\
{\
cursor:pointer;\
font-weight:bold;\
}\
.sp_5\
{\
width:10px;\
height:5px;	\
}\
.label\
{\
float:left;\
width:50%;\
color:blue;\
margin-bottom:-2px;\
font-size:13px;\
}\
.lab_r\
{\
float:left;\
width:50%;\
color:blue;\
font-size:13px;\
}\
.line\
{\
height:1px;\
background-color:blue;\
width:60%;\
margin-top:5px;\
margin-bottom:5px;\
overflow:hidden;\
}\
.lab_l\
{\
float:left;\
width:40%;\
color:blue;\
margin-bottom:-2px;\
margin-left:10%;\
font-size:13px;\
}\
.line_l\
{\
height:1px;\
background-color:blue;\
width:825px;\
margin-top:5px;\
margin-bottom:5px;\
margin-left:165px;\
overflow:hidden;\
}\
\
.div_c\
{\
margin-left:50px;\
margin-right:50px;\
margin-top:50px;\
margin-bottom:50px;\
}\
\
.fw\
{\
float:left;\
width:50%;\
}\
.btn\
{\
width:66px;\
height:27px;\
border-style:none;\
border-radius:3px 3px 3px 3px;\
font-size:14px;\
color:#0163AC;\
cursor:pointer;\
}\
.l4\
{\
margin-left:4px;\
}\
.lab_4\
{\
margin-left:70px;\
margin-right:50px;\
color:blue;\
font-size:13px;\
}\
.tr\
{\
text-align:center;\
}\
.r10\
{\
margin-right:10px;\
}\
.blue\
{\
color:blue;\
}\
.s15\
{\
font-size:15px;\
}\
.b\
{\
font-weight:bold;\
}\
.sp_20\
{\
height:20px;\
width:500px;\
}\
.sp_10\
{\
width:10px;\
height:10px;\
}\
.sp_30\
{\
width:10px;\
height:30px;\
}\
</style>\
\
</head>\
\
<BODY style=\"background-color:#F5F5F5\" onload=\"init_main_page()\">\
\
<div class=\"div_c\" style=\"font-family:微软雅黑\" >\
\
\
<div id=\"op_1\" class=\"opt_sel\" ><div class=\"opt_cn_big\">无线AP设置</div></div>\
\
<div class=\"div_c\" style=\"font-family:微软雅黑\">\
<div id=\"port_cn\">\
<font class=\"blue s15 b\">启动参数设置</font>\
<form name= \"form_net_setting\" method=\"post\" action=\"do_cmd_cn.html\">\
<input type=\"hidden\" name=\"net_setting_pro\" />\
<input type=\"hidden\" name=\"net_setting_cs\" />\
<div class=\"lab_4\">\
\
<div class=\"label\">模式选择</div>\
<div class=\"fw\">\
<select id=\"div_5_4_wep\" style=\"width:150px;height:20px\"  onchange=\"auth_change1()\" name=\"wifi_mode\">\
<option value=\"AP\">AP </option>\
<option value=\"STA\">STA </option>\
<option value=\"APSTA\">AP+STA </option>\
</select>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
\
<div id=\"ap_show\"> \
\
<div class=\"label\">网络名称（AP_SSID）</div>\
<div class=\"fw\"><input type=\"text\" style=\"width:150px;height:20px\" value=\"\" name=\"ap_setting_ssid\"></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
\
<div class=\"label\">加密模式</div>\
<div class=\"fw\"><select id=\"div_5_4_wep\" style=\"width:150px;height:20px\" onchange=\"auth_change2()\" name=\"ap_setting_auth\">\
<option value=\"CLOSE\" selected=\"\">Disable</option>\
<option value=\"WPA2PSK\">WPA2-PSK</option></select>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div id=\"ap_pass_show\"> \
<div class=\"label\">密码(AP_PASSWD)</div>\
<div class=\"fw\">\
<input name=\"ap_setting_wpakey\" type=\"password\" class=\"text\" id=\"wpa_key_text\">\
<div>\
<input onclick=\"wpa_key_check_changed()\" type=\"checkbox\" style=\"margin-left: 0px;\"/>\
<font style=\"color:blue;font-size:13px\">显示密码</font>\
</div>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
</div>\
<div class=\"label\">无线信道选择</div>\
<div class=\"fw\"><select style=\"width:150px;height:20px\" name=\"ap_setting_freq\">\
<option id=\"ap_FrequencyAS_lan_id\" value=\"0\" selected=\"\">自动选取</option>\
<option value=\"1\">2412MHz(信道 1)</option>\
<option value=\"2\">2417MHz(信道 2)</option>\
<option value=\"3\">2422MHz(信道 3)</option>\
<option value=\"4\">2427MHz(信道 4)</option>\
<option value=\"5\">2432MHz(信道 5)</option>\
<option value=\"6\">2437MHz(信道 6)</option>\
<option value=\"7\">2442MHz(信道 7)</option>\
<option value=\"8\">2447MHz(信道 8)</option>\
<option value=\"9\">2452MHz(信道 9)</option>\
<option value=\"10\">2457MHz(信道 10)</option>\
<option value=\"11\">2462MHz(信道 11)</option>\
</select>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
</div>\
\
<div id=\"sta_show\">\
<div class=\"label\">网络名称（STA_SSID）<br />注意区分大小写</div>\
<div class=\"fw\"><input name=\"sta_setting_ssid\" type=\"text\" style=\"width:150px;height:20px;font-size:14px\" />\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
\
<div class=\"label\">加密方式</div>\
<div class=\"fw\"><select name=\"sta_setting_auth_sel\" onchange=\"auth_change()\" style=\"width:100px;height:20px\">\
<option value=\"OPENNONE\">OPEN</option>\
<option value=\"OPENWEP\">OPENWEP</option>\
<option value=\"SHAREDWEP\">SHAREDWEP</option>\
<option value=\"WPAPSK\">WPAPSK</option>\
<option value=\"WPA2PSK\">WPA2PSK</option>\
</select>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
\
<div id=\"pass_show\">\
<div class=\"label\">密码</div>\
<div class=\"fw\">\
<input name=\"sta_setting_wpakey\" type=\"password\" class=\"text\" id=\"wpa_key_text1\">\
<div>\
<input onclick=\"wpa_key_check_changed1()\" type=\"checkbox\" style=\"margin-left: 0px;\"/>\
<font style=\"color:blue;font-size:13px\">显示密码</font>\
</div>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
</div>\
\
<div class=\"label\">自动获得IP</div>\
<div class=\"fw\">\
<select name=\"wan_setting_dhcp\" id=\"div_3_dhcp\" style=\"width:100px;height:20px\" onchange=\"dhcp(2)\">\
<option value=\"DHCP\">Enable</option>\
<option value=\"STATIC\">Disable</option>\
</select>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">IP地址</div>\
<div class=\"fw\"><input name=\"wan_setting_ip\" id=\"div_3_ip\" type=\"text\" class=\"text\"></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">子网掩码</div>\
<div class=\"fw\"><input name=\"wan_setting_msk\" id=\"div_3_sub\" type=\"text\" class=\"text\"></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">网关地址</div>\
<div class=\"fw\"><input name=\"wan_setting_gw\" id=\"div_3_gate\" type=\"text\" class=\"text\"></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">DNS服务器地址</div>\
<div class=\"fw\"><input name=\"wan_setting_dns\" id=\"div_3_dns\" type=\"text\" class=\"text\">\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
\
</div>\
</div>\
\
<font class=\"blue s15 b\">网络参数设置</font>\
<div class=\"lab_4\">\
<div class=\"label\">协议</div>\
<div class=\"fw\"><select  name=\"sta_net\" id=\"div_5_3_tcp\" style=\"width:150px;height:20px\" onclick=\"internetSet()\">\
<option value=\"TCPSERVER\">TCP-Server</option>\
<option value=\"TCPCLIENT\">TCP-Client</option>\
<option value=\"UDPSERVER\">UDP</option></select>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">端口</div>\
<div class=\"fw\"><input name=\"net_setting_port\" id=\"div_5_3_port\" type=\"text\" style=\"width:150px;height:20px\" /></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">服务器地址</div>\
<div class=\"fw\"><input name=\"net_setting_ip\" id=\"div_5_3_add\" type=\"text\" style=\"width:150px;height:20px\" /></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
</div>\
\
\
\
\
<font class=\"blue s15 b\">串口参数设置</font>\
\
<div class=\"lab_4\">\
<div class=\"label\">波特率</div>\
<div class=\"fw\"><select style=\"width:150px;height:20px\" name=\"uart_setting_baud\">\
<option value=\"300\">300</option>\
<option value=\"600\">600</option>\
<option value=\"1200\">1200</option>\
<option value=\"1800\">1800</option>\
<option value=\"2400\">2400</option>\
<option value=\"4800\">4800</option>\
<option value=\"9600\">9600</option>\
<option value=\"19200\">19200</option>\
<option value=\"38400\">38400</option>\
<option value=\"57600\">57600</option>\
<option selected=\"selected\" value=\"115200\">115200</option>\
<option value=\"230400\">230400</option>\
<option value=\"380400\">380400</option>\
<option value=\"460800\">460800</option></select>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">数据位</div>\
<div class=\"fw\"><select style=\"width:150px;height:20px\" name=\"uart_setting_data\">\
<option value=\"5\">5</option>\
<option value=\"6\">6</option>\
<option value=\"7\">7</option>\
<option selected=\"selected\" value=\"8\">8</option></select>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">校验位</div>\
<div class=\"fw\"><select style=\"width:150px;height:20px\" name=\"uart_setting_parity\">\
<option value=\"0\">None</option>\
<option value=\"1\">Odd</option>\
<option value=\"2\">Even</option>\
<option value=\"3\">Mark</option>\
<option value=\"4\">Space</option>\
</select></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">停止位</div>\
<div class=\"fw\"><select style=\"width:150px;height:20px\" name=\"uart_setting_stop\">\
<option value=\"1\">1</option>\
<option value=\"2\">2</option>\
</select></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
<div class=\"label\">CTSRTS</div>\
<div class=\"fw\"><select style=\"width:150px;height:20px\" name=\"uart_setting_fc\">\
<option value=\"0\">Disable</option>\
<option value=\"1\">Enable</option>\
</select>\
</div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
</div>\
<font class=\"blue s15 b\">修改登录密码</font>\
<div class=\"lab_4\">\
<div class=\"label\">新用户名(0-10位)</div>\
<div class=\"fw\"><input type=\"text\" style=\"width:150px;height:20px\" value=\"\" name=\"set_passname\"></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
</div>	\
<div class=\"lab_4\">\
<div class=\"label\">新密码(0-10位)</div>\
<div class=\"fw\"><input type=\"text\" style=\"width:150px;height:20px\" value=\"\" name=\"set_pwd\"></div>\
<div class=\"cl\"></div>\
<div class=\"line\"></div>\
</div>	\
<div class=\"lab_4 tr\"><input onclick=\"uart_setting_apply()\" type=\"button\" class=\"btn\" value=\"保存\"/></div>\
\
\
</form>\
</div>\
</div>\
\
</div>\
\
</BODY>\
\
\
</html>"
     


#define MAX_SOFTAP_SSID_LEN      32
#define MAX_PASSWORD_LEN          32
#define MAX_CHANNEL_NUM             13

#if INCLUDE_uxTaskGetStackHighWaterMark
	static volatile unsigned portBASE_TYPE uxHighWaterMark_web = 0;
#endif

/* ------------------------ Prototypes ------------------------------------ */
static void     vProcessConnection( struct netconn *pxNetCon );


/*------------------------------------------------------------------------------*/
/*                            GLOBALS                                          */
/*------------------------------------------------------------------------------*/
rtw_wifi_setting_t wifi_setting = {RTW_MODE_NONE, {0}, 0, RTW_SECURITY_OPEN, {0}};

#ifndef WLAN0_NAME
  #define WLAN0_NAME		"wlan0"
#endif

#ifndef WLAN1_NAME
  #define WLAN1_NAME      	"wlan1"
#endif 

static void LoadWifiSetting()
{
    const char *ifname = WLAN0_NAME;

    if(rltk_wlan_running(WLAN1_IDX))
    {//STA_AP_MODE
    	ifname = WLAN1_NAME;
    }

    wifi_get_setting(ifname, &wifi_setting);

    //printf("\r\nLoadWifiSetting(): wifi_setting.ssid=%s\n", wifi_setting.ssid); 
    //printf("\r\nLoadWifiSetting(): wifi_setting.channel=%d\n", wifi_setting.channel);
    //printf("\r\nLoadWifiSetting(): wifi_setting.security_type=%d\n", wifi_setting.security_type); 
    //printf("\r\nLoadWifiSetting(): wifi_setting.password=%s\n", wifi_setting.password); 
}

#if CONFIG_READ_FLASH
#ifndef CONFIG_PLATFORM_8195A
void LoadWifiConfig()
{
    rtw_wifi_config_t local_config;
    uint32_t address;
#ifdef STM32F10X_XL
    address = 0x08080000;   //bank2 domain
#else
    uint16_t sector_nb = FLASH_Sector_11;
    address = flash_SectorAddress(sector_nb);
#endif
    printf("\r\nLoadWifiConfig(): Read from FLASH!\n"); 
    flash_Read(address, (char *)&local_config, sizeof(local_config));
    
    printf("\r\nLoadWifiConfig(): local_config.boot_mode=0x%x\n", local_config.boot_mode); 
    printf("\r\nLoadWifiConfig(): local_config.ssid=%s\n", local_config.ssid); 
    printf("\r\nLoadWifiConfig(): local_config.channel=%d\n", local_config.channel);
    printf("\r\nLoadWifiConfig(): local_config.security_type=%d\n", local_config.security_type); 
    printf("\r\nLoadWifiConfig(): local_config.password=%s\n", local_config.password);

    if(local_config.boot_mode == 0x77665502)
    {
        wifi_setting.mode = RTW_MODE_AP;
        if(local_config.ssid_len > 32)
            local_config.ssid_len = 32;
        memcpy(wifi_setting.ssid, local_config.ssid, local_config.ssid_len);
        wifi_setting.ssid[local_config.ssid_len] = '\0';
        wifi_setting.channel = local_config.channel;
        wifi_setting.security_type = local_config.security_type;
        if(local_config.password_len > 32)
            local_config.password_len = 32;
        memcpy(wifi_setting.password, local_config.password, local_config.password_len);
        wifi_setting.password[local_config.password_len] = '\0';
    }
    else
    {
        LoadWifiSetting();
    }
}

int StoreApInfo()//#ifndef CONFIG_PLATFORM_8195A
{
	rtw_wifi_config_t wifi_config;
	uint32_t address;
#ifdef STM32F10X_XL
	address = 0x08080000;	//bank2 domain
#else
	uint16_t sector_nb = FLASH_Sector_11;
	address = flash_SectorAddress(sector_nb);
#endif
	wifi_config.boot_mode = 0x77665502;
	memcpy(wifi_config.ssid, wifi_setting.ssid, strlen((char*)wifi_setting.ssid));
	wifi_config.ssid_len = strlen((char*)wifi_setting.ssid);
	wifi_config.security_type = wifi_setting.security_type;
	memcpy(wifi_config.password, wifi_setting.password, strlen((char*)wifi_setting.password));
	wifi_config.password_len= strlen((char*)wifi_setting.password);
	wifi_config.channel = wifi_setting.channel;

	printf("\n\rWritting boot mode 0x77665502 and Wi-Fi setting to flash ...");
#ifdef STM32F10X_XL
	FLASH_ErasePage(address);
#else
	flash_EraseSector(sector_nb);
#endif
	flash_Wrtie(address, (char *)&wifi_config, sizeof(rtw_wifi_config_t));

	return 0;
}


#else

void LoadWifiConfig()
{


    flash_t flash;

    rtw_wifi_config_t local_config;
    uint32_t address;

    address = DATA_SECTOR;

    
    //memset(&local_config,0,sizeof(rtw_wifi_config_t));
    printf("\r\nLoadWifiConfig(): Read from FLASH!\n"); 
   // flash_Read(address, &local_config, sizeof(local_config));

    flash_stream_read(&flash, address, sizeof(rtw_wifi_config_t),(uint8_t *)(&local_config));

    
    printf("\r\nLoadWifiConfig(): local_config.boot_mode=0x%x\n", local_config.boot_mode); 
    printf("\r\nLoadWifiConfig(): local_config.ssid=%s\n", local_config.ssid); 
    printf("\r\nLoadWifiConfig(): local_config.channel=%d\n", local_config.channel);
    printf("\r\nLoadWifiConfig(): local_config.security_type=%d\n", local_config.security_type); 
    printf("\r\nLoadWifiConfig(): local_config.password=%s\n", local_config.password);

    if(local_config.boot_mode == 0x77665502)
    {
        wifi_setting.mode = RTW_MODE_AP;
//        wifi_setting.mode =RTW_MODE_STA_AP;
        if(local_config.ssid_len > 32)
            local_config.ssid_len = 32;
        memcpy(wifi_setting.ssid, local_config.ssid, local_config.ssid_len);
        wifi_setting.ssid[local_config.ssid_len] = '\0';
        wifi_setting.channel = local_config.channel;
        if(local_config.security_type == 1)
          wifi_setting.security_type = RTW_SECURITY_WPA2_AES_PSK;
        else
          wifi_setting.security_type = RTW_SECURITY_OPEN;
        if(local_config.password_len > 32)
            local_config.password_len = 32;
        memcpy(wifi_setting.password, local_config.password, local_config.password_len);
        wifi_setting.password[local_config.password_len] = '\0';
    }
    else
    {
        LoadWifiSetting();
    }

}

int StoreNetInfo()
{
  flash_t flash;
//  uint8_t *data;
//  printf("I AM %d\r\n",web_set_msg);
  flash_erase_sector(&flash, NET_INFO_ADD);
  web_set_msg.head1 = 0xee;
  web_set_msg.head2 = 0xcc;
  web_set_msg.tail = 0xff;
  flash_stream_write(&flash, NET_INFO_ADD, sizeof(web_set_msg_t), (uint8_t *)&web_set_msg);
//  flash_stream_read(&flash, NET_INFO_ADD, sizeof(web_set_msg_t), (uint8_t *)&data);
//  printf("I AM %d\r\n",data);
  return 1;
}

int StoreLoginInfo()
{
  flash_t flash;
  flash_erase_sector(&flash, LOGIN_INFO_ADD);
  flash_stream_write(&flash, LOGIN_INFO_ADD, sizeof(login_set_msg_t), (uint8_t *)&login_set_msg);
  return 1;
}

int StoreApInfo()//#ifdef CONFIG_PLATFORM_8195A
{

    flash_t flash;

	rtw_wifi_config_t wifi_config;
	uint32_t address;
        uint32_t data,i = 0;


    address = DATA_SECTOR;

    wifi_config.boot_mode = 0x77665502;
    memcpy(wifi_config.ssid, wifi_setting.ssid, strlen((char*)wifi_setting.ssid));
    wifi_config.ssid_len = strlen((char*)wifi_setting.ssid);
    wifi_config.security_type = wifi_setting.security_type;
    if(wifi_setting.security_type !=0)
        wifi_config.security_type = 1;
    else
        wifi_config.security_type = 0;
    memcpy(wifi_config.password, wifi_setting.password, strlen((char*)wifi_setting.password));
    wifi_config.password_len= strlen((char*)wifi_setting.password);
    wifi_config.channel = wifi_setting.channel;
    printf("\n\rWritting boot mode 0x77665502 and Wi-Fi setting to flash ...");
    //printf("\n\r &wifi_config = 0x%x",&wifi_config);

   flash_read_word(&flash,address,&data);

   
    if(data == ~0x0)

      flash_stream_write(&flash, address,sizeof(rtw_wifi_config_t), (uint8_t *)&wifi_config);

    else{
    //flash_EraseSector(sector_nb);
      
      
        flash_erase_sector(&flash,BACKUP_SECTOR);
        for(i = 0; i < 0x1000; i+= 4){
            flash_read_word(&flash, DATA_SECTOR + i, &data);
            if(i < sizeof(rtw_wifi_config_t))
            {
                 memcpy(&data,(char *)(&wifi_config) + i,4);
                 //printf("\n\r Wifi_config + %d = 0x%x",i,(void *)(&wifi_config + i));
                 //printf("\n\r Data = %d",data);
            }
            flash_write_word(&flash, BACKUP_SECTOR + i,data);
        }
        flash_read_word(&flash,BACKUP_SECTOR + 68,&data);
        //printf("\n\r Base + BACKUP_SECTOR + 68 wifi channel = %d",data);
        //erase system data
        flash_erase_sector(&flash, DATA_SECTOR);
        //write data back to system data
        for(i = 0; i < 0x1000; i+= 4){
            flash_read_word(&flash, BACKUP_SECTOR + i, &data);
            flash_write_word(&flash, DATA_SECTOR + i,data);
        }
                  //erase backup sector
           flash_erase_sector(&flash, BACKUP_SECTOR);
        }
        

	return 0;
}
#endif


//static void RestartSoftAP()
static void RestartSoftAP()
{
	wifi_restart_ap(wifi_setting.ssid,
					wifi_setting.security_type,
					wifi_setting.password,
					strlen((char*)wifi_setting.ssid),
					strlen((char*)wifi_setting.password),
					wifi_setting.channel);
}
#endif


u32 web_atoi(char* s)
{
	int num=0,flag=0;
	int i;

	for(i=0;i<=strlen(s);i++)
	{
		if(s[i] >= '0' && s[i] <= '9')
			num = num * 10 + s[i] -'0';
		else if(s[0] == '-' && i==0) 
			flag =1;
		else 
			break;
	}

	if(flag == 1)
		num = num * -1;

	return(num); 
}

static void GenerateIndexHtmlPage(portLONG* cDynamicPage, portCHAR *LocalBuf)
{
        strcpy( cDynamicPage, webHTML_TEST);  
 //         strcpy( cDynamicPage, data_1_bk_htm); 
 //       printf("\r\nGenerateIndexHtmlPage(): %s\n", cDynamicPage);
        printf("\r\nGenerateIndexHtmlPage Len: %d\n", strlen( cDynamicPage ));
}

static void GenerateLoadHtmlPage(portCHAR* cDynamicPage)
{
        strcpy( cDynamicPage, logePage_HTML); 
        printf("\r\nGenerateIndexHtmlPage Len: %d\n", strlen( cDynamicPage ));
}

static void GenerateAlartHtmlPage(portCHAR* cDynamicPage)
{
        strcpy( cDynamicPage, webHTML_alert); 
        printf("\r\nGenerateIndexHtmlPage Len: %d\n", strlen( cDynamicPage ));
}

static u8_t ProcessPostMessage(struct netbuf  *pxRxBuffer, portCHAR *LocalBuf)
{
    struct pbuf *p;
//    static portCHAR cDynamicPage[webMAX_PAGE_SIZE];
    portCHAR *pcRxString, *ptr;
    unsigned portSHORT usLength;
    u8_t bChanged = 0;
    rtw_security_t secType;
    u8_t channel;
    u8_t len = 0;
    
    pcRxString = LocalBuf;
    p = pxRxBuffer->p;
    usLength = p->tot_len;
//    printf("\r\n !!!!!!!!!POST!p->tot_len =%d p->len=%d\n", p->tot_len, p->len);            
    while(p)
    {
        memcpy(pcRxString, p->payload, p->len);
        pcRxString += p->len;
        p = p->next;
    }
    pcRxString = LocalBuf;
    pcRxString[usLength] = '\0';
    
    printf("\r\n APPOST--------------------usLength=%d pcRxString = %s\n", usLength, pcRxString);//POST MSG has saved in pcRxString 20150624
    
/*******************************************************************************
*
*验证登陆页账号密码
*
********************************************************************************/
    ptr = (char*)strstr(pcRxString, "username=");
    if(ptr)
    {
        flash_stream_read(&flash, (0x00093000), sizeof(login_set_msg_t), (uint8_t *)&login_data);
        login_msg = (login_set_msg_t *)login_data;
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 9;
        printf("login_name=%s\r\n",login_msg->login_name);
        printf("login_passwd=%s\r\n",login_msg->login_passwd);
        if(!strcmp(ptr, login_msg->login_name))//如果账号是admin
        {
            ptr = (char*)strstr(pcRxString, "password=");
            pcRxString = (char*)strstr(ptr, "&");
            *pcRxString++ = '\0';
            ptr += 9;
            if(!strcmp(ptr, login_msg->login_passwd))//如果密码是admin
            {
                GenerateIndexHtmlPage(cDynamicPage, LocalBuf);
                netconn_write( pxNewConnection, cDynamicPage, ( u16_t ) strlen( cDynamicPage ), NETCONN_NOCOPY );
//                vPortFree(cDynamicPage);
            }
            else//密码错误
            {
                GenerateAlartHtmlPage(cDynamicPage);
                netconn_write( pxNewConnection, cDynamicPage, ( u16_t ) strlen( cDynamicPage ), NETCONN_NOCOPY );
//                vPortFree(cDynamicPage);
            }
        }
        else//账号错误
        {
            GenerateAlartHtmlPage(cDynamicPage);
            netconn_write( pxNewConnection, cDynamicPage, ( u16_t ) strlen( cDynamicPage ), NETCONN_NOCOPY );
//            vPortFree(cDynamicPage);
        }
    }
    
    
/*******************************************************************************
*
*配置页信息获取
*
********************************************************************************/
    ptr = (char*)strstr(pcRxString, "wifi_mode=");//存储模块启动模式
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 10;
        if(!strcmp(ptr, "AP"))
        {
          bChanged = 1;
          web_set_msg.start_mode = AP;
        }
        else if(!strcmp(ptr, "STA"))
        {
          bSTAChanged = 1;
            web_set_msg.start_mode = STA;
        }
        else if(!strcmp(ptr, "APSTA"))
        {
          bChanged = 1;
            web_set_msg.start_mode = STAAP;
        }
        
    }
    
    ptr = (char*)strstr(pcRxString, "ap_setting_ssid=");
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 16;
        if(strcmp((char*)wifi_setting.ssid, ptr))
        {
//            bChanged = 1;
            len = strlen(ptr);
            if(len > MAX_SOFTAP_SSID_LEN){
                len = MAX_SOFTAP_SSID_LEN;
                ptr[len] = '\0';
            }
            strcpy((char*)wifi_setting.ssid, ptr);
        }
    }
    
    //printf("\r\n wifi_config.ssid = %s\n", wifi_setting.ssid);
    ptr = (char*)strstr(pcRxString, "ap_setting_auth=");
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 16;
        if(!strcmp(ptr, "OPEN"))
           secType = RTW_SECURITY_OPEN;
        else if(!strcmp(ptr, "WPA2PSK"))
            secType = RTW_SECURITY_WPA2_AES_PSK;
        else
            secType = RTW_SECURITY_OPEN;
        if(wifi_setting.security_type != secType)
        {
//            bChanged = 1;
            wifi_setting.security_type = secType;
        }
    }
    
    //printf("\r\n wifi_config.security_type = %d\n", wifi_setting.security_type);
    if(wifi_setting.security_type > RTW_SECURITY_OPEN)
    {
        ptr = (char*)strstr(pcRxString, "ap_setting_wpakey=");
        if(ptr)
        {
            pcRxString = (char*)strstr(ptr, "&");
            *pcRxString++ = '\0';
            ptr += 18;
            if(strcmp((char*)wifi_setting.password, ptr))
            {
 //               bChanged = 1;
                len = strlen(ptr);
                if(len > MAX_PASSWORD_LEN){
                    len = MAX_PASSWORD_LEN;
                    ptr[len] = '\0';
                }
                strcpy((char*)wifi_setting.password, ptr);
            }
        }
        //printf("\r\n wifi_config.password = %s\n", wifi_setting.password);
    }
    ptr = (char*)strstr(pcRxString, "ap_setting_freq=");
    if(ptr)
    {
        ptr += 16;
        channel = web_atoi(ptr);
        if((channel>MAX_CHANNEL_NUM)||(channel < 1))
            channel = 1;
        if(wifi_setting.channel !=channel)
        {
 //           bChanged = 1;
            wifi_setting.channel = channel;
        }
    }
    //printf("\r\n wifi_config.channel = %d\n", wifi_setting.channel);
    
    
    
    
    ptr = (char*)strstr(pcRxString, "sta_setting_ssid=");//sta接入网络的名称
    
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 17;
        if(strcmp((char*)wifi.ssid.val, ptr))
        {
            bSTAChanged = 1;
/*            len = strlen(ptr);
            strcpy((char*)wifi.ssid.val, ptr);
            printf("\r\n wifi.ssid.val:%s",wifi.ssid.val);
            wifi.ssid.len = len;*/
//          fATW0(ptr);
            strcpy((char*)web_set_msg.sta_ssid, ptr);
        }
    }
    
    ptr = (char*)strstr(pcRxString, "sta_setting_wpakey=");//sta接入网络的密码
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 19;
        if(strcmp((char*)wifi.password, ptr))
        {
 /*           len = strlen(ptr);
            
            wifi.password = malloc(len);
            strcpy((char*)wifi.password, ptr);
            
            printf("\r\n wifi.password:%s",wifi.password);
            wifi.password_len = len;
            if(wifi.password != NULL){
		if((wifi.key_id >= 0)&&(wifi.key_id <= 3)) {
			wifi.security_type = RTW_SECURITY_WEP_PSK;
		}
		else{
			wifi.security_type = RTW_SECURITY_WPA2_AES_PSK;
		}
            }
            else{
		wifi.security_type = RTW_SECURITY_OPEN;
            }*/
//          fATW1(ptr);
          strcpy((char*)web_set_msg.sta_passwd, ptr);
        }
    }       
    
    ptr = (char*)strstr(pcRxString, "wan_setting_dhcp=");//sta模式下是否动态分配ip
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 17;
        if(!strcmp(ptr, "STATIC"))
        {
          web_set_msg.is_dhcp = 1;
        }
        else if(!strcmp(ptr, "DHCP"))
        {
            web_set_msg.is_dhcp = 0;
        }       
    }
    
    ptr = (char*)strstr(pcRxString, "wan_setting_ip=");//存储本地ip地址
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 15;
        len = strlen(ptr);
        if(web_set_msg.is_dhcp == 1)
        {
 //         web_set_msg.sta_ip = malloc(len);
          strcpy((char*)web_set_msg.sta_ip, ptr);
        }
    }
    
    ptr = (char*)strstr(pcRxString, "wan_setting_msk=");//存储本地dns地址
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 16;
        len = strlen(ptr);
        if(web_set_msg.is_dhcp == 1)
        {
 //         web_set_msg.sta_dns = malloc(len);
          strcpy((char*)web_set_msg.sta_msk, ptr);
        }
    }
    
    ptr = (char*)strstr(pcRxString, "wan_setting_gw=");//存储本地网关地址
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 15;
        len = strlen(ptr);
        if(web_set_msg.is_dhcp == 1)
        {
 //         web_set_msg.sta_gateway = malloc(len);
          strcpy((char*)web_set_msg.sta_gateway, ptr);
        }
    }
       
    ptr = (char*)strstr(pcRxString, "sta_net=");//存储模块工作模式
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 8;
        if(!strcmp(ptr, "TCPSERVER"))
        {
          web_set_msg.work_mode = 1;
        }
        else if(!strcmp(ptr, "TCPCLIENT"))
        {
            web_set_msg.work_mode = 0;
        }
        else if(!strcmp(ptr, "UDPSERVER"))
        {
            web_set_msg.work_mode = 2;
        }
        else if(!strcmp(ptr, "UDPCLIENT"))
        {
            web_set_msg.work_mode = 3;
        }
//        printf("I AM %d\r\n",web_set_msg.work_mode);
    }
    
    ptr = (char*)strstr(pcRxString, "net_setting_port=");//存储端口号
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 17;
        if(web_set_msg.work_mode == 1)
          strcpy((char*)web_set_msg.tcp_server_port, ptr);
        else if(web_set_msg.work_mode == 0)
          strcpy((char*)web_set_msg.tcp_client_port, ptr);
        else if(web_set_msg.work_mode == 2)
          strcpy((char*)web_set_msg.udp_server_port, ptr);
        else if(web_set_msg.work_mode == 3)
          strcpy((char*)web_set_msg.udp_client_port, ptr);
    }
    
    ptr = (char*)strstr(pcRxString, "net_setting_ip=");//存储服务器ip地址
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 15;
        len = strlen(ptr);
        if(web_set_msg.work_mode == 1)
        {
 //         web_set_msg.tcp_server_ip = malloc(len);
          strcpy((char*)web_set_msg.tcp_server_ip, ptr);
        }
        else if(web_set_msg.work_mode == 0)
        {
//          web_set_msg.tcp_client_ip = malloc(len);
          strcpy((char*)web_set_msg.tcp_client_ip, ptr);
        }
        else if(web_set_msg.work_mode == 2)
        {
//          web_set_msg.udp_ip = malloc(len);
          strcpy((char*)web_set_msg.udp_server_ip, ptr);
        }
        else if(web_set_msg.work_mode == 3)
        {
//          web_set_msg.udp_ip = malloc(len);
          strcpy((char*)web_set_msg.udp_client_ip, ptr);
        }
    }
    
    
    
    ptr = (char*)strstr(pcRxString, "uart_setting_baud=");//存储串口波特率
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 18;
        strcpy((char*)web_set_msg.uart_baud, ptr);
    }
    
    ptr = (char*)strstr(pcRxString, "uart_setting_data=");//存储串口数据位
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 18;
        strcpy((char*)web_set_msg.uart_num, ptr);
    }
    
    ptr = (char*)strstr(pcRxString, "uart_setting_stop=");//存储串口停止位
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 18;
        strcpy((char*)web_set_msg.uart_stopbit, ptr);
    }
    
    ptr = (char*)strstr(pcRxString, "uart_setting_fc=");//存储串口流控
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 16;
        strcpy((char*)web_set_msg.uart_flowctl, ptr);
    }
    
    ptr = (char*)strstr(pcRxString, "set_passname=");//存储登录用户名
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 13;
        strcpy((char*)login_set_msg.login_name, ptr);
    }
    
    ptr = (char*)strstr(pcRxString, "set_pwd=");//存储登录密码
    if(ptr)
    {
        pcRxString = (char*)strstr(ptr, "&");
        *pcRxString++ = '\0';
        ptr += 8;
        strcpy((char*)login_set_msg.login_passwd, ptr);
    }
//    printf("\r\n web_set_msg = %s\n", (portCHAR *)&web_set_msg);
//    netbuf_delete( pcRxString );
    return bChanged;
}


struct netconn *pxHTTPListener = NULL;
static void vProcessConnection( struct netconn *pxNetCon )
{
//    static portCHAR cDynamicPage[2300];
//    static portCHAR *cDynamicPage;
    struct netbuf  *pxRxBuffer;
    portCHAR       *pcRxString;
    unsigned portSHORT usLength;
    static portCHAR LocalBuf[1];
    
    int ret_recv = ERR_OK;
    int ret_accept = ERR_OK;
    int ret,mode;
    unsigned long tick1 = xTaskGetTickCount();
    unsigned long tick2, tick3;
    char empty_bssid[6] = {0}, assoc_by_bssid = 0;


    /* Load WiFi Setting*/
    LoadWifiSetting();

    /* We expect to immediately get data. */
	// Evan mopdified for adapt two version lwip api diff
    port_netconn_recv( pxNetCon , pxRxBuffer, ret_recv);
    
    if( pxRxBuffer != NULL && ret_recv == ERR_OK)
    {
         /* Where is the data? */
        netbuf_data( pxRxBuffer, ( void * )&pcRxString, &usLength );
        
//printf("\r\nusLength=%d pcRxString=%s \n", usLength, pcRxString);//debug the post&&get 20150624

	/* Is this a GET?  We don't handle anything else. */
        if( !strncmp( pcRxString, "GET", 3 ) )//GET--get msg from server 20150623
        {
            //printf("\r\nusLength=%d pcRxString=%s \n", usLength, pcRxString);
            pcRxString = cDynamicPage;

            /* Write out the HTTP OK header. */            
            netconn_write( pxNetCon, webHTTP_OK, ( u16_t ) strlen( webHTTP_OK ), NETCONN_COPY );

            /* Generate index.html page. */
            GenerateLoadHtmlPage(cDynamicPage);
 //           GenerateIndexHtmlPage(cDynamicPage, LocalBuf);
            
            /* Write out the dynamically generated page. */
 //           netconn_write( pxNetCon, cDynamicPage, ( u16_t ) strlen( cDynamicPage ), NETCONN_COPY );//WRITE PAGE20150618
            netconn_write( pxNetCon, cDynamicPage, ( u16_t ) strlen( cDynamicPage ), NETCONN_NOCOPY );//WRITE PAGE20150619
            
        }
        else if(!strncmp( pcRxString, "POST", 4 ) )//POST--send msg to server 20150623
        {
            /* Write out the HTTP OK header. */            
            netconn_write( pxNetCon, webHTTP_OK, ( u16_t ) strlen( webHTTP_OK ), NETCONN_COPY );
            bChanged = ProcessPostMessage(pxRxBuffer, LocalBuf);
            

#if CONFIG_READ_FLASH
            if(bChanged || bSTAChanged)
            {
                StoreApInfo();
                StoreNetInfo();
                StoreLoginInfo();
            }
#endif
        }
        netbuf_delete( pxRxBuffer );
//        netbuf_delete(pcRxString);
    }
    netconn_close( pxNetCon );

    if(bChanged)
    {
        struct netconn *pxNewConnection;
        bSTAChanged = 0;
        vTaskDelay(200/portTICK_RATE_MS);
        //printf("\r\n%d:before restart ap\n", xTaskGetTickCount());
        RestartSoftAP();
        //printf("\r\n%d:after restart ap\n", xTaskGetTickCount());
        pxHTTPListener->recv_timeout = 1;		
		// Evan mopdified for adapt two version lwip api diff
        port_netconn_accept( pxHTTPListener , pxNewConnection, ret_accept);
        if( pxNewConnection != NULL && ret_accept == ERR_OK)
        {
            //printf("\r\n%d: got a conn\n", xTaskGetTickCount());
            netconn_close( pxNewConnection );
            while( netconn_delete( pxNewConnection ) != ERR_OK )
            {
                vTaskDelay( webSHORT_DELAY );
            }
        }
        //printf("\r\n%d:end\n", xTaskGetTickCount());
        pxHTTPListener->recv_timeout = 0;		
    }
    
    if(bSTAChanged)
    {             
      is_Init = 1;
       bSTAChanged = 0;
       bChanged = 0;
//       vTaskDelay(2000);
       fATW0(web_set_msg.sta_ssid);
       fATW1(web_set_msg.sta_passwd);
       fATWC(NULL);
       is_Init = 0;
    }
//    netbuf_delete(pcRxString);
}

/*------------------------------------------------------------*/
xTaskHandle webs_task = NULL;
xSemaphoreHandle webs_sema = NULL;
u8_t webs_terminate = 0;
void vBasicWEBServer( void *pvParameters )
{
    
    //struct ip_addr  xIpAddr, xNetMast, xGateway;
    extern err_t ethernetif_init( struct netif *netif );
    int ret = ERR_OK;
    /* Parameters are not used - suppress compiler error. */
    ( void )pvParameters;

    /* Create a new tcp connection handle */
    pxHTTPListener = netconn_new( NETCONN_TCP );
    netconn_bind( pxHTTPListener, NULL, webHTTP_PORT );
    netconn_listen( pxHTTPListener );

#if CONFIG_READ_FLASH
  /* Load wifi_config */
    if(web_set_msg.start_mode == AP)
    {
      LoadWifiConfig();
      RestartSoftAP();
    }
#endif
    //printf("\r\n-0\n");

    /* Loop forever */
    for( ;; )
    {	
        if(webs_terminate)
            break;

        //printf("\r\n%d:-1\n", xTaskGetTickCount());
        /* Wait for connection. */
		// Evan mopdified for adapt two version lwip api diff
        port_netconn_accept( pxHTTPListener , pxNewConnection, ret);
        //printf("\r\n%d:-2\n", xTaskGetTickCount());

        if( pxNewConnection != NULL && ret == ERR_OK)
        {
            /* Service connection. */
            vProcessConnection( pxNewConnection );
            while( netconn_delete( pxNewConnection ) != ERR_OK )
            {
                vTaskDelay( webSHORT_DELAY );
            }
        }
        //printf("\r\n%d:-3\n", xTaskGetTickCount());
    }
    //printf("\r\n-4\n");
    if(pxHTTPListener)
    {
        netconn_close(pxHTTPListener);
        netconn_delete(pxHTTPListener);
        pxHTTPListener = NULL;
    }

    //printf("\r\nExit Web Server Thread!\n");
    xSemaphoreGive(webs_sema);
}

#define STACKSIZE				512
void start_web_server()
{
    	printf("\r\nWEB:Enter start web server!\n");
	webs_terminate = 0;
	if(webs_task == NULL)
	{
		if(xTaskCreate(vBasicWEBServer, (const char *)"web_server", STACKSIZE, NULL, tskIDLE_PRIORITY + 1, &webs_task) != pdPASS)
			printf("\n\rWEB: Create webserver task failed!\n");
	}
	if(webs_sema == NULL)
	{
		webs_sema = xSemaphoreCreateCounting(0xffffffff, 0);	//Set max count 0xffffffff
	}
    	//printf("\r\nWEB:Exit start web server!\n");
}

void stop_web_server()
{
    	//printf("\r\nWEB:Enter stop web server!\n");
	webs_terminate = 1;
    	if(pxHTTPListener)
		netconn_abort(pxHTTPListener);
	if(webs_sema)
	{
		if(xSemaphoreTake(webs_sema, 15 * configTICK_RATE_HZ) != pdTRUE)
		{
			if(pxHTTPListener)
			{
				netconn_close(pxHTTPListener);
  				netconn_delete(pxHTTPListener);
				pxHTTPListener = NULL;
			}
			printf("\r\nWEB: Take webs sema(%p) failed!!!!!!!!!!!\n", webs_sema);
		}
		vSemaphoreDelete(webs_sema);
		webs_sema = NULL;
	}
	if(webs_task)
	{
		vTaskDelete(webs_task);
		webs_task = NULL;
	}
    	printf("\r\nWEB:Exit stop web server!\n");		
}
