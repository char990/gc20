#pragma once


#include <string>
#include <module/IGcEvent.h>
#include <tsisp003/TsiSp003Const.h>
#include <layer/ILayer.h>

class IOperator : public IGcEvent , public ILowerLayer
{
public:
    virtual ~IOperator(){};
    bool IsTxRdy() { return txsize==0 ; };

    int TxBytes(uint8_t * data, int len);
    int TxHandle();
    void ClrTx();
protected:
    uint8_t txbuf[MAX_DATA_PACKET_SIZE];
    int txsize;
    int txcnt;
};

