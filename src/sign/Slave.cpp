#include <cstring>
#include <sign/Slave.h>
#include <sign/Sign.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>

using namespace Utils;

uint8_t Slave::numberOfTiles = 0;
uint8_t Slave::numberOfColours = 0;

Slave::Slave(uint8_t id)
    : slaveId(id)
{
    if (numberOfTiles == 0)
    {
        throw std::invalid_argument("Error: Slave::numberOfTiles is 0");
    }
    if (numberOfColours == 0)
    {
        throw std::invalid_argument("Error: Slave::numberOfColours is 0");
    }
    Reset();
    isSimSlave = DbHelper::Instance().GetUciHardware().IsSimSlave(id);
    if (isSimSlave)
    {
        printf("SLAVE[%d] EMULATOR\n", id);
        mledchainFault = 0;
        overTemp = 0;
        selfTest = 0;
        singleLedFault = 0;
        lanternFan = 0;
        lightSensorFault = 0;
        currentFrmId = 0;
        currentFrmCrc = 0x55AA;
        nextFrmId = 0;
        nextFrmCrc = 0xAA55;
        controlByte = 0;
        for (int i = 0; i < 4; i++)
        {
            dimming[i] = 0; // no use, reported dimming level in SESR is controller's setting
        }
        voltage = 12000 + slaveId * 100;
        hours = 0x99;
        temperature = 300 + slaveId * 10;
        humidity = 50 + slaveId;
        lux = 12000 + slaveId * 100;
    }
}

Slave::~Slave()
{
}

void Slave::Reset()
{
    rxStatus = 0;
    rxExtSt = 0;
    for (int i = 0; i < 7; i++)
    {
        frmCurrentCrc[i] = 0;
        frmNextCrc[i] = 0;
    }
    expectCurrentFrmId = 0; // display frame command
    expectNextFrmId = 0;    // set frame command
    numberOfFaultyLed.assign(numberOfTiles * numberOfColours, 0);
    faultyLedPerColour.assign(numberOfColours, 0);
}

int Slave::DecodeStRpl(uint8_t *buf, int len)
{
    if (isSimSlave)
    {
        return 0;
    }
    if (*buf++ != slaveId)
    {
        return -1;
    }
    if (*buf++ != 0x06)
    {
        return -2;
    }
    if (len != 14)
    {
        return -3;
    }
    rxStatus = 1;
    rqstNoRplTmr.Clear();
    mledchainFault = *buf++;
    auto slavepixel = DbHelper::Instance().GetUciHardware().PixelsPerSlave();
    for (int j = 0; j < faultyLedPerColour.size(); j++)
    {
        if (faultyLedPerColour[j] == slavepixel)
        {
            mledchainFault |= 1 << j;
        }
    }

    overTemp = *buf++;
    selfTest = *buf++;
    singleLedFault = *buf++;
    lanternFan = *buf++;
    lightSensorFault = *buf++;
    currentFrmId = *buf++;
    currentFrmCrc = Cnvt::GetU16(buf);
    buf += 2;
    nextFrmId = *buf++;
    nextFrmCrc = Cnvt::GetU16(buf);
    stCurrentCrc = CheckCurrent();
    stNextCrc = CheckNext();
    return 0;
}

int Slave::DecodeExtStRpl(uint8_t *buf, int len)
{
    if (isSimSlave)
    {
        return 0;
    }
    if (buf[0] != slaveId)
    {
        return -1;
    }
    if (buf[1] != 0x08)
    {
        return -2;
    }
    if (len != 22 + numberOfTiles * numberOfColours)
    {
        return -3;
    }
    if (numberOfTiles != buf[20])
    {
        return -4;
    }
    if (numberOfColours != buf[21])
    {
        return -5;
    }
    buf += 2;
    rxExtSt = 1;
    controlByte = *buf++;
    for (int i = 0; i < 4; i++)
    {
        dimming[i] = Cnvt::GetU16(buf);
        buf += 2;
    }
    voltage = Cnvt::GetU16(buf);
    buf += 2;
    hours = Cnvt::GetU16(buf);
    buf += 2;
    temperature = Cnvt::GetU16(buf);
    buf += 2;
    humidity = *buf++;
    lux = Cnvt::GetU16(buf);
    buf += 4;
    memcpy(numberOfFaultyLed.data(), buf, numberOfTiles * numberOfColours);
    faultyLedPerColour.assign(faultyLedPerColour.size(), 0);
    for (int i = 0; i < numberOfTiles; i++)
    {
        for (int j = 0; j < faultyLedPerColour.size(); j++)
        {
            faultyLedPerColour[j] += *buf++;
        }
    }
    // check hours
    if (hours != hoursBak)
    {
        if (hours == 0 && hoursBak != 65535)
        {
            char buf[64];
            sprintf(buf,"Sign[%d].Slave[%d] reset: hours=0", sign->SignId(), slaveId);
            DbHelper::Instance().GetUciAlarm().Push(sign->SignId(), buf);
            DebugLog(buf);
        }
        hoursBak = hours;
    }
    return 0;
}

uint8_t Slave::GetRxStatus()
{
    if (isSimSlave)
    {
        sign->RefreshSlaveStatusAtSt();
        sign->RefreshSlaveStatusAtExtSt();
        return 1;
    }
    return rxStatus;
}

uint8_t Slave::GetRxExtSt()
{
    if (isSimSlave)
    {
        return 1;
    }
    return rxExtSt;
}

int Slave::CheckCurrent()
{
    if (expectCurrentFrmId == currentFrmId && frmCurrentCrc[expectCurrentFrmId] == currentFrmCrc)
    {
        return 0;
    }
    else
    {
        DebugPrt("Sign[%d].Slave[%d] NOT matched: current(%d:%04X) expect(%d:%04X)",
                 sign->SignId(), slaveId,
                 currentFrmId, currentFrmCrc,
                 expectCurrentFrmId, frmCurrentCrc[expectCurrentFrmId]);
        if (expectCurrentFrmId != currentFrmId)
        {
            if (currentFrmIdBak == currentFrmId && frmCurrentCrc[currentFrmIdBak] == currentFrmCrc)
            { // may missed the command
                if (currentFrmId != 0)
                {
                    return 1; // lastFrm matched. Re-send Setstored/Display frame
                }
                else
                { // if currentFrmId==0, slave may reset
                    return 2;
                }
            }
        }
        return 3; // fatal error, frames in slave lost. Should re-SetFrame
    }
}

int Slave::GetStCurrent()
{
    return isSimSlave ? 0 : (rxStatus == 0 ? -1 : stCurrentCrc);
}

int Slave::CheckNext()
{
    if (expectNextFrmId == nextFrmId && frmNextCrc[expectNextFrmId] == nextFrmCrc)
    {
        return 0;
    }
    else
    {
        DebugPrt("Sign[%d].Slave[%d] NOT matched: next(%d:%04X) expect(%d:%04X)",
                 sign->SignId(), slaveId, nextFrmId, nextFrmCrc, expectNextFrmId, frmNextCrc[expectNextFrmId]);
        return 1; // NOT matched
    }
}

int Slave::GetStNext()
{
    return isSimSlave ? 0 : (rxStatus == 0 ? -1 : stNextCrc);
}

void Slave::ReportOffline(bool v)
{
    int iv = v ? 1 : 0;
    if (isOffline != iv)
    {
        isOffline = iv;
        char buf[STRLOG_SIZE];
        snprintf(buf, STRLOG_SIZE - 1, "Sign[%d].Slave[%d] %s-line", sign->SignId(), slaveId, v ? "OFF" : "On");
        DebugLog(buf);
        DbHelper::Instance().GetUciAlarm().Push(sign->SignId(), buf);
        sign->RefreshDevErr(DEV::ERROR::InternalCommunicationsFailure);
    }
}

void Slave::ReportCNCrcErr(bool v)
{
    int iv = v ? 1 : 0;
    if (isCNCrcErr != iv)
    {
        isCNCrcErr = iv;
        char buf[STRLOG_SIZE];
        snprintf(buf, STRLOG_SIZE - 1, "Sign[%d].Slave[%d] C|N CRC %s (0x21=MPCerror)", sign->SignId(), slaveId, v ? "ERR" : "OK");
        DebugLog(buf);
        DbHelper::Instance().GetUciAlarm().Push(sign->SignId(), buf);
        sign->RefreshDevErr(DEV::ERROR::MainProcessorCommunicationsError);
    }
}
