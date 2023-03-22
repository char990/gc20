#pragma once

#include <string>
#include <module/IGcEvent.h>
#include <tsisp003/TsiSp003Const.h>
#include <layer/ILayer.h>
#include <module/RingBuf.h>

#define OPTXBUF_SIZE 1024
class IOperator : public IGcEvent, public ILowerLayer
{
public:
    IOperator() : IOperator(POWEROF2_MAX_DATA_PACKET_SIZE){};
    IOperator(int buf_size) : ringBuf(buf_size){};
    virtual ~IOperator() {};
    bool IsTxRdy() { return ringBuf.Cnt()==0 && txsize == 0; };

    int TxBytes(uint8_t *data, int len);
    int TxHandle();
    void ClrTx() override;

    void PrintRxBuf() override{} ;

protected:
    uint8_t optxbuf[OPTXBUF_SIZE];
    int bufsize;
    int txsize{0};
    int txcnt{0};
    RingBuf ringBuf;
};
