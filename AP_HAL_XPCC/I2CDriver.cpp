
#include "AP_HAL_XPCC.h"
#include "I2CDriver.h"
#include <xpcc/architecture.hpp>

using namespace XpccHAL;

extern const AP_HAL::HAL& hal;

void I2CDriver::begin() {
	hal.scheduler->register_timer_process(AP_HAL_MEMBERPROC(&I2CDriver::watchdog));
}

void I2CDriver::end() {}

void I2CDriver::setTimeout(uint16_t ms) {}
void I2CDriver::setHighSpeed(bool active) {}

#define I2C xpcc::lpc17::I2cMaster2
extern const AP_HAL::HAL& hal;

uint8_t I2CDriver::write(uint8_t addr, uint8_t len, uint8_t* data)
{
	if(!initialize(addr, data, len, 0, 0)) {
		error_count++;
		return 1;
	}
	if(!I2C::start(this)) {
		error_count++;
		return 1;
	}
	while(getState() == xpcc::I2cWriteReadAdapter::AdapterState::Busy) {
		xpcc::yield();
	}
	bool failed = getState() != xpcc::I2cWriteReadAdapter::AdapterState::Idle;
	if(failed) error_count++;
	return failed;
}
uint8_t I2CDriver::writeRegister(uint8_t addr, uint8_t reg, uint8_t val)
{
	uint8_t data[2];
	data[0] = reg;
	data[1] = val;
	if(!initialize(addr, data, sizeof(data), 0, 0)) {
		error_count++;
		return 1;
	}
	if(!I2C::start(this)) {
		error_count++;
		return 1;
	}
	while(getState() == xpcc::I2cWriteReadAdapter::AdapterState::Busy) {
		xpcc::yield();
	}
	bool failed = getState() != xpcc::I2cWriteReadAdapter::AdapterState::Idle;
	if(failed) error_count++;
	return failed;
}
uint8_t I2CDriver::writeRegisters(uint8_t addr, uint8_t reg,
                               uint8_t len, uint8_t* data)
{
	uint8_t buf[len + 1];
	buf[0] = reg;
	memcpy(&buf[1], data, len);

	if(!initialize(addr, buf, len+1, 0, 0)) {
		error_count++;
		return 1;
	}
	if(!I2C::start(this)) {
		error_count++;
		return 1;
	}
	while(getState() == xpcc::I2cWriteReadAdapter::AdapterState::Busy) {
		xpcc::yield();
	}
	bool failed = getState() != xpcc::I2cWriteReadAdapter::AdapterState::Idle;
	if(failed) error_count++;
	return failed;
}

uint8_t I2CDriver::read(uint8_t addr, uint8_t len, uint8_t* data)
{
	if(!initialize(addr, 0, 0, data, len)) {
		error_count++;
		return 1;
	}
	if(!I2C::start(this)) {
		error_count++;
		return 1;
	}
	while(getState() == xpcc::I2cWriteReadAdapter::AdapterState::Busy) {
		xpcc::yield();
	}
	bool failed = getState() != xpcc::I2cWriteReadAdapter::AdapterState::Idle;
	if(failed) error_count++;
	return failed;
}

uint8_t I2CDriver::readRegister(uint8_t addr, uint8_t reg, uint8_t* data)
{
	if(!initialize(addr, &reg, 1, data, 1)) {
		error_count++;
		return 1;
	}
	if(!I2C::start(this)) {
		error_count++;
		return 1;
	}
	while(getState() == xpcc::I2cWriteReadAdapter::AdapterState::Busy) {
		xpcc::yield();
	}
	bool failed = getState() != xpcc::I2cWriteReadAdapter::AdapterState::Idle;
	if(failed) error_count++;
	return failed;
}

uint8_t I2CDriver::readRegisters(uint8_t addr, uint8_t reg,
                                      uint8_t len, uint8_t* data)
{
	if(!len) return 1;

	if(!initialize(addr, &reg, 1, data, len)) {
		error_count++;
		return 1;
	}
	if(!I2C::start(this)) {
		error_count++;
		return 1;
	}
	while(getState() == xpcc::I2cWriteReadAdapter::AdapterState::Busy) {
		xpcc::yield();
	}
	bool failed = getState() != xpcc::I2cWriteReadAdapter::AdapterState::Idle;
	if(failed) error_count++;
	return failed;
}

void I2CDriver::stopped(DetachCause cause) {
	xpcc::atomic::Lock l;
	I2cWriteReadAdapter::stopped(cause);

	if(nb_transaction) {
		if(nb_callback != 0){
			nb_transaction = 0;
			AP_HAL::MemberProc cb = nb_callback;
			nb_callback = 0;
			{
				xpcc::atomic::Unlock u;
				cb();
				//dbgclr(1);
			}
		}

	}
}

bool I2CDriver::readNonblocking(uint8_t addr, uint8_t reg,
                              uint8_t len, uint8_t* data,
							  AP_HAL::MemberProc callback) {
	if(nb_transaction || !len) {
		error_count++;
		return false;
	}

	data[0] = reg;
	if(!initialize(addr, data, 1, data, len)) {
		error_count++;
		return false;
	}
	{
		xpcc::atomic::Lock l;
		nb_callback = callback;
		nb_transaction = hal.scheduler->millis();
		if(!nb_transaction) nb_transaction = 1; //wraparound case, v unlikely

		if(!I2C::start(this)) {
			error_count++;
			nb_transaction = 0;
			return false;
		}

	}
	//dbgset(1);
	return true;
}

void I2CDriver::watchdog() {
	if(nb_transaction && (hal.scheduler->millis() - nb_transaction) > 50) {
		//lockup occurred
		//this is a bug, but to recover, reset i2c transaction and unlock semaphore
		error_count++;
		nb_transaction = 0;
		nb_callback = 0;
		get_semaphore()->give();
		XPCC_LOG_DEBUG .printf("i2c lockup\n");
	}
}

uint8_t I2CDriver::lockup_count() {
	return error_count;
}
