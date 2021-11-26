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
        MyThrow("Error: Slave::numberOfTiles is 0");
    }
    if (numberOfColours == 0)
    {
        MyThrow("Error: Slave::numberOfColours is 0");
    }
    numberOfFaultyLed = new uint8_t[numberOfTiles * numberOfColours];
    Reset();
}

Slave::~Slave()
{
    delete[] numberOfFaultyLed;
}

void Slave::Reset()
{
    rxStatus = 0;
    rxExtSt = 0;
    for (int i = 0; i < 7; i++)
    {
        frmCrc[i] = 0;
    }
    expectCurrentFrmId = 0; // display frame command
    expectNextFrmId = 0;    // set frame command
    memset(numberOfFaultyLed, 0, numberOfTiles * numberOfColours);
}

int Slave::DecodeStRpl(uint8_t *buf, int len)
{
    if (*buf != slaveId)
    {
        return -1;
    }
    buf++;
    if (*buf != 0x06)
    {
        return -2;
    }
    if (len != 14)
    {
        return -3;
    }
    buf++;
    rxStatus = 1;
    panelFault = *buf++;
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
    return 0;
}

int Slave::DecodeExtStRpl(uint8_t *buf, int len)
{
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
    memcpy(numberOfFaultyLed, buf, numberOfTiles * numberOfColours);
    return 0;
}

int Slave::CheckCurrent()
{
    if (rxStatus == 0)
    {
        return -1;
    }
    if (expectCurrentFrmId == currentFrmId && frmCrc[expectCurrentFrmId] == currentFrmCrc)
    {
        return 0;
    }
    else
    {
        PrintDbg(DBG_LOG, "Sign[%d].Slave[%d] NOT matched: current(%d:%04X) expect(%d:%04X)\n",
                sign->SignId(), slaveId, currentFrmId, currentFrmCrc, expectCurrentFrmId, frmCrc[expectCurrentFrmId]);
        if (expectCurrentFrmId != currentFrmId)
        {
            if (currentFrmIdBak == currentFrmId && currentFrmId != 0 && frmCrc[currentFrmId] == currentFrmCrc)
            {
                return 1; // lastFrm matched. Re-send Setstored/Display frame
            }
        }
        return 2; // fatal error, frames in slave lost. Should re-SetFrame
    }
}

int Slave::CheckNext()
{
    if (rxStatus == 0)
    {
        return -1;
    }
    if (expectNextFrmId == nextFrmId && frmCrc[expectNextFrmId] == nextFrmCrc)
    {
        return 0;
    }
    else
    {
        PrintDbg(DBG_LOG, "Sign[%d].Slave[%d] NOT matched: next(%d:%04X) expect(%d:%04X)\n",
                sign->SignId(), slaveId, nextFrmId, nextFrmCrc, expectNextFrmId, frmCrc[expectNextFrmId]);
        return 1; // NOT matched
    }
}

void Slave::ReportOffline(bool v)
{
    char buf[64];
    snprintf(buf, 63, "Sign[%d].Slave[%d] %s", sign->SignId(), slaveId, v ? "OFF-line" : "On-line");
    PrintDbg(DBG_LOG, "%s\n", buf);
    DbHelper::Instance().GetUciAlarm().Push(sign->SignId(), buf);
    if(v)
    {
        sign->SignErr(DEV::ERROR::InternalCommunicationsFailure, v);
    }
    isOffline = v;
}
