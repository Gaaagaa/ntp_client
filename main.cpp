#include "ntp_client.h"
#include <iostream>
#include <string>
#include <WinSock2.h>
#include <errno.h>
using namespace std;
//NTP�������б�
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
		case WSASYSNOTREADY:cout << "����������ϵͳ��δ׼���ý�������ͨ�š�" << endl; break;
		case WSAVERNOTSUPPORTED:cout << "���ض���Windows�׽���ʵ��δ�ṩ�������Windows�׽���֧�ֵİ汾��" << endl; break;
		case WSAEINPROGRESS:cout << "Windows Sockets 1.1����ֹ�������ڽ����С�" << endl; break;
		case WSAEPROCLIM:cout << "Windows�׽���ʵ����֧�ֵ��������Ѵﵽ���ơ�" << endl; break;
		case WSAEFAULT:cout << "lpWSAData ����������Ч��ָ�롣" << endl; break;
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
	//���������׽��ֹ���
	WSADATA xwsa_data;
	if (!OpenWSA(xwsa_data))
	{
		cout << "�������󼴽��˳�!" << endl;
		system("pause");
		exit(1);
	}
	//��ʼ��NTP��������
	xntp_cliptr_t xntp_this = X_NULL;
	xntp_this = ntpcli_open();//��ʼ��
	if (X_NULL == xntp_this)//�ж��Ƿ��ʼ���ɹ�
	{
		cout << "����:" << errno << endl;
		system("pause");
	}
	//������Ϣ
	int host = 0;
	ntpcli_config(xntp_this, xszt_host[host], NTP_PORT);
	//��������
	xtime_vnsec_t xtm_vnsec = ntpcli_req_time(xntp_this, 3000);//��ȡ����ʱ��
	for (size_t i = 1; i <= 3; i++)
	{
		if (XTMVNSEC_IS_VALID(xtm_vnsec))
		{
			xtime_vnsec_t xtm_ltime = time_vnsec();//��ȡϵͳʱ��
			xtime_descr_t xtm_local = time_vtod(xtm_ltime);//ת��ϵͳʱ��
			xtime_descr_t xtm_descr = time_vtod(xtm_vnsec);//ת������ʱ��
			//��ʾʱ��
			cout << "����ʱ��:[ " << xtm_descr.ctx_year << "��" << xtm_descr.ctx_month << "��" << xtm_descr.ctx_day << "�� ����" << xtm_descr.ctx_week << " " << xtm_descr.ctx_hour << "Сʱ" << xtm_descr.ctx_minute << "����" << xtm_descr.ctx_second << "��" << xtm_descr.ctx_msec << "����" << xtm_vnsec % 10000000 << "΢�� ]\n";
			cout << "ϵͳʱ��:[ " << xtm_local.ctx_year << "��" << xtm_local.ctx_month << "��" << xtm_local.ctx_day << "�� ����" << xtm_local.ctx_week << " " << xtm_local.ctx_hour << "Сʱ" << xtm_local.ctx_minute << "����" << xtm_local.ctx_second << "��" << xtm_local.ctx_msec << "����" << xtm_ltime % 10000000 << "΢�� ]\n";

			break;
		}
		else
		{
			cout << "��������:" << errno << endl;
			cout << "NTP������IP:" << xszt_host[host++];
			cout << "�Ժ�����..." << endl;
			Sleep(5000);
			cout << "[" << i << "]��������..." << endl;
		}
	}

	return 0;
}