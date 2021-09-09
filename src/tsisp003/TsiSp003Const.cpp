#include <tsisp003/TsiSp003Const.h>

const char * TSISP003VER[TSISP003VER_SIZE]={
    "QLD,2.1",
    "NSW,3.1",
    "NSW,5.0"
};

const char *PRODTYPE[PRODTYPE_SIZE] = {
    "VMS",   // 1 sign at 1 com, has 1-x slaves, slave id 1-x
    "ISLUS", //  1 group of ISLUS at 1 com, 1 sign has 1 slave, slave id 1
};
