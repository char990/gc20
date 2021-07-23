#include "TsiSp003App.h"

TsiSp003App::TsiSp003App():adaptor(nullptr)
{

}

TsiSp003App::~TsiSp003App()
{

}

int TsiSp003App::Rx(uint8_t * data, int len)
{
    return 0;
}

int TsiSp003App::Tx(uint8_t * data, int len)
{
    if(adaptor!=nullptr)
    {
        return  adaptor->Tx(data,len);
    }
    return -1;
}
