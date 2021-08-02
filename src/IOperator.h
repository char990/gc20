#ifndef __IOPERATOR_H__
#define __IOPERATOR_H__

#include <string>
#include "IGcEvent.h"
#include "TsiSp003Const.h"
#include "ILayer.h"

class IOperator : public IGcEvent , public ILowerLayer
{
public:

protected:
    int TxBytes(uint8_t * data, int len);
    int TxHandle();
    void ClrTx();
    uint8_t txbuf[MAX_DATA_PACKET_SIZE];
    int txsize;
    int txcnt;
};

#endif
