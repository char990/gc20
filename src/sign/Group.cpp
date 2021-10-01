#include <cstring>
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

    maxTxSize = 1 + 9 + prod.MaxFrmLen() + 2; // slaveId(1) + MIcode-datalen(9) + bitmapdata(x) + appcrc(2)
    txBuf = new uint8_t[maxTxSize];
    txLen = 0;

    // TODO load process
    UciPln &pln = db.GetUciPln();
    UciProcess &proc = db.GetUciProcess();
    devSet = 1;
    devCur = 1;
    dsNext->dispType = DISP_STATUS::TYPE::FRM;
    dsNext->fmpid[0] = 1;
}

Group::~Group()
{
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
    }

    //TaskPln(&taskPlnLine);
    //TaskMsg(&taskMsgLine);
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
        return true;
    }
    if (newCurrent)
    {
        *_ptLine = 0;
        newCurrent = 0;
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
        printf("TaskPln, delay 5 sec\n");
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
    if (!IsBusFree() ||
        dsCurrent->dispType != DISP_STATUS::TYPE::MSG &&
        dsCurrent->dispType != DISP_STATUS::TYPE::FSW &&
        dsCurrent->dispType != DISP_STATUS::TYPE::EXT &&
        (dsCurrent->dispType == DISP_STATUS::TYPE::PLN &&
         plnEntryType != typeMSG))
    {
        return true;
    }
    if (newCurrent)
    {
        *_ptLine = 0;
        newCurrent = 0;
    }

    PT_BEGIN();
    while (true)
    {
        // step 1: load new msg
        //      search all frms in msg and set OR buffer
        //      disp first frm
        // step 2: wait first frm loaded(if frm time=0, set OR) and display
        // step 3: load next frm(if frm time=0, set OR)
        // step 4:
        // if(transtime)
        //load next frm
        if (1) //new msg
        {
            if (1) // msg is defined
            {
                // is this msg same Ext-input
                //
                // else
                // Load 1st frm
                // load ok?
                // display 1st frm
                // display ok?
                // if first frm is overlay, set/clear or-buf
                // if 2nd frm, load 2nd frm with or-buf
                // load ok?
            }
            else
            {
                // only update msg id
            }
        }
        else
        {
            // msg end? set endMsg
            // is this msg Ext-input
        }
        printf("TaskMsg, every 2 sec\n");
        taskMsgTmr.Setms(2000);
        PT_WAIT_UNTIL(taskMsgTmr.IsExpired());
    };
    PT_END();
}

bool Group::TaskFrm(int *_ptLine)
{
    if(!IsBusFree())return true;
    if (dsCurrent->dispType == DISP_STATUS::TYPE::FRM ||
        dsCurrent->dispType == DISP_STATUS::TYPE::ATF ||
        (dsCurrent->dispType == DISP_STATUS::TYPE::PLN && plnEntryType == typeFRM))
    {
        if (newCurrent)
        {
            TaskFrmReset();
            newCurrent = 0;
        }
        PT_BEGIN();
            // step1: set frame
            if (dsCurrent->dispType == DISP_STATUS::TYPE::ATF)
            {
                for (frmFor = 0; frmFor < vSigns.size(); frmFor++)
                {
                }
            }
            else
            {
                newFrmId = (dsCurrent->dispType == DISP_STATUS::TYPE::FRM) ? dsCurrent->fmpid[0] : plnEntryId;
                do
                {
                    SetFrame(0xFF, (newFrmId == 0) ? 0 : 1, newFrmId);
                    ClrAllSlavesRxStatus();
                    PT_WAIT_UNTIL(CheckAllSlavesNext() != STATE3::S_NA);
                } while (allSlavesNext == STATE3::S_0);
            }
            // step2: display frame
            do
            {
                SetStoredFrame(0xFF, (newFrmId == 0) ? 0 : 1);
                ClrAllSlavesRxStatus();
                do
                {
                    // step3: check current & next
                    taskFrmTmr.Setms(db.GetUciProd().SlaveRqstInterval() - 1);
                    PT_WAIT_UNTIL(CheckAllSlavesRxStatus()!=STATE3::S_NA && taskFrmTmr.IsExpired());
                    CheckAllSlavesCurrent();
                    CheckAllSlavesNext();
                } while (allSlavesCurrent == STATE3::S_1 && allSlavesNext == STATE3::S_1);
            } while (allSlavesNext == STATE3::S_1);
            PT_RESTART();
        PT_END();
    }
    return true;
}

bool Group::TaskRqstSlave(int *_ptLine)
{
    if(!IsBusFree())return true;
    PT_BEGIN();
    while (true)
    {
        rqstNoRplCnt = 0;
        do
        {
            RqstStatus(rqstStCnt);
            taskRqstSlaveTmr.Setms(db.GetUciProd().SlaveRqstInterval() - 1);
            PT_WAIT_UNTIL(taskRqstSlaveTmr.IsExpired());
            //PT_YIELD();
            //vSlaves[rqstStCnt]->rxStatus = 1;
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
            //PT_YIELD();
            //vSlaves[rqstExtStCnt]->rxExtSt = 1;
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
    for (auto s : vSlaves)
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
    for (auto s : vSlaves)
    {
        allSlavesCurrent = s->IsCurrentMatched();
        if (allSlavesCurrent != Utils::STATE3::S_1)
        {
            break;
        }
    }
    return allSlavesCurrent;
}

bool Group::CheckAllSlavesRxStatus()
{
    for (auto s : vSlaves)
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
    for (auto s : vSlaves)
    {
        s->rxStatus = 0;
    }
    TaskRqstSlaveReset();
}

bool Group::CheckAllSlavesRxExtSt()
{
    for (auto s : vSlaves)
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
    for (auto s : vSlaves)
    {
        s->rxExtSt = 0;
    }
    TaskRqstSlaveReset();
}

bool Group::IsSignInGroup(uint8_t id)
{
    for (auto s : vSigns)
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
    for (auto s : vSigns)
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
    for (auto s : vSlaves)
    {
        if (s->SlaveId() == id)
        {
            return s;
        }
    }
    return nullptr;
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
    for (auto sign : vSigns)
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
    for (auto sign : vSigns)
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
        for (auto sign : vSigns)
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
    for (auto sign : vSigns)
    {
        //sign->SetPower(power);
    }
}

// TODO
APP::ERROR Group::SetDevice(uint8_t endis)
{
    devSet = (endis == 0) ? DEV_DIS : DEV_EN;
    db.GetUciProcess().SetDevice(groupId, devSet);
    for (auto sign : vSigns)
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
        auto s = vSlaves[i];
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
        auto s = vSlaves[i];
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
    for(auto s : vSigns)
    {
        s->RefreshSlaveStatus();
    }
}

void Group::RefreshSignByExtSt()
{
    for(auto s : vSigns)
    {
        s->RefreshSlaveExtSt();
    }
}

int Group::RqstStatus(uint8_t slvindex)
{
    PrintDbg("RqstStatus\n");
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
    PrintDbg("RqstExtSt\n");
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
    *p++ = 0;                   // TODO control byte
    p = Cnvt::PutU16(65535, p); // TODO dimming
    p = Cnvt::PutU16(65535, p); // TODO dimming
    p = Cnvt::PutU16(65535, p); // TODO dimming
    p = Cnvt::PutU16(65535, p); // TODO dimming
    txLen = 11;
    return Tx();
}

void Group::TranslateFrame(uint8_t frmId)
{
    if (0) // or buf
    {
    }
    else
    {
        //just make a frame to show
        uint8_t *p = txBuf + 1;
        *p++ = SLVCMD::SET_TXT_FRM;
        p++;      // don not care frame id
        *p++ = 1; // font
        *p++ = 0; // colour
        *p++ = 0; // conspicuity
        *p++ = 2; // char spacing
        *p++ = 2; // line spacing
        *p++ = 2; // number of chars
        *p++ = '1';
        *p++ = 'A';
        txLen = p - txBuf;
    }
}

int Group::SetFrame(uint8_t slvindex, uint8_t slvFrmId, uint8_t uciFrmId)
{
    PrintDbg("SetFrame\n");
    txBuf[0] = (slvindex == 0xFF) ? 0xFF : vSlaves[slvindex]->SlaveId();
    TranslateFrame(uciFrmId);
    txBuf[2] = slvFrmId;
    char * asc = new char[(txLen-1)*2];
    Cnvt::ParseToAsc(txBuf+1, asc, txLen-1);
    uint16_t crc = Crc::Crc16_8005((uint8_t *)asc, (txLen-1)*2);
    delete [] asc;
    Cnvt::PutU16(crc, txBuf + txLen);
    txLen += 2;
    if (slvindex == 0xFF)
    {
        for (auto s : vSlaves)
        {
            s->expectNextFrmId = slvFrmId;
            s->frmCrc[slvFrmId] = crc;
            s->nextState = Slave::FRM_ST::MATCH_NA;
        }
    }
    else
    {
        auto s = vSlaves[slvindex];
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

int Group::DisplayFrame(uint8_t slvindex, uint8_t slvFrmId)
{
    PrintDbg("DisplayFrame\n");
    LockBus(db.GetUciProd().SlaveDispDly());
    if (slvindex == 0xFF)
    {
        txBuf[0] = 0xFF;
        for (auto s : vSlaves)
        {
            s->expectCurrentFrmId = slvFrmId;
            s->currentState = Slave::FRM_ST::MATCH_NA;
        }
    }
    else
    {
        auto s = vSlaves[slvindex];
        txBuf[0] = s->SlaveId();
        s->expectCurrentFrmId = slvFrmId;
        s->currentState = Slave::FRM_ST::MATCH_NA;
    }
    txBuf[1] = SLVCMD::DISPLAY_FRM;
    txBuf[2] = slvFrmId;
    txLen = 3;
    return Tx();
}

int Group::SetStoredFrame(uint8_t slvindex, uint8_t slvFrmId)
{
    PrintDbg("SetStoredFrame\n");
    LockBus(db.GetUciProd().SlaveDispDly());
    if (slvindex == 0xFF)
    {
        txBuf[0] = 0xFF;
        for (auto s : vSlaves)
        {
            s->expectCurrentFrmId = slvFrmId;
            s->currentState = Slave::FRM_ST::MATCH_NA;
        }
    }
    else
    {
        auto s = vSlaves[slvindex];
        txBuf[0] = s->SlaveId();
        s->expectCurrentFrmId = slvFrmId;
        s->currentState = Slave::FRM_ST::MATCH_NA;
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
