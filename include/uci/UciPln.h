#ifndef __UCIPLN_H__
#define __UCIPLN_H__

#include <string>
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
    # enabled(51), 88 chars
    option pln_1  "0D01......CRC51"
    # disabled(50), 88 chars
    option pln_2  "0D02......CRC050"

    # If plan CRC is not matched, discard plan
--- End ---
*/

class UciPln : public UciCfg
{
public:
    UciPln(UciFrm &uciFrm, UciMsg &uciMsg);
    ~UciPln();

    std::string SECTION;

    /// \brief  load plns[] from "UciPln"
    void LoadConfig() override;

	void Dump() override;

    /// \brief  Sum all frames's crc
    uint16_t ChkSum();

    /// \brief  Get plns[i]
    Plan * GetPln(int i);

    /// \brief  Set a plan from hex array, e.g. app layer data of SighSetPlan
    ///         plan will be stored in plns[] (but not saved in "UciPln")
    ///         new plan is at disable state
    /// \param  buf: hex array
    /// \param  len: array length
    /// \return APP::ERROR
    uint8_t SetPln(uint8_t * buf, int len);

    /// \brief  Save plns[i] to "UciPln", with CRC and en/dis attached
    ///         When TsiSp003 set a plan, call SetMsg then SaveMsg
    ///         When TsiSp003 enable/disable a plan, call SavePln
    /// \param  i: plns index
    void SavePln(int i);

    /// \brief  Check if there is undefined frm/msg in pln
    /// \return -1:pln has no frm/msg; 0:OK; 1: msg has undefined frm/msg 
    int CheckPlnEntries(Plan  * pln);

private:
    Plan * plns[256];  // [0] is nullptr
    uint16_t chksum;
    UciFrm &uciFrm;
    UciMsg &uciMsg;
};


#endif
