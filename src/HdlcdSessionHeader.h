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

#ifndef HDLCD_SESSION_HEADER_H
#define HDLCD_SESSION_HEADER_H

#include "Frame.h"
#include <memory>
#include <string>

class HdlcdSessionHeader: public Frame {
public:
    static HdlcdSessionHeader Create(uint8_t a_ServiceAccessPointSpecifier, const std::string& a_SerialPortName) {
        // Called for transmission
        HdlcdSessionHeader l_HdlcdSessionHeader;
        l_HdlcdSessionHeader.m_ServiceAccessPointSpecifier = a_ServiceAccessPointSpecifier;
        l_HdlcdSessionHeader.m_SerialPortName = a_SerialPortName;
        return l_HdlcdSessionHeader;
    }

    static std::shared_ptr<HdlcdSessionHeader> CreateDeserializedPacket() {
        // Called on reception: evaluate type field
        auto l_HdlcdSessionHeader(std::shared_ptr<HdlcdSessionHeader>(new HdlcdSessionHeader));
        l_HdlcdSessionHeader->m_eDeserialize = DESERIALIZE_HEADER; // Next: read fixed-sized part of the session header
        l_HdlcdSessionHeader->m_BytesRemaining = 3;
        return l_HdlcdSessionHeader;
    }
    
    // Getter
    uint8_t GetServiceAccessPointSpecifier() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_ServiceAccessPointSpecifier;
    }

    const std::string& GetSerialPortName() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_SerialPortName;
    }

private:
    // Private CTOR
    HdlcdSessionHeader(): m_ServiceAccessPointSpecifier(0x00), m_eDeserialize(DESERIALIZE_FULL) {
    }

    // Serializer and deserializer
    const std::vector<unsigned char> Serialize() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        std::vector<unsigned char> l_Buffer;
        l_Buffer.emplace_back(0x00); // Version
        l_Buffer.emplace_back(m_ServiceAccessPointSpecifier);
        l_Buffer.emplace_back(m_SerialPortName.size());
        l_Buffer.insert(l_Buffer.end(), m_SerialPortName.data(), (m_SerialPortName.data() + m_SerialPortName.size()));
        return l_Buffer;
    }

    bool ParseBytes(const unsigned char *a_ReadBuffer, size_t &a_ReadBufferOffset, size_t &a_BytesAvailable) {
        if (Frame::ParseBytes(a_ReadBuffer, a_ReadBufferOffset, a_BytesAvailable)) {
            // Subsequent bytes are required
            return true; // no error (yet)
        } // if
        
        // All requested bytes are available
        switch (m_eDeserialize) {
        case DESERIALIZE_HEADER: {
            // Deserialize the length field
            assert(m_Payload.size() == 3);

            // Deserialize the version field
            if (m_Payload[0] != 0) {
                // Wrong version field
                m_eDeserialize = DESERIALIZE_ERROR;
                return false;
            } // if
            
            // Deserialize the service access point identifier and the length field of the serial port name
            m_ServiceAccessPointSpecifier = m_Payload[1];
            m_BytesRemaining = m_Payload[2];
            m_Payload.clear();
            if (m_BytesRemaining) {
                m_eDeserialize = DESERIALIZE_BODY;
            } else {
                // An empty serial port specifier... may happen
                m_eDeserialize = DESERIALIZE_FULL;
            } // else

            break;
        }
        case DESERIALIZE_BODY: {
            // Read of payload completed
            m_SerialPortName.append(m_Payload.begin(), m_Payload.end());
            m_eDeserialize = DESERIALIZE_FULL;
            break;
        }
        case DESERIALIZE_ERROR:
        case DESERIALIZE_FULL:
        default:
            assert(false);
        } // switch
        
        // Maybe subsequent bytes are required?
        if ((m_BytesRemaining) && (a_BytesAvailable)) {
            return (this->ParseBytes(a_ReadBuffer, a_ReadBufferOffset, a_BytesAvailable));
        } else {        
            // No error, but maybe subsequent bytes are still required
            return true;
        } // else
    }
    
    // Members
    uint8_t m_ServiceAccessPointSpecifier;
    std::string m_SerialPortName;
    typedef enum {
        DESERIALIZE_ERROR  = 0,
        DESERIALIZE_HEADER = 1,
        DESERIALIZE_BODY   = 2,
        DESERIALIZE_FULL   = 3
    } E_DESERIALIZE;
    E_DESERIALIZE m_eDeserialize;
};

#endif // HDLCD_SESSION_HEADER_H
