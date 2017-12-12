/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef GM_SECURE_SOCKET_DRIVER_H
#define GM_SECURE_SOCKET_DRIVER_H

#include <AzCore/std/chrono/types.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzCore/State/HSM.h>

#include <GridMate/Carrier/SocketDriver.h>

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)
struct ssl_st;
struct ssl_ctx_st;
struct dh_st;
struct bio_st;
struct x509_store_ctx_st;
struct evp_pkey_st;
struct x509_st;
struct x509_store_ct;

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct dh_st DH;
typedef struct bio_st BIO;
typedef struct x509_store_ctx_st X509_STORE_CTX;
typedef struct x509_store_st X509_STORE;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct x509_st X509;

namespace GridMate
{
    static const int COOKIE_SECRET_LENGTH = 16; //128 bit key
    struct SecureSocketDesc
    {
        SecureSocketDesc()
            : m_connectionTimeoutMS(5000)
            , m_authenticateClient(false)
            , m_privateKeyPEM(nullptr)
            , m_certificatePEM(nullptr)
            , m_certificateAuthorityPEM(nullptr) {};

        AZ::u64     m_connectionTimeoutMS;
        bool        m_authenticateClient;          // Ensure that a client must be authenticated (the server is
                                                   // always authenticated). Only required to be set on the server!
        const char* m_privateKeyPEM;               // A base-64 encoded PEM format private key.
        const char* m_certificatePEM;              // A base-64 encoded PEM format certificate.
        const char* m_certificateAuthorityPEM;     // A base-64 encoded PEM format CA root certificate.
    };

    /**
    * A driver implementation that encrypts and decrypts data sent between the application
    * and the underlying socket. The driver depends on a socket being successfully created
    * and bound to a port so it derives from the existing SocketDriver implementation (this
    * approach also makes SecureSocketDriver fairly platform agnostic).
    *
    * In order to establish a secure channel between two peers a formal connection needs to be
    * created and a TLS handshake performed. During this handshake a cipher is agreed upon, a
    * shared symmetric key generated and peers authenticated.
    *
    * Connections are created when sending or receiving a packet from a peer for the first time
    * and removed when explicitly disconnected or times out.
    *
    * The driver API is stateless so a user needn't know about the internal connections to
    * remote peers. The user simply sends and receive datagrams as normal to endpoints on the network.
    * All user datagrams sent during the connection handshake are queued up and sent encrypted when
    * the connection has been successfully established.
    */
    class SecureSocketDriver
        : public SocketDriver
    {
    public:
        GM_CLASS_ALLOCATOR(SecureSocketDriver);

        typedef AZStd::intrusive_ptr<DriverAddress> AddrPtr;
        typedef AZStd::vector<char>                 Datagram;
        typedef AZStd::pair<Datagram, AddrPtr>      DatagramAddr;

        SecureSocketDriver(const SecureSocketDesc& desc, bool isFullPackets = false, bool crossPlatform = false);
        virtual ~SecureSocketDriver();

        AZ::u32 GetMaxSendSize() const override;

        Driver::ResultCode Initialize(AZ::s32 ft, const char* address, AZ::u32 port, bool isBroadcast, AZ::u32 receiveBufferSize, AZ::u32 sendBufferSize) override;
        void               Update() override;
        Driver::ResultCode Receive(char* data, AZ::u32 maxDataSize, AddrPtr& from, ResultCode* resultCode) override;
        AZ::u32            Send(const AddrPtr& to, const char* data, AZ::u32 dataSize) override;

    private:
        enum ConnectionState
        {
            CS_TOP,
                CS_ACCEPT,          // Perform TLS/DTLS handshake for an incoming connection.
                CS_CONNECT,         // Perform TLS/DTLS handshake for an outgoing connection.
                CS_ESTABLISHED,     // SSL handshake succeeded.
                CS_DISCONNECTED,    // Normal disconnect.
                CS_ERROR,           // Error.
            CS_MAX
        };

        /**
        * Manage a single DTLS connection to a remote peer. It is ideally backed by two buffers
        * that will contain ciphertext and two queues that contain plaintext.
        *
        * There are two flows of data:
        *  - Plaintext is pulled from the out queue, encrypted, and ciphertext written to the out buffer.
        *  - Ciphertext is read from the in buffer, decrypted, and plaintext added to the in queue.
        *
        * In practice there is two buffers of ciphertext and only one queue that contains plaintext.
        *
        * As an optimization, the connection does not hold it's own plaintext in queue. Since the
        * user is going to be polling the single driver for plaintext datagrams it is better to
        * add all decrypted datagrams to a single, shared queue for the driver to pull from.
        *
        * Connections have their own timeout which is set during construction. The connection will
        * be disconnected on a timeout and no further communication will be possible.
        */
        class Connection
        {
        public:
            GM_CLASS_ALLOCATOR(Connection);
            Connection(const AddrPtr& addr, AZ::u32 bufferSize, AZStd::queue<DatagramAddr>* inQueue, AZ::u64 timeoutMS);
            virtual ~Connection();

            bool    Initialize(SSL_CTX* sslContext, ConnectionState startState, AZ::u32 mtu);
            void    Shutdown();

            void    Update();
            void    AddDgram(const char* data, AZ::u32 dataSize);
            void    AddDTLSDgram(const char* data, AZ::u32 dataSize);
            AZ::u32 GetDTLSDgram(char* data, AZ::u32 dataSize);
            bool    IsDisconnected() const;

            const SSL* GetSSLContext() { return m_ssl; }

        private:
            bool OnStateTop(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateAccept(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateConnect(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateEstablished(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateDisconnected(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateError(AZ::HSM& sm, const AZ::HSM::Event& event);

            // Read a DTLS record (datagram) from a BIO buffer. Return true if records were read and stored
            // in outDgramList, or false if nothing was found.
            bool ReadDgramFromBuffer(BIO* buffer, AZStd::vector<SecureSocketDriver::Datagram>& outDgramQueue);

            enum ConnectionEvents
            {
                CE_UPDATE = 1,
            };

            bool m_isInitialized;
            TimeStamp m_establishedTime;    // The time the connection reached the established state.
            AZ::u64 m_timeoutMS;
            BIO* m_inDTLSBuffer;
            BIO* m_outDTLSBuffer;
            AZStd::queue<Datagram> m_inPlainQueue;       // Plaintext datagrams given to the connection.
            AZStd::queue<Datagram> m_outDTLSQueue;       // DTLS datagrams output by the connection.
            AZStd::queue<DatagramAddr>* m_outPlainQueue; // Plaintext datagrams output by the connection.
            AZ::HSM m_sm;
            SSL* m_ssl;
            AddrPtr m_addr;
            char* m_tempBIOBuffer;
            AZ::u32 m_maxTempBufferSize;
            AZ::s32 m_sslError;
            AZ::u32 m_mtu;
        };

        bool GetPeerAddress(SSL* ssl, string& addr) const;
        bool RotateCookieSecret(bool bForce = false);
        static int VerifyCertificate(int ok, X509_STORE_CTX* ctx);
        int GenerateCookie(SSL* ssl, unsigned char* cookie, unsigned int* cookieLen);
        int VerifyCookie(SSL* ssl, unsigned char* cookie, unsigned int cookieLen);

        void FlushSocketToConnectionBuffer();
        void UpdateConnections();
        void FlushConnectionBuffersToSocket();

        DH* m_dh;
        EVP_PKEY* m_privateKey;
        X509* m_certificate;
        SSL_CTX* m_sslContext;
        char* m_tempSocketBuffer;
        struct GridMateSecret
        {
            TimeStamp m_lastSecretGenerationTime;
            unsigned char m_currentSecret[COOKIE_SECRET_LENGTH];
            unsigned char m_previousSecret[COOKIE_SECRET_LENGTH];
            bool m_isCurrentSecretValid;
            bool m_isPreviousSecretValid;

            GridMateSecret()
                : m_isCurrentSecretValid(false)
                , m_isPreviousSecretValid(false)
            { }
        } m_cookieSecret;

        AZ::u32 m_maxTempBufferSize;
        AZStd::queue<DatagramAddr> m_globalInQueue;
        AZStd::unordered_map<SocketDriverAddress, Connection*, SocketDriverAddress::Hasher> m_connections;
        AZStd::unordered_map<const SSL*, string> m_sslToAddress;
        static const AZ::u32 kSSLContextDriverPtrArg;
        static const AZ::u32 kDTLSSecretExpirationTime;



        SecureSocketDesc m_desc;
    };
}

#endif
#endif