#include <cstring>
#include <sign/GroupIslus.h>
#include <sign/Slave.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <module/ptcpp.h>
#include <module/Utils.h>

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

using namespace Utils;

GroupIslus::GroupIslus(uint8_t id)
    : Group(id)
{
    UciProd &prod = db.GetUciProd();
    if (prod.SlavesPerSign() != 1)
    {
        MyThrow("ISLUS: Sign can only have ONE slave");
    }
    vSlaves.resize(SignCnt());
    for (int i = 0; i < SignCnt(); i++)
    {
        auto sign = vSigns.at(i);
        auto slv = new Slave(sign->SignId()); // slave id = Sign Id
        vSlaves.at(i) = slv;
        sign->AddSlave(slv);
    }

    atfSt.assign(SignCnt(), 1); // resize atfSt and init
    LoadLastDisp();
}

GroupIslus::~GroupIslus()
{
}

void GroupIslus::PeriodicHook()
{
}

APP::ERROR GroupIslus::DispAtomicFrm(uint8_t *cmd)
{
    uint8_t signCnt = cmd[2];
    if (signCnt != SignCnt())
    {
        return APP::ERROR::SyntaxError;
    }
    uint8_t *pnext = cmd + 3;
    uint8_t speed_frm_id = 0;
    DispStatus ds{signCnt};
    ds.dispType = DISP_TYPE::ATF;
    for (int i = 0; i < signCnt; i++)
    {
        auto sign_id = *pnext++;
        auto frm_id = *pnext++;
        // check if there is a duplicate sign id
        auto p2 = pnext;
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
        if (!DbHelper::Instance().GetUciFrm().IsFrmDefined(frm_id))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
        // check if same speed
        if (IsSpeedFrame(frm_id))
        {
            if (speed_frm_id == 0)
            { // first speed frame, mark it
                speed_frm_id = frm_id;
            }
            else
            {
                if (speed_frm_id != frm_id)
                { // have differrent speed frames
                    return APP::ERROR::SyntaxError;
                }
            }
        }
        // reject frames => check exit
        if (db.GetUciProd().GetSignCfg(sign_id).rjctFrm.GetBit(frm_id))
        {
            return APP::ERROR::SyntaxError;
        }
        // check lane merge
        if (i < signCnt - 1)
        {
#define IS_LEFT(frm) (frm == DN_LEFT_0 || frm == DN_LEFT_1)
#define IS_RIGHT(frm) (frm == DN_RIGHT_0 || frm == DN_RIGHT_1)
#define IS_CROSS(frm) (frm == RED_CROSS_0 || frm == RED_CROSS_1)
            uint8_t frm_next = pnext[1];
            if ((IS_RIGHT(frm_id) && (IS_CROSS(frm_next) || IS_LEFT(frm_next))) ||
                (IS_CROSS(frm_id) && IS_LEFT(frm_next)))
            { // merge to closed lane
                return APP::ERROR::SyntaxError;
            }
#undef IS_CROSS
#undef IS_RIGHT
#undef IS_LEFT
        }
        // all good
        for (int j = 0; j < signCnt; j++)
        {
            if (sign_id == vSigns.at(j)->SignId())
            { // matched sign id
                ds.fmpid[j] = frm_id;
                break;
            }
        }
    }
    dsNext->Clone(&ds);
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

void GroupIslus::TaskSetATFReset()
{
    taskATFLine = 0;
    atfSt.assign(atfSt.size(), 1);
}

bool GroupIslus::TaskSetATF(int *_ptLine)
{
    PT_BEGIN();
    while (1)
    {
        for (sATF = 0; sATF < SlaveCnt(); sATF++)
        {
            if (atfSt.at(sATF) != 0)
            {
                SlaveSetFrame(vSlaves.at(sATF)->SlaveId() /*signId=slaveId */, 1, dsCurrent->fmpid[sATF]);
                PT_WAIT_UNTIL(IsBusFree());
            }
        }
        ClrAllSlavesRxStatus();
        PT_WAIT_UNTIL(CheckAllSlavesNext() >= 0);
        if (allSlavesNext == 0)
        { // finished
            PT_EXIT();
        }
        for (int i = 0; i < SlaveCnt(); i++)
        { // mark atfSt
            atfSt.at(i) = vSlaves.at(i)->CheckNext();
        }
    }
    PT_END();
}

void GroupIslus::IMakeFrameForSlave(uint8_t uciFrmId)
{
    if (db.GetUciProd().IsIslusSpFrm(uciFrmId))
    {
        auto &prod = db.GetUciProd();
        auto &user = db.GetUciUser();
        uint8_t *p = txBuf + 1;
        *p++ = SET_ISLUS_SP_FRM; // ISLUS special frame
        p++;                     // skip slave frame id
        p = Cnvt::PutU32(uciFrmId, p);
        txLen = p - txBuf;
    }
    else
    {
        auto frm = db.GetUciFrm().GetFrm(uciFrmId);
        if (frm == nullptr)
        {
            MyThrow("ERROR: MakeFrameForSlave(frmId=%d): Frm is null", uciFrmId);
        }
        MakeFrameForSlave(frm);
    }
}

int GroupIslus::ITransFrmWithOrBuf(uint8_t uciFrmId, uint8_t *dst)
{
    if (db.GetUciProd().IsIslusSpFrm(uciFrmId))
    {
        return 0;
    }
    else
    {
        auto frm = db.GetUciFrm().GetFrm(uciFrmId);
        if (frm == nullptr)
        {
            MyThrow("ERROR: TransFrmWithOrBuf(frmId=%d): Frm is null", uciFrmId);
        }
        return TransFrmWithOrBuf(frm, dst);
    }
}
