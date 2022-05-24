#include <iostream>
#include <tuple>

#include "icsnVC40.h"

#include <ice.h>

std::string base36_encode(unsigned int value)
{
    char base36[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    /* log(2**64) / log(36) = 12.38 => max 13 char + '\0' */
    char buffer[100];
    unsigned int offset = sizeof(buffer);

    buffer[--offset] = '\0';
    do {
        buffer[--offset] = base36[value % 36];
    } while (value /= 36);
    return std::string(&buffer[offset]);
}

auto base36_decode(std::string value)
{
    return std::strtol(value.c_str(), nullptr, 36);
}

int main(int argc, char* argv[]) 
{
    std::cout << "Hello World!\n";
#if 1
    try {
        ice::Library icsneo("icsneo40");
        ice::Function<int __stdcall (NeoDeviceEx*, int*, unsigned int*, unsigned int, POptionsFindNeoEx*, unsigned long)> icsneoFindDevices(&icsneo, "icsneoFindDevices");
        ice::Function<int __stdcall (NeoDeviceEx*, void**, unsigned char*, int, int, OptionsFindNeoEx*, unsigned long)> icsneoOpenDevice(&icsneo, "icsneoOpenDevice");
        ice::Function<int __stdcall (void*, unsigned int)> icsneoWaitForRxMessagesWithTimeOut(&icsneo, "icsneoWaitForRxMessagesWithTimeOut");
        ice::Function<int __stdcall (void*, icsSpyMessage*, int*, int*)> icsneoGetMessages(&icsneo, "icsneoGetMessages");

        auto _ics_msg_to_message = [=](icsSpyMessage& ics_msg) {
            auto is_fd = ics_msg.Protocol == SPY_PROTOCOL_CANFD;
            void* data = nullptr;
            if (is_fd) {
                if (ics_msg.ExtraDataPtrEnabled && ics_msg.ExtraDataPtr) {
                    data = ics_msg.ExtraDataPtr;
                } else {
                    data = (void*)ics_msg.Data;
                }
            }
            return std::make_tuple(ics_msg.NumberBytesData, data);
        };

        auto get_device = [=](std::string serial_number) {
            const unsigned int DEVICE_COUNT = 255;
            NeoDeviceEx devices[DEVICE_COUNT] = {};
            int device_count = DEVICE_COUNT;

            if (!icsneoFindDevices(devices, &device_count, NULL, 0, NULL, 0)) {
                throw std::runtime_error("icsneoFindDevices() failed!");
            }
            for (int i=0; i < device_count; ++i) {
                if (devices[i].neoDevice.SerialNumber == base36_decode(serial_number))
                    return devices[i];
            }
            throw std::runtime_error("Failed to find device");
        };
        // Lets find the device and open it
        auto device = get_device("V46508");
        std::cout << "Opening device " << base36_encode(device.neoDevice.SerialNumber) << "...\n";
        void* dev = nullptr;
        if (!icsneoOpenDevice(&device, &dev, NULL, 0, 0, NULL, 0)) {
            std::cerr << "Failed to open " << base36_encode(device.neoDevice.SerialNumber) << "!\n";
            return 1;
        }
        std::cout << "Opened device " << base36_encode(device.neoDevice.SerialNumber) << "!\n";


        // Lets poll for messages forever or until icsneoWaitForRxMessagesWithTimeOut fails
        unsigned long total_count = 0;
        while (true) {
            // Wait for messages to come across the bus before we do anything
            if (!icsneoWaitForRxMessagesWithTimeOut(dev, 1000)) {
                continue;
            }
            // Grab the actual messages
            const unsigned int MSG_SIZE = 20000;
            auto msgs = std::unique_ptr<icsSpyMessage[]>(new icsSpyMessage[MSG_SIZE]);
            int count = MSG_SIZE;
            int errors = 0;
            if (!icsneoGetMessages(dev, msgs.get(), &count, &errors)) {
                throw std::runtime_error("icsneoGetMessages() failed!");
            }
            // Iterate through all the messages
            for (int i=0; i < count; ++i) {
                ++total_count;
                if (msgs[i].ExtraDataPtrEnabled) {
                    std::cout << total_count << " " << msgs[i].ArbIDOrHeader << " " << (int)msgs[i].NumberBytesData << "\t";
                    auto& [size, data] = _ics_msg_to_message(msgs[i]);
                    if (data) {
                        for (int j=0; j < size && j < 16; ++j) {
                            std::cout << (int)((uint8_t*)data)[j] << ", ";
                        }
                    }
                }
                else {
                    std::cout << total_count;
                }
                std::cout << "                             \r";
            }
        }
#endif
    } catch (ice::Exception& ex) {
        std::cerr << ex.whatString() << "\n";
        return 1;
    } catch (std::runtime_error& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    } catch (...) {
        std::cerr << "Unknown error occurred!\n";
    }

    return 0;
}
