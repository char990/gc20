#pragma once


#include <cstdint>
#include <cstring>
#include <string>
#include <tsisp003/TsiSp003Const.h>

class StFrm
{
public:
    StFrm(){};
    ~StFrm()
    {
        if(rawData!=nullptr)
        {
            delete [] rawData;
        }
    };
    int dataLen{0};
    uint8_t * rawData{nullptr};
    void Init(uint8_t * buf, int len)
    {
        if(rawData!=nullptr)
        {
            delete [] rawData;
        }
        dataLen=len;
        rawData = new uint8_t[len];
        memcpy(rawData, buf, len);
    }
};

class Frame
{
public:
    Frame();
    virtual ~Frame();
    APP::ERROR appErr{APP::ERROR::AppNoError};
    uint8_t micode{0};
    uint8_t frmId;
    uint8_t frmRev;
    uint8_t colour;
    uint8_t conspicuity;
    int frmBytes;           // 
    int frmOffset;
    uint16_t crc{0};
    StFrm stFrm;

    /// \brief Check item
    /// \return 0:success
    int FrameCheck(uint8_t *frm, int len);   

    virtual std::string ToString()=0;

    // colourType could 0/1/4/24
    // For Gfx & Hrg frames
    // For Txt frame, override this function
    virtual uint8_t* ToSlaveFormat(uint8_t *buf, uint8_t colourType, uint8_t *orBuf);

protected:
    /// \brief Check item
    /// \return 0:success
    //int CheckConspicuity();
    virtual int CheckColour()=0;
    virtual int CheckLength(int len)=0;
    /// \brief Check subclass items
    virtual int CheckSub(uint8_t *frm, int len)=0;
};

class FrmTxt : public Frame
{
public:
    /// \breif  ini frame with hex array
    FrmTxt(uint8_t * frm, int len);
    virtual ~FrmTxt(){};
    
    uint8_t font;

    std::string ToString() override;

    virtual uint8_t* ToSlaveFormat(uint8_t *buf, uint8_t colourType, uint8_t *orBuf) override;

private:
    virtual int CheckColour() override;
    virtual int CheckLength(int len)override;
    virtual int CheckSub(uint8_t *frm, int len) override;
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
    virtual int CheckSub(uint8_t *frm, int len) override;
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
    virtual int CheckSub(uint8_t *frm, int len) override;
};
