#include <windows.h>
#pragma comment(lib, "Netapi32.lib")
#include <iostream>

namespace
{
	bool GetAdapterInfo(int adapterNum, std::string& macOUT)
	{
		NCB Ncb;
		memset(&Ncb, 0, sizeof(Ncb));
		Ncb.ncb_command = NCBRESET; // 重置网卡，以便我们可以查询
		Ncb.ncb_lana_num = adapterNum;
		if (Netbios(&Ncb) != NRC_GOODRET)
			return false;

		// 准备取得接口卡的状态块
		memset(&Ncb, sizeof(Ncb), 0);
		Ncb.ncb_command = NCBASTAT;
		Ncb.ncb_lana_num = adapterNum;
		strcpy((char*)Ncb.ncb_callname, "*");
		struct ASTAT
		{
			ADAPTER_STATUS adapt;
			NAME_BUFFER nameBuff[30];
		}adapter;
		memset(&adapter, sizeof(adapter), 0);
		Ncb.ncb_buffer = (unsigned char*)&adapter;
		Ncb.ncb_length = sizeof(adapter);
		if (Netbios(&Ncb) != 0)
			return false;

		char acMAC[32];
		sprintf(acMAC, "%02X-%02X-%02X-%02X-%02X-%02X",
			int(adapter.adapt.adapter_address[0]),
			int(adapter.adapt.adapter_address[1]),
			int(adapter.adapt.adapter_address[2]),
			int(adapter.adapt.adapter_address[3]),
			int(adapter.adapt.adapter_address[4]),
			int(adapter.adapt.adapter_address[5]));
		macOUT = acMAC;
		return true;
	}
}

bool GetMacByNetBIOS(std::string& macOUT)
{
	// 取得网卡列表
	LANA_ENUM adapterList;
	NCB Ncb;
	memset(&Ncb, 0, sizeof(NCB));
	Ncb.ncb_command = NCBENUM;
	Ncb.ncb_buffer = (unsigned char*)&adapterList;
	Ncb.ncb_length = sizeof(adapterList);
	Netbios(&Ncb);

	// 取得MAC
	for (int i = 0; i < adapterList.length; ++i)
	{
		if (GetAdapterInfo(adapterList.lana[i], macOUT))
			return true;
	}

	return false;
}