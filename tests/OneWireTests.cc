#include "gtest/gtest.h"
#include "OneWireDriver.h"
#include <vector>
#include <memory>

namespace test_OneWireDriver {

const int SEND_BIT_SET_US_TIME = 2;
const int SEND_BIT_CLEAR_US_TIME = 2;
const int SEND_BIT_WAIT_TO_END = 80;
const int SEND_BIT_WAIT_TO_NEXT_BIT = 100;
const int SEND_BYTE_WAIT_TO_NEXT_BYTE = 100;

const int GET_BIT_RESET_US_TIME = 2;
const int GET_BIT_CLEAR_US_TIME = 2;
const int GET_BIT_WAIT_TO_READ_US_TIME = 4;
const int GET_BIT_WAIT_TO_NEXT_BIT = 100;
const int GET_BYTE_WAIT_TO_NEXT_BYTE = 100;

enum DataType {
    TYPE_TIME_US = 0,
    TYPE_TIME_MS,
    TYPE_GPIO,
};

class TestStruct {
public:
    TestStruct(DataType type, int value)
        :
            type_(type),
            value_(value)
    {}

    DataType type_;
    int value_;
};

std::vector<std::unique_ptr<TestStruct>> received_data;

class OneWireGpioMock : public gpio_driver::IGpio {

public:

    OneWireGpioMock(std::vector<uint8_t>& get_state)
        :
            get_state_(get_state) {}

    virtual void SetDirection(gpio_driver::GpioDirection direction) {}
    virtual void SetPull(gpio_driver::GpioPull pull) {}
    virtual void Set(void) { received_data.push_back(std::unique_ptr<TestStruct>(new TestStruct(TYPE_GPIO, 1))); }
    virtual void Clear(void) { received_data.push_back(std::unique_ptr<TestStruct>(new TestStruct(TYPE_GPIO, 0))); }
    virtual void Toggle(void) {}

    virtual uint8_t GetState(void) {
        assert(!get_state_.empty());
        uint8_t ret = get_state_.back();
        get_state_.pop_back();
        return ret;
    }

    std::vector<uint8_t>& get_state_;
};

class OneWireWaitMock : public iwait::IWait {

public:
    virtual void wait_us(uint16_t time) { received_data.push_back(std::unique_ptr<TestStruct>(new TestStruct(TYPE_TIME_US, time))); }
    virtual void wait_ms(uint16_t time) { received_data.push_back(std::unique_ptr<TestStruct>(new TestStruct(TYPE_TIME_MS, time))); }
};

TEST(OneWireDriver, Reset_Slave_not_present) {

    std::vector<TestStruct> expected {
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, 480),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, 65),
        TestStruct(TYPE_TIME_US, 480)
    };

    std::vector<uint8_t> get_state { 1, 0 };
    std::reverse(get_state.begin(), get_state.end());

    OneWireWaitMock wait;
    OneWireGpioMock gpio(get_state);

    received_data.clear();

    one_wire_driver::OneWireDriver one_wire(
            gpio,
            wait);

    uint8_t is_present = one_wire.Reset();

    EXPECT_TRUE(is_present == 0);

    EXPECT_TRUE(expected.size() == received_data.size())                \
            << "expected.size()=" << expected.size()                    \
            << " != received_data.size()=" << received_data.size()      \
            << std::endl;

    for (int i = 0 ; i < expected.size() && i < received_data.size() ; i++) {
        EXPECT_TRUE(expected[i].value_ == received_data[i]->value_)     \
            << "Value exp=" << expected[i].value_                       \
            << " != recv[i]= " << received_data[i]->value_              \
            << " at position: " << i << std::endl;

        EXPECT_TRUE(expected[i].type_ == received_data[i]->type_)       \
            << "Type exp=" << expected[i].type_                         \
            << " != recv[i]= " << received_data[i]->type_               \
            << " at position: " << i << std::endl;
    }
}

TEST(OneWireDriver, Reset_Slave_present) {

    std::vector<TestStruct> expected {
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, 480),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, 65),
        TestStruct(TYPE_TIME_US, 480)
    };

    std::vector<uint8_t> get_state { 0, 1 };
    std::reverse(get_state.begin(), get_state.end());

    OneWireWaitMock wait;
    OneWireGpioMock gpio(get_state);

    received_data.clear();

    one_wire_driver::OneWireDriver one_wire(
            gpio,
            wait);

    uint8_t is_present = one_wire.Reset();

    EXPECT_TRUE(is_present == 1);

    EXPECT_TRUE(expected.size() == received_data.size())                \
            << "expected.size()=" << expected.size()                    \
            << " != received_data.size()=" << received_data.size()      \
            << std::endl;

    for (int i = 0 ; i < expected.size() && i < received_data.size(); i++) {
        EXPECT_TRUE(expected[i].value_ == received_data[i]->value_)     \
            << "Value exp=" << expected[i].value_                       \
            << " != recv= " << received_data[i]->value_                 \
            << " at position: " << i << std::endl;

        EXPECT_TRUE(expected[i].type_ == received_data[i]->type_)       \
            << "Type exp=" << expected[i].type_                         \
            << " != recv= " << received_data[i]->type_                  \
            << " at position: " << i << std::endl;
    }
}

TEST(OneWireDriver, SendData) {

    std::vector<TestStruct> expected {
        TestStruct(TYPE_GPIO, 1),

                                                                // First byte 0xDD
                                                                // 0b 1101 1101
                                                                //
        TestStruct(TYPE_GPIO, 1),                               // LSB (1)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (0)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (1)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (1)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (1)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               //(0)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               //(1)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // MSB (1)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_TIME_US, SEND_BYTE_WAIT_TO_NEXT_BYTE),  // End of byte wait 100us


                                                                // Second byte 0x25
                                                                // 0b 0010 0101
        TestStruct(TYPE_GPIO, 1),                               // LSB (1)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (0)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (1)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (0)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (0)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (1)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // (0)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_GPIO, 1),                               // MSB (0)
        TestStruct(TYPE_TIME_US, SEND_BIT_SET_US_TIME),         //
        TestStruct(TYPE_GPIO, 0),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_CLEAR_US_TIME),       //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_END),         //
        TestStruct(TYPE_GPIO, 1),                               //
        TestStruct(TYPE_TIME_US, SEND_BIT_WAIT_TO_NEXT_BIT),    //

        TestStruct(TYPE_TIME_US, SEND_BYTE_WAIT_TO_NEXT_BYTE),  //

    };

    std::vector<uint8_t> get_state;

    OneWireWaitMock wait;
    OneWireGpioMock gpio(get_state);

    received_data.clear();

    one_wire_driver::OneWireDriver one_wire(
            gpio,
            wait);

    uint8_t one_wire_send[] = { 0xDD, 0x25 };

    one_wire.Send(one_wire_send, sizeof(one_wire_send));

    EXPECT_TRUE(expected.size() == received_data.size())            \
            << "expected.size()=" << expected.size()                \
            << " != received_data.size()=" << received_data.size()  \
            << std::endl;

    for (int i = 0; i < expected.size() && i < received_data.size(); i++) {
        EXPECT_TRUE(expected[i].value_ == received_data[i]->value_) \
            << "Value exp=" << expected[i].value_                   \
            << " != recv=" << received_data[i]->value_              \
            << " at position: " << i << std::endl;

        EXPECT_TRUE(expected[i].type_ == received_data[i]->type_)   \
            << "Type exp=" << expected[i].type_                     \
            << " != recv=" << received_data[i]->type_               \
            << " at position: " << i << std::endl;
    }
}

TEST(OneWireDriver, GetData) {

    std::vector<TestStruct> expected {
        TestStruct(TYPE_GPIO, 1),

        TestStruct(TYPE_GPIO, 1),                               // LSB
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),                               // MSB
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_TIME_US, GET_BYTE_WAIT_TO_NEXT_BYTE),


        TestStruct(TYPE_GPIO, 1),                               // LSB
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_GPIO, 1),                               // MSB
        TestStruct(TYPE_TIME_US, GET_BIT_RESET_US_TIME),
        TestStruct(TYPE_GPIO, 0),
        TestStruct(TYPE_TIME_US, GET_BIT_CLEAR_US_TIME),
        TestStruct(TYPE_GPIO, 1),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_READ_US_TIME),
        TestStruct(TYPE_TIME_US, GET_BIT_WAIT_TO_NEXT_BIT),

        TestStruct(TYPE_TIME_US, GET_BYTE_WAIT_TO_NEXT_BYTE),
    };

    std::vector<uint8_t> expect_get_data { 0x29, 0xEA };

    std::vector<uint8_t> get_state {
        // Get back 0x29 0xEA
        // 0b0010 1001    1110 1010
        1, 0, 0, 1, 0, 1, 0, 0,
        0, 1, 0, 1, 0, 1, 1, 1
    };

    std::reverse(get_state.begin(), get_state.end());

    OneWireWaitMock wait;
    OneWireGpioMock gpio(get_state);

    received_data.clear();

    one_wire_driver::OneWireDriver one_wire(
            gpio,
            wait);

    uint8_t one_wire_get[2] = { 0x00, 0x00 };

    one_wire.Get(one_wire_get, sizeof(one_wire_get));

    EXPECT_TRUE(expected.size() == received_data.size())            \
            << "expected.size()=" << expected.size()                \
            << " != received_data.size()=" << received_data.size()  \
            << std::endl;

    for (int i = 0; i < expected.size() && i < received_data.size(); i++) {
        EXPECT_TRUE(expected[i].value_ == received_data[i]->value_) \
                << "Value exp=" << (int)expected[i].value_          \
                << " != recv=" << (int)received_data[i]->value_     \
                << " at position: " << i << std::endl;

        EXPECT_TRUE(expected[i].type_ == received_data[i]->type_)   \
                << "Type exp=" << expected[i].type_                 \
                << " != recv=" << received_data[i]->type_           \
                << " at position: " << i << std::endl;
    }

    for (int i = 0; i < expect_get_data.size(); i++)
        EXPECT_TRUE(expect_get_data[i] == one_wire_get[i])          \
                << "Expected=" << (int)expect_get_data[i]           \
                << " Got=" << (int)one_wire_get[i]                  \
                << " at position:" << i << std::endl;
}

} /* namespace test_OneWireDriver */
