#pragma once


#include <cstdint>

class Slave
{
public:
    Slave(uint8_t id);
    ~Slave();

    void Reset();

    uint8_t SlaveId() {return slaveId; };

    // static for all slaves
    static uint8_t numberOfTiles;
    static uint8_t numberOfColours;

    // ------------------------- status reply
    uint8_t rxStatus{0};
    // uint8_t slaveId; check rxbuf[0]
    uint8_t panelFault;
    bool IsChainFault(int i) { return (panelFault & (1<<i))!=0; };
    bool IsMultiLedFault(int i) { return ((panelFault>>4) & (1<<i))!=0; };
    uint8_t overTemp;
    uint8_t selfTest;
    uint8_t singleLedFault;
    uint8_t lanternFan;
    bool IsLantern1Fault(int i) { return (lanternFan & (1<<0))!=0; };
    bool IsLantern2Fault(int i) { return (lanternFan & (1<<1))!=0; };
    bool IsFan1Fault(int i) { return (lanternFan & (1<<4))!=0; };
    bool IsFan2Fault(int i) { return (lanternFan & (1<<5))!=0; };
    
    uint8_t lightSensorFault;

    uint8_t currentFrmId;       // display frame command
    uint16_t currentFrmCrc;

    uint8_t nextFrmId;          // set frame command
    uint16_t nextFrmCrc;

    int DecodeStRpl(uint8_t * buf, int len);

    // -------------------------- ext-status reply
    uint8_t rxExtSt{0};
    uint8_t controlByte;
    uint16_t dimming[4];    // [0]:R [1]:G [2]:B/W [3]:A
    uint16_t voltage;       // mV
    uint16_t hours;
    uint16_t temperature;   // 0.1'C
    uint8_t humidity;
    uint16_t lux;
    uint8_t *numberOfFaultyLed;

    int DecodeExtStRpl(uint8_t * buf, int len);

    // --------------------------- settings

    enum FRM_ST {MATCH_NA, MATCH_OK, MATCH_NG};

    uint16_t frmCrc[7]; // 0-6, keep [0] as 0
    uint8_t expectCurrentFrmId{0};       // display frame command
    uint8_t expectNextFrmId{0};       // set frame command

    // ----------------------------- result
    FRM_ST currentState{FRM_ST::MATCH_NA};
    FRM_ST nextState{FRM_ST::MATCH_NA};
private:
    uint8_t slaveId;
};
