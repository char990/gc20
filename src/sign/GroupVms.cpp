#include <cstring>
#include <sign/GroupVms.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>

GroupVms::GroupVms(uint8_t id)
    : Group(id)
{
    if (vSigns.size() != 1)
    {
        MyThrow("VMS: Group can only have ONE sign");
    }
    UciProd & prod = DbHelper::Instance().GetUciProd();
    for (int i = 0; i < prod.SlaveRowsPerSign() * prod.SlaveColumnsPerSign(); i++)
    {// slave id = 1~n
        vSlaves.push_back(new Slave(i + 1));
    }
    DispFrm(0);
}

GroupVms::~GroupVms()
{
}

void GroupVms::PeriodicHook()
{
}

APP::ERROR GroupVms::DispAtomicFrm(uint8_t *cmd)
{
    return APP::ERROR::MiNotSupported;
}
