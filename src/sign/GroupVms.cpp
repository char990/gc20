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
    if (vSigns.size() != 1)
    {
        MyThrow("VMS: Group can only have ONE sign");
    }
    UciProd &prod = db.GetUciProd();
    for (int i = 0; i < prod.SlavesPerSign(); i++)
    { // slave id = 1~n
        auto s = new Slave(i + 1);
        vSlaves.push_back(s);
        vSigns[0]->AddSlave(s);
    }
    // load process
    auto disp = db.GetUciProcess().GetDisp(groupId);
    if (disp[0] > 0)
    {
        switch (disp[1])
        {
        case static_cast<uint8_t>(MI::CODE::SignDisplayFrame):
            dsNext->dispType = DISP_TYPE::FRM;
            dsNext->fmpid[0] = disp[3];
            break;
        case static_cast<uint8_t>(MI::CODE::SignDisplayMessage):
            dsNext->dispType = DISP_TYPE::MSG;
            dsNext->fmpid[0] = disp[3];
            break;
        default:
            MyThrow("Syntax Error: UciProcess.Group%d.Display", groupId);
            break;
        }
    }
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
    MyThrow("VMS can NOT run ATF");
    return true;
}

void GroupVms::IMakeFrameForSlave(uint8_t uciFrmId)
{
    auto frm = db.GetUciFrm().GetFrm(uciFrmId);
    if (frm == nullptr)
    {
        MyThrow("ERROR: MakeFrameForSlave(frmId=%d): Frm is null", uciFrmId);
    }
    return MakeFrameForSlave(frm);
}

int GroupVms::ITransFrmWithOrBuf(uint8_t uciFrmId, uint8_t *dst)
{
    auto frm = db.GetUciFrm().GetFrm(uciFrmId);
    if (frm == nullptr)
    {
        MyThrow("ERROR: TransFrmWithOrBuf(frmId=%d): Frm is null", uciFrmId);
    }
    return TransFrmWithOrBuf(frm, dst);
}
