/*
 * engr2350_msp432.h
 *
 * Last Modified: 2024-04-03
 */

#ifndef ENGR2350_MSP432_H_
#define ENGR2350_MSP432_H_

#include <stdio.h>
#include <file.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// Clock and UART initializations
void SysInit();

// I2C high-level functions
void I2C_writeData(uint32_t moduleInstance
                  ,uint8_t PeriphAddress
                  ,uint8_t StartReg
                  ,uint8_t *data
                  ,uint8_t len);
void I2C_readData(uint32_t moduleInstance
                 ,uint8_t PeriphAddress
                 ,uint8_t StartReg
                 ,uint8_t *data
                 ,uint8_t len);

// stdio.h support
int dopen(const char *path, unsigned flags, int llv_fd);
int dclose(int dev_fd);
int dread(int dev_fd, char *buf, unsigned count);
int dwrite(int dev_fd, const char *buf, unsigned count);
long dlseek(int dev_fd, off_t offset, int origin);
int dunlink(const char *path);
int drename(const char *old_name, const char *new_name);

uint8_t getchar_nw();

#endif /* ENGR2350_MSP432_H_ */
