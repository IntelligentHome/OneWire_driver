#pragma once

#include "ITransport.h"
#include "IGpioDriver.h"
#include "IWait.h"

namespace one_wire_driver {

class OneWireDriver : public transport::ITransport {

public:

    OneWireDriver(
            gpio_driver::IGpio&     gpio,
            iwait::IWait&           wait);

    virtual uint8_t Reset(void);
    virtual void Send(uint8_t send_buff[], uint16_t size);
    virtual void Get(uint8_t recv_buff[], uint16_t size);
    virtual void SendAndGet(uint8_t send_buff[], uint8_t recv_buff[], uint16_t size);

private:
    gpio_driver::IGpio&     gpio_;
    iwait::IWait&           wait_;

    void SendBit(uint8_t bit);
    uint8_t GetBit(void);

};

} /* namespace one_wire_driver */
