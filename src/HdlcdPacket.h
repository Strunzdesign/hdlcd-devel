/**
 * \file      HdlcdPacket.h
 * \brief     This file contains the header declaration of class HdlcdPacket
 * \author    Florian Evers, florian-evers@gmx.de
 * \copyright BSD 3 Clause licence
 *
 * Copyright (c) 2016, Florian Evers, florian-evers@gmx.de
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 
 *     (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.  
 *     
 *     (3)The name of the author may not be used to
 *     endorse or promote products derived from this software without
 *     specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HDLCD_PACKET_H
#define HDLCD_PACKET_H

#include "Frame.h"

/*! enum E_HDLCD_PACKET
 *  \brief The enum E_HDLCD_PACKET
 * 
 *  This enum specifies the set of packet types of the HDLCd access protocol. Each of them is represented
 *  by a specific derived class of base class HdlcdPacket.
 */
typedef enum {
    HDLCD_PACKET_DATA    = 0x00, //!< A data packet of the HDLCd access protocol
    HDLCD_PACKET_CTRL    = 0x10, //!< A control packet of the HDLCd access protocol
    HDLCD_PACKET_UNKNOWN = 0xFF, //!< Unknown packet type
} E_HDLCD_PACKET;



/*! \class HdlcdPacket
 *  \brief Class HdlcdPacket
 * 
 *  This is the base class for all packet types of the HDLCd access protocol. This base class inherits from
 *  the Frame class and thus allows easy exchange via FrameEndpoint entities.
 */
class HdlcdPacket: public Frame {
public:
    virtual E_HDLCD_PACKET GetHdlcdPacketType() const { return HDLCD_PACKET_UNKNOWN; }
};

#endif // HDLCD_PACKET_H
