#include <cstring>
#include <sign/GroupIslus.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>

GroupIslus::GroupIslus(uint8_t id)
    : Group(id)
{
    if (vSlaves.size() != 1)
    {
        MyThrow("ISLUS: Sign can only have ONE slave");
    }

    for (int i = 0; i < vSigns.size(); i++)
    { // slave id = Sign Id
        vSlaves.push_back(new Slave(vSigns[i]->SignId()));
    }
}

GroupIslus::~GroupIslus()
{
}

void GroupIslus::PeriodicHook()
{
}

APP::ERROR GroupIslus::DispAtomicFrm(uint8_t *cmd)
{
#if 0
    if (FacilitySwitch::FS_STATE::AUTO != fcltSw.Get())
    {
        return APP::ERROR::FacilitySwitchOverride;
    }
    if (cmd[2] != signCnt)
    {
        return APP::ERROR::SyntaxError;
    }
    uint8_t *p = cmd + 3;
    for (int i = 0; i < signCnt; i++)
    {
        // check if there is a duplicate sign id
        uint8_t *p2 = p + 2;
        for (int j = i + 1; j < signCnt; j++)
        {
            if (*p == *p2)
            {
                return APP::ERROR::SyntaxError;
            }
            p2 += 2;
        }
        // sign is in this group
        if (!IsSignInGroup(*p))
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        p++;
        // frm is defined or frm0
        if ((*p != 0) && DbHelper::Instance().GetUciFrm().GetFrm(*p) == nullptr)
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
        p++;
    }
    db.GetUciProcess().SetDisp(groupId, cmd, 3+signCnt*2);
    dsNext->dispType = DISP_STATUS::TYPE::ATF;
    p = cmd + 3;
    for (int i = 0; i < signCnt; i++)
    {
        dsNext->fmpid[i] = 0;
        for (int j = 0; i < signCnt; j++)
        {
            if (*p == vSigns[i]->SignId())
            { // matched sign id
                dsNext->fmpid[i] = *(p + 1);
                break;
            }
        }
        p += 2;
    }
#endif
    return APP::ERROR::AppNoError;
}
