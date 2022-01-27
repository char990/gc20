#pragma once


#include <module/SerialPort.h>
#include <module/IGcEvent.h>

class Monitor: public IGcEvent
{
public:
    Monitor(uint8_t comX, int bps);

    void TX(uint8_t buf, int len);

    /// \brief  Called by Eepoll, sending handle
    virtual void EventsHandle(uint32_t events) override;

private:
    uint8_t comX;
    SerialPort *sp;
    int TxHandle();
    uint8_t buffer[1024*1024];
};
