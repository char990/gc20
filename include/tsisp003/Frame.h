#pragma once


#include <cstdint>
#include <string>
#include <tsisp003/TsiSp003Const.h>

class StFrm
{
public:
    int dataLen;
    uint8_t *rawData;
};

class Frame
{
public:
    Frame();
    virtual ~Frame();
    APP::ERROR appErr;
    uint8_t micode;
    uint8_t frmId;
    uint8_t frmRev;
    uint8_t colour;
    uint8_t conspicuity;
    int frmlen;
    int frmOffset;
    uint16_t crc;
    StFrm stFrm;

    void FrameCheck();

    virtual std::string ToString()=0;

protected:
    /// \brief Check item
    /// \return 0:success
    //int CheckConspicuity();
    virtual int CheckColour()=0;
    virtual int CheckLength(int len)=0;
    /// \brief Check subclass items
    virtual int CheckSub()=0;
};

class FrmTxt : public Frame
{
public:
    /// \breif  ini frame with hex array
    FrmTxt(uint8_t * frm, int len);
    virtual ~FrmTxt(){};
    
    uint8_t font;

    std::string ToString() override;

private:
    virtual int CheckColour() override;
    virtual int CheckLength(int len)override;
    virtual int CheckSub() override;
};

class FrmGfx : public Frame
{
public:
    /// \breif  ini frame with hex array
    FrmGfx(uint8_t * frm, int len);
    virtual ~FrmGfx(){};

    uint8_t pixelRows;
    uint8_t pixelColumns;


    std::string ToString() override;

private:
    virtual int CheckColour() override;
    virtual int CheckLength(int len)override;
    virtual int CheckSub() override;
};

class FrmHrg : public Frame
{
public:
    /// \breif  ini frame with hex array
    FrmHrg(uint8_t * frm, int len);
    virtual ~FrmHrg(){};


    uint16_t pixelRows;
    uint16_t pixelColumns;
    std::string ToString() override;
    
private:
    virtual int CheckColour() override;
    virtual int CheckLength(int len)override;
    virtual int CheckSub() override;
};
