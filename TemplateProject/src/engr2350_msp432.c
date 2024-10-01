

#include "engr2350_msp432.h"

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#ifndef NULL
#define NULL 0
#endif

void init_hfxtclk();
void init_std_uart();

void SysInit(){
    init_hfxtclk();
    init_std_uart();
}

void init_hfxtclk(){
    CS_setExternalClockSourceFrequency(32000,48000000); // Inform the program of the external clock rates

    // Enable pins to use the HXFT
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,GPIO_PIN2|GPIO_PIN3,GPIO_PRIMARY_MODULE_FUNCTION);
    CS_startHFXT(0); // Start the external 48 MHz oscillator
    // Set SMCLK and MCLK to use the HFXT/2 (=24 MHz)
    CS_initClockSignal(CS_SMCLK,CS_HFXTCLK_SELECT,CS_CLOCK_DIVIDER_2);
    CS_initClockSignal(CS_MCLK,CS_HFXTCLK_SELECT,CS_CLOCK_DIVIDER_2);

}

void init_std_uart(){
    char dev[] = "UART";
    add_device(dev, _MSA, dopen, dclose, dread, dwrite, dlseek, dunlink, drename);
    freopen("UART:0", "w", stdout);
    freopen("UART:0", "r", stdin);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
}

void I2C_writeData(uint32_t moduleInstance
                  ,uint8_t PeriphAddress
                  ,uint8_t StartReg
                  ,uint8_t *data
                  ,uint8_t len)
{
    I2C_setSlaveAddress(moduleInstance,PeriphAddress); // Set the peripheral address
    I2C_setMode(moduleInstance,EUSCI_B_I2C_TRANSMIT_MODE); // Indicate a write operation

    I2C_masterSendMultiByteStart(moduleInstance,StartReg); // Start the communication.
                // This function does three things. It sends the START signal,
                // sends the address, and then sends the start register.

    // This code loops through all of the bytes to send.
    uint8_t ctr;
    for(ctr = 0;ctr<len;ctr++){
        I2C_masterSendMultiByteNext(moduleInstance,data[ctr]);
    }
    // Once all bytes are sent, the I2C transaction is stopped by sending the STOP signal
    I2C_masterSendMultiByteStop(moduleInstance);

    __delay_cycles(200); // A short delay to avoid starting another I2C transaction too quickly
}

void I2C_readData(uint32_t moduleInstance
                 ,uint8_t PeriphAddress
                 ,uint8_t StartReg
                 ,uint8_t *data
                 ,uint8_t len)
{
    // First write the start register to the peripheral device. This can be
    // done by using the I2C_writeData function with a length of 0.
    I2C_writeData(moduleInstance,PeriphAddress,StartReg,0,0);

    Interrupt_disableMaster(); //  Disable all interrupts to prevent timing issues

    // Then do read transaction...
    I2C_setSlaveAddress(moduleInstance,PeriphAddress); // Set the peripheral address
    I2C_setMode(moduleInstance,EUSCI_B_I2C_RECEIVE_MODE); // Indicate a read operation
    I2C_masterReceiveStart(moduleInstance); // Start the communication. This function
                // does two things: It first sends the START signal and
                // then sends the peripheral address. Once started, the eUSCI
                // will automatically fetch bytes from the peripheral until
                // a STOP signal is requested to be sent.

    // This code loops through 1 less than all bytes to receive
    uint8_t ctr;
    for(ctr = 0;ctr<(len-1);ctr++){
        uint32_t tout_tmp = 10000;
        while(!(EUSCI_B_CMSIS(moduleInstance)->IFG & EUSCI_B_IFG_RXIFG0) && --tout_tmp); // Wait for a data byte to become available
        if(tout_tmp){
            data[ctr] = I2C_masterReceiveMultiByteNext(moduleInstance); // read and store received byte
        }else{
            data[ctr] = 0xFF;
        }
    }
    // Prior to receiving the final byte, request the STOP signal such that the
    // communication will halt after the byte is received.
    data[ctr] = I2C_masterReceiveMultiByteFinish(moduleInstance); // send STOP, read and store received byte

    Interrupt_enableMaster(); // Re-enable interrupts

    __delay_cycles(200); // A short delay to avoid starting another I2C transaction too quickly
}




int dopen(const char *path, unsigned flags, int llv_fd){
    eUSCI_UART_ConfigV1 uart = {0};
    uart.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    uart.clockPrescalar = 13;
    uart.firstModReg = 0;
    uart.secondModReg = 0x25;
    uart.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;
    uart.parity = EUSCI_A_UART_NO_PARITY;
    uart.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    uart.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    uart.uartMode = EUSCI_A_UART_MODE;
    uart.dataLength = EUSCI_A_UART_8_BIT_LEN;

    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    MAP_UART_initModule(EUSCI_A0_BASE, &uart);
    MAP_UART_enableModule(EUSCI_A0_BASE);
    return 0;
}

int dclose(int dev_fd){
    MAP_UART_disableModule(EUSCI_A0_BASE);
    return 0;
}

int dread(int dev_fd, char *buf, unsigned count){
    int i;
    for(i = 0; i < count; i++){
        buf[i] = MAP_UART_receiveData(EUSCI_A0_BASE);
        if(buf[i] == '\n') return count;
    }
    return count;
}

int dwrite(int dev_fd, const char *buf, unsigned count){
    int i;
    for(i = 0; i < count; i++) MAP_UART_transmitData(EUSCI_A0_BASE, buf[i]);
    return count;
}

long dlseek(int dev_fd, off_t offset, int origin){ return -1; }
int dunlink(const char *path){ return -1; }
int drename(const char *old_name, const char *new_name){ return -1; }

uint8_t getchar_nw(){
    // Implementation of a non-blocking getchar
    if((BITBAND_PERI(EUSCI_A_CMSIS(EUSCI_A0_BASE)->IFG, EUSCI_A_IFG_RXIFG_OFS)))
        return EUSCI_A_CMSIS(EUSCI_A0_BASE)->RXBUF;
    else
        return 0;
}
