#include <sign/SignCfg.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>

const char *SIGNTYPE[SIGNTYPE_SIZE]={
    "TYPEC-1",     // typec, 288*64, 6 slaves, 4-bit 
    "MINIVMS-1",    // minivms(floodsign), 64*25, 1 slave, amber
};

SignCfg::SignCfg(uint8_t signType)
:signType(signType)
{
    switch(signType)
    {
        case 0:
            MakeSignType0();
        break;
        case 1:
            MakeSignType1();
        break;
        default:
            MyThrow ("Unknown sighType:%d",signType);
    }
}


void SignCfg::MakeSignType0()
{
    maxTextFrmLen=255;
    pixelRows=64;
    pixelColumns=288;
    DbHelper::Instance().uciProd.FontName(0);
    charRows=3;
    charColumns=18;
}

void SignCfg::MakeSignType1()
{

}


int SignCfg::MinGfxFrmLen()
{
    return (Pixels() + 7) / 8;
}

int SignCfg::MaxGfxFrmLen()
{
    switch (DbHelper::Instance().uciProd.ColourBits())
    {
    case 1:
        return (Pixels() + 7) / 8;
    case 4:
        return (Pixels() + 2) / 2;
    default:
        return 0;
    }
}

int SignCfg::MinHrgFrmLen()
{
    return (Pixels() + 7) / 8;
}

int SignCfg::MaxHrgFrmLen()
{
    switch (DbHelper::Instance().uciProd.ColourBits())
    {
    case 1:
        return (Pixels() + 7) / 8;
    case 4:
        return (Pixels() + 2) / 2;
    default:
        return (Pixels() * 3);
    }
}

