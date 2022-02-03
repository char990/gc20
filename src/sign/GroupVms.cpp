#include <cstring>
#include <sign/GroupVms.h>
#include <sign/Slave.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>
#include <module/MyDbg.h>

using namespace Utils;

GroupVms::GroupVms(uint8_t id)
    : Group(id)
{
    if (SignCnt() != 1)
    {
        throw std::invalid_argument("VMS: Group can only have ONE sign");
    }
    UciProd &prod = db.GetUciProd();
    vSlaves.resize(prod.SlavesPerSign());
    for (int i = 0; i < prod.SlavesPerSign(); i++)
    { // slave id = 1~n
        auto s = new Slave(i + 1);
        vSlaves.at(i) = s;
        vSigns.at(0)->AddSlave(s);
    }
    LoadLastDisp();
}

GroupVms::~GroupVms()
{
}

void GroupVms::PeriodicHook()
{
}

APP::ERROR GroupVms::DispAtomicFrm(uint8_t *cmd)
{
    return APP::ERROR::MiNotSupported;
}

bool GroupVms::TaskSetATF(int *_ptLine)
{
    throw std::runtime_error("VMS can NOT run ATF");
    return true;
}

void GroupVms::IMakeFrameForSlave(uint8_t uciFrmId)
{
    auto frm = db.GetUciFrm().GetFrm(uciFrmId);
    if (frm == nullptr)
    {
        throw std::invalid_argument(FmtException("ERROR: MakeFrameForSlave(frmId=%d): Frm is null", uciFrmId));
    }
    MakeFrameForSlave(frm);
}

int GroupVms::ITransFrmWithOrBuf(uint8_t uciFrmId, uint8_t *dst)
{
    auto frm = db.GetUciFrm().GetFrm(uciFrmId);
    if (frm == nullptr)
    {
        throw std::invalid_argument(FmtException("ERROR: TransFrmWithOrBuf(frmId=%d): Frm is null", uciFrmId));
    }
    return TransFrmWithOrBuf(frm, dst);
}
