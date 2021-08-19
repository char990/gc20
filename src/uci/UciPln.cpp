#include <uci/UciPln.h>
#include <module/Utils.h>


UciPln::UciPln(UciFrm &uciFrm, UciMsg &uciMsg) 
:uciFrm(uciFrm), uciMsg(uciMsg)
{
    for(int i=0;i<=255;i++)
    {
        plns[i]=nullptr;
    }
}

UciPln::~UciPln() 
{
    for(int i=0;i<=255;i++)
    {
        if(plns[i]!=nullptr)
        {
            delete plns[i];
        }
    }
}

void UciPln::LoadConfig()
{

    Dump();
}

void UciPln::Dump()
{


}

uint16_t UciPln::ChkSum()
{
    return chksum;
}