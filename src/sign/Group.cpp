#include <sign/Group.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>
#include <module/ptcpp.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>
#include <gpio/GpioIn.h>
#include <gpio/GpioOut.h>
#include <sign/Controller.h>

using namespace Utils;

Group::Group(uint8_t groupId)
    : groupId(groupId), db(DbHelper::Instance()),
      fcltSw(PIN_AUTO, PIN_MSG1, PIN_MSG2)
{
    pPinCmdPower = new GpioOut(PIN_MOSFET1_CTRL, 0);
    extInput.push_back(new GpioIn(2, 2, PIN_CN7_7_MSG3));
    extInput.push_back(new GpioIn(2, 2, PIN_CN7_8_MSG4));
    extInput.push_back(new GpioIn(2, 2, PIN_CN7_9_MSG5));

    busLockTmr.Setms(0);
    dimmingAdjTimer.Setms(0);
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
    auto &proc = db.GetUciProcess();
    devSet = proc.GetDevice(groupId);
    devCur = !devSet;

    for (int i = 1; i <= 255; i++)
    {
        if (proc.IsPlanEnabled(groupId, i))
        {
            LoadPlanToPlnMin(i);
        }
    }
    //PrintPlnMin();
    for (auto &s : vSigns)
    {
        s->SignErr(proc.SignErr(s->SignId())->Get());
    }
    currentDimmingLvl = proc.GetDimming(groupId);
    targetDimmingLvl = currentDimmingLvl | 0x80;
    cmdPwr = proc.GetPower(groupId);
}

Group::~Group()
{
    for (auto& s : extInput)
    {
        delete s;
    }
    delete pPinCmdPower;
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

    // components PeriodicRun
    FcltSwitchFunc();
#if 0
    ExtInputFunc();
#endif

    if (power != PWR_STATE::ON || !IsBusFree())
    {
        return;
    }

    if (IsDsNextEmergency())
    {
        readyToLoad = 1;
    }
    if (readyToLoad)
    {
        if (devSet != devCur)
        {
            TaskMsgReset();
            TaskFrmReset();
            devCur = devSet;
            for (auto &s : vSigns)
            {
                s->Device(devCur);
            }
        }
        newCurrent = 0;
        if(dsCurrent->dispType == DISP_STATUS::TYPE::EXT && extDispTmr.IsExpired())
        {
            extDispTmr.Clear();
            DispBackup();
            newCurrent = 1;
        }
        newCurrent |= LoadDsNext(); // current->bak and next/fs/ext->current
    }

    TaskPln(&taskPlnLine);
    TaskMsg(&taskMsgLine);
    TaskFrm(&taskFrmLine);
    TaskRqstSlave(&taskRqstSlaveLine);

    PeriodicHook();
}

void Group::FcltSwitchFunc()
{
    fcltSw.PeriodicRun();
    FacilitySwitch::FS_STATE fs = fcltSw.Get();
    if (fcltSw.IsChanged())
    {
        fcltSw.ClearChanged();
        Controller::Instance().ctrllerError.Push(
            DEV::ERROR::FacilitySwitchOverride, fs != FacilitySwitch::FS_STATE::AUTO);
        char buf[64];
        int len = sprintf(buf, "Group%d:", groupId);
        fcltSw.ToStr(buf + len);
        db.GetUciEvent().Push(0, buf);
        if (cmdPwr)
        {
            if (fs == FacilitySwitch::FS_STATE::OFF)
            {
                if (power != PWR_STATE::OFF)
                {
                    GoPowerOff();
                }
            }
            else
            {
                if (power != PWR_STATE::ON)
                {
                    GoPowerOn();
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
    }
    else
    {
        if (FacilitySwitch::FS_STATE::OFF != fs)
        {
            if (power == PWR_STATE::RISING)
            {
                if (pwrUpTmr.IsExpired())
                {
                    power = PWR_STATE::ON;
                    // TODO reset sign
                    // reload
                }
            }
        }
    }
}

void Group::ExtInputFunc()
{
    if (power == PWR_STATE::ON && FacilitySwitch::FS_STATE::AUTO == fcltSw.Get())
    {
        if (++extInputCnt >= 10)
        {
            extInputCnt = 0;
            for (int i = 0; i < extInput.size(); i++)
            {
                auto gin = extInput[i];
                gin->PeriodicRun();
                if (gin->IsChanged() && gin->Value() == Utils::STATE3::S_1)
                {
                    gin->ClearChanged();
                    uint8_t msg = i + 3;
                    if (dsExt->dispType != DISP_STATUS::TYPE::EXT)
                    {
                        DispNext(DISP_STATUS::TYPE::EXT, msg);
                    }
                    else
                    {
                        if (msg < dsExt->fmpid[0])
                        {
                            DispNext(DISP_STATUS::TYPE::EXT, msg);
                        }
                        else if (msg == dsExt->fmpid[0])
                        {
                            auto time = db.GetUciUser().ExtSwCfgX(msg)->dispTime;
                            if (time != 0)
                            {
                                extDispTmr.Setms(time * 1000);
                            }
                        }
                    }
                    while(++i < extInput.size())
                    {// clear others changed flag
                        auto gin = extInput[i];
                        gin->PeriodicRun();
                        gin->ClearChanged();
                    }
                    return;
                }
            }
        }
    }
    else
    {
        extInputCnt = 0;
    }
}

bool Group::TaskPln(int *_ptLine)
{
    if (!IsBusFree() || dsCurrent->dispType != DISP_STATUS::TYPE::PLN || !readyToLoad)
    {
        return PT_RUNNING;
    }
    if (newCurrent)
    {
        newCurrent = 0;
        ClrOrBuf();
        TaskPlnReset();
    }
    PT_BEGIN();
    while (true)
    {
        {
            auto &plnmin = GetCurrentMinPln();
            if (plnmin.plnId == 0)
            {
                if (onDispPlnId != 0 || onDispMsgId != 0 || onDispFrmId != 0)
                { // previouse is not BLANK
                    PrintDbg("Plan:BLANK\n");
                    TaskFrmReset();
                    TaskMsgReset();
                    onDispPlnId = 0;
                    onDispNewMsg = 0;
                    onDispMsgId = 0;
                    onDispNewFrm = 1;
                    onDispPlnEntryType = PLN_ENTRY_FRM;
                    onDispPlnEntryId = 0;
                    activeMsg.ClrAll();
                    activeFrm.ClrAll();
                }
            }
            else
            {
                if (onDispPlnId != plnmin.plnId)
                {
                    PrintDbg("Load Plan:%d\n", plnmin.plnId);
                    activeMsg.ClrAll();
                    activeFrm.ClrAll();
                    auto pln = db.GetUciPln().GetPln(plnmin.plnId);
                    if (pln == nullptr)
                    {
                        readyToLoad = 0;
                        // TODO log: start plan but undefined
                        return false;
                    }
                    for (int i = 0; i < pln->entries; i++)
                    {
                        if (pln->plnEntries[i].fmType == PLN_ENTRY_FRM)
                        {
                            activeFrm.Set(pln->plnEntries[i].fmId);
                        }
                        else // if (pln->plnEntries[i].fmType == PLN_ENTRY_MSG)
                        {
                            SetActiveMsg(pln->plnEntries[i].fmId);
                        }
                    }
                }
                if (plnmin.fmType == PLN_ENTRY_FRM) // frame
                {
                    if (onDispPlnId != plnmin.plnId || onDispMsgId != 0 || onDispFrmId != plnmin.fmId)
                    { // previouse is not same, set pln:frm type
                        TaskFrmReset();
                        TaskMsgReset();
                        onDispPlnId = plnmin.plnId;
                        onDispNewMsg = 0;
                        onDispMsgId = 0;
                        onDispNewFrm = 1;
                        onDispPlnEntryType = PLN_ENTRY_FRM;
                        onDispPlnEntryId = plnmin.fmId;
                    }
                }
                else // if(plnmin.fmType==PLN_ENTRY_MSG) // msg
                {
                    if (onDispPlnId != plnmin.plnId || onDispMsgId != plnmin.fmId)
                    { // previouse is not same, set pln:msg type
                        TaskFrmReset();
                        TaskMsgReset();
                        onDispPlnId = plnmin.plnId;
                        onDispNewMsg = 1;
                        onDispPlnEntryType = PLN_ENTRY_MSG;
                        onDispPlnEntryId = plnmin.fmId;
                    }
                }
            }
        }
        // task sleep 1 second
        taskPlnTmr.Setms(1000);
        PT_WAIT_UNTIL(taskPlnTmr.IsExpired());
    };
    PT_END();
}

PlnMinute &Group::GetCurrentMinPln()
{
    time_t t = time(nullptr);
    struct tm stm;
    localtime_r(&t, &stm);
    return plnMin.at((stm.tm_wday * 24 + stm.tm_hour) * 60 + stm.tm_min);
}

bool Group::TaskMsg(int *_ptLine)
{
    uint8_t nextEntry; // temp using
    if (!IsBusFree() ||
        (dsCurrent->dispType != DISP_STATUS::TYPE::MSG &&
         dsCurrent->dispType != DISP_STATUS::TYPE::FSW &&
         dsCurrent->dispType != DISP_STATUS::TYPE::EXT &&
         !(dsCurrent->dispType == DISP_STATUS::TYPE::PLN && onDispPlnEntryType == PLN_ENTRY_MSG)))
    {
        return PT_RUNNING;
    }
    if (newCurrent)
    {
        newCurrent = 0;
        onDispNewMsg = 1;
        ClrOrBuf();
    }
    if (onDispNewMsg)
    {
        TaskMsgReset();
        onDispNewMsg = 0;
        if (dsCurrent->dispType == DISP_STATUS::TYPE::PLN && onDispPlnEntryType == PLN_ENTRY_MSG)
        {
            onDispMsgId = onDispPlnEntryId;
            // No need to set active msg, 'cause set in TaskPln
        }
        else
        {
            onDispPlnId = 0;
            onDispMsgId = dsCurrent->fmpid[0];
            if (dsCurrent->dispType == DISP_STATUS::TYPE::MSG)
            {
                activeMsg.ClrAll(); // this is CmdDisplayMsg, Clear all previouse active msg
                // But for FSW/EXT, do not clear previouse
            }
            SetActiveMsg(onDispMsgId);
        }
        PrintDbg("Display Msg:%d\n", onDispMsgId);
    }
    PT_BEGIN();
    while (true)
    {
        pMsg = db.GetUciMsg().GetMsg(onDispMsgId);
        if (pMsg == nullptr)
        { // null msg task, only for DISP_STATUS::TYPE::FSW/EXT
            if (dsCurrent->dispType != DISP_STATUS::TYPE::FSW &&
                dsCurrent->dispType != DISP_STATUS::TYPE::EXT)
            {
                // Something wrong, Load pln0 to run plan
                newCurrent = 1;
                dsCurrent->Pln0();
                // log
                TaskMsgReset();
                return false;
            }
            //for FSW/EXT, if msg undefined, show BLANK and report msgX+frm0
            ClrOrBuf();
            while (true)
            {
                do
                {
                    SlaveSetStoredFrame(0xFF, 0);
                    onDispFrmId = 0;
                    GroupSetReportDisp(onDispFrmId, onDispMsgId, onDispPlnId);
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
                while (pMsg->msgEntries[msgEntryCnt].onTime == 0 && msgEntryCnt < (pMsg->entries - 1))
                { // onTime==0 && not last entry, trans frame to set orBuf
                    TransFrmToOrBuf(pMsg->msgEntries[msgEntryCnt].frmId);
                    msgEntryCnt++;
                    msgSetEntry++;
                };
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
                    if (devCur)
                    {
                        readyToLoad = 0; // when a msg begin, clear readyToLoad
                    }

                    if (msgEntryCnt == pMsg->entries - 1 || pMsg->msgEntries[msgEntryCnt].onTime != 0)
                    { // overlay frame do not need to run DispFrm
                        onDispFrmId = pMsg->msgEntries[msgEntryCnt].frmId;
                        GroupSetReportDisp(onDispFrmId, onDispMsgId, onDispPlnId);
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
                        if (pMsg->msgEntries[nextEntry].onTime == 0 && nextEntry != (pMsg->entries - 1))
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
    msgOverlay = 0;                 // no overlay
    msgSetEntryMax = pMsg->entries; // no overlay
    bool onTime1 = false;
    for (int i = 0; i < pMsg->entries; i++)
    {
        auto &me = pMsg->msgEntries[i];
        if (me.onTime == 0)
        {
            if (i != (pMsg->entries - 1))
            { // NOT last entry
                msgOverlay = db.GetUciProd().ColourBits();
                if (onTime1)
                { // previouse entry ontime>0
                    msgSetEntryMax = pMsg->entries + i;
                }
            }
            else
            { // last entry is 0, so just set max
                msgSetEntryMax = pMsg->entries;
            }
        }
        else
        { // this entry onTime>0
            onTime1 = true;
        }
    }
}

bool Group::TaskFrm(int *_ptLine)
{
    if (!IsBusFree() ||
        (dsCurrent->dispType != DISP_STATUS::TYPE::FRM &&
         dsCurrent->dispType != DISP_STATUS::TYPE::ATF &&
         !(dsCurrent->dispType == DISP_STATUS::TYPE::PLN && onDispPlnEntryType == PLN_ENTRY_FRM)))
    {
        return PT_RUNNING;
    }

    if (newCurrent)
    {
        newCurrent = 0;
        onDispNewFrm = 1;
        ClrOrBuf();
    }
    if (onDispNewFrm)
    {
        TaskFrmReset();
        onDispNewFrm = 0;
        onDispMsgId = 0;
    }
    PT_BEGIN();
    while (true)
    {
        // step1: set frame
        if (dsCurrent->dispType == DISP_STATUS::TYPE::ATF)
        {
            onDispPlnId = 0;
            onDispFrmId = 1; // set onDispFrmId as 'NOT 0'
            PT_WAIT_UNTIL(TaskSetATF(&taskATFLine));
            PrintDbg("Display ATF\n");
        }
        else
        {
            if (dsCurrent->dispType == DISP_STATUS::TYPE::FRM)
            { // from CmdDispFrm
                onDispPlnId = 0;
                onDispFrmId = dsCurrent->fmpid[0];
                activeFrm.ClrAll();
                activeFrm.Set(onDispFrmId);
            }
            else
            { // from plan
                onDispFrmId = onDispPlnEntryId;
                // frm set active in TaskPln
            }
            PrintDbg("Display Frm:%d\n", onDispFrmId);
            if (onDispFrmId > 0)
            {
                do
                {
                    SlaveSetFrame(0xFF, (onDispFrmId == 0) ? 0 : 1, onDispFrmId);
                    ClrAllSlavesRxStatus();
                    PT_WAIT_UNTIL(CheckAllSlavesNext() != STATE3::S_NA);
                } while (allSlavesNext == STATE3::S_0);
            }
            GroupSetReportDisp(onDispFrmId, onDispMsgId, onDispPlnId);
        }
        // step2: display frame
        do
        {
            SlaveSetStoredFrame(0xFF, (onDispFrmId == 0) ? 0 : 1);
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

void Group::SetActiveMsg(uint8_t mid)
{
    auto msg = db.GetUciMsg().GetMsg(mid);
    if (msg == nullptr)
        return;
    for (int i = 0; i < msg->entries; i++)
    {
        activeFrm.Set(msg->msgEntries[i].frmId);
    }
    activeMsg.Set(mid);
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

        // dimming adjusting
        if (DimmingAdjust())
        {
            taskRqstSlaveTmr.Setms(db.GetUciProd().SlaveRqstInterval() - 1);
            PT_WAIT_UNTIL(taskRqstSlaveTmr.IsExpired());
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

void Group::GroupSetReportDisp(uint8_t onDispFrmId, uint8_t onDispMsgId, uint8_t onDispPlnId)
{
    for (auto &s : vSigns)
    {
        s->SetReportDisp(onDispFrmId, onDispMsgId, onDispPlnId);
    }
}

bool Group::IsEnPlanOverlap(uint8_t id)
{
    Plan *pln = db.GetUciPln().GetPln(id);
    if (pln == nullptr)
    {
        return false;
    }
    uint8_t week = 0x01;
    for (int i = 0; i < 7; i++)
    {
        if (pln->weekdays & week)
        {
            for (int j = 0; j < pln->entries; j++)
            {
                PlnEntry &entry = pln->plnEntries[j];
                int start = GetMinOffset(i, &(entry.start));
                int stop = GetMinOffset(i, &(entry.stop));
                if (stop <= start)
                {
                    stop += 24 * 60;
                    if (stop >= 7 * 24 * 60)
                    {
                        stop -= 7 * 24 * 60;
                    }
                }
                do
                {
                    if (plnMin.at(start).plnId != 0)
                    {
                        return true;
                    }
                    start++;
                    if (start == 7 * 24 * 60)
                    {
                        start = 0;
                    }
                } while (start != stop);
            }
        }
        week <<= 1;
    }
    return false;
}

int Group::GetMinOffset(int day, Hm *t)
{
    return (day * 24 + t->hour) * 60 + t->min;
}

bool Group::IsPlanActive(uint8_t id)
{
    return (id == 0) ? (onDispPlnId != 0) : (onDispPlnId == id);
}

bool Group::IsPlanEnabled(uint8_t id)
{
    auto &proc = db.GetUciProcess();
    if (id == 0)
    { // check all plans
        for (int i = 1; i <= 255; i++)
        {
            if (proc.IsPlanEnabled(groupId, id))
            {
                return true;
            }
        }
        return false;
    }
    return proc.IsPlanEnabled(groupId, id);
}

void Group::LoadPlanToPlnMin(uint8_t id)
{
    Plan *pln = db.GetUciPln().GetPln(id);
    if (pln == nullptr)
    {
        // Log
        return;
    }
    uint8_t week = 0x01;
    for (int i = 0; i < 7; i++)
    {
        if (pln->weekdays & week)
        {
            for (int j = 0; j < pln->entries; j++)
            {
                PlnEntry &entry = pln->plnEntries[j];
                int start = GetMinOffset(i, &(entry.start));
                int stop = GetMinOffset(i, &(entry.stop));
                if (stop <= start)
                {
                    stop += 24 * 60;
                    if (stop >= 7 * 24 * 60)
                    {
                        stop -= 7 * 24 * 60;
                    }
                }
                do
                {
                    auto &k = plnMin.at(start);
                    k.plnId = id;
                    k.fmType = entry.fmType;
                    k.fmId = entry.fmId;
                    if (++start == 7 * 24 * 60)
                    {
                        start = 0;
                    }
                } while (start != stop);
            }
        }
        week <<= 1;
    }
}

APP::ERROR Group::EnDisPlan(uint8_t id, bool endis)
{
    if (IsPlanActive(id))
    {
        return APP::ERROR::FrmMsgPlnActive;
    }
    if (id == 0)
    {
        plnMin.assign(plnMin.size(), PlnMinute{});
    }
    else
    {
        if (!db.GetUciPln().IsPlnDefined(id))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
        APP::ERROR r = endis ? EnPlan(id) : DisPlan(id);
        if (r != APP::ERROR::AppNoError)
        {
            return r;
        }
    }
    db.GetUciProcess().EnDisPlan(groupId, id, endis);
    //PrintPlnMin();
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::EnPlan(uint8_t id)
{
    if (IsPlanEnabled(id))
    {
        return APP::ERROR::PlanEnabled;
    }
    if (IsEnPlanOverlap(id))
    {
        return APP::ERROR::OverlaysNotSupported;
    }
    LoadPlanToPlnMin(id);
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::DisPlan(uint8_t id)
{
    if (!IsPlanEnabled(id))
    {
        return APP::ERROR::PlanNotEnabled;
    }
    db.GetUciProcess().EnDisPlan(groupId, id, false);
    Plan *pln = db.GetUciPln().GetPln(id);
    uint8_t week = 0x01;
    for (int i = 0; i < 7; i++)
    {
        if (pln->weekdays & week)
        {
            for (int j = 0; j < pln->entries; j++)
            {
                PlnEntry &entry = pln->plnEntries[j];
                int start = GetMinOffset(i, &(entry.start));
                int stop = GetMinOffset(i, &(entry.stop));
                if (stop <= start)
                {
                    stop += 24 * 60;
                    if (stop >= 7 * 24 * 60)
                    {
                        stop -= 7 * 24 * 60;
                    }
                }
                do
                {
                    auto &k = plnMin.at(start);
                    k.plnId = 0;
                    if (++start == 7 * 24 * 60)
                    {
                        start = 0;
                    }
                } while (start != stop);
            }
        }
        week <<= 1;
    }
    return APP::ERROR::AppNoError;
}

bool Group::IsMsgActive(uint8_t p)
{
    return activeMsg.Get(p);
}

bool Group::IsFrmActive(uint8_t p)
{
    return activeFrm.Get(p);
}

void Group::DispNext(DISP_STATUS::TYPE type, uint8_t id)
{
    if (type == DISP_STATUS::TYPE::EXT)
    {
        auto cfg = db.GetUciUser().ExtSwCfgX(id);
        auto time = cfg->dispTime;
        if (time != 0)
        {
            if (dsCurrent->dispType == DISP_STATUS::TYPE::N_A ||
                dsCurrent->dispType == DISP_STATUS::TYPE::BLK ||
                (onDispMsgId == 0 && onDispFrmId == 0) ||
                cfg->flashingOv == 0 ||
                (cfg->flashingOv != 0 &&
                 ((onDispMsgId != 0 && !db.GetUciMsg().IsMsgFlashing(onDispMsgId)) ||
                  onDispMsgId == 0 && onDispFrmId != 0 && !db.GetUciFrm().IsFrmFlashing(onDispFrmId))))
            {
                extDispTmr.Setms(time * 1000);
                dsExt->dispType = type;
                dsExt->fmpid[0] = id;
            }
        }
    }
    else
    {
        dsNext->dispType = type;
        dsNext->fmpid[0] = id;
    }
}

void Group::DispBackup()
{
    dsCurrent->Clone(dsBak);
    dsBak->N_A();
}

APP::ERROR Group::DispFrm(uint8_t id)
{
    if (FacilitySwitch::FS_STATE::AUTO == fcltSw.Get())
    {
        uint8_t buf[3];
        buf[0] = static_cast<uint8_t>(MI::CODE::SignDisplayFrame);
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
        buf[0] = static_cast<uint8_t>(MI::CODE::SignDisplayMessage);
        buf[1] = groupId;
        buf[2] = id;
        db.GetUciProcess().SetDisp(groupId, buf, 3);
        DispNext(DISP_STATUS::TYPE::MSG, id);
        return APP::ERROR::AppNoError;
    }
    return APP::ERROR::FacilitySwitchOverride;
}

APP::ERROR Group::SetDimming(uint8_t dimming)
{
    db.GetUciProcess().SetDimming(groupId, dimming);
    targetDimmingLvl = 0x80 | dimming;
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::SetPower(uint8_t v)
{
    if (cmdPwr != v)
    {
        if (FacilitySwitch::FS_STATE::OFF != fcltSw.Get())
        {
            if (v == 1)
            {
                GoPowerOn();
                // TODO load ds
            }
            else
            {
                GoPowerOff();
                // TODO backup ds
            }
        }
        cmdPwr = v;
        db.GetUciProcess().SetPower(groupId, v);
        for (auto &sign : vSigns)
        {
            sign->SignErr(DEV::ERROR::PoweredOffByCommand, v == 0);
        }
    }
    return APP::ERROR::AppNoError;
}

void Group::GoPowerOff()
{
    PinCmdPowerOff();
    power = PWR_STATE::OFF;
    //    dsBak->Clone(dsCurrent);
    //   dsCurrent->Frm0();
    //    dsNext->Frm0();
    //    dsExt->N_A();
    for (auto &sign : vSigns)
    {
        sign->Reset();
    }
}

void Group::GoPowerOn()
{
    PinCmdPowerOn();
    pwrUpTmr.Setms(db.GetUciProd().SlavePowerUpDelay() * 1000);
    power = PWR_STATE::RISING;
}

APP::ERROR Group::SetDevice(uint8_t endis)
{
    devSet = (endis == 0) ? 0 : 1;
    db.GetUciProcess().SetDevice(groupId, devSet);
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
            if (db.GetUciUser().ExtSwCfgX(mid)->emergency)
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
    UciProd &prod = db.GetUciProd();
    LockBus(prod.SlaveRqstExtTo());
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
    *p++ = 0; // TODO control byte
    uint8_t *pc = prod.ColourRatio();
    bool dmode = prod.DriverMode();
    for (int i = 0; i < 4; i++)
    {
        uint16_t dim = dmode ? (pc[i] * 0x100 + setDimming) : (setDimming * 0x100 + pc[i]);
        p = Cnvt::PutU16(dim, p);
    }
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
    int ms = 10;
    if (devCur)
    {
        //PrintDbg("Slv-SetFrame [%X]:%d<-%d\n", slvindex, slvFrmId, uciFrmId);
        MakeFrameForSlave(uciFrmId);
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
        ms = Tx();
        auto dly = db.GetUciProd().SlaveSetStFrmDly();
        if (ms < dly)
        {
            ms = dly;
        }
    }
    /*
    else
    {
        if (slvindex == 0xFF)
        {
            for (auto &s : vSlaves)
            {
                s->expectNextFrmId = 0;
                s->frmCrc[0] = 0;
                s->nextState = Slave::FRM_ST::MATCH_NA;
            }
        }
        else
        {
            auto &s = vSlaves[slvindex];
            s->expectNextFrmId = 0;
            s->frmCrc[0] = 0;
            s->nextState = Slave::FRM_ST::MATCH_NA;
        }
    }*/
    LockBus(ms);
    return ms;
}

// this function is actually for transistion time
int Group::SlaveDisplayFrame(uint8_t slvindex, uint8_t slvFrmId)
{
    if (devCur == 0)
    {
        slvFrmId = 0;
    }
    //PrintDbg("Slv-DisplayFrm [%X]:%d\n", slvindex, slvFrmId);
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
    Tx();
    int ms = db.GetUciProd().SlaveDispDly();
    LockBus(ms);
    return ms;
}

int Group::SlaveSetStoredFrame(uint8_t slvindex, uint8_t slvFrmId)
{
    if (devCur == 0)
    {
        slvFrmId = 0;
    }
    //PrintDbg("Slv-SetStoredFrm [%X]:%d\n", slvindex, slvFrmId);
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
    Tx();
    int ms = db.GetUciProd().SlaveDispDly();
    LockBus(ms);
    return ms;
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

void Group::PrintPlnMin()
{
    std::vector<PlnMinute>::iterator it = plnMin.begin();
    for (int d = 0; d < 7; d++)
    {
        printf("\nD%d:", d);
        for (int x = 0; x < 60; x++)
        {
            printf("%02d|", x);
        }
        printf("\n");
        for (int h = 0; h < 24; h++)
        {
            printf("%02d:", h);
            for (int k = 0; k < 60; k++)
            {
                auto plnId = it->plnId;
                if (plnId == 0)
                {
                    printf("()|");
                }
                else
                {
                    printf("%02X|", plnId);
                }
                std::advance(it, 1);
            }
            printf("\n");
        }
    }
}

bool Group::DimmingAdjust()
{
    bool r = false;
    if (targetDimmingLvl & 0x80)
    {
        targetDimmingLvl &= 0x7F;
        if (targetDimmingLvl == 0)
        {
            for (auto &sign : vSigns)
            {
                sign->DimmingSet(targetDimmingLvl);
                sign->DimmingV(currentDimmingLvl);
            }
        }
        else
        {
            for (auto &sign : vSigns)
            {
                sign->DimmingSet(targetDimmingLvl);
                sign->DimmingV(targetDimmingLvl);
            }
        }
        adjDimmingSteps = 0;
        dimmingAdjTimer.Setms(0);
    }
    if (dimmingAdjTimer.IsExpired())
    {
        // TODO auto dimming
        int tgt = (targetDimmingLvl == 0) ? 15 : targetDimmingLvl - 1; // 0-15
        int cur = (currentDimmingLvl - 1);                             // 0-15
        UciProd &prod = db.GetUciProd();
        if (tgt != cur)
        {
            uint8_t *p = prod.Dimming();
            uint8_t newdim;
            if (++adjDimmingSteps < 16)
            {
                tgt = (tgt > cur) ? *(p + cur + 1) : *(p + cur - 1);
                cur = *(p + cur);
                newdim = cur + (tgt - cur) * adjDimmingSteps / 16;
            }
            else
            { // last step
                if (tgt > cur)
                {
                    currentDimmingLvl++;
                }
                else
                {
                    currentDimmingLvl--;
                }
                newdim = *(p + currentDimmingLvl - 1);
                adjDimmingSteps = 0;
            }
            if (newdim != setDimming)
            {
                setDimming = newdim;
                /*
                PrintDbg("currentDimmingLvl=%d, targetDimmingLvl=%d, setDimming=%d\n",
                        currentDimmingLvl, targetDimmingLvl, setDimming);
                */
                RqstExtStatus(0xFF);
                r = true;
            }
        }
        dimmingAdjTimer.Setms(prod.DimmingAdjTime() * 1000 / 16);
    }
    return r;
}

APP::ERROR Group::SystemReset(uint8_t v)
{
    if (v >= 0)
    {
        SystemReset0();
    }
    if (v >= 1)
    {
        SystemReset1();
    }
    if (v >= 2)
    {
        SystemReset2();
    }
    return APP::ERROR::AppNoError;
}

void Group::SystemReset0()
{
    uint8_t buf[3];
    buf[0] = static_cast<uint8_t>(MI::CODE::SignDisplayFrame);
    buf[1] = groupId;
    buf[2] = 0;
    db.GetUciProcess().SetDisp(groupId, buf, 3);
    DispNext(DISP_STATUS::TYPE::FRM, 0);
    readyToLoad = 1;
    SetDimming(0);
    SetDevice(1);
    activeMsg.ClrAll();
    activeFrm.ClrAll();
}

void Group::SystemReset1()
{
    onDispPlnId = 0;
    onDispFrmId = 1; // force to issue a BLANK cmd to slaves
    EnDisPlan(0, false);
}

void Group::SystemReset2()
{
    // TODO reset all faults

    // TODO uciprocess
}
