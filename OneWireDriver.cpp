#include "OneWireDriver.h"

namespace one_wire_driver {

OneWireDriver::OneWireDriver(
        gpio_driver::IGpio&     gpio,
        iwait::IWait&           wait)
    :
        gpio_(gpio),
        wait_(wait)
{
    // By default set line to high state.
    gpio_.Set();
}

uint8_t OneWireDriver::Reset(void) {

    uint8_t is_present = 0;

    gpio_.Clear();
    wait_.wait_us(500);
    gpio_.Set();
    wait_.wait_us(45);

    // if received low state then slave is present.
    is_present = (gpio_.GetState() == 0);

    wait_.wait_us(470);

    return is_present;
}

void OneWireDriver::Send(uint8_t send_buff[], uint16_t size) {

    for (uint16_t i = 0; i < size; i++) {
        for (uint16_t bit = 0; bit < 8; bit++)
            this->SendBit(send_buff[i] & (1 << bit));

        this->wait_.wait_us(100);
    }
}

void OneWireDriver::Get(uint8_t recv_buff[], uint16_t size) {

    for (uint16_t i = 0; i < size; i++) {
        recv_buff[i] = 0;
        for (uint8_t bit = 0; bit < 8; bit++)
            recv_buff[i] |= (this->GetBit() << bit);

        this->wait_.wait_us(100);
    }
}

void OneWireDriver::SendAndGet(uint8_t send_buff[], uint8_t recv_buff[], uint16_t size) {
    //TODO: Add implementation.
}

void OneWireDriver::SendBit(uint8_t bit) {
    this->gpio_.Set();
    this->wait_.wait_us(5);
    this->gpio_.Clear();
    this->wait_.wait_us(5);

    if (bit)
        this->gpio_.Set();

    this->wait_.wait_us(80);
    this->gpio_.Set();
}

uint8_t OneWireDriver::GetBit(void) {

    this->gpio_.Set();
    this->wait_.wait_us(5);
    this->gpio_.Clear();
    this->wait_.wait_us(2);
    this->gpio_.Set();
    this->wait_.wait_us(15);

    return (this->gpio_.GetState() != 0);
}


} /* namespace one_wire_driver */
