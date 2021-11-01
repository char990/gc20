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
    uint8_t *p = txBuf + 1;
    auto &prod = db.GetUciProd();
    auto &user = db.GetUciUser();
    auto xfrm = db.GetUciFrm().GetFrm(uciFrmId);
    if (xfrm == nullptr)
    {
        MyThrow("ERROR: MakeFrameForSlave(frmId=%d): Frm is null", uciFrmId);
    }
    if (xfrm->micode == static_cast<uint8_t>(MI::CODE::SignSetTextFrame))
    {
        FrmTxt *frm = dynamic_cast<FrmTxt *>(xfrm);
        if (msgOverlay == 0)
        {
            *p++ = 0x0A; // Text frame
            p++;         // skip slave frame id
            uint8_t font = (frm->font == 0) ? user.DefaultFont() : frm->font;
            *p++ = font;
            *p++ = frm->colour;
            *p++ = frm->conspicuity;
            auto pFont = prod.Fonts(font);
            *p++ = pFont->CharSpacing();
            *p++ = pFont->LineSpacing();
            *p++ = frm->frmBytes;
            memcpy(p, frm->stFrm.rawData + frm->frmOffset, frm->frmBytes);
            p += frm->frmBytes;
        }
        else if (msgOverlay == 1)
        {
        }
        else if (msgOverlay == 4)
        {
        }
        else // if(msgOverlay==24)
        {
        }
    }
    else if (xfrm->micode == static_cast<uint8_t>(MI::CODE::SignSetGraphicsFrame) ||
             xfrm->micode == static_cast<uint8_t>(MI::CODE::SignSetHighResolutionGraphicsFrame))
    {
        auto frm = xfrm;
        if (msgOverlay == 0)
        {
            *p++ = 0x0B; // Gfx frame
            p++;         // skip slave frame id
            *p++ = prod.PixelRows();
            p = Cnvt::PutU16(prod.PixelColumns(), p);
            *p++ = frm->colour;
            *p++ = frm->conspicuity;
            p = Cnvt::PutU16(frm->frmBytes, p);
            memcpy(p, frm->stFrm.rawData + frm->frmOffset, frm->frmBytes);
            p += frm->frmBytes;
        }
        else if (msgOverlay == 1)
        {
        }
        else if (msgOverlay == 4)
        {
        }
        else // if(msgOverlay==24)
        {
        }
    }
    else
    {
        MyThrow("ERROR: Unknown MIcode of Frame(frmId=%d, micode=0x%02X)", uciFrmId, xfrm->micode);
    }
    txLen = p - txBuf;
}

void GroupVms::TransFrmToOrBuf(uint8_t frmId)
{
}
