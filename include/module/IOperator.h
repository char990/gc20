#ifndef __IOPERATOR_H__
#define __IOPERATOR_H__

#include <string>
#include <module/IGcEvent.h>
#include <tsisp003/TsiSp003Const.h>
#include <layer/ILayer.h>

class IOperator : public IGcEvent , public ILowerLayer
{
public:
    int TxBytes(uint8_t * data, int len);
    int TxHandle();
    void ClrTx();
protected:
    uint8_t txbuf[MAX_DATA_PACKET_SIZE];
    int txsize;
    int txcnt;
};

#endif
