/**
 * \file      HdlcdClient.h
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

#ifndef HDLCD_CLIENT_H
#define HDLCD_CLIENT_H

#include <boost/asio.hpp>
#include <vector>
#include <string>
#include "HdlcdPacketEndpoint.h"
#include "HdlcdSessionHeader.h"
#include "HdlcdPacketData.h"
#include "HdlcdPacketCtrl.h"

/*! \class HdlcdClient
 *  \brief Class HdlcdClient
 * 
 *  The main helper class to easily implement clients of the HDLCd access protocol
 */
class HdlcdClient {
public:
    /*! \brief The constructor of HdlcdClient objects
     * 
     *  The connection is established on instantiation (RAII)
     * 
     *  \param a_IOService the boost IOService object
     *  \param a_EndpointIterator the boost endpoint iteratior referring to the destination
     *  \param a_SerialPortName the name of the serial port device
     *  \param a_ServiceAccessPointSpecifier the numerical indentifier of the service access protocol
     */
    HdlcdClient(boost::asio::io_service& a_IOService, boost::asio::ip::tcp::resolver::iterator a_EndpointIterator, const std::string &a_SerialPortName, uint16_t a_ServiceAccessPointSpecifier):
        m_IOService(a_IOService),
        m_bClosed(false),
        m_TcpSocketData(a_IOService),
        m_TcpSocketCtrl(a_IOService) {
        // Connect the data socket
        boost::asio::async_connect(m_TcpSocketData, a_EndpointIterator,
                                   [this, a_SerialPortName, a_ServiceAccessPointSpecifier](boost::system::error_code a_ErrorCode, boost::asio::ip::tcp::resolver::iterator) {
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (a_ErrorCode) {
                std::cerr << "Unable to connect to the HDLC daemon (data socket)" << std::endl;
                Close();
            } else {
                // Create and start the packet endpoint for the exchange of user data packets
                m_PacketEndpointData = std::make_shared<HdlcdPacketEndpoint>(m_IOService, m_TcpSocketData);
                m_PacketEndpointData->SetOnDataCallback([this](std::shared_ptr<const HdlcdPacketData> a_PacketData){ return OnDataReceived(a_PacketData); });
                m_PacketEndpointData->SetOnClosedCallback([this](){ OnClosed(); });
                m_PacketEndpointData->Start();
                m_PacketEndpointData->Send(HdlcdSessionHeader::Create(a_ServiceAccessPointSpecifier, a_SerialPortName));
            } // else
        });
        
        // Connect the control socket
        boost::asio::async_connect(m_TcpSocketCtrl, a_EndpointIterator,
                                   [this, a_SerialPortName](boost::system::error_code a_ErrorCode, boost::asio::ip::tcp::resolver::iterator) {
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (a_ErrorCode) {
                std::cerr << "Unable to connect to the HDLC daemon (control socket)" << std::endl;
                Close();
            } else {
                // Create and start the packet endpoint for the exchange of control packets
                m_PacketEndpointCtrl = std::make_shared<HdlcdPacketEndpoint>(m_IOService, m_TcpSocketCtrl);
                m_PacketEndpointCtrl->SetOnCtrlCallback([this](const HdlcdPacketCtrl& a_PacketCtrl){ return OnCtrlReceived(a_PacketCtrl); });
                m_PacketEndpointCtrl->SetOnClosedCallback([this](){ OnClosed(); });
                m_PacketEndpointCtrl->Start();
                m_PacketEndpointCtrl->Send(HdlcdSessionHeader::Create(0x10, a_SerialPortName)); // 0x10: control flow only
            } // else
        });
    }

    /*! \brief The destructor of HdlcdClient objects
     * 
     *  All open connections will automatically be closed by the destructor
     */
    ~HdlcdClient() {
        m_OnDataCallback = NULL;
        m_OnCtrlCallback = NULL;
        m_OnClosedCallback = NULL;
        Close();
    }

    /*! \brief Shuts all TCP connections down
     * 
     *  Initiates a shutdown procedure for correct teardown of all TCP connections
     */
    void Shutdown() {
        if (m_PacketEndpointData) {
            m_PacketEndpointData->Shutdown();
        } // if
        
        if (m_PacketEndpointCtrl) {
            m_PacketEndpointCtrl->Shutdown();
        } // if
    }

    /*! \brief Close the client entity
     * 
     *  Close the client entity and all its TCP connections
     */
    void Close() {
        if (m_bClosed == false) {
            m_bClosed = true;
            if (m_PacketEndpointData) {
                m_PacketEndpointData->Close();
                m_PacketEndpointData.reset();
            } else {
                m_TcpSocketData.cancel();
                m_TcpSocketData.close();
            } // else
            
            if (m_PacketEndpointCtrl) {
                m_PacketEndpointCtrl->Close();
                m_PacketEndpointCtrl.reset();
            } else {
                m_TcpSocketCtrl.cancel();
                m_TcpSocketCtrl.close();
            } // else
            
            if (m_OnClosedCallback) {
                m_OnClosedCallback();
            } // if
        } // if
    }
    
    /*! \brief Provide a callback method to be called for received data packets
     * 
     *  Data packets are received in an asynchronous way. Use this method to specify a callback method to be called on reception of single data packets
     * 
     *  \param a_OnDataCallback the funtion pointer to the callback method, may be an empty function pointer to remove the callback
     */
    void SetOnDataCallback(std::function<void(const HdlcdPacketData& a_PacketData)> a_OnDataCallback) {
        m_OnDataCallback = a_OnDataCallback;
    }
    
    /*! \brief Provide a callback method to be called for received control packets
     * 
     *  Control packets are received in an asynchronous way. Use this method to specify a callback method to be called on reception of single control packets
     * 
     *  \param a_OnCtrlCallback the funtion pointer to the callback method, may be an empty function pointer to remove the callback
     */
    void SetOnCtrlCallback(std::function<void(const HdlcdPacketCtrl& a_PacketCtrl)> a_OnCtrlCallback) {
        m_OnCtrlCallback = a_OnCtrlCallback;
    }
    
    /*! \brief Provide a callback method to be called if this client entity is closing
     * 
     *  This method can be used to provide a function pointer callback to be called if this entoty is closing, e.g., the peer closed its endpoint
     * 
     *  \param a_OnClosedCallback the funtion pointer to the callback method, may be an empty function pointer to remove the callback
     */
    void SetOnClosedCallback(std::function<void()> a_OnClosedCallback) {
        m_OnClosedCallback = a_OnClosedCallback;
    }
    
    /*! \brief Send a single data packet to the peer entity
     * 
     *  Send a single data packet to the peer entity
     * 
     *  \param a_PacketData the data packet to be transmitted
     *  \param a_OnSendDoneCallback the callback handler to be called if the provided data packet was sent (optional)
     */
    bool Send(const HdlcdPacketData& a_PacketData, std::function<void()> a_OnSendDoneCallback = std::function<void()>()) {
        bool l_bRetVal = false;
        if (m_PacketEndpointData) {
            l_bRetVal = m_PacketEndpointData->Send(a_PacketData, a_OnSendDoneCallback);
        } else {
            m_IOService.post([a_OnSendDoneCallback](){ a_OnSendDoneCallback(); });
        } // else
        
        return l_bRetVal;
    }

    /*! \brief Send a single control packet to the peer entity
     * 
     *  Send a single control packet to the peer entity
     * 
     *  \param a_PacketCtrl the control packet to be transmitted
     *  \param a_OnSendDoneCallback the callback handler to be called if the provided control packet was sent (optional)
     */
    bool Send(const HdlcdPacketCtrl& a_PacketCtrl, std::function<void()> a_OnSendDoneCallback = std::function<void()>()) {
        bool l_bRetVal = false;
        if (m_PacketEndpointCtrl) {
            l_bRetVal= m_PacketEndpointCtrl->Send(a_PacketCtrl, a_OnSendDoneCallback);
        } else {
            m_IOService.post([a_OnSendDoneCallback](){ a_OnSendDoneCallback(); });
        } // else

        return l_bRetVal;
    }
    
private:    
    /*! \brief Internal callback method to be called on reception of data packets
     * 
     *  This is an internal callback method to be called on reception of data packets
     * 
     *  \param a_PacketData the received data packet
     */
    bool OnDataReceived(std::shared_ptr<const HdlcdPacketData> a_PacketData) {
        if (m_OnDataCallback) {
            m_OnDataCallback(*(a_PacketData.get()));
        } // if
        
        return true; // Do not stall the receiver
    }

    /*! \brief Internal callback method to be called on reception of control packets
     * 
     *  This is an internal callback method to be called on reception of control packets
     * 
     *  \param a_PacketCtrl the received data packet
     */
    void OnCtrlReceived(const HdlcdPacketCtrl& a_PacketCtrl) {
        if (m_OnCtrlCallback) {
            m_OnCtrlCallback(a_PacketCtrl);
        } // if
    }

    /*! \brief Internal callback method to be called on close of one of the TCP sockets
     * 
     *  This is an internal callback method to be called on close of one of the TCP sockets
     */
    void OnClosed() {
        Close();
    }
    
    // Members
    boost::asio::io_service& m_IOService;
    bool m_bClosed; //!< Indicates whether the HDLCd access protocol entity has already been closed
    boost::asio::ip::tcp::socket m_TcpSocketData; //!< The TCP socket dedicated to user data
    boost::asio::ip::tcp::socket m_TcpSocketCtrl; //!< The TCP socket dedicated to control data
    
    std::shared_ptr<HdlcdPacketEndpoint> m_PacketEndpointData; //!< The packet endpoint class responsible for the connected data socket
    std::shared_ptr<HdlcdPacketEndpoint> m_PacketEndpointCtrl; //!< The packet endpoint class responsible for the connected control socket
    
    // All possible callbacks for a user of this class
    std::function<void(const HdlcdPacketData&)> m_OnDataCallback; //!< The callback function that is invoked on reception of a data packet
    std::function<void(const HdlcdPacketCtrl&)> m_OnCtrlCallback; //!< The callback function that is invoked on reception of a control packet
    std::function<void()> m_OnClosedCallback;  //!< The callback function that is invoked if the either this endpoint or that of the peer goes down
};

#endif // HDLCD_CLIENT_H
