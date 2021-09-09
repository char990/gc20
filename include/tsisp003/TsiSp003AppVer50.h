#ifndef __TSISP003APPVER50_H__
#define __TSISP003APPVER50_H__

#include <tsisp003/TsiSp003AppVer31.h>

class TsiSp003AppVer50 : public TsiSp003AppVer31
{
public:
    TsiSp003AppVer50();
    virtual ~TsiSp003AppVer50();
    virtual std::string Version() override { return "Ver5.0"; }

    virtual int Rx(uint8_t *data, int len) override;

private:
    void SignRequestStoredFMP(uint8_t *data, int len);
    void SignSetHighResolutionGraphicsFrame(uint8_t *data, int len);
    void SignConfigurationRequest(uint8_t *data, int len);
    void SignDisplayAtomicFrames(uint8_t *data, int len);
};

#endif
