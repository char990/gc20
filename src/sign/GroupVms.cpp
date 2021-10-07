#include <cstring>
#include <sign/GroupVms.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>
#include <module/MyDbg.h>

GroupVms::GroupVms(uint8_t id)
    : Group(id)
{
    if (vSigns.size() != 1)
    {
        MyThrow("VMS: Group can only have ONE sign");
    }
    UciProd &prod = db.GetUciProd();
    for (int i = 0; i < prod.SlaveRowsPerSign() * prod.SlaveColumnsPerSign(); i++)
    { // slave id = 1~n
        auto s = new Slave(i + 1);
        vSlaves.push_back(s);
        vSigns[0]->AddSlave(s);
    }
    // load process
    auto disp = db.GetUciProcess().GetDisp(groupId);
    if (disp[0] > 0)
    {
        switch (disp[1])
        {
        case MI::CODE::SignDisplayFrame:
            DispFrm(disp[3]);
            break;
        case MI::CODE::SignDisplayMessage:
            DispMsg(disp[3]);
            break;
        default:
            MyThrow("Syntax Error: UciProcess.Group%d.Display", groupId);
            break;
        }
    }
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

bool GroupVms::TaskSetATF(int *_ptLine)
{
    MyThrow("VMS can not run ATF");
    return true;
}
