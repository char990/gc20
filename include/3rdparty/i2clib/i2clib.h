#ifndef __I2CLIB_H__
#define __I2CLIB_H__

#ifdef __cplusplus
extern "C"
{
#endif 

int i2cget(int i2cbus, int address, int daddress, int size);
int i2cset(int i2cbus, int address, int daddress, int value, int size);

#ifdef __cplusplus
}
#endif

#endif
