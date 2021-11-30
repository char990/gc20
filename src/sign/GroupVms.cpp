#include <cstring>
#include <sign/GroupVms.h>
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
    for (int i = 0; i < prod.SlaveRowsPerSign() * prod.SlaveColumnsPerSign(); i++)
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
    MyThrow("VMS can not run ATF");
    return true;
}

// TODO ToSlaveFormat with orBuf
void GroupVms::MakeFrameForSlave(uint8_t uciFrmId)
{
    auto frm = db.GetUciFrm().GetFrm(uciFrmId);
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

/*
if (msgOverlay == 0)
{
    *p++ = 0x0A; // Text frame
    p++;         // skip slave frame id
    uint8_t font = (frm->font == 0) ? user.DefaultFont() : frm->font;
    *p++ = font;
    *p++ = (frm->colour==0) ? prod.GetMappedColour(user.DefaultColour()) : frm->colour;
    *p++ = frm->conspicuity;
    auto pFont = prod.Fonts(font);
    *p++ = pFont->CharSpacing();
    *p++ = pFont->LineSpacing();
    *p++ = frm->frmBytes;
    memcpy(p, frm->stFrm.rawData + frm->frmOffset, frm->frmBytes);
    p += frm->frmBytes;
}
*/

int GroupVms::TransFrmWithOrBuf(uint8_t uciFrmId, uint8_t *dst)
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
