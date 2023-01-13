#ifndef __I2CLIB_H__
#define __I2CLIB_H__

#ifdef __cplusplus
extern "C"
{
#endif 

int i2cget(int i2cbus, int slave_addr, int dst_addr, int size);
int i2cset(int i2cbus, int slave_addr, int dst_addr, int value, int size);

int rd_i2c(int i2cbus, int slave_addr, int dst_addr, unsigned char len/*MAX=32*/, unsigned char *buf);
int wr_i2c(int i2cbus, int slave_addr, int dst_addr, unsigned char len/*MAX=32*/, const unsigned char *buf);

#ifdef __cplusplus
}
#endif

#endif
