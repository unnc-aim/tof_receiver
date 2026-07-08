//
// Created by txy on 7/8/26.
//

#ifndef PACKET_HPP
#define PACKET_HPP

#include <algorithm>
#include <cstdint>
#include <vector>

namespace rm_serial_driver
{
    struct ReceivePacket
    {
        uint8_t header = 0x5A;
        uint16_t ch1;
        uint16_t ch2;
        uint16_t ch3;
        uint16_t ch4;
        uint32_t timestamp;
        uint16_t checksum = 0;
    } __attribute__((packed));

    inline ReceivePacket fromVector(const std::vector<uint8_t> & data)
    {
        ReceivePacket packet;
        std::copy(data.begin(), data.end(), reinterpret_cast<uint8_t*>(&packet));
        return packet;
    }

} // namespace tof_receiver


#endif //PACKET_HPP
