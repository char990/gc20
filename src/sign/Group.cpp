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

extern time_t GetTime(time_t *);

using namespace Utils;

Group::Group(uint8_t groupId)
    : groupId(groupId),
      db(DbHelper::Instance()),
      prod(DbHelper::Instance().GetUciProd()),
      user(DbHelper::Instance().GetUciUser()),
      fcltSw(PIN_G1_AUTO, PIN_G1_MSG1, PIN_G1_MSG2)
{
    busLockTmr.Setms(0);
    dimmingAdjTimer.Setms(0);
    int totalSign = prod.NumberOfSigns();
    switch (prod.ExtStsRplSignType())
    {
    case SESR_SIGN_TYPE::TEXT:
        for (int i = 1; i <= totalSign; i++)
        {
            if (prod.GetGroupIdOfSign(i) == groupId)
            {
                vSigns.push_back(new SignTxt(i));
            }
        }
        break;
    case SESR_SIGN_TYPE::GFX:
        for (int i = 1; i <= totalSign; i++)
        {
            if (prod.GetGroupIdOfSign(i) == groupId)
            {
                vSigns.push_back(new SignGfx(i));
            }
        }
        break;
    case SESR_SIGN_TYPE::ADVGFX:
        for (int i = 1; i <= totalSign; i++)
        {
            if (prod.GetGroupIdOfSign(i) == groupId)
            {
                vSigns.push_back(new SignAdg(i));
            }
        }
        break;
    }
    if (SignCnt() == 0)
    {
        throw std::invalid_argument(FmtException("Error:There is no sign in Group[%d]", groupId));
    }
    dsBak = new DispStatus(SignCnt());
    dsCurrent = new DispStatus(SignCnt());
    dsNext = new DispStatus(SignCnt());
    dsExt = new DispStatus(SignCnt());
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
        s->SignErr(proc.SignErr(s->SignId()));
        s->InitFaults();
    }
    auto tgtDim = proc.GetDimming(groupId);
    targetDimmingLvl = tgtDim | 0x80;
    currentDimmingLvl = 1;
    uint8_t tdv = tgtDim != 0 ? tgtDim : 1;
    for (auto &sign : vSigns)
    {
        sign->DimmingSet(tgtDim);
        sign->DimmingV(tdv);
    }

    bool pwr = (proc.GetPower(groupId) != 0);
    pPinCmdPower = new GpioOut(PIN_MOSFET1_CTRL, pwr);
    for (auto &sign : vSigns)
    {
        sign->SignErr(DEV::ERROR::PoweredOffByCommand, !pwr);
    }
    cmdPwr = pwr ? PWR_STATE::RISING : PWR_STATE::OFF;
    fatalError.Init(STATE5::S5_0);
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
    for (int i = 0; i < SignCnt(); i++)
    {
        delete vSigns[i];
    }
    for (int i = 0; i < SlaveCnt(); i++)
    {
        delete vSlaves[i];
    }
}

void Group::LoadLastDisp()
{ // load disp
    if (prod.LoadLastDisp())
    {
        auto disp = db.GetUciProcess().GetDisp(groupId);
        if (disp[0] > 0)
        {
            switch (disp[1])
            {
            case static_cast<uint8_t>(MI::CODE::SignDisplayFrame):
                DispFrm(disp[3], false);
                break;
            case static_cast<uint8_t>(MI::CODE::SignDisplayMessage):
                DispMsg(disp[3], false);
                break;
            case static_cast<uint8_t>(MI::CODE::SignDisplayAtomicFrames):
                DispAtmFrm(disp + 1, false);
                break;
            default:
                throw std::invalid_argument(FmtException("Syntax Error: UciProcess.Group%d.Display", groupId));
                break;
            }
        }
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
    bool fatal = false;
    for (auto &sign : vSigns)
    {
        fatal |= sign->fatalError.IsHigh();
    }
    for (auto &slv : vSlaves)
    {
        fatal |= slv->isOffline;
    }
    fatal ? fatalError.Set() : fatalError.Clr();

    if (fatalError.IsRising())
    {
        if (dsCurrent->dispType != DISP_TYPE::EXT)
        {
            dsBak->Clone((dsNext->dispType != DISP_TYPE::N_A && dsNext->dispType != DISP_TYPE::EXT) ? dsNext : dsCurrent);
        }
        ReloadCurrent(1); // force to reload
        dsNext->N_A();    // avoid to call LoadDsNext()
        dsCurrent->BLK(); // set current, force TaskFrm to display Frm[0]
        fatalError.ClearRising();
        Ldebug("Group[%d] - fatalError Set", groupId);
    }
    else if (fatalError.IsFalling())
    {
        DispBackup();
        dsCurrent->N_A();
        ReadyToLoad(1);
        fatalError.ClearFalling();
        Ldebug("Group[%d] - fatalError Clr", groupId);
    }

    if (fatalError.IsLow())
    { // only when there is NO fatal error
        if (IsDsNextEmergency())
        {
            ReadyToLoad(1);
        }
        if (readyToLoad)
        {
            EnDisDevice();
            ReloadCurrent(0);
            if (dsCurrent->dispType == DISP_TYPE::EXT && extDispTmr.IsExpired())
            {
                extDispTmr.Clear();
                DispBackup();
            }
            if (LoadDsNext()) // current->bak and next/fs/ext->current
            {
                ReloadCurrent(1);
            }
        }
    }
    else
    { // if there is fatal, refresh en/dis at anytime
        EnDisDevice();
    }

    TaskPln(&taskPlnLine);
    TaskMsg(&taskMsgLine);
    TaskFrm(&taskFrmLine);

    TaskAdjustDimming(&taskAdjustDimmingLine);

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
                pwrUpTmr.Setms(prod.SlavePowerUpDelay() * 1000);
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
                //rqstNoRplTmr.Setms((prod.OfflineDebounce() - 1) * 1000);
                TaskRqstSlaveReset();
                mainPwr = PWR_STATE::ON;
                cmdPwr = PWR_STATE::ON;
                fsPwr = PWR_STATE::ON;
                ReadyToLoad(1);
                RqstExtStatus(0xFF);
                Ldebug("Group[%d] - PowerUp done", groupId);
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
        char buf[256];
        snprintf(buf, 255, "Group[%d] - %s", groupId, fcltSw.ToStr());
        Ldebug("%s", buf);
        db.GetUciEvent().Push(0, buf);
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
            else if (fsPwr == PWR_STATE::NA)
            {
                fsPwr = PWR_STATE::ON;
            }
            else
            {
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

bool Group::TaskPln(int *_ptLine)
{
    if (!IsBusFree())
        return PT_RUNNING;
    if (dsCurrent->dispType != DISP_TYPE::PLN || !readyToLoad)
    {
        return PT_RUNNING;
    }
    if (reloadCurrent)
    {
        ReloadCurrent(0);
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
                    char buf[256];
                    snprintf(buf, 255, "Group[%d] - TaskPln : Display : BLANK", groupId);
                    Ldebug("%s", buf);
                    db.GetUciEvent().Push(0, buf);
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
                    char buf[256];
                    snprintf(buf, 255, "Group[%d] - TaskPln : Plan[%d] start", groupId, plnmin.plnId);
                    Ldebug("%s", buf);
                    db.GetUciEvent().Push(0, buf);
                    activeMsg.ClrAll();
                    activeFrm.ClrAll();
                    auto pln = db.GetUciPln().GetPln(plnmin.plnId);
                    for (int i = 0; i < pln->entries; i++)
                    {
                        if (pln->plnEntries[i].fmType == PLN_ENTRY_FRM)
                        {
                            activeFrm.SetBit(pln->plnEntries[i].fmId);
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

PlnMinute &Group::GetCurrentMinPln()
{
    time_t t = GetTime(nullptr);
    struct tm stm;
    localtime_r(&t, &stm);
    return plnMin.at((stm.tm_wday * 24 + stm.tm_hour) * 60 + stm.tm_min);
}

bool Group::TaskMsg(int *_ptLine)
{
    uint8_t nextEntry; // temp using
    if (!IsBusFree())
        return PT_RUNNING;
    if ((dsCurrent->dispType != DISP_TYPE::MSG &&
         dsCurrent->dispType != DISP_TYPE::FSW &&
         dsCurrent->dispType != DISP_TYPE::EXT &&
         !(dsCurrent->dispType == DISP_TYPE::PLN && onDispPlnEntryType == PLN_ENTRY_MSG)))
    {
        return PT_RUNNING;
    }
    if (reloadCurrent)
    {
        ReloadCurrent(0);
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
        Ldebug("Group[%d] - TaskMsg : Display Msg[%d]", groupId, onDispMsgId);
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
                ReloadCurrent(1);
                dsCurrent->Pln0();
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
                        taskFrmTmr.Setms(prod.SlaveRqstInterval() - MS_SHIFT);
                        PT_WAIT_UNTIL(CheckAllSlavesCurrent() >= 0 && taskFrmTmr.IsExpired());
                    } while (allSlavesCurrent == 0); // all good
                };
            }
        }
        else
        {
            while (true) // normal msg task
            {
            NORMAL_MSG_TASK_START:
                /// #1: init orbuf and counters
                InitMsgOverlayBuf(pMsg);
                //printf("\nmsg start\n");
                //PrintOrBuf();
                msgSetEntry = 0;
                msgDispEntry = 0;
                // set & display, TotalRound
                // +++++++++ first round & first entry
                if (pMsg->msgEntries[msgDispEntry].onTime == 0)
                {
                    ITransFrmWithOrBuf(pMsg->msgEntries[msgDispEntry].frmId, orBuf);
                }
                else
                {
                    do
                    {
                        SlaveSetFrame(0xFF, 1, pMsg->msgEntries[msgDispEntry].frmId);
                        ClrAllSlavesRxStatus();
                        PT_WAIT_UNTIL(CheckAllSlavesNext() >= 0);
                    } while (allSlavesNext == 1);
                }
                msgSetEntry++;
                //printf("\nAfter 1st entry\n");
                //PrintOrBuf();
                // +++++++++ after first round & first frame
                /// #3: msg loop begin
                do
                {
                    if (deviceEnDisCur)
                    {
                        ReadyToLoad(0); // when a msg begin, clear readyToLoad
                    }
                    if (msgDispEntry == pMsg->entries - 1 || pMsg->msgEntries[msgDispEntry].onTime != 0)
                    { // overlay frame do not need to run DispFrm
                        onDispFrmId = pMsg->msgEntries[msgDispEntry].frmId;
                        GroupSetReportDisp(onDispFrmId, onDispMsgId, onDispPlnId);
                        (pMsg->msgEntries[msgDispEntry].onTime == 0) ? taskMsgTmr.Clear() // last entry && onTIme is 0, display forever
                                                                     : taskMsgTmr.Setms(pMsg->msgEntries[msgDispEntry].onTime * 100 - 1);
                        if (msgDispEntry == pMsg->entries - 1)
                        { // last entry, start lastFrmOn
                            long lastFrmOn = user.LastFrmOn();
                            (lastFrmOn == 0) ? taskMsgLastFrmTmr.Clear() : taskMsgLastFrmTmr.Setms(lastFrmOn * 1000);
                        }
                        else
                        {
                            taskMsgLastFrmTmr.Clear();
                        }
                        msgSlaveErrCnt = 0;
                        do // +++++++++ DispFrm X
                        {
                            SlaveSetStoredFrame(0xFF, msgDispEntry + 1);
                            ClrAllSlavesRxStatus();
                            PT_WAIT_UNTIL(CheckAllSlavesCurrent() >= 0);
                            if (allSlavesCurrent == 0)
                            {
                                AllSlavesUpdateCurrentBak();
                            }
                            else if (allSlavesCurrent == 3)
                            {
                                if (++msgSlaveErrCnt == 3)
                                {
                                    Ldebug("Group[%d] - TaskMsg : SetStoredFrame : Slave may reset, RESTART", groupId);
                                    goto NORMAL_MSG_TASK_START;
                                }
                            }
                        } while (allSlavesCurrent == 1 || allSlavesCurrent == 3); // Current is NOT matched but last is matched, re-issue SlaveSetStoredFrame
                        if (allSlavesCurrent == 2)
                        { // this is a fatal error, restart
                            Ldebug("Group[%d] - TaskMsg : SetStoredFrame : Current NOT matched, RESTART", groupId);
                            goto NORMAL_MSG_TASK_START;
                        }
                        // ++++++++++ DispFrm X done
                    }
                    // load next frame to set or make orbuf
                    if (msgSetEntry < msgSetEntryMax)
                    {
                        nextEntry = (msgDispEntry == pMsg->entries - 1) ? 0 : (msgDispEntry + 1);
                        //printf("\nBefore nextEntry=%d\n", nextEntry);
                        //PrintOrBuf();
                        if (pMsg->msgEntries[nextEntry].onTime == 0 && nextEntry != (pMsg->entries - 1))
                        { // onTime==0 and not last entry, trans frame to set orBuf
                            if (msgSetEntry < pMsg->entries)
                            { // set orBuf at first round
                                ITransFrmWithOrBuf(pMsg->msgEntries[nextEntry].frmId, orBuf);
                            }
                        }
                        else
                        {
                            // Set next frame
                            do
                            {
                                SlaveSetFrame(0xFF, nextEntry + 1, pMsg->msgEntries[nextEntry].frmId);
                                ClrAllSlavesRxStatus();
                                PT_WAIT_UNTIL(CheckAllSlavesNext() >= 0);
                            } while (allSlavesNext == 1);
                        }
                        msgSetEntry++;
                        //printf("\nAfter nextEntry=%d\n", nextEntry);
                        //PrintOrBuf();
                    }
                    if (msgDispEntry == pMsg->entries - 1 || pMsg->msgEntries[msgDispEntry].onTime != 0)
                    {      // OnTime is for displayed frm only
                        do // +++++++++ OnTime
                        {
                            taskFrmTmr.Setms(prod.SlaveRqstInterval() - MS_SHIFT);
                            PT_WAIT_UNTIL(CheckAllSlavesCurrent() >= 0 && taskFrmTmr.IsExpired());
                            if (msgDispEntry == pMsg->entries - 1)
                            {
                                if (taskMsgLastFrmTmr.IsExpired())
                                {
                                    taskMsgLastFrmTmr.Clear();
                                    ReadyToLoad(1); // last frame on expired
                                }
                            }
                        } while (allSlavesCurrent == 0 && !taskMsgTmr.IsExpired());
                        if (!taskMsgTmr.IsExpired())
                        { // Currnet is NOT matched, fatal error
                            Ldebug("Group[%d] - TaskMsg : Frm-onTime : Current NOT matched, RESTART", groupId);
                            goto NORMAL_MSG_TASK_START;
                        }
                    }
                    // ++++++++++ OnTime finish
                    if (pMsg->msgEntries[msgDispEntry].onTime != 0)
                    { // overlay frame do not need to run transition
                        // ++++++++++ transition time begin
                        if (pMsg->transTime != 0)
                        {
                            if (msgDispEntry == (pMsg->entries - 1))
                            {
                                // this is last entry, msg finished, set readyToLoad=1. If there is new setting, could be loaded
                                ReadyToLoad(1);
                            }
                            taskMsgTmr.Setms(pMsg->transTime * 10 - MS_SHIFT);
                            do
                            {
                                SlaveDisplayFrame(0xFF, 0);
                                ClrAllSlavesRxStatus();
                                AllSlavesUpdateCurrentBak();
                                PT_WAIT_UNTIL(CheckAllSlavesCurrent() > 0 || taskMsgTmr.IsExpired());
                            } while (!taskMsgTmr.IsExpired());
                        }
                        // +++++++++++ transition time end
                    }
                    // entry done!
                    if (msgDispEntry == (pMsg->entries - 1))
                    {
                        // last frm finished, set readyToLoad=1 and YIELD. If there is new setting, could be loaded
                        ReadyToLoad(1);
                        PT_YIELD();
                        msgDispEntry = 0;
                    }
                    else
                    {
                        msgDispEntry++;
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
    for (int i = 0; i < pMsg->entries; i++)
    {
        if (i == (pMsg->entries - 1))
        {
            return; // no overlay
        }
        else
        {
            if (pMsg->msgEntries[i].onTime == 0)
            {
                break; // has overlay
            }
        }
    }
    bool onTime1 = false;
    for (int i = 0; i < pMsg->entries; i++)
    {
        auto &me = pMsg->msgEntries[i];
        if (me.onTime == 0)
        {
            if (i != (pMsg->entries - 1))
            { // NOT last entry
                msgOverlay = prod.ColourBits();
                if (onTime1)
                { // had an entry with ontime>0
                    msgSetEntryMax = pMsg->entries + i;
                }
            }
            else
            { // last entry ontime is 0, so just set max
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
    if (!IsBusFree())
        return PT_RUNNING;
    if ((dsCurrent->dispType != DISP_TYPE::FRM &&
         dsCurrent->dispType != DISP_TYPE::BLK &&
         dsCurrent->dispType != DISP_TYPE::ATF &&
         !(dsCurrent->dispType == DISP_TYPE::PLN && onDispPlnEntryType == PLN_ENTRY_FRM)))
    {
        return PT_RUNNING;
    }

    if (reloadCurrent)
    {
        ReloadCurrent(0);
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
        taskFrmRefreshTmr.Clear();
        if (dsCurrent->dispType == DISP_TYPE::ATF)
        {
            onDispPlnId = 0;
            onDispFrmId = 1; // set onDispFrmId as 'NOT 0'
            activeMsg.ClrAll();
            activeFrm.ClrAll();
            for (int i = 0; i < SignCnt(); i++)
            {
                activeFrm.SetBit(dsCurrent->fmpid[i]);
                vSigns.at(i)->SetReportDisp(dsCurrent->fmpid[i], 0, 0);
            }
            TaskSetATFReset();
            PT_WAIT_TASK(TaskSetATF(&taskATFLine));
            {
                char buf[256];
                int len = 0;
                for (int i = 0; i < SlaveCnt(); i++)
                {
                    sprintf(buf + len, " %d-%d", vSlaves.at(i)->SlaveId(), dsCurrent->fmpid[i]);
                }
                Ldebug("Group[%d] - TaskFrm : Display ATF :(signId-frmId)%s", groupId, buf);
            }
        }
        else
        {
            if (dsCurrent->dispType == DISP_TYPE::FRM)
            { // from CmdDispFrm
                onDispPlnId = 0;
                onDispFrmId = dsCurrent->fmpid[0];
                activeMsg.ClrAll();
                activeFrm.ClrAll();
                activeFrm.SetBit(onDispFrmId);
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
            Ldebug("Group[%d] - TaskFrm : Display Frm[%d]", groupId, onDispFrmId);
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
            taskFrmRefreshTmr.Setms(taskFrmRefreshTmr.IsClear() ? 600 * 1000 : 1000);
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
                taskFrmTmr.Setms(prod.SlaveRqstInterval() - MS_SHIFT);
                PT_WAIT_UNTIL(CheckAllSlavesCurrent() >= 0 && taskFrmTmr.IsExpired());
                if (allSlavesCurrent == 0)
                {
                    AllSlavesUpdateCurrentBak();
                }
            } while (allSlavesCurrent == 0); // all good
        } while (allSlavesCurrent == 1);     // Current is NOT matched but last is matched, re-issue SlaveSetStoredFrame
        // if allSlavesCurrent == 2, slave may reset, re-start
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
        activeFrm.SetBit(msg->msgEntries[i].frmId);
    }
    activeMsg.SetBit(mid);
}

uint8_t Group::GetTargetDimmingLvl() // 1-16
{
    int tgt = targetDimmingLvl & 0x7F; // 0, 1-16
    if (tgt == 0 || tgt > 16)
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
        groupLux = lux;
        tgt = user.GetLuxLevel(lux); // 1- 16
    }
    return tgt;
}

//#define DEBUG_ADJ_DIMMING
bool Group::TaskAdjustDimming(int *_ptLine)
{
    PT_BEGIN();
    if (targetDimmingLvl & 0x80) // bootup flag to set dimming for first time
    {
        PT_WAIT_UNTIL(IsBusFree());
        // currentDimmingLvl = 1;
        setDimming = prod.Dimming(currentDimmingLvl);
        RqstExtStatus(0xFF); // set 1
        targetDimmingLvl &= 0x7F;
    }
    while (true)
    {
        PT_YIELD();
        tdl = GetTargetDimmingLvl();
        if (tdl == currentDimmingLvl)
        {
            return PT_RUNNING;
        }
        if (tdl > currentDimmingLvl)
        {
            targetDimmingV = prod.Dimming(currentDimmingLvl + 1);
        }
        else
        {
            targetDimmingV = prod.Dimming(currentDimmingLvl - 1);
        }
        currentDimmingV = prod.Dimming(currentDimmingLvl);
#ifdef DEBUG_ADJ_DIMMING
        Ldebug("Group[%d] - current:lvl=%d,V=%d; target:lvl=%d,V=%d",
                 groupId, currentDimmingLvl, currentDimmingV, tdl, targetDimmingV);
#endif
        for (adjDimmingSteps = 0; adjDimmingSteps < 16; adjDimmingSteps++)
        {
            dimmingAdjTimer.Setms(prod.DimmingAdjTime() * 1000 / 16);
            uint8_t dv;
            if (adjDimmingSteps == 15)
            {
                if (targetDimmingV < currentDimmingV)
                {
                    currentDimmingLvl--;
                }
                else
                {
                    currentDimmingLvl++;
                }
                dv = targetDimmingV;
            }
            else
            {
                dv = currentDimmingV + (targetDimmingV - currentDimmingV) * (adjDimmingSteps + 1) / 16;
            }
            if (setDimming != dv)
            {
                setDimming = dv;
#ifdef DEBUG_ADJ_DIMMING
                Ldebug("Group[%d] - adjDimmingSteps=%d, currentDimmingLvl=%d, setDimming=%d",
                         groupId, adjDimmingSteps, currentDimmingLvl, setDimming);
#endif
                PT_WAIT_UNTIL(IsBusFree());
                RqstExtStatus(0xFF);
            }
            PT_WAIT_UNTIL(dimmingAdjTimer.IsExpired());
        }
#ifdef DEBUG_ADJ_DIMMING
        Ldebug("Group[%d] - AdjDiming done - current:lvl=%d,V=%d; target:lvl=%d,V=%d",
                 groupId, currentDimmingLvl, setDimming, tdl, targetDimmingV);
#endif
        if (targetDimmingLvl == 0)
        {
            for (auto &sign : vSigns)
            {
                sign->DimmingSet(targetDimmingLvl);
                sign->DimmingV(currentDimmingLvl);
            }
        }
    }; // end of while(true)
    PT_END();
}

bool Group::TaskRqstSlave(int *_ptLine)
{
    if (!IsBusFree())
        return PT_RUNNING;
    PT_BEGIN();
    while (true)
    {
        // Request status 1-n
        do
        {
            taskRqstSlaveTmr.Setms(prod.SlaveRqstInterval() - MS_SHIFT);
            RqstStatus(rqstSt_slvindex);
            {
                auto slave = vSlaves.at(rqstSt_slvindex);
                slave->rxStatus = 0;
                if (slave->rqstNoRplTmr.IsClear())
                {
                    slave->rqstNoRplTmr.Setms(prod.OfflineDebounce() * 1000);
                }
            }
            PT_YIELD_UNTIL(taskRqstSlaveTmr.IsExpired());
            {
                auto s = vSlaves.at(rqstSt_slvindex);
                if (s->GetRxStatus())
                {
                    s->rqstNoRplTmr.Clear();
                    if (s->isOffline)
                    {
                        s->ReportOffline(false);
                        s->sign->RefreshDevErr(DEV::ERROR::InternalCommunicationsFailure);
                    }
                }
                else
                {
                    if (s->rqstNoRplTmr.IsExpired())
                    {
                        if (!s->isOffline)
                        {
                            s->ReportOffline(true);
                            s->sign->RefreshDevErr(DEV::ERROR::InternalCommunicationsFailure);
                        }
                    }
                }
            }

            if (rqstSt_slvindex == SlaveCnt() - 1)
            {
                rqstSt_slvindex = 0;
            }
            else
            {
                rqstSt_slvindex++;
            }
        } while (rqstSt_slvindex != 0);

        // Request Ext-status x
        taskRqstSlaveTmr.Setms(prod.SlaveRqstInterval() - MS_SHIFT);
        RqstExtStatus(rqstExtSt_slvindex);
        vSlaves.at(rqstExtSt_slvindex)->rxExtSt = 0;
        PT_YIELD_UNTIL(taskRqstSlaveTmr.IsExpired());
        if (++rqstExtSt_slvindex == SlaveCnt())
        {
            rqstExtSt_slvindex = 0;
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
            return allSlavesCurrent;
        }
    }
    allSlavesCurrent = 0;
    return allSlavesCurrent;
}

bool Group::AllSlavesGotStatus()
{
    for (auto &s : vSlaves)
    {
        if (s->GetRxStatus() == 0)
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
    //TaskRqstSlaveReset();
}

bool Group::AllSlavesGotExtSt()
{
    for (auto &s : vSlaves)
    {
        if (s->GetRxExtSt() == 0)
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
    //TaskRqstSlaveReset();
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
    Ldebug("Group[%d] - %sable plan[%d]", groupId, endis == 0 ? "Dis" : "En", id);
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
    return activeMsg.GetBit(p);
}

bool Group::IsFrmActive(uint8_t p)
{
    return activeFrm.GetBit(p);
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
        if (id >= 3 && id <= 6)
        {
            auto &cfg = user.ExtSwCfgX(id - 3);
            auto time = cfg.dispTime * 100;
            if (time != 0)
            {
                if (dsCurrent->dispType == DISP_TYPE::EXT && dsCurrent->fmpid[0] == id)
                { // same, reload timer
                    extDispTmr.Setms(time);
                    Pdebug("Group[%d] - EXT timer reload: %dms", groupId, time);
                }
                else if (dsCurrent->dispType == DISP_TYPE::N_A ||
                         dsCurrent->dispType == DISP_TYPE::BLK ||
                         (onDispMsgId == 0 && onDispFrmId == 0) ||
                         cfg.flashingOv != 0 ||  // flash override OFF
                         (cfg.flashingOv == 0 && // flash override ON && current msg/frm NOT flashing
                          ((onDispMsgId != 0 && !db.GetUciMsg().IsMsgFlashing(onDispMsgId)) ||
                           onDispMsgId == 0 && onDispFrmId != 0 && !db.GetUciFrm().IsFrmFlashing(onDispFrmId))))
                {
                    extDispTmr.Setms(time);
                    dsExt->dispType = type;
                    dsExt->fmpid[0] = id;
                    Pdebug("Group[%d] - EXT timer start: %dms", groupId, time);
                }
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
    if (dsNext->dispType == DISP_TYPE::N_A)
    {
        dsNext->Clone(dsBak);
        dsBak->N_A();
    }
}

APP::ERROR Group::DispFrm(uint8_t id, bool chk)
{
    if (chk)
    {
        if (mainPwr == PWR_STATE::OFF || cmdPwr == PWR_STATE::OFF)
        {
            return APP::ERROR::PowerIsOff;
        }
        if (FacilitySwitch::FS_STATE::AUTO != fcltSw.Get())
        {
            return APP::ERROR::FacilitySwitchOverride;
        }
    }
    for (auto &sign : vSigns)
    {
        auto &signCfg = prod.GetSignCfg(sign->SignId());
        if (signCfg.rejectFrms.GetBit(id))
        {
            return APP::ERROR::SyntaxError;
        }
    }
    uint8_t buf[3];
    buf[0] = static_cast<uint8_t>(MI::CODE::SignDisplayFrame);
    buf[1] = groupId;
    buf[2] = id;
    if (chk)
    {
        db.GetUciProcess().SetDisp(groupId, buf, 3);
    }
    DispNext(DISP_TYPE::FRM, id);
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::DispMsg(uint8_t id, bool chk)
{
    if (chk)
    {
        if (mainPwr == PWR_STATE::OFF || cmdPwr == PWR_STATE::OFF)
        {
            return APP::ERROR::PowerIsOff;
        }
        if (FacilitySwitch::FS_STATE::AUTO != fcltSw.Get())
        {
            return APP::ERROR::FacilitySwitchOverride;
        }
    }
    uint8_t buf[3];
    buf[0] = static_cast<uint8_t>(MI::CODE::SignDisplayMessage);
    buf[1] = groupId;
    buf[2] = id;
    if (chk)
    {
        db.GetUciProcess().SetDisp(groupId, buf, 3);
    }
    DispNext(DISP_TYPE::MSG, id);
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::DispAtmFrm(uint8_t *cmd, bool chk)
{
    if (chk && FacilitySwitch::FS_STATE::AUTO != fcltSw.Get())
    {
        return APP::ERROR::FacilitySwitchOverride;
    }
    auto atm = DispAtomicFrm(cmd);
    if (chk && atm == APP::ERROR::AppNoError)
    {
        db.GetUciProcess().SetDisp(groupId, cmd, 3 + SignCnt() * 2);
    }
    return atm;
}

APP::ERROR Group::SetDimming(uint8_t dimming)
{
    db.GetUciProcess().SetDimming(groupId, dimming);
    targetDimmingLvl = dimming;
    for (auto &sign : vSigns)
    {
        sign->DimmingSet(dimming);
    }
    Ldebug("Group[%d] - SetDimming : %d", groupId, dimming);
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::SetPower(uint8_t v)
{
    Ldebug("Group[%d] - Power : %s", groupId, v == 0 ? "OFF" : "ON");
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
            s->DeviceOnOff(deviceEnDisSet);
        }
        Ldebug("Group[%d] - %sable device", groupId, deviceEnDisCur == 0 ? "Dis" : "En");
    }
}

bool Group::IsDsNextEmergency()
{
    bool r = false;
    if (dsExt->dispType == DISP_TYPE::EXT)
    { // external input
        uint8_t mid = dsExt->fmpid[0];
        if (mid >= 3 && mid <= 6)
        {
            if (user.ExtSwCfgX(mid - 3).emergency == 0)
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
    for (int i = 0; i < SlaveCnt(); i++)
    {
        if (vSlaves[i]->DecodeStRpl(data, len) == 0)
        {
            vSlaves[i]->sign->RefreshSlaveStatusAtSt();
            return;
        }
    }
}

void Group::SlaveExtStatusRpl(uint8_t *data, int len)
{
    if (len < 22)
        return;
    for (int i = 0; i < SlaveCnt(); i++)
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
    LockBus(prod.SlaveRqstStTo());
    Slave *s = vSlaves[slvindex];
    s->rxStatus = 0;
    txBuf[0] = s->SlaveId();
    txBuf[1] = SLVCMD::RQST_STATUS;
    txLen = 2;
    Tx();
    int ms = prod.SlaveRqstStTo();
    LockBus(ms);
    return ms;
}

int Group::RqstExtStatus(uint8_t slvindex)
{
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
    Tx();
    int ms = prod.SlaveRqstExtTo();
    LockBus(ms);
    return ms;
}

int Group::SlaveSync()
{
    txBuf[0] = 0xFF;
    txBuf[1] = SLVCMD::SYNC;
    txLen = 2;
    Tx();
    int ms = prod.SlaveDispDly();
    LockBus(ms);
    return ms;
}

int Group::SlaveSetFrame(uint8_t slvId, uint8_t slvFrmId, uint8_t uciFrmId)
{
    if (slvFrmId == 0 || uciFrmId == 0)
    {
        throw std::invalid_argument(FmtException("ERROR: SlaveSetFrame(slvId=%d, slvFrmId=%d, uciFrmId=%d)",
                                                 slvId, slvFrmId, uciFrmId));
    }
    int ms = 10;
    if (deviceEnDisCur)
    {
        IMakeFrameForSlave(uciFrmId);
        txBuf[0] = slvId;
        txBuf[2] = slvFrmId;
        char *asc = new char[(txLen - 1) * 2];
        Cnvt::ParseToAsc(txBuf + 1, asc, txLen - 1);
        uint16_t crc = Crc::Crc16_8005((uint8_t *)asc, (txLen - 1) * 2);
        delete[] asc;
        Cnvt::PutU16(crc, txBuf + txLen);
        txLen += 2;
        for (auto &s : vSlaves)
        {
            if (slvId == 0xFF || slvId == s->SlaveId())
            {
                s->expectNextFrmId = slvFrmId;
                s->frmCrc[slvFrmId] = crc;
            }
        }
        ms = Tx() + prod.SlaveSetStFrmDly();
    }
    LockBus(ms);
    return ms;
}

int Group::SlaveSDFrame(uint8_t slvId, uint8_t slvFrmId)
{
    if (deviceEnDisCur == 0)
    {
        slvFrmId = 0;
    }
    for (auto &s : vSlaves)
    {
        if (slvId == s->SlaveId() || slvId == 0xFF)
        {
            s->expectCurrentFrmId = slvFrmId;
            s->expectNextFrmId = slvFrmId;
        }
    }
    txBuf[0] = slvId;
    //txBuf[1] = cmd;
    txBuf[2] = slvFrmId;
    txLen = 3;
    Tx();
    int ms = (slvFrmId == 0) ? prod.SlaveRqstInterval() : prod.SlaveDispDly();
    LockBus(ms);
    return ms;
}

// this function is actually for transistion time ONLY
int Group::SlaveDisplayFrame(uint8_t slvId, uint8_t slvFrmId)
{
    //PrintDbg(DBG_LEVEL::DBG_PRT, "Slave[%d]DisplayFrame:%d", slvId, slvFrmId);
    txBuf[1] = SLVCMD::DISPLAY_FRM;
    return SlaveSDFrame(slvId, slvFrmId);
}

int Group::SlaveSetStoredFrame(uint8_t slvId, uint8_t slvFrmId)
{
    //PrintDbg(DBG_LEVEL::DBG_PRT, "Slave[%d]SetStoredFrame:%d", slvId, slvFrmId);
    txBuf[1] = SLVCMD::SET_STD_FRM;
    return SlaveSDFrame(slvId, slvFrmId);
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

APP::ERROR Group::SystemReset(uint8_t v)
{
    Ldebug("Group[%d] - SystemReset : Level=%d", groupId, v);
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
    ReadyToLoad(1);
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
        proc.SaveSignErr(s->SignId(), s->SignErr().GetV());
    }
}

void Group::MakeFrameForSlave(Frame *frm)
{
    uint8_t *p = txBuf + 1;
    *p++ = SET_GFX_FRM; // Gfx frame
    p++;                // skip slave frame id
    *p++ = prod.PixelRows();
    p = Cnvt::PutU16(prod.PixelColumns(), p);
    if (msgOverlay == 0 || msgOverlay == 1)
    {
        *p++ = (frm->colour >= (uint8_t)FRMCOLOUR::MonoFinished) ? frm->colour : prod.GetMappedColour(frm->colour);
    }
    else if (msgOverlay == 4)
    {
#ifdef HALF_BYTE
        *p++ = (uint8_t)FRMCOLOUR::HalfByte;
#else
        *p++ = (uint8_t)FRMCOLOUR::MultipleColours;
#endif
    }
    *p++ = frm->conspicuity;
    int frmlen = TransFrmWithOrBuf(frm, p + 2);
    p = Cnvt::PutU16(frmlen, p);
    txLen = p + frmlen - txBuf;
}

int Group::TransFrmWithOrBuf(Frame *frm, uint8_t *dst)
{
    FrmTxt *txtfrm = nullptr;
    if (frm->micode == static_cast<uint8_t>(MI::CODE::SignSetTextFrame))
    { // text frame trans to bitmap
        txtfrm = static_cast<FrmTxt *>(frm);
        if (txtfrm == nullptr)
        {
            throw std::runtime_error(
                FmtException("ERROR: TransFrmWithOrBuf(frmId=%d): dynamic_cast<FrmTxt *> failed", frm->frmId));
        }
    }
    if (msgOverlay == 0)
    { // no overlay
        if (txtfrm != nullptr)
        { // text frame trans to bitmap
            return txtfrm->ToBit(msgOverlay, dst);
        }
        else
        {
            memcpy(dst, frm->stFrm.rawData.data() + frm->frmOffset, frm->frmBytes);
            return frm->frmBytes;
        }
    }
    // overlay
    int frmlen = (msgOverlay == 1) ? prod.Gfx1FrmLen() : ((msgOverlay == 4) ? prod.Gfx4FrmLen() : prod.Gfx24FrmLen());
    uint8_t *buf = nullptr;
    uint8_t *orsrc;
    if (txtfrm != nullptr)
    { // text frame trans to bitmap
        buf = new uint8_t[frmlen];
        txtfrm->ToBit(msgOverlay, buf);
        orsrc = buf;
    }
    else
    { // gfx/hrg
        if (msgOverlay == 1 && frm->colour < static_cast<uint8_t>(FRMCOLOUR::MonoFinished))
        { // overlay but no need to trans or copy
            orsrc = frm->stFrm.rawData.data() + frm->frmOffset;
        }
        else if (msgOverlay == 4 && frm->colour <= static_cast<uint8_t>(FRMCOLOUR::MultipleColours))
        { //frame is mono but should transfer from 1-bit to 4-bit
            buf = new uint8_t[frmlen];
            frm->ToBit(msgOverlay, buf);
            orsrc = buf;
        }
        else
        {
            // TODO 24-bit
            throw std::runtime_error(FmtException("ERROR: TransFrmWithOrBuf(frmId=%d): 24-bit unsupported", frm->frmId));
        }
    }
    // frame is ready in orsrc, then OR with orbuf if overlay
    if (msgOverlay == 1)
    { // with overlay
        SetWithOrBuf1(dst, orsrc, frmlen);
    }
    else if (msgOverlay == 4)
    {
        SetWithOrBuf4(dst, orsrc, frmlen);
    }
    else
    {
        // TODO 24-bit
        throw std::runtime_error(FmtException("ERROR: TransFrmWithOrBuf(frmId=%d): 24-bit unsupported", frm->frmId));
    }
    if (buf != nullptr)
    {
        delete[] buf;
    }
    return frmlen;
}

void Group::SetWithOrBuf1(uint8_t *dst, uint8_t *src, int len)
{ // 1-bit
    uint8_t *p = orBuf;
    while (len--)
    {
        *dst++ = *src++ | *p++;
    };
}

void Group::SetWithOrBuf4(uint8_t *dst, uint8_t *src, int len)
{ // 4-bit
    uint8_t *p = orBuf;
    while (len--)
    {
        uint8_t d = *src++;
        uint8_t o = *p++;
        if ((d & 0xF0) == 0)
        {                    // high 4-bit is OFF
            d |= (o & 0xF0); // use orbuf value for this pixel
        }
        if ((d & 0x0F) == 0)
        {                    // low 4-bit is OFF
            d |= (o & 0x0F); // use orbuf value for this pixel
        }
        *dst++ = d;
    };
}

void Group::PrintOrBuf()
{
    puts("OrBuf:\n");
    auto Y = prod.PixelColumns() / 8;
    for (int i = 0; i < orLen; i++)
    {
        uint8_t x = *(orBuf + i);
        uint8_t b = 1;
        for (int j = 0; j < 8; j++)
        {
            putchar((x & b) ? '*' : '-');
            b <<= 1;
        }
        if ((i % Y) == (Y - 1))
        {
            putchar('\n');
        }
    }
}
