#include <cstring>
#include <sign/Slave.h>
#include <module/MyDbg.h>

uint8_t Slave::numberOfTiles=0;
uint8_t Slave::numberOfColours=0;

Slave::Slave()
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
}

Slave::~Slave()
{
    delete [] numberOfFaultyLed;
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
    panelFault = buf[2];
    overTemp = buf[3];
    selfTest = buf[4];
    singleLedFault = buf[5];
    lanternFan = buf[6];
    lightSensorFault = buf[7];
    currentFrmId = buf[8];
    currentFrmCrc = GetU16(buf+9);
    nextFrmId = buf[11];
    nextFrmCrc = GetU16(buf+12);
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
    controlByte=buf[2];
    for(int i=0;i<4;i++)
    {
        dimming[i] = GetU16(buf+3+i*2);
    }
    voltage = GetU16(buf+11);
    hours = GetU16(buf+13);
    temperature = GetU16(buf+15);
    humidity=buf[17];
    lux=GetU16(buf+18);
    memcpy(numberOfFaultyLed, buf+22, numberOfTiles*numberOfColours);
    return 0;
}

uint16_t Slave::GetU16(uint8_t *p)
{
    return (*p)*0x100+(*(p+1));
}
