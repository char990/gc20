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
	: frmSize(0)
{
}

UciFrm::~UciFrm()
{
	if (frmSize != 0)
	{
		for (int i = 0; i < 255; i++)
		{
			delete[] frms[i].rawData;
		}
	}
}

void UciFrm::LoadConfig()
{
	// using HRGFRM to allocate the memory
	frmSize = DbHelper::Instance().uciProd.MaxFrmLen() + HRGFRM_HEADER_SIZE + 2; // 2 bytes crc
	for (int i = 0; i < 255; i++)
	{
		frms[i].dataLen = 0;
		frms[i].rawData = new uint8_t[frmSize];
	}
	chksum = 0;
	char filename[256];
	uint8_t *b = new uint8_t[frmSize];
	char *v = new char[frmSize * 2 + 1]; // with '\n' at the end
	try
	{
		for (int i = 1; i <= 255; i++)
		{
			snprintf(filename, 255, "%s/frm_%03d", PATH, i);
			int frm_fd = open(filename, O_RDONLY);
			if (frm_fd > 0)
			{
				int len = read(frm_fd, v, frmSize * 2 + 1) - 1;
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
		PrintDbg(e.what());
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
		if (frms[i].dataLen != 0)
		{
			switch (frms[i].rawData[0])
			{
			case MI::CODE::SignSetTextFrame:
				{
					FrmTxt frm(frms[i].rawData, frms[i].dataLen);
					if(frm.appErr==APP::ERROR::AppNoError)
					{
						printf("\tfrm_%03d: %s\n", i+1,  frm.ToString().c_str());
					}
					else
					{
						printf("!!! frm_%03d Error = 0x%02X\n", i+1, frm.appErr);
					}
				}
				break;
			case MI::CODE::SignSetGraphicsFrame:
				{
					FrmGfx frm(frms[i].rawData, frms[i].dataLen);
					if(frm.appErr==APP::ERROR::AppNoError)
					{
						printf("\tfrm_%03d: %s\n", i+1,  frm.ToString().c_str());
					}
					else
					{
						printf("!!! frm_%03d Error = 0x%02X\n", i+1, frm.appErr);
					}
				}
				break;
			case MI::CODE::SignSetHighResolutionGraphicsFrame:
				{
					FrmHrg frm(frms[i].rawData, frms[i].dataLen);
					if(frm.appErr==APP::ERROR::AppNoError)
					{
						printf("\tfrm_%03d: %s\n", i+1,  frm.ToString().c_str());
					}
					else
					{
						printf("!!! frm_%03d Error = 0x%02X\n", i+1, frm.appErr);
					}
				}
				break;
			}
		}
	}
}

uint16_t UciFrm::ChkSum()
{
	return chksum;
}

bool UciFrm::IsFrmDefined(uint8_t i)
{
	return (i != 0 && frms[i - 1].dataLen != 0);
}

StFrm *UciFrm::GetFrm(uint8_t i)
{
	return IsFrmDefined(i) ? &frms[i - 1] : nullptr;
}

uint8_t UciFrm::GetFrmRev(uint8_t i)
{
	return IsFrmDefined(i) ? *(frms[i - 1].rawData + OFFSET_FRM_REV) : 0;
}

APP::ERROR UciFrm::SetFrm(uint8_t *buf, int len)
{
	if (len > frmSize)
		return APP::ERROR::LengthError;
	uint16_t crc;
	if (buf[0] == MI::CODE::SignSetTextFrame)
	{
		FrmTxt frm(buf, len);
		if (frm.appErr != APP::ERROR::AppNoError)
		{
			return frm.appErr;
		}
		crc = frm.crc;
	}
	else if (buf[0] == MI::CODE::SignSetGraphicsFrame)
	{
		FrmGfx frm(buf, len);
		if (frm.appErr != APP::ERROR::AppNoError)
		{
			return frm.appErr;
		}
		crc = frm.crc;
	}
	else if (buf[0] == MI::CODE::SignSetHighResolutionGraphicsFrame)
	{
		FrmHrg frm(buf, len);
		if (frm.appErr != APP::ERROR::AppNoError)
		{
			return frm.appErr;
		}
		crc = frm.crc;
	}
	else if (buf[1] == 0)
	{
		return APP::ERROR::FrmMsgPlnUndefined;
	}
	else
	{
		return APP::ERROR::UnknownMi;
	}
	StFrm *frm = &frms[buf[1] - 1];
	if (frm->dataLen != 0)
	{
		chksum -= Cnvt::GetU16(frm->rawData + frm->dataLen - 2);
	}
	chksum += crc;
	frm->dataLen = len;
	memcpy(frm->rawData, buf, len);
	return APP::ERROR::AppNoError;
}

void UciFrm::SaveFrm(uint8_t i)
{
	if (!IsFrmDefined(i))
		return;
	StFrm *frm = GetFrm(i);
	char filename[256];
	snprintf(filename, 255, "%s/frm_%03d", PATH, i);
	int len = frm->dataLen * 2;
	char *v = new char[len + 1]; // 1 bytes space for '\n'
	Cnvt::ParseToStr(frm->rawData, v, frm->dataLen);
	v[len++] = '\n';
	char buf[64];
	int frm_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0770);
	if (frm_fd < 0)
	{
		snprintf(buf, 63, "Open frm_%03d failed", i);
		DbHelper::Instance().uciAlm.Push(0, buf);
		PrintDbg("%s\n", buf);
	}
	else
	{
		if (write(frm_fd, v, len) != len)
		{
			snprintf(buf, 63, "Write frm_%03d failed", i);
			DbHelper::Instance().uciAlm.Push(0, buf);
			PrintDbg("%s\n", buf);
		}
		close(frm_fd);
		DbHelper::Instance().RefreshSync();
	}
	delete v;
}
