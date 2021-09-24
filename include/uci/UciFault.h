#pragma once


#include <uci/UciLog.h>
#include <uci/FaultLog.h>

#define FAULT_LOG_ENTRIES 500

#define FAULT_LOG_SIZE 10

/*
Filename: "./config/UciFault"
Format: UCI
--- Start ---
config UciFault flt
    # flt_xxx : xxx is fault ID, 0-499
    // data format: 11 bytes
    // uint8_t id
    // uint16_t entry number, 0=>65535=>0
    // time_t log time, <0 means empty
    // uint8_t error code
    // uint8_t fault clear/onset
    // uint16_t crc
    option flt_0  "............."
    # If flt CRC is not matched, discard flt

    // option LastLog is the last log entry number
    option LastLog "244"

--- End ---
*/

class UciFault : public UciLog
{
public:
    UciFault();
    ~UciFault();

    virtual void LoadConfig() override;

	virtual void Dump() override ;

    /// \brief  for TSI-SP-003 Fault log Reply, 20 entries, entry number is 0-255 
    int GetFaultLog003(uint8_t *dst);

    /// \brief  for TSI-SP-008 Fault log Reply, 500 entries, entry number is 0-499 
    int GetLog(uint8_t *dst);

    /// \brief  Push a fault in faultLog and save to UciFault
    void Push(uint8_t id, uint8_t errorCode, uint8_t onset);

private:
    FaultLog *faultLog;
};

