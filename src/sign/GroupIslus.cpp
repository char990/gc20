#include <cstring>
#include <sign/GroupIslus.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <module/ptcpp.h>
#include <module/Utils.h>

using namespace Utils;

#define UP_LEFT_0 182
#define UP_RIGHT_0 183
#define DN_LEFT_0 184
#define DN_RIGHT_0 185
#define RED_CROSS_0 189

#define UP_LEFT_1 192
#define UP_RIGHT_1 193
#define DN_LEFT_1 194
#define DN_RIGHT_1 195
#define RED_CROSS_1 199

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
        case static_cast<uint8_t>(MI::CODE::SignDisplayFrame):
            DispFrm(disp[3]);
            break;
        case static_cast<uint8_t>(MI::CODE::SignDisplayMessage):
            DispMsg(disp[3]);
            break;
        case static_cast<uint8_t>(MI::CODE::SignDisplayAtomicFrames):
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
    uint8_t speed_frm_id=0;
    for (int i = 0; i < signCnt; i++)
    {
        auto sign_id = *p++;
        auto frm_id = *p++;
        // check if there is a duplicate sign id
        auto p2 = p;
        for (int j = i + 1; j < signCnt; j++)
        {
            if (sign_id == *p2)
            {
                return APP::ERROR::SyntaxError;
            }
            p2 += 2;
        }
        // sign is in this group
        if (!IsSignInGroup(sign_id))
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        // frm is defined
        if (!DbHelper::Instance().GetUciFrm().IsFrmDefined(*p))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
        // check if same speed
        if(IsSpeedFrame(frm_id))
        {
            if(speed_frm_id==0)
            {
                speed_frm_id=frm_id;
            }
            else
            {
                if(speed_frm_id!=frm_id)
                {
                    return APP::ERROR::SyntaxError;
                }
            }
        }
        // check if test frames: 250,251,252, not allowed
        if(frm_id>=250&&frm_id<=252)
        {
            return APP::ERROR::SyntaxError;
        }
        // check lane merge or 
        if(i < signCnt-1)
        {
            uint8_t frm_r = *(p+1);
            if(((frm_id==DN_RIGHT_0 || frm_id==DN_RIGHT_1) && 
                    (frm_r==RED_CROSS_0 || frm_r==RED_CROSS_1 || frm_r==DN_LEFT_0 || frm_r==DN_LEFT_1)) ||
                ((frm_id==RED_CROSS_0 || frm_id==RED_CROSS_1) && (frm_r==DN_LEFT_0 || frm_r==DN_LEFT_1)) )
            {// merge to closed lane
                return APP::ERROR::SyntaxError;
            }
        }
        // reject frames
        auto & signCfg = db.GetUciProd().GetSignCfg(sign_id);
        if(signCfg.rjctFrm.Get(frm_id))
        {
            return APP::ERROR::SyntaxError;
        }
    }
    db.GetUciProcess().SetDisp(groupId, cmd, 3+signCnt*2);
    dsNext->dispType = DISP_TYPE::ATF;
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

bool GroupIslus::IsSpeedFrame(uint8_t frmId)
{
    if (frmId >= 25 && frmId <= 29)
    {
        return true;
    }
    uint8_t ab = frmId / 10;
    uint8_t c = frmId % 10;
    return (ab >= 1 && ab <= 11 && c >= 0 && c <= 4);
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

void GroupIslus::IMakeFrameForSlave(uint8_t uciFrmId)
{
    if (uciFrmId == 189 || uciFrmId == 199)
    { // Red Cross "X"
    }
    else
    {
        Frame *frm = db.GetUciFrm().GetIslusFrm(uciFrmId);
        if (frm == nullptr)
        {
            MyThrow("ERROR: MakeFrameForSlave(frmId=%d): Frm is null", uciFrmId);
        }
        MakeFrameForSlave(frm);
    }
}

int GroupIslus::ITransFrmWithOrBuf(uint8_t uciFrmId, uint8_t *dst)
{
    Frame *frm = db.GetUciFrm().GetIslusFrm(uciFrmId);
    if (frm == nullptr)
    {
        MyThrow("ERROR: TransFrmWithOrBuf(frmId=%d): Frm is null", uciFrmId);
    }
    return TransFrmWithOrBuf(frm, dst);
}
