#include <sign/Group.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>
#include <module/ptcpp.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>

using namespace Utils;

Group::Group(uint8_t groupId)
    : groupId(groupId), db(DbHelper::Instance())
{
    pwrUpTmr.Setms(0);
    busLockTmr.Setms(0);
    UciProd &prod = db.GetUciProd();
    int signCnt = prod.NumberOfSigns();
    switch (prod.ExtStsRplSignType())
    {
    case SESR::SIGN_TYPE::TEXT:
        for (int i = 1; i <= signCnt; i++)
        {
            if (prod.GetGroupIdOfSign(i) == groupId)
            {
                vSigns.push_back(new SignTxt(i));
            }
        }
        break;
    case SESR::SIGN_TYPE::GFX:
        for (int i = 1; i <= signCnt; i++)
        {
            if (prod.GetGroupIdOfSign(i) == groupId)
            {
                vSigns.push_back(new SignGfx(i));
            }
        }
        break;
    case SESR::SIGN_TYPE::ADVGFX:
        for (int i = 1; i <= signCnt; i++)
        {
            if (prod.GetGroupIdOfSign(i) == groupId)
            {
                vSigns.push_back(new SignAdg(i));
            }
        }
        break;
    }
    if (vSigns.size() == 0)
    {
        MyThrow("Error:There is no sign in Group[%d]", groupId);
    }
    dsBak = new DispStatus(vSigns.size());
    dsCurrent = new DispStatus(vSigns.size());
    dsNext = new DispStatus(vSigns.size());
    dsExt = new DispStatus(vSigns.size());
    // defult dsNext is BLANK
    dsNext->dispType = DISP_STATUS::TYPE::BLK;
    dsNext->fmpid[0] = 0;

    maxTxSize = 1 + 9 + prod.MaxFrmLen() + 2; // slaveId(1) + MIcode-datalen(9) + bitmapdata(x) + appcrc(2)
    txBuf = new uint8_t[maxTxSize];
    txLen = 0;

    switch (prod.ColourBits())
    {
    case 1:
        orLen = prod.Gfx1FrmLen();
        break;
    case 4:
        orLen = prod.Gfx4FrmLen();
        break;
    case 24:
        orLen = prod.Gfx24FrmLen();
        break;
    }
    orBuf = new uint8_t[orLen];

    // load process
    devSet = db.GetUciProcess().GetDevice(groupId);
    devCur = devSet;
    // TODO power
    // TODO enabled plan
    // TODO Dimming
}

Group::~Group()
{
    delete[] orBuf;
    delete txBuf;
    delete dsBak;
    delete dsCurrent;
    delete dsNext;
    delete dsExt;
    for (int i = 0; i < vSigns.size(); i++)
    {
        delete vSigns[i];
    }
    for (int i = 0; i < vSlaves.size(); i++)
    {
        delete vSlaves[i];
    }
}

void Group::PeriodicRun()
{
#if 0
    // components PeriodicRun
    fcltSw.PeriodicRun();
    extInput.PeriodicRun();

    if (fcltSw.IsChanged())
    {
        FcltSwitchFunc();
        fcltSw.ClearChangeFlag();
    }
    else
    {
        if (FacilitySwitch::FS_STATE::OFF != fcltSw.Get())
        {
            if (power == PWR_STATE::RISING)
            {
                if (pwrUpTmr.IsExpired())
                {
                    power = PWR_STATE::ON;
                    SignSetPower(1);
                    // TODO reset sign
                }
            }
            if (power == PWR_STATE::ON &&
                FacilitySwitch::FS_STATE::AUTO == fcltSw.Get())
            {
                ExtInputFunc();
            }
        }
    }
#endif

    if (!IsBusFree())
    {
        return;
    }

    if (IsDsNextEmergency())
    {
        readyToLoad = 1;
    }
    if (readyToLoad)
    {
        if (devSet == DEV_DIS && devCur == DEV_EN)
        {
            devCur = DEV_DIS;
            // TODO send blank
        }
        else if (devSet == DEV_EN && devCur == DEV_DIS)
        {
            devCur = DEV_DIS;
            // TODO reload
        }
        newCurrent = LoadDsNext(); // current->bak and next/fs/ext->current
        if (newCurrent)
        {
            newPlnId = 0;
            newMsgId = 0;
            newFrmId = 0;
        }
    }

    //TaskPln(&taskPlnLine);
    TaskMsg(&taskMsgLine);
    TaskFrm(&taskFrmLine);
    TaskRqstSlave(&taskRqstSlaveLine);

    PeriodicHook();
}

void Group::FcltSwitchFunc()
{
    FacilitySwitch::FS_STATE fs = fcltSw.Get();
    if (fs == FacilitySwitch::FS_STATE::OFF)
    {
        // PowerOutput(0);
        SetPower(0);
        // reset signs
    }
    else
    {
        if (power == PWR_STATE::OFF)
        {
            //PowerOutput(1);
            SetPower(1);
        }
        if (fs == FacilitySwitch::FS_STATE::AUTO)
        {
            DispNext(DISP_STATUS::TYPE::FRM, 0);
        }
        else
        {
            uint8_t msg = (fs == FacilitySwitch::FS_STATE::MSG1) ? 1 : 2;
            DispNext(DISP_STATUS::TYPE::FSW, msg);
        }
    }
}

void Group::ExtInputFunc()
{
    if (extInput.IsChanged())
    {
        uint8_t msg = 0;
        if ((dsExt->dispType == DISP_STATUS::TYPE::EXT && msg <= dsExt->fmpid[0]) ||
            (dsExt->dispType != DISP_STATUS::TYPE::EXT))
        {
            DispNext(DISP_STATUS::TYPE::EXT, msg);
        }
        extInput.ClearChangeFlag();
    }
}

// TODO
bool Group::TaskPln(int *_ptLine)
{
    if (!IsBusFree() || dsCurrent->dispType != DISP_STATUS::TYPE::PLN)
    {
        return PT_RUNNING;
    }
    if (newCurrent)
    {
        newCurrent = 0;
        newPln = 1;
    }
    if (newPln)
    {
        ClrOrBuf();
        TaskPlnReset();
        newPln = 0;
        newPlnId = dsCurrent->fmpid[0];
    }

    PT_BEGIN();
    while (true)
    {
        // TODO new plan
        // step 1: load msg/frm in plan
        // if msg, set load msg flag
        // if frm, set load frm flag
        // step 2: wait readyToLoad
        // step 3: check time
        // if(new localtime::minute)
        //      check msg/frm
        //      if(new msg/frm)
        //          continue; // got to step1:
        PrintDbg("TaskPln, delay 5 sec\n");
        taskPlnTmr.Setms(5000);
        PT_WAIT_UNTIL(taskPlnTmr.IsExpired());

        if (dsCurrent->dispType == DISP_STATUS::TYPE::PLN)
        {
        }
    };
    PT_END();
}

// TODO
bool Group::TaskMsg(int *_ptLine)
{
    uint8_t nextEntry;
    if (!IsBusFree() ||
        (dsCurrent->dispType != DISP_STATUS::TYPE::MSG &&
         dsCurrent->dispType != DISP_STATUS::TYPE::FSW &&
         dsCurrent->dispType != DISP_STATUS::TYPE::EXT &&
         !(dsCurrent->dispType == DISP_STATUS::TYPE::PLN && plnEntryType == typeMSG)))
    {
        return PT_RUNNING;
    }
    if (newCurrent)
    {
        newCurrent = 0;
        newMsg = 1;
        ClrOrBuf();
    }
    if (newMsg)
    {
        TaskMsgReset();
        newMsg = 0;
        newMsgId = (dsCurrent->dispType == DISP_STATUS::TYPE::PLN && plnEntryType == typeMSG) ? plnEntryId : dsCurrent->fmpid[0];
    }
    PT_BEGIN();
    while (true)
    {
        pMsg = db.GetUciMsg().GetMsg(newMsgId);
        if (pMsg == nullptr)
        { // msg undedined, BLANK=frm0
            ClrOrBuf();
            while (true) // null msg task, only for DISP_STATUS::TYPE::FSW/EXT
            {
                // step1: set frame
                do
                {
                    SlaveSetFrame(0xFF, 0, 0);
                    ClrAllSlavesRxStatus();
                    PT_WAIT_UNTIL(CheckAllSlavesNext() != STATE3::S_NA);
                } while (allSlavesNext == STATE3::S_0);
                // step2: display frame
                do
                {
                    SlaveSetStoredFrame(0xFF, 0);
                    GroupSetReportDisp(0, newMsgId, 0);
                    ClrAllSlavesRxStatus();
                    do
                    {
                        // step3: check current & next
                        taskFrmTmr.Setms(db.GetUciProd().SlaveRqstInterval() - 1);
                        PT_WAIT_UNTIL(AllSlavesGotStatus() && taskFrmTmr.IsExpired());
                        CheckAllSlavesCurrent();
                        CheckAllSlavesNext();
                    } while (allSlavesCurrent == STATE3::S_1 && allSlavesNext == STATE3::S_1); // all good
                } while (allSlavesNext == STATE3::S_1);                                        // Current is NOT matched but Next is matched, re-issue SlaveSetStoredFrame
                // Next is NOT matched, restart from SlaveSetFrame
            }; // end of null msg task
        }
        else
        {
            while (true) // normal msg task
            {
            NORMAL_MSG_TASK_START:
                ClrOrBuf();
                InitMsgOverlayBuf(pMsg);
                msgSetEntry = 0;
                msgEntryCnt = 0;
                // set & display, TotalRound
                // +++++++++ first round & first frame, set orbuf and frame 1
                while (pMsg->msgEntries[msgEntryCnt].onTime == 0)
                { // onTime==0, trans frame to set orBuf
                    TransFrmToOrBuf(pMsg->msgEntries[msgEntryCnt].frmId);
                    msgEntryCnt++;
                    msgSetEntry++;
                }
                do
                {
                    SlaveSetFrame(0xFF, 1, pMsg->msgEntries[msgEntryCnt].frmId);
                    ClrAllSlavesRxStatus();
                    PT_WAIT_UNTIL(CheckAllSlavesNext() != STATE3::S_NA);
                } while (allSlavesNext == STATE3::S_0);
                msgSetEntry++;
                // +++++++++ after first round & first frame
                // msg loop begin
                do
                {
                    readyToLoad = 0; // when a msg begin, clear readyToLoad
                    if (msgEntryCnt == pMsg->entries - 1 || pMsg->msgEntries[msgEntryCnt].onTime != 0)
                    { // overlay frame do not need to run DispFrm
                        GroupSetReportDisp(pMsg->msgEntries[msgEntryCnt].frmId, newMsgId, newPlnId);
                        do // +++++++++ DispFrm X
                        {
                            SlaveSetStoredFrame(0xFF, msgEntryCnt + 1);
                            (pMsg->msgEntries[msgEntryCnt].onTime == 0) ? taskMsgTmr.Clear() : taskMsgTmr.Setms(pMsg->msgEntries[msgEntryCnt].onTime * 100 - 1);
                            if (msgEntryCnt == pMsg->entries - 1)
                            {
                                long lastFrmOn = db.GetUciUser().LastFrmOn();
                                (lastFrmOn == 0) ? taskMsgLastFrmTmr.Clear() : taskMsgLastFrmTmr.Setms(lastFrmOn * 1000);
                            }
                            else
                            {
                                taskMsgLastFrmTmr.Clear();
                            }
                            ClrAllSlavesRxStatus();
                            PT_WAIT_UNTIL(AllSlavesGotStatus());
                            CheckAllSlavesCurrent();
                            CheckAllSlavesNext();
                        } while (allSlavesCurrent == STATE3::S_0 && allSlavesNext == STATE3::S_1);
                        // Current is NOT matched but Next is matched, re-issue SlaveSetStoredFrame
                        if (allSlavesNext != STATE3::S_1)
                        { // Next is NOT matched, this is a fatal error, restart
                            PrintDbg("Current&Next NOT matched, RESTART\n");
                            goto NORMAL_MSG_TASK_START;
                        }
                        // ++++++++++ DispFrm X done
                    }
                    // load next frame to set or make orbuf
                    if (msgSetEntry < msgSetEntryMax)
                    {
                        nextEntry = (msgEntryCnt == pMsg->entries - 1) ? 0 : (msgEntryCnt + 1);
                        if (pMsg->msgEntries[nextEntry].onTime == 0 && nextEntry != (pMsg->entries-1))
                        { // onTime==0 and not last entry, trans frame to set orBuf
                            if (msgSetEntry < pMsg->entries)
                            { // only set orBuf at first round
                                TransFrmToOrBuf(pMsg->msgEntries[nextEntry].frmId);
                            }
                        }
                        else
                        { // Set next frame
                            do
                            {
                                SlaveSetFrame(0xFF, nextEntry + 1, pMsg->msgEntries[nextEntry].frmId);
                                ClrAllSlavesRxStatus();
                                PT_WAIT_UNTIL(CheckAllSlavesNext() != STATE3::S_NA);
                            } while (allSlavesNext == STATE3::S_0);
                        }
                        msgSetEntry++;
                    }
                    do // +++++++++ OnTime
                    {
                        taskFrmTmr.Setms(db.GetUciProd().SlaveRqstInterval() - 1);
                        PT_WAIT_UNTIL(AllSlavesGotStatus() && taskFrmTmr.IsExpired());
                        CheckAllSlavesCurrent();
                        CheckAllSlavesNext();
                        if (msgEntryCnt == pMsg->entries - 1)
                        {
                            if (taskMsgLastFrmTmr.IsExpired())
                            {
                                taskMsgLastFrmTmr.Clear();
                                readyToLoad = 1; // last frame on expired
                            }
                        }
                    } while (allSlavesCurrent == STATE3::S_1 && allSlavesNext == STATE3::S_1 && !taskMsgTmr.IsExpired());
                    if (!taskMsgTmr.IsExpired())
                    { // Currnet/Next is NOT matched, this is a fatal error, restart
                        PrintDbg("Current&Next NOT matched, RESTART\n");
                        goto NORMAL_MSG_TASK_START;
                    }
                    // ++++++++++ OnTime finish
                    if (pMsg->msgEntries[msgEntryCnt].onTime != 0)
                    { // overlay frame do not need to run transition
                        // ++++++++++ transition time begin
                        if (pMsg->transTime != 0)
                        {
                            SlaveDisplayFrame(0xFF, 0);
                            taskMsgTmr.Setms(pMsg->transTime * 10 - 1);
                            PT_WAIT_UNTIL(taskMsgTmr.IsExpired());
                            // because transition time is generally short, don have enough time to check, so just wait for expired
                        }
                        // +++++++++++ transition time end
                    }
                    if (msgEntryCnt == (pMsg->entries - 1))
                    {
                        // last frm finished, set readyToLoad=1 and YIELD. If there is new setting, could be loaded
                        readyToLoad = 1;
                        PT_YIELD();
                        msgEntryCnt = 0;
                        if (msgSetEntry < msgSetEntryMax)
                        {
                            msgSetEntry++;
                        }
                    }
                    else
                    {
                        msgEntryCnt++;
                    }
                } while (true); // end of 'msg loop'
            };                  // end of 'normal msg task'
        }                       // end of if(pMsg == nullptr) else
    };
    PT_END();
}

void Group::InitMsgOverlayBuf(Message *pMsg)
{
    int colour = -1;
    msgSetEntryMax = pMsg->entries;
    bool onTime1 = false;
    for (int i = 0; i < pMsg->entries; i++)
    {
        auto &me = pMsg->msgEntries[i];
        if (me.onTime == 0)
        {
            auto frm = db.GetUciFrm().GetFrm(me.frmId);
            if (frm != nullptr)
            {
                if ((frm->colour > colour) && (colour != -1 || i < (pMsg->entries - 1)))
                {
                    colour = frm->colour; // get max colour
                }
                if(onTime1)
                {
                    msgSetEntryMax = pMsg->entries+i;
                }
            }
        }
        else
        { // this entry onTime>0
            onTime1 = true;
        }
    }
    if(colour==-1) // no overlay
    {
        orType = 0;
    }
    else if (colour < FRM::COLOUR::MonoFinished)
    {
        orType = 1;
    }
    else if (colour == FRM::COLOUR::MultipleColours)
    {
        orType = 4;
    }
    else if (colour == FRM::COLOUR::RGB24)
    {
        orType = 24;
    }
}

void Group::TransFrmToOrBuf(uint8_t frmId)
{

}

bool Group::TaskFrm(int *_ptLine)
{
    if (!IsBusFree() ||
        (dsCurrent->dispType != DISP_STATUS::TYPE::FRM &&
         dsCurrent->dispType != DISP_STATUS::TYPE::ATF &&
         !(dsCurrent->dispType == DISP_STATUS::TYPE::PLN && plnEntryType == typeFRM)))
    {
        return PT_RUNNING;
    }

    if (newCurrent)
    {
        newCurrent = 0;
        newFrm = 1;
        ClrOrBuf();
    }
    if (newFrm)
    {
        TaskFrmReset();
        newFrm = 0;
    }
    PT_BEGIN();
    while (true)
    {
        // step1: set frame
        if (dsCurrent->dispType == DISP_STATUS::TYPE::ATF)
        {
            PT_WAIT_UNTIL(TaskSetATF(&taskATFLine));
            newFrmId = 255; // set newFrmId as 'NOT 0'
            // TODO
            for (auto &s : vSigns)
            {
                s->SetReportDisp(newFrmId, newMsgId, newPlnId);
            }
        }
        else
        {
            newFrmId = (dsCurrent->dispType == DISP_STATUS::TYPE::FRM) ? dsCurrent->fmpid[0] : plnEntryId;
            do
            {
                SlaveSetFrame(0xFF, (newFrmId == 0) ? 0 : 1, newFrmId);
                ClrAllSlavesRxStatus();
                PT_WAIT_UNTIL(CheckAllSlavesNext() != STATE3::S_NA);
            } while (allSlavesNext == STATE3::S_0);
            GroupSetReportDisp(newFrmId, newMsgId, newPlnId);
        }
        // step2: display frame
        do
        {
            SlaveSetStoredFrame(0xFF, (newFrmId == 0) ? 0 : 1);
            ClrAllSlavesRxStatus();
            do
            {
                // step3: check current & next
                taskFrmTmr.Setms(db.GetUciProd().SlaveRqstInterval() - 1);
                PT_WAIT_UNTIL(AllSlavesGotStatus() && taskFrmTmr.IsExpired());
                CheckAllSlavesCurrent();
                CheckAllSlavesNext();
            } while (allSlavesCurrent == STATE3::S_1 && allSlavesNext == STATE3::S_1); // all good
        } while (allSlavesNext == STATE3::S_1);                                        // Current is NOT matched but Next is matched, re-issue SlaveSetStoredFrame
        // Next is NOT matched, restart from SlaveSetFrame
    };
    PT_END();
}

bool Group::TaskRqstSlave(int *_ptLine)
{
    if (!IsBusFree())
        return PT_RUNNING;
    PT_BEGIN();
    while (true)
    {
        rqstNoRplCnt = 0;
        do
        {
            RqstStatus(rqstStCnt);
            taskRqstSlaveTmr.Setms(db.GetUciProd().SlaveRqstInterval() - 1);
            PT_WAIT_UNTIL(taskRqstSlaveTmr.IsExpired());
            if (vSlaves[rqstStCnt]->rxStatus == 0)
            {
                if (++rqstNoRplCnt >= db.GetUciProd().OfflineDebounce())
                { // offline
                    rqstNoRplCnt = 0;
                    PrintDbg("vSlaves[%d] RqstStatus offline\n", rqstStCnt);
                    //PT_EXIT();
                }
            }
            else
            {
                rqstNoRplCnt = 0;
            }
        } while (rqstNoRplCnt > 0);
        if (++rqstStCnt == vSlaves.size())
        {
            rqstStCnt = 0;
        }

        rqstNoRplCnt = 0;
        do
        {
            RqstExtStatus(rqstExtStCnt);
            taskRqstSlaveTmr.Setms(db.GetUciProd().SlaveRqstInterval() - 1);
            PT_WAIT_UNTIL(taskRqstSlaveTmr.IsExpired());
            if (vSlaves[rqstExtStCnt]->rxExtSt == 0)
            {
                if (++rqstNoRplCnt >= db.GetUciProd().OfflineDebounce())
                { // offline
                    rqstNoRplCnt = 0;
                    PrintDbg("vSlaves[%d] RqstExtStatus offline\n", rqstExtStCnt);
                    //PT_EXIT();
                }
            }
            else
            {
                rqstNoRplCnt = 0;
            }
        } while (rqstNoRplCnt > 0);
        if (++rqstExtStCnt == vSlaves.size())
        {
            rqstExtStCnt = 0;
        }
    };
    PT_END();
}

Utils::STATE3 Group::CheckAllSlavesNext()
{
    for (auto &s : vSlaves)
    {
        allSlavesNext = s->IsNextMatched();
        if (allSlavesNext != Utils::STATE3::S_1)
        {
            break;
        }
    }
    return allSlavesNext;
}

Utils::STATE3 Group::CheckAllSlavesCurrent()
{
    for (auto &s : vSlaves)
    {
        allSlavesCurrent = s->IsCurrentMatched();
        if (allSlavesCurrent != Utils::STATE3::S_1)
        {
            break;
        }
    }
    return allSlavesCurrent;
}

bool Group::AllSlavesGotStatus()
{
    for (auto &s : vSlaves)
    {
        if (s->rxStatus == 0)
        {
            return false;
        }
    }
    return true;
}

void Group::ClrAllSlavesRxStatus()
{
    for (auto &s : vSlaves)
    {
        s->rxStatus = 0;
    }
    TaskRqstSlaveReset();
}

bool Group::AllSlavesGotExtSt()
{
    for (auto &s : vSlaves)
    {
        if (s->rxExtSt == 0)
        {
            return false;
        }
    }
    return true;
}

void Group::ClrAllSlavesRxExtSt()
{
    for (auto &s : vSlaves)
    {
        s->rxExtSt = 0;
    }
    TaskRqstSlaveReset();
}

bool Group::IsSignInGroup(uint8_t id)
{
    for (auto &s : vSigns)
    {
        if (s->SignId() == id)
        {
            return true;
        }
    }
    return false;
}

Sign *Group::GetSign(uint8_t id)
{
    for (auto &s : vSigns)
    {
        if (s->SignId() == id)
        {
            return s;
        }
    }
    return nullptr;
}

Slave *Group::GetSlave(uint8_t id)
{
    for (auto &s : vSlaves)
    {
        if (s->SlaveId() == id)
        {
            return s;
        }
    }
    return nullptr;
}

void Group::GroupSetReportDisp(uint8_t newFrmId, uint8_t newMsgId, uint8_t newPlnId)
{
    for (auto &s : vSigns)
    {
        s->SetReportDisp(newFrmId, newMsgId, newPlnId);
    }
}

// TODO
bool Group::IsEnPlanOverlap(uint8_t id)
{
    if (id == 0)
        return false;
    return false;
}

// TODO
bool Group::IsPlanActive(uint8_t id)
{
    if (id == 0)
    { // check all plans
    }

    return false;
}

// TODO
bool Group::IsPlanEnabled(uint8_t id)
{
    if (id == 0)
    { // check all plans
    }
    return db.GetUciProcess().IsPlanEnabled(groupId, id);
}

APP::ERROR Group::EnablePlan(uint8_t id)
{
    if (id == 0)
    {
        return DisablePlan(0);
    }
    if (IsPlanActive(id))
    {
        return APP::ERROR::FrmMsgPlnActive;
    }
    if (IsPlanEnabled(id))
    {
        return APP::ERROR::PlanEnabled;
    }
    if (IsEnPlanOverlap(id))
    {
        return APP::ERROR::OverlaysNotSupported;
    }
    db.GetUciProcess().EnablePlan(groupId, id);
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::DisablePlan(uint8_t id)
{
    if (IsPlanActive(id))
    {
        return APP::ERROR::FrmMsgPlnActive;
    }
    if (id != 0 && !IsPlanEnabled(id))
    {
        return APP::ERROR::PlanNotEnabled;
    }
    db.GetUciProcess().DisablePlan(groupId, id);
    return APP::ERROR::AppNoError;
}

// TODO
bool Group::IsMsgActive(uint8_t p)
{

    return false;
}

// TODO
bool Group::IsFrmActive(uint8_t p)
{

    return false;
}

void Group::DispNext(DISP_STATUS::TYPE type, uint8_t id)
{
    if (type == DISP_STATUS::TYPE::EXT)
    {
        dsExt->dispType = type;
        dsExt->fmpid[0] = id;
    }
    else
    {
        dsNext->dispType = type;
        dsNext->fmpid[0] = id;
    }
}

// TODO
void Group::DispBackup()
{
    for (auto &sign : vSigns)
    {
        //sign->DispBackup();
    }
}

APP::ERROR Group::DispFrm(uint8_t id)
{
    if (FacilitySwitch::FS_STATE::AUTO == fcltSw.Get())
    {
        uint8_t buf[3];
        buf[0] = 0x0E;
        buf[1] = groupId;
        buf[2] = id;
        db.GetUciProcess().SetDisp(groupId, buf, 3);
        DispNext(DISP_STATUS::TYPE::FRM, id);
        return APP::ERROR::AppNoError;
    }
    return APP::ERROR::FacilitySwitchOverride;
}

APP::ERROR Group::DispMsg(uint8_t id)
{
    if (FacilitySwitch::FS_STATE::AUTO == fcltSw.Get())
    {
        uint8_t buf[3];
        buf[0] = 0x0F;
        buf[1] = groupId;
        buf[2] = id;
        db.GetUciProcess().SetDisp(groupId, buf, 3);
        DispNext(DISP_STATUS::TYPE::MSG, id);
        return APP::ERROR::AppNoError;
    }
    return APP::ERROR::FacilitySwitchOverride;
}

void Group::DispExtSw(uint8_t id)
{
    DispNext(DISP_STATUS::TYPE::EXT, id);
}

APP::ERROR Group::SetDimming(uint8_t dimming)
{
    db.GetUciProcess().SetDimming(groupId, dimming);
    for (auto &sign : vSigns)
    {
        sign->DimmingSet(dimming);
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::SetPower(uint8_t v)
{
    if (v == 0)
    {
        // TODO set power off
        power = PWR_STATE::OFF;
        db.GetUciProcess().SetPower(groupId, 0);
        dsBak->Frm0();
        dsCurrent->Frm0();
        dsNext->Frm0();
        dsExt->N_A();
        for (auto &sign : vSigns)
        {
            sign->Reset();
        }
    }
    else
    {
        // TODO set power on
        if (power == PWR_STATE::OFF)
        {
            pwrUpTmr.Setms(db.GetUciProd().SlavePowerUpDelay() * 1000);
            power = PWR_STATE::RISING;
            db.GetUciProcess().SetPower(groupId, 1);
        }
    }
    return APP::ERROR::AppNoError;
}

// TODO
void Group::SignSetPower(uint8_t v)
{
    for (auto &sign : vSigns)
    {
        //sign->SetPower(power);
    }
}

// TODO
APP::ERROR Group::SetDevice(uint8_t endis)
{
    devSet = (endis == 0) ? DEV_DIS : DEV_EN;
    db.GetUciProcess().SetDevice(groupId, devSet);
    for (auto &sign : vSigns)
    {
        //sign->SetDevice(endis);
    }
    return APP::ERROR::AppNoError;
}

bool Group::IsDsNextEmergency()
{
    bool r = false;
    if (dsExt->dispType == DISP_STATUS::TYPE::EXT)
    { // external input
        uint8_t mid = dsExt->fmpid[0];
        if (mid >= 3 && mid <= 5)
        {
            if (DbHelper::Instance().GetUciUser().ExtSwCfgX(mid)->emergency)
            {
                r = true;
            }
        }
    }
    return r;
}

bool Group::LoadDsNext()
{
    bool r = false;
    if (dsNext->dispType == DISP_STATUS::TYPE::FSW)
    { // facility switch
        dsBak->Frm0();
        dsCurrent->Clone(dsNext);
        dsNext->N_A();
        r = true;
    }
    else if (dsExt->dispType == DISP_STATUS::TYPE::EXT)
    { // external input
        dsBak->Clone(dsCurrent);
        dsCurrent->Clone(dsExt);
        dsExt->N_A();
        r = true;
    }
    else if (dsNext->dispType != DISP_STATUS::TYPE::N_A)
    { // display command
        dsBak->Clone(dsCurrent);
        dsCurrent->Clone(dsNext);
        dsNext->N_A();
        r = true;
    }
    if (dsCurrent->dispType == DISP_STATUS::TYPE::N_A)
    { // if current == N/A, load frm[0] to activate plan
        dsBak->Frm0();
        dsCurrent->Frm0();
        r = true;
    }
    if (dsCurrent->fmpid[0] == 0 &&
        (dsCurrent->dispType == DISP_STATUS::TYPE::FRM || dsCurrent->dispType == DISP_STATUS::TYPE::MSG))
    { //frm[0] or msg[0], activate plan
        dsCurrent->dispType = DISP_STATUS::TYPE::PLN;
        r = true;
    }
    return r;
}

/*
    uint8_t signId, power;
    uint8_t dimmingSet, dimmingV;
    uint8_t deviceSet, deviceV;

    DispStatus dsBak;
    DispStatus dsCurrent;
    DispStatus dsNext;
    DispStatus dsExt;
        uint8_t reportPln, reportMsg, reportFrm;
*/

int Group::Rx(uint8_t *data, int len)
{
    switch (data[1])
    {
    case 0x06:
        SlaveStatusRpl(data, len);
        break;
    case 0x08:
        SlaveExtStatusRpl(data, len);
        break;
    default:
        return -1;
    }
    return 0;
}

void Group::ClrRx()
{
}

int Group::Tx()
{
    return lowerLayer->Tx(txBuf, txLen);
}

void Group::SlaveStatusRpl(uint8_t *data, int len)
{
    if (len != 14)
        return;
    for (int i = 0; i < vSlaves.size(); i++)
    {
        auto &s = vSlaves[i];
        if (s->SlaveId() == *data)
        {
            if (s->DecodeStRpl(data, len) == 0)
            {
                if (i == vSlaves.size() - 1)
                { // last slave of sign, make sign status
                    RefreshSignByStatus();
                }
            }
        }
    }
}

void Group::SlaveExtStatusRpl(uint8_t *data, int len)
{
    if (len < 22)
        return;
    for (int i = 0; i < vSlaves.size(); i++)
    {
        auto &s = vSlaves[i];
        if (s->SlaveId() == *data)
        {
            if (s->DecodeExtStRpl(data, len) == 0)
            {
                if (i == vSlaves.size() - 1)
                { // last slave of sign, make sign ext status
                    RefreshSignByExtSt();
                }
            }
        }
    }
}

void Group::RefreshSignByStatus()
{
    for (auto &s : vSigns)
    {
        s->RefreshSlaveStatus();
    }
}

void Group::RefreshSignByExtSt()
{
    for (auto &s : vSigns)
    {
        s->RefreshSlaveExtSt();
    }
}

int Group::RqstStatus(uint8_t slvindex)
{
    //PrintDbg("RqstStatus\n");
    LockBus(db.GetUciProd().SlaveRqstStTo());
    Slave *s = vSlaves[slvindex];
    s->rxStatus = 0;
    txBuf[0] = s->SlaveId();
    txBuf[1] = SLVCMD::RQST_STATUS;
    txLen = 2;
    return Tx();
}

int Group::RqstExtStatus(uint8_t slvindex)
{
    //PrintDbg("RqstExtSt\n");
    LockBus(db.GetUciProd().SlaveRqstExtTo());
    if (slvindex == 0xFF)
    {
        txBuf[0] = 0xFF;
    }
    else
    {
        Slave *s = vSlaves[slvindex];
        txBuf[0] = s->SlaveId();
        s->rxExtSt = 0;
    }

    uint8_t *p = txBuf + 1;
    *p++ = SLVCMD::RQST_EXT_ST;
    *p++ = 0;                    // TODO control byte
    p = Cnvt::PutU16(0x1010, p); // TODO dimming
    p = Cnvt::PutU16(0x1010, p); // TODO dimming
    p = Cnvt::PutU16(0x1010, p); // TODO dimming
    p = Cnvt::PutU16(0x1010, p); // TODO dimming
    txLen = 11;
    return Tx();
}

int Group::SlaveSetFrame(uint8_t slvindex, uint8_t slvFrmId, uint8_t uciFrmId)
{
    if (slvFrmId == 0 || uciFrmId == 0)
    {
        MyThrow("ERROR: SlaveSetFrame(slvindex=%d, slvFrmId=%d, uciFrmId=%d)",
                slvindex, slvFrmId, uciFrmId);
    }
    PrintDbg("SlaveSetFrame [%X]:%d<-%d\n", slvindex, slvFrmId, uciFrmId);
    auto frm = db.GetUciFrm().GetFrm(uciFrmId);
    if (frm == nullptr)
    {
        MyThrow("ERROR: TranslateFrame(frmId=%d): Frm is null", uciFrmId);
    }
    // TODO ToSlaveFormat with orBuf
    txLen = frm->ToSlaveFormat(txBuf + 1, orType, orBuf) - txBuf;
    txBuf[0] = (slvindex == 0xFF) ? 0xFF : vSlaves[slvindex]->SlaveId();
    txBuf[2] = slvFrmId;

    char *asc = new char[(txLen - 1) * 2];
    Cnvt::ParseToAsc(txBuf + 1, asc, txLen - 1);
    uint16_t crc = Crc::Crc16_8005((uint8_t *)asc, (txLen - 1) * 2);
    delete[] asc;
    Cnvt::PutU16(crc, txBuf + txLen);
    txLen += 2;
    if (slvindex == 0xFF)
    {
        for (auto &s : vSlaves)
        {
            s->expectNextFrmId = slvFrmId;
            s->frmCrc[slvFrmId] = crc;
            s->nextState = Slave::FRM_ST::MATCH_NA;
        }
    }
    else
    {
        auto &s = vSlaves[slvindex];
        s->expectNextFrmId = slvFrmId;
        s->frmCrc[slvFrmId] = crc;
        s->nextState = Slave::FRM_ST::MATCH_NA;
    }
    auto ms = Tx();
    auto dly = db.GetUciProd().SlaveSetStFrmDly();
    if (ms < dly)
    {
        ms = dly;
    }
    LockBus(ms);
    return ms;
}

int Group::SlaveDisplayFrame(uint8_t slvindex, uint8_t slvFrmId)
{
    PrintDbg("SlaveDisplayFrame [%X]:%d\n", slvindex, slvFrmId);
    LockBus(db.GetUciProd().SlaveDispDly());
    if (slvindex == 0xFF)
    {
        txBuf[0] = 0xFF;
        for (auto &s : vSlaves)
        {
            s->expectCurrentFrmId = slvFrmId;
            s->currentState = Slave::FRM_ST::MATCH_NA;
            s->expectNextFrmId = slvFrmId;
            s->nextState = Slave::FRM_ST::MATCH_NA;
        }
    }
    else
    {
        auto &s = vSlaves[slvindex];
        txBuf[0] = s->SlaveId();
        s->expectCurrentFrmId = slvFrmId;
        s->currentState = Slave::FRM_ST::MATCH_NA;
        s->expectNextFrmId = slvFrmId;
        s->nextState = Slave::FRM_ST::MATCH_NA;
    }
    txBuf[1] = SLVCMD::DISPLAY_FRM;
    txBuf[2] = slvFrmId;
    txLen = 3;
    return Tx();
}

int Group::SlaveSetStoredFrame(uint8_t slvindex, uint8_t slvFrmId)
{
    PrintDbg("SlaveSetStoredFrame [%X]:%d\n", slvindex, slvFrmId);
    LockBus(db.GetUciProd().SlaveDispDly());
    if (slvindex == 0xFF)
    {
        txBuf[0] = 0xFF;
        for (auto &s : vSlaves)
        {
            s->expectCurrentFrmId = slvFrmId;
            s->currentState = Slave::FRM_ST::MATCH_NA;
            s->expectNextFrmId = slvFrmId;
            s->nextState = Slave::FRM_ST::MATCH_NA;
        }
    }
    else
    {
        auto &s = vSlaves[slvindex];
        txBuf[0] = s->SlaveId();
        s->expectCurrentFrmId = slvFrmId;
        s->currentState = Slave::FRM_ST::MATCH_NA;
        s->expectNextFrmId = slvFrmId;
        s->nextState = Slave::FRM_ST::MATCH_NA;
    }
    txBuf[1] = SLVCMD::SET_STD_FRM;
    txBuf[2] = slvFrmId;
    txLen = 3;
    return Tx();
}

bool Group::IsBusFree()
{
    return busLockTmr.IsExpired();
}

void Group::LockBus(int ms)
{
    if (--ms < 1)
    {
        ms = 1;
    }
    busLockTmr.Setms(ms);
}
