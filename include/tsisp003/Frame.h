#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <tsisp003/TsiSp003Const.h>
#include <uci/Font.h>

class StFrm
{
public:
    StFrm(){};
    ~StFrm()
    {
        if (rawData != nullptr)
        {
            delete[] rawData;
        }
    };
    int dataLen{0};
    uint8_t *rawData{nullptr};
    void Init(uint8_t *buf, int len)
    {
        if (rawData != nullptr)
        {
            delete[] rawData;
        }
        dataLen = len;
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
    int frmBytes; // frame string/bitmap data
    int frmOffset;
    uint16_t crc{0};
    StFrm stFrm;

    uint16_t pixelRows;
    uint16_t pixelColumns;

    /// \brief Check item
    /// \return 0:success
    int FrameCheck(uint8_t *frm, int len);

    virtual std::string ToString() = 0;

    /// \brief Convert 1/4-bit Gfx/Hrg frame to 4/24-bit bitmap
    ///         Text frame override this function
    /// \param  buf: buffer, be cleared in this function 
    /// \return bytes
    virtual int ToBitmap(uint8_t colourbit, uint8_t *buf);

protected:
    /// \brief Check item
    /// \return 0:success
    //int CheckConspicuity();
    virtual int CheckColour() = 0;

    /// \brief Check subclass items
    virtual int CheckSub(uint8_t *frm, int len) = 0;

    /// \brief Check Gfx/Hrg len
    /// \return 0:success
    virtual int CheckLength(int len);

    /// \brief Check Gfx/Hrg multicolour
    /// \return 0:success
    virtual int CheckMultiColour(uint8_t *frm, int len);

    /// \brief  Set a pixel in display buffer which is 1/4/24 bits map
    /// \param  colourbit:1/4/24
    /// \param  buf: buffer, preset as 0
    /// \param  x: x position of pixel
    /// \param  y: y position of pixel
    /// \param  monocolour: pixel colour (must be mono:0-9)
    void SetPixel(uint8_t colourbit, uint8_t *buf, int x, int y, uint8_t monocolour);
};

class FrmTxt : public Frame
{
public:
    /// \breif  ini frame with hex array
    FrmTxt(uint8_t *frm, int len);
    virtual ~FrmTxt(){};

    uint8_t font;

    std::string ToString() override;

    /// \brief  trans txt frame to 1/4/24 bits map
    /// \param  colourbit:0/1/4/24
    /// \param  buf: buffer, be cleared in this function 
    int ToBitmap(uint8_t colourbit, uint8_t *buf) override;
    void StrToBitmap(uint8_t colourbit, uint8_t *buf, int x, int y, uint8_t monocolour, char *str, Font * pfont);
    void CharToBitmap(uint8_t colourbit, uint8_t *buf, int x, int y, uint8_t monocolour, char c, Font * pfont);

private:
    virtual int CheckColour() override;
    virtual int CheckLength(int len) override;
    virtual int CheckSub(uint8_t *frm, int len) override;
};

class FrmGfx : public Frame
{
public:
    /// \breif  ini frame with hex array
    FrmGfx(uint8_t *frm, int len);
    virtual ~FrmGfx(){};

    std::string ToString() override;

private:
    virtual int CheckColour() override;
    virtual int CheckSub(uint8_t *frm, int len) override;
};

class FrmHrg : public Frame
{
public:
    /// \breif  ini frame with hex array
    FrmHrg(uint8_t *frm, int len);
    virtual ~FrmHrg(){};

    std::string ToString() override;

private:
    virtual int CheckColour() override;
    virtual int CheckSub(uint8_t *frm, int len) override;
};
