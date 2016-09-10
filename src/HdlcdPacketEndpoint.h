/**
 * \file      HdlcdPacketEndpoint.h
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

#ifndef HDLCD_PACKET_ENDPOINT_H
#define HDLCD_PACKET_ENDPOINT_H

#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include "FrameEndpoint.h"
#include "HdlcdPacketData.h"
#include "HdlcdPacketCtrl.h"

class HdlcdPacketEndpoint: public std::enable_shared_from_this<HdlcdPacketEndpoint> {
public:
    HdlcdPacketEndpoint(boost::asio::io_service& a_IOService, boost::asio::ip::tcp::socket& a_TCPSocket): m_IOService(a_IOService), m_KeepAliveTimer(a_IOService) {
        m_bStarted = false;
        m_bStopped = false;
        m_FrameEndpoint = std::make_shared<FrameEndpoint>(a_IOService, a_TCPSocket, 0xF0);
        m_FrameEndpoint->RegisterFrameFactory(HDLCD_PACKET_DATA, []()->std::shared_ptr<Frame>{ return HdlcdPacketData::CreateDeserializedPacket(); });
        m_FrameEndpoint->RegisterFrameFactory(HDLCD_PACKET_CTRL, []()->std::shared_ptr<Frame>{ return HdlcdPacketCtrl::CreateDeserializedPacket(); });
        m_FrameEndpoint->SetOnFrameCallback([this](std::shared_ptr<Frame> a_Frame)->bool{ return OnFrame(a_Frame); });
        m_FrameEndpoint->SetOnClosedCallback ([this](){ OnClosed(); });
    }
    
    ~HdlcdPacketEndpoint() {
        m_OnDataCallback = NULL;
        m_OnCtrlCallback = NULL;
        m_OnClosedCallback = NULL;
        Close();
    }
            
    // Callback methods
    void SetOnDataCallback(std::function<bool(std::shared_ptr<const HdlcdPacketData> a_PacketData)> a_OnDataCallback) {
        m_OnDataCallback = a_OnDataCallback;
    }
    
    void SetOnCtrlCallback(std::function<void(const HdlcdPacketCtrl& a_PacketCtrl)> a_OnCtrlCallback) {
        m_OnCtrlCallback = a_OnCtrlCallback;
    }
    
    void SetOnClosedCallback(std::function<void()> a_OnClosedCallback) {
        m_OnClosedCallback = a_OnClosedCallback;
    }
    
    bool Send(const HdlcdPacket& a_HdlcdPacket, std::function<void()> a_OnSendDoneCallback = std::function<void()>()) {
        return (m_FrameEndpoint->SendFrame(a_HdlcdPacket, a_OnSendDoneCallback));
    }
    
    void Start() {
        assert(m_bStarted == false);
        assert(m_bStopped == false);
        m_bStarted = true;
        m_FrameEndpoint->Start();
        StartKeepAliveTimer();
    }

    void Shutdown() {
        m_FrameEndpoint->Shutdown();
        m_KeepAliveTimer.cancel();
    }

    void Close() {
        if (m_bStarted && (!m_bStopped)) {
            m_bStopped = true;
            m_KeepAliveTimer.cancel();
            m_FrameEndpoint->Close();
            if (m_OnClosedCallback) {
                m_OnClosedCallback();
            } // if
        } // if
    }
    
    void TriggerNextDataPacket() {
        m_FrameEndpoint->TriggerNextFrame();
    }

private:
    void StartKeepAliveTimer() {
        auto self(shared_from_this());
        m_KeepAliveTimer.expires_from_now(boost::posix_time::minutes(1));
        m_KeepAliveTimer.async_wait([this, self](const boost::system::error_code& a_ErrorCode) {
            if (!a_ErrorCode) {
                // Periodically send keep alive packets
                m_FrameEndpoint->SendFrame(HdlcdPacketCtrl::CreateKeepAliveRequest());
                StartKeepAliveTimer();
            } // if
        }); // async_wait
    }

private:
    // Members
    void OnClosed() {
        Close();
    }
    
    bool OnFrame(std::shared_ptr<Frame> a_Frame) {
        // Reception completed, deliver the packet
        bool l_bReceiving = true;
        auto l_PacketData = std::dynamic_pointer_cast<HdlcdPacketData>(a_Frame);
        if (l_PacketData) {
            if (m_OnDataCallback) {
                // Deliver the data packet but stall the receiver
                l_bReceiving = m_OnDataCallback(l_PacketData);
            } // if
        } else {
            auto l_PacketCtrl = std::dynamic_pointer_cast<HdlcdPacketCtrl>(a_Frame);
            if (l_PacketCtrl) {
                bool l_bDeliver = true;
                if (l_PacketCtrl->GetPacketType() == HdlcdPacketCtrl::CTRL_TYPE_KEEP_ALIVE) {
                    // This is a keep alive packet, simply drop it.
                    l_bDeliver = false;
                } // if
                
                if ((l_bDeliver) && (m_OnCtrlCallback)) {
                    m_OnCtrlCallback(*(l_PacketCtrl.get()));
                } // if
            } else {
                assert(false);
            } // else
        } // else
        
        return l_bReceiving;
    }
    
    boost::asio::io_service& m_IOService;
    std::shared_ptr<FrameEndpoint> m_FrameEndpoint;
    
    
    // All possible callbacks for a user of this class
    std::function<bool(std::shared_ptr<const HdlcdPacketData> a_PacketData)> m_OnDataCallback;
    std::function<void(const HdlcdPacketCtrl& a_PacketCtrl)> m_OnCtrlCallback;
    std::function<void()> m_OnClosedCallback;
    
    bool m_bStarted;
    bool m_bStopped;
    
    // The keep alive timer
    boost::asio::deadline_timer m_KeepAliveTimer;
};

#endif // HDLCD_PACKET_ENDPOINT_H
