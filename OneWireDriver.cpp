#include "OneWireDriver.h"

namespace one_wire_driver {

OneWireDriver::OneWireDriver(
        gpio_driver::IGpio& gpio,
        wait::IWait&        wait)
    :
        gpio_(gpio),
        wait_(wait)
{
}

uint8_t OneWireDriver::Reset(void) {
    return 0;
}

void OneWireDriver::Send(uint8_t send_buff[], uint16_t size) {

}

void OneWireDriver::Get(uint8_t recv_buff[], uint16_t size) {

}

void OneWireDriver::SendAndGet(uint8_t send_buff[], uint8_t recv_buff[], uint16_t size) {

}


} /* namespace one_wire_driver */
