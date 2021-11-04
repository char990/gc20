#pragma once


#include <string>
#include <array>
#include <tsisp003/Plan.h>
#include <uci/UciCfg.h>
#include <uci/UciFrm.h>
#include <uci/UciMsg.h>
/*
File: "./config/UciPln"
Format: UCI
Note : CRC is uint16_t of HEX
--- Start ---
config UciPln pln

    # pln_xxx : xxx is plan ID, 1-255, 0 is not allowed

    # mostly same as SetPlan
    # min = 11(4+6*1+1) + crc(2) + "enable_flag"(1)
    # max = 40(4+6*6) + crc(2) + "enable_flag"(1)
    # enabled(01), 88 chars
    option pln_1  "0D01......CRC01"
    # disabled(00), 88 chars
    option pln_2  "0D02......CRC00"

    # If plan CRC is not matched, discard plan
--- End ---
*/

class UciPln : public UciCfg
{
public:
    UciPln();
    ~UciPln();

    /// \brief  load plns[] from "UciPln"
    void LoadConfig() override;

	void Dump() override;

    /// \brief  Sum all frames's crc
    uint16_t ChkSum();

    /// \brief  Get plns[i-1]
    Plan * GetPln(uint8_t i);

    bool IsPlnDefined(uint8_t i);

    /// \brief  Get plns[i-1]->plnRev, plns[0] is 0
    uint8_t GetPlnRev(uint8_t i);

    /// \brief  Set a plan from hex array, e.g. app layer data of SighSetPlan
    ///         plan will be stored in plns[] (but not saved in "UciPln")
    ///         new plan is at disable state
    /// \param  buf: hex array
    /// \param  len: array length
    /// \return APP::ERROR
    APP::ERROR SetPln(uint8_t * buf, int len);

    /// \brief  Save plns[i-1] to "UciPln", with CRC and en/dis attached
    ///         When TsiSp003 set a plan, call SetMsg then SaveMsg
    ///         When TsiSp003 enable/disable a plan, call SavePln
    /// \param  i: plns index
    void SavePln(uint8_t i);

    void Reset();

private:
    std::array<Plan,255> plns;  // 255 plans
    uint16_t chksum;
    const char * _Option = "pln_%d";
};


