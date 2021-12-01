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

void GroupIslus::MakeFrameForSlave(uint8_t uciFrmId)
{
    auto frm = db.GetUciFrm().GetIslusFrm(uciFrmId);
    if (frm == nullptr)
    {
        MyThrow("ERROR: MakeFrameForSlave(frmId=%d): Frm is null", uciFrmId);
    }
    auto &prod = db.GetUciProd();
    auto &user = db.GetUciUser();
    uint8_t *p = txBuf + 1;
    *p++ = 0x0B; // Gfx frame
    p++;         // skip slave frame id
    *p++ = prod.PixelRows();
    p = Cnvt::PutU16(prod.PixelColumns(), p);
    auto mappedcolour = (frm->colour == 0) ? prod.GetMappedColour(user.DefaultColour()) : frm->colour;
    if (msgOverlay == 0 || msgOverlay == 1)
    {
        *p++ = mappedcolour;
    }
    else if (msgOverlay == 4)
    {
        *p++ = (uint8_t)FRMCOLOUR::MultipleColours;
    }
    *p++ = frm->conspicuity;
    int frmlen = TransFrmWithOrBuf(uciFrmId, p+2);
    p = Cnvt::PutU16(frmlen, p);
    txLen = p + frmlen - txBuf;
}

int GroupIslus::TransFrmWithOrBuf(uint8_t uciFrmId, uint8_t *dst)
{
    auto frm = db.GetUciFrm().GetFrm(uciFrmId);
    if (frm == nullptr)
    {
        MyThrow("ERROR: TransFrmWithOrBuf(frmId=%d): Frm is null", uciFrmId);
    }
    auto &prod = db.GetUciProd();
    int frmlen;
    if (msgOverlay == 0)
    {
        frmlen = (frm->micode == static_cast<uint8_t>(MI::CODE::SignSetTextFrame)) ? prod.Gfx1FrmLen() : frm->frmBytes;
    }
    else if (msgOverlay == 1)
    {
        frmlen = prod.Gfx1FrmLen();
    }
    else if (msgOverlay == 4)
    {
        frmlen = prod.Gfx4FrmLen();
    }
    else
    {
        // TODO: 24-bit
    }
    uint8_t * orsrc=dst;
    if (frm->micode == static_cast<uint8_t>(MI::CODE::SignSetTextFrame))
    {
        FrmTxt *txtfrm = static_cast<FrmTxt *>(frm);
        if (txtfrm != nullptr)
        {
            txtfrm->ToBitmap(msgOverlay, dst);
        }
        else
        {
            MyThrow("ERROR: TransFrmWithOrBuf(frmId=%d): dynamic_cast<FrmTxt *> failed", uciFrmId);
        }
    }
    else// if (frm->micode == static_cast<uint8_t>(MI::CODE::SignSetGraphicsFrame) ||
        //     frm->micode == static_cast<uint8_t>(MI::CODE::SignSetHighResolutionGraphicsFrame))
    {
        if (msgOverlay == 0)
        { // colour may be 0/mono/multi/RGB
            memcpy(dst, frm->stFrm.rawData + frm->frmOffset, frmlen);
        }
        else
        {
            if ((msgOverlay == 1 && frm->colour < (uint8_t)FRMCOLOUR::MonoFinished) ||
                (msgOverlay == 4 && frm->colour == (uint8_t)FRMCOLOUR::MultipleColours))
            {// just set orsrc, do not memcpy
                orsrc = frm->stFrm.rawData + frm->frmOffset;
            }
            else if (msgOverlay == 4 && frm->colour < (uint8_t)FRMCOLOUR::MonoFinished)
            { // 1-bit -> 4-bit
                frm->ToBitmap(msgOverlay, dst);
            }
        }
    }
    if (msgOverlay > 0)
    {// with overlay
        SetWithOrBuf(dst, orsrc, frmlen);
    }
    return frmlen;
}
