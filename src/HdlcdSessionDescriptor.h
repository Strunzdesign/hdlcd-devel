/**
 * \file      HdlcdSessionDescriptor.h
 * \brief     This file contains the header declaration of class HdlcdSessionDescriptor
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

#ifndef HDLCD_SESSION_DESCRIPTOR_H
#define HDLCD_SESSION_DESCRIPTOR_H

#include <stdint.h>

/*! enum E_SESSION_TYPE
 *  \brief The enum E_SESSION_TYPE
 * 
 *  This enum specifies the set of available session types according to the HDLCd access protocol.
 */
typedef enum {
    SESSION_TYPE_TRX_ALL              = 0x00, //!< Payload,          data read and write, port status read and write
    SESSION_TYPE_TRX_STATUS           = 0x10, //!< Port status only, no data exchange,    port status read and write
    SESSION_TYPE_RX_PAYLOAD           = 0x20, //!< Payload Raw,      data read only,      port status read only
    SESSION_TYPE_RX_HDLC              = 0x30, //!< HDLC Raw,         data read only,      port status read only
    SESSION_TYPE_RX_HDLC_DISSECTED    = 0x40, //!< HDLC dissected,   data read only,      port status read only

    // Book keeping
    SESSION_TYPE_ARITHMETIC_ENDMARKER = 0x50, //!< The lowest invalid session type number 
    SESSION_TYPE_MASK                 = 0xF0, //!< Bit mask to query the session type
    SESSION_TYPE_UNSET                = 0xFF  //!< Invalid entry, to indicate unset state
} E_SESSION_TYPE;



/*! enum E_SESSION_FLAGS
 *  \brief The enum E_SESSION_FLAGS
 * 
 *  This enum specifies the set of available session flags according to the HDLCd access protocol.
 *  Multiple combination of flags are possible
 */
typedef enum {
    SESSION_FLAGS_NONE                = 0x00, //!< Empty list of flags
    SESSION_FLAGS_DELIVER_RCVD        = 0x01, //!< Deliver data packets sent by the device and received by the HDLCd
    SESSION_FLAGS_DELIVER_SENT        = 0x02, //!< Deliver data packets sent by the HDLCd and received by the device
    SESSION_FLAGS_DELIVER_INVALIDS    = 0x04, //!< Deliver also invalid frames with broken CRC checksum
    SESSION_FLAGS_RESERVED            = 0x08, //!< Reserved bit
    
    // Book keeping
    SESSION_FLAGS_MASK                = 0x0F  //!< Bit mask to query the session flags
} E_SESSION_FLAGS;



/*! \class HdlcdSessionDescriptor
 *  \brief Class HdlcdSessionDescriptor
 * 
 *  This is a convenience class to assemble the service access point specifier byte (SAP) of the HDLCd access protocol.
 *  The use of enums allows a human-readable specification of sessions.
 */
class HdlcdSessionDescriptor {
public:   
    /*! \brief  The constructor of HdlcdSessionDescriptor objects
     * 
     *  This constructor is used to assemble the service access point specifier as a combination of enums
     * 
     *  \param  a_eSessionType the session type enum
     *  \param  a_SessionFlags a combination of E_SESSION_FLAGS enums
     */
    HdlcdSessionDescriptor(E_SESSION_TYPE a_eSessionType, uint8_t a_SessionFlags): m_ServiceAccessPointSpecifier(a_eSessionType | a_SessionFlags) {
        // Checks
        if (((a_eSessionType & SESSION_TYPE_MASK) >= SESSION_TYPE_ARITHMETIC_ENDMARKER) ||
            (a_eSessionType & ~SESSION_TYPE_MASK)) {
            // Invalid session type
            m_ServiceAccessPointSpecifier = SESSION_TYPE_UNSET;
        } // if
        
        if (a_SessionFlags & ~SESSION_FLAGS_MASK) {
            // Invalid flags
            m_ServiceAccessPointSpecifier = SESSION_TYPE_UNSET;
        } // if
    }
    
    /*! \brief  The constructor of HdlcdSessionDescriptor objects
     * 
     *  This constructor is used to parse the service access point specifier using the assembled octett
     * 
     *  \param  a_ServiceAccessPointSpecifier the service access point specifier octett
     */
    explicit HdlcdSessionDescriptor(uint8_t a_ServiceAccessPointSpecifier): m_ServiceAccessPointSpecifier(a_ServiceAccessPointSpecifier) {
        // Checks
        if ((m_ServiceAccessPointSpecifier & SESSION_TYPE_MASK) >= SESSION_TYPE_ARITHMETIC_ENDMARKER) {
            // Invalid session type
            m_ServiceAccessPointSpecifier = SESSION_TYPE_UNSET;
        } // if
    }
    
    /*! \brief  Query the service access point specifier octett
     * 
     *  \return uint8_t the service access point specifier octett
     */
    operator uint8_t() const {
        return m_ServiceAccessPointSpecifier;
    }

    /*! \brief  Query the session type
     * 
     *  \return E_SESSION_TYPE the session type enum
     */
    E_SESSION_TYPE GetSessionType() const {
        return E_SESSION_TYPE(m_ServiceAccessPointSpecifier & SESSION_TYPE_MASK);
    }

    /*! \brief  Query whether data packets sent by a device to the HDLCd should be delivered
     * 
     *  \retval true data packets received from the HDLCd should be delivered
     *  \retval false data packets received from the HDLCd should be ignored
     *  \return bool indicates whether data packets received from the HDLCd should be delivered or ignored
     */
    bool DeliversRcvdData() const {
        return (m_ServiceAccessPointSpecifier & SESSION_FLAGS_DELIVER_RCVD);
    }

    /*! \brief  Query whether data packets sent by the HDLCd to a device should be delivered
     * 
     *  \retval true data packets sent to a device should be delivered
     *  \retval false data packets sent to a device should be ignored
     *  \return bool indicates whether data packets sent to a device should be delivered or ignored
     */
    bool DeliversSentData() const {
        return (m_ServiceAccessPointSpecifier & SESSION_FLAGS_DELIVER_SENT);
    }

    /*! \brief  Query whether invalid data packets with a broken CRC checksum should be delivered
     * 
     *  \retval true invalid data packets with a broken CRC checksum should be delivered
     *  \retval false invalid data packets with a broken CRC checksum should be ignored
     *  \return bool indicates whether invalid data packets with a broken CRC checksum should be delivered or ignored
     */
    bool DeliversInvalidData() const {
        return (m_ServiceAccessPointSpecifier & SESSION_FLAGS_DELIVER_INVALIDS);
    }

private:
    // Internal members
    uint8_t m_ServiceAccessPointSpecifier; //!< The service access point specifier octett
};

#endif // HDLCD_SESSION_DESCRIPTOR_H
