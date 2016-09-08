/**
 * \file      HdlcdPacket.h
 * \brief     
 * \author    Florian Evers, florian-evers@gmx.de
 * \copyright GNU Public License version 3.
 *
 * The HDLC Deamon implements the HDLC protocol to easily talk to devices connected via serial communications.
 * Copyright (C) 2016  Florian Evers, florian-evers@gmx.de
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HDLCD_PACKET_H
#define HDLCD_PACKET_H

#include "Frame.h"

typedef enum {
    HDLCD_PACKET_DATA    = 0x00,
    HDLCD_PACKET_CTRL    = 0x10,
    HDLCD_PACKET_UNKNOWN = 0xFF,
} E_HDLCD_PACKET;

class HdlcdPacket: public Frame {
public:
    virtual E_HDLCD_PACKET GetHdlcdPacketType() const { return HDLCD_PACKET_UNKNOWN; }
};

#endif // HDLCD_PACKET_H
