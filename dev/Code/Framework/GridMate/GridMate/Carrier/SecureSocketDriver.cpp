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
#ifndef AZ_UNITY_BUILD

#include <AzCore/PlatformDef.h>
#include <GridMate/Carrier/SecureSocketDriver.h>
#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/DataMarshal.h>

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

namespace GridMate
{
    const AZ::u32 SecureSocketDriver::kSSLContextDriverPtrArg = 0;
    const AZ::u32 SecureSocketDriver::kDTLSSecretExpirationTime = 1000 * 30; // 30 Seconds

    namespace ConnectionSecurity
    {
        const AZ::u8 kExpectedCookieSize = 20;

        // Unpacking
        // 
        template<typename TOut, size_t TBitsOffset>
        AZ_INLINE static TOut UnpackByte(ReadBuffer& input)
        {
            if (input.IsValid())
            {
                AZ::u8 byte;
                input.Read(byte);
                TOut value = static_cast<TOut>(byte) << TBitsOffset;
                return value;
            }
            AZ_TracePrintf("GridMateSecure", "Read buffer unpacked too many bytes.");
            return 0;
        }

        AZ_INLINE static void UnpackNetwork1ToHost1(ReadBuffer& input, AZ::u8& value)
        {
            value = UnpackByte<AZ::u8, 0>(input);
        }

        AZ_INLINE static void UnpackNetwork2ToHost2(ReadBuffer& input, AZ::u16& value)
        {
            if (input.IsValid())
            {
                input.Read(value);
            }
        }

        AZ_INLINE static void UnpackNetwork3ToHost4(ReadBuffer& input, AZ::u32& value)
        {
            value = UnpackByte<AZ::u32, 16>(input)
                  | UnpackByte<AZ::u32, 8 >(input)
                  | UnpackByte<AZ::u32, 0 >(input);
        }

        AZ_INLINE static void UnpackNetwork6ToHost8(ReadBuffer& input, AZ::u64& value)
        {
            value = UnpackByte<AZ::u64, 40>(input)
                  | UnpackByte<AZ::u64, 32>(input)
                  | UnpackByte<AZ::u64, 24>(input)
                  | UnpackByte<AZ::u64, 16>(input)
                  | UnpackByte<AZ::u64, 8 >(input)
                  | UnpackByte<AZ::u64, 0 >(input);
        }

        AZ_INLINE static void UnpackOpaque(ReadBuffer& input, uint8_t* bytes, size_t bytesSize)
        {
            if (input.IsValid())
            {
                input.ReadRaw(bytes, bytesSize);
            }
        }

        AZ_INLINE static void UnpackRange(ReadBuffer& input, AZ::u8 minBytes, AZ::u8 maxBytes, AZ::u8* output, AZ::u8& outputSize)
        {
            if (input.IsValid())
            {
                outputSize = 0;
                UnpackNetwork1ToHost1(input, outputSize);
                if (outputSize < minBytes || outputSize > maxBytes)
                {
                    AZ_TracePrintf("GridMate", "Unpack out of range");
                }
                if (outputSize > 0)
                {
                    input.ReadRaw(output, outputSize);
                }
            }
        }

        // Packing
        //
        template<typename TInput, size_t TBytePos>
        AZ_INLINE static AZ::u8 PackByte(TInput input)
        {
            return static_cast<AZ::u8>(input >> (8 * TBytePos));
        }

        AZ_INLINE static void PackHost4Network3(AZ::u32 value, WriteBuffer& writeBuffer)
        {
            writeBuffer.Write(PackByte<AZ::u32, 2>(value));
            writeBuffer.Write(PackByte<AZ::u32, 1>(value));
            writeBuffer.Write(PackByte<AZ::u32, 0>(value));
        }

        AZ_INLINE static void PackHost8Network6(AZ::u64 value, WriteBuffer& writeBuffer)
        {
            writeBuffer.Write(PackByte<AZ::u64, 5>(value));
            writeBuffer.Write(PackByte<AZ::u64, 4>(value));
            writeBuffer.Write(PackByte<AZ::u64, 3>(value));
            writeBuffer.Write(PackByte<AZ::u64, 2>(value));
            writeBuffer.Write(PackByte<AZ::u64, 1>(value));
            writeBuffer.Write(PackByte<AZ::u64, 0>(value));
        }

        AZ_INLINE static AZ::u32 CalculatePeerCRC32(const SecureSocketDriver::AddrPtr& from)
        {
            // Calculate CRC32 from remote address
            AZ::u32 port = from->GetPort();
            AZ::Crc32 crc;
            crc.Add(from->GetIP().c_str());
            crc.Add(&port, sizeof(port));
            return crc;
        }

        // Structures
        //
        struct RecordHeader                 // 13 bytes = DTLS1_RT_HEADER_LENGTH
        {
            const AZ::u32 kExpectedSize;
                                             // offset  size
            AZ::u8  m_type;                  // [ 0]    1
            AZ::u16 m_version;               // [ 1]    2
            AZ::u16 m_epoch;                 // [ 3]    2
            AZ::u64 m_sequenceNumber;        // [ 5]    6
            AZ::u16 m_length;                // [11]    2
            
            RecordHeader()
                : kExpectedSize(13)
            {
            }
            RecordHeader& operator= (const RecordHeader& other) = delete;

            bool Unpack(ReadBuffer& readBuffer)
            {
                UnpackNetwork1ToHost1(readBuffer, m_type);
                UnpackNetwork2ToHost2(readBuffer, m_version);
                UnpackNetwork2ToHost2(readBuffer, m_epoch);
                UnpackNetwork6ToHost8(readBuffer, m_sequenceNumber);
                UnpackNetwork2ToHost2(readBuffer, m_length);
                return readBuffer.IsValid();
            }

            bool Pack(WriteBuffer& writeBuffer) const
            {
                writeBuffer.Write(m_type);
                writeBuffer.Write(m_version);
                writeBuffer.Write(m_epoch);
                PackHost8Network6(m_sequenceNumber, writeBuffer);
                writeBuffer.Write(m_length);
                return writeBuffer.Size() == kExpectedSize;
            }
        };

        struct HandshakeHeader              // 12 bytes = DTLS1_HM_HEADER_LENGTH
            : public RecordHeader
        {
            const AZ::u32 kExpectedSize;
                                             // offset  size
            AZ::u8  m_hsType;                // [13]    1
            AZ::u32 m_hsLength;              // [14]    3
            AZ::u16 m_hsSequence;            // [17]    2
            AZ::u32 m_hsFragmentOffset;      // [19]    3
            AZ::u32 m_hsFragmentLength;      // [22]    3

            HandshakeHeader()
                : kExpectedSize(12)
            {
            }
            
            bool Unpack(ReadBuffer& readBuffer)
            {
                if (!RecordHeader::Unpack(readBuffer))
                {
                    return false;
                }
                UnpackHandshake(readBuffer);
                return readBuffer.IsValid();
            }

            bool UnpackHandshake(ReadBuffer& readBuffer)
            {
                UnpackNetwork1ToHost1(readBuffer, m_hsType);
                UnpackNetwork3ToHost4(readBuffer, m_hsLength);
                UnpackNetwork2ToHost2(readBuffer, m_hsSequence);
                UnpackNetwork3ToHost4(readBuffer, m_hsFragmentOffset);
                UnpackNetwork3ToHost4(readBuffer, m_hsFragmentLength);
                return readBuffer.IsValid();
            }

            bool Pack(WriteBuffer& writeBuffer) const
            {
                if (!RecordHeader::Pack(writeBuffer))
                {
                    return false;
                }
                writeBuffer.Write(m_hsType);
                PackHost4Network3(m_hsLength, writeBuffer);
                writeBuffer.Write(m_hsSequence);
                PackHost4Network3(m_hsFragmentOffset, writeBuffer);
                PackHost4Network3(m_hsFragmentLength, writeBuffer);
                return writeBuffer.Size() == kExpectedSize + RecordHeader::kExpectedSize;
            }
        };

        struct ClientHello               // 56 + other stuff the client sent... the headers and cookie are the only things considered
            : public HandshakeHeader     
        {
            const AZ::u32 kBaseExpectedSize;

            AZ::u16 m_clientVersion;              // [25] 2
            AZ::u8 m_randomBytes[32];             // [27] 32
            AZ::u8 m_sessionSize;                 // [29] 1 (should be zero value)
            AZ::u8 m_sessionId[32];               // [__] 0 (Client Hello should not have any session data)
            AZ::u8 m_cookieSize;                  // [30] 1 (normally the value is kExpectedCookieSize)
            AZ::u8 m_cookie[DTLS1_COOKIE_LENGTH]; // [31] 0 up to 255

            ClientHello() 
                : kBaseExpectedSize(sizeof(m_clientVersion) + sizeof(m_randomBytes) + sizeof(m_sessionSize) + 0 + sizeof(m_cookieSize))
            {
            }

            bool Unpack(ReadBuffer& readBuffer)
            {
                if (!HandshakeHeader::Unpack(readBuffer))
                {
                    return false;
                }

                memset(m_sessionId, 0, sizeof(m_sessionId));
                memset(m_randomBytes, 0, sizeof(m_randomBytes));
                memset(m_cookie, 0, sizeof(m_cookie));

                UnpackNetwork2ToHost2(readBuffer, m_clientVersion);
                UnpackOpaque(readBuffer, &m_randomBytes[0], sizeof(m_randomBytes));
                UnpackRange(readBuffer, 0, sizeof(m_sessionId), &m_sessionId[0], m_sessionSize);
                UnpackRange(readBuffer, 0, sizeof(m_cookie) - 1, &m_cookie[0], m_cookieSize);
                return readBuffer.IsValid() && readBuffer.Read() == (HandshakeHeader::kExpectedSize + RecordHeader::kExpectedSize + kBaseExpectedSize + m_cookieSize);
            }

            bool VerifyCookie(const uint8_t* secret, const uint32_t secretSize, const SecureSocketDriver::AddrPtr& from) const
            {
                // Calculate HMAC of buffer using the secret and remote address
                AZ::u32 crcValue = CalculatePeerCRC32(from);

                AZ::u8 result[EVP_MAX_MD_SIZE];
                AZ::u32 resultLen = 0;
                HMAC(EVP_sha1(),
                    secret,
                    secretSize,
                    reinterpret_cast<const AZ::u8*>(&crcValue),
                    sizeof(crcValue),
                    result,
                    &resultLen);

                if (resultLen != m_cookieSize)
                {
                    AZ_TracePrintf("GridMateSecure", "Wrong cookie size %d expected %d", resultLen, m_cookieSize);
                    return false;
                }
                if (memcmp(result, &m_cookie[0], resultLen) != 0)
                {
                    AZ_TracePrintf("GridMateSecure", "Invalid cookie");
                    return false;
                }
                return true;
            }
        };

        struct HelloVerifyRequest                   // 25 = 3 + sizeof cookie (kExpectedCookieSize)
            : public HandshakeHeader
        {
            const AZ::u32 kFragmentLength;

            AZ::u16 m_serverVersion;                 // [26] 2
            AZ::u8 m_cookieSize;                     // [27] 1
            AZ::u8 m_cookie[DTLS1_COOKIE_LENGTH];    // [29] up to 254

            HelloVerifyRequest() 
                // The server_version field has the same syntax as in TLS.However, in order to avoid the requirement to do 
                // version negotiation in the initial handshake, DTLS 1.2 server implementations SHOULD use DTLS version 1.0 
                // regardless of the version of TLS that is expected to be negotiated.
                : kFragmentLength(sizeof(m_serverVersion) + sizeof(m_cookieSize) + kExpectedCookieSize)
                , m_serverVersion(DTLS1_VERSION)
                , m_cookieSize(kExpectedCookieSize)
            {
                RecordHeader::m_type = SSL3_RT_HANDSHAKE;
                RecordHeader::m_version = DTLS1_VERSION;
                RecordHeader::m_epoch = 0;
                RecordHeader::m_sequenceNumber = 0;
                RecordHeader::m_length = static_cast<AZ::u16>(DTLS1_HM_HEADER_LENGTH + kFragmentLength);

                HandshakeHeader::m_hsLength = kFragmentLength;
                HandshakeHeader::m_hsType = DTLS1_MT_HELLO_VERIFY_REQUEST;
                HandshakeHeader::m_hsSequence = 0;
                HandshakeHeader::m_hsFragmentOffset = 0;
                HandshakeHeader::m_hsFragmentLength = kFragmentLength;
            }

            bool PrepareCookie(const uint8_t* secret, const uint32_t secretSize, const SecureSocketDriver::AddrPtr& from)
            {
                AZ::u32 crcValue = CalculatePeerCRC32(from);

                // Calculate HMAC of buffer using the secret and remote address
                AZ::u32 resultLen = 0;
                HMAC(EVP_sha1(), 
                    secret,
                    secretSize,
                    reinterpret_cast<const AZ::u8*>(&crcValue),
                    sizeof(crcValue),
                    m_cookie,
                    &resultLen);

                if (resultLen > DTLS1_COOKIE_LENGTH)
                {
                    AZ_TracePrintf("GridMateSecure", "Insufficient cookie buffer: %u > %u\n", resultLen, DTLS1_COOKIE_LENGTH);
                    return false;
                }
                m_cookieSize = static_cast<AZ::u8>(resultLen);
                return true;
            }

            bool Pack(WriteBuffer& writeBuffer) const
            {
                if (!HandshakeHeader::Pack(writeBuffer))
                {
                    return false;
                }
                writeBuffer.Write(m_serverVersion);
                writeBuffer.Write(static_cast<AZ::u8>(m_cookieSize));
                writeBuffer.WriteRaw(&m_cookie[0], m_cookieSize);
                return writeBuffer.Size() == (HandshakeHeader::kExpectedSize + RecordHeader::kExpectedSize + kFragmentLength);
            }
        };

        struct HelloRequest          // 0 (headers only)
            : public HandshakeHeader
        {
            HelloRequest()
            {
                RecordHeader::m_type = SSL3_RT_HANDSHAKE;
                RecordHeader::m_version = DTLS1_VERSION;
                RecordHeader::m_epoch = 0;
                RecordHeader::m_sequenceNumber = 0;
                RecordHeader::m_length = static_cast<AZ::u16>(DTLS1_HM_HEADER_LENGTH);

                HandshakeHeader::m_hsLength = 0;
                HandshakeHeader::m_hsType = SSL3_MT_HELLO_REQUEST;
                HandshakeHeader::m_hsSequence = 0;
                HandshakeHeader::m_hsFragmentOffset = 0;
                HandshakeHeader::m_hsFragmentLength = 0;
            }

            bool Pack(WriteBuffer& writeBuffer) const
            {
                return HandshakeHeader::Pack(writeBuffer);
            }
        };

        // Constants
        //
        const AZ::u32 kMaxPacketSize = sizeof(HelloVerifyRequest); // largest to be written to client

        // Functions
        //
        AZ_INLINE AZ::u32 PrepareHelloVerifyRequest(WriteBuffer& writeBuffer, const SecureSocketDriver::AddrPtr& from, const AZ::u8* secret, const AZ::u32 secretSize)
        {
            HelloVerifyRequest helloVerifyRequest;
            if (helloVerifyRequest.PrepareCookie(secret, secretSize, from))
            {
                if (helloVerifyRequest.Pack(writeBuffer))
                {
                    return static_cast<AZ::u32>(writeBuffer.Size());
                }
            }
            return 0;
        }
        
        AZ_INLINE AZ::u32 PrepareHelloRequest(char* inputBytes, AZ::u32 inputSize, WriteBuffer& writeBuffer, const SecureSocketDriver::AddrPtr& from, const AZ::u8* secret, const AZ::u32 secretSize)
        {
            ReadBuffer buffer(EndianType::BigEndian, inputBytes, inputSize);
            ClientHello hello;
            if (hello.Unpack(buffer))
            {
                if (hello.VerifyCookie(secret, secretSize, from))
                {
                    HelloRequest helloRequest;
                    if (helloRequest.Pack(writeBuffer))
                    {
                        return static_cast<AZ::u32>(writeBuffer.Size());
                    }
                }
            }
            return 0;
        }

        enum class NextAction
        {
            Error,
            MakeNewConnection,
            SendHelloVerifyRequest
        };


        AZ_INLINE NextAction DetermineHandshakeState(char* ptr, AZ::u32 bytesReceived)
        {
            if (bytesReceived < (DTLS1_HM_HEADER_LENGTH + DTLS1_RT_HEADER_LENGTH))
            {
                return NextAction::Error;
            }
            if (*ptr != SSL3_RT_HANDSHAKE)
            {
                return NextAction::Error;
            }

            ReadBuffer buffer(EndianType::BigEndian, ptr, bytesReceived);
            ClientHello clientHello;
            if (!clientHello.Unpack(buffer))
            {
                return NextAction::Error;
            }

            if (clientHello.m_version != DTLS1_VERSION &&
                clientHello.m_version != DTLS1_2_VERSION)
            {
                return NextAction::Error;
            }
            if (clientHello.m_length > bytesReceived)
            {
                return NextAction::Error;
            }

            if (clientHello.m_hsType == SSL3_MT_CLIENT_HELLO)
            {
                // RFC-6347
                // The first message each side transmits in each handshake always has message_seq = 0.  Whenever each new message is generated, the
                // message_seq value is incremented by one.Note that in the case of a re-handshake, this implies that the HelloRequest will have message_seq
                // = 0 and the ServerHello will have message_seq = 1.  When a message is retransmitted, the same message_seq value is used.

                if (clientHello.m_hsSequence == 0)
                {
                    // is the ClientHello(0)
                    //   1. send back HelloVerifyRequest
                    return NextAction::SendHelloVerifyRequest;
                }
                else if (clientHello.m_hsSequence == 1)
                {
                    // is the ClientHello(1)
                    //   1. check for cookie
                    // if all good then:
                    //   1. Send back HelloRequest to restart the HelloClient sequence
                    //   2. Prepare a new connection for the remote address
                    //   3. process the DTLS sequence for ssl_accept latter on during CS_ACCEPT
                    if (clientHello.m_cookieSize == kExpectedCookieSize)
                    {
                        return NextAction::MakeNewConnection;
                    }
                }
            }
            return NextAction::Error;
        }

        AZ_INLINE bool IsHelloRequestHandshake(const char* data, AZ::u32 dataSize)
        {
            if (dataSize >= (DTLS1_RT_HEADER_LENGTH + DTLS1_HM_HEADER_LENGTH))
            {
                const AZ::u8* bytes = reinterpret_cast<const AZ::u8*>(data);
                if (bytes[0] == SSL3_RT_HANDSHAKE)
                {
                    if (bytes[1] == DTLS1_VERSION_MAJOR)
                    {
                        return (bytes[DTLS1_RT_HEADER_LENGTH] == SSL3_MT_HELLO_REQUEST);
                    }
                }
            }
            return false;
        }
    }

    X509* CreateCertificateFromEncodedPEM(const char* encodedPem)
    {
        BIO* tempBio = BIO_new(BIO_s_mem());
        BIO_puts(tempBio, encodedPem);
        X509* certificate = PEM_read_bio_X509(tempBio, nullptr, nullptr, nullptr);
        BIO_free(tempBio);
        return certificate;
    }

    EVP_PKEY* CreatePrivateKeyFromEncodedPEM(const char* encodedPem)
    {
        BIO* tempBio = BIO_new(BIO_s_mem());
        BIO_puts(tempBio, encodedPem);
        EVP_PKEY* privateKey = PEM_read_bio_PrivateKey(tempBio, nullptr, nullptr, nullptr);
        BIO_free(tempBio);
        return privateKey;
    }

    void CreateCertificateChainFromEncodedPEM(const char* encodedPem, AZStd::vector<X509*>& certificateChain)
    {
        const char startCertHeader[] = "-----BEGIN CERTIFICATE-----";
        const char endCertHeader[] = "-----END CERTIFICATE-----";
        const size_t endCertHeaderLen = strlen(endCertHeader);

        size_t offset = 0;
        AZStd::string encodedPemStr(encodedPem);
        while (true)
        {
            size_t beginStartIdx = encodedPemStr.find(startCertHeader, offset);
            size_t endStartIdx = encodedPemStr.find(endCertHeader, offset);

            if (beginStartIdx == AZStd::string::npos ||
                endStartIdx == AZStd::string::npos ||
                beginStartIdx >= endStartIdx)
            {
                break;
            }

            size_t certLen = (endStartIdx - beginStartIdx) + endCertHeaderLen;
            AZStd::string encodedPemCertStr = encodedPemStr.substr(beginStartIdx, certLen);
            X509* newCertificate = CreateCertificateFromEncodedPEM(encodedPemCertStr.c_str());
            if (newCertificate == nullptr)
            {
                break;
            }

            certificateChain.push_back(newCertificate);

            offset = (endStartIdx + endCertHeaderLen);
        }
    }

    SecureSocketDriver::Connection::Connection(const AddrPtr& addr, AZ::u32 bufferSize, AZStd::queue<DatagramAddr>* inQueue, AZ::u64 timeoutMS)
        : m_isInitialized(false)
        , m_timeoutMS(timeoutMS)
        , m_inDTLSBuffer(nullptr)
        , m_outDTLSBuffer(nullptr)
        , m_outPlainQueue(inQueue)
        , m_ssl(nullptr)
        , m_addr(addr)
        , m_tempBIOBuffer(nullptr)
        , m_maxTempBufferSize(bufferSize)
        , m_sslError(SSL_ERROR_NONE)
        , m_mtu(576) // RFC-791
    {
    }

    SecureSocketDriver::Connection::~Connection()
    {
        Shutdown();
    }

    bool SecureSocketDriver::Connection::Initialize(SSL_CTX* sslContext, ConnectionState startState, AZ::u32 mtu)
    {
        if (m_isInitialized)
        {
            return false;
        }

        if (startState != ConnectionState::CS_ACCEPT && startState != ConnectionState::CS_CONNECT)
        {
            return false;
        }

        m_ssl = SSL_new(sslContext);
        if (m_ssl == nullptr)
        {
            return false;
        }

        // Set the internal DTLS MTU for use when fragmenting DTLS handshake datagrams only,
        // and not the application datagrams (i.e. internally generated datagrams). Datagrams
        // passed into the SecureSocketDriver are expected to already be smaller them GetMaxSendSize().
        // This is particularly relevant when sending certificates in the handshake which will
        // likely be larger than the MTU.
        m_mtu = mtu;
        SSL_set_mtu(m_ssl, mtu);

        m_inDTLSBuffer = BIO_new(BIO_s_mem());
        if (m_inDTLSBuffer == nullptr)
        {
            SSL_free(m_ssl);
            m_ssl = nullptr;

            return false;
        }

        m_outDTLSBuffer = BIO_new(BIO_s_mem());
        if (m_outDTLSBuffer == nullptr)
        {
            SSL_free(m_ssl);
            m_ssl = nullptr;

            BIO_free(m_inDTLSBuffer);
            m_inDTLSBuffer = nullptr;

            return false;
        }

        BIO_set_mem_eof_return(m_inDTLSBuffer, -1);
        BIO_set_mem_eof_return(m_outDTLSBuffer, -1);

        SSL_set_bio(m_ssl, m_inDTLSBuffer, m_outDTLSBuffer);

        m_tempBIOBuffer = static_cast<char*>(azmalloc(m_maxTempBufferSize));
        if (m_tempBIOBuffer == nullptr)
        {
            return false;
        }

        AZ::HSM::StateId startStateId = static_cast<AZ::HSM::StateId>(startState);
        // the client will call Initialize() again during the handshake, so the HSM might already be ready
        if (!m_sm.IsInState(startStateId))
        {
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_TOP), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateTop), AZ::HSM::InvalidStateId, startStateId);

            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_ACCEPT), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateAccept), CS_TOP);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_CONNECT), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateConnect), CS_TOP);

            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_ESTABLISHED), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateEstablished), CS_TOP);

            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_DISCONNECTED), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateDisconnected), CS_TOP);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_ERROR), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateError), CS_TOP);

            m_sm.Start();
        }

        m_isInitialized = true;

        return true;
    }

    void SecureSocketDriver::Connection::Shutdown()
    {
        if (m_tempBIOBuffer)
        {
            azfree(m_tempBIOBuffer);
            m_tempBIOBuffer = nullptr;
        }

        if (m_ssl)
        {
            // Calls to SSL_free() also free any attached BIO objects.
            SSL_free(m_ssl);
            m_ssl = nullptr;
        }

        m_isInitialized = false;
    }

    bool SecureSocketDriver::Connection::OnStateTop(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        (void)sm;
        (void)event;
        return true;
    }

    bool SecureSocketDriver::Connection::OnStateAccept(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        switch (event.id)
        {
        case AZ::HSM::EnterEventId:
            AZ_TracePrintf("GridMateSecure", "Accepting a new connection from %s.\n", m_addr->ToString().c_str());
            SSL_set_accept_state(m_ssl);
            return true;
        case AZ::HSM::ExitEventId:
            return true;
        }

        bool changedState = false;

        int result = SSL_accept(m_ssl);
        if (result == 1)
        {
            sm.Transition(ConnectionState::CS_ESTABLISHED);
            changedState = true;
        }
        else if (result <= 0)
        {
            AZ::s32 sslError = SSL_get_error(m_ssl, result);
            if (sslError != SSL_ERROR_WANT_READ)
            {
                m_sslError = sslError;
                sm.Transition(ConnectionState::CS_ERROR);
                changedState = true;
            }
        }

        AZStd::vector<SecureSocketDriver::Datagram> dgramList;
        if (ReadDgramFromBuffer(m_outDTLSBuffer, dgramList))
        {
            for (const auto& dgram : dgramList)
            {
                m_outDTLSQueue.push(dgram);
            }
        }

        return changedState;
    }

    bool SecureSocketDriver::Connection::OnStateConnect(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        switch (event.id)
        {
        case AZ::HSM::EnterEventId:
            AZ_TracePrintf("GridMateSecure", "Connecting to %s.\n", m_addr->ToString().c_str());
            SSL_set_connect_state(m_ssl);
            return true;
        case AZ::HSM::ExitEventId:
            return true;
        }

        bool changedState = false;

        int result = SSL_connect(m_ssl);
        if (result == 1)
        {
            sm.Transition(ConnectionState::CS_ESTABLISHED);
            changedState = true;
        }
        else if (result <= 0)
        {
            AZ::s32 sslError = SSL_get_error(m_ssl, result);
            if (sslError != SSL_ERROR_WANT_READ)
            {
                m_sslError = sslError;
                sm.Transition(ConnectionState::CS_ERROR);
                changedState = true;
            }
        }

        AZStd::vector<SecureSocketDriver::Datagram> dgramList;
        if (ReadDgramFromBuffer(m_outDTLSBuffer, dgramList))
        {
            for (const auto& dgram : dgramList)
            {
                m_outDTLSQueue.push(dgram);
            }
        }
        return changedState;
    }

    bool SecureSocketDriver::Connection::OnStateEstablished(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        switch (event.id)
        {
        case AZ::HSM::EnterEventId:
            AZ_TracePrintf("GridMateSecure", "Successfully established connection to %s.\n", m_addr->ToString().c_str());
            m_establishedTime = AZStd::chrono::system_clock::now();
            return true;
        case AZ::HSM::ExitEventId:
            return true;
        }

        if (!m_addr->IsBoundToCarrierConnection())
        {
            // Carrier connection binding is async, so we need to give it some time.
            if (AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now() - m_establishedTime).count() > m_timeoutMS)
            {
                m_sm.Transition(CS_DISCONNECTED);
            }
        }

        while (true)
        {
            AZ::s32 bytesRead = SSL_read(m_ssl, m_tempBIOBuffer, m_maxTempBufferSize);
            if (bytesRead <= 0)
            {
                AZ::s32 sslError = SSL_get_error(m_ssl, bytesRead);
                if (sslError != SSL_ERROR_WANT_READ)
                {
                    m_sslError = sslError;
                    sm.Transition(ConnectionState::CS_ERROR);
                    return true;
                }
                break;
            }

            m_outPlainQueue->push(DatagramAddr(Datagram(m_tempBIOBuffer, m_tempBIOBuffer + bytesRead), m_addr));
        }

        while (!m_inPlainQueue.empty())
        {
            const Datagram& plainDgram = m_inPlainQueue.front();
            AZ::s32 bytesWritten = SSL_write(m_ssl, plainDgram.data(), static_cast<int>(plainDgram.size()));
            if (bytesWritten <= 0)
            {
                AZ::s32 sslError = SSL_get_error(m_ssl, bytesWritten);
                if (sslError != SSL_ERROR_WANT_WRITE)
                {
                    m_sslError = sslError;
                    sm.Transition(ConnectionState::CS_ERROR);
                    return true;
                }
                break;
            }

            AZStd::vector<SecureSocketDriver::Datagram> cipherDgramList;
            if (ReadDgramFromBuffer(m_outDTLSBuffer, cipherDgramList))
            {
                for (const auto& cipherDgram : cipherDgramList)
                {
                    m_outDTLSQueue.push(cipherDgram);
                }
            }

            m_inPlainQueue.pop();
        }

        return false;
    }

    bool SecureSocketDriver::Connection::OnStateDisconnected(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        (void)sm;
        (void)event;
        switch (event.id)
        {
        case AZ::HSM::EnterEventId:
            AZ_TracePrintf("GridMateSecure", "Lost connection to %s.\n", m_addr->ToString().c_str());
            return true;
        }
        return false;
    }

    bool SecureSocketDriver::Connection::OnStateError(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        (void)sm;
        (void)event;
        switch (event.id)
        {
        case AZ::HSM::EnterEventId:
        {
            static const AZ::u32 BUFFER_SIZE = 256;
            char buffer[BUFFER_SIZE];
            if (m_sslError == SSL_ERROR_SSL)
            {
                ERR_error_string_n(ERR_get_error(), buffer, BUFFER_SIZE);
                AZ_TracePrintf("GridMateSecure", "Connection error occured on %s with SSL error %s.\n", m_addr->ToString().c_str(), buffer);
            }
            else
            {
                AZ_TracePrintf("GridMateSecure", "Connection error occured on %s with SSL error %d.\n", m_addr->ToString().c_str(), m_sslError);
            }
            return true;
        }

        case CE_UPDATE:
        {
            m_sm.Transition(CS_DISCONNECTED);
            return true;
        }
        }

        return false;
    }

    bool SecureSocketDriver::Connection::ReadDgramFromBuffer(BIO* bio, AZStd::vector<SecureSocketDriver::Datagram>& outDgramList)
    {
        //NOTE: It is expected that this BIO buffer has been filled up with multiple DTLS records
        //      created by SSL functions: SSL_write(), SSL_accept(), or SSL_connect().

        // Drain the BIO buffer and store the contents in a temporary buffer for
        // deserialization. This is necessary because the BIO object doesn't provide
        // a way to directly access it's memory.
        AZStd::vector<char> buffer;
        while (true)
        {
            AZ::s32 bytesRead = BIO_read(bio, m_tempBIOBuffer, m_maxTempBufferSize);
            if (bytesRead <= 0)
            {
                break;
            }

            AZStd::vector<char> tempBuffer(m_tempBIOBuffer, m_tempBIOBuffer + bytesRead);
            buffer.insert(buffer.begin(), tempBuffer.begin(), tempBuffer.end());
        }

        // Multiple DTLS records (datagrams) may have been written to the BIO buffer so
        // each one must be extracted and stored as a separate Datagram object in the driver.
        static const size_t lengthOffsetInDTLSRecord = DTLS1_RT_HEADER_LENGTH - sizeof(AZ::u16);
        size_t recordStart = 0, recordEnd = (lengthOffsetInDTLSRecord + sizeof(AZ::u16));
        while (recordEnd < buffer.size())
        {
            AZ::u16 recordLength = *reinterpret_cast<AZ::u16*>(buffer.data() + recordEnd - sizeof(AZ::u16));

            // The fields in a DTLS record are stored in big-endian format so perform
            // an endian swap on little-endian machines.
#if !defined(AZ_BIG_ENDIAN)
            AZStd::endian_swap<AZ::u16>(recordLength);
#endif

            recordEnd += recordLength;
            if (recordEnd > buffer.size())
            {
                break;
            }

            outDgramList.push_back(Datagram(buffer.data() + recordStart, buffer.data() + recordEnd));

            recordStart = recordEnd;
            recordEnd += (lengthOffsetInDTLSRecord + sizeof(AZ::u16));
        }

        // If a deserialization error occurred (i.e. not all bytes in the buffer were read) all datagrams
        // after the malformation are discarded. It's assumed that the BIO buffer only contained complete
        // DTLS record datagrams.
        bool isSuccess = (recordStart == buffer.size());
        AZ_Assert(isSuccess, "Malformed DTLS record found, dropping remaining records in the buffer (%d bytes lost).\n", (buffer.size() - recordStart));
        return isSuccess;
    }

    void SecureSocketDriver::Connection::Update()
    {
        if (!m_sm.IsDispatching())
        {
            m_sm.Dispatch(ConnectionEvents::CE_UPDATE);
        }
    }

    void SecureSocketDriver::Connection::AddDgram(const char* data, AZ::u32 dataSize)
    {
        m_inPlainQueue.push(Datagram(data, data + dataSize));
    }

    void SecureSocketDriver::Connection::AddDTLSDgram(const char* data, AZ::u32 dataSize)
    {
        if (ConnectionSecurity::IsHelloRequestHandshake(data, dataSize) && m_sm.GetCurrentState() == ConnectionState::CS_CONNECT)
        {
            SSL_CTX* ctx = SSL_get_SSL_CTX(m_ssl);
            Shutdown();
            Initialize(ctx, ConnectionState::CS_CONNECT, m_mtu);
            SSL_set_connect_state(m_ssl);
        }
        else
        {
            BIO_write(m_inDTLSBuffer, data, dataSize);
        }
    }

    AZ::u32 SecureSocketDriver::Connection::GetDTLSDgram(char* data, AZ::u32 dataSize)
    {
        AZ::u32 dgramSize = 0;
        if (!m_outDTLSQueue.empty())
        {
            const Datagram& dgram = m_outDTLSQueue.front();
            if (dgram.size() <= dataSize)
            {
                memcpy(data, dgram.data(), dgram.size());
                dgramSize = static_cast<AZ::u32>(dgram.size());
            }
            else
            {
                AZ_TracePrintf("GridMateSecure", "Dropped datagram of %d bytes.\n", dgram.size());
            }

            m_outDTLSQueue.pop();
        }

        return dgramSize;
    }

    bool SecureSocketDriver::Connection::IsDisconnected() const
    {
        return m_sm.GetCurrentState() == CS_DISCONNECTED;
    }

    SecureSocketDriver::SecureSocketDriver(const SecureSocketDesc& desc, bool isFullPackets, bool crossPlatform)
        : SocketDriver(isFullPackets, crossPlatform)
        , m_dh(nullptr)
        , m_privateKey(nullptr)
        , m_certificate(nullptr)
        , m_sslContext(nullptr)
        , m_tempSocketBuffer(nullptr)
        , m_maxTempBufferSize(10 * 1024)
        , m_desc(desc)
    {
        m_tempSocketBuffer = static_cast<char*>(azmalloc(m_maxTempBufferSize));
    }

    SecureSocketDriver::~SecureSocketDriver()
    {
        for (AZStd::pair<SocketDriverAddress, Connection*>& addrConn : m_connections)
        {
            delete addrConn.second;
        }

        if (m_tempSocketBuffer)
        {
            azfree(m_tempSocketBuffer);
            m_tempSocketBuffer = nullptr;
        }

        if (m_certificate)
        {
            X509_free(m_certificate);
            m_certificate = nullptr;
        }

        if (m_privateKey)
        {
            EVP_PKEY_free(m_privateKey);
            m_privateKey = nullptr;
        }

        if (m_dh)
        {
            DH_free(m_dh);
            m_dh = nullptr;
        }

        if (m_sslContext)
        {
            // Calls to SSL_CTX_free() also free any attached X509_STORE objects.
            SSL_CTX_free(m_sslContext);
            m_sslContext = nullptr;
        }
    }

    AZ::u32 SecureSocketDriver::GetMaxSendSize() const
    {
        // This is the size of the DTLS header when sending application data. The DTLS
        // header during handshake is larger but isn't relevant here since the user can
        // only send data as application data and never during the handshake.
        static const AZ::u32 sDTLSHeader = DTLS1_RT_HEADER_LENGTH;

        // An additional overhead, as a result of encrypting the data (padding, etc), needs
        // to be calculated and added.
        //NOTE: This value was determined by looking at different sized DTLS datagrams, but
        //      needs some more thought put into it based on the cipher type and mode of operation
        //      (i.e. AES and GCM).
        static const AZ::u32 sCipherOverhead = 30;

        return SocketDriverCommon::GetMaxSendSize() - sDTLSHeader - sCipherOverhead;
    }

    Driver::ResultCode SecureSocketDriver::Initialize(AZ::s32 ft, const char* address, AZ::u32 port, bool isBroadcast, AZ::u32 receiveBufferSize, AZ::u32 sendBufferSize)
    {
        if (m_desc.m_privateKeyPEM != nullptr && m_desc.m_certificatePEM == nullptr)
        {
            AZ_TracePrintf("GridMateSecure", "If a private key is provided, so must a corresponding certificate.\n");
            return EC_SECURE_CONFIG;
        }

        if (m_desc.m_certificatePEM != nullptr && m_desc.m_privateKeyPEM == nullptr)
        {
            AZ_TracePrintf("GridMateSecure", "If a certificate is provided, so must a corresponding private key.\n");
            return EC_SECURE_CONFIG;
        }

        ResultCode result = SocketDriver::Initialize(ft, address, port, isBroadcast, receiveBufferSize, sendBufferSize);
        if (result != EC_OK)
        {
            return result;
        }

        SSL_library_init();
        ERR_load_SSL_strings();

        m_sslContext = SSL_CTX_new(DTLSv1_2_method());
        if (m_sslContext == nullptr)
        {
            return EC_SECURE_CREATE;
        }

        // Disable automatic MTU discovery so it can be set explicitly in SecureSocketDriver::Connection.
        SSL_CTX_set_options(m_sslContext, SSL_OP_NO_QUERY_MTU | SSL_OP_COOKIE_EXCHANGE);

        // Only support a single cipher suite in OpenSSL that supports:
        //
        //  ECDHE       Master key exchange using ephemeral elliptic curve diffie-hellman.
        //  RSA         Authentication (public and private key) used to sign ECDHE parameters and can be checked against a CA.
        //  AES256      AES cipher for symmetric key encryption using a 256-bit key.
        //  GCM         Mode of operation for symmetric key encryption.
        //  SHA384      SHA-2 hashing algorithm.
        if (SSL_CTX_set_cipher_list(m_sslContext, "ECDHE-RSA-AES256-GCM-SHA384") != 1)
        {
            return EC_SECURE_CREATE;
        }

        // Automatically generate parameters for elliptic-curve diffie-hellman (i.e. curve type and coefficients).
        SSL_CTX_set_ecdh_auto(m_sslContext, 1);

        if (m_desc.m_certificatePEM)
        {
            m_certificate = CreateCertificateFromEncodedPEM(m_desc.m_certificatePEM);
            if (m_certificate == nullptr || SSL_CTX_use_certificate(m_sslContext, m_certificate) != 1)
            {
                return EC_SECURE_CERT;
            }
        }

        if (m_desc.m_privateKeyPEM)
        {
            m_privateKey = CreatePrivateKeyFromEncodedPEM(m_desc.m_privateKeyPEM);
            if (m_privateKey == nullptr || SSL_CTX_use_PrivateKey(m_sslContext, m_privateKey) != 1)
            {
                return EC_SECURE_PKEY;
            }
        }

        // Determine if both client and server must be authenticated or only the server.
        // The default behavior only authenticates the server, and not the client.
        int verificationMode = SSL_VERIFY_PEER;
        if (m_desc.m_authenticateClient)
        {
            verificationMode = SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        }

        if (m_desc.m_certificateAuthorityPEM)
        {
            // SSL context should already have empty cert storage
            X509_STORE* caLocalStore = SSL_CTX_get_cert_store(m_sslContext);
            if (caLocalStore == nullptr)
            {
                return EC_SECURE_CA_CERT;
            }

            AZStd::vector<X509*> certificateChain;
            CreateCertificateChainFromEncodedPEM(m_desc.m_certificateAuthorityPEM, certificateChain);
            if (certificateChain.size() == 0)
            {
                return EC_SECURE_CA_CERT;
            }

            for (auto certificate : certificateChain)
            {
                X509_STORE_add_cert(caLocalStore, certificate);
            }
            SSL_CTX_set_verify(m_sslContext, verificationMode, nullptr);
        }
        else
        {
            SSL_CTX_set_verify(m_sslContext, verificationMode, SecureSocketDriver::VerifyCertificate);
        }

        if (!SSL_CTX_set_ex_data(m_sslContext, kSSLContextDriverPtrArg, this))
        {
            AZ_TracePrintf("GridMateSecure", "Failed to set driver for ssl context");
            return EC_SECURE_CREATE;
        }

        //Generate the initial key
        RotateCookieSecret(true);

        /* Set DTLS cookie generation and verification callbacks */
        SSL_CTX_set_cookie_generate_cb(m_sslContext, [](SSL* ssl, unsigned char* cookie, unsigned int* cookieLen)
        {
            SSL_CTX* ctx = SSL_get_SSL_CTX(ssl);
            if (!ctx)
            {
                return 0;
            }

            void* ptr = SSL_CTX_get_ex_data(ctx, kSSLContextDriverPtrArg);
            if (!ptr)
            {
                return 0;
            }

            return static_cast<SecureSocketDriver*>(ptr)->GenerateCookie(ssl, cookie, cookieLen);
        });


        SSL_CTX_set_cookie_verify_cb(m_sslContext, [](SSL* ssl, unsigned char* cookie, unsigned int cookieLen)
        {
            SSL_CTX* ctx = SSL_get_SSL_CTX(ssl);
            if (!ctx)
            {
                return 0;
            }

            void* ptr = SSL_CTX_get_ex_data(ctx, kSSLContextDriverPtrArg);
            if (!ptr)
            {
                return 0;
            }

            return static_cast<SecureSocketDriver*>(ptr)->VerifyCookie(ssl, cookie, cookieLen);
        });

        return EC_OK;
    }

    void SecureSocketDriver::Update()
    {
        FlushSocketToConnectionBuffer();
        UpdateConnections();
        FlushConnectionBuffersToSocket();
    }

    AZ::u32 SecureSocketDriver::Receive(char* data, AZ::u32 maxDataSize, AddrPtr& from, ResultCode* resultCode)
    {
        if (m_globalInQueue.empty())
        {
            if (resultCode)
            {
                *resultCode = EC_OK;
            }
            return 0;
        }

        const Datagram& datagram = m_globalInQueue.front().first;
        const AddrPtr& fromAddr = m_globalInQueue.front().second;
        if (datagram.size() <= maxDataSize)
        {
            memcpy(data, datagram.data(), datagram.size());
            if (resultCode)
            {
                *resultCode = EC_OK;
            }
            from = fromAddr;
            m_globalInQueue.pop();
            return static_cast<AZ::u32>(datagram.size());
        }

        AZ_TracePrintf("GridMateSecure", "Dropped datagram of %d bytes from %s.\n", datagram.size(), fromAddr->ToString().c_str());
        if (resultCode)
        {
            *resultCode = EC_RECEIVE;
        }
        m_globalInQueue.pop();
        return 0;
    }

    bool SecureSocketDriver::RotateCookieSecret(bool force /*= false*/)
    {
        TimeStamp currentTime = AZStd::chrono::system_clock::now();

        // Get the last time since we updated our secret.
        AZ::u64 durationSinceLastMS = AZStd::chrono::milliseconds(currentTime - m_cookieSecret.m_lastSecretGenerationTime).count();

        if (force || durationSinceLastMS > kDTLSSecretExpirationTime)
        {
            m_cookieSecret.m_lastSecretGenerationTime = currentTime;
            //Should we copy the old key to keep it just in case there's a handshake with the old secret?
            if (durationSinceLastMS < 2 * kDTLSSecretExpirationTime)
            {
                memcpy(m_cookieSecret.m_previousSecret, m_cookieSecret.m_currentSecret, sizeof(m_cookieSecret.m_previousSecret));
                m_cookieSecret.m_isPreviousSecretValid = true;
            }
            else
            {
                memset(m_cookieSecret.m_previousSecret, 0, sizeof(m_cookieSecret.m_previousSecret));
                m_cookieSecret.m_isPreviousSecretValid = false;
            }
            int cookieGeneration = RAND_bytes(m_cookieSecret.m_currentSecret, sizeof(m_cookieSecret.m_currentSecret));
            m_cookieSecret.m_isCurrentSecretValid = true;
            AZ_Verify(cookieGeneration == 1, "Failed to generate the cookie");
            return cookieGeneration == 1;
        }
        return true;
    }

    Driver::ResultCode SecureSocketDriver::Send(const AddrPtr& to, const char* data, AZ::u32 dataSize)
    {
        SocketDriverAddress* connKey = static_cast<SocketDriverAddress*>(to.get());
        Connection* connection = nullptr;
        auto connItr = m_connections.find(*connKey);
        if (connItr == m_connections.end())
        {
            connection = aznew Connection(to, m_maxTempBufferSize, &m_globalInQueue, m_desc.m_connectionTimeoutMS);
            if (connection->Initialize(m_sslContext, ConnectionState::CS_CONNECT, SecureSocketDriver::GetMaxSendSize()))
            {
                m_connections[*connKey] = connection;
            }
            else
            {
                delete connection;
                connection = nullptr;
                return EC_SEND;
            }
        }
        else
        {
            connection = connItr->second;
        }

        connection->AddDgram(data, dataSize);

        return EC_OK;
    }

    bool SecureSocketDriver::GetPeerAddress(SSL* ssl, string& addr) const
    {
        addr = string();

        if (!ssl)
        {
            return false;
        }

        auto it = m_sslToAddress.find(ssl);
        if (it == m_sslToAddress.end())
        {
            return false;
        }

        addr = it->second;
        return true;
    }

    int SecureSocketDriver::VerifyCertificate(int ok, X509_STORE_CTX* ctx)
    {
        // Called when a certificate has been received and needs to be verified (e.g.
        // verify that it has been signed by the appropriate CA, has the correct
        // hostname, etc).

        (void)ok;
        (void)ctx;
        return 1;
    }


    int SecureSocketDriver::GenerateCookie(SSL* ssl, unsigned char* cookie, unsigned int* cookieLen)
    {
        const unsigned int cookieMaxLen = *cookieLen;
        (void)cookieMaxLen;

        *cookieLen = 0;

        if (cookie == nullptr)
        {
            AZ_TracePrintf("GridMateSecure", "Cookie is nullptr, it needs to be allocated");
            return 0;
        }

        if (!RotateCookieSecret())
        {
            AZ_TracePrintf("GridMateSecure", "Failed to rotate secret\n");
            return 0;
        }

        string addrStr;
        if (!GetPeerAddress(ssl, addrStr))
        {
            AZ_TracePrintf("GridMateSecure", "Failed to get address for ssl instance %p\n", ssl);
            return 0;
        }

        // Calculate HMAC of buffer using the secret and peer address
        unsigned char result[EVP_MAX_MD_SIZE];
        unsigned int resultLen = 0;
        HMAC(EVP_sha1(), m_cookieSecret.m_currentSecret, sizeof(m_cookieSecret.m_currentSecret),
            reinterpret_cast<const unsigned char*>(addrStr.c_str()), addrStr.length(),
            result, &resultLen);

        if (resultLen > DTLS1_COOKIE_LENGTH)
        {
            AZ_TracePrintf("GridMateSecure", "Insufficient cookie buffer: %u > %u\n", resultLen, cookieMaxLen);
            return 0;
        }

        memcpy(cookie, result, resultLen);
        *cookieLen = resultLen;
        return 1;
    }

    int SecureSocketDriver::VerifyCookie(SSL* ssl, unsigned char* cookie, unsigned int cookieLen)
    {
        if (cookie == nullptr)
        {
            AZ_TracePrintf("GridMateSecure", "Cookie is nullptr");
            return 0;
        }
        string addrStr;
        if (!GetPeerAddress(ssl, addrStr))
        {
            AZ_TracePrintf("GridMateSecure", "Failed to get address for ssl instance %p\n", ssl);
            return 0;
        }

        if (!m_cookieSecret.m_isCurrentSecretValid)
        {
            AZ_TracePrintf("GridMateSecure", "Secret not initialized, can't verify cookie\n");
            return 0;
        }

        // Calculate HMAC of buffer using the secret and peer address
        unsigned char result[EVP_MAX_MD_SIZE];
        unsigned int resultLen = 0;
        HMAC(EVP_sha1(), m_cookieSecret.m_currentSecret, COOKIE_SECRET_LENGTH,
            reinterpret_cast<const unsigned char*>(addrStr.c_str()), addrStr.length(),
            result, &resultLen);

        if (cookieLen == resultLen && memcmp(result, cookie, resultLen) == 0)
        {
            return 1;
        }

        //This part was added to check for older handshakes, it only allows checks for 1 secret that's maximum 2* max life of key
        if (m_cookieSecret.m_isPreviousSecretValid)
        {
            /* Calculate HMAC of buffer using the secret */
            HMAC(EVP_sha1(), m_cookieSecret.m_previousSecret, COOKIE_SECRET_LENGTH,
                reinterpret_cast<const unsigned char*>(addrStr.c_str()), addrStr.length(),
                result, &resultLen);

            if (cookieLen == resultLen && memcmp(result, cookie, resultLen) == 0)
            {
                return 1;
            }
        }

        AZ_Printf("GridMate", "Failed to validate the cookie for %s\n", addrStr.c_str());
        return 0;
    }

    void SecureSocketDriver::FlushSocketToConnectionBuffer()
    {
        while (true)
        {
            AddrPtr from;
            ResultCode result;
            AZ::u32 bytesReceived = SocketDriver::Receive(m_tempSocketBuffer, m_maxTempBufferSize, from, &result);
            if (result != EC_OK || bytesReceived == 0)
            {
                break;
            }

            Connection* connection = nullptr;
            SocketDriverAddress* connKey = static_cast<SocketDriverAddress*>(from.get());
            const auto itConnection = m_connections.find(*connKey);
            if (itConnection != m_connections.end())
            {
                connection = itConnection->second;
            }
            else
            {
                const auto nextAction = ConnectionSecurity::DetermineHandshakeState(m_tempSocketBuffer, bytesReceived);
                if (nextAction == ConnectionSecurity::NextAction::SendHelloVerifyRequest)
                {
                    char buffer[ConnectionSecurity::kMaxPacketSize];
                    WriteBufferStaticInPlace writer(EndianType::BigEndian, buffer, sizeof(buffer));
                    AZ::u32 packetSize = ConnectionSecurity::PrepareHelloVerifyRequest(writer, from, &m_cookieSecret.m_currentSecret[0], sizeof(m_cookieSecret.m_currentSecret));
                    if (packetSize == 0 || (SocketDriver::Send(from, buffer, packetSize) != EC_OK))
                    {
                        AZ_TracePrintf("GridMateSecure", "Could not send HelloRequest");
                    }
                }
                else if (nextAction == ConnectionSecurity::NextAction::MakeNewConnection)
                {
                    char buffer[ConnectionSecurity::kMaxPacketSize];
                    WriteBufferStaticInPlace writer(EndianType::BigEndian, buffer, sizeof(buffer));
                    AZ::u32 packetSize = ConnectionSecurity::PrepareHelloRequest(m_tempSocketBuffer, bytesReceived, writer, from, &m_cookieSecret.m_currentSecret[0], sizeof(m_cookieSecret.m_currentSecret));
                    if (packetSize == 0 || (SocketDriver::Send(from, buffer, packetSize) != EC_OK))
                    {
                        AZ_TracePrintf("GridMateSecure", "Did not process HelloClient with cookie");
                        return;
                    }
                    connection = aznew Connection(from, m_maxTempBufferSize, &m_globalInQueue, m_desc.m_connectionTimeoutMS);
                    if (connection->Initialize(m_sslContext, ConnectionState::CS_ACCEPT, SecureSocketDriver::GetMaxSendSize()))
                    {
                        m_connections[*connKey] = connection;
                        m_sslToAddress[connection->GetSSLContext()] = from->ToAddress();
                        return; // skip this Client Hello for now... SSL_accept will process the next sequence of handshake packets
                    }
                    else
                    {
                        delete connection;
                        connection = nullptr;
                    }
                }
                else
                {
                    AZ_TracePrintf("GridMateSecure", "ERROR processing handshake packet");
                }
            }

            if (connection)
            {
                connection->AddDTLSDgram(m_tempSocketBuffer, bytesReceived);
            }
        }
    }

    void SecureSocketDriver::UpdateConnections()
    {
        for (AZStd::pair<SocketDriverAddress, Connection*>& addrConn : m_connections)
        {
            addrConn.second->Update();
        }

        auto itr = m_connections.begin();
        while (itr != m_connections.end())
        {
            AZStd::pair<SocketDriverAddress, Connection*> addrConn = *itr;
            itr++;
            if (addrConn.second->IsDisconnected())
            {
                m_sslToAddress.erase(addrConn.second->GetSSLContext());
                delete addrConn.second;
                m_connections.erase(addrConn.first);
            }
        }
    }

    void SecureSocketDriver::FlushConnectionBuffersToSocket()
    {
        for (AZStd::pair<SocketDriverAddress, Connection*>& addrConn : m_connections)
        {
            Connection* connection = addrConn.second;
            while (true)
            {
                AZ::s32 bytesRead = connection->GetDTLSDgram(m_tempSocketBuffer, m_maxTempBufferSize);
                if (bytesRead <= 0)
                {
                    break;
                }

                DriverAddress* addr = static_cast<DriverAddress*>(&addrConn.first);
                SocketDriver::Send(addr, m_tempSocketBuffer, bytesRead);
            }
        }
    }
}
#endif

#endif