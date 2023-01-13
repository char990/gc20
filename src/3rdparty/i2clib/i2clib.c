#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "3rdparty/i2clib/i2c-dev.h"
#include "3rdparty/i2clib/i2cbusses.h"

/*
 * Params:
 * 	i2cbus   - bus number
 * 	address  - chip address
 * 	daddress - data address
 * 	size     - one of I2C_SMBUS_BYTE, I2C_SMBUS_BYTE_DATA or I2C_SMBUS_WORD_DATA
 */

int i2cget(int i2cbus, int address, int daddress, int size)
{
	int res, file;
	char filename[20];
	int force = 0;

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
	if (file < 0 || set_slave_addr(file, address, force))
		return -1;

	switch (size)
	{
	case I2C_SMBUS_BYTE:
		if (daddress >= 0)
		{
			res = i2c_smbus_write_byte(file, daddress);
			if (res < 0)
			{
				fprintf(stderr, "Warning - write failed\n");
				return -2;
			}
		}
		res = i2c_smbus_read_byte(file);
		break;
	case I2C_SMBUS_WORD_DATA:
		res = i2c_smbus_read_word_data(file, daddress);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		res = i2c_smbus_read_byte_data(file, daddress);
	}
	close(file);

	if (res < 0)
	{
		// fprintf(stderr, "Error: I2C Read failed\n");
		return -3;
	}

	// printf("0x%0*x\n", size == I2C_SMBUS_WORD_DATA ? 4 : 2, res);

	return res;
}

/*
 * Params:
 * 	i2cbus   - bus number
 * 	address  - chip address
 * 	daddress - data address
 * 	size     - one of I2C_SMBUS_BYTE, I2C_SMBUS_BYTE_DATA or I2C_SMBUS_WORD_DATA
 */

int i2cset(int i2cbus, int address, int daddress, int value, int size)
{
	int res, file;
	int vmask = 0;
	char filename[20];
	int force = 0, readback = 0;
	unsigned char block[I2C_SMBUS_BLOCK_MAX];
	int len = 0;

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
	if (file < 0 || set_slave_addr(file, address, force))
		return -1;

	if (vmask)
	{
		int oldvalue;

		switch (size)
		{
		case I2C_SMBUS_BYTE:
			oldvalue = i2c_smbus_read_byte(file);
			break;
		case I2C_SMBUS_WORD_DATA:
			oldvalue = i2c_smbus_read_word_data(file, daddress);
			break;
		default:
			oldvalue = i2c_smbus_read_byte_data(file, daddress);
		}

		if (oldvalue < 0)
		{
			fprintf(stderr, "Error: Failed to read old value\n");
			return -1;
		}

		value = (value & vmask) | (oldvalue & ~vmask);
	}

	switch (size)
	{
	case I2C_SMBUS_BYTE:
		res = i2c_smbus_write_byte(file, daddress);
		break;
	case I2C_SMBUS_WORD_DATA:
		res = i2c_smbus_write_word_data(file, daddress, value);
		break;
	case I2C_SMBUS_BLOCK_DATA:
		res = i2c_smbus_write_block_data(file, daddress, len, block);
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		res = i2c_smbus_write_i2c_block_data(file, daddress, len, block);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		res = i2c_smbus_write_byte_data(file, daddress, value);
		break;
	}
	if (res < 0)
	{
		fprintf(stderr, "Error: Write failed\n");
		close(file);
		return -1;
	}

	if (!readback)
	{ /* We're done */
		close(file);
		return 0;
	}

	switch (size)
	{
	case I2C_SMBUS_BYTE:
		res = i2c_smbus_read_byte(file);
		value = daddress;
		break;
	case I2C_SMBUS_WORD_DATA:
		res = i2c_smbus_read_word_data(file, daddress);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		res = i2c_smbus_read_byte_data(file, daddress);
	}
	close(file);

	if (res < 0)
	{
		printf("Warning - readback failed\n");
	}
	else if (res != value)
	{
		printf("Warning - data mismatch - wrote "
			   "0x%0*x, read back 0x%0*x\n",
			   size == I2C_SMBUS_WORD_DATA ? 4 : 2, value,
			   size == I2C_SMBUS_WORD_DATA ? 4 : 2, res);
	}
	else
	{
		printf("Value 0x%0*x written, readback matched\n",
			   size == I2C_SMBUS_WORD_DATA ? 4 : 2, value);
	}

	return 0;
}

int rd_i2c(int i2cbus, int slave_addr, int dst_addr, unsigned char len, unsigned char *buf)
{
	int res, file;
	char filename[32];
	int force = 0;
	if (len <= 0)
		return 0;

	res = -1;
	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
	if (file < 0)
		return -1;
	if (set_slave_addr(file, slave_addr, force) == 0)
	{
		res = i2c_smbus_read_i2c_block_data(file, dst_addr, len, buf);
	}
	close(file);
	return res;
}

int wr_i2c(int i2cbus, int slave_addr, int dst_addr, unsigned char len, const unsigned char *buf)
{
	int res, file;
	char filename[32];
	int force = 0;
	if (len <= 0)
		return 0;

	res = -1;
	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
	if (file < 0)
		return -1;
	if (set_slave_addr(file, slave_addr, force) == 0)
	{
		res = i2c_smbus_write_i2c_block_data(file, dst_addr, len, buf);
	}
	close(file);
	return res;
}