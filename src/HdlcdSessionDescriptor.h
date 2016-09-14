/**
 * \file      HdlcdSessionDescriptor.h
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

#ifndef HDLCD_SESSION_DESCRIPTOR_H
#define HDLCD_SESSION_DESCRIPTOR_H

#include <stdint.h>

// Session types. Select only one!
typedef enum {
    SESSION_TYPE_TRX_ALL              = 0x00,
    SESSION_TYPE_TRX_STATUS           = 0x10,
    SESSION_TYPE_RX_PAYLOAD           = 0x20,
    SESSION_TYPE_RX_HDLC              = 0x30,
    SESSION_TYPE_RX_HDLC_DISSECTED    = 0x40,

    // Bookkeeping
    SESSION_TYPE_ARITHMETIC_ENDMARKER = 0x50,
    SESSION_TYPE_MASK                 = 0xF0,
    SESSION_TYPE_UNSET                = 0xFF
} E_SESSION_TYPE;

// Multiple combination of flags are possible
typedef enum {
    SESSION_FLAGS_NONE                = 0x00,
    SESSION_FLAGS_DELIVER_RCVD        = 0x01,
    SESSION_FLAGS_DELIVER_SENT        = 0x02,
    SESSION_FLAGS_DELIVER_INVALIDS    = 0x04,
    SESSION_FLAGS_RESERVED            = 0x08,
    
    // Bookkeeping
    SESSION_FLAGS_MASK                = 0x0F
} E_SESSION_FLAGS;

class HdlcdSessionDescriptor {
public:   
    // CTORs
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
    
    explicit HdlcdSessionDescriptor(uint8_t a_ServiceAccessPointSpecifier): m_ServiceAccessPointSpecifier(a_ServiceAccessPointSpecifier) {
        // Checks
        if ((m_ServiceAccessPointSpecifier & SESSION_TYPE_MASK) >= SESSION_TYPE_ARITHMETIC_ENDMARKER) {
            // Invalid session type
            m_ServiceAccessPointSpecifier = SESSION_TYPE_UNSET;
        } // if
    }
    
    // Getter
    operator uint8_t() const {
        return m_ServiceAccessPointSpecifier;
    }
    
    E_SESSION_TYPE GetSessionType() const {
        return E_SESSION_TYPE(m_ServiceAccessPointSpecifier & SESSION_TYPE_MASK);
    }

    bool DeliversRcvdData() const {
        return (m_ServiceAccessPointSpecifier & SESSION_FLAGS_DELIVER_RCVD);
    }

    bool DeliversSentData() const {
        return (m_ServiceAccessPointSpecifier & SESSION_FLAGS_DELIVER_SENT);
    }

    bool DeliversInvalidData() const {
        return (m_ServiceAccessPointSpecifier & SESSION_FLAGS_DELIVER_INVALIDS);
    }

private:
    uint8_t m_ServiceAccessPointSpecifier;
};

#endif // HDLCD_SESSION_DESCRIPTOR_H
