#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <exception>
#include <string>
#include <uci.h>

#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;

UciFrm::UciFrm()
{
	for (int i = 0; i < 255; i++)
	{
		frms[i]=nullptr;
	}
}

UciFrm::~UciFrm()
{
	if (maxFrmSize != 0)
	{
		for (int i = 0; i < 255; i++)
		{
			if(frms[i]!=nullptr)
			{
				delete frms[i];
			}
		}
	}
}

void UciFrm::LoadConfig()
{
	PATH = DbHelper::Instance().Path();
	// using HRGFRM to allocate the memory
	maxFrmSize = DbHelper::Instance().GetUciProd().MaxFrmLen() + HRGFRM_HEADER_SIZE + 2; // 2 bytes crc
	chksum = 0;
	char filename[256];
	uint8_t *b = new uint8_t[maxFrmSize];
	char *v = new char[maxFrmSize * 2 + 1]; // with '\n' at the end
	try
	{
		for (int i = 1; i <= 255; i++)
		{
			snprintf(filename, 255, "%s/frm_%03d", PATH, i);
			int frm_fd = open(filename, O_RDONLY);
			if (frm_fd > 0)
			{
				int len = read(frm_fd, v, maxFrmSize * 2 + 1) - 1;
				if (Cnvt::ParseToU8(v, b, len) == 0)
				{
					SetFrm(b, len / 2);
				}
				close(frm_fd);
			}
		}
	}
	catch (const std::exception &e)
	{
		PrintDbg("%s\n",e.what());
	}
	delete[] v;
	delete[] b;
	Dump();
}

void UciFrm::Dump()
{
    PrintDash();
	printf("%s/frm_xxx\n", PATH);
	for (int i = 0; i < 255; i++)
	{
		if (frms[i] != nullptr)
		{
			printf("\tfrm_%03d: %s\n", i+1,  frms[i]->ToString().c_str());
		}
	}
}

uint16_t UciFrm::ChkSum()
{
	return chksum;
}

bool UciFrm::IsFrmDefined(uint8_t i)
{
	return (i != 0 && frms[i - 1] != nullptr);
}

StFrm *UciFrm::GetStFrm(uint8_t i)
{
	return IsFrmDefined(i) ? &(frms[i - 1]->stFrm) : nullptr;
}

Frame* UciFrm::GetFrm(uint8_t i)
{
	return (i != 0) ? frms[i - 1] : nullptr;
}

uint8_t UciFrm::GetFrmRev(uint8_t i)
{
	return IsFrmDefined(i) ? frms[i - 1]->frmRev : 0;
}

APP::ERROR UciFrm::SetFrm(uint8_t *buf, int len)
{
	if (len > maxFrmSize)
		return APP::ERROR::LengthError;
	Frame *pFrm;
	if (buf[0] == MI::CODE::SignSetTextFrame)
	{
		pFrm = new FrmTxt(buf, len);
	}
	else if (buf[0] == MI::CODE::SignSetGraphicsFrame)
	{
		pFrm = new FrmGfx(buf, len);
	}
	else if (buf[0] == MI::CODE::SignSetHighResolutionGraphicsFrame)
	{
		pFrm = new FrmHrg(buf, len);
	}
	else if (buf[1] == 0)
	{
		return APP::ERROR::FrmMsgPlnUndefined;
	}
	else
	{
		return APP::ERROR::UnknownMi;
	}
	if (pFrm->appErr != APP::ERROR::AppNoError)
	{
		auto r = pFrm->appErr;
		delete pFrm;
		return r;
	}
	auto vFrm = GetFrm(pFrm->frmId);
	if(vFrm!=nullptr)
	{
		chksum -= vFrm->crc;
		delete vFrm;
	}
	chksum -= pFrm->crc;
	frms[pFrm->frmId-1]=pFrm;
	return APP::ERROR::AppNoError;
}

void UciFrm::SaveFrm(uint8_t i)
{
	auto frm = GetStFrm(i);
	if(frm==nullptr)
	{
		return;
	}
	char filename[256];
	snprintf(filename, 255, "%s/frm_%03d", PATH, i);
	int len = frm->dataLen * 2;
	char *v = new char[len + 1]; // 1 bytes space for '\n'
	Cnvt::ParseToStr(frm->rawData, v, frm->dataLen);
	v[len++] = '\n';
	char buf[64];
	int frm_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0770);
	DbHelper & db = DbHelper::Instance();
	if (frm_fd < 0)
	{
		snprintf(buf, 63, "Open frm_%03d failed", i);
		db.GetUciAlarm().Push(0, buf);
		PrintDbg("%s\n", buf);
	}
	else
	{
		if (write(frm_fd, v, len) != len)
		{
			snprintf(buf, 63, "Write frm_%03d failed", i);
			db.GetUciAlarm().Push(0, buf);
			PrintDbg("%s\n", buf);
		}
		fsync(frm_fd);
		close(frm_fd);
	}
	delete v;
}
