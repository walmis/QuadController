
#include <AP_HAL.h>
#if CONFIG_HAL_BOARD == HAL_BOARD_XPCC

#include "HAL_XPCC_Class.h"
#include "AP_HAL_Empty_Private.h"

using namespace XpccHAL;

extern UARTDriver uartADriver;
extern UARTDriver uartBDriver;
extern UARTDriver uartCDriver;
extern UARTDriver uartDDriver;
extern UARTDriver uartEDriver;
extern UARTDriver uartConsoleDriver;

Semaphore  i2cSemaphore;
I2CDriver  i2cDriver(&i2cSemaphore);
SPIDeviceManager spiDeviceManager;
AnalogIn analogIn;
Storage storageDriver;
GPIO gpioDriver;
RCInput rcinDriver;
RCOutput rcoutDriver;
Scheduler schedulerInstance;
Util utilInstance;

HAL_XPCC::HAL_XPCC() :
    AP_HAL::HAL(
        &uartADriver,
        &uartBDriver,
        &uartCDriver,
        &uartDDriver,
        &uartEDriver,
        &i2cDriver,
        &spiDeviceManager,
        &analogIn,
        &storageDriver,
        &uartConsoleDriver,
        &gpioDriver,
        &rcinDriver,
        &rcoutDriver,
        &schedulerInstance,
        &utilInstance)
{}

void HAL_XPCC::init(int argc,char* const argv[]) const {
    /* initialize all drivers and private members here.
     * up to the programmer to do this in the correct order.
     * Scheduler should likely come first. */
	analogin->init(0);

    scheduler->init(0);

    storage->init(0);
    rcout->init(0);

    i2c->begin();

}

const HAL_XPCC AP_HAL_XPCC;

#endif
