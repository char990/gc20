#ifndef __TSISP003APPVER10_H__
#define __TSISP003APPVER10_H__

#include "TsiSp003Cfg.h"
#include "ITsiSp003App.h"

class TsiSp003AppVer10:ITsiSp003App
{
public:
    TsiSp003AppVer10();
    ~TsiSp003AppVer10();

    virtual std::string Version() override { return std::string{"Ver1.0"}; };

    virtual int Rx(uint8_t * data, int len) override
    {
        if(NewMi(data, len)==0)
        {
            return 0;
        }
        /* run app layer functions */
        return -1;
    }

    /// \brief  
    virtual int NewMi(uint8_t * data, int len)
    {
        return -1;
    }
};

#endif
