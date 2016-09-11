/**
 * \file      HdlcdPacketData.h
 * \brief 
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

#ifndef HDLCD_PACKET_DATA_H
#define HDLCD_PACKET_DATA_H

#include "HdlcdPacket.h"
#include <memory>

class HdlcdPacketData: public HdlcdPacket {
public:
    static HdlcdPacketData CreatePacket(const std::vector<unsigned char> a_Payload, bool a_bReliable, bool a_bInvalid = false, bool a_bWasSent = false) {
        // Called for transmission
        HdlcdPacketData l_PacketData;
        l_PacketData.m_Buffer = std::move(a_Payload);
        l_PacketData.m_bReliable = a_bReliable;
        l_PacketData.m_bInvalid = a_bInvalid;
        l_PacketData.m_bWasSent = a_bWasSent;
        return l_PacketData;
    }

    static std::shared_ptr<HdlcdPacketData> CreateDeserializedPacket() {
        // Called on reception: evaluate type field
        auto l_PacketData(std::shared_ptr<HdlcdPacketData>(new HdlcdPacketData));
        l_PacketData->m_eDeserialize = DESERIALIZE_HEADER; // Next: read header including the packet type byte
        l_PacketData->m_BytesRemaining = 3;
        return l_PacketData;
    }
    
    const std::vector<unsigned char>& GetData() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_Buffer;
    }
    
    bool GetReliable() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_bReliable;
    }
    
    bool GetInvalid() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_bInvalid;
    }
    
    bool GetWasSent() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_bWasSent;
    }
    
private:
    // Private CTOR
    HdlcdPacketData(): m_bReliable(false), m_bInvalid(false), m_bWasSent(false), m_eDeserialize(DESERIALIZE_FULL) {
    }
    
    // Internal helpers
    E_HDLCD_PACKET GetHdlcdPacketType() const { return HDLCD_PACKET_DATA; }

    // Serializer
    const std::vector<unsigned char> Serialize() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        std::vector<unsigned char> l_Buffer;
        
        // Prepare type field
        unsigned char l_Type = 0x00;
        if (m_bReliable) { l_Type |= 0x04; }
        if (m_bInvalid)  { l_Type |= 0x02; }
        if (m_bWasSent)  { l_Type |= 0x01; }
        l_Buffer.emplace_back(l_Type);
        
        // Prepare length field
        l_Buffer.emplace_back((m_Buffer.size() >> 8) & 0xFF);
        l_Buffer.emplace_back((m_Buffer.size() >> 0) & 0xFF);
        
        // Add payload
        l_Buffer.insert(l_Buffer.end(), m_Buffer.begin(), m_Buffer.end());
        return l_Buffer;
    }
    
    // Deserializer
    bool Deserialize() {
        // All requested bytes are available
        switch (m_eDeserialize) {
        case DESERIALIZE_HEADER: {
            // Deserialize the length field
            assert(m_Buffer.size() == 3);

            // Deserialize the control byte
            const unsigned char &l_Control = m_Buffer[0];
            if (l_Control & 0x08) {
                // The reserved bit was set... abort
                m_eDeserialize = DESERIALIZE_ERROR;
                return false;
            } // if

            m_bReliable = (l_Control & 0x04);
            m_bInvalid  = (l_Control & 0x02);
            m_bWasSent  = (l_Control & 0x01);
            
            // Parse length field
            m_BytesRemaining = ntohs(*(reinterpret_cast<const uint16_t*>(&m_Buffer[1])));
            m_Buffer.clear();
            if (m_BytesRemaining) {
                m_eDeserialize = DESERIALIZE_BODY;
            } else {
                // An empty data packet... may happen
                m_eDeserialize = DESERIALIZE_FULL;
            } // else

            break;
        }
        case DESERIALIZE_BODY: {
            // Read of payload completed
            m_eDeserialize = DESERIALIZE_FULL;
            break;
        }
        case DESERIALIZE_ERROR:
        case DESERIALIZE_FULL:
        default:
            assert(false);
        } // switch

        // No error
        return true;
    }
    
    // Members
    bool m_bReliable;
    bool m_bInvalid;
    bool m_bWasSent;
    typedef enum {
        DESERIALIZE_ERROR  = 0,
        DESERIALIZE_HEADER = 1,
        DESERIALIZE_BODY   = 2,
        DESERIALIZE_FULL   = 3
    } E_DESERIALIZE;
    E_DESERIALIZE m_eDeserialize;
};

#endif // HDLCD_PACKET_DATA_H
