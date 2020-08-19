#ifndef _I2C_DRIVER_H_
#define _I2C_DRIVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

bool InitI2C();
bool writeI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *Data, uint8_t ui8ByteCount);
bool readI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *Data, uint8_t ui8ByteCount);
bool readBurstI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *Data, uint32_t ui32ByteCount);
bool writeI2CData(uint8_t ui8Addr, uint8_t *Data, uint8_t ui8ByteCount);
#endif /* _I2C_DRIVER_H_ */
