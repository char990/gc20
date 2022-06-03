#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <exception>
#include <uci.h>

#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;
using namespace std;

UciFrm::UciFrm()
{
	frms.fill(nullptr);
}

UciFrm::~UciFrm()
{
	for (auto &s : frms)
	{
		if (s != nullptr)
		{
			delete s;
		}
	}
}

void UciFrm::LoadConfig()
{
	Ldebug(">>> Loading 'frames'");
	auto &db = DbHelper::Instance();
	PATH = db.Path();
	auto prodtype = db.GetUciProd().ProdType();
	LoadFrms("%s/frm_%03d");
}

void UciFrm::LoadFrms(const char *FMT)
{
	// using HRGFRM to allocate the memory
	maxFrmSize = DbHelper::Instance().GetUciProd().MaxFrmLen() + HRGFRM_HEADER_SIZE + 2; // 2 bytes crc
	chksum = 0;
	char filename[256];
	uint8_t *b = new uint8_t[maxFrmSize];
	char *v = new char[maxFrmSize * 2 + 1]; // with '\n' or '\0' at the end
	try
	{
		for (int i = 1; i <= 255; i++)
		{
			frms.at(i - 1) = nullptr;
			snprintf(filename, 255, FMT, PATH, i);
			int frm_fd = open(filename, O_RDONLY);
			if (frm_fd > 0)
			{
				int len = read(frm_fd, v, maxFrmSize * 2 + 1);
				close(frm_fd);
				if (len < 9 * 2)
				{
					continue;
				}
				if (v[len - 1] == '\n')
				{
					len--;
				}
				bool fakeCRC = false;
				if (memcmp(v + len - 4, "!!!!", 4) == 0) // if crc is "!!!!", make crc
				{
					memset(v + len - 4, '0', 4);
					fakeCRC = true;
				}
				if (Cnvt::ParseToU8(v, b, len) == 0)
				{
					len /= 2;
					if (fakeCRC)
					{
						Cnvt::PutU16(Crc::Crc16_1021(b, len - 2), b + len - 2);
					}
					SetFrm(b, len);
				}
			}
		}
	}
	catch (const std::exception &e)
	{
		Ldebug("%s", e.what());
	}
	delete[] v;
	delete[] b;
	//Dump();
}

void UciFrm::Dump()
{
	PrintDash('<');
	printf("%s/frm_xxx\n", PATH);
	for (int i = 0; i < 255; i++)
	{
		if (frms[i] != nullptr)
		{
			printf("\tfrm_%03d: %s\n", i + 1, frms[i]->ToString().c_str());
		}
	}
	PrintDash('>');
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

Frame *UciFrm::GetFrm(uint8_t i)
{
	return (i != 0) ? frms[i - 1] : nullptr;
}

uint8_t UciFrm::GetFrmRev(uint8_t i)
{
	return IsFrmDefined(i) ? frms[i - 1]->frmRev : 0;
}

APP::ERROR UciFrm::SetFrm(uint8_t *buf, int len)
{
	Frame *pFrm;
	if (buf[0] == static_cast<uint8_t>(MI::CODE::SignSetTextFrame))
	{
		pFrm = new FrmTxt(buf, len);
	}
	else if (buf[0] == static_cast<uint8_t>(MI::CODE::SignSetGraphicsFrame))
	{
		pFrm = new FrmGfx(buf, len);
	}
	else if (buf[0] == static_cast<uint8_t>(MI::CODE::SignSetHighResolutionGraphicsFrame))
	{
		pFrm = new FrmHrg(buf, len);
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
	// refresh all-frame crc
	auto &vFrm = frms.at(pFrm->frmId - 1);
	if (vFrm != nullptr)
	{
		chksum -= vFrm->crc;
		delete vFrm;
	}
	vFrm = pFrm;
	chksum += vFrm->crc;
	return APP::ERROR::AppNoError;
}

void UciFrm::SaveFrm(uint8_t i)
{
	auto stfrm = GetStFrm(i);
	if (stfrm == nullptr)
	{
		return;
	}
	char filename[256];
	// save uci format
	snprintf(filename, 255, "%s/frm_%03d", PATH, i);
	int len = stfrm->rawData.size() * 2;
	char *v = new char[len + 1]; // 1 bytes space for '\n'
	Cnvt::ParseToStr(stfrm->rawData.data(), v, stfrm->rawData.size());
	v[len++] = '\n';
	char buf[64];
	int frm_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	auto &alm = DbHelper::Instance().GetUciAlarm();
	if (frm_fd < 0)
	{
		snprintf(buf, 63, "Open frm_%03d failed", i);
		alm.Push(0, buf);
		Ldebug("%s", buf);
	}
	else
	{
		if (write(frm_fd, v, len) != len)
		{
			snprintf(buf, 63, "Write frm_%03d failed", i);
			alm.Push(0, buf);
			Ldebug("%s", buf);
		}
		fdatasync(frm_fd);
		close(frm_fd);
	}
	delete[] v;
}

void UciFrm::Reset()
{
	for (auto &s : frms)
	{
		if (s != nullptr)
		{
			delete s;
		}
	}
	frms.fill(nullptr);
	chksum = 0;
	Exec::Shell("rm %s/frm* > /dev/null 2>&1", PATH);
}

bool UciFrm::IsFrmFlashing(uint8_t i)
{
	if (!IsFrmDefined(i))
	{
		return false;
	}
	return frms[i - 1]->conspicuity != 0;
}

