#pragma once

#include <uci/UciLog.h>
#include <uci/FaultLog.h>
#include<tsisp003/TsiSp003Const.h>


#define FAULT_LOG_ENTRIES 500

/*
Filename: "./config/UciFault"
Format: UCI
--- Start ---
config UciFault flt
    # flt_xxx : xxx is fault ID, 0-499
    option flt_0  "[d/m/yyyy h:mm:ss]ID=%d, EntryNo=%d, Error=%d, Onset=%d, Crc=%d"
    # If flt CRC is not matched, discard flt
--- End ---
*/

class UciFault : public UciLog
{
public:
    UciFault();
    ~UciFault();

    virtual void LoadConfig() override;

    virtual void Dump() override;

    /// \brief  for TSI-SP-003 Fault log Reply, 20 entries, entry number is 0-255
    int GetFaultLog20(uint8_t *dst);

    /// \brief  for TSI-SP-008 Fault log Reply, 500 entries, entry number is 0-499
    int GetLog(uint8_t *dst);

    /// \brief  Push a fault in faultLog and save to UciFault
    void Push(uint8_t id, DEV::ERROR errorCode, uint8_t onset);
    
    /// \brief  This is a special Push for logging reset onset time
    void Push(uint8_t id, DEV::ERROR errorCode, uint8_t onset, time_t t);

    void Reset();

private:
    FaultLog *faultLog;
    const char * _Log = "log_";
    const char * _Fmt = "]ID=%d, EntryNo=%d, Error=%d, Onset=%d, Crc=%d";
};
