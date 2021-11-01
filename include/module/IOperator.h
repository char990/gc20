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
    void ClrTx() override;
protected:
    uint8_t optxbuf[MAX_DATA_PACKET_SIZE];
    int txsize{0};
    int txcnt{0};
};

