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
        l_PacketData.m_Payload = std::move(a_Payload);
        l_PacketData.m_bReliable = a_bReliable;
        l_PacketData.m_bInvalid = a_bInvalid;
        l_PacketData.m_bWasSent = a_bWasSent;
        return l_PacketData;
    }

    static std::shared_ptr<HdlcdPacketData> CreateDeserializedPacket(unsigned char a_Type) {
        // Called on reception: evaluate type field
        auto l_PacketData(std::shared_ptr<HdlcdPacketData>(new HdlcdPacketData));
        //bool l_bReserved = (a_Type & 0x08); // TODO: abort if set
        l_PacketData->m_bReliable = (a_Type & 0x04);
        l_PacketData->m_bInvalid  = (a_Type & 0x02);
        l_PacketData->m_bWasSent  = (a_Type & 0x01);
        l_PacketData->m_eDeserialize = DESERIALIZE_SIZE;
        l_PacketData->m_BytesRemaining = 2;
        return l_PacketData;
    }
    
    const std::vector<unsigned char>& GetData() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_Payload;
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
    HdlcdPacketData() {
        m_bReliable = false;
        m_bInvalid = false;
        m_bWasSent = false;
        m_eDeserialize = DESERIALIZE_FULL;
    }
    
    // Internal helpers
    E_HDLCD_PACKET GetHdlcdPacketType() const { return HDLCD_PACKET_DATA; }

    // Serializer and deserializer
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
        l_Buffer.emplace_back((m_Payload.size() >> 8) & 0x00FF);
        l_Buffer.emplace_back((m_Payload.size() >> 0) & 0x00FF);
        
        // Add payload
        l_Buffer.insert(l_Buffer.end(), m_Payload.begin(), m_Payload.end());
        return l_Buffer;
    }
    
    bool BytesReceived(const unsigned char *a_ReadBuffer, size_t a_BytesRead) {
        if (Frame::BytesReceived(a_ReadBuffer, a_BytesRead)) {
            // Subsequent bytes are required
            return true; // no error (yet)
        } // if
        
        // All requested bytes are available
        switch (m_eDeserialize) {
        case DESERIALIZE_SIZE: {
            // Deserialize the length field
            assert(m_Payload.size() == 2);
            m_BytesRemaining = ntohs(*(reinterpret_cast<const uint16_t*>(&m_Payload[0])));
            m_Payload.clear();
            if (m_BytesRemaining) {
                m_eDeserialize = DESERIALIZE_DATA;
            } else {
                // An empty data packet... may happen
                m_eDeserialize = DESERIALIZE_FULL;
            } // else

            break;
        }
        case DESERIALIZE_DATA: {
            // Read of payload completed
            m_eDeserialize = DESERIALIZE_FULL;
            break;
        }
        case DESERIALIZE_FULL:
        default:
            assert(false);
        } // switch
        
        // No error, maybe subsequent bytes are required
        return true;
    }
    
    // Members
    bool m_bReliable;
    bool m_bInvalid;
    bool m_bWasSent;
    typedef enum {
        DESERIALIZE_ERROR = 0,
        DESERIALIZE_SIZE  = 1,
        DESERIALIZE_DATA  = 2,
        DESERIALIZE_FULL  = 3
    } E_DESERIALIZE;
    E_DESERIALIZE m_eDeserialize;
};

#endif // HDLCD_PACKET_DATA_H
