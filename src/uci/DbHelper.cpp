#include <uci/DbHelper.h>


DbHelper::~DbHelper()
{
    if(fonts!=nullptr)
    {
        for(int i=0;i<MAX_FONT+1;i++)
        {
            if(fonts[i]!=nullptr)
            {
                delete fonts[i];
            }
        }
        delete [] fonts;
    }
}

void DbHelper::Init()
{
    uciProd.LoadConfig();
    uciUser.LoadConfig();
    fonts = new Font[MAX_FONT+1];
    for(int i=0;i<MAX_FONT+1;i++)
    {
        fonts[i] = (uciProd.IsFont(i)) ? (new Font{FontName(i)}) : nullptr;
    }
    uciFrm.LoadConfig();
}

uint16_t DbHelper::HdrChksum()
{
    return uciFrm.ChkSum() + uciMsg.ChkSum() + uciPln.ChkSum();
}

Font * DbHelper::GetFont(int i)
{
    return (uciProd.IsFont(i))?font[i]:font[0];
}
