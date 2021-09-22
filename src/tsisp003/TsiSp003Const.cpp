#include <tsisp003/TsiSp003Const.h>

const char * TSISP003VER[TSISP003VER_SIZE]={
    "QLD,2.1",
    "NSW,3.1",
    "NSW,5.0"
};

const char *PRODTYPE[PRODTYPE_SIZE] = {
    "VMS",   // 1 group of VMS at 1 com, 1 group has 1 sign, 1 sign has 1-x slaves, sign id is not equal to slave id
    "ISLUS", //  1 group of ISLUS at 1 com, 1 group has 1-x sign, 1 sign has 1 slave, slave id is same as sign Id
};
