#include <cstring>
#include <sign/GroupIslus.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <module/ptcpp.h>
#include <module/Utils.h>
using namespace Utils;

GroupIslus::GroupIslus(uint8_t id)
    : Group(id)
{
    if (vSlaves.size() != 1)
    {
        MyThrow("ISLUS: Sign can only have ONE slave");
    }

    for (int i = 0; i < vSigns.size(); i++)
    { // slave id = Sign Id
        vSlaves.push_back(new Slave(vSigns[i]->SignId()));
    }
    // load process
    auto disp = db.GetUciProcess().GetDisp(groupId);
    if (disp[0] > 0)
    {
        switch (disp[1])
        {
        case MI::CODE::SignDisplayFrame:
            DispFrm(disp[3]);
            break;
        case MI::CODE::SignDisplayMessage:
            DispMsg(disp[3]);
            break;
        case MI::CODE::SignDisplayAtomicFrames:
            DispAtomicFrm(&disp[1]);
            break;
        default:
            MyThrow("Syntax Error: UciProcess.Group%d.Display", groupId);
            break;
        }
    }
}

GroupIslus::~GroupIslus()
{
}

void GroupIslus::PeriodicHook()
{
}

APP::ERROR GroupIslus::DispAtomicFrm(uint8_t *cmd)
{
#if 0
    if (FacilitySwitch::FS_STATE::AUTO != fcltSw.Get())
    {
        return APP::ERROR::FacilitySwitchOverride;
    }
    if (cmd[2] != signCnt)
    {
        return APP::ERROR::SyntaxError;
    }
    uint8_t *p = cmd + 3;
    for (int i = 0; i < signCnt; i++)
    {
        // check if there is a duplicate sign id
        uint8_t *p2 = p + 2;
        for (int j = i + 1; j < signCnt; j++)
        {
            if (*p == *p2)
            {
                return APP::ERROR::SyntaxError;
            }
            p2 += 2;
        }
        // sign is in this group
        if (!IsSignInGroup(*p))
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        p++;
        // frm is defined or frm0
        if ((*p != 0) && !DbHelper::Instance().GetUciFrm().IsFrmDefined(*p))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
        p++;
    }
    db.GetUciProcess().SetDisp(groupId, cmd, 3+signCnt*2);
    dsNext->dispType = DISP_STATUS::TYPE::ATF;
    p = cmd + 3;
    for (int i = 0; i < signCnt; i++)
    {
        dsNext->fmpid[i] = 0;
        for (int j = 0; i < signCnt; j++)
        {
            if (*p == vSigns[i]->SignId())
            { // matched sign id
                dsNext->fmpid[i] = *(p + 1);
                break;
            }
        }
        p += 2;
    }
#endif
    return APP::ERROR::AppNoError;
}

// TODO set slave frame, active frm, reprot frm
bool GroupIslus::TaskSetATF(int *_ptLine)
{
    PT_BEGIN();
    for (sATF = 0; sATF < vSigns.size(); sATF++)
    {
        do
        {
            //SetFrame(vSigns[sATF]->SignId(), , onDispFrmId); // SignId is same as SlaveId
            ClrAllSlavesRxStatus();
            PT_WAIT_UNTIL(CheckAllSlavesNext() >= 0);
        } while (allSlavesNext == 1);
    }
    PT_END();
}

void GroupIslus::MakeFrameForSlave(uint8_t fid)
{

}

void GroupIslus::TransFrmToOrBuf(uint8_t frmId)
{

}
