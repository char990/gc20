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
	auto prodtype = db.GetUciHardware().ProdType();
	LoadFrms("%s/frm_%03d");
}

void UciFrm::LoadFrms(const char *FMT)
{
	// using HRGFRM to allocate the memory
	maxFrmSize = DbHelper::Instance().GetUciHardware().MaxCoreLen() + HRGFRM_HEADER_SIZE + 2; // 2 bytes crc
	chksum = 0;
	char filename[PRINT_BUF_SIZE];
	vector<uint8_t> b(maxFrmSize);
	vector<char> v(maxFrmSize * 2 + 1); // with '\n' or '\0' at the end
	try
	{
		for (int i = 1; i <= 255; i++)
		{
			snprintf(filename, PRINT_BUF_SIZE - 1, FMT, PATH, i);
			int frm_fd = open(filename, O_RDONLY);
			if (frm_fd > 0)
			{
				int len = read(frm_fd, v.data(), maxFrmSize * 2 + 1);
				close(frm_fd);
				if (len < 9 * 2)
				{
					continue;
				}
				if (v[len - 1] == '\n')
				{
					len--;
				}
				if (Cnvt::ParseToU8(v.data(), b.data(), len) == 0)
				{
					len /= 2;
					if (b.at(len - 2) == 0 && b.at(len - 1) == 0) // if crc is 0x0000, make crc
					{
						Cnvt::PutU16(Crc::Crc16_1021(b.data(), len - 2), b.data() + len - 2);
					}
					SetFrm(b.data(), len);
				}
			}
		}
	}
	catch (const std::exception &e)
	{
		Ldebug("%s", e.what());
	}
	// Dump();
}

void UciFrm::Dump()
{
	PrintDash('<');
	printf("%s/frm_xxx\n", PATH);
	for (int i = 1; i <= 255; i++)
	{
		if (frms[i] != nullptr)
		{
			printf("\tfrm_%03d: %s\n", i, frms[i]->ToString().c_str());
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
	return (frms[i] != nullptr);
}

StFrm *UciFrm::GetStFrm(uint8_t i)
{
	return IsFrmDefined(i) ? &(frms[i]->stFrm) : nullptr;
}

Frame *UciFrm::GetFrm(uint8_t i)
{
	return frms[i];
}

uint8_t UciFrm::GetFrmRev(uint8_t i)
{
	return IsFrmDefined(i) ? frms[i]->frmRev : 0;
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
	auto &vFrm = frms.at(pFrm->frmId);
	if (pFrm->frmId > 0)
	{
		if (vFrm != nullptr)
		{
			chksum -= vFrm->crc;
			delete vFrm;
		}
		chksum += pFrm->crc;
	}
	vFrm = pFrm;
	return APP::ERROR::AppNoError;
}

void UciFrm::SaveFrm(uint8_t i)
{
	auto stfrm = GetStFrm(i);
	if (stfrm == nullptr || i == 0)
	{
		return;
	}
	char filename[PRINT_BUF_SIZE];
	// save uci format
	snprintf(filename, PRINT_BUF_SIZE - 1, "%s/frm_%03d", PATH, i);
	vector<char> v(stfrm->rawData.size() * 2 + 1);// 1 bytes space for '\n'
	Cnvt::ParseToStr(stfrm->rawData.data(), v.data(), stfrm->rawData.size());
	v.back() = '\n';
	char buf[STRLOG_SIZE];
	int frm_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	auto &alm = DbHelper::Instance().GetUciAlarm();
	if (frm_fd < 0)
	{
		snprintf(buf, STRLOG_SIZE - 1, "Open frm_%03d failed", i);
		alm.Push(0, buf);
		Ldebug(buf);
	}
	else
	{
		if (write(frm_fd, v.data(), v.size()) != v.size())
		{
			snprintf(buf, STRLOG_SIZE - 1, "Write frm_%03d failed", i);
			alm.Push(0, buf);
			Ldebug(buf);
		}
		fdatasync(frm_fd);
		close(frm_fd);
	}
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
	return frms[i]->conspicuity != 0;
}
