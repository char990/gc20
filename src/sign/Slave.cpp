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
    if(buf[0]!=slaveId)
    {
        return -1;
    }
    if(buf[1]!=0x06)
    {
        return -2;
    }
    if(len!=14)
    {
        return -3;
    }
    rxStatus=1;
    panelFault = buf[2];
    overTemp = buf[3];
    selfTest = buf[4];
    singleLedFault = buf[5];
    lanternFan = buf[6];
    lightSensorFault = buf[7];
    currentFrmId = buf[8];
    currentFrmCrc = Cnvt::GetU16(buf+9);
    nextFrmId = buf[11];
    nextFrmCrc = Cnvt::GetU16(buf+12);
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
    rxExtSt=1;
    controlByte=buf[2];
    for(int i=0;i<4;i++)
    {
        dimming[i] = Cnvt::GetU16(buf+3+i*2);
    }
    voltage = Cnvt::GetU16(buf+11);
    hours = Cnvt::GetU16(buf+13);
    temperature = Cnvt::GetU16(buf+15);
    humidity=buf[17];
    lux=Cnvt::GetU16(buf+18);
    memcpy(numberOfFaultyLed, buf+22, numberOfTiles*numberOfColours);
    return 0;
}
