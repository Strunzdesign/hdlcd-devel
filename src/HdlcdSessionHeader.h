/**
 * \file      HdlcdSessionHeader.h
 * \brief     This file contains the header declaration of class HdlcdSessionHeader
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

#ifndef HDLCD_SESSION_HEADER_H
#define HDLCD_SESSION_HEADER_H

#include "Frame.h"
#include <memory>
#include <string>
#include "HdlcdSessionDescriptor.h"

/*! \class HdlcdSessionHeader
 *  \brief Class HdlcdSessionHeader
 * 
 *  This class implements the session header as specified in the HDLCd access protocol. It inherits from
 *  the Frame class and thus allows easy exchange via FrameEndpoint entities.
 */
class HdlcdSessionHeader: public Frame {
public:
    /*! \brief  Static creator to create an object in the process of transmission
     *
     *  \param  a_HdlcdSessionDescriptor the service access point specifier octett
     *  \param  a_SerialPortName the file name of the serial port
     * 
     *  \return The created HDLCd session header object
     */
    static HdlcdSessionHeader Create(HdlcdSessionDescriptor a_HdlcdSessionDescriptor, const std::string& a_SerialPortName) {
        // Called for transmission
        HdlcdSessionHeader l_HdlcdSessionHeader;
        l_HdlcdSessionHeader.m_ServiceAccessPointSpecifier = a_HdlcdSessionDescriptor;
        l_HdlcdSessionHeader.m_SerialPortName = a_SerialPortName;
        return l_HdlcdSessionHeader;
    }

    /*! \brief  Static creator to create an object in the process of reception
     * 
     *  \return The created but empty HDLCd session header object
     */
    static std::shared_ptr<HdlcdSessionHeader> CreateDeserializedFrame() {
        // Called on reception
        auto l_HdlcdSessionHeader(std::shared_ptr<HdlcdSessionHeader>(new HdlcdSessionHeader));
        l_HdlcdSessionHeader->m_eDeserialize = DESERIALIZE_HEADER; // Next: read fixed-sized part of the session header
        l_HdlcdSessionHeader->m_BytesRemaining = 3;
        return l_HdlcdSessionHeader;
    }
    
    /*! \brief  Query the service access point specifier octett
     * 
     *  \return The service access point specifier octett
     */
    uint8_t GetServiceAccessPointSpecifier() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_ServiceAccessPointSpecifier;
    }

    /*! \brief  Query the file name of the serial port
     * 
     *  \return The file name of the serial port
     */
    const std::string& GetSerialPortName() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_SerialPortName;
    }

private:
    /*! \brief  The default constructor
     * 
     *  The default constructor is private. To create an object one has to use one of the static creator methods
     */
    HdlcdSessionHeader(): m_ServiceAccessPointSpecifier(0x00), m_eDeserialize(DESERIALIZE_FULL) {
    }

    /*! \brief  The serializer
     * 
     *  The serializer creates a buffer of bytes containing the assembled HDLCd session header ready for transmission
     * 
     *  \return The buffer of bytes containing the assembled HDLCd session header
     */
    const std::vector<unsigned char> Serialize() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        std::vector<unsigned char> l_Buffer;
        l_Buffer.emplace_back(0x00); // Version
        l_Buffer.emplace_back(m_ServiceAccessPointSpecifier);
        l_Buffer.emplace_back(m_SerialPortName.size());
        l_Buffer.insert(l_Buffer.end(), m_SerialPortName.data(), (m_SerialPortName.data() + m_SerialPortName.size()));
        return l_Buffer;
    }

    /*! \brief  The deserializer
     * 
     *  The deserializer consumes and parses all collected bytes
     * 
     *  \retval true no error occured
     *  \retval false a protocol violation was detected
     *  \return Indicates whether parsing the available bytes succeeded
     */
    bool Deserialize() {
        // All requested bytes are available
        switch (m_eDeserialize) {
        case DESERIALIZE_HEADER: {
            // Deserialize the length field
            assert(m_Buffer.size() == 3);

            // Deserialize the version field
            if (m_Buffer[0] != 0) {
                // Wrong version field
                m_eDeserialize = DESERIALIZE_ERROR;
                return false;
            } // if
            
            // Deserialize the service access point identifier and the length field of the serial port name
            m_ServiceAccessPointSpecifier = m_Buffer[1];
            m_BytesRemaining = m_Buffer[2];
            m_Buffer.clear();
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
            m_SerialPortName.append(m_Buffer.begin(), m_Buffer.end());
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
    
    // Internal members
    uint8_t m_ServiceAccessPointSpecifier; //!< The service access point specifier octett
    std::string m_SerialPortName;          //!< The file name of the serial port
    
    /*! \enum E_DESERIALIZE
     *  \brief The enum E_DESERIALIZE to specify the progress of deserialization
     */
    typedef enum {
        DESERIALIZE_ERROR  = 0,            //!< An error occured
        DESERIALIZE_HEADER = 1,            //!< Currently the header of the HDLCd session header is deserialized
        DESERIALIZE_BODY   = 2,            //!< Currently the body of the HDLCd session header is deserialized
        DESERIALIZE_FULL   = 3             //!< The HDLCd session header is complete
    } E_DESERIALIZE;
    E_DESERIALIZE m_eDeserialize;          //!< The state in the progress of deserialization
};

#endif // HDLCD_SESSION_HEADER_H
