#include <3rdparty/catch2/enable_test.h>
#ifdef CATCH2TEST
#define CATCH_CONFIG_MAIN
#include <3rdparty/catch2/catch.hpp>
#include <module/Utils.h>
#include <module/Debounce.h>
#include <time.h>

using namespace Utils;

TEST_CASE("Class Utils::Bits", "[Bits]")
{
    Bits b1(256);

    SECTION("256 bits empty")
    {
        REQUIRE(b1.Size() == 256);
        REQUIRE(b1.Data() != nullptr);
        for (int i = 0; i < 256; i++)
        {
            REQUIRE(b1.GetBit(i) == false);
        }
        REQUIRE(b1.GetMaxBit() == -1);
    }

    SECTION("SetBit:0")
    {
        b1.SetBit(0);
        REQUIRE(b1.GetMaxBit() == 0);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        REQUIRE(b1.GetMaxBit() == 7);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
            case 7:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        REQUIRE(b1.GetMaxBit() == 255);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
            case 7:
            case 255:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255 and ClrBit:1")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        b1.ClrBit(1);
        REQUIRE(b1.GetMaxBit() == 255);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
            case 7:
            case 255:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255 and ClrBit:7")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        b1.ClrBit(7);
        REQUIRE(b1.GetMaxBit() == 255);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
            case 255:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255 and ClrBit:7,255")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        b1.ClrBit(7);
        b1.ClrBit(255);
        REQUIRE(b1.GetMaxBit() == 0);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255 and ClrBit:7,255,0")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        b1.ClrBit(7);
        b1.ClrBit(255);
        b1.ClrBit(0);
        REQUIRE(b1.GetMaxBit() == -1);
        for (int i = 0; i < 256; i++)
        {
            REQUIRE(b1.GetBit(i) == false);
        }
    }

    SECTION("SetBit=8,16,63")
    {
        b1.SetBit(8);
        b1.SetBit(16);
        b1.SetBit(63);
        REQUIRE(b1.GetMaxBit() == 63);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 8:
            case 16:
            case 63:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("b1.ClrAll")
    {
        b1.ClrAll();
        REQUIRE(b1.GetMaxBit() == -1);
        for (int i = 0; i < 256; i++)
        {
            REQUIRE(b1.GetBit(i) == false);
        }
    }

    SECTION("b2=72-bit.SetBit(0,1,7,8,31,32,63)")
    {
        Bits b2;
        b2.Init(72);
        b2.SetBit(0);
        b2.SetBit(1);
        b2.SetBit(7);
        b2.SetBit(8);
        b2.SetBit(31);
        b2.SetBit(32);
        b2.SetBit(63);
        REQUIRE(b2.GetMaxBit() == 63);
        for (int i = 0; i < b2.Size(); i++)
        {
            switch (i)
            {
            case 0:
            case 1:
            case 7:
            case 8:
            case 31:
            case 32:
            case 63:
                REQUIRE(b2.GetBit(i) == true);
                break;
            default:
                REQUIRE(b2.GetBit(i) == false);
                break;
            }
        }
        b1.Clone(b2);
        REQUIRE(b1.GetMaxBit() == 63);
        REQUIRE(b1.Size() == b2.Size());
        for (int i = 0; i < b1.Size(); i++)
        {
            switch (i)
            {
            case 0:
            case 1:
            case 7:
            case 8:
            case 31:
            case 32:
            case 63:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }
}



TEST_CASE("Class DebounceByTime", "[DebounceByTime]")
{
#define DBNC_TIME_START 10000
#define SET_DBNC_TIMET(a) ((time_t)(a+DBNC_TIME_START))
    DebounceByTime dbnc;
    int RISING_CNT = 50;
    int FALING_CNT = 20;
    
    dbnc.SetCNT(RISING_CNT, FALING_CNT);
    dbnc.Reset();
    SECTION("dbnc by time start")
    {
        int t=0;
        dbnc.Check(1, SET_DBNC_TIMET(t));
        REQUIRE(dbnc.Value() == Utils::STATE5::S5_NA);
        t+=RISING_CNT-1;
        dbnc.Check(1, SET_DBNC_TIMET(t));
        REQUIRE(dbnc.Value() == Utils::STATE5::S5_NA);
        t++;
        dbnc.Check(1, SET_DBNC_TIMET(t));
        REQUIRE(dbnc.IsRising() == true);
        
        dbnc.Check(0, SET_DBNC_TIMET(t));
        REQUIRE(dbnc.IsRising() == true);
        t+=FALING_CNT-1;
        dbnc.Check(0, SET_DBNC_TIMET(t));
        REQUIRE(dbnc.IsRising() == true);
        t++;
        dbnc.Check(0, SET_DBNC_TIMET(t));
        REQUIRE(dbnc.IsFalling() == true);
        t++;
        dbnc.Check(1, SET_DBNC_TIMET(t));
        REQUIRE(dbnc.IsFalling() == true);

        t+=RISING_CNT-1;
        dbnc.Check(1, SET_DBNC_TIMET(t));
        REQUIRE(dbnc.IsFalling() == true);
        t++;
        dbnc.Check(1, SET_DBNC_TIMET(t));
        REQUIRE(dbnc.IsRising() == true);
    }
}

TEST_CASE("Class Crc16_8005", "[Crc16_8005]")
{
    uint8_t buf1[] = {0x02, 0x30, 0x31, 0x30, 0x35};
    uint8_t buf2[] = {
        0x02, 0x30, 0x32, 0x30, 0x36,
        0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
        0x31, 0x30, 0x31, 0x34, 0x44, 0x46, 0x34, 0x30, 0x31, 0x34, 0x44, 0x46, 0x34};
    SECTION("Crc16_8005")
    {
        REQUIRE(Crc::Crc16_8005(buf1, sizeof(buf1)) == 0x18F3);
        REQUIRE(Crc::Crc16_8005(buf2, sizeof(buf2)) == 0x4AFF);
    }
}

TEST_CASE("Class Debounce", "[Debounce]")
{
    int RISING_CNT = 50;
    int FALING_CNT = 20;
    Debounce test(RISING_CNT, FALING_CNT);

    SECTION("N/A->Rising")
    {
        for(int i=0;i<RISING_CNT-1;i++)
        {
            test.Check(1);
            REQUIRE(test.Value() == Utils::STATE5::S5_NA);
        }
        test.Check(1);
        REQUIRE(test.IsRising() == true);
    }
    
    SECTION("N/A->Falling")
    {
        for(int i=0;i<FALING_CNT-1;i++)
        {
            test.Check(0);
            REQUIRE(test.Value() == Utils::STATE5::S5_NA);
        }
        test.Check(0);
        REQUIRE(test.IsFalling() == true);
    }

    SECTION("1->Falling")
    {
        test.Check(1, RISING_CNT);
        REQUIRE(test.IsRising() == true);
        for(int i=0;i<FALING_CNT-1;i++)
        {
            test.Check(0);
            REQUIRE(test.IsRising() == true);
        }
        test.Check(0);
        REQUIRE(test.IsFalling() == true);
    }

    SECTION("0->Rising")
    {
        test.Check(0, FALING_CNT);
        REQUIRE(test.IsFalling() == true);
        for(int i=0;i<RISING_CNT-1;i++)
        {
            test.Check(1);
            REQUIRE(test.IsFalling() == true);
        }
        test.Check(1);
        REQUIRE(test.IsRising() == true);
    }
}


#endif
