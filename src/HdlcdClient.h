/**
 * \file      HdlcdClient.h
 * \brief     This file contains the header declaration of class HdlcdClient
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

#ifndef HDLCD_CLIENT_H
#define HDLCD_CLIENT_H

#include <boost/asio.hpp>
#include <vector>
#include <string>
#include "HdlcdPacketEndpoint.h"
#include "HdlcdSessionHeader.h"
#include "HdlcdPacketData.h"
#include "HdlcdPacketCtrl.h"
#include "FrameEndpoint.h"

/*! \class HdlcdClient
 *  \brief Class HdlcdClient
 * 
 *  The main helper class to easily implement clients of the HDLCd access protocol. It implements the HDLCd access protocol
 *  and makes use of two TCP sockets: one TCP socket is dedicated to the exchange of user data, the second solely to the exchange
 *  of control packets. However, all the socket handling is performed internally and is not visible to the user of this class.
 *  
 *  This class provides an asynchronous interface, but it can also be used in a quasi-synchronous way.
 */
class HdlcdClient {
public:
    /*! \brief  The constructor of HdlcdClient objects
     * 
     *  \param  a_IOService the boost IOService object
     *  \param  a_SerialPortName the name of the serial port device
     *  \param  a_HdlcdSessionDescriptor the indentifier of the session, see "service access point"
     */
    HdlcdClient(boost::asio::io_service& a_IOService, const std::string &a_SerialPortName, HdlcdSessionDescriptor a_HdlcdSessionDescriptor):
        m_IOService(a_IOService),
        m_SerialPortName(a_SerialPortName),
        m_HdlcdSessionDescriptor(a_HdlcdSessionDescriptor),
        m_bClosed(false),
        m_TcpSocketData(a_IOService),
        m_TcpSocketCtrl(a_IOService),
        m_eTcpSocketDataState(SOCKET_STATE_ERROR),
        m_eTcpSocketCtrlState(SOCKET_STATE_ERROR) {
    }
    
    /*! \brief  Perform an asynchronous connect procedure regarding both TCP sockets
     * 
     *  \param  a_EndpointIterator the boost endpoint iteratior referring to the destination
     *  \param  a_OnConnectedCallback the callback to be called if a result is available.
     */
    void AsyncConnect(boost::asio::ip::tcp::resolver::iterator a_EndpointIterator, std::function<void(bool a_bSuccess)> a_OnConnectedCallback) {
        // Checks
        assert(a_OnConnectedCallback);
        assert(m_eTcpSocketDataState == SOCKET_STATE_ERROR); // not tried yet, or retrying
        assert(m_eTcpSocketCtrlState == SOCKET_STATE_ERROR); // not tried yet, or retrying

        // Connect the data socket
        m_OnConnectedCallback = a_OnConnectedCallback;
        m_eTcpSocketDataState = SOCKET_STATE_CONNECTING;
        boost::asio::async_connect(m_TcpSocketData, a_EndpointIterator, [this](boost::system::error_code a_ErrorCode, boost::asio::ip::tcp::resolver::iterator) {
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (a_ErrorCode) {
                OnTcpSocketDataConnected(false);
            } else {
                OnTcpSocketDataConnected(true);
            } // else
        });
        
        // Connect the control socket
        m_eTcpSocketCtrlState = SOCKET_STATE_CONNECTING;
        boost::asio::async_connect(m_TcpSocketCtrl, a_EndpointIterator, [this](boost::system::error_code a_ErrorCode, boost::asio::ip::tcp::resolver::iterator) {
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (a_ErrorCode) {
                OnTcpSocketCtrlConnected(false);
            } else {
                OnTcpSocketCtrlConnected(true);
            } // else
        });
    }

    /*! \brief  The destructor of HdlcdClient objects
     * 
     *  All open connections will automatically be closed by the destructor
     */
    ~HdlcdClient() {
        m_OnDataCallback   = nullptr;
        m_OnCtrlCallback   = nullptr;
        m_OnClosedCallback = nullptr;
        Close();
    }

    /*! \brief  Shuts all TCP connections down
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

    /*! \brief  Close the client entity
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
    
    /*! \brief  Provide a callback method to be called for received data packets
     * 
     *  Data packets are received in an asynchronous way. Use this method to specify a callback method to be called on reception of single data packets
     * 
     *  \param  a_OnDataCallback the funtion pointer to the callback method, may be an empty function pointer to remove the callback
     */
    void SetOnDataCallback(std::function<void(const HdlcdPacketData& a_PacketData)> a_OnDataCallback) {
        m_OnDataCallback = a_OnDataCallback;
    }
    
    /*! \brief  Provide a callback method to be called for received control packets
     * 
     *  Control packets are received in an asynchronous way. Use this method to specify a callback method to be called on reception of single control packets
     * 
     *  \param  a_OnCtrlCallback the funtion pointer to the callback method, may be an empty function pointer to remove the callback
     */
    void SetOnCtrlCallback(std::function<void(const HdlcdPacketCtrl& a_PacketCtrl)> a_OnCtrlCallback) {
        m_OnCtrlCallback = a_OnCtrlCallback;
    }
    
    /*! \brief  Provide a callback method to be called if this client entity is closing
     * 
     *  This method can be used to provide a function pointer callback to be called if this entoty is closing, e.g., the peer closed its endpoint
     * 
     *  \param  a_OnClosedCallback the funtion pointer to the callback method, may be an empty function pointer to remove the callback
     */
    void SetOnClosedCallback(std::function<void()> a_OnClosedCallback) {
        m_OnClosedCallback = a_OnClosedCallback;
    }
    
    /*! \brief  Send a single data packet to the peer entity
     * 
     *  Send a single data packet to the peer entity. Due to the asynchronous mode the data packet is enqueued for later transmission.
     * 
     *  \param  a_PacketData the data packet to be transmitted
     *  \param  a_OnSendDoneCallback the callback handler to be called if the provided data packet was sent (optional)
     * 
     *  \retval true the data packet was enqueued for transmission
     *  \retval false the data packet was not enqueued, e.g., the send queue was full or a problem with one of the sockets occured
     *  \return bool indicates whether the provided data packet was successfully enqueued for transmitted
     */
    bool Send(const HdlcdPacketData& a_PacketData, std::function<void()> a_OnSendDoneCallback = nullptr) {
        bool l_bRetVal = false;
        if (m_PacketEndpointData) {
            l_bRetVal = m_PacketEndpointData->Send(a_PacketData, a_OnSendDoneCallback);
        } else {
            if (a_OnSendDoneCallback) {
                m_IOService.post([a_OnSendDoneCallback](){ a_OnSendDoneCallback(); });
            } // if
        } // else
        
        return l_bRetVal;
    }

    /*! \brief  Send a single control packet to the peer entity
     * 
     *  Send a single control packet to the peer entity. Due to the asynchronous mode the control packet is enqueued for later transmission.
     * 
     *  \param  a_PacketCtrl the control packet to be transmitted
     *  \param  a_OnSendDoneCallback the callback handler to be called if the provided control packet was sent (optional)
     * 
     *  \retval true the control packet was enqueued for transmission
     *  \retval false the control packet was not enqueued, e.g., the send queue was full or a problem with one of the sockets occured
     *  \return bool indicates whether the provided control packet was successfully enqueued for transmitted
     */
    bool Send(const HdlcdPacketCtrl& a_PacketCtrl, std::function<void()> a_OnSendDoneCallback = nullptr) {
        bool l_bRetVal = false;
        if (m_PacketEndpointCtrl) {
            l_bRetVal= m_PacketEndpointCtrl->Send(a_PacketCtrl, a_OnSendDoneCallback);
        } else {
            if (a_OnSendDoneCallback) {
                m_IOService.post([a_OnSendDoneCallback](){ a_OnSendDoneCallback(); });
            } // if
        } // else

        return l_bRetVal;
    }
    
private:
    /*! \brief  Indicate that the data socket was established or that an error occured
     * 
     *  Internal helper: indicate that the data socket was established or that an error occured
     * 
     *  \param  l_bSuccess to indicate whether the data socket was successfully connected or not
     */
    void OnTcpSocketDataConnected(bool l_bSuccess) {
        // Checks
        assert(m_eTcpSocketDataState == SOCKET_STATE_CONNECTING);
        if (l_bSuccess) {
            m_eTcpSocketDataState = SOCKET_STATE_CONNECTED;
        } else {
            m_eTcpSocketDataState = SOCKET_STATE_ERROR;
	} // else
        
        OnTcpSocketConnected();
    }

    /*! \brief  Indicate that the control socket was established or that an error occured
     * 
     *  Internal helper: indicate that the control socket was established or that an error occured
     * 
     *  \param  l_bSuccess to indicate whether the control socket was successfully connected or not
     */
    void OnTcpSocketCtrlConnected(bool l_bSuccess) {
        // Checks
        assert(m_eTcpSocketCtrlState == SOCKET_STATE_CONNECTING);
        if (l_bSuccess) {
            m_eTcpSocketCtrlState = SOCKET_STATE_CONNECTED;
        } else {
            m_eTcpSocketCtrlState = SOCKET_STATE_ERROR;
	} // else
        
        OnTcpSocketConnected();
    }
    
    /*! \brief  Indicate that one of both sockets were connetced or that an error happened
     * 
     *  Internal helper: if a status regarding both sockets is available trigger the callback and maybe start subsequent activity
     */
    void OnTcpSocketConnected() {
        // Checks
        if ((m_eTcpSocketDataState == SOCKET_STATE_CONNECTING) && (m_eTcpSocketCtrlState == SOCKET_STATE_CONNECTING)) {
            // Invalid state
            assert(false);
            return;
        } // if
        
        if ((m_eTcpSocketDataState == SOCKET_STATE_CONNECTED) && (m_eTcpSocketCtrlState == SOCKET_STATE_CONNECTED)) {
            // Success!
            // Create and start the packet endpoint for the exchange of user data packets
            m_PacketEndpointData = std::make_shared<HdlcdPacketEndpoint>(m_IOService, std::make_shared<FrameEndpoint>(m_IOService, m_TcpSocketData));
            m_PacketEndpointData->SetOnDataCallback([this](std::shared_ptr<const HdlcdPacketData> a_PacketData){ return OnDataReceived(a_PacketData); });
            m_PacketEndpointData->SetOnClosedCallback([this](){ OnClosed(); });
            m_PacketEndpointData->Start();
            m_PacketEndpointData->Send(HdlcdSessionHeader::Create(m_HdlcdSessionDescriptor, m_SerialPortName));
            
            // Create and start the packet endpoint for the exchange of control packets
            m_PacketEndpointCtrl = std::make_shared<HdlcdPacketEndpoint>(m_IOService, std::make_shared<FrameEndpoint>(m_IOService, m_TcpSocketCtrl));
            m_PacketEndpointCtrl->SetOnCtrlCallback([this](const HdlcdPacketCtrl& a_PacketCtrl){ return OnCtrlReceived(a_PacketCtrl); });
            m_PacketEndpointCtrl->SetOnClosedCallback([this](){ OnClosed(); });
            m_PacketEndpointCtrl->Start();
            m_PacketEndpointCtrl->Send(HdlcdSessionHeader::Create(HdlcdSessionDescriptor(SESSION_TYPE_TRX_STATUS, SESSION_FLAGS_NONE), m_SerialPortName));
            m_OnConnectedCallback(true);
            return;
        } // if
        
        if ((m_eTcpSocketDataState == SOCKET_STATE_CONNECTING) || (m_eTcpSocketCtrlState == SOCKET_STATE_CONNECTING)) {
            // Needs to wait for the second socket
            return;
        } // if
        
        
        if (m_eTcpSocketDataState == SOCKET_STATE_CONNECTED) {
            // The control socket failed after the data socket succeeded
            assert(m_eTcpSocketCtrlState == SOCKET_STATE_ERROR);
            m_eTcpSocketDataState = SOCKET_STATE_ERROR;
            m_TcpSocketData.close();
            m_OnConnectedCallback(false);
            return;
        } // else if
        
        if (m_eTcpSocketCtrlState == SOCKET_STATE_CONNECTED) {
            // The data socket failed after the control socket succeeded
            assert(m_eTcpSocketDataState == SOCKET_STATE_ERROR);
            m_eTcpSocketCtrlState = SOCKET_STATE_ERROR;
            m_TcpSocketCtrl.close();
            m_OnConnectedCallback(false);
            return;
        } // else if
        
        // Both sockets failed
        assert(m_eTcpSocketDataState == SOCKET_STATE_ERROR);
        assert(m_eTcpSocketCtrlState == SOCKET_STATE_ERROR);
        m_OnConnectedCallback(false);
        return;
    }
    
    /*! \brief  Internal callback method to be called on reception of data packets
     * 
     *  This is an internal callback method to be called on reception of data packets
     * 
     *  \param  a_PacketData the received data packet
     * 
     *  \retval true demand for delivey of subsequent packets
     *  \retval false no subsequent packets must be delivered before the next explicit poll
     *  \return bool indicates whether the receiver should be stalled
     */
    bool OnDataReceived(std::shared_ptr<const HdlcdPacketData> a_PacketData) {
        if (m_OnDataCallback) {
            m_OnDataCallback(*(a_PacketData.get()));
        } // if
        
        /// \todo Implement asynchronous behavior: the receiver must be stalled
        return true; // Do not stall the receiver
    }

    /*! \brief  Internal callback method to be called on reception of control packets
     * 
     *  This is an internal callback method to be called on reception of control packets
     * 
     *  \param  a_PacketCtrl the received data packet
     */
    void OnCtrlReceived(const HdlcdPacketCtrl& a_PacketCtrl) {
        if (m_OnCtrlCallback) {
            m_OnCtrlCallback(a_PacketCtrl);
        } // if
    }

    /*! \brief  Internal callback method to be called on close of one of the TCP sockets
     * 
     *  This is an internal callback method to be called on close of one of the TCP sockets
     */
    void OnClosed() {
        Close();
    }
    
    // Members
    boost::asio::io_service& m_IOService; //!< The boost IOService object
    const std::string m_SerialPortName;   //!< The name of the serial port to connect to a device
    const HdlcdSessionDescriptor m_HdlcdSessionDescriptor; //!< The service access point specifier regarding the protocol specification
    bool m_bClosed; //!< Indicates whether the HDLCd access protocol entity has already been closed
    
    std::function<void(bool a_bSuccess)> m_OnConnectedCallback;
    boost::asio::ip::tcp::socket m_TcpSocketData; //!< The TCP socket dedicated to user data
    boost::asio::ip::tcp::socket m_TcpSocketCtrl; //!< The TCP socket dedicated to control data
    typedef enum {
        SOCKET_STATE_ERROR      = 0,
        SOCKET_STATE_CONNECTING = 1,
        SOCKET_STATE_CONNECTED  = 2,
    } E_SOCKET_STATE;
    E_SOCKET_STATE m_eTcpSocketDataState; //!< The state of the TCP data socket
    E_SOCKET_STATE m_eTcpSocketCtrlState; //!< The state of the TCP control socket
    
    std::shared_ptr<HdlcdPacketEndpoint> m_PacketEndpointData; //!< The packet endpoint class responsible for the connected data socket
    std::shared_ptr<HdlcdPacketEndpoint> m_PacketEndpointCtrl; //!< The packet endpoint class responsible for the connected control socket
    
    // All possible callbacks for a user of this class
    std::function<void(const HdlcdPacketData&)> m_OnDataCallback; //!< The callback function that is invoked on reception of a data packet
    std::function<void(const HdlcdPacketCtrl&)> m_OnCtrlCallback; //!< The callback function that is invoked on reception of a control packet
    std::function<void()> m_OnClosedCallback;  //!< The callback function that is invoked if the either this endpoint or that of the peer goes down
};

#endif // HDLCD_CLIENT_H
