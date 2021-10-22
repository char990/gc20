#include <cstring>
#include <sign/Slave.h>
#include <module/MyDbg.h>
#include <module/Utils.h>

using namespace Utils;

uint8_t Slave::numberOfTiles=0;
uint8_t Slave::numberOfColours=0;

Slave::Slave(uint8_t id)
:slaveId(id)
{
    if(numberOfTiles==0)
    {
        MyThrow("Error: Slave::numberOfTiles is 0");
    }
    if(numberOfColours==0)
    {
        MyThrow("Error: Slave::numberOfColours is 0");
    }
    numberOfFaultyLed = new uint8_t[numberOfTiles*numberOfColours];
    Reset();
}

Slave::~Slave()
{
    delete [] numberOfFaultyLed;
}

void Slave::Reset()
{
    rxStatus=0;
    rxExtSt=0;
    for(int i=0;i<7;i++)
    {
        frmCrc[i]=0;
    }
    expectCurrentFrmId=0;       // display frame command
    expectNextFrmId=0;       // set frame command
}

int Slave::DecodeStRpl(uint8_t * buf, int len)
{
    if(*buf!=slaveId)
    {
        return -1;
    }
    buf++;
    if(*buf!=0x06)
    {
        return -2;
    }
    if(len!=14)
    {
        return -3;
    }
    buf++;
    rxStatus=1;
    panelFault = *buf++;
    overTemp = *buf++;
    selfTest = *buf++;
    singleLedFault = *buf++;
    lanternFan = *buf++;
    lightSensorFault = *buf++;
    currentFrmId = *buf++;
    currentFrmCrc = Cnvt::GetU16(buf); buf+=2;
    nextFrmId = *buf++;
    nextFrmCrc = Cnvt::GetU16(buf);
    return 0;
}

int Slave::DecodeExtStRpl(uint8_t * buf, int len)
{
    if(buf[0]!=slaveId)
    {
        return -1;
    }
    if(buf[1]!=0x08)
    {
        return -2;
    }
    if(len!=22+numberOfTiles*numberOfColours)
    {
        return -3;
    }
    if(numberOfTiles!=buf[20])
    {
        return -4;
    }
    if(numberOfColours!=buf[21])
    {
        return -5;
    }
    buf+=2;
    rxExtSt=1;
    controlByte=*buf++;
    for(int i=0;i<4;i++)
    {
        dimming[i] = Cnvt::GetU16(buf);
        buf+=2;
    }
    voltage = Cnvt::GetU16(buf); buf+=2;
    hours = Cnvt::GetU16(buf); buf+=2;
    temperature = Cnvt::GetU16(buf); buf+=2;
    humidity=*buf++;
    lux=Cnvt::GetU16(buf);  buf+=4;
    memcpy(numberOfFaultyLed, buf, numberOfTiles*numberOfColours);
    return 0;
}

Utils::STATE3 Slave::IsCurrentMatched()
{
    if(rxStatus==0)
    {
        return Utils::STATE3::S_NA;
    }
    if(expectCurrentFrmId == currentFrmId && frmCrc[expectCurrentFrmId] == currentFrmCrc)
    {
        return Utils::STATE3::S_1;
    }
    PrintDbg("NOT matched: current(%d:%04X) expect(%d:%04X)\n",
        currentFrmId, currentFrmCrc, expectCurrentFrmId, frmCrc[expectCurrentFrmId]);
    return Utils::STATE3::S_0;
}

Utils::STATE3 Slave::IsNextMatched()
{
    if(rxStatus==0)
    {
        return Utils::STATE3::S_NA;
    }
    if(expectNextFrmId == nextFrmId && frmCrc[expectNextFrmId] == nextFrmCrc)
    {
        return Utils::STATE3::S_1;
    }
    PrintDbg("NOT matched: next(%d:%04X) expect(%d:%04X)\n",
        nextFrmId, nextFrmCrc, expectNextFrmId, frmCrc[expectNextFrmId]);
    return Utils::STATE3::S_0;
}

