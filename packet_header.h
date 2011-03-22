#ifndef PACKET_HEADER_H
#define PACKET_HEADER_H
// Packet types for notifycation
#include <inttypes.h>

enum {
    kPacketType_R_M_I = 2,
    kPacketType_R_M_C
};

struct PacketHeader {
    int          fType;
    uint32_t        fSize; // includes size of header itself
};

struct PacketTest {     
    PacketHeader    fHeader;            // fType is kPacketTypeTest
    char            fMessage[32];       // Just for fun.
};

#endif	
