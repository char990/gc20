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
#include <layer/SLV_LayerManager.h>

#define MS_SHIFT 5

using namespace Utils;

Group::Group(uint8_t groupId)
    : groupId(groupId), db(DbHelper::Instance()),
      fcltSw(PIN_G1_AUTO, PIN_G1_MSG1, PIN_G1_MSG2)
{
    busLockTmr.Setms(0);
    dimmingAdjTimer.Setms(0);
    UciProd &prod = db.GetUciProd();
    int signCnt = prod.NumberOfSigns();
    switch (prod.ExtStsRplSignType())
    {
    case SESR_SIGN_TYPE::TEXT:
        for (int i = 1; i <= signCnt; i++)
        {
            if (prod.GetGroupIdOfSign(i) == groupId)
            {
                vSigns.push_back(new SignTxt(i));
            }
        }
        break;
    case SESR_SIGN_TYPE::GFX:
        for (int i = 1; i <= signCnt; i++)
        {
            if (prod.GetGroupIdOfSign(i) == groupId)
            {
                vSigns.push_back(new SignGfx(i));
            }
        }
        break;
    case SESR_SIGN_TYPE::ADVGFX:
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
    dsNext->dispType = DISP_TYPE::BLK;
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
    deviceEnDisSet = proc.GetDevice(groupId);
    deviceEnDisCur = !deviceEnDisSet;

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
        s->InitFaults();
    }
    currentDimmingLvl = proc.GetDimming(groupId);
    targetDimmingLvl = currentDimmingLvl | 0x80;
    for (auto &sign : vSigns)
    {
        sign->DimmingSet(currentDimmingLvl);
        sign->DimmingV(currentDimmingLvl);
    }

    if (proc.GetPower(groupId) == 0)
    { // PowerOff
        pPinCmdPower = new GpioOut(PIN_MOSFET1_CTRL, 0);
        //PinCmdPowerOff();
        for (auto &sign : vSigns)
        {
            sign->SignErr(DEV::ERROR::PoweredOffByCommand, 1);
        }
        cmdPwr = PWR_STATE::OFF;
    }
    else
    { // PowerOn
        pPinCmdPower = new GpioOut(PIN_MOSFET1_CTRL, 1);
        //PinCmdPowerOn();
        for (auto &sign : vSigns)
        {
            sign->SignErr(DEV::ERROR::PoweredOffByCommand, 0);
        }
        cmdPwr = PWR_STATE::RISING;
    }
}

Group::~Group()
{
    delete pPinCmdPower;
    delete[] orBuf;
    delete[] txBuf;
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
    if (++grpTick >= CTRLLER_MS(100)) // every 10*10ms
    {
        grpTick = 0;
        FcltSwitchFunc();
        PowerFunc();
    }

    if (!IsPowerOn() || !IsBusFree())
    {
        return;
    }

    // fatal error
    bool rising = false;
    bool falling = false;
    bool fatal = false;
    for (auto &s : vSigns)
    {
        if (s->fatalError.IsRising())
        {
            rising = true;
            s->fatalError.ClearEdge();
        }
        else if (s->fatalError.IsFalling())
        {
            falling = true;
            s->fatalError.ClearEdge();
        }
        fatal |= s->fatalError.IsHigh();
    }
    if (rising)
    {
        newCurrent = 1;
        if (dsCurrent->dispType != DISP_TYPE::EXT)
        {
            dsBak->Clone(dsCurrent);
        }
        dsCurrent->BLK();
        dsNext->N_A();
    }
    else if (falling)
    {
        DispBackup();
    }

    if (!fatal)
    {
        if (IsDsNextEmergency())
        {
            readyToLoad = 1;
        }
        if (readyToLoad)
        {
            EnDisDevice();
            newCurrent = 0;
            if (dsCurrent->dispType == DISP_TYPE::EXT && extDispTmr.IsExpired())
            {
                extDispTmr.Clear();
                DispBackup();
            }
            newCurrent |= LoadDsNext(); // current->bak and next/fs/ext->current
        }
    }
    else
    { // if there is fatal, refresh en/dis at anytime
        EnDisDevice();
    }

    TaskPln(&taskPlnLine);
    TaskMsg(&taskMsgLine);
    TaskFrm(&taskFrmLine);

    TaskRqstSlave(&taskRqstSlaveLine);

    PeriodicHook();
}

void Group::PowerFunc()
{
    if (Controller::Instance().ctrllerError.IsSet(DEV::ERROR::PowerFailure))
    { // main power failed
        if (mainPwr != PWR_STATE::OFF)
        {
            mainPwr = PWR_STATE::OFF;
        }
    }
    else
    { // main power ok
        if (mainPwr == PWR_STATE::OFF || mainPwr == PWR_STATE::NA)
        {
            mainPwr = PWR_STATE::RISING;
        }
    }

    // combine mainPwr,cmdPwr,fsPwr
    if (mainPwr == PWR_STATE::OFF || cmdPwr == PWR_STATE::OFF || fsPwr == PWR_STATE::OFF)
    { // any one is OFF
        pwrUpTmr.Clear();
        GroupSetReportDisp(0, 0, 0);
    }
    else
    { // rising/on
        if (mainPwr == PWR_STATE::RISING || cmdPwr == PWR_STATE::RISING || fsPwr == PWR_STATE::RISING)
        { // any one is rising
            if (pwrUpTmr.IsClear())
            { // start power-up
                pwrUpTmr.Setms(db.GetUciProd().SlavePowerUpDelay() * 1000);
            }
            else if (pwrUpTmr.IsExpired())
            {
                if (fsPwr == PWR_STATE::RISING)
                { // from OFF switch to AUTO/MSG1/MSG2
                    auto fs = fcltSw.Get();
                    if (fs == FacilitySwitch::FS_STATE::AUTO)
                    { // from OFF switch to AUTO
                        DispNext(DISP_TYPE::FRM, 0);
                    }
                    else
                    { // from OFF switch to MSG1/MSG2
                        uint8_t msg = (fs == FacilitySwitch::FS_STATE::MSG1) ? 1 : 2;
                        DispNext(DISP_TYPE::FSW, msg);
                    }
                }
                // power-up done
                pwrUpTmr.Clear();
                extDispTmr.Clear();
                mainPwr = PWR_STATE::ON;
                cmdPwr = PWR_STATE::ON;
                fsPwr = PWR_STATE::ON;
                readyToLoad = 1;
            }
        }
    }
}

void Group::FcltSwitchFunc()
{
    fcltSw.PeriodicRun();
    auto fs = fcltSw.Get();
    if (fcltSw.IsChanged())
    {
        fcltSw.ClearChanged();
        Controller::Instance().ctrllerError.Push(
            DEV::ERROR::FacilitySwitchOverride, fs != FacilitySwitch::FS_STATE::AUTO);
        db.GetUciEvent().Push(0, "Group%d:%s", groupId, fcltSw.ToStr());
        if (fs == FacilitySwitch::FS_STATE::OFF)
        {
            fsPwr = PWR_STATE::OFF;
        }
        else
        { // not OFF
            if (fsPwr == PWR_STATE::OFF)
            {
                fsPwr = PWR_STATE::RISING;
            }
            else
            {
                if (IsPowerOn())
                { // power is ok, this means fsState is turning: AUTO<->MSG1<->MSG2
                    if (fs == FacilitySwitch::FS_STATE::AUTO)
                    {
                        DispNext(DISP_TYPE::FRM, 0);
                    }
                    else
                    {
                        uint8_t msg = (fs == FacilitySwitch::FS_STATE::MSG1) ? 1 : 2;
                        DispNext(DISP_TYPE::FSW, msg);
                    }
                }
            }
        }
    }
}

bool Group::TaskPln(int *_ptLine)
{
    if (!IsBusFree() || dsCurrent->dispType != DISP_TYPE::PLN || !readyToLoad)
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
            if (plnmin.plnId == 0 || !db.GetUciPln().IsPlnDefined(plnmin.plnId))
            {
                if (onDispPlnId != 0 || onDispMsgId != 0 || onDispFrmId != 0)
                { // previouse is not BLANK
                    PrintDbg(DBG_LOG, "TaskPln:Display:BLANK\n");
                    db.GetUciEvent().Push(0, "Display:BLANK");
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
                { // reset active frm/msg
                    PrintDbg(DBG_LOG, "TaskPln:Plan%d start\n", plnmin.plnId);
                    db.GetUciEvent().Push(0, "Plan%d start", plnmin.plnId);
                    activeMsg.ClrAll();
                    activeFrm.ClrAll();
                    auto pln = db.GetUciPln().GetPln(plnmin.plnId);
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
        taskPlnTmr.Setms(1000 - MS_SHIFT);
        PT_WAIT_UNTIL(taskPlnTmr.IsExpired());
    };
    PT_END();
}

extern time_t ds3231time(time_t *);
PlnMinute &Group::GetCurrentMinPln()
{
    time_t t = ds3231time(nullptr);
    struct tm stm;
    localtime_r(&t, &stm);
    return plnMin.at((stm.tm_wday * 24 + stm.tm_hour) * 60 + stm.tm_min);
}

bool Group::TaskMsg(int *_ptLine)
{
    uint8_t nextEntry; // temp using
    if (!IsBusFree() ||
        (dsCurrent->dispType != DISP_TYPE::MSG &&
         dsCurrent->dispType != DISP_TYPE::FSW &&
         dsCurrent->dispType != DISP_TYPE::EXT &&
         !(dsCurrent->dispType == DISP_TYPE::PLN && onDispPlnEntryType == PLN_ENTRY_MSG)))
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
        if (dsCurrent->dispType == DISP_TYPE::PLN && onDispPlnEntryType == PLN_ENTRY_MSG)
        {
            onDispMsgId = onDispPlnEntryId;
            // No need to set active msg, 'cause set in TaskPln
        }
        else
        {
            onDispPlnId = 0;
            onDispMsgId = dsCurrent->fmpid[0];
            if (dsCurrent->dispType == DISP_TYPE::MSG)
            {
                activeMsg.ClrAll(); // this is CmdDisplayMsg, Clear all previouse active msg
                // But for FSW/EXT, do not clear previouse
            }
            SetActiveMsg(onDispMsgId);
        }
        PrintDbg(DBG_LOG, "TaskMsg:Display Msg:%d\n", onDispMsgId);
    }
    PT_BEGIN();
    while (true)
    {
        pMsg = db.GetUciMsg().GetMsg(onDispMsgId);
        if (pMsg == nullptr)
        { // null msg task, only for DISP_TYPE::FSW/EXT
            if (dsCurrent->dispType != DISP_TYPE::FSW &&
                dsCurrent->dispType != DISP_TYPE::EXT)
            {
                // Something wrong, Load pln0 to run plan
                newCurrent = 1;
                dsCurrent->Pln0();
                // log
                TaskMsgReset();
                return false;
            }
            else
            {
                //for FSW/EXT, if msg undefined, show BLANK and report msgX+frm0
                ClrOrBuf();
                onDispFrmId = 0;
                GroupSetReportDisp(onDispFrmId, onDispMsgId, onDispPlnId);
                while (true)
                {
                    SlaveSetStoredFrame(0xFF, 0);
                    ClrAllSlavesRxStatus();
                    do
                    {
                        // step3: check current
                        taskFrmTmr.Setms(db.GetUciProd().SlaveRqstInterval() - MS_SHIFT);
                        PT_WAIT_UNTIL(CheckAllSlavesCurrent() >= 0 && taskFrmTmr.IsExpired());
                    } while (allSlavesCurrent == 0); // all good
                };                                   // end of null msg task
            }
        }
        else
        {
            while (true) // normal msg task
            {
            NORMAL_MSG_TASK_START:
                /// #1: init orbuf and counters
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
                /// #2: first frame 
                do
                {
                    SlaveSetFrame(0xFF, 1, pMsg->msgEntries[msgEntryCnt].frmId);
                    ClrAllSlavesRxStatus();
                    PT_WAIT_UNTIL(CheckAllSlavesNext() >= 0);
                } while (allSlavesNext == 1);
                msgSetEntry++;
                // +++++++++ after first round & first frame
                /// #3: msg loop begin
                do
                {
                    if (deviceEnDisCur)
                    {
                        readyToLoad = 0; // when a msg begin, clear readyToLoad
                    }

                    if (msgEntryCnt == pMsg->entries - 1 || pMsg->msgEntries[msgEntryCnt].onTime != 0)
                    { // overlay frame do not need to run DispFrm
                        onDispFrmId = pMsg->msgEntries[msgEntryCnt].frmId;
                        GroupSetReportDisp(onDispFrmId, onDispMsgId, onDispPlnId);
                        (pMsg->msgEntries[msgEntryCnt].onTime == 0) ? taskMsgTmr.Clear() // last entry && onTIme is 0, display forever
                                                                    : taskMsgTmr.Setms(pMsg->msgEntries[msgEntryCnt].onTime * 100 - 1);
                        if (msgEntryCnt == pMsg->entries - 1)
                        {
                            long lastFrmOn = db.GetUciUser().LastFrmOn();
                            (lastFrmOn == 0) ? taskMsgLastFrmTmr.Clear() : taskMsgLastFrmTmr.Setms(lastFrmOn * 1000);
                        }
                        else
                        {
                            taskMsgLastFrmTmr.Clear();
                        }
                        do // +++++++++ DispFrm X
                        {
                            SlaveSetStoredFrame(0xFF, msgEntryCnt + 1);
                            ClrAllSlavesRxStatus();
                            PT_WAIT_UNTIL(CheckAllSlavesCurrent() >= 0);
                            if (allSlavesCurrent == 0)
                            {
                                AllSlavesUpdateCurrentBak();
                            }
                        } while (allSlavesCurrent == 1); // Current is NOT matched but last is matched, re-issue SlaveSetStoredFrame
                        if (allSlavesCurrent == 2)
                        { // this is a fatal error, restart
                            PrintDbg(DBG_LOG, "TaskMsg:SetStoredFrame: Current NOT matched, RESTART\n");
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
                                PT_WAIT_UNTIL(CheckAllSlavesNext() >= 0);
                            } while (allSlavesNext == 1);
                        }
                        msgSetEntry++;
                    }
                    do // +++++++++ OnTime
                    {
                        taskFrmTmr.Setms(db.GetUciProd().SlaveRqstInterval() - MS_SHIFT);
                        PT_WAIT_UNTIL(CheckAllSlavesCurrent() >= 0 && taskFrmTmr.IsExpired());
                        if (msgEntryCnt == pMsg->entries - 1)
                        {
                            if (taskMsgLastFrmTmr.IsExpired())
                            {
                                taskMsgLastFrmTmr.Clear();
                                readyToLoad = 1; // last frame on expired
                            }
                        }
                    } while (allSlavesCurrent == 0 && !taskMsgTmr.IsExpired());
                    if (!taskMsgTmr.IsExpired())
                    { // Currnet is NOT matched, fatal error
                        PrintDbg(DBG_LOG, "TaskMsg:Frm-onTime: Current NOT matched, RESTART\n");
                        goto NORMAL_MSG_TASK_START;
                    }

                    // ++++++++++ OnTime finish
                    if (pMsg->msgEntries[msgEntryCnt].onTime != 0)
                    { // overlay frame do not need to run transition
                        // ++++++++++ transition time begin
                        if (pMsg->transTime != 0)
                        {
                            SlaveDisplayFrame(0xFF, 0);
                            taskMsgTmr.Setms(pMsg->transTime * 10 - MS_SHIFT);
                            PT_WAIT_UNTIL(taskMsgTmr.IsExpired());
                            AllSlavesUpdateCurrentBak();
                            // because transition time is generally short, don't have enough time to reload if there is an error, so just wait for expired
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
    ClrOrBuf();
    msgOverlay = 0;                 // no overlay
    msgSetEntryMax = pMsg->entries; // no overlay
    return;

    for (int i = 0; i < pMsg->entries; i++)
    {
        if ((pMsg->msgEntries[i].onTime == 0) && (i != (pMsg->entries - 1)))
        {
            msgOverlay=1;
        }
    }
    if(msgOverlay==0)
    {
        return;
    }
    // has overlay
    msgOverlay = 0; // colour code
    for (int i = 0; i < pMsg->entries; i++)
    {
        if ((pMsg->msgEntries[i].onTime == 0) && (i != (pMsg->entries - 1)))
        {
            msgOverlay=1;
        }
    }
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
        (dsCurrent->dispType != DISP_TYPE::FRM &&
         dsCurrent->dispType != DISP_TYPE::BLK &&
         dsCurrent->dispType != DISP_TYPE::ATF &&
         !(dsCurrent->dispType == DISP_TYPE::PLN && onDispPlnEntryType == PLN_ENTRY_FRM)))
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
        if (dsCurrent->dispType == DISP_TYPE::ATF)
        {
            onDispPlnId = 0;
            onDispFrmId = 1; // set onDispFrmId as 'NOT 0'
            activeMsg.ClrAll();
            activeFrm.Set(1); // TODO all frames in ATF
            PT_WAIT_UNTIL(TaskSetATF(&taskATFLine));
            PrintDbg(DBG_LOG, "TaskFrm:Display ATF\n");
        }
        else
        {
            if (dsCurrent->dispType == DISP_TYPE::FRM)
            { // from CmdDispFrm
                onDispPlnId = 0;
                onDispFrmId = dsCurrent->fmpid[0];
                activeMsg.ClrAll();
                activeFrm.Set(onDispFrmId);
            }
            else if (dsCurrent->dispType == DISP_TYPE::BLK)
            {
                onDispPlnId = 0;
                onDispFrmId = 0;
            }
            else
            { // from plan
                onDispFrmId = onDispPlnEntryId;
                // frm set active in TaskPln
            }
            PrintDbg(DBG_LOG, "TaskFrm:Display Frm:%d\n", onDispFrmId);
            if (onDispFrmId > 0)
            {
                do
                {
                    SlaveSetFrame(0xFF, (onDispFrmId == 0) ? 0 : 1, onDispFrmId);
                    ClrAllSlavesRxStatus();
                    PT_WAIT_UNTIL(CheckAllSlavesNext() >= 0);
                } while (allSlavesNext == 1);
            }
            GroupSetReportDisp(onDispFrmId, onDispMsgId, onDispPlnId);
        }
        // step2: display frame
        do
        {
            SlaveSetStoredFrame(0xFF, (onDispFrmId == 0) ? 0 : 1);
            taskFrmRefreshTmr.Setms(600 * 1000);
            ClrAllSlavesRxStatus();
            do
            {
                // step3: check current & next
                if (taskFrmRefreshTmr.IsExpired())
                { // sync
                    taskFrmRefreshTmr.Setms(600 * 1000);
                    SlaveSync();
                    PT_YIELD();
                }
                taskFrmTmr.Setms(db.GetUciProd().SlaveRqstInterval() - MS_SHIFT);
                PT_WAIT_UNTIL(CheckAllSlavesCurrent() >= 0 && taskFrmTmr.IsExpired());
                if (allSlavesCurrent == 0)
                {
                    AllSlavesUpdateCurrentBak();
                }
            } while (allSlavesCurrent == 0); // all good
        } while (allSlavesCurrent == 1);     // Current is NOT matched but last is matched, re-issue SlaveSetStoredFrame
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
        // Request status 1-n
        rqstStCnt = 0;
        do
        {
            rqstNoRplCnt = 0;
            do
            {
                taskRqstSlaveTmr.Setms(db.GetUciProd().SlaveRqstInterval() - MS_SHIFT);
                RqstStatus(rqstStCnt);
                vSlaves[rqstStCnt]->rxStatus = 0;
                PT_YIELD_UNTIL(taskRqstSlaveTmr.IsExpired());
                {
                    auto &s = vSlaves[rqstStCnt];
                    if (s->rxStatus == 0)
                    {
                        if (rqstNoRplCnt < db.GetUciProd().OfflineDebounce())
                        {
                            rqstNoRplCnt++;
                        }
                        else if (rqstNoRplCnt == db.GetUciProd().OfflineDebounce())
                        { // offline
                            rqstNoRplCnt++;
                            oprSp->ReOpen();
                        }
                        else if (rqstNoRplCnt < (db.GetUciProd().OfflineDebounce() + 3))
                        {
                            rqstNoRplCnt++;
                        }
                        else if (rqstNoRplCnt == (db.GetUciProd().OfflineDebounce() + 3))
                        {
                            rqstNoRplCnt++;
                            s->ReportOffline(true);
                        }
                    }
                    else
                    {
                        rqstNoRplCnt = 0;
                        if (s->sign->SignErr().IsSet(DEV::ERROR::InternalCommunicationsFailure))
                        {
                            s->ReportOffline(false);
                        }
                    }
                }
            } while (rqstNoRplCnt > 0);
            if (++rqstStCnt == vSlaves.size())
            {
                rqstStCnt = 0;
            }
        } while (rqstStCnt != 0);

        // Request Ext-status x
        rqstNoRplCnt = 0;
        do
        {
            taskRqstSlaveTmr.Setms(db.GetUciProd().SlaveRqstInterval() - MS_SHIFT);
            RqstExtStatus(rqstExtStCnt);
            vSlaves[rqstExtStCnt]->rxExtSt = 0;
            PT_YIELD_UNTIL(taskRqstSlaveTmr.IsExpired());
            {
                auto &s = vSlaves[rqstExtStCnt];
                if (s->rxExtSt == 0)
                {
                    if (rqstNoRplCnt < db.GetUciProd().OfflineDebounce())
                    {
                        rqstNoRplCnt++;
                    }
                    else if (rqstNoRplCnt == db.GetUciProd().OfflineDebounce())
                    { // offline
                        rqstNoRplCnt++;
                        oprSp->ReOpen();
                    }
                    else if (rqstNoRplCnt < (db.GetUciProd().OfflineDebounce() + 3))
                    {
                        rqstNoRplCnt++;
                    }
                    else if (rqstNoRplCnt == (db.GetUciProd().OfflineDebounce() + 3))
                    {
                        rqstNoRplCnt++;
                        s->ReportOffline(true);
                    }
                    //PT_EXIT();
                }
                else
                {
                    rqstNoRplCnt = 0;
                    if (s->sign->SignErr().IsSet(DEV::ERROR::InternalCommunicationsFailure))
                    {
                        s->ReportOffline(false);
                    }
                }
            }
        } while (rqstNoRplCnt > 0);
        if (++rqstExtStCnt == vSlaves.size())
        {
            rqstExtStCnt = 0;
        }

        // dimming adjusting
        if (DimmingAdjust())
        {
            taskRqstSlaveTmr.Setms(db.GetUciProd().SlaveRqstInterval() - MS_SHIFT);
            PT_WAIT_UNTIL(taskRqstSlaveTmr.IsExpired());
        }
    };
    PT_END();
}

int Group::CheckAllSlavesNext()
{
    for (auto &s : vSlaves)
    {
        int x = s->CheckNext();
        if (x != 0)
        {
            allSlavesNext = x;
            return x;
        }
    }
    allSlavesNext = 0;
    return allSlavesNext;
}

int Group::CheckAllSlavesCurrent()
{
    for (auto &s : vSlaves)
    {
        int x = s->CheckCurrent();
        if (x != 0)
        {
            allSlavesCurrent = x;
            return x;
        }
    }
    allSlavesCurrent = 0;
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

void Group::GroupSetReportDisp(uint8_t frmId, uint8_t msgId, uint8_t plnId)
{
    for (auto &s : vSigns)
    {
        s->SetReportDisp(frmId, msgId, plnId);
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
        APP::ERROR r = endis ? EnablePlan(id) : DisablePlan(id);
        if (r != APP::ERROR::AppNoError)
        {
            return r;
        }
    }
    db.GetUciProcess().EnDisPlan(groupId, id, endis);
    //PrintPlnMin();
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::EnablePlan(uint8_t id)
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

APP::ERROR Group::DisablePlan(uint8_t id)
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

bool Group::DispExt(uint8_t msgX)
{
    if (IsPowerOn() && FacilitySwitch::FS_STATE::AUTO == fcltSw.Get() && msgX >= 3 && msgX <= 6 &&
        // if there is a higher priority External input, ignore new input
        !((dsCurrent->dispType == DISP_TYPE::EXT && dsCurrent->fmpid[0] < msgX) ||
          (dsExt->dispType == DISP_TYPE::EXT && dsExt->fmpid[0] < msgX)))
    {
        DispNext(DISP_TYPE::EXT, msgX);
        return true;
    }
    return false;
}

void Group::DispNext(DISP_TYPE type, uint8_t id)
{
    if (type == DISP_TYPE::EXT)
    {
        auto cfg = db.GetUciUser().ExtSwCfgX(id);
        auto time = cfg->dispTime;
        if (time != 0)
        {
            if (dsCurrent->dispType == DISP_TYPE::EXT && dsCurrent->fmpid[0] == id)
            {
                extDispTmr.Setms(time * 1000);
            }
            else if (dsCurrent->dispType == DISP_TYPE::N_A ||
                     dsCurrent->dispType == DISP_TYPE::BLK ||
                     (onDispMsgId == 0 && onDispFrmId == 0) ||
                     cfg->flashingOv != 0 ||  // flash override OFF
                     (cfg->flashingOv == 0 && // flash override ON
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
    newCurrent = 1;
}

APP::ERROR Group::DispFrm(uint8_t id)
{
    if (mainPwr == PWR_STATE::OFF)
    {
        return APP::ERROR::PowerIsOff;
    }
    if (FacilitySwitch::FS_STATE::AUTO != fcltSw.Get())
    {
        return APP::ERROR::FacilitySwitchOverride;
    }
    if (cmdPwr == PWR_STATE::OFF)
    {
        return APP::ERROR::PowerIsOff;
    }
    uint8_t buf[3];
    buf[0] = static_cast<uint8_t>(MI::CODE::SignDisplayFrame);
    buf[1] = groupId;
    buf[2] = id;
    db.GetUciProcess().SetDisp(groupId, buf, 3);
    DispNext(DISP_TYPE::FRM, id);
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::DispMsg(uint8_t id)
{
    if (mainPwr == PWR_STATE::OFF)
    {
        return APP::ERROR::PowerIsOff;
    }
    if (FacilitySwitch::FS_STATE::AUTO != fcltSw.Get())
    {
        return APP::ERROR::FacilitySwitchOverride;
    }
    if (cmdPwr == PWR_STATE::OFF)
    {
        return APP::ERROR::PowerIsOff;
    }
    uint8_t buf[3];
    buf[0] = static_cast<uint8_t>(MI::CODE::SignDisplayMessage);
    buf[1] = groupId;
    buf[2] = id;
    db.GetUciProcess().SetDisp(groupId, buf, 3);
    DispNext(DISP_TYPE::MSG, id);
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::SetDimming(uint8_t dimming)
{
    db.GetUciProcess().SetDimming(groupId, dimming);
    targetDimmingLvl = 0x80 | dimming;
    for (auto &sign : vSigns)
    {
        sign->DimmingSet(dimming);
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::SetPower(uint8_t v)
{
    if (v == 0)
    { // PowerOff
        if (cmdPwr != PWR_STATE::OFF)
        {
            PinCmdPowerOff();
            db.GetUciProcess().SetPower(groupId, 0);
            for (auto &sign : vSigns)
            {
                sign->SignErr(DEV::ERROR::PoweredOffByCommand, 1);
            }
            cmdPwr = PWR_STATE::OFF;
        }
    }
    else
    { // PowerOn
        if (cmdPwr == PWR_STATE::OFF || cmdPwr == PWR_STATE::NA)
        {
            PinCmdPowerOn();
            db.GetUciProcess().SetPower(groupId, 1);
            for (auto &sign : vSigns)
            {
                sign->SignErr(DEV::ERROR::PoweredOffByCommand, 0);
            }
            cmdPwr = PWR_STATE::RISING;
        }
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::SetDevice(uint8_t endis)
{
    deviceEnDisSet = (endis == 0) ? 0 : 1;
    db.GetUciProcess().SetDevice(groupId, deviceEnDisSet);
    return APP::ERROR::AppNoError;
}

void Group::EnDisDevice()
{
    if (deviceEnDisSet != deviceEnDisCur)
    {
        TaskMsgReset();
        TaskFrmReset();
        deviceEnDisCur = deviceEnDisSet;
        for (auto &s : vSigns)
        {
            s->Device(deviceEnDisSet);
        }
    }
}

bool Group::IsDsNextEmergency()
{
    bool r = false;
    if (dsExt->dispType == DISP_TYPE::EXT)
    { // external input
        uint8_t mid = dsExt->fmpid[0];
        if (mid >= 3 && mid <= 5)
        {
            if (db.GetUciUser().ExtSwCfgX(mid)->emergency == 0)
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
    if (dsNext->dispType == DISP_TYPE::FSW)
    { // facility switch
        dsBak->Frm0();
        dsCurrent->Clone(dsNext);
        dsNext->N_A();
        r = true;
    }
    else if (dsExt->dispType == DISP_TYPE::EXT)
    { // external input
        dsBak->Clone(dsCurrent);
        dsCurrent->Clone(dsExt);
        dsExt->N_A();
        r = true;
    }
    else if (dsNext->dispType != DISP_TYPE::N_A)
    { // display command
        dsBak->Clone(dsCurrent);
        dsCurrent->Clone(dsNext);
        dsNext->N_A();
        r = true;
    }
    if (dsCurrent->dispType == DISP_TYPE::N_A)
    { // if current == N/A, load frm[0] to activate plan
        dsBak->Frm0();
        dsCurrent->Frm0();
        r = true;
    }
    if (dsCurrent->fmpid[0] == 0 &&
        (dsCurrent->dispType == DISP_TYPE::FRM || dsCurrent->dispType == DISP_TYPE::MSG))
    { //frm[0] or msg[0], activate plan
        dsCurrent->dispType = DISP_TYPE::PLN;
        r = true;
    }
    return r;
}

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
    for (auto &s : vSlaves)
    {
        s->DecodeStRpl(data, len);
    }
}

void Group::SlaveExtStatusRpl(uint8_t *data, int len)
{
    if (len < 22)
        return;
    for (int i = 0; i < vSlaves.size(); i++)
    {
        if (vSlaves[i]->DecodeExtStRpl(data, len) == 0)
        {
            vSlaves[i]->sign->RefreshSlaveStatusAtExtSt();
            return;
        }
    }
}

int Group::RqstStatus(uint8_t slvindex)
{
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

int Group::SlaveSync()
{
    txBuf[0] = 0xFF;
    txBuf[1] = SLVCMD::SYNC;
    txLen = 2;
    Tx();
    int ms = db.GetUciProd().SlaveDispDly();
    LockBus(ms);
    return ms;
}

int Group::SlaveSetFrame(uint8_t slvindex, uint8_t slvFrmId, uint8_t uciFrmId)
{
    if (slvFrmId == 0 || uciFrmId == 0)
    {
        MyThrow("ERROR: SlaveSetFrame(slvindex=%d, slvFrmId=%d, uciFrmId=%d)",
                slvindex, slvFrmId, uciFrmId);
    }
    int ms = 10;
    if (deviceEnDisCur)
    {
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
            }
        }
        else
        {
            auto &s = vSlaves[slvindex];
            s->expectNextFrmId = slvFrmId;
            s->frmCrc[slvFrmId] = crc;
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
    if (deviceEnDisCur == 0)
    {
        slvFrmId = 0;
    }
    if (slvindex == 0xFF)
    {
        txBuf[0] = 0xFF;
        for (auto &s : vSlaves)
        {
            s->expectCurrentFrmId = slvFrmId;
            s->expectNextFrmId = slvFrmId;
        }
    }
    else
    {
        auto &s = vSlaves[slvindex];
        txBuf[0] = s->SlaveId();
        s->expectCurrentFrmId = slvFrmId;
        s->expectNextFrmId = slvFrmId;
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
    if (deviceEnDisCur == 0)
    {
        slvFrmId = 0;
    }
    if (slvindex == 0xFF)
    {
        txBuf[0] = 0xFF;
        for (auto &s : vSlaves)
        {
            s->expectCurrentFrmId = slvFrmId;
            s->expectNextFrmId = slvFrmId;
        }
    }
    else
    {
        auto &s = vSlaves[slvindex];
        txBuf[0] = s->SlaveId();
        s->expectCurrentFrmId = slvFrmId;
        s->expectNextFrmId = slvFrmId;
    }
    txBuf[1] = SLVCMD::SET_STD_FRM;
    txBuf[2] = slvFrmId;
    txLen = 3;
    Tx();
    int ms = db.GetUciProd().SlaveDispDly();
    LockBus(ms);
    return ms;
}

void Group::AllSlavesUpdateCurrentBak()
{
    for (auto &s : vSlaves)
    {
        s->currentFrmIdBak = s->expectCurrentFrmId;
    }
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
        if (targetDimmingLvl != 0)
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
        int tgt;
        if (targetDimmingLvl == 0)
        {
            int lux = 0;
            int luxCnt = 0;
            for (auto &s : vSigns)
            {
                if (s->luminanceFault.IsLow())
                {
                    lux += s->Lux();
                    luxCnt++;
                }
            }
            lux = (luxCnt == 0) ? -1 /*all lightsensors are faulty*/ : lux / luxCnt /*average*/;
            tgt = db.GetUciUser().GetLuxLevel(lux) - 1; // 0-15
        }
        else
        {
            tgt = targetDimmingLvl - 1; // 0-15
        }
        int cur = (currentDimmingLvl - 1); // 0-15
        UciProd &prod = db.GetUciProd();
        if (tgt != cur)
        {
            int tgtLevel = tgt + 1;
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
                PrintDbg(DBG_LOG, "currentDimmingLvl=%d, targetDimmingLvl=%d(%d), setDimming=%d\n",
                         currentDimmingLvl, targetDimmingLvl, tgtLevel, setDimming);
                         */
                RqstExtStatus(0xFF);
                r = true;
                if (targetDimmingLvl == 0)
                {
                    for (auto &sign : vSigns)
                    {
                        sign->DimmingSet(targetDimmingLvl);
                        sign->DimmingV(currentDimmingLvl);
                    }
                }
            }
        }
        dimmingAdjTimer.Setms(prod.DimmingAdjTime() * 1000 / 16 - MS_SHIFT);
    }
    return r;
}

APP::ERROR Group::SystemReset(uint8_t v)
{
    switch (v)
    {
    case 0:
        SystemReset0();
        break;
    case 1:
        SystemReset1();
        break;
    case 2:
    case 3:
    case 255:
        SystemReset2();
        break;
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
    DispNext(DISP_TYPE::FRM, 0);
    readyToLoad = 1;
    SetDimming(0);
    SetDevice(1);
    activeMsg.ClrAll();
    activeFrm.ClrAll();
}

void Group::SystemReset1()
{
    SystemReset0();
    onDispPlnId = 0;
    onDispFrmId = 1; // force to issue a BLANK cmd to slaves
    EnDisPlan(0, false);
}

void Group::SystemReset2()
{
    SystemReset1();
    auto &proc = db.GetUciProcess();
    // clear all faults
    for (auto &s : vSigns)
    {
        s->ClearFaults();
        proc.SaveSignErr(s->SignId(), 0);
    }
}
