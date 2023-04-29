#include "ntp_client.h"
#include <iostream>
#include <string>
#include <WinSock2.h>
#include <errno.h>
using namespace std;
//NTP服务器列表
x_cstring_t xszt_host[] = {
	"ntp.tencent.com" ,
	"ntp1.tencent.com",
	"ntp2.tencent.com",
	"ntp3.tencent.com",
	"ntp4.tencent.com",
	"ntp5.tencent.com",
	"ntp1.aliyun.com" ,
	"ntp2.aliyun.com" ,
	"ntp3.aliyun.com" ,
	"ntp4.aliyun.com" ,
	"ntp5.aliyun.com" ,
	"ntp6.aliyun.com" ,
	"ntp7.aliyun.com" ,
	"time.edu.cn"     ,
	"s2c.time.edu.cn" ,
	"s2f.time.edu.cn" ,
	"s2k.time.edu.cn" ,
	X_NULL };
[[nodiscard]] bool OpenWSA(WSADATA& xwsa_data)
{
	int nRes = WSAStartup(MAKEWORD(2, 0), &xwsa_data);
	if (nRes != 0)
	{
		switch (nRes)
		{
		case WSASYSNOTREADY:cout << "基础网络子系统尚未准备好进行网络通信。" << endl; break;
		case WSAVERNOTSUPPORTED:cout << "此特定的Windows套接字实现未提供所请求的Windows套接字支持的版本。" << endl; break;
		case WSAEINPROGRESS:cout << "Windows Sockets 1.1的阻止操作正在进行中。" << endl; break;
		case WSAEPROCLIM:cout << "Windows套接字实现所支持的任务数已达到限制。" << endl; break;
		case WSAEFAULT:cout << "lpWSAData 参数不是有效的指针。" << endl; break;
		default:break;
		}
		return false;
	}
	else
	{
		return true;
	}
}
int main()
{
	//启用网络套接字功能
	WSADATA xwsa_data;
	if (!OpenWSA(xwsa_data))
	{
		cout << "遇到错误即将退出!" << endl;
		system("pause");
		exit(1);
	}
	//初始化NTP工作对象
	xntp_cliptr_t xntp_this = X_NULL;
	xntp_this = ntpcli_open();//初始化
	if (X_NULL == xntp_this)//判断是否初始化成功
	{
		cout << "错误:" << errno << endl;
		system("pause");
	}
	//设置信息
	int host = 0;
	ntpcli_config(xntp_this, xszt_host[host], NTP_PORT);
	//发送请求
	xtime_vnsec_t xtm_vnsec = ntpcli_req_time(xntp_this, 3000);//获取网络时间
	for (size_t i = 1; i <= 3; i++)
	{
		if (XTMVNSEC_IS_VALID(xtm_vnsec))
		{
			xtime_vnsec_t xtm_ltime = time_vnsec();//获取系统时间
			xtime_descr_t xtm_local = time_vtod(xtm_ltime);//转换系统时间
			xtime_descr_t xtm_descr = time_vtod(xtm_vnsec);//转换网络时间
			//显示时间
			cout << "网络时间:[ " << xtm_descr.ctx_year << "年" << xtm_descr.ctx_month << "月" << xtm_descr.ctx_day << "日 星期" << xtm_descr.ctx_week << " " << xtm_descr.ctx_hour << "小时" << xtm_descr.ctx_minute << "分钟" << xtm_descr.ctx_second << "秒" << xtm_descr.ctx_msec << "毫秒" << xtm_vnsec % 10000000 << "微秒 ]\n";
			cout << "系统时间:[ " << xtm_local.ctx_year << "年" << xtm_local.ctx_month << "月" << xtm_local.ctx_day << "日 星期" << xtm_local.ctx_week << " " << xtm_local.ctx_hour << "小时" << xtm_local.ctx_minute << "分钟" << xtm_local.ctx_second << "秒" << xtm_local.ctx_msec << "毫秒" << xtm_ltime % 10000000 << "微秒 ]\n";

			break;
		}
		else
		{
			cout << "遇到错误:" << errno << endl;
			cout << "NTP服务器IP:" << xszt_host[host++];
			cout << "稍后重试..." << endl;
			Sleep(5000);
			cout << "[" << i << "]正在重试..." << endl;
		}
	}

	return 0;
}