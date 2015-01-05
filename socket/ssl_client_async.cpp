//
//  ssl_client_async.cpp
//  my_push_server
//
//  Created by luoning on 14-11-14.
//  Copyright (c) 2014年 luoning. All rights reserved.
//

#include "ssl_client_async.h"
#include "socket_io_define.h"
#include "io_loop.h"
CSSLClientAsync::CSSLClientAsync(CIOLoop* pIO) : CTCPClientAsync(pIO)
{
    m_ctx = NULL;
    m_ssl = NULL;
}

CSSLClientAsync::~CSSLClientAsync()
{
    Close();
    UnInitSSL();
}

BOOL CSSLClientAsync::InitSSL(const char* cert_file, const char* key_file, const char* key_password)
{
    BOOL bRet = FALSE;
    m_ctx = SSL_CTX_new(SSLv23_client_method());
    if (m_ctx)
    {
        if (cert_file)
        {
            if (!SSL_CTX_use_certificate_file(GetSSLCTX(), cert_file, SSL_FILETYPE_PEM))
            {
                SOCKET_IO_ERROR("init ssl: use certificate file failed.");
                SSL_CTX_free(GetSSLCTX());
                m_ctx = NULL;
                return bRet;
            }
        }
        
        SSL_CTX_set_default_passwd_cb_userdata(GetSSLCTX(), (void*)key_password);

        if (key_file)
        {
            if (!SSL_CTX_use_PrivateKey_file(GetSSLCTX(), key_file, SSL_FILETYPE_PEM))
            {
                SOCKET_IO_ERROR("init ssl: use private key file failed.");
                SSL_CTX_free(GetSSLCTX());
                m_ctx = NULL;
                return bRet;
            }
        }
        

        if (!SSL_CTX_check_private_key(GetSSLCTX()))
        {
            SOCKET_IO_ERROR("init ssl: check private key file failed.");
            SSL_CTX_free(GetSSLCTX());
            m_ctx = NULL;
            return bRet;
        }
        
        m_ssl = SSL_new(m_ctx);
        if (m_ssl)
        {
            bRet = TRUE;
        }
        else
        {
            if (m_ctx)
            {
                SSL_CTX_free(m_ctx);
                m_ctx = NULL;
            }
            SOCKET_IO_ERROR("init ssl: create SSL object failed.");
        }
    }
    else
    {
        SOCKET_IO_ERROR("init ssl: create new SSL_CTX object failed.");
    }
    return bRet;
}

void CSSLClientAsync::UnInitSSL()
{
    if (m_ssl)
    {
        SSL_free(m_ssl);
        m_ssl = NULL;
    }
    if (m_ctx)
    {
        SSL_CTX_free(m_ctx);
        m_ctx = NULL;
    }
}
    
void CSSLClientAsync::OnConnect(BOOL bConnected)
{
    //无论是否连接成功，都认为已经判断结束
    SetCheckConnect(FALSE);
    //连接完毕，则删除写/错误事件的注册,改成读事件
    m_pio->Remove_WriteEvent(this);
    if (TRUE == bConnected)
    {
        SOCKET_IO_INFO("socket connect successed, remote ip: %s, port: %d.", GetRemoteIP(),
                       GetRemotePort());
        DoConnect(GetSocketID());
        
        if (SSLConnect() == SOCKET_IO_RESULT_OK)
        {
            DoSSLConnect(GetSocket());
        }
        else
        {
            DoException(GetSocketID(), SOCKET_IO_SSL_CONNECT_FAILED);
        }
    }
    else
    {
        SOCKET_IO_ERROR("socket connect failed, remote ip: %s, port: %d.", GetRemoteIP(), GetRemotePort());
        DoException(GetSocketID(), SOCKET_IO_TCP_CONNECT_FAILED);
    }
}

void CSSLClientAsync::OnRecv()
{
    char szBuf[TCP_RECV_SIZE] = {0};
    int32_t nRet = SSL_read(GetSSL(), szBuf, TCP_RECV_SIZE);
    if (nRet > 0)
    {
        int32_t nBufSize = nRet;
        char szIP[32] = {0};
        int32_t nPort = 0;
        S_GetPeerName(GetSocket(), szIP, &nPort);
        DoRecv(GetSocketID(), szBuf, nBufSize, szIP, nPort);
    }
    else if (nRet == 0)
    {
        int32_t nErrorCode = SSL_get_error(GetSSL(), nRet);
        if (SSL_ERROR_ZERO_RETURN == nErrorCode)
        {
            //对方关闭socket
            SOCKET_IO_WARN("recv ssl data error, peer closed.");
            DoException(GetSocketID(), SOCKET_IO_SSL_RECV_FAILED);
        }
        else
        {
            SOCKET_IO_ERROR("recv ssl data error.");
            DoException(GetSocketID(), SOCKET_IO_SSL_RECV_FAILED);
        }
    }
    else
    {
        int32_t nErrorCode = SSL_get_error(GetSSL(), nRet);
        if (SSL_ERROR_WANT_READ == nErrorCode || SSL_ERROR_WANT_WRITE == nErrorCode)
        {
            //用select/epoll/iocp的方式应该很少会有这个情况出现
            SOCKET_IO_DEBUG("recv ssl data error, buffer is blocking.");
        }
        else
        {
            SOCKET_IO_ERROR("recv ssl data error, errno: %d.", nErrorCode);
            DoException(GetSocketID(), SOCKET_IO_SSL_RECV_FAILED);
        }
    }
}


int32_t CSSLClientAsync::SSLConnect()
{
    //ssl_handshake采用blocking方式，且在handshake过程中不能由io_loop进行读取数据，否则会有问题，
    //因为ssl_handshake过程也会触发io_loop的可读。
    //看官方文档，ssl_connect支持非阻塞，只是需要采用底层的bio进行操作，此处暂时使用blocking简单化处理
    //但是可能会有一个问题，服务端如果不对此处理，可能会一直卡在SSL_connect这个接口
    m_pio->Remove_Handler(this);
    S_SetNoBlock(GetSocket(), FALSE);
    
    int32_t nErrorCode = SOCKET_IO_RESULT_OK;
    SSL_set_mode(GetSSL(), SSL_MODE_AUTO_RETRY);
    if (SSL_set_fd(GetSSL(), GetSocket()) != 1)
    {
        nErrorCode = SOCKET_IO_SSL_CONNECT_FAILED;
        SOCKET_IO_ERROR("ssl set fd failed");
        return nErrorCode;
    }
    
    int32_t nRet = SSL_connect(GetSSL());
    if (nRet != 1)
    {
        nErrorCode = SOCKET_IO_SSL_CONNECT_FAILED;
        int32_t ssl_error_code = SSL_get_error(GetSSL(), nRet);
        SOCKET_IO_ERROR("ssl connect failed, remote ip: %s, port: %d, error code: %d.",
                        GetRemoteIP(), GetRemotePort(), ssl_error_code);
    }
    else
    {
        SOCKET_IO_INFO("ssl connect successed, remote ip: %s, port: %d.", GetRemoteIP(), GetRemotePort());
        //ssl_handshake成功后在设置成
        S_SetNoBlock(GetSocket(), TRUE);
        m_pio->Add_Handler(this);
    }
    return nErrorCode;
}

int32_t CSSLClientAsync::SendMsgAsync(const char *szBuf, int32_t nBufSize)
{
    int32_t nRet = SSL_write(GetSSL(), (void*)szBuf, nBufSize);
    if ( nRet < 0)
    {
        int32_t nError = SSL_get_error(GetSSL(), nRet);
        if (SSL_ERROR_WANT_WRITE == nError || SSL_ERROR_WANT_READ == nError)
        {
            CBufferLoop* pBufferLoop = new CBufferLoop();
            pBufferLoop->create_buffer(nBufSize);
            pBufferLoop->append_buffer(szBuf, nBufSize);
            m_sendqueuemutex.Lock();
            m_sendqueue.push(pBufferLoop);
            m_sendqueuemutex.Unlock();
            //有数据放入待发送队列，则注册为写事件
            m_pio->Add_WriteEvent(this);
            SOCKET_IO_DEBUG("send ssl data, buffer is blocking.");
        }
        else
        {
            SOCKET_IO_ERROR("send ssl data error, errno: %d.", nError);
            DoException(GetSocketID(), SOCKET_IO_SSL_SEND_FAILED);
        }
    }
    else if (nRet == 0)
    {
        int32_t nError = SSL_get_error(GetSSL(), nRet);
        if (SSL_ERROR_ZERO_RETURN == nError)
        {
            SOCKET_IO_WARN("send ssl data error, peer closed.");
        }
        else
        {
            SOCKET_IO_ERROR("send ssl data error, errno: %d.", nError);
        }
        DoException(GetSocketID(), SOCKET_IO_SSL_SEND_FAILED);
    }
    else if (nRet != nBufSize)
    {
        int32_t nRest = nBufSize - nRet;
        CBufferLoop* pBufferLoop = new CBufferLoop();
        pBufferLoop->create_buffer(nRest);
        pBufferLoop->append_buffer(szBuf + nRet, nRest);
        
        m_sendqueuemutex.Lock();
        m_sendqueue.push(pBufferLoop);
        m_sendqueuemutex.Unlock();
        //有数据放入待发送队列，则注册为写事件
        m_pio->Add_WriteEvent(this);
        SOCKET_IO_DEBUG("send ssl data, send size: %d, less than %d.", nRet, nBufSize);
    }
    return SOCKET_IO_RESULT_OK;
}

int32_t CSSLClientAsync::SendBufferAsync()
{
    int32_t nErrorCode = SOCKET_IO_RESULT_OK;
    m_sendqueuemutex.Lock();
    if (m_sendqueue.size() == 0)
    {
        //待发送队列中为空，则删除写事件的注册,改成读事件
        m_pio->Remove_WriteEvent(this);
        m_sendqueuemutex.Unlock();
        return nErrorCode;
    }
    CBufferLoop* pBufferLoop = m_sendqueue.front();
    m_sendqueuemutex.Unlock();
    char* szSendBuffer = new char[pBufferLoop->get_used_size()];
    int32_t nRealSize = 0;
    pBufferLoop->get_buffer_tmp(szSendBuffer, pBufferLoop->get_used_size(), &nRealSize);
    int32_t nRet = SSL_write(GetSSL(), (void*)szSendBuffer, nRealSize);
    if ( nRet < 0)
    {
        int32_t nError = SSL_get_error(GetSSL(), nRet);
        if (SSL_ERROR_WANT_WRITE == nError || SSL_ERROR_WANT_READ == nError)
        {
            SOCKET_IO_DEBUG("send ssl data, buffer is blocking.")
        }
        else
        {
            SOCKET_IO_ERROR("send ssl data error, errno: %d.", nError);
            DoException(GetSocketID(), SOCKET_IO_SSL_SEND_FAILED);
        }
    }
    else if (nRet == 0)
    {
        int32_t nError = SSL_get_error(GetSSL(), nRet);
        if (SSL_ERROR_ZERO_RETURN == nError)
        {
            SOCKET_IO_WARN("send ssl data error, peer closed.");
        }
        else
        {
            SOCKET_IO_ERROR("send ssl data error, errno: %d.", nError);
        }
        DoException(GetSocketID(), SOCKET_IO_SSL_SEND_FAILED);
    }
    else if (nRet != nRealSize)
    {
        //将未成功的数据重新放置buffer loop中，待下次发送
        int32_t nSize = 0;
        pBufferLoop->get_buffer(szSendBuffer, nRet, &nSize);
        if (nRet != nSize)
        {
            //一般不可能出现这种情况
            SOCKET_IO_ERROR("send ssl data, send size: %d, less than %d, get buffer size wrong.", nRet, nRealSize);
        }
        else
        {
            SOCKET_IO_DEBUG("send ssl data, send size: %d, less than %d.", nRet, nRealSize);
        }
    }
    else
    {
        m_sendqueuemutex.Lock();
        m_sendqueue.pop();
        m_sendqueuemutex.Unlock();
    }
    return nErrorCode;
}

void CSSLClientAsync::Close()
{
    if (m_ssl)
    {
        SSL_shutdown(m_ssl);
    }
    if (GetSocket() != S_INVALID_SOCKET)
    {
        if (m_pio)
        {
            m_pio->Remove_Handler(this);
        }
        
        S_CloseSocket(GetSocket());
        
        SOCKET_IO_WARN("close ssl socket, sock %d, real sock: %d.", GetSocketID(), GetSocket());
        DoClose(GetSocketID());
    }
    m_socket = S_INVALID_SOCKET;

}
