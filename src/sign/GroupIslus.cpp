#include <cstring>
#include <sign/GroupIslus.h>
#include <sign/Slave.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <module/ptcpp.h>
#include <module/Utils.h>


using namespace Utils;
using namespace std;

GroupIslus::GroupIslus(uint8_t id)
    : Group(id)
{
    auto &prod = db.GetUciProd();
    if (prod.SlavesPerSign() != 1)
    {
        throw invalid_argument("ISLUS: Sign can only have ONE slave");
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
    auto pnext = cmd + 3;
    uint8_t signCnt = SignCnt();
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
        if (db.GetUciProd().GetSignCfg(sign_id).rejectFrms.GetBit(frm_id))
        {
            return APP::ERROR::SyntaxError;
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
            // Here, use PT_EXIT() to terminate TaskSetATF
            // Must call TaskSetATFReset() before calling of TaskSetATF
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
        //auto &prod = db.GetUciProd();
        //auto &usercfg = db.GetUciUserCfg();
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
            throw runtime_error(StrFn::PrintfStr("ERROR: MakeFrameForSlave(frmId=%d): Frm is null", uciFrmId));
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
            throw runtime_error(StrFn::PrintfStr("ERROR: TransFrmWithOrBuf(frmId=%d): Frm is null", uciFrmId));
        }
        return TransFrmWithOrBuf(frm, dst);
    }
}
