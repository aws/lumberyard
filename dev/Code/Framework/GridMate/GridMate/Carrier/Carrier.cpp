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

#include <limits>

#include <GridMate/Types.h>

#include <GridMate/Carrier/Carrier.h>
#include <GridMate/Carrier/Compressor.h>
#include <GridMate/Carrier/Cripter.h>
#include <GridMate/Carrier/DefaultHandshake.h>
#include <GridMate/Carrier/DefaultTrafficControl.h>
#include <GridMate/Carrier/Simulator.h>
#include <GridMate/Carrier/SocketDriver.h>

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/DataMarshal.h>
#include <GridMate/Serialize/UtilityMarshal.h>

#include <GridMate/Containers/vector.h>
#include <GridMate/Containers/list.h>
#include <GridMate/Containers/queue.h>
#include <GridMate/Containers/unordered_set.h>

#include <AzCore/std/containers/intrusive_slist.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/queue.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/algorithm.h>

// Enable to insert Crc checks for each message
#if defined(AZ_DEBUG_BUILD)
//#define GM_CARRIER_MESSAGE_CRC
#endif // AZ_DEBUG_BUILD

#if defined(GM_CARRIER_MESSAGE_CRC)
    #include <AzCore/Math/Crc.h>
#endif // GM_CARRIER_MESSAGE_CRC

namespace GridMate
{
    static const EndianType kCarrierEndian = EndianType::BigEndian;

    static const int m_maxNumberOfChannels = 4;
    static const int m_systemChannel = 3;

    class CarrierThread;

    /**
     * Carrier message. This is the smallest unit of data
     * we transport in the carrier.
     */
    struct MessageData
        : public AZStd::intrusive_list_node<MessageData>
    {
        GM_CLASS_ALLOCATOR(MessageData); // make a pool and use it...

        MessageData()
            : m_data(nullptr)
        {}
        enum MessageFlags
        {
            MF_RELIABLE         = (1 << 0),
            MF_CHUNKS           = (1 << 2),       ///< Flag that we have more than 1 (default chunks) to form the full message.
            MF_SQUENTIAL_ID     = (1 << 3),
            MF_SQUENTIAL_REL_ID = (1 << 4),
            MF_DATA_CHANNEL     = (1 << 5),

            MF_CONNECTING       = (1 << 7),       ///< Set when we are in connection state (trying to connect). This is used to reject generic datagrams/messages
        };

        Carrier::DataReliability m_reliability;
        unsigned char   m_channel;              ///< channel for sending
        SequenceNumber  m_numChunks;            ///< number of chunks/messages that we need to assemble for this message [1..N]

        SequenceNumber  m_sequenceNumber;       ///< Message sequence number.
        SequenceNumber  m_sendReliableSeqNum;   ///< Reliable sequence number. Valid if the reliability is RELIABLE.
        void*           m_data;
        AZ::u16         m_dataSize;
        bool            m_isConnecting;         ///< True if this message is generated while we are in connecting state, otherwise false.
        AZStd::unique_ptr<CarrierACKCallback> m_ackCallback;    ///< After receiving an ACK execute the callback
    };

    typedef AZStd::intrusive_list<MessageData, AZStd::list_base_hook<MessageData> > MessageDataListType;

    /**
     * Carrier data gram. A group of \ref MessageData.
     */
    struct DatagramData
        : public AZStd::intrusive_slist_node<DatagramData>
    {
        GM_CLASS_ALLOCATOR(DatagramData); // make a pool and use it...

        TrafficControl::DataGramControlData m_flowControl;
        unsigned short      m_resendDataSize;   ///< Size of the data in the toResend list (not including any headers, just the sum of messages data)

        MessageDataListType m_toResend[Carrier::PRIORITY_MAX];  ///< A list of all reliable messages that were part if the datagram. We might need to resend them.
        AZStd::vector<AZStd::unique_ptr<CarrierACKCallback> > m_ackCallbacks;
    };

    typedef AZStd::intrusive_slist<DatagramData, AZStd::slist_base_hook<DatagramData> > DatagramDataListType;

    struct OutgoingDataGramContext
    {
        SequenceNumber lastSequenceNumber[m_maxNumberOfChannels];
        SequenceNumber lastSeqReliableNumber[m_maxNumberOfChannels];
        bool isWrittenFirstSequenceNum[m_maxNumberOfChannels];
        bool isWrittenFirstRelSeqNum[m_maxNumberOfChannels];

        OutgoingDataGramContext()
        {
            memset(lastSequenceNumber, 0, sizeof(lastSequenceNumber));
            memset(lastSeqReliableNumber, 0, sizeof(lastSeqReliableNumber));
            memset(isWrittenFirstSequenceNum, 0, sizeof(isWrittenFirstSequenceNum));
            memset(isWrittenFirstRelSeqNum, 0, sizeof(isWrittenFirstRelSeqNum));
        }
    };


    /**
     * Datagram history container is a specialized container that handles a datagram history list.
     * it's based on a fixed ring buffer that in addition handles the unique datagram history data.
     * For example when you call GetForAck it will remove the element from the buffer automatically
     * if the datagram has acked enough times.
     */
    struct DataGramHistoryList
    {
        static const unsigned char m_datagramHistoryMaxNumberOfAck = 3; // We confirm a datagram 3 times before we remove it from the history queue.
        static const unsigned char m_datagramHistoryMaxNumberOfBytesMask = 127; // A bit mask for the first 7 bits of a byte.
        static const unsigned char m_datagramHistoryMaxNumberOfBytes = 64; // The maximum number of bytes that is power of 2 and can be counted in 7 bits.
        static const unsigned int m_dataGramHistoryIDSize = m_datagramHistoryMaxNumberOfBytes * 8; ///< How long we should keep a history of datagrams, max 1 bit for each datagram.
        static const unsigned short m_maxNumberOfElements = m_dataGramHistoryIDSize;

        struct Element
        {
            SequenceNumber m_sequenceNumber = 0;
            int            m_numAcksSend = -1;       ///< -1 if the slot is not used
        };

        DataGramHistoryList()
            : m_first(0)
            , m_last(0)
            , m_numActiveElements(0) { /*m_array.resize(MaxNumberOfElements);*/ }

        /// Return the number of valid elements (datagrams) in the history list.
        inline size_t Size() const { return m_numActiveElements; }
        /// Retrieve a datagram data from a specific slow. It's important to note that data MAY NOT BE VALID, use IsValid function to check if the slot contains valid data.
        inline SequenceNumber At(size_t offset) const { return m_array[offset].m_sequenceNumber; }
        /// Get a datagram for an ACK. In addition we count the number of acks and if it's enough, the element will be removed from the buffer.
        inline SequenceNumber GetForAck(size_t offset, size_t* nextOffset = nullptr)
        {
            Element& el = m_array[offset];
            SequenceNumber toAck = el.m_sequenceNumber;
            el.m_numAcksSend++;
            if (el.m_numAcksSend >= m_datagramHistoryMaxNumberOfAck)  // remove if enough acks are sent
            {
                size_t nextValidOffset = Remove(offset);
                if (nextOffset)
                {
                    * nextOffset = nextValidOffset;
                }
            }
            else
            {
                if (nextOffset)
                {
                    * nextOffset = Add(offset);
                }
            }

            return toAck;
        }
        /// Check if a specific offset is with-in the range of valid entries (this doesn't mean it has a valid data, just that it's in the range.)
        inline bool IsInRange(size_t offset) const
        {
            if (m_first == m_last)
            {
                return m_numActiveElements != 0; // either full or empty
            }
            else if (m_first < m_last)
            {
                return offset >= m_first && offset < m_last;
            }
            else
            {
                return offset >= m_first || offset < m_last;
            }
        }

        /// Same as IsInRange but based on datagram id, not offset.
        inline bool IsInRangeId(SequenceNumber id) const
        {
            if (m_numActiveElements == 0)
            {
                return false;
            }

            SequenceNumber beforeFirst = m_array[m_first].m_sequenceNumber - (SequenceNumber)1;
            SequenceNumber afterLast = m_array[Last()].m_sequenceNumber + (SequenceNumber)1;

            if (SequenceNumberLessThan(beforeFirst, id) && SequenceNumberGreaterThan(afterLast, id))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// True if the buffer is full. When empty or full Begin()==End() so you can use IsFull to distinguish between those cases.
        inline bool IsFull() const { return (m_first == m_last) && (m_numActiveElements != 0); }
        /// Increments an offset considering the looping in the buffer.
        inline void Increment(size_t& num) const
        {
            if (num == m_maxNumberOfElements - 1)
            {
                num = 0;
            }
            else
            {
                ++num;
            }
        }

        /// Decrements an offset considering the looping in the buffer.
        inline void Decrement(size_t& num) const
        {
            if (num == 0)
            {
                num = m_maxNumberOfElements - 1;
            }
            else
            {
                --num;
            }
        }

        /// \ref Decrement
        inline size_t Sub(size_t num) const
        {
            if (num == 0)
            {
                return m_maxNumberOfElements - 1;
            }
            else
            {
                return num - 1;
            }
        }

        /// \ref Increment
        inline size_t Add(size_t num) const
        {
            if (num == m_maxNumberOfElements - 1)
            {
                return 0;
            }
            else
            {
                return num + 1;
            }
        }

        /// Clear a specific slot and adjust Begin/End if needed
        size_t Remove(size_t offset)  // return the next valid offset
        {
            AZ_Assert(IsInRange(offset), "Offset outside valid range!");
            if (m_array[offset].m_numAcksSend != -1)
            {
                --m_numActiveElements;
                m_array[offset].m_numAcksSend = -1; // reset element
            }
            if (m_numActiveElements == 0)
            {
                m_first = m_last;
                return m_last;
            }
            else if (m_first == offset)
            {
                while (m_array[m_first].m_numAcksSend == -1)
                {
                    Increment(m_first);
                }
                return m_first;
            }
            else if (Last() == offset)
            {
                while (m_array[Last()].m_numAcksSend == -1)
                {
                    Decrement(m_last);
                }
                return m_last;
            }
            return Add(offset);
        }

        /// Inserts an element in the buffer. This function will override existing slot if needed. It returns false only if the element is already in the list.
        inline bool Insert(SequenceNumber id)
        {
            SequenceNumber offset = id % m_maxNumberOfElements;
            if (m_numActiveElements)
            {
                if (SequenceNumberLessThan(m_array[Last()].m_sequenceNumber, id))
                {
                    // compute where the first should be at minimum (last number - max number of stored elements)
                    SequenceNumber newFirst = id - (m_maxNumberOfElements - 1);
                    while (m_numActiveElements != 0 && SequenceNumberLessThan(m_array[m_first].m_sequenceNumber, newFirst))
                    {
                        if (m_array[m_first].m_numAcksSend != -1)
                        {
                            --m_numActiveElements;
                            m_array[m_first].m_numAcksSend = -1; // discard older elements
                        }
                        Increment(m_first);
                    }

                    size_t newLast = Add(offset);
                    if (m_numActiveElements == 0)
                    {
                        m_first = offset;
                        m_last = newLast;
                    }
                    else
                    {
                        while (m_last != newLast)
                        {
                            AZ_Assert(m_array[m_last].m_numAcksSend == -1, "Found a slot-in-use while advancing m_last. This indicates that we are about to stomp a slot in use!");
                            Increment(m_last);
                        }
                    }
                    ++m_numActiveElements;

                    // set the element
                    m_array[Last()].m_sequenceNumber = id;
                    m_array[Last()].m_numAcksSend = 0;
                }
                else if (IsInRangeId(id))
                {
                    if (m_array[offset].m_sequenceNumber == id)
                    {
                        return false; // already inserted
                    }
                    if (m_array[offset].m_numAcksSend == -1)
                    {
                        ++m_numActiveElements;
                    }
                    m_array[offset].m_sequenceNumber = id;
                    m_array[offset].m_numAcksSend = 0;
                }
                else
                {
                    // it's too old and we don't care
                }
            }
            else
            {
                m_first = offset;
                m_last = Add(offset);
                m_numActiveElements = 1;
                m_array[offset].m_sequenceNumber = id;
                m_array[offset].m_numAcksSend = 0;
            }
            AZ_Assert(m_numActiveElements <= m_maxNumberOfElements, "Carrier ring buffer overflow, data loss has occurred.");
            return true;
        }

        inline size_t           Begin() const                   { return m_first; }
        inline size_t           End() const                     { return m_last; }
        inline size_t           Last() const                    { return Sub(m_last); }
        inline bool             IsValid(size_t offset) const    { return (m_array[offset].m_numAcksSend != -1); }

        inline SequenceNumber   First() const { return m_array[m_first].m_sequenceNumber; }

        Element                 m_array[m_maxNumberOfElements];
        //AZStd::fixed_vector<Element,MaxNumberOfElements> m_array;
        size_t  m_first;
        size_t  m_last;
        size_t  m_numActiveElements;
    };

    /**
     * Carrier connection. This connection is used on the "main" thread only.
     * It stores all the connection specific data for the user which is
     * messages to send and receive.
     */
    struct Connection
        : public ConnectionCommon
    {
        GM_CLASS_ALLOCATOR(Connection); // make a pool and use it...

        Connection(CarrierThread* threadOwner, const string& address);
        ~Connection();

        CarrierThread*              m_threadOwner;                                  ///< Pointer to the carrier thread that operates with this connection.
        AZStd::atomic<struct ThreadConnection*> m_threadConn;                       ///< Pointer to a thread connection. You can use it in the main thread only for a reference.
        string                      m_fullAddress;                                  ///< Connection full address.

        Carrier::ConnectionStates   m_state;

        SequenceNumber              m_sendSeqNum[m_maxNumberOfChannels];            ///< Next message sequence number.
        SequenceNumber              m_sendReliableSeqNum[m_maxNumberOfChannels];    ///< Next reliable message sequence number.

        // TODO: Unless we use lockless queues for messages we should have minimize the amount of
        // interlocked or lock operations as they are expensive. One lock per connection or even one per update should be fine.

        AZStd::mutex                m_toSendLock;
        MessageDataListType         m_toSend[Carrier::PRIORITY_MAX];                ///< Send lists based on priority.
        AZStd::mutex                m_toReceiveLock;
        MessageDataListType         m_toReceive[m_maxNumberOfChannels];             ///< Received messages in order for the user to receive, sorted on a channel.

        AZStd::mutex                m_statsLock;
        TrafficControl::CongestionState m_congestionState;
        TrafficControl::Statistics  m_statsLastSecond;
        TrafficControl::Statistics  m_statsLifetime;
        TrafficControl::Statistics  m_statsEffectiveLastSecond;
        TrafficControl::Statistics  m_statsEffectiveLifetime;

        AZ::u32                     m_bytesInQueue = 0;
        bool                        m_rateLimitedByQueueSize = false;
    };

    struct PendingHandshake
    {
        struct Hasher : public AZStd::unary_function<PendingHandshake, AZStd::size_t>
        {
            AZ_FORCE_INLINE AZStd::size_t operator()(const PendingHandshake& h) const
            {
                static_assert(sizeof(AZStd::size_t) >= sizeof(h.m_connection), "Insufficient size for hash");
                union SizeAndHash
                {
                    Connection* conn;
                    AZStd::size_t hash;
                } u;

                u.hash = 0;
                u.conn = h.m_connection;
                return u.hash;
            }
        };

        struct EqualTo : public AZStd::binary_function<PendingHandshake, PendingHandshake, bool>
        {
            AZ_FORCE_INLINE bool operator()(const PendingHandshake& a, const PendingHandshake& b) const
            {
                return a.m_connection == b.m_connection;
            }
        };

        PendingHandshake()
            : m_connection(nullptr)
            , m_payload(kCarrierEndian, 64)
            , m_numRetries(0)
        {

        }
        explicit PendingHandshake(Connection* connection)
            : PendingHandshake()
        {
            m_connection = connection;
        }

        Connection* m_connection; // main connection
        WriteBufferDynamic m_payload; // Will keep handshake data to repeat conn request
        size_t m_numRetries; // Number of retries done for this handshake
        TimeStamp m_retryTime; // Next retry time
    };

    /**
     * Carrier thread connection. This connection is used on the "carrier" thread only.
     * It stores all the connection specific data for the carrier thread.
     */
    struct ThreadConnection
        : public TrafficControl::TrafficControlConnection
    {
        GM_CLASS_ALLOCATOR(ThreadConnection); // make a pool and use it...

        ThreadConnection(CarrierThread* threadOwner);
        ~ThreadConnection();

        CarrierThread*          m_threadOwner;                                      ///< Carrier thread that created this connection.

        Connection*             m_mainConnection;                                   ///< User to read-write messages to send/receive queue.

        AZStd::intrusive_ptr<DriverAddress> m_target;                               ///< Driver address of this connection.

        SequenceNumber          m_receivedSeqNum[m_maxNumberOfChannels];            ///< Last received seq number.
        SequenceNumber          m_receivedReliableSeqNum[m_maxNumberOfChannels];    ///< Last received reliable seq number.

        MessageDataListType::iterator m_receivedLastInsert[m_maxNumberOfChannels];  ///< Cached iterator in the received list when we insert packets.
        MessageDataListType::iterator m_receivedLastReliableChunk[m_maxNumberOfChannels];   ///< Cached iterator pointing to the last received sequential chunk (used for chucked messages only).
        MessageDataListType           m_received[m_maxNumberOfChannels];                    ///< Messages in process to be received by the main connection.

        DatagramDataListType    m_sendDataGrams;                                    ///< List with sent datagrams waiting for ACK.

        inline void PopReceived(unsigned char channel)
        {
            // Update cached iterators
            if (m_receivedLastInsert[channel] == m_received[channel].begin())
            {
                ++m_receivedLastInsert[channel];
            }
            m_received[channel].pop_front();
        }

        DataGramHistoryList     m_receivedDatagramsHistory;

        SequenceNumber          m_dataGramSeqNum;               ///< Last send datagram sequence number.
        SequenceNumber          m_lastAckedDatagram;            ///< Last datagram we send ack for.
        TimeStamp               m_lastReceivedDatagramTime;
        TimeStamp               m_createTime;                   ///< Connection create time or disconnect start time. Depending on the isDisconnecting flag.
        // add states
        bool                    m_isDisconnecting;
        bool                    m_isBadConnection;
        bool                    m_isDisconnected;
        bool                    m_isBadPackets;                  ///< Flag that indicates when we receive bad data packets.
        CarrierDisconnectReason m_disconnectReason; ///< Used as a temp storage for the reason till we execute it.
    };

    enum CarrierThreadMsg
    {
        CTM_CONNECT,
        CTM_DISCONNECT,
        CTM_DELETE_CONNECTION,
        CTM_HANDSHAKE_COMPLETE,
    };

    enum MainThreadMsg
    {
        MTM_NEW_CONNECTION,
        MTM_DISCONNECT,
        MTM_DISCONNECT_TIMEOUT,
        MTM_DELETE_CONNECTION,
        MTM_ON_ERROR,
        MTM_STATS_UPDATE,       ///< Currently unused
        MTM_RATE_UPDATE,        ///< Notification of connection rate change (congestion occurring or clearing up)
        MTM_ACK_NOTIFY,         ///< notify of packet ACK'd
    };

    /**
     * Used for communication between the main thread and the carrier thread.
     */
    struct ThreadMessage
    {
        GM_CLASS_ALLOCATOR(ThreadMessage); // make a pool and use it...

        ThreadMessage(MainThreadMsg mtm)
            : m_code(mtm)
            , m_connection(NULL)
            , m_threadConnection(NULL)
        {}
        ThreadMessage(CarrierThreadMsg ctm)
            : m_code(ctm)
            , m_connection(NULL)
            , m_threadConnection(NULL)
        {}

        int                     m_code;
        Connection*             m_connection;
        ThreadConnection*       m_threadConnection;

        string                  m_newConnectionAddress;
        CarrierErrorCode        m_errorCode;
        AZ::u32                 m_newRateBytesPerSec;           ///< new send rate
        AZStd::vector<AZStd::unique_ptr<CarrierACKCallback> > m_ackCallbacks;
        union
        {
            DriverError	        m_driverError;
            SecurityError       m_securityError;
        }                       m_error;
        CarrierDisconnectReason m_disconnectReason;
    };

    /**
     * System messages id, send on the SystemChannel
     */
    enum SystemMessageId
    {
        // Messages processed or main thread (when we call update)
        SM_CONNECT_REQUEST = 1, ///< Message send to initiate a connection, the data will contain the Handshake::GetWelcomeData.
        SM_CONNECT_ACK,         ///< Message send to ack a connection, the data will contain the Handshake::GetWelcomeData.
        SM_DISCONNECT,          ///< Message to indicate that the remote peer called disconnect.
        SM_CLOCK_SYNC,          ///< Message to synchronize the clock.

        // Messages processed on carrier thread (when the thread is processing incoming data
        SM_CT_FIRST,            ///< Not a real message, just indicator where Carrier thread messages will start.
        SM_CT_ACKS,             ///< Datagrams acks system message or just a keep alive packet if AHF_KEEP_ALIVE
        SM_CT_CONN_CONTROL,     ///< Connection control system message. (Window size, send rate, etc.)
        SM_CT_BANDWIDTH,        ///< Connection bandwidth data (possibly make part of SM_CT_CONN_CONTROL)
    };

    /**
     * Carrier thread implementation.
     */
    class CarrierThread
    {
    public:
        enum AckHistoryFlags
        {
            AHF_BITS            = (1 << 7),     ///< Set if the next 7 bits are number of bytes with ACKS bits data. 0-127 bytes mapping to 0-1016 ACK'd packets (each byte can ACK 8 packets).
            AHF_CONTINUOUS_ACK  = (1 << 6),     ///< Set if there is a first ACK packet ID after the flags (for CONTINUOUS acks)
            AHF_KEEP_ALIVE      = (1 << 5),     ///< Set if none of the first bits are set and there is no ACK data with this datagram. This flag is use in keep alive (heart beat)  packets.
        };

        GM_CLASS_ALLOCATOR(CarrierThread);

        CarrierThread(const CarrierDesc& desc, Compressor* compressor);
        ~CarrierThread();

        ThreadConnection*       MakeNewConnection(const AZStd::intrusive_ptr<DriverAddress>& driverAddress);

        void                    Quit();
        void                    ThreadPump();
        void                    UpdateReceive();
        void                    UpdateSend();
        void                    ProcessConnections();
        void                    UpdateStats();

        inline MessageData&     AllocateMessage();
        inline void             FreeMessage(MessageData& msg);

        inline DatagramData&    AllocateDatagram();
        inline void             FreeDatagram(DatagramData& dgram);

        inline void*            AllocateMessageData(unsigned int size);
        inline void             FreeMessageData(void* data, unsigned int size);

        inline unsigned int GetDataGramHeaderSize();
        inline void WriteDataGramHeader(WriteBuffer& writeBuffer, const DatagramData& dgram);
        inline SequenceNumber ReadDataGramHeader(ReadBuffer& readBuffer);
        bool IsConnectRequestDataGram(const char* data, size_t dataSize) const;

        void WriteAckData(ThreadConnection* connection, WriteBuffer& writeBuffer);
        void ReadAckData(ThreadConnection* connection, ReadBuffer& readBuffer);

        inline unsigned int GetMaxMessageHeaderSize() const;
        inline unsigned int GetMessageHeaderSize(const MessageData& msg, bool isWriteSeqNumber, bool isWriteReliableSeqNum, bool isWriteChannel) const;
        inline void WriteMessageHeader(WriteBuffer& writeBuffer, const MessageData& msg, bool isWriteSeqNumber, bool isWriteReliableSeqNum, bool isWriteChannel) const;
        inline bool ReadMessageHeader(ReadBuffer& readBuffer, MessageData& dgram, SequenceNumber* prevMsgSeqNum, SequenceNumber* prevReliableMsgSeqNum, unsigned char& channel) const;

        inline void ProcessIncomingDataGram(ThreadConnection* connection, DatagramData& dgram, ReadBuffer& readBuffer);
        inline void InitOutgoingDatagram(ThreadConnection* connection, WriteBuffer& writeBuffer);
        inline void GenerateOutgoingDataGram(ThreadConnection* connection, DatagramData& dgram, WriteBuffer& writeBuffer, OutgoingDataGramContext& ctx, size_t maxDatagramSize);
        //inline void ResendDataGram(DatagramData& dgram, WriteBuffer& writeBuffer);

        void OnReceivedIncomingDataGram(ThreadConnection* from, ReadBuffer& readBuffer, unsigned int recvDataGramSize);

        void SendSystemMessage(SystemMessageId id, WriteBuffer& wb, ThreadConnection* target);

        /// Return number of messages waiting to be send.
        inline bool HasDataToSend(ThreadConnection* connection);

        inline bool IsReady() const         { return m_quitThread; }

        Driver*                     m_driver;
        bool                        m_ownDriver;

        TrafficControl*             m_trafficControl;
        bool                        m_ownTrafficControl;

        unsigned int                m_handshakeTimeoutMS;

        CompressionFactory*         m_compressionFactory;
        Compressor*                 m_compressor;
        Simulator*                  m_simulator;

        unsigned int                m_maxDataGramSizeBytes;     ///< Maximum datagram size in bytes.
        unsigned int                m_maxMsgDataSizeBytes;      ///< Maximum message payload data size in bytes.

        bool                        m_enableDisconnectDetection;
        unsigned int                m_connectionTimeoutMS;  ///< Connection timeout in milliseconds.
        bool                        m_threadInstantResponse;

        float                       m_connectionEvaluationThreshold;    ///< \ref CarrierDesc

        unsigned int                m_maxConnections;

        // \todo Create datagram pool
        //forward_list<DatagramData*>::type m_dataGramPool;
        //AZStd::mutex              m_freeDataGramsLock;
        DatagramDataListType        m_freeDataGrams;
        AZStd::mutex                m_freeMessagesLock;
        MessageDataListType         m_freeMessages;

        typedef queue<void*>        MessageDataBlockQueueBlock;
        AZStd::mutex                m_freeDataBlocksLock;
        MessageDataBlockQueueBlock  m_freeDataBlocks;

        TimeStamp                   m_currentTime;
        AZStd::atomic<AZ::u64>      m_lastMainThreadUpdate;         ///< Used to detect main thread crashed and long time without updates.
        bool                        m_reportedDisconnect;           ///< Only log disconnect once to prevent excessive reporting during debugging

        //////////////////////////////////////////////////////////////////////////
        // TODO profile this, we can use lockless queue (ABA can't happen) or we can use command buffer (ring buffer) with event.
        AZ_FORCE_INLINE void PushCarrierThreadMessage(ThreadMessage* msg)
        {
            AZStd::lock_guard<AZStd::mutex> l(m_carrierMsgQueueLock);
            m_carrierMsgQueue.push(msg);
        }

        AZ_FORCE_INLINE void PushMainThreadMessage(ThreadMessage* msg)
        {
            AZStd::lock_guard<AZStd::mutex> l(m_mainMsgQueueLock);
            m_mainMsgQueue.push(msg);
        }

        AZ_FORCE_INLINE ThreadMessage* PopCarrierThreadMessage()
        {
            AZStd::lock_guard<AZStd::mutex> l(m_carrierMsgQueueLock);
            ThreadMessage* res = NULL;
            if (!m_carrierMsgQueue.empty())
            {
                res = m_carrierMsgQueue.front();
                m_carrierMsgQueue.pop();
            }
            return res;
        }

        AZ_FORCE_INLINE ThreadMessage* PopMainThreadMessage()
        {
            AZStd::lock_guard<AZStd::mutex> l(m_mainMsgQueueLock);
            ThreadMessage* res = NULL;
            if (!m_mainMsgQueue.empty())
            {
                res = m_mainMsgQueue.front();
                m_mainMsgQueue.pop();
            }
            return res;
        }
        //////////////////////////////////////////////////////////////////////////

        typedef queue<ThreadMessage*>    ThreadMessageQueueType;
        AZStd::mutex                m_carrierMsgQueueLock;
        ThreadMessageQueueType      m_carrierMsgQueue;          ///< Message queue for the carrier thread. Main Thread to Carrier Thread

        AZStd::mutex                m_mainMsgQueueLock;
        ThreadMessageQueueType      m_mainMsgQueue;             ///< Message queue for the main thread. Carrier Thread to Main Thread

        typedef list<ThreadConnection*> ThreadConnectionList;
        ThreadConnectionList        m_threadConnections;        ///< Connections active on the carrier thread.

        void NotifyRateUpdate(ThreadConnection* conn);          ///< Notifies connection rate changed

        AZStd::thread               m_thread;                   ///< Carrier thread. ...
        AZStd::atomic_bool          m_quitThread;
        AZStd::chrono::milliseconds m_threadSleepTime;

        // Temporary write buffer used trough the Carrier code. It's not thread safe and you should not store references to it in any way.
        // We hardcode the values to 64KiB, because this is the max theoretical size. Otherwise the max size is determined by the driver
        // as it's platform dependent.
        WriteBufferStatic<64* 1024> m_datagramTempWriteBuffer;

        vector<AZ::u8> m_compressionMem; // Temp buffer used for compression, basically acts like linear allocator for compressor. Should only be used on carrier thread.
    };

    //////////////////////////////////////////////////////////////////////////
    // Carrier Implementation
    // Keep it here so we have simpler looking interface and simpler includes
    class CarrierImpl
        : public Carrier
    {
    public:
        GM_CLASS_ALLOCATOR(CarrierImpl);

        CarrierImpl(const CarrierDesc& desc, IGridMate* gridMate);
        ~CarrierImpl() override;

        void            Shutdown() override;

        /// Connect with host and port. This is ASync operation, the connection is active after OnConnectionEstablished is called.
        ConnectionID    Connect(const char* hostAddress, unsigned int port) override;
        /// Connect with internal address format. This is ASync operation, the connection is active after OnConnectionEstablished is called.
        ConnectionID    Connect(const string& address) override;
        /// Request a disconnect procedure. This is ASync operation, the connection is closed after OnDisconnect is called.
        void            Disconnect(ConnectionID id) override;

        unsigned int    GetPort() const override                    { return m_port; }

        unsigned int    GetMessageMTU() override                    { return m_maxMsgDataSizeBytes; }

        string          ConnectionToAddress(ConnectionID id) override;

        void            SendWithCallback(const char* data, unsigned int dataSize, AZStd::unique_ptr<CarrierACKCallback> ackCallback, ConnectionID target = AllConnections, DataReliability reliability = SEND_RELIABLE, DataPriority priority = PRIORITY_NORMAL, unsigned char channel = 0) override;
        void            Send(const char* data, unsigned int dataSize, ConnectionID target = AllConnections, DataReliability reliability = SEND_RELIABLE, DataPriority priority = PRIORITY_NORMAL, unsigned char channel = 0) override
        {
            SendWithCallback(data, dataSize, AZStd::unique_ptr<CarrierACKCallback>(), target, reliability, priority, channel);
        };
        /**
        * Receive the data for the specific connection.
        * \note Internal buffers are user make sure you periodically receive data for all connections,
        * otherwise you might cause buffer overflow error.
        */
        ReceiveResult Receive(char* data, unsigned int maxDataSize, ConnectionID from, unsigned char channel = 0) override;

        void            Update() override;

        unsigned int        GetNumConnections() const override                      { return static_cast<unsigned int>(m_connections.size()); }

        ConnectionID        GetConnectionId(unsigned int index) const override      { return m_connections[index]; }
        //virtual ConnectionStates  GetConnectionState(unsigned int index) const    { return m_connections[index]->m_state; }
        //virtual ConnectionStates  GetConnectionState(ConnectionID id) const       { AZ_Assert(id!=InvalidConnectionID,"Invalid connection ID"); return static_cast<Connection*>(id)->m_state; }

        /**
        * Return connection statistics.
        * \param isLifetime if true we return stats for the connection lifetime, otherwise for the last 1 second (or LastUpdate?).
        */
        ConnectionStates    QueryStatistics(ConnectionID id, TrafficControl::Statistics* lastSecond = nullptr, TrafficControl::Statistics* lifetime = nullptr,
            TrafficControl::Statistics* effectiveLastSecond = nullptr, TrafficControl::Statistics* effectiveLifetime = nullptr,
            FlowInformation* flowInformation = nullptr) override;

        /**
        * Debug function, prints the connection status report to the stdout.
        */
        void            DebugStatusReport(ConnectionID id, unsigned char channel = 0) override;
        void            DebugDeleteConnection(ConnectionID id) override;
        void            DebugEnableDisconnectDetection(bool isEnabled) override;
        bool            DebugIsEnableDisconnectDetection() const override;

        //////////////////////////////////////////////////////////////////////////
        // Synchronized clock in milliseconds. It will wrap around ~49.7 days.
        /**
        * Enables sync of the clock every syncInterval milliseconds.
        * Only one peer in the connected grid can have the clock sync enabled, all the others will sync to it,
        * if you enable it on two or more an assert will occur.
        * \note syncInterval is just so the clocks stay is sync, GetTime will adjust the time using the last received sync value (or the creation of the carrier).
        * keep the synIterval high >= 1 sec. Systems will use their internal timer to adjust anyway, this will minimize the bandwidth usage
        */
        void            StartClockSync(unsigned int syncInterval = 1000, bool isReset = false) override;
        void            StopClockSync() override;
        /**
        * Returns current carrier time in milliseconds. If nobody syncs the clock the time will be relative
        * to the carrier creation.
        */
        AZ::u32 GetTime() override;
        //////////////////////////////////////////////////////////////////////////

        //private:
        void SendSystemMessage(SystemMessageId id, WriteBuffer& wb, ConnectionID target = AllConnections, DataReliability reliability = SEND_RELIABLE, bool isForValidConnectionsOnly = false);
        static bool     ReceiveSystemMessage(const char* srcData, unsigned int srcDataSize, SystemMessageId& msgId, ReadBuffer& rb);

        //
        void            DisconnectRequest(ConnectionID id, CarrierDisconnectReason reason);
        void            DeleteConnection(Connection* conn, CarrierDisconnectReason reason);

        void GenerateSendMessages(const char* data, unsigned int dataSize, ConnectionID target, DataReliability reliability, DataPriority priority, unsigned char channel, AZStd::unique_ptr<CarrierACKCallback> ackCallback = AZStd::unique_ptr<CarrierACKCallback>());

        /// Send sync time if possible
        void SendSyncTime();
        void OnReceivedTime(ConnectionID fromId, AZ::u32 time);

        typedef vector<Connection*> ConnectionList;
        ConnectionList       m_connections;
        unordered_set<PendingHandshake, PendingHandshake::Hasher, PendingHandshake::EqualTo> m_pendingHandshakes;

        Handshake*           m_handshake;
        bool                 m_ownHandshake;

        unsigned int         m_port;

        unsigned int         m_maxMsgDataSizeBytes;     ///< Maximum message payload data size in bytes.

        // clock
        static const unsigned int InvalidSyncInterval = 0xffffffff;
        unsigned int         m_clockSyncInterval;       ///< Interval in milliseconds on which we should send clock sync message.
        AZ::u32              m_lastReceivedTime;        ///< Last time value received from the sync or 0 if we never synced.
        AZ::u32              m_lastUsedTime;            ///< Last returned time (to prevent from us getting back in time.
        TimeStamp            m_lastReceivedTimeStamp;   ///< System clock time when we received the last host value or the creation of the carrier.
        TimeStamp            m_lastSyncTimeStamp;       ///< System clock time when we last send clock sync event.

        TimeStamp            m_currentTime;             ///< Current time updated every tick.

        CarrierThread*       m_thread;

#ifdef GM_CARRIER_MESSAGE_CRC
        vector<AZ::u8>      m_dbgCrcMessageBuffer;     ///< Temporary memory buffer used for Crc Message checks
#endif // GM_CARRIER_MESSAGE_CRC
    };
    //////////////////////////////////////////////////////////////////////////
}

using namespace GridMate;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Connection
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
Connection::Connection(CarrierThread* threadOwner, const string& address)
    : m_threadOwner(threadOwner)
    , m_threadConn(NULL)
    , m_fullAddress(address)
    , m_state(Carrier::CST_CONNECTING)
{
    for (unsigned char iChannel = 0; iChannel < m_maxNumberOfChannels; ++iChannel)
    {
        m_sendSeqNum[iChannel] = SequenceNumberMax;
        m_sendReliableSeqNum[iChannel] = SequenceNumberMax;
    }

    memset(&m_congestionState, 0, sizeof(m_congestionState));
    memset(&m_statsLastSecond, 0, sizeof(m_statsLastSecond));
    memset(&m_statsLifetime, 0, sizeof(m_statsLifetime));
    memset(&m_statsEffectiveLastSecond, 0, sizeof(m_statsEffectiveLastSecond));
    memset(&m_statsEffectiveLifetime, 0, sizeof(m_statsEffectiveLifetime));
}

Connection::~Connection()
{
    AZ_Assert(m_threadConn.load(AZStd::memory_order_acquire) == NULL, "We must detach the thread connection first!");

    // Make sure render thread doesn't reference is at this point... it's too late
    for (unsigned int i = 0; i < AZ_ARRAY_SIZE(m_toSend); ++i)
    {
        //AZStd::lock_guard<AZStd::mutex> l(toSendLock);
        while (!m_toSend[i].empty())
        {
            MessageData& msg = m_toSend[i].front();
            m_toSend[i].pop_front();
            m_threadOwner->FreeMessage(msg);
        }
    }

    for (unsigned char iChannel = 0; iChannel < m_maxNumberOfChannels; ++iChannel)
    {
        //AZStd::lock_guard<AZStd::mutex> l(toReceive);
        while (!m_toReceive[iChannel].empty())
        {
            MessageData& msg = m_toReceive[iChannel].front();
            m_toReceive[iChannel].pop_front();
            m_threadOwner->FreeMessage(msg);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ThreadConnection
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ThreadConnection::ThreadConnection(CarrierThread* threadOwner)
    : m_threadOwner(threadOwner)
    , m_mainConnection(NULL)
    , m_dataGramSeqNum(1) // IMPORTANT to start with 1 if we not received any datagrams we will confirm a datagram with value of 0.
    , m_lastAckedDatagram(0)
    , m_lastReceivedDatagramTime(AZStd::chrono::system_clock::now())
    , m_createTime(m_lastReceivedDatagramTime)
    , m_isDisconnecting(false)
    , m_isBadConnection(false)
    , m_isDisconnected(false)
    , m_isBadPackets(false)
{
    for (unsigned char iChannel = 0; iChannel < m_maxNumberOfChannels; ++iChannel)
    {
        m_receivedSeqNum[iChannel] = SequenceNumberMax;
        m_receivedReliableSeqNum[iChannel] = SequenceNumberMax;
        m_receivedLastInsert[iChannel] = m_received[iChannel].end();
        m_receivedLastReliableChunk[iChannel] = m_received[iChannel].end();
    }
}

ThreadConnection::~ThreadConnection()
{
    AZ_Assert(m_mainConnection == NULL || m_mainConnection->m_threadConn.load() == NULL, "We should have unbound the thread connection by now!");
    m_target->m_threadConnection = NULL;
    m_target = NULL;

    for (unsigned char iChannel = 0; iChannel < m_maxNumberOfChannels; ++iChannel)
    {
        while (!m_received[iChannel].empty())
        {
            MessageData& msg = m_received[iChannel].front();
            m_received[iChannel].pop_front();
            m_threadOwner->FreeMessage(msg);
        }
    }

    while (!m_sendDataGrams.empty())
    {
        DatagramData& dgram = m_sendDataGrams.front();
        m_sendDataGrams.pop_front();
        m_threadOwner->FreeDatagram(dgram);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CarrierThread
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// CarrierThread
// [2/10/2011]
//=========================================================================
CarrierThread::CarrierThread(const CarrierDesc& desc, Compressor* compressor)
    : m_compressor(compressor)
    , m_reportedDisconnect(false)
    , m_datagramTempWriteBuffer(kCarrierEndian)
{
    //////////////////////////////////////////////////////////////////////////
    // Driver setup
    m_ownDriver = false;
    m_driver = desc.m_driver;
    m_compressionFactory = desc.m_compressionFactory;

    if (m_driver == 0)
    {
        m_ownDriver = true;
        m_driver = aznew SocketDriver(desc.m_driverIsFullPackets, desc.m_driverIsCrossPlatform);
    }

    Driver::ResultCode initResult = m_driver->Initialize(desc.m_familyType, desc.m_address, desc.m_port, false, desc.m_driverReceiveBufferSize, desc.m_driverSendBufferSize);

    //////////////////////////////////////////////////////////////////////////
    // Traffic control
    m_ownTrafficControl = false;
    m_trafficControl = desc.m_trafficControl;

    if (m_trafficControl == 0)
    {
        m_ownTrafficControl = true;
        m_trafficControl = aznew DefaultTrafficControl(m_driver->GetMaxSendSize(),
            desc.m_disconnectDetectionRttThreshold,
            desc.m_disconnectDetectionPacketLossThreshold,
            desc.m_recvPacketsLimit);
    }

    m_handshakeTimeoutMS = desc.m_connectionTimeoutMS; // default value update from the handshake class
    m_lastMainThreadUpdate = AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now().time_since_epoch()).count();

    //////////////////////////////////////////////////////////////////////////
    m_simulator = desc.m_simulator;
    if (m_simulator)
    {
        m_simulator->BindDriver(m_driver);
    }

    m_maxDataGramSizeBytes = m_driver->GetMaxSendSize();
    m_maxMsgDataSizeBytes = m_maxDataGramSizeBytes - GetDataGramHeaderSize() - GetMaxMessageHeaderSize();
    if (m_compressor)
    {
        m_maxMsgDataSizeBytes = (unsigned int)m_compressor->GetMaxChunkSize(m_maxMsgDataSizeBytes);
    }

    m_connectionTimeoutMS = desc.m_connectionTimeoutMS;
    m_enableDisconnectDetection = desc.m_enableDisconnectDetection;
    m_connectionEvaluationThreshold = AZStd::max AZ_PREVENT_MACRO_SUBSTITUTION (0.0f, AZStd::min AZ_PREVENT_MACRO_SUBSTITUTION (desc.m_connectionEvaluationThreshold, 1.0f));
    m_maxConnections = desc.m_maxConnections;

    //////////////////////////////////////////////////////////////////////////
    // Initializing compressor
    if (m_compressor)
    {
        m_compressionMem.reserve(128 * 1024);
        bool isInit = m_compressor->Init();
        AZ_UNUSED(isInit);
        AZ_Error("GridMate", isInit, "GridMate carrier failed to initialize compression\n");
    }
    //////////////////////////////////////////////////////////////////////////

    // set up carrier thread
    if (initResult == Driver::EC_OK)
    {
        AZ_Assert(desc.m_threadUpdateTimeMS >= 0 && desc.m_threadUpdateTimeMS <= 100, "Thread update time should be within [0,100] range, currently %d!", desc.m_threadUpdateTimeMS);
        m_quitThread = false;
        m_threadSleepTime = AZStd::chrono::milliseconds(desc.m_threadUpdateTimeMS);
        m_threadInstantResponse = desc.m_threadInstantResponse;
        AZStd::thread_desc threadDesc;
        threadDesc.m_name = "GridMate-Carrier";
        if (desc.m_threadCpuID != -1)
        {
            threadDesc.m_cpuId = desc.m_threadCpuID;
        }
        if (desc.m_threadPriority != -100000)
        {
            threadDesc.m_priority = desc.m_threadPriority;
        }
        m_thread = AZStd::thread(AZStd::bind(&CarrierThread::ThreadPump, this), &threadDesc);
    }
    else
    {
        AZ_Warning("GridMate", false, "We could not initialize the driver at port %d (possibly already in use)!", desc.m_port);
        // if case of driver error we will just time out on because we will fail to connect
        m_quitThread = true;
        ThreadMessage* tm = aznew ThreadMessage(MTM_ON_ERROR);
        tm->m_errorCode = CarrierErrorCode::EC_DRIVER;
        tm->m_error.m_driverError.m_errorCode = (Driver::ErrorCodes)initResult;
        PushMainThreadMessage(tm);
    }
}

//=========================================================================
// ~CarrierThread
// [2/10/2011]
//=========================================================================
CarrierThread::~CarrierThread()
{
    Quit();

    {
        AZStd::lock_guard<AZStd::mutex> l(m_carrierMsgQueueLock);
        while (!m_carrierMsgQueue.empty())
        {
            delete m_carrierMsgQueue.front();
            m_carrierMsgQueue.pop();
        }
    }
    {
        AZStd::lock_guard<AZStd::mutex> l(m_mainMsgQueueLock);
        while (!m_mainMsgQueue.empty())
        {
            delete m_mainMsgQueue.front();
            m_mainMsgQueue.pop();
        }
    }

    {
        AZStd::lock_guard<AZStd::mutex> l(m_freeMessagesLock);
        while (!m_freeMessages.empty())
        {
            MessageData* msg = &m_freeMessages.front();
            m_freeMessages.pop_front();
            delete msg;
        }
    }

    {
        //AZStd::lock_guard<AZStd::mutex> l(m_freeDataGramsLock);
        while (!m_freeDataGrams.empty())
        {
            DatagramData* dgram = &m_freeDataGrams.front();
            m_freeDataGrams.pop_front();
            delete dgram;
        }
    }

    {
        AZStd::lock_guard<AZStd::mutex> l(m_freeDataBlocksLock);
        while (!m_freeDataBlocks.empty())
        {
            void* data = m_freeDataBlocks.front();
            m_freeDataBlocks.pop();
            azfree(data, GridMateAllocatorMP);
        }
    }

    if (m_compressor)
    {
        m_compressionFactory->DestroyCompressor(m_compressor);
    }

    if (m_ownTrafficControl)
    {
        delete m_trafficControl;
    }

    if (m_ownDriver)
    {
        delete m_driver;
    }
}

//=========================================================================
// Quit
// [8/2/2012]
//=========================================================================
void
CarrierThread::Quit()
{
    if (!m_quitThread)
    {
        m_quitThread = true;
        // wait for network thread
        m_thread.join();
    }
}

//=========================================================================
// MakeNewConnection
// [2/10/2011]
//=========================================================================
ThreadConnection*
CarrierThread::MakeNewConnection(const AZStd::intrusive_ptr<DriverAddress>& target)
{
    ThreadConnection* conn = aznew ThreadConnection(this);
    conn->m_target = target;
    target->m_threadConnection = conn;
    conn->m_createTime = m_currentTime;
    m_trafficControl->OnConnect(conn, conn->m_target);
    if (m_simulator)
    {
        m_simulator->OnConnect(conn->m_target);
    }

    //AZ_TracePrintf("GM","[%08x] New connection 0x%08x %s\n",this, conn, target->ToAddress().c_str());

    // add it to connection list
    m_threadConnections.push_back(conn);

    return conn;
}

void CarrierThread::NotifyRateUpdate(ThreadConnection* conn)
{
    TrafficControl::CongestionState cState;
    TrafficControl::Statistics lifetime;
    AZ::u32 bytesPerSecond;

    //Calculate Bytes per second
    m_trafficControl->QueryCongestionState(conn, &cState);
    m_trafficControl->QueryStatistics(conn, nullptr, &lifetime);
    float rtt = lifetime.m_rtt > 1.f ? lifetime.m_rtt : 100.f; //For unknown RTT use conservative 100ms to avoid buffer bloat
                                                                //Note: using lifetime RTT as stand-in for smoothed RTT
    float ratef = (1010 * (cState.m_congestionWindow)) / rtt;   //Add 10% to allow rate increases until buffer fills up
    AZ_Assert(ratef <= 0x7FFFFFFFf, " ratef %f > 0x7FFFFFFFf", ratef);
    bytesPerSecond = static_cast<AZ::u32>(ratef);
    //AZ_TracePrintf("GridMate", "Min rate updated -> %u : cwnd=%u rtt=%f ratef=%f queueB=%u\n", 
    //    bytesPerSecond, cState.m_congestionWindow, lifetime.m_rtt, ratef, conn->m_mainConnection->m_bytesInQueue);

    //Avoid buffer bloat
    AZ::u32 k_maxBufferBytes = bytesPerSecond / 5;      //Attempt to cap buffer at 200ms second worth of data volume; realistically it is capped at 1 Second for large bursts
    const AZ::u32 bytesInQueue = conn->m_mainConnection->m_bytesInQueue;

    if (bytesInQueue / k_maxBufferBytes > 0.5f)         //Buffer above 50% load
    {
        if (bytesInQueue >= k_maxBufferBytes)
        {
            bytesPerSecond = m_maxDataGramSizeBytes;    //Slow to 1 packet/sec
        }
        else
        {
            bytesPerSecond = static_cast<AZ::u32>(bytesPerSecond * (1.f - bytesInQueue / k_maxBufferBytes));    //If >50% full, slow down proportional to buffer load
        }
        conn->m_mainConnection->m_rateLimitedByQueueSize = true;
    }
    else
    {
        conn->m_mainConnection->m_rateLimitedByQueueSize = false;
    }

    ThreadMessage* mtm = aznew ThreadMessage(MTM_RATE_UPDATE);
    mtm->m_connection = conn->m_mainConnection;
    mtm->m_newRateBytesPerSec = bytesPerSecond;
    PushMainThreadMessage(mtm);
    //AZ_TracePrintf("GridMate", "%p Min rate updated -> %u \n", this, bytesPerSecond);
}

//=========================================================================
// UpdateSend
// [9/14/2010]
//=========================================================================
void
CarrierThread::UpdateSend()
{
    bool notifyRateChanged = false;

    //////////////////////////////////////////////////////////////////////////
    // Send as much data as we have or traffic control allows
    {
        for (ThreadConnectionList::iterator iConn = m_threadConnections.begin(); iConn != m_threadConnections.end(); ++iConn)
        {
            ThreadConnection* conn = *iConn;
            if (conn->m_mainConnection == NULL)
            {
                continue;
            }

            //////////////////////////////////////////////////////////////////////////
            // if we are in thread instance response mode, make sure we are NOT flooding connection
            // with ACK datagrams.
            if (m_threadInstantResponse)
            {
                // if we have no data to send and traffic control doesn't need to send
                // an ACK and last WaitForData was NOT interrupted. We can skip this ACK packet.
                if (!HasDataToSend(conn) && !m_trafficControl->IsSendACKOnly(conn) && m_driver->WasStopeedWaitingForData())
                {
                    continue; // skip this ACK packet send
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // Check for messages to resend
            Connection* mainConn = conn->m_mainConnection;

            while (!conn->m_sendDataGrams.empty())
            {
                DatagramData& dgram = conn->m_sendDataGrams.front();
                if (m_trafficControl->IsResend(conn, dgram.m_flowControl, dgram.m_resendDataSize))
                {
                    notifyRateChanged = true;       //Notify data generators of new rate
                                                    //Note: IsResend() prevents this from executing for immediately subsequent timeouts

                    //AZ_TracePrintf("GridMate","%p packet %d to %s is lost!\n",this,dgram.m_flowControl.m_sequenceNumber,conn->target->ToString().c_str());
                    conn->m_sendDataGrams.pop_front();

                    if (dgram.m_resendDataSize > 0)
                    {
                        AZStd::lock_guard<AZStd::mutex> l(mainConn->m_toSendLock);

                        // add all messages that need to be resend at the top of the send list.
                        for (int iPriority = 0; iPriority < Carrier::PRIORITY_MAX; ++iPriority)
                        {
                            MessageDataListType::iterator insertPos =  mainConn->m_toSend[iPriority].begin();
                            while (!dgram.m_toResend[iPriority].empty())
                            {
                                MessageData& msg = dgram.m_toResend[iPriority].front();
                                dgram.m_toResend[iPriority].pop_front();
                                mainConn->m_toSend[iPriority].insert(insertPos, msg);
                                mainConn->m_bytesInQueue += msg.m_dataSize;
                            }
                        }

                        m_trafficControl->OnReSend(conn, dgram.m_flowControl, dgram.m_resendDataSize);
                    }

                    FreeDatagram(dgram);
                }
                else
                {
                    break; // should we break after the first no or try all waiting datagrams... they should be in order
                }
            }

            const char* data = nullptr;
            unsigned int dataSize = 0;
            WriteBuffer& writeBuffer = m_datagramTempWriteBuffer;

            //////////////////////////////////////////////////////////////////////////
            // We send one data gram for each connection, every frame to maintain correct RTT, ACK, detect connection lost.
            unsigned int maxDatagramSize = AZStd::GetMin(m_driver->GetMaxSendSize(), m_trafficControl->GetMaxPacketSize(conn)); ///< This doesn't change at the moment
            do
            {
                if (!m_trafficControl->IsSend(conn))
                {
                    break;
                }

                writeBuffer.Clear();
                OutgoingDataGramContext dgramCtx;
                DatagramData& dgram = AllocateDatagram();
                // Generating acks message before sending data
                InitOutgoingDatagram(conn, writeBuffer);

                if (m_compressor && conn->m_mainConnection->m_state != Carrier::CST_CONNECTING) // do not start compression until connection established
                {
                    m_compressionMem.clear(); // clearing old data

                    size_t compPacketSize = 0;
                    CompressorError compErr = CompressorError::Ok;

                    while (true)
                    {
                        size_t chunkSize = m_compressor->GetMaxChunkSize(maxDatagramSize - compPacketSize);
                        GenerateOutgoingDataGram(conn, dgram, writeBuffer, dgramCtx, chunkSize);
                        if (writeBuffer.Size() <= GetDataGramHeaderSize()) // no payload in the datagram
                        {
                            break;
                        }
                        ++conn->m_dataGramSeqNum;
                        size_t maxSizeNeeded = m_compressor->GetMaxCompressedBufferSize(writeBuffer.Size());
                        m_compressionMem.resize(maxSizeNeeded + compPacketSize);
                        size_t compSize = 0;
                        compErr = m_compressor->Compress(writeBuffer.Get(), writeBuffer.Size(), m_compressionMem.data() + compPacketSize, compSize);
                        compPacketSize += compSize;
                        writeBuffer.Clear();
                    }

                    AZ_Assert(compErr == CompressorError::Ok, "Failed to compress chunk with error=%d.", static_cast<int>(compErr));

                    data = (char*)(m_compressionMem.data());
                    dataSize = static_cast<unsigned int>(compPacketSize);
                }
                else
                {
                    GenerateOutgoingDataGram(conn, dgram, writeBuffer, dgramCtx, maxDatagramSize);
                    ++conn->m_dataGramSeqNum;
                    data = writeBuffer.Get();
                    dataSize = static_cast<unsigned int>(writeBuffer.Size());
                }

                AZ_Assert(dataSize <= maxDatagramSize, "We wrote more bytes to the datagram than allowed. Internal error!");

                if (conn->m_isDisconnected || conn->m_isDisconnecting)
                {
                    // If we are disconnecting make sure we send only DataGrams with system messages
                    // GenerateOutgoingDataGram will pack only system messages, so no empty (heartbeats) datagrams.
                    if (dgram.m_flowControl.m_effectiveSize > 0)
                    {
                        FreeDatagram(dgram);
                        break;
                    }
                }

                dgram.m_flowControl.m_size = static_cast<unsigned short>(dataSize);
                m_trafficControl->OnSend(conn, dgram.m_flowControl);

                conn->m_sendDataGrams.push_back(dgram);

                if (m_simulator)
                {
                    if (m_simulator->OnSend(conn->m_target, data, dataSize))
                    {
                        // the simulator will handle the data
                        continue;
                    }
                }

                if (auto sendResult = m_driver->Send(conn->m_target, data, dataSize) != Driver::EC_OK)
                {
                    AZ_TracePrintf("Carrier", "Send error!");
                    ThreadMessage* mtm = aznew ThreadMessage(MTM_ON_ERROR);
                    mtm->m_connection = conn->m_mainConnection;
                    mtm->m_errorCode = CarrierErrorCode::EC_DRIVER;
                    mtm->m_error.m_driverError.m_errorCode = (Driver::ErrorCodes)sendResult;
                    mtm->m_disconnectReason = CarrierDisconnectReason::DISCONNECT_DRIVER_ERROR;
                    PushMainThreadMessage(mtm);
                    break;
                }

            } while (HasDataToSend(conn));

            if (notifyRateChanged)
            {
                //AZ_TracePrintf("GridMate", "%p SendUpdateAndMakeHeap -> %u \n", this, minRateBytesPerSecond);
                NotifyRateUpdate(conn);
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////
}

//=========================================================================
// UpdateReceive
// [2/10/2011]
//=========================================================================
void
CarrierThread::UpdateReceive()
{
    // process packets - pack, split, compress, script, etc. execute from a thread too
    char* data = reinterpret_cast<char*>(AllocateMessageData(m_maxDataGramSizeBytes));
    AZStd::intrusive_ptr<DriverAddress> fromAddress;
    Driver::ResultCode resultCode;
    unsigned int recvDataGramSize;
    // read all datagrams
    while (true)
    {
        // read data from the driver layer or simulator
        recvDataGramSize = m_driver->Receive(data, m_maxDataGramSizeBytes, fromAddress, &resultCode);
        if (m_simulator)
        {
            if (recvDataGramSize > 0)
            {
                // we received some data allow the simulator to add to something with it
                if (m_simulator->OnReceive(fromAddress, data, recvDataGramSize))
                {
                    // the simulator has the data
                    continue;
                }
            }
            else
            {
                // we have no more data see if the simulator has something for us
                recvDataGramSize = m_simulator->ReceiveDataFrom(fromAddress, data, m_maxDataGramSizeBytes);
                resultCode = Driver::EC_OK; // simulator should not fail.
            }
        }

        if (resultCode != Driver::EC_OK)
        {
            AZ_TracePrintf("Carrier", "Receive error!");
            ThreadMessage* tm = aznew ThreadMessage(MTM_ON_ERROR);
            tm->m_errorCode = CarrierErrorCode::EC_DRIVER;
            tm->m_error.m_driverError.m_errorCode = Driver::ErrorCodes(resultCode);
            PushMainThreadMessage(tm);
            break;
        }
        if (recvDataGramSize == 0)
        {
            break;
        }

        ReadBuffer readBuffer(kCarrierEndian, data, recvDataGramSize);

        ThreadConnection* conn = 0;
        if (fromAddress->m_threadConnection != NULL)
        {
            conn = fromAddress->m_threadConnection;
            if (!m_trafficControl->IsCanReceiveData(conn))
            {
                //Unexpected packet or malicious activity. Report It!
                ThreadMessage* mtm = aznew ThreadMessage(MTM_ON_ERROR);
                mtm->m_errorCode = CarrierErrorCode::EC_SECURITY;
                mtm->m_error.m_securityError.m_errorCode = SecurityError::EC_DATA_RATE_TOO_HIGH;
                PushMainThreadMessage(mtm);
                break;
            }

            if (m_compressor) // should probably have some compression handshake than relying on existence of compressor
            {
                CompressorError compErr;
                do
                {
                    m_compressionMem.clear();
                    size_t uncompSize = 0;
                    size_t chunkSize = 0;
                    m_compressionMem.resize(m_maxDataGramSizeBytes);
                    compErr = m_compressor->Decompress(readBuffer.GetCurrent(), readBuffer.Left().GetBytes(), m_compressionMem.data(), chunkSize, uncompSize);
                    if (compErr != CompressorError::Ok)
                    {
                        break;
                    }

                    ReadBuffer chunkRb(kCarrierEndian, reinterpret_cast<char*>(m_compressionMem.data()), uncompSize);
                    OnReceivedIncomingDataGram(conn, chunkRb, recvDataGramSize);

                    readBuffer.Skip(chunkSize);
                } while (!readBuffer.IsEmpty() && !readBuffer.IsOverrun());

                if (compErr != CompressorError::Ok && !IsConnectRequestDataGram(data, recvDataGramSize)) // Some uncompressed connection requests might be still in fly after we already created connection object for this peer
                {
                    conn->m_isBadPackets = true;
                }

                continue;
            }
        } 
        else
        {
            if (m_threadConnections.size() >= m_maxConnections)
            {
                continue;
            }
            //////////////////////////////////////////////////////////////////////////
            // Check if this packet comes from a GridMate trying to connect. If not discard it.
            if (!IsConnectRequestDataGram(data, recvDataGramSize))
            {
                continue; /// Discard invalid datagrams
            }
            //////////////////////////////////////////////////////////////////////////

            conn = MakeNewConnection(fromAddress);

            ThreadMessage* tm = aznew ThreadMessage(MTM_NEW_CONNECTION);
            tm->m_newConnectionAddress = fromAddress->ToAddress();
            tm->m_threadConnection = conn;
            PushMainThreadMessage(tm);
        }

        if (!conn->m_isBadConnection && !conn->m_isBadPackets)  // don't bother receiving from bad connections
        {
            OnReceivedIncomingDataGram(conn, readBuffer, recvDataGramSize);
        }
    }
    FreeMessageData(data, m_maxDataGramSizeBytes);
}

//=========================================================================
// ProcessConnections
// [2/10/2011]
//=========================================================================
void
CarrierThread::ProcessConnections()
{
    float disconnectConditionFactor = 1.0f; // used to determine % of the disconnected condition to apply. 1.0f = 100%
    for (ThreadConnectionList::iterator i = m_threadConnections.begin(); i != m_threadConnections.end(); )
    {
        ThreadConnection* connection = *i;
        if (connection->m_mainConnection == nullptr)
        {
            ++i;
            continue; // skip inactive connections
        }

        for (unsigned char channel = 0; channel < m_maxNumberOfChannels; ++channel) // we will need a different approach if we have more channels.
        {
            while (!connection->m_received[channel].empty())
            {
                MessageData& msg = connection->m_received[channel].front();
                if (msg.m_reliability == Carrier::SEND_UNRELIABLE)
                {
                    // For unreliable we use the reliable sequence number
                    // to make sure they are NOT delivered before last reliable message.
                    // But we increment it (reliable sequence number) only for reliable msg.
                    if (SequenceNumberSequentialDistance(connection->m_receivedReliableSeqNum[channel], msg.m_sendReliableSeqNum) > 0)
                    {
                        // we have not received the last reliable message send before this unreliable, so wait!
                        //AZ_TracePrintf("GridMate","[%08x] Waiting on reliable message %d received %d on channel %d!\n",this, msg.reliableSeqNum,connection->receivedReliableSeqNum[channel],channel);
                        break;
                    }

                    connection->PopReceived(channel);
                    connection->m_receivedSeqNum[channel] = msg.m_sequenceNumber; // Update the last processed packet
                    {
                        AZStd::lock_guard<AZStd::mutex> l(connection->m_mainConnection->m_toReceiveLock);
                        connection->m_mainConnection->m_toReceive[channel].push_back(msg);
                    }
                }
                else //if(msg.reliability==Carrier::SEND_RELIABLE)
                {
                    // Check message order and process the message for the user to receive.
                    if (SequenceNumberSequentialDistance(connection->m_receivedReliableSeqNum[channel], msg.m_sendReliableSeqNum) != 1) // if we are next in line
                    {
                        break; // this message is not sequential, no gaps in reliable messages allowed.. so wait
                    }

                    if (msg.m_numChunks == 1)  // If this message is only one chunk process it
                    {
                        connection->PopReceived(channel);
                        connection->m_receivedReliableSeqNum[channel] = msg.m_sendReliableSeqNum; // Update the last processed reliable packet
                        connection->m_receivedSeqNum[channel] = msg.m_sequenceNumber; // Update the last processed packet

                        {
                            AZStd::lock_guard<AZStd::mutex> l(connection->m_mainConnection->m_toReceiveLock);
                            connection->m_mainConnection->m_toReceive[channel].push_back(msg);
                        }
                        //AZ_TracePrintf("GridMate","[%08x] Received reliable message %d on channel %d!\n",this, msg.reliableSeqNum,channel);
                    }
                    else
                    {
                        // We have multiple chunks, make sure we haev all of them before we process them....
                        if (connection->m_received[channel].size() < msg.m_numChunks) // early out check for number of chunks
                        {
                            break; // we obviously don't have all the chunks... wait
                        }

                        // check if we have all the chunks
                        MessageDataListType::iterator chunkIter;
                        SequenceNumber numChunks = msg.m_numChunks;
                        // check if we have cached traversal of the reliable chunks, if so use it. Otherwise start from the beginning.
                        if (connection->m_receivedLastReliableChunk[channel] == connection->m_received[channel].end())
                        {
                            chunkIter = connection->m_received[channel].begin();
                        }
                        else
                        {
                            chunkIter = connection->m_receivedLastReliableChunk[channel]; // use cached iterator
                            numChunks -= SequenceNumberSequentialDistance(msg.m_sendReliableSeqNum, chunkIter->m_sendReliableSeqNum); // skip traversed chunks
                        }
                        // go to next element
                        MessageDataListType::iterator prevChunkIter = chunkIter;
                        --numChunks;
                        chunkIter++;
                        while (chunkIter != connection->m_received[channel].end() && numChunks > 0)
                        {
                            // TODO: Technically the two "if" below can be replaced with a signle sequence check,
                            // as all chunks will have sequential sequenceNumber and there will no need to check for
                            // unrealiblae packets in the middle. While "faster" the code will harder to flollow, so until we have
                            // all the code well codument keep it this way
                            // if( SequenceNumberSequentialDistance(prevChunkIter->m_sequenceNumber,chunkIter->m_sequenceNumber) != 1 )
                            //    break; // wait for consecutive messages

                            if (chunkIter->m_reliability == Carrier::SEND_UNRELIABLE)
                            {
                                // Chunks are switched to reliable mode, so they should always be realible... wait
                                break;
                            }

                            if (SequenceNumberSequentialDistance(prevChunkIter->m_sendReliableSeqNum, chunkIter->m_sendReliableSeqNum) != 1)
                            {
                                // there is a gap message - 0 distance ... wait
                                break;
                            }

                            prevChunkIter = chunkIter++;
                            --numChunks;
                        }
                        if (numChunks == 0)
                        {
                            connection->m_receivedReliableSeqNum[channel] = prevChunkIter->m_sendReliableSeqNum;
                            connection->m_receivedSeqNum[channel] = prevChunkIter->m_sequenceNumber; // Update the last processed packet
                            connection->m_receivedLastReliableChunk[channel] = connection->m_received[channel].end(); // reset cached iterator

                            // since we pop multiple messages without a call to PopReceived, invalidate cached insert iterators manually
                            // (we need to do this only if the iterator was in the removed chunk).
                            if (connection->m_receivedLastInsert[channel] != connection->m_received[channel].end() && // if we have a valid cached insert position
                                SequenceNumberGreaterEqualThan(prevChunkIter->m_sequenceNumber, connection->m_receivedLastInsert[channel]->m_sequenceNumber)) // if cached iterator is in the messages we are removing
                            {
                                connection->m_receivedLastInsert[channel] = connection->m_received[channel].end(); // invalidate the iterator
                            }

                            {
                                AZStd::lock_guard<AZStd::mutex> l(connection->m_mainConnection->m_toReceiveLock);
                                connection->m_mainConnection->m_toReceive[channel].splice(connection->m_mainConnection->m_toReceive[channel].end(),
                                    connection->m_received[channel], connection->m_received[channel].begin(), chunkIter);
                            }
                        }
                        else
                        {
                            connection->m_receivedLastReliableChunk[channel] = prevChunkIter; // store position to cache, so we start from there.
                            break; // we don't have all the chunks.. wait
                        }
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Check for disconnect conditions
        CarrierDisconnectReason disconnectReason = CarrierDisconnectReason::DISCONNECT_BAD_CONNECTION;

        bool isHandshakeTimeOut = false;
        bool isConnectionTimeout = false;
        bool isBadTrafficConditions = false;
        bool isBadPackets = false;
        if (connection->m_isBadPackets)
        {
            isBadPackets = true;
            disconnectReason = CarrierDisconnectReason::DISCONNECT_BAD_PACKETS;
        }
        if (!connection->m_isDisconnecting && connection->m_mainConnection->m_state != Carrier::CST_CONNECTING /* NAT punch allowance */)
        {
            // after the connection was established (handshake complete)... check the status.
            if (m_enableDisconnectDetection && !connection->m_isBadConnection)
            {
                unsigned int connectionTimeout = static_cast<unsigned int>(static_cast<float>(m_connectionTimeoutMS) * disconnectConditionFactor);
                if (AZStd::chrono::milliseconds(m_currentTime - connection->m_lastReceivedDatagramTime).count() > connectionTimeout)
                {
                    isConnectionTimeout = true;
                    AZ_TracePrintf("GridMate", "[%08x] We have NOT received packet from %s for %d ms. Connection is lost!\n", this, connection->m_target->ToString().c_str(), connectionTimeout);
                }

                if (m_trafficControl->IsDisconnect(connection, disconnectConditionFactor))
                {
                    isBadTrafficConditions = true;
                    AZ_TracePrintf("GridMate", "[%08x] Disconnecting %s to due to bad traffic conditions!\n", this, connection->m_target->ToAddress().c_str());
                }
            }
        }
        else
        {
            AZStd::chrono::milliseconds timeNotReady = m_currentTime - connection->m_createTime;
            if (connection->m_isDisconnecting)
            {
                if (!connection->m_isDisconnected)
                {
                    // time not ready at disconnect
                    // wait for waitAfterDisconnectMSG after the disconnect message to tear down connections.
                    static const unsigned int waitAfterDisconnectMSG = 500;
                    if (timeNotReady.count() > waitAfterDisconnectMSG)
                    {
                        connection->m_isDisconnected = true;
                        ThreadMessage* mtm = aznew ThreadMessage(MTM_DISCONNECT_TIMEOUT); // send message to me, so next frame we will delete the connection
                        mtm->m_connection = connection->m_mainConnection;
                        mtm->m_disconnectReason = connection->m_disconnectReason;
                        PushMainThreadMessage(mtm);
                    }
                }
            }
            else if (m_enableDisconnectDetection && !connection->m_isBadConnection)
            {
                // time not ready on connect
                if (timeNotReady.count() > m_handshakeTimeoutMS)
                {
                    disconnectReason = CarrierDisconnectReason::DISCONNECT_HANDSHAKE_TIMEOUT;
                    AZ_TracePrintf("GridMate", "[%08x] Handshake to %s did not complete within %d ms!\n", this, connection->m_target->ToString().c_str(), m_handshakeTimeoutMS);
                    isHandshakeTimeOut = true;
                }
            }
        }

        if (isHandshakeTimeOut || isConnectionTimeout || isBadTrafficConditions || isBadPackets)
        {
            connection->m_isBadConnection = true;
            AZ_Assert(connection->m_mainConnection->m_fullAddress == connection->m_target->ToAddress(), "Connection name mismatch on disconnect connection (%s, %s)!", connection->m_mainConnection->m_fullAddress.c_str(), connection->m_target->ToAddress().c_str());
            ThreadMessage* mtm = aznew ThreadMessage(MTM_DISCONNECT); // ask main thread to close the connection properly
            mtm->m_connection = connection->m_mainConnection;
            mtm->m_disconnectReason = disconnectReason;
            PushMainThreadMessage(mtm);
            // Evaluate all other connection and if all are in bad state (defined by a threshold) and disconnect if needed.
            if (m_connectionEvaluationThreshold < 1.0f)
            {
                if (disconnectConditionFactor == 1.0f)
                {
                    i = m_threadConnections.begin();
                    disconnectConditionFactor = m_connectionEvaluationThreshold;
                    continue;
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////

        ++i;
    }
}

//=========================================================================
// UpdateStats
// [5/25/2011]
//=========================================================================
void
CarrierThread::UpdateStats()
{
    for (ThreadConnectionList::iterator iConn = m_threadConnections.begin(); iConn != m_threadConnections.end(); ++iConn)
    {
        ThreadConnection* conn = *iConn;
        if (conn->m_mainConnection == NULL)
        {
            continue;
        }
        Connection* mainConn = conn->m_mainConnection;
        {
            AZStd::lock_guard<AZStd::mutex> l(mainConn->m_statsLock);
            m_trafficControl->QueryStatistics(conn, &mainConn->m_statsLastSecond, &mainConn->m_statsLifetime, &mainConn->m_statsEffectiveLastSecond, &mainConn->m_statsEffectiveLifetime);
            m_trafficControl->QueryCongestionState(conn, &mainConn->m_congestionState);
        }
    }
}

//=========================================================================
// ThreadPump
// [2/7/2011]
//=========================================================================
void
CarrierThread::ThreadPump()
{
    while (m_quitThread == false)
    {
        AZ::u64 lastMainThreadUpdateMS = m_lastMainThreadUpdate;    // since m_lastMainThreadUpdate is updated from another thread make sure we read the value.
        m_currentTime = AZStd::chrono::system_clock::now();
        AZ::u64 currentTimeStampMS = AZStd::chrono::milliseconds(m_currentTime.time_since_epoch()).count();

        //////////////////////////////////////////////////////////////////////////
        // Check if main thread updates often enough
        AZ_Assert(currentTimeStampMS >= lastMainThreadUpdateMS, "Time values are not consistent across threads! We need a better way to measure time");
        if (m_enableDisconnectDetection && currentTimeStampMS > lastMainThreadUpdateMS && (currentTimeStampMS - lastMainThreadUpdateMS) > m_connectionTimeoutMS)
        {
            // Main thread has not update for a long time.
            if (!m_reportedDisconnect)
            {
                m_reportedDisconnect = true;
                AZ_Warning("GridMate", false, "Carrier was NOT updated for >%u ms, you should call Update() regularly!\n", m_connectionTimeoutMS);

                ThreadMessage* mtm = aznew ThreadMessage(MTM_ON_ERROR);
                mtm->m_errorCode = CarrierErrorCode::EC_SECURITY;
                mtm->m_error.m_securityError.m_errorCode = SecurityError::EC_UPDATE_TIMEOUT;
                PushMainThreadMessage(mtm);
            }
        }
        else if (m_reportedDisconnect)
        {
            AZ_Warning("GridMate", false, "Carrier was updated after no update for >%u ms, you should call Update() regularly!\n", m_connectionTimeoutMS);
            m_reportedDisconnect = false;
        }

        //////////////////////////////////////////////////////////////////////////
        // Process messages for us
        {
            ThreadMessage* msg;
            while ((msg = PopCarrierThreadMessage()) != NULL)
            {
                switch (msg->m_code)
                {
                case CTM_CONNECT:
                {
                    AZ_Assert(msg->m_connection != NULL, "You must provide a valid connection pointer!");
                    // if this connect was initiated from a remote machine msg->threadConnection will be != NULL
                    ThreadConnection* conn = msg->m_threadConnection;
                    if (!conn)
                    {
                        // The main thread is initiating this connection
                        AZStd::intrusive_ptr<DriverAddress> driverAddress = m_driver->CreateDriverAddress(msg->m_connection->m_fullAddress);
                        if (driverAddress->m_threadConnection != NULL)
                        {
                            // we already have such thread connection
                            conn = driverAddress->m_threadConnection;
                            // make sure the existing connection is not bound
                            AZ_Assert(conn->m_mainConnection == NULL, "This thread connection should be unbound!");
                        }
                        else
                        {
                            conn = MakeNewConnection(driverAddress);
                        }
                    }
                    AZ_Assert(conn->m_mainConnection == NULL || conn->m_mainConnection == msg->m_connection, "This thread connection should be unbound or bound to the imcomming main connection!");
                    conn->m_mainConnection = msg->m_connection;
                    AZ_Assert(conn->m_mainConnection->m_threadConn.load() == NULL || conn->m_mainConnection->m_threadConn.load() == conn, "This main connection should be unbound or bound to us!");
                    conn->m_mainConnection->m_threadConn = conn;
                } break;
                case CTM_DISCONNECT:
                {
                    AZ_Assert(msg->m_connection != NULL, "You must provide a valid connection pointer!");
                    ThreadConnection* tc = msg->m_connection->m_threadConn;
                    if (tc && !tc->m_isDisconnecting)
                    {
                        AZ_Assert(tc->m_mainConnection == msg->m_connection, "We must have properly bound connections 0x%08x==0x%08x!", tc->m_mainConnection, msg->m_connection);
                        tc->m_isDisconnecting = true;
                        tc->m_createTime = m_currentTime;
                        tc->m_disconnectReason = msg->m_disconnectReason;
                    }
                } break;
                case CTM_DELETE_CONNECTION:
                {
                    ThreadConnection* tc = NULL;
                    tc = msg->m_threadConnection;
                    if (tc)
                    {
                        if (m_simulator)
                        {
                            m_simulator->OnDisconnect(tc->m_target);
                        }
                        m_trafficControl->OnDisconnect(tc);

                        ThreadConnectionList::iterator tcIter = AZStd::find(m_threadConnections.begin(), m_threadConnections.end(), tc);
                        if (tcIter != m_threadConnections.end())
                        {
                            m_threadConnections.erase(tcIter);
                        }

                        delete tc;

                        if (msg->m_connection)
                        {
                            ThreadMessage* mtm = aznew ThreadMessage(MTM_DELETE_CONNECTION);
                            //AZ_TracePrintf("GM","[%08x] MTM_DELETE_CONNECTION send for 0x%08x\n",this,msg->connection);
                            mtm->m_connection = msg->m_connection;
                            mtm->m_threadConnection = NULL;
                            mtm->m_disconnectReason = msg->m_disconnectReason;
                            PushMainThreadMessage(mtm);
                        }
                    }
                } break;
                case CTM_HANDSHAKE_COMPLETE:
                {
                    ThreadConnection* tc = msg->m_connection->m_threadConn.load(AZStd::memory_order_acquire);
                    if (tc)
                    {
                        m_trafficControl->OnHandshakeComplete(tc);
                    }
                } break;
                }
                ;

                delete msg;
            }
        }
        //////////////////////////////////////////////////////////////////////////

        if (m_trafficControl->Update())
        {
            UpdateStats();
        }

        m_driver->Update();

        if (m_simulator)
        {
            m_simulator->Update();
        }

        UpdateReceive();

        UpdateSend();

        ProcessConnections();

        AZStd::chrono::milliseconds updateTime = AZStd::chrono::system_clock::now() - m_currentTime;

        if (updateTime < m_threadSleepTime)
        {
            AZStd::chrono::milliseconds maxToSleepTime = m_threadSleepTime - updateTime;
            if (m_threadInstantResponse)
            {
                m_driver->WaitForData(maxToSleepTime); // for instance response allow the Wait to wake up on received data (or send)
            }
            else
            {
                AZStd::this_thread::sleep_for(maxToSleepTime); // fully sleep for maxToSleepTime
            }
        }
        else
        {
            AZStd::this_thread::yield();
            AZ_TracePrintf("GridMate", "Carrier thread %p updated for %u ms which is >= than thread update time %u ms! Yield will be called!\n", 
                this, static_cast<unsigned int>(updateTime.count()), static_cast<unsigned int>(m_threadSleepTime.count()));
        }
    }

    // clean up connections
    for (ThreadConnectionList::iterator i = m_threadConnections.begin(); i != m_threadConnections.end(); ++i)
    {
        ThreadConnection* tc = *i;
        if (m_simulator)
        {
            m_simulator->OnDisconnect(tc->m_target);
        }
        m_trafficControl->OnDisconnect(tc);
        // we can't wait for main thread to unbind since we are exiting...
        if (tc->m_mainConnection)
        {
            tc->m_mainConnection->m_threadConn = NULL;
            ThreadMessage* mtm = aznew ThreadMessage(MTM_DELETE_CONNECTION);
            mtm->m_connection = tc->m_mainConnection;
            mtm->m_threadConnection = NULL;
            mtm->m_disconnectReason = CarrierDisconnectReason::DISCONNECT_SHUTTING_DOWN;
            PushMainThreadMessage(mtm);
        }
        delete tc;
    }
    m_threadConnections.clear();

    if (m_simulator)
    {
        m_simulator->UnbindDriver();
    }
}

//=========================================================================
// AllocateMessage
// [10/4/2010]
//=========================================================================
inline MessageData&
CarrierThread::AllocateMessage()
{
    if (!m_freeMessages.empty())
    {
        AZStd::lock_guard<AZStd::mutex> l(m_freeMessagesLock);
        if (!m_freeMessages.empty())
        {
            MessageData& front = m_freeMessages.front();
            m_freeMessages.pop_front();
            return front;
        }
    }
    return *aznew MessageData();
}

//=========================================================================
// FreeMessage
// [10/13/2010]
//=========================================================================
inline void
CarrierThread::FreeMessage(MessageData& msg)
{
    AZStd::lock_guard<AZStd::mutex> l(m_freeMessagesLock);
    if (msg.m_data)
    {
        FreeMessageData(msg.m_data, msg.m_dataSize);
        msg.m_data = 0;
    }
    // if we have too many free, delete some
    m_freeMessages.push_back(msg);
    //m_freeMessages.push_front(msg);
    //delete &msg;
}

//=========================================================================
// AllocateDatagram
// [10/5/2010]
//=========================================================================
inline DatagramData&
CarrierThread::AllocateDatagram()
{
    if (!m_freeDataGrams.empty())
    {
        //AZStd::lock_guard<AZStd::mutex> l(m_freeDataGramsLock);
        if (!m_freeDataGrams.empty())
        {
            DatagramData& front = m_freeDataGrams.front();
            m_freeDataGrams.pop_front();
            return front;
        }
    }
    return *aznew DatagramData();
}

//=========================================================================
// FreeDatagram
// [10/13/2010]
//=========================================================================
inline void
CarrierThread::FreeDatagram(DatagramData& dgram)
{
    //AZStd::lock_guard<AZStd::mutex> l(m_freeDataGramsLock);
    if (dgram.m_resendDataSize > 0)
    {
        for (int i = 0; i < Carrier::PRIORITY_MAX; ++i)
        {
            while (!dgram.m_toResend[i].empty())
            {
                MessageData& msg = dgram.m_toResend[i].front();
                dgram.m_toResend[i].pop_front();
                FreeMessage(msg);
            }
        }
    }
    // if we have too many free, delete some
    m_freeDataGrams.push_back(dgram);
    //m_freeDataGrams.push_front(dgram);
    //delete &dgram;
}

//=========================================================================
// AllocateMessageData
// [8/2/2012]
//=========================================================================
inline void*
CarrierThread::AllocateMessageData(unsigned int size)
{
    (void)size;
    AZ_Assert(size <= m_maxDataGramSizeBytes, "The message size is too big to be one block!");
    if (!m_freeDataBlocks.empty())
    {
        AZStd::lock_guard<AZStd::mutex> l(m_freeDataBlocksLock);
        if (!m_freeDataBlocks.empty())
        {
            void* front = m_freeDataBlocks.front();
            m_freeDataBlocks.pop();
            return front;
        }
    }
    return azmalloc(m_maxDataGramSizeBytes, 1, GridMateAllocatorMP);
    //return azmalloc(size,1,GridMateAllocatorMP);
}

//=========================================================================
// FreeMessageData
// [8/2/2012]
//=========================================================================
inline void
CarrierThread::FreeMessageData(void* data, unsigned int size)
{
    (void)size;
    AZStd::lock_guard<AZStd::mutex> l(m_freeDataBlocksLock);
    // if we have too many free them, delete some
    m_freeDataBlocks.push(data);
    //  return azfree(data,GridMateAllocatorMP,size);
}

//=========================================================================
// GetDataGramHeaderSize
// [10/7/2010]
//=========================================================================
inline unsigned int
CarrierThread::GetDataGramHeaderSize()
{
    static const unsigned int sequenceSize = sizeof(SequenceNumber);
    return sequenceSize;
}

//=========================================================================
// WriteAckData
// [8/7/2012]
//=========================================================================
void CarrierThread::WriteAckData(ThreadConnection* connection, WriteBuffer& writeBuffer)
{
    unsigned char ackFlags = 0;
    size_t numDatagramsToAck = connection->m_receivedDatagramsHistory.Size();

    if (numDatagramsToAck)
    {
        // Generate ACK bits
        SequenceNumber lastToAck;   // last received datagram
        SequenceNumber firstToAck;      // first received datagram (still in the list)
        unsigned char* ackHistoryBits = NULL;
        unsigned char ackNumHistoryBytes = 0;
        unsigned char ackHistoryBitsStorage[DataGramHistoryList::m_datagramHistoryMaxNumberOfBytes];

        size_t historyIndex = connection->m_receivedDatagramsHistory.Begin();
        firstToAck = connection->m_receivedDatagramsHistory.At(historyIndex);
        lastToAck = connection->m_receivedDatagramsHistory.GetForAck(connection->m_receivedDatagramsHistory.Last());
        // get the area we need to cover
        SequenceNumber dist = SequenceNumberSequentialDistance(firstToAck, lastToAck);
        if (dist == 0)
        {
            // do nothing just send the last ack
        }
        else if (dist == (numDatagramsToAck - 1))
        {
            // We ack continuous array store only the last number (sizeof(SequenceNumber) overhead)
            ackFlags |= AHF_CONTINUOUS_ACK;
            // mark all dgrams as acked
            do
            {
                if (connection->m_receivedDatagramsHistory.IsValid(historyIndex))
                {
                    connection->m_receivedDatagramsHistory.GetForAck(historyIndex, &historyIndex);
                }
                else
                {
                    connection->m_receivedDatagramsHistory.Increment(historyIndex);
                }
            } while (historyIndex != connection->m_receivedDatagramsHistory.Last() && historyIndex != connection->m_receivedDatagramsHistory.End());
            //AZ_TracePrintf("GridMate","%p Send Ack[%d,%d]\n",this,firstToAck,lastToAck);
        }
        else
        {
            // we have some packet lost, encode a bit for each datagram
            ackFlags |= AHF_BITS;
            ackHistoryBits = ackHistoryBitsStorage;
            ackNumHistoryBytes = AZ_SIZE_ALIGN(dist, 8) / 8; // get number of bytes necessary to store
            AZ_Assert(ackNumHistoryBytes <= AZ_ARRAY_SIZE(ackHistoryBitsStorage), "This should be impossible as we keep limited amount of consequtive packets in the history!");

            // LMBR-23432: Perform a boundary check on ackNumHistoryBytes to prevent the buffer overflow in
            //             memset() below. This is a temporary fix until the cause of the overflow is fixed.
            if (ackNumHistoryBytes > DataGramHistoryList::m_datagramHistoryMaxNumberOfBytes)
            {
                ackNumHistoryBytes = DataGramHistoryList::m_datagramHistoryMaxNumberOfBytes;
            }

            ackFlags |= ackNumHistoryBytes;
            memset(ackHistoryBits, 0, ackNumHistoryBytes); // reset all the bits
            //AZ_TracePrintf("GridMate","%p Send Ack Bits[%d,%d]\n",this,firstToAck,lastToAck);
        }

        if (ackHistoryBits)
        {
            do
            {
                if (connection->m_receivedDatagramsHistory.IsValid(historyIndex))
                {
                    SequenceNumber currentToAck = connection->m_receivedDatagramsHistory.GetForAck(historyIndex, &historyIndex);
                    dist = SequenceNumberSequentialDistance(currentToAck, lastToAck);
                    //AZ_TracePrintf("GridMate","%p confirm: %d in (%d)\n",this, currentToAck, dgram.m_flowControl.m_sequenceNumber);
                    AZ_Assert(dist > 0, "Invalid distance");
                    --dist; // 1 distance is encoded as 0 offset

                    unsigned int byteIndex = dist / 8;
                    unsigned int byteOffset = dist % 8;

                    AZ_Assert(byteIndex < ackNumHistoryBytes, "We should be able to fit all bit in the buffer!");

                    // LMBR-23231: Perform a boundary check on byteIndex to prevent the buffer overflow
                    //             below. This is a temporary fix until the cause of the overflow is fixed.
                    if (byteIndex < ackNumHistoryBytes)
                    {
                        ackHistoryBits[byteIndex] |= (1 << byteOffset); // set the ack bit
                    }
                }
                else
                {
                    connection->m_receivedDatagramsHistory.Increment(historyIndex);
                }
            }
            while (historyIndex != connection->m_receivedDatagramsHistory.Last() && historyIndex != connection->m_receivedDatagramsHistory.End());
        }

        writeBuffer.Write(ackFlags);
        writeBuffer.Write(lastToAck);
        if ((ackFlags & AHF_BITS) != 0)   // if we have history bytes write them
        {
            writeBuffer.WriteRaw(ackHistoryBits, ackNumHistoryBytes);
        }
        else if ((ackFlags & AHF_CONTINUOUS_ACK) != 0)   // if we have continuous packet ack write the first ack
        {
            writeBuffer.Write(firstToAck);
        }

        connection->m_lastAckedDatagram = lastToAck;
    }
    else
    {
        ackFlags |= AHF_KEEP_ALIVE;
        writeBuffer.Write(ackFlags);
    }

    m_trafficControl->OnSendAck(connection);
}

//=========================================================================
// ReadAckData
// [8/7/2012]
//=========================================================================
void
CarrierThread::ReadAckData(ThreadConnection* connection, ReadBuffer& readBuffer)
{
    // Read ACK bits
    unsigned char ackFlags = 0;
    unsigned char ackHistoryBitsStorage[DataGramHistoryList::m_datagramHistoryMaxNumberOfBytes];
    unsigned char ackNumHistoryBytes = 0;
    int ackNumHistoryBits = 0;

    bool isAckData = true;
    SequenceNumber lastToAck = 0, firstToAck = 0;               // last received datagram
    readBuffer.Read(ackFlags);
    if ((ackFlags & AHF_BITS) != 0)   // if we have history bytes write them
    {
        readBuffer.Read(lastToAck);
        firstToAck = lastToAck;
        ackNumHistoryBytes = ackFlags & DataGramHistoryList::m_datagramHistoryMaxNumberOfBytesMask;

        // There should never be more ack bytes sent than the maximum being tracked,
        // unless there is a bug or we are being attacked with malformed packets
        if (ackNumHistoryBytes > DataGramHistoryList::m_datagramHistoryMaxNumberOfBytes)
        {
            AZ_Assert(false, "ackNumHistoryBytes claims that the datagram contains more ACK bytes (%u) than possible!", static_cast<unsigned int>(ackNumHistoryBytes));
            ackNumHistoryBytes = 0;
            return;
        }
        ackNumHistoryBits = ackNumHistoryBytes * 8;

        readBuffer.ReadRaw(ackHistoryBitsStorage, ackNumHistoryBytes);
    }
    else if ((ackFlags & AHF_CONTINUOUS_ACK) != 0)
    {
        readBuffer.Read(lastToAck);
        readBuffer.Read(firstToAck);
        //AZ_TracePrintf("GridMate","%p Ack[%d,%d]\n",this,firstToAck,lastToAck);
    }
    else if ((ackFlags & AHF_KEEP_ALIVE) != 0)  // only a keep alive ack packet
    {
        isAckData = false;
    }
    else
    {
        readBuffer.Read(lastToAck); // only one packet to ack
        firstToAck = lastToAck;
    }

    if (readBuffer.IsOverrun())
    {
        return;
    }

    if (connection != NULL && isAckData)
    {
        if (firstToAck != lastToAck)
        {
            // process range ACK from firstToAck<->lastToAck
            DatagramDataListType::iterator dgPrevIter = connection->m_sendDataGrams.before_begin();
            DatagramDataListType::iterator dgIter = connection->m_sendDataGrams.begin();
            while (dgIter != connection->m_sendDataGrams.end())
            {
                DatagramData& dg = *dgIter;
                bool isAck = false;
                bool isNAck = false;
                SequenceNumber seqNum = dg.m_flowControl.m_sequenceNumber;

                if (seqNum == firstToAck || seqNum == lastToAck || (SequenceNumberGreaterThan(seqNum, firstToAck) && SequenceNumberLessThan(seqNum, lastToAck))) // if in range [firstToAck,lastToAck]
                {
                    isAck = true;
                }
                else if (SequenceNumberLessThan(seqNum, firstToAck)) // if on the left of the range (older)
                {
                    isNAck = true; // if we are before firstToAck consider it NAck
                }
                else
                {
                    break; // if on the right on the range (we are done)
                }
                if (isAck)
                {
                    dgIter = connection->m_sendDataGrams.erase_after(dgPrevIter); // remove datagram from list

                    bool windowChanged = false;
                    m_trafficControl->OnAck(connection, dg.m_flowControl, windowChanged);
                    if(windowChanged)
                    {
                        NotifyRateUpdate(connection);
                    }

                    if ( dg.m_ackCallbacks.size() > 0)
                    {
                        ThreadMessage* mtm = aznew ThreadMessage(MTM_ACK_NOTIFY);
                        mtm->m_connection = connection->m_mainConnection;
                        mtm->m_ackCallbacks = AZStd::move(dg.m_ackCallbacks);
                        PushMainThreadMessage(mtm);
                    }
                    FreeDatagram(dg);
                    //AZ_TracePrintf("GridMate","%p ACK: %d from %s\n",this,dg.m_flowControl.m_sequenceNumber,connection->m_target->ToString().c_str());
                }
                else
                {
                    if (isNAck)
                    {
                        m_trafficControl->OnNAck(connection, dg.m_flowControl);
                        //AZ_TracePrintf("GridMate", "%p NACK: %d in (%d) from %s\n", this, dg.m_flowControl.m_sequenceNumber, seqNum, connection->m_target->ToString().c_str());
                    }

                    ++dgIter;
                    ++dgPrevIter;
                }
            }
        }
        else
        {
            // Process bit ACKS (TODO reverse the traverse, sendDataGrams is sorted old->new)
            int ackIndex = ackNumHistoryBits - 1;
            SequenceNumber ackPacket = lastToAck - static_cast<SequenceNumber>(ackNumHistoryBits);
            bool isAck;
            DatagramDataListType::iterator dgPrevIter = connection->m_sendDataGrams.before_begin();
            DatagramDataListType::iterator dgIter = connection->m_sendDataGrams.begin();
            while (ackIndex >= -1 && dgIter != connection->m_sendDataGrams.end())
            {
                if (ackIndex >= 0)
                {
                    int byteIndex = ackIndex / 8;
                    int byteOffset = ackIndex % 8;
                    isAck = ((ackHistoryBitsStorage[byteIndex] & (1 << byteOffset)) != 0);
                }
                else
                {
                    isAck = true; // lastToAck
                }

                // connection->sendDataGrams is a sorted in acceding order list.
                while (SequenceNumberLessThan(dgIter->m_flowControl.m_sequenceNumber, ackPacket))
                {
                    m_trafficControl->OnNAck(connection, dgIter->m_flowControl);
                    //AZ_TracePrintf("GridMate", "%p NACK: %d in (%d) from %s\n", this, dgIter->m_flowControl.m_sequenceNumber, ackPacket, connection->m_target->ToString().c_str());
                    ++dgPrevIter;
                    ++dgIter;
                    if (dgIter == connection->m_sendDataGrams.end())  // traversed all sent packets
                    {
                        return;
                    }
                }

                if (dgIter->m_flowControl.m_sequenceNumber == ackPacket)
                {
                    DatagramData& dg = *dgIter;
                    if (isAck)
                    {
                        bool windowChanged = false;
                        dgIter = connection->m_sendDataGrams.erase_after(dgPrevIter); // remove from list
                        m_trafficControl->OnAck(connection, dg.m_flowControl, windowChanged);
                        if (windowChanged)
                        {
                            NotifyRateUpdate(connection);
                        }
                        FreeDatagram(dg);
                        //AZ_TracePrintf("GridMate", "%p ACK: %d in (%d) from %s\n", this, dg.m_flowControl.m_sequenceNumber, ackPacket, connection->m_target->ToString().c_str());
                    }
                    else
                    {
                        m_trafficControl->OnNAck(connection, dg.m_flowControl);
                        //AZ_TracePrintf("GridMate", "%p NACK: %d in (%d) from %s\n", this, dg.m_flowControl.m_sequenceNumber, ackPacket, connection->m_target->ToString().c_str());
                        ++dgPrevIter;
                        ++dgIter;
                    }
                }

                ++ackPacket;
                --ackIndex;
            }
        }
    }
}

//=========================================================================
// WriteDataGramHeader
// [10/7/2010]
//=========================================================================
inline void
CarrierThread::WriteDataGramHeader(WriteBuffer& writeBuffer, const DatagramData& dgram)
{
    // Set the datagram ID
    writeBuffer.Write(dgram.m_flowControl.m_sequenceNumber);
}

//=========================================================================
// ReadDataGramHeader
// [10/7/2010]
//=========================================================================
SequenceNumber
CarrierThread::ReadDataGramHeader(ReadBuffer& readBuffer)
{
    SequenceNumber sequenceNumber;
    readBuffer.Read(sequenceNumber);
    return sequenceNumber;
}

//=========================================================================
// IsConnectRequestDataGram
// Parses datagram to find out if it contains connection request
//=========================================================================
bool CarrierThread::IsConnectRequestDataGram(const char* data, size_t dataSize) const
{
    MessageData msg;
    ReadBuffer tempBuf(kCarrierEndian, data, dataSize);
    tempBuf.Skip(sizeof(SequenceNumber));
    SequenceNumber tempSeq[m_maxNumberOfChannels];
    unsigned char channel = 0;
    if (ReadMessageHeader(tempBuf, msg, tempSeq, tempSeq, channel))
    {
        AZ::u8 msgId = 0;
        size_t messageIdOffset = msg.m_dataSize - sizeof(msgId);

        tempBuf.Skip(messageIdOffset);
        tempBuf.Read(msgId);
        return !tempBuf.IsOverrun() && msgId == SM_CONNECT_REQUEST;
    }
    else
    {
        return false;
    }
}

//=========================================================================
// ProcessIncomingDataGram
// [10/7/2010]
//=========================================================================
inline void
CarrierThread::ProcessIncomingDataGram(ThreadConnection* connection, DatagramData& dgram, ReadBuffer& readBuffer)
{
    if (connection->m_receivedDatagramsHistory.IsFull())  // if we are full make sure we don't miss to ack any packets (at least once)
    {
        SequenceNumber firstId = connection->m_receivedDatagramsHistory.At(connection->m_receivedDatagramsHistory.Begin());

        if (SequenceNumberLessThan(connection->m_lastAckedDatagram, firstId)) // we are about to lose some packets
        {
            //AZ_TracePrintf("GridMate","%p Send Extra Ack LastAcked: %d First: %d\n",this,connection->m_lastAckedDatagram,firstId);
            WriteBufferStatic<> wb(kCarrierEndian);
            WriteAckData(connection, wb);
            SendSystemMessage(SM_CT_ACKS, wb, connection);
        }
    }

    bool duplicate = ! connection->m_receivedDatagramsHistory.Insert(dgram.m_flowControl.m_sequenceNumber);
    // We should detect bad connection anyway, but it will be good to check if the packet ID is within reasonable expected one.
    //bool isWithinReasonable = SequenceNumberSequentialDistance(connection->m_receivedDatagramsHistory.First(),dgram.m_flowControl.m_sequenceNumber) < DataGramHistoryList::m_maxNumberOfElements;
    if (duplicate /*|| isWithinReasonable == false*/)
    {
        //Duplicate packet received. Could be nominal network behaviors or malicious activity. Report it!
        ThreadMessage* mtm = aznew ThreadMessage(MTM_ON_ERROR);
        mtm->m_errorCode = CarrierErrorCode::EC_SECURITY;
        mtm->m_error.m_securityError.m_errorCode = SecurityError::EC_SEQUENCE_NUMBER_DUPLICATED;
        PushMainThreadMessage(mtm);
        return;
    }

    //AZ_TracePrintf("GridMate","%p incoming %d\n",this,dgram.m_flowControl.m_sequenceNumber);

    dgram.m_flowControl.m_effectiveSize = 0;

    // prevSeqNum and prevReliableSeqNum should not be init to 0, ReadMessageHeader will set the first value (always)
    SequenceNumber prevSeqNum[m_maxNumberOfChannels], prevReliableSeqNum[m_maxNumberOfChannels];
    unsigned char channel = 0; // We need to init to 0 because we imply 0 channel if it's not set.

    while (!readBuffer.IsEmpty() && !readBuffer.IsOverrun())
    {
        MessageData& msg = AllocateMessage();
        if (!ReadMessageHeader(readBuffer, msg, prevSeqNum, prevReliableSeqNum, channel))
        {
            connection->m_isBadPackets = true;
            break;
        }
        AZ_Assert(msg.m_dataSize, "Received Message with 0 size! This is bad data!");

        // Process carrier thread system messages (they should execute really quick, otherwise a blocking will occur).
        if (msg.m_channel == m_systemChannel)
        {
            size_t messageIdSize = 1; // 1 byte
            size_t messageIdOffset = msg.m_dataSize - messageIdSize;

            // Ensure that the system message id offset lies within the read buffer. If it does
            // not, skip these bytes in the buffer which will mark the buffer as overflowed and
            // be handled accordingly.
            if ((messageIdSize + messageIdOffset) > readBuffer.Left())
            {
                readBuffer.Skip(messageIdOffset + messageIdSize);
                continue;
            }

#ifdef GM_CARRIER_MESSAGE_CRC
            messageIdOffset -= sizeof(AZ::u32);
            AZ::u32 dataCrc = AZ::Crc32(readBuffer.GetCurrent(), msg.m_dataSize - sizeof(AZ::u32));
            AZ::u32 sendCrc = *reinterpret_cast<const AZ::u32*>(readBuffer.GetCurrent() + msg.m_dataSize - sizeof(AZ::u32));
            AZ_Assert(dataCrc == sendCrc, "System Message Crc mismatch 0x%08x vs 0x%08x", dataCrc, sendCrc);
#endif //GM_CARRIER_MESSAGE_CRC

            // The system message id is encoded in 1 byte in the buffer, so it is read into a char before
            // being cast to the SystemMessageId enum which is 4 bytes long.
            AZ::u8 msgIdRaw = *(readBuffer.GetCurrent() + messageIdOffset);
            SystemMessageId msgId = static_cast<SystemMessageId>(msgIdRaw);

            if (msgId > SM_CT_FIRST)  // if it's carrier thread message
            {
                // We do the reliable processing at a later stage and we need buffer and so on for it. We can
                // do reliable processing for those messages too, but it should not be needed.
                AZ_Assert(msg.m_reliability == Carrier::SEND_UNRELIABLE, "All carrier thread messages must be unreliable!");
                switch (msgId)
                {
                case SM_CT_ACKS:
                    ReadAckData(connection, readBuffer);
                    break;
                default:
                    readBuffer.Skip(messageIdOffset);
                    break;
                }
                readBuffer.Skip(messageIdSize); // the message id (we already have it)
#ifdef GM_CARRIER_MESSAGE_CRC
                readBuffer.Skip(sizeof(AZ::u32));
#endif // #ifdef GM_CARRIER_MESSAGE_CRC
                FreeMessage(msg);
                continue;
            }
        }

        if (msg.m_channel >= m_maxNumberOfChannels) // checking for spoofed channel
        {
            //Corrupted packet or malicious activity. Report It!
            ThreadMessage* mtm = aznew ThreadMessage(MTM_ON_ERROR);
            mtm->m_errorCode = CarrierErrorCode::EC_SECURITY;
            mtm->m_error.m_securityError.m_errorCode = SecurityError::EC_CHANNEL_ID_OUT_OF_BOUND;
            PushMainThreadMessage(mtm);
            continue;
        }

        /**
        * Sort incoming packets on based on their sequence number (regardless of reliability)
        * To improve performance when we have big lists of packets we cache the last insert
        * position "m_receivedLastInsert" and use that as a starting point when possible.
        */

        // Check if receive a duplicated packet that we have already processed
        bool isDuplicated = !SequenceNumberIsSequential(connection->m_receivedSeqNum[msg.m_channel], msg.m_sequenceNumber);

        // check if we already received this message and process it.
        if (!isDuplicated)
        {
            // find insertion position

            // start from the last insert position
            auto insertPos = connection->m_receivedLastInsert[msg.m_channel];

            if (connection->m_receivedLastInsert[msg.m_channel] == connection->m_received[msg.m_channel].end() ||  // we are at the end of list
                SequenceNumberLessThan(msg.m_sequenceNumber, insertPos->m_sequenceNumber)) // the message is in the past
            {
                // search backward
                auto first = connection->m_received[msg.m_channel].begin();
                while (insertPos != first)
                {
                    --insertPos;
                    if (insertPos->m_sequenceNumber == msg.m_sequenceNumber) // check for duplication (already in the list)
                    {
                        isDuplicated = true;
                        break;
                    }
                    if (SequenceNumberGreaterThan(msg.m_sequenceNumber, insertPos->m_sequenceNumber))
                    {
                        ++insertPos; // go to next to insert element
                        // we found our position
                        break;
                    }
                }
            }
            else
            {
                // search forward
                auto last = connection->m_received[msg.m_channel].end();
                for (; insertPos != last; ++insertPos)
                {
                    if (insertPos->m_sequenceNumber == msg.m_sequenceNumber) // check for duplication (already in the list)
                    {
                        isDuplicated = true;
                        break;
                    }

                    if (SequenceNumberGreaterThan(insertPos->m_sequenceNumber, msg.m_sequenceNumber)) // we have found our position
                    {
                        break;
                    }
                }
            }

            if (!isDuplicated)
            {
                // insert new packet and cache insert position
                connection->m_receivedLastInsert[msg.m_channel] = connection->m_received[msg.m_channel].insert(insertPos, msg);
            }
        }

        if (isDuplicated)
        {
            // If this is duplicated packet, just ignore the package
            readBuffer.Skip(msg.m_dataSize);
            FreeMessage(msg);
        }
        else
        {
            // LMBR-23475: AllocateMessageData() below always allocates m_maxDataGramSizeBytes bytes
            //             of data regardless of the size argument (msg.m_dataSize). As a fix, reject
            //             any datagram that is larger than m_maxMsgDataSizeBytes, which is
            //             m_maxDataGramSizeBytes with the header sizes subtracted (i.e. smaller). This
            //             is conveniently also the maximum size of a message chunk (used in
            //             CarrierImpl::GenerateSendMessages()).
            if (msg.m_dataSize > m_maxMsgDataSizeBytes)
            {
                //Corrupted packet or malicious activity. Report It!
                ThreadMessage* mtm = aznew ThreadMessage(MTM_ON_ERROR);
                mtm->m_errorCode = CarrierErrorCode::EC_SECURITY;
                mtm->m_error.m_securityError.m_errorCode = SecurityError::EC_DATAGRAM_TOO_LARGE;
                PushMainThreadMessage(mtm);
                break;
            }

            // Read the payload
            msg.m_data = AllocateMessageData(msg.m_dataSize);
            readBuffer.ReadRaw(msg.m_data, msg.m_dataSize);
            if (msg.m_channel != m_systemChannel)
            {
                dgram.m_flowControl.m_effectiveSize += msg.m_dataSize;
            }
        }
    }

    if (readBuffer.IsOverrun())
    {
        connection->m_isBadPackets = true;      //Should not occur. State might be corrupt. Abort!
    }

    connection->m_lastReceivedDatagramTime = m_currentTime;
}

void CarrierThread::InitOutgoingDatagram(ThreadConnection* connection, WriteBuffer& writeBuffer)
{
    // send acks if needed
    if (m_trafficControl->IsSendAck(connection))
    {
        WriteAckData(connection, writeBuffer);
        SendSystemMessage(SM_CT_ACKS, writeBuffer, connection);
        writeBuffer.Clear();
    }
}

void CarrierThread::GenerateOutgoingDataGram(ThreadConnection* connection, DatagramData& dgram, WriteBuffer& writeBuffer, OutgoingDataGramContext& ctx, size_t maxDatagramSize)
{
    if (maxDatagramSize <= GetDataGramHeaderSize())
    {
        return;
    }

    dgram.m_flowControl.m_sequenceNumber = connection->m_dataGramSeqNum + 1;
    dgram.m_resendDataSize = 0;
    dgram.m_flowControl.m_effectiveSize = 0;

    WriteDataGramHeader(writeBuffer, dgram);

    Connection* mainConn = connection->m_mainConnection;
    unsigned char channel = 0;

    for (int iPriority = 0; iPriority < Carrier::PRIORITY_MAX; ++iPriority)
    {
        if (mainConn->m_state != Carrier::CST_CONNECTED && iPriority != Carrier::PRIORITY_SYSTEM)
        {
            continue; // only system priority messages are processed
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        //TODO: add extra resend test here of unAck'd data older than X ms
        ////////////////////////////////////////////////////////////////////////////////////////////////

        if (!mainConn->m_toSend[iPriority].empty())
        {
            AZStd::lock_guard<AZStd::mutex> l(mainConn->m_toSendLock);
            while (!mainConn->m_toSend[iPriority].empty())
            {
                MessageData& msg = mainConn->m_toSend[iPriority].front();

                bool isWriteChannel = false;
                if (msg.m_channel != channel)
                {
                    isWriteChannel = true;
                    channel = msg.m_channel;
                }

                //////////////////////////////////////////////////////////////////////////
                // Check if messages are sequential, so we don't need to store the ID (just increment it)
                bool isWriteMessageSequenceId = true;
                bool isWriteReliableMessageSequenceId = true;

                if (ctx.isWrittenFirstSequenceNum[channel])  // we always write the first message id, then we check for sequence.
                {
                    isWriteMessageSequenceId = (SequenceNumberSequentialDistance(ctx.lastSequenceNumber[channel], msg.m_sequenceNumber) != 1);
                }
                else
                {
                    ctx.isWrittenFirstSequenceNum[channel] = true;
                }

                if (ctx.isWrittenFirstRelSeqNum[channel])  // we always write the first reliable message id.
                {
                    // We always write the reliable sequence number.
                    // For unreliable is used to make sure they are NOT
                    // delivered before last reliable message.
                    // But we increment it (reliable sequence number) only for reliable msg.
                    if (msg.m_reliability == Carrier::SEND_RELIABLE)
                    {
                        isWriteReliableMessageSequenceId = (SequenceNumberSequentialDistance(ctx.lastSeqReliableNumber[channel], msg.m_sendReliableSeqNum) != 1);
                    }
                    else
                    {
                        isWriteReliableMessageSequenceId = (ctx.lastSeqReliableNumber[channel] != msg.m_sendReliableSeqNum);
                    }
                }
                else
                {
                    ctx.isWrittenFirstRelSeqNum[channel] = true;
                }
                //////////////////////////////////////////////////////////////////////////

                //If this Message can fit into the remaining Datagram buffer space
                if ((msg.m_dataSize + GetMessageHeaderSize(msg, isWriteMessageSequenceId, isWriteReliableMessageSequenceId, isWriteChannel)) 
                        > (maxDatagramSize - writeBuffer.Size()))
                {
                    break; // we can't add this message
                }

                // we can fit this message in the datagram
                mainConn->m_toSend[iPriority].pop_front();
                mainConn->m_bytesInQueue -= msg.m_dataSize;

                //////////////////////////////////////////////////////////////////////////
                if (mainConn->m_rateLimitedByQueueSize)
                {
                    NotifyRateUpdate(connection);
                }
                //////////////////////////////////////////////////////////////////////////

                WriteMessageHeader(writeBuffer, msg, isWriteMessageSequenceId, isWriteReliableMessageSequenceId, isWriteChannel);

                // add the message data to the datagram
                writeBuffer.WriteRaw(msg.m_data, msg.m_dataSize);

                if (msg.m_channel != m_systemChannel)
                {
                    dgram.m_flowControl.m_effectiveSize += msg.m_dataSize;
                }

                // store the last message id
                ctx.lastSequenceNumber[channel] = msg.m_sequenceNumber;

                // if the message is reliable add it to the resend list
                if (msg.m_reliability == Carrier::SEND_RELIABLE)
                {
                    dgram.m_toResend[iPriority].push_back(msg);
                    dgram.m_resendDataSize += msg.m_dataSize;

                    // store the last reliable message id
                    ctx.lastSeqReliableNumber[channel] = msg.m_sendReliableSeqNum;
                }
                else
                {
                    if (msg.m_ackCallback)
                    {
                        dgram.m_ackCallbacks.push_back(AZStd::move(msg.m_ackCallback));
                    }
                    FreeMessage(msg);
                }
            }
        }
    }
}

void CarrierThread::OnReceivedIncomingDataGram(ThreadConnection* from, ReadBuffer& readBuffer, unsigned int recvDataGramSize)
{
    DatagramData& dgram = AllocateDatagram();

    // read data gram header and store the payload data pointer
    SequenceNumber dgramSequenceNumber = ReadDataGramHeader(readBuffer);

    dgram.m_flowControl.m_sequenceNumber = dgramSequenceNumber;
    dgram.m_flowControl.m_size = static_cast<unsigned short>(recvDataGramSize); // saving original datagram size before decompression for flow control and correct statistics

    ProcessIncomingDataGram(from, dgram, readBuffer);

    m_trafficControl->OnReceived(from, dgram.m_flowControl);
    FreeDatagram(dgram);
}

#if 0
//=========================================================================
// ResendDataGram
// [11/12/2010]
//=========================================================================
inline void
CarrierThread::ResendDataGram(ThreadConnection* connection, DatagramData& dgram, WriteBuffer& writeBuffer)
{
    // Write the datagram header
    WriteDataGramHeader(writeBuffer, dgram);

    // generate
    SequenceNumber lastSequenceNumber[MaxNumberOfChannels] = {0};
    SequenceNumber lastSeqReliableNumber[MaxNumberOfChannels] = {0};
    bool isWrittenFirstSequenceNum[MaxNumberOfChannels] = {0};
    bool isWrittenFirstRelSeqNum[MaxNumberOfChannels] = {0};
    unsigned char channel = 0;

    for (int iPriority = 0; iPriority < Carrier::PRIORITY_MAX; ++iPriority)
    {
        for (MessageDataListType::iterator msgIter = dgram.toResend[iPriority].begin(); msgIter != dgram.toResend[iPriority].end(); ++msgIter)
        {
            MessageData& msg = *msgIter;

            bool isWriteChannel = false;
            if (msg.channel != channel)
            {
                isWriteChannel = true;
                channel = msg.channel;
            }

            //////////////////////////////////////////////////////////////////////////
            // Check if messages are sequential, so we don't need to store the ID (just increment it)
            bool isWriteMessageSequenceId = true;
            bool isWriteReliableMessageSequenceId = true;

            if (isWrittenFirstSequenceNum[channel])  // we always write the first message id, then we check for sequence.
            {
                isWriteMessageSequenceId = (SequenceNumberSequentialDistance(lastSequenceNumber[channel], msg.sequenceNumber) != 1);
            }
            else
            {
                isWrittenFirstSequenceNum[channel] = true;
            }

            AZ_Assert(msg.reliability == Carrier::SEND_RELIABLE, "All messages that we resend must be reliable! Internal Carrier Error");
            {
                if (isWrittenFirstRelSeqNum[channel])  // we always write the first reliable message id.
                {
                    isWriteReliableMessageSequenceId = (SequenceNumberSequentialDistance(lastSeqReliableNumber[channel], msg.reliableSeqNum) != 1);
                }
                else
                {
                    isWrittenFirstRelSeqNum[channel] = true;
                }
            }
            //////////////////////////////////////////////////////////////////////////

            WriteMessageHeader(writeBuffer, msg, isWriteMessageSequenceId, isWriteReliableMessageSequenceId, isWriteChannel);

            // add the message data to the datagram
            writeBuffer.WriteRaw(msg.data, msg.dataSize);

            // store the message numbers
            lastSequenceNumber[channel] = msg.sequenceNumber;
            lastSeqReliableNumber[channel] = msg.reliableSeqNum;
        }
    }
}
#endif //0

//=========================================================================
// SendSystemMessage
// [8/7/2012]
//=========================================================================
void
CarrierThread::SendSystemMessage(SystemMessageId id, WriteBuffer& wb, ThreadConnection* target)
{
    // Currently the send queue, sequence numbers and connection state are owned by the main connection,
    // so there is no way to send anything out it the main connection has been terminated for whatever reason.
    // This flow prevents the network from being able to flush remaining system messages before tear down,
    // like a tear down message for example, so it should really be re-evaluated.
    // For now, don't try to send if the main connection is already down.
    // TODO Share the code with GenerateMessage
    if (!target->m_mainConnection)
    {
        AZ_TracePrintf("GridMate", "Discarding outbound system message 0x%x. Can't send any more system messages because the main connection has been torn down.\n", id);
        return;
    }
    Connection* conn = target->m_mainConnection;

    AZ_Assert(id > SM_CT_FIRST, "This function is for CarrierThread system messages!");
    unsigned char byteId = static_cast<unsigned char>(id);
    /*
    * On the receiving end, carrier reads this message id at the end of the buffer and
    * assumes it's last byte in the messages, thus, if we have been bit packing, we have to make sure we are byte alignment as we message id.
    */
    wb.WriteWithByteAlignment(byteId);

#if defined(GM_CARRIER_MESSAGE_CRC)
    {
        AZ::u32 dataCrc = AZ::Crc32(wb.Get(), wb.Size());
        wb.WriteRaw(&dataCrc, sizeof(dataCrc));
    }
#endif // AZ_DEBUG_BUILD

    const char* data = wb.Get();
    unsigned short dataSize = static_cast<unsigned short>(wb.Size());
    AZ_Assert(dataSize <= m_maxMsgDataSizeBytes, "System message is too long, we don't support split for Carrier system messages!");
    CarrierImpl::DataReliability reliability = CarrierImpl::SEND_UNRELIABLE;
    CarrierImpl::DataPriority priority = CarrierImpl::PRIORITY_SYSTEM;
    unsigned char channel = m_systemChannel;
    unsigned short dataSendStep = dataSize;
    unsigned short numChunks = 1;

    {
        AZStd::lock_guard<AZStd::mutex> l(conn->m_toSendLock);
        //do{
        void* dataBuffer = AllocateMessageData(dataSendStep);
        //////////////////////////////////////////////////////////////////////////
        memcpy(dataBuffer, data, dataSendStep);

        MessageData& msg = AllocateMessage();
        msg.m_channel = channel;
        msg.m_numChunks = numChunks;
        msg.m_dataSize = dataSendStep;
        msg.m_reliability = reliability;
        msg.m_data = dataBuffer;
        msg.m_sequenceNumber = ++conn->m_sendSeqNum[channel];
        msg.m_isConnecting = conn->m_state == Carrier::CST_CONNECTING;
        if (reliability == CarrierImpl::SEND_RELIABLE)
        {
            ++conn->m_sendReliableSeqNum[channel];
        }
        msg.m_sendReliableSeqNum = conn->m_sendReliableSeqNum[channel];
        conn->m_toSend[priority].push_back(msg);
        conn->m_bytesInQueue += msg.m_dataSize;

        data += dataSendStep;
        dataSize -= dataSendStep;
        //numChunks--;
        if (dataSendStep > dataSize)
        {
            dataSendStep = dataSize;
        }
        //}while(dataSize>0);
    }
}

//=========================================================================
// GetMaxMessageHeaderSize
// [10/8/2010]
//=========================================================================
inline unsigned int
CarrierThread::GetMaxMessageHeaderSize() const
{
    static const unsigned int flags = 1;            /// u8
    static const unsigned int dataSize = 2;         /// u16

    static const unsigned int channelInfo = 1;      ///< u8
    static const unsigned int splitPacketInfo = sizeof(SequenceNumber); ///< Added only if we have split information
    static const unsigned int sequenceNumber = sizeof(SequenceNumber);  ///< Dynamic and true only for the first message
    static const unsigned int sequenceReliableNumber = sizeof(SequenceNumber);  ///< Dynamic and true only for the first reliable message

    return flags + dataSize + sequenceNumber + splitPacketInfo + sequenceReliableNumber + channelInfo;
}

//=========================================================================
// GetMessageHeaderSize
// [7/31/2012]
//=========================================================================
inline unsigned int
CarrierThread::GetMessageHeaderSize(const MessageData& msg, bool isWriteSeqNumber, bool isWriteReliableSeqNum, bool isWriteChannel) const
{
    unsigned int size = sizeof(char) /* flags */ + sizeof(msg.m_dataSize);

    if (isWriteChannel)
    {
        size += sizeof(msg.m_channel);
    }
    if (msg.m_numChunks > 1)
    {
        size += sizeof(msg.m_numChunks);
    }
    if (isWriteSeqNumber)
    {
        size += sizeof(msg.m_sequenceNumber);
    }
    if (isWriteReliableSeqNum)
    {
        size += sizeof(msg.m_sendReliableSeqNum);
    }

    return size;
}

//=========================================================================
// WriteMessageHeader
// [10/8/2010]
//=========================================================================
inline void
CarrierThread::WriteMessageHeader(WriteBuffer& writeBuffer, const MessageData& msg, bool isWriteSeqNumber, bool isWriteReliableSeqNum, bool isWriteChannel) const
{
    char flags = 0;
    if (msg.m_reliability == Carrier::SEND_RELIABLE)
    {
        flags |= MessageData::MF_RELIABLE;
    }
    if (msg.m_numChunks > 1)
    {
        flags |= MessageData::MF_CHUNKS;
    }
    if (!isWriteSeqNumber)
    {
        flags |= MessageData::MF_SQUENTIAL_ID;
    }
    if (!isWriteReliableSeqNum)
    {
        flags |= MessageData::MF_SQUENTIAL_REL_ID;
    }
    if (isWriteChannel)
    {
        flags |= MessageData::MF_DATA_CHANNEL;
    }
    if (msg.m_isConnecting)
    {
        flags |= MessageData::MF_CONNECTING;
    }

    writeBuffer.Write(flags);
    writeBuffer.Write(msg.m_dataSize);

    if (isWriteChannel)
    {
        writeBuffer.Write(msg.m_channel);
    }
    if (msg.m_numChunks > 1)
    {
        writeBuffer.Write(msg.m_numChunks);
    }
    if (isWriteSeqNumber)
    {
        writeBuffer.Write(msg.m_sequenceNumber);
    }
    if (isWriteReliableSeqNum)
    {
        writeBuffer.Write(msg.m_sendReliableSeqNum);
    }
}

//=========================================================================
// ReadMessageHeader
// [10/8/2010]
//=========================================================================
inline bool
CarrierThread::ReadMessageHeader(ReadBuffer& readBuffer, MessageData& msg, SequenceNumber* prevMsgSeqNum, SequenceNumber* prevReliableMsgSeqNum, unsigned char& channel) const
{
    char flags;
    readBuffer.Read(flags);
    readBuffer.Read(msg.m_dataSize);

    if (flags & MessageData::MF_RELIABLE)
    {
        msg.m_reliability = Carrier::SEND_RELIABLE;
    }
    else
    {
        msg.m_reliability = Carrier::SEND_UNRELIABLE;
    }

    if (flags & MessageData::MF_DATA_CHANNEL)
    {
        readBuffer.Read(channel);
    }

    if (channel >= m_maxNumberOfChannels)
    {
        return false;
    }

    msg.m_channel = channel;

    if (flags & MessageData::MF_CHUNKS)
    {
        readBuffer.Read(msg.m_numChunks);
    }
    else
    {
        msg.m_numChunks = 1;
    }

    if (flags & MessageData::MF_SQUENTIAL_ID)
    {
        ++prevMsgSeqNum[channel];
    }
    else
    {
        readBuffer.Read(prevMsgSeqNum[channel]);
    }
    msg.m_sequenceNumber = prevMsgSeqNum[channel];

    if (flags & MessageData::MF_SQUENTIAL_REL_ID)
    {
        // We always write the reliable sequence number.
        // For unreliable is used to make sure they are NOT
        // delivered before last reliable message.
        // But we increment it (reliable sequence number) only for reliable msg.
        if (flags & MessageData::MF_RELIABLE)
        {
            ++prevReliableMsgSeqNum[channel];
        }
    }
    else
    {
        readBuffer.Read(prevReliableMsgSeqNum[channel]);
    }
    msg.m_sendReliableSeqNum = prevReliableMsgSeqNum[channel];

    msg.m_isConnecting = flags & MessageData::MF_CONNECTING ? true : false;

    return true;
}

//=========================================================================
// HasDataToSend
// [10/13/2010]
//=========================================================================
inline bool
CarrierThread::HasDataToSend(ThreadConnection* connection)
{
    if (connection->m_mainConnection)
    {
        //      AZStd::lock_guard<AZStd::mutex> l(connection->m_mainConnection->m_toSendLock);
        for (AZStd::size_t i = 0; i < Carrier::PRIORITY_MAX; ++i)
        {
            if (connection->m_mainConnection->m_state != Carrier::CST_CONNECTED && i != Carrier::PRIORITY_SYSTEM)
            {
                continue; // only system priority messages are processed
            }
            if (!connection->m_mainConnection->m_toSend[i].empty())
            {
                return true;
            }
        }
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CarrierImpl - main thread
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// CarrierImpl
// [9/14/2010]
//=========================================================================
CarrierImpl::CarrierImpl(const CarrierDesc& desc, IGridMate* gridMate)
    : Carrier(gridMate)
{
    //////////////////////////////////////////////////////////////////////////
    // Handshake
    m_ownHandshake = false;
    m_handshake = desc.m_handshake;

    if (m_handshake == 0)
    {
        m_ownHandshake = true;
        m_handshake = aznew DefaultHandshake(desc.m_connectionTimeoutMS, desc.m_version);
    }
    //////////////////////////////////////////////////////////////////////////

    // used to initialize the clock
    StartClockSync(InvalidSyncInterval, true);

    Compressor* compressor = nullptr;
    if (desc.m_compressionFactory) // Initializing compression
    {
        compressor = desc.m_compressionFactory->CreateCompressor();
    }

    m_thread = aznew CarrierThread(desc, compressor);
    m_thread->m_handshakeTimeoutMS = m_handshake->GetHandshakeTimeOutMS();
    m_maxMsgDataSizeBytes = m_thread->m_maxMsgDataSizeBytes;
    m_maxSendRateMS = desc.m_threadUpdateTimeMS;
    m_connectionRetryIntervalBase = desc.m_connectionRetryIntervalBase;
    m_connectionRetryIntervalMax = desc.m_connectionRetryIntervalMax;
    m_port = m_thread->m_driver->GetPort();
}

//=========================================================================
// ~CarrierImpl
// [9/14/2010]
//=========================================================================
CarrierImpl::~CarrierImpl()
{
    Shutdown();

    while (!m_connections.empty())
    {
        DeleteConnection(m_connections.front(), CarrierDisconnectReason::DISCONNECT_SHUTTING_DOWN);
    }

    delete m_thread;
    m_thread = NULL;

    if (m_ownHandshake)
    {
        delete m_handshake;
        m_handshake = NULL;
    }
}

//=========================================================================
// Shutdown
// [4/15/2016]
//=========================================================================
void CarrierImpl::Shutdown()
{
    m_thread->Quit();
}

//=========================================================================
// Connect
// [9/14/2010]
//=========================================================================
ConnectionID
CarrierImpl::Connect(const char* hostAddress, unsigned int port)
{
    return Connect(m_thread->m_driver->IPPortToAddress(hostAddress, port));
}

//=========================================================================
// Connect
// [1/12/2011]
//=========================================================================
ConnectionID
CarrierImpl::Connect(const string& address)
{
    // check if we don't have it in the list.
    for (ConnectionList::iterator i = m_connections.begin(); i != m_connections.end(); ++i)
    {
        if ((*i)->m_fullAddress == address)
        {
            return *i;
        }
    }

    Connection* conn = aznew Connection(m_thread, address);
    m_connections.push_back(conn);

    auto it = m_pendingHandshakes.emplace(conn);
    AZ_Assert(it.second, "Failed to create handshake object");
    if (!it.second)
    {
        delete conn;
        return nullptr;
    }

    PendingHandshake& handshake = *it.first;

    ThreadMessage* ctm = aznew ThreadMessage(CTM_CONNECT);
    ctm->m_connection = conn;
    m_thread->PushCarrierThreadMessage(ctm);

    //////////////////////////////////////////////////////////////////////////
    // Send timer and request to connect.
    SendSyncTime();  // send time sync first if we are the clock.

    m_handshake->OnInitiate(conn, handshake.m_payload);
    AZ_Warning("GridMate", handshake.m_payload.Size() > 0, "You should provide initiatial handshake data! This is not only important for version check, but connect/disconnect issues!");
    SendSystemMessage(SM_CONNECT_REQUEST, handshake.m_payload, conn, SEND_UNRELIABLE);
    handshake.m_retryTime = AZStd::chrono::system_clock::now() + AZStd::chrono::milliseconds(m_connectionRetryIntervalBase);
    //////////////////////////////////////////////////////////////////////////

    return conn;
}

//=========================================================================
// Disconnect
// [9/14/2010]
//=========================================================================
void
CarrierImpl::Disconnect(ConnectionID id)
{
    if (id != InvalidConnectionID)
    {
        AZ_Assert(id == AllConnections || AZStd::find(m_connections.begin(), m_connections.end(), id) != m_connections.end(), "This connection ID is not valid! Not in the list 0x%08x", id);
        DisconnectRequest(id, CarrierDisconnectReason::DISCONNECT_USER_REQUESTED);
    }
}

//=========================================================================
// DisconnectRequest
// [9/14/2010]
//=========================================================================
void
CarrierImpl::DisconnectRequest(ConnectionID id, CarrierDisconnectReason reason)
{
    AZ_Assert(id != InvalidConnectionID, "Invalid connection id!");
    if (id == AllConnections)
    {
        for (AZStd::size_t i = 0; i < m_connections.size(); ++i)
        {
            DisconnectRequest(m_connections[i], reason);
        }
    }
    else if (id != InvalidConnectionID)
    {
        Connection* conn = static_cast<Connection*>(id);
        //AZ_TracePrintf("GridMate","DisconnectRequest %s\n",conn->fullAddress.c_str());
        switch (conn->m_state)
        {
        case Carrier::CST_CONNECTED:
        {
            conn->m_state = Carrier::CST_DISCONNECTING;
            EBUS_EVENT(Debug::CarrierDrillerBus, OnConnectionStateChanged, this, conn, conn->m_state);
            m_handshake->OnDisconnect(conn);
            EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnDisconnect, this, id, reason);
            EBUS_EVENT(Debug::CarrierDrillerBus, OnDisconnect, this, id, reason);

            ThreadMessage* ctm = aznew ThreadMessage(CTM_DISCONNECT);
            ctm->m_connection = conn;
            m_thread->PushCarrierThreadMessage(ctm);

            WriteBufferStatic<> wb(kCarrierEndian);
            wb.Write(reason);
            SendSystemMessage(SM_DISCONNECT, wb, id, SEND_RELIABLE);
        } break;
        case Carrier::CST_CONNECTING:
        {
            conn->m_state = Carrier::CST_DISCONNECTING;
            EBUS_EVENT(Debug::CarrierDrillerBus, OnConnectionStateChanged, this, id, conn->m_state);
            m_handshake->OnDisconnect(conn);
            EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnFailedToConnect, this, id, reason);
            EBUS_EVENT(Debug::CarrierDrillerBus, OnFailedToConnect, this, id, reason);

            ThreadMessage* ctm = aznew ThreadMessage(CTM_DISCONNECT);
            ctm->m_connection = conn;
            m_thread->PushCarrierThreadMessage(ctm);

            WriteBufferStatic<> wb(kCarrierEndian);
            wb.Write(reason);
            SendSystemMessage(SM_DISCONNECT, wb, id, SEND_RELIABLE);
        } break;
        case Carrier::CST_DISCONNECTED:
        case Carrier::CST_DISCONNECTING:
        {
        } break;
        }
    }
}

//=========================================================================
// DeleteConnection
// [12/3/2010]
//=========================================================================
void
CarrierImpl::DeleteConnection(Connection* conn, CarrierDisconnectReason reason)
{
    ConnectionList::iterator iter = AZStd::find(m_connections.begin(), m_connections.end(), conn);
    //AZ_TracePrintf("GM","[%08x] DeleteConnection 0x%08x!\n",this,conn);
    AZ_Assert(iter != m_connections.end(), "We are trying to delete an unknown connection 0x%08x", conn);

    switch (conn->m_state)
    {
    case Carrier::CST_CONNECTED:
    {
        m_handshake->OnDisconnect(conn);
        EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnDisconnect, this, conn, reason);
        EBUS_EVENT(Debug::CarrierDrillerBus, OnDisconnect, this, conn, reason);
    } break;
    case Carrier::CST_CONNECTING:
    {
        m_handshake->OnDisconnect(conn);
        EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnFailedToConnect, this, conn, reason);
        EBUS_EVENT(Debug::CarrierDrillerBus, OnFailedToConnect, this, conn, reason);
    } break;
    case Carrier::CST_DISCONNECTED:
    case Carrier::CST_DISCONNECTING:
    {
    } break;
    }

    m_pendingHandshakes.erase(PendingHandshake(conn));

    delete conn;

    if (iter != m_connections.end())
    {
        m_connections.erase(iter);
    }
}

//=========================================================================
// Carrier
// [9/14/2010]
//=========================================================================
string
CarrierImpl::ConnectionToAddress(ConnectionID id)
{
    string str;
    AZ_Assert(id != InvalidConnectionID, "Invalid connection id!");
    if (id != InvalidConnectionID)
    {
        str = static_cast<Connection*>(id)->m_fullAddress;
    }

    return str;
}

//=========================================================================
// Generate Messages
// [10/4/2010]
//=========================================================================
inline void
CarrierImpl::GenerateSendMessages(const char* data, unsigned int dataSize, ConnectionID target, DataReliability reliability, DataPriority priority, unsigned char channel, AZStd::unique_ptr<CarrierACKCallback> ackCallback)
{
    unsigned short dataSendStep;
    unsigned int numChunks = 1;

    if (dataSize > m_maxMsgDataSizeBytes)
    {
        dataSendStep = static_cast<unsigned short>(m_maxMsgDataSizeBytes);
        // in order to merge the messages we need to send it reliable
        reliability = SEND_RELIABLE;
        
        numChunks += ((dataSize - 1) / m_maxMsgDataSizeBytes);
        // We use half the range only because we sort messages based on that, even that might not be 100% safe is all cases.
        // avoid sending messages > 1 MB in general. If we need to push the limits PLEASE change all sequence numbers
        // to 32 bit (instead of 16) then we can send really big chunks of data.
        unsigned int maxNumChunks = SequenceNumberHalfSpan - 1;
        (void)maxNumChunks;
        AZ_Assert(numChunks <= maxNumChunks, "We can't transfer such big data packets %d bytes, the limit is %d bytes", dataSize, maxNumChunks * m_maxMsgDataSizeBytes);
    }
    else
    {
        dataSendStep = static_cast<unsigned short>(dataSize);
    }

    Connection* conn = static_cast<Connection*>(target);

    {
        AZStd::lock_guard<AZStd::mutex> l(conn->m_toSendLock);
        do
        {
            void* dataBuffer = m_thread->AllocateMessageData(dataSendStep);
            //////////////////////////////////////////////////////////////////////////
            memcpy(dataBuffer, data, dataSendStep);

            MessageData& msg = m_thread->AllocateMessage();
            msg.m_channel = channel;
            msg.m_numChunks = static_cast<SequenceNumber>(numChunks);
            msg.m_dataSize = dataSendStep;
            msg.m_reliability = reliability;
            msg.m_data = dataBuffer;
            msg.m_sequenceNumber = ++conn->m_sendSeqNum[channel];
            
            msg.m_isConnecting = conn->m_state == Carrier::CST_CONNECTING;
            //static long long urSent = 0, rSent=0;
            if (reliability == SEND_RELIABLE)
            {
                //if ((++rSent % 100) == 0)
                //{
                //    AZ_Printf("GridMate", "Reliable-Message ratio sent %d/%d\n", rSent, urSent);
                //}
                ++conn->m_sendReliableSeqNum[channel];

                if(ackCallback)
                {
                    ackCallback->Run();
                }
            }
            else
            {
                //if ((++urSent % 10000) == 0)
                //{
                //    //AZ_Printf("GridMate", "UnReliable Messages sent %d!\n", urSent);
                //    AZ_Printf("GridMate", "Reliable-Message ratio sent %d/%d\n", rSent, urSent);
                //}
                AZ_Assert(dataSize <= dataSendStep, "Cannot split unreliable messages.");
                if (ackCallback)
                {
                    msg.m_ackCallback = AZStd::move(ackCallback);
                }
            }
            
            msg.m_sendReliableSeqNum = conn->m_sendReliableSeqNum[channel];
            conn->m_toSend[priority].push_back(msg);
            conn->m_bytesInQueue += msg.m_dataSize;

            data += dataSendStep;
            dataSize -= dataSendStep;
            numChunks--; // TODO: technically we need the number of chunks only if the first message, the rest should be 1 (to avoid sending multiple times the data)
            if (dataSendStep > dataSize)
            {
                dataSendStep = static_cast<unsigned short>(dataSize);
            }
        } while (dataSize > 0);
    }

    // If we need to get instant response, make sure we wake up the carrier thread.
    if (m_thread->m_threadInstantResponse)
    {
        m_thread->m_driver->StopWaitForData(); // This function MUST be thread safe
    }
}

//=========================================================================
// Send
// [9/14/2010]
//=========================================================================
void
CarrierImpl::SendWithCallback(const char* data, unsigned int dataSize, AZStd::unique_ptr<CarrierACKCallback> ackCallback, ConnectionID target, DataReliability reliability, DataPriority priority, unsigned char channel)
{
    AZ_Assert(dataSize > 0, "You can NOT send empty messages!");
    AZ_Assert(priority > PRIORITY_SYSTEM, "PRIORITY_SYSTEM is reserved for internal use!");
    AZ_Assert(channel < m_maxNumberOfChannels, "Invalid channel index!");
    AZ_Assert(channel != m_systemChannel, "The channel number %d is reserved for system communication!", m_systemChannel);

#if defined(GM_CARRIER_MESSAGE_CRC)
    AZ::u32 dataCrc = AZ::Crc32(data, dataSize);

    m_dbgCrcMessageBuffer.resize(dataSize + sizeof(dataCrc));
    AZ::u8* newData = &m_dbgCrcMessageBuffer[0];
    memcpy(newData, data, dataSize);
    memcpy(newData + dataSize, &dataCrc, sizeof(dataCrc));

    data = reinterpret_cast<const char*>(newData);
    dataSize += sizeof(dataCrc);

#endif // AZ_DEBUG_BUILD

    if (target == AllConnections)
    {
        AZ_Assert(!ackCallback, "ACK Callback not compatible with Broadcast sends!");
        for (AZStd::size_t i = 0; i < m_connections.size(); ++i)
        {
            if (m_connections[i]->m_state == Carrier::CST_CONNECTED)
            {
                GenerateSendMessages(data, dataSize, m_connections[i], reliability, priority, channel);
            }
            else
            {
            }
        }
    }
    else
    {
        AZ_Assert(AZStd::find(m_connections.begin(), m_connections.end(), target) != m_connections.end(), "This connection ID is not valid! Not in the list 0x%08x", target);
        if (static_cast<Connection*>(target)->m_state == Carrier::CST_CONNECTED)
        {
            GenerateSendMessages(data, dataSize, target, reliability, priority, channel, AZStd::move(ackCallback));
        }
        else
        {
        }
    }
}

//=========================================================================
// Receive
// [9/14/2010]
//=========================================================================
Carrier::ReceiveResult
CarrierImpl::Receive(char* data, unsigned int maxDataSize, ConnectionID from, unsigned char channel)
{
    ReceiveResult result;
    result.m_state = ReceiveResult::NO_MESSAGE_TO_RECEIVE;
    result.m_numBytes = 0;
    if (channel >= m_maxNumberOfChannels)
    {
        AZ_Assert(false, "Invalid channel index!");

        ThreadMessage* mtm = aznew ThreadMessage(MTM_ON_ERROR);
        mtm->m_errorCode = CarrierErrorCode::EC_SECURITY;
        mtm->m_error.m_securityError.m_errorCode = SecurityError::EC_CHANNEL_ID_OUT_OF_BOUND;
        mtm->m_connection = static_cast<Connection*>(from);
        m_thread->PushMainThreadMessage(mtm);
    }

    (void)maxDataSize;
    AZ_Assert(AZStd::find(m_connections.begin(), m_connections.end(), from) != m_connections.end(), "This connection ID is not valid! Not in the list 0x%08x", from);
    Connection* fromConn = static_cast<Connection*>(from);
    if (fromConn)
    {
        /// If you are NOT connected, we allow only system channel messages to be received.
        if (fromConn->m_state == Carrier::CST_CONNECTED || channel == m_systemChannel)
        {
            // Carrier thread has already sorted and prepared
            // all the messages in the correct order to receive.
            // Just process them in order!
            AZStd::lock_guard<AZStd::mutex> l(fromConn->m_toReceiveLock);
            if (!fromConn->m_toReceive[channel].empty())
            {
                MessageData& msg = fromConn->m_toReceive[channel].front();

                if (msg.m_reliability == SEND_UNRELIABLE)
                {
                    // we just process the unreliable messages
                    result.m_numBytes = msg.m_dataSize;

                    if (maxDataSize < msg.m_dataSize)
                    {
                        AZ_TracePrintf("GridMate", "%s:%d no buffer space %u < %u",__FILE__, __LINE__, maxDataSize, msg.m_dataSize)
                        result.m_state = ReceiveResult::UNSUFFICIENT_BUFFER_SIZE;
                        result.m_numBytes = msg.m_dataSize;
                        return result;
                    }

                    fromConn->m_toReceive[channel].pop_front();

                    memcpy(data, msg.m_data, msg.m_dataSize);

                    m_thread->FreeMessage(msg);
                }
                else //if(msg.reliability==SEND_RELIABLE)
                {
                    //NOTE: This code has always assumed that all chunks for a given split up message
                    //      have already been added to m_toReceive (i.e. if msg.m_numChunks is 10 there
                    //      will be 10 messages in m_toReceive).

                    // It's assumed that all messages, by this point, have a size less than a maximum
                    // size of CarrierThread::m_maxMsgDataSizeBytes. The check to reject datagrams that
                    // are larger is done at the end of CarrierThread::ProcessIncomingDataGram() using
                    // the same maximum of CarrierThread::m_maxMsgDataSizeBytes.
                    unsigned int requiredBufferSize = (msg.m_numChunks * m_thread->m_maxMsgDataSizeBytes);

                    if (maxDataSize < requiredBufferSize)
                    {
                        AZ_TracePrintf("GridMate", "%s:%d no buffer space %u < %u", __FILE__, __LINE__, maxDataSize, requiredBufferSize)
                        result.m_state = ReceiveResult::UNSUFFICIENT_BUFFER_SIZE;
                        result.m_numBytes = requiredBufferSize;
                        return result;
                    }

                    // check order and receive message
                    if (msg.m_numChunks == 1)
                    {
                        fromConn->m_toReceive[channel].pop_front();
                        memcpy(data, msg.m_data, msg.m_dataSize);
                        result.m_numBytes = msg.m_dataSize;

                        m_thread->FreeMessage(msg);
                    }
                    else
                    {
                        SequenceNumber numChunks = msg.m_numChunks;
                        for (SequenceNumber iChunk = 0; iChunk < numChunks; ++iChunk)
                        {
                            MessageData& chunkMsg = fromConn->m_toReceive[channel].front();

                            fromConn->m_toReceive[channel].pop_front();
                            memcpy(data, chunkMsg.m_data, chunkMsg.m_dataSize);
                            result.m_numBytes += chunkMsg.m_dataSize;
                            data += chunkMsg.m_dataSize;

                            m_thread->FreeMessage(chunkMsg);
                        }

#if defined(GM_CARRIER_MESSAGE_CRC)
                        data -= received; //rewind data pointer
#endif
                    }
                }
                result.m_state = ReceiveResult::RECEIVED;
            }
        }
    }

#if defined(GM_CARRIER_MESSAGE_CRC)
    if (received > 0)
    {
        unsigned int realDataSize = received - sizeof(AZ::u32);

        AZ::u32 dataCrc = AZ::Crc32(data, realDataSize);

        AZ::u32 sendCrc = *reinterpret_cast<AZ::u32*>(data + realDataSize);

        AZ_Assert(dataCrc == sendCrc, "Carrier Message Crc check failed, this must be memory corruption or a Carrier bug 0x%08x vs 0x%08x", dataCrc, sendCrc);

        received = realDataSize;
    }
#endif // AZ_DEBUG_BUILD

    return result;
}

//=========================================================================
// Update
// [9/14/2010]
//=========================================================================
void
CarrierImpl::Update()
{
    m_currentTime = AZStd::chrono::system_clock::now();
    m_thread->m_lastMainThreadUpdate = AZStd::chrono::milliseconds(m_currentTime.time_since_epoch()).count();

    //////////////////////////////////////////////////////////////////////////
    // Process messages from the carrier thread
    {
        ThreadMessage* msg;
        while ((msg = m_thread->PopMainThreadMessage()) != NULL)
        {
            switch (msg->m_code)
            {
            case MTM_NEW_CONNECTION:
            {
                Connection* conn = NULL;
                // check if we don't have it in the list.
                for (ConnectionList::iterator i = m_connections.begin(); i != m_connections.end(); ++i)
                {
                    if ((*i)->m_fullAddress == msg->m_newConnectionAddress)
                    {
                        conn = *i;
                        break;
                    }
                }
                if (conn)
                {
                    // we already have such connection
                    ThreadConnection* threadConn = conn->m_threadConn.load(AZStd::memory_order_acquire);
                    AZ_Assert(threadConn == NULL || threadConn == msg->m_threadConnection, "This main connection 0x%08x (%s) already have bound thread connection 0x%08x->0x%08x!", conn, conn->m_fullAddress.c_str(), threadConn, threadConn->m_mainConnection);
                    if (threadConn == NULL)
                    {
                        // request a bind we have not already
                        ThreadMessage* ctm = aznew ThreadMessage(CTM_CONNECT);
                        ctm->m_connection = conn;
                        ctm->m_threadConnection = msg->m_threadConnection;
                        m_thread->PushCarrierThreadMessage(ctm);
                    }
                }
                else if (m_handshake->OnNewConnection(msg->m_newConnectionAddress))
                {
                    conn = aznew Connection(m_thread, msg->m_newConnectionAddress);
                    m_connections.push_back(conn);
                    //AZ_TracePrintf("GM","New Connection() %08x\n",conn);

                    EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnIncomingConnection, this, conn);
                    EBUS_EVENT(Debug::CarrierDrillerBus, OnIncomingConnection, this, conn);

                    ThreadMessage* ctm = aznew ThreadMessage(CTM_CONNECT);
                    ctm->m_connection = conn;
                    ctm->m_threadConnection = msg->m_threadConnection;
                    m_thread->PushCarrierThreadMessage(ctm);
                }
                else
                {
                    // we will not even make a connection
                    ThreadMessage* ctm = aznew ThreadMessage(CTM_DELETE_CONNECTION);
                    ctm->m_threadConnection = msg->m_threadConnection;
                    ctm->m_connection = NULL;
                    ctm->m_disconnectReason = CarrierDisconnectReason::DISCONNECT_HANDSHAKE_REJECTED;
                    m_thread->PushCarrierThreadMessage(ctm);
                }
            } break;
            case MTM_DISCONNECT:
            {
                AZ_Assert(msg->m_connection != NULL, "You must provide a valid connection pointer!");
                DisconnectRequest(msg->m_connection, msg->m_disconnectReason);
            } break;
            case MTM_DISCONNECT_TIMEOUT:
            {
                AZ_Assert(msg->m_connection != NULL, "You must provide a valid connection pointer!");
                if (msg->m_connection->m_state == Carrier::CST_DISCONNECTING)
                {
                    // unbind from the thread connection and inform carrier thread to delete it.
                    ThreadConnection* threadConn = msg->m_connection->m_threadConn.exchange(NULL);
                    msg->m_connection->m_state = Carrier::CST_DISCONNECTED;
                    ThreadMessage* ctm = aznew ThreadMessage(CTM_DELETE_CONNECTION);
                    ctm->m_connection = msg->m_connection;
                    ctm->m_threadConnection = threadConn;
                    ctm->m_disconnectReason = msg->m_disconnectReason;
                    m_thread->PushCarrierThreadMessage(ctm);
                    EBUS_EVENT(Debug::CarrierDrillerBus, OnConnectionStateChanged, this, msg->m_connection, msg->m_connection->m_state);
                }
            } break;
            case MTM_DELETE_CONNECTION:
            {
                AZ_Assert(msg->m_connection != NULL, "You must provide a valid connection pointer!");
                DeleteConnection(msg->m_connection, msg->m_disconnectReason);
            } break;
            case MTM_ON_ERROR:
            {
                if (msg->m_connection)
                {
                    AZ_TracePrintf("GridMate", "Carrier::Connection %s had an error %d with error code %d\n",
                        msg->m_connection->m_fullAddress.c_str(),
                        msg->m_errorCode,
                        (msg->m_errorCode == CarrierErrorCode::EC_DRIVER) ? static_cast<int>(msg->m_error.m_driverError.m_errorCode) : static_cast<int>(msg->m_error.m_securityError.m_errorCode));
                }
                else
                {
                    AZ_TracePrintf("GridMate", "Carrier::Error %d with error code %d\n",
                        msg->m_errorCode,
                        (msg->m_errorCode == CarrierErrorCode::EC_DRIVER) ? static_cast<int>(msg->m_error.m_driverError.m_errorCode) : static_cast<int>(msg->m_error.m_securityError.m_errorCode));
                }

                if (msg->m_errorCode == CarrierErrorCode::EC_DRIVER)
                {
                    EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnDriverError, this, msg->m_connection, msg->m_error.m_driverError);
                    EBUS_EVENT(Debug::CarrierDrillerBus, OnDriverError, this, msg->m_connection, msg->m_error.m_driverError);

                    if (msg->m_connection)
                    {
                        //TODO: Disconnect only when DriverError is critical
                        DisconnectRequest(msg->m_connection, CarrierDisconnectReason::DISCONNECT_DRIVER_ERROR);
                    }
                }
                else
                {
                    EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnSecurityError, this, msg->m_connection, msg->m_error.m_securityError);
                    EBUS_EVENT(Debug::CarrierDrillerBus, OnSecurityError, this, msg->m_connection, msg->m_error.m_securityError);
                }
            } break;
            case MTM_RATE_UPDATE:
            {
                EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnRateChange, this, msg->m_connection, msg->m_newRateBytesPerSec);
            } break;
            case MTM_ACK_NOTIFY:
            {
                for (auto& cb : msg->m_ackCallbacks)
                {
                    if (cb)
                    {
                        cb->Run();
                    }
                }
            }break;
            default:
                AZ_Assert(false, "Unknown message type! %d", msg->m_code);
            }

            delete msg;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    // Clock Sync
    if (m_clockSyncInterval != InvalidSyncInterval)
    {
        if (static_cast<unsigned int>(AZStd::chrono::milliseconds(m_currentTime - m_lastSyncTimeStamp).count()) >= m_clockSyncInterval)
        {
            SendSyncTime();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Process any system messages we have received
    {
        char* systemMessageBuffer = reinterpret_cast<char*>(m_thread->AllocateMessageData(m_thread->m_maxDataGramSizeBytes));

        for (auto& h : m_pendingHandshakes)
        {
            if (m_currentTime >= h.m_retryTime)
            {
                SendSystemMessage(SM_CONNECT_REQUEST, h.m_payload, h.m_connection, SEND_UNRELIABLE);
                AZ::u64 nextRetryTimeOut = AZStd::min<AZ::u64>(static_cast<AZ::u64>(m_connectionRetryIntervalMax), m_connectionRetryIntervalBase * (1ull << h.m_numRetries));
                h.m_retryTime = m_currentTime + AZStd::chrono::milliseconds(nextRetryTimeOut);
                ++h.m_numRetries;
            }
        }

        for (AZStd::size_t iConnection = 0; iConnection < m_connections.size(); ++iConnection)
        {
            Connection* conn = m_connections[iConnection];
            while (true)
            {
                Carrier::ReceiveResult result = Receive(systemMessageBuffer, m_thread->m_maxDataGramSizeBytes, conn, m_systemChannel);
                if (result.m_state == ReceiveResult::RECEIVED)
                {
                    SystemMessageId msgId;
                    ReadBuffer rb(kCarrierEndian);
                    if (!ReceiveSystemMessage(systemMessageBuffer, result.m_numBytes, msgId, rb))
                    {
                        continue;
                    }

                    switch (msgId)
                    {
                    case SM_CONNECT_REQUEST:
                    {
                        WriteBufferStatic<> wb(kCarrierEndian);
                        switch (conn->m_state)
                        {
                        case Carrier::CST_CONNECTING:
                        {
                            HandshakeErrorCode requestError = m_handshake->OnReceiveRequest(conn, rb, wb);
                            if (requestError == HandshakeErrorCode::OK)
                            {
                                conn->m_state = Carrier::CST_CONNECTED;
                                EBUS_EVENT(Debug::CarrierDrillerBus, OnConnectionStateChanged, this, conn, conn->m_state);

                                SendSyncTime();         // send time first if we have the clock.
                                SendSystemMessage(SM_CONNECT_ACK, wb, conn, SEND_RELIABLE);

                                EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnConnectionEstablished, this, conn);
                                EBUS_EVENT(Debug::CarrierDrillerBus, OnConnectionEstablished, this, conn);

                                ThreadMessage* ctm = aznew ThreadMessage(CTM_HANDSHAKE_COMPLETE);
                                ctm->m_connection = conn;
                                m_thread->PushCarrierThreadMessage(ctm);
                            }
                            else if (requestError == HandshakeErrorCode::PENDING)
                            {

                            }
                            else if (requestError == HandshakeErrorCode::VERSION_MISMATCH)
                            {
                                DisconnectRequest(conn, CarrierDisconnectReason::DISCONNECT_VERSION_MISMATCH);
                            }
                            else
                            {
                                DisconnectRequest(conn, CarrierDisconnectReason::DISCONNECT_HANDSHAKE_REJECTED);
                            }
                        } break;
                        case Carrier::CST_CONNECTED:
                        {
                            if (!m_handshake->OnConfirmRequest(conn, rb))
                            {
                                // Some how the other side is trying to initiate a different connection while
                                // we locally still maintain the first one. So close it.
                                DisconnectRequest(conn, CarrierDisconnectReason::DISCONNECT_WAS_ALREADY_CONNECTED);
                            }
                        } break;
                        case Carrier::CST_DISCONNECTED:
                        case Carrier::CST_DISCONNECTING:
                        {
                        } break;
                        }
                    } break;
                    case SM_CONNECT_ACK:
                    {
                        switch (conn->m_state)
                        {
                        case Carrier::CST_CONNECTING:
                        {
                            if (m_handshake->OnReceiveAck(conn, rb))
                            {
                                m_pendingHandshakes.erase(PendingHandshake(conn)); // Connected -> no need to retry handshake anymore

                                conn->m_state = Carrier::CST_CONNECTED;
                                EBUS_EVENT(Debug::CarrierDrillerBus, OnConnectionStateChanged, this, conn, conn->m_state);
                                EBUS_EVENT_ID(m_gridMate, CarrierEventBus, OnConnectionEstablished, this, conn);
                                EBUS_EVENT(Debug::CarrierDrillerBus, OnConnectionEstablished, this, conn);

                                ThreadMessage* ctm = aznew ThreadMessage(CTM_HANDSHAKE_COMPLETE);
                                ctm->m_connection = conn;
                                m_thread->PushCarrierThreadMessage(ctm);
                            }
                            else
                            {
                                DisconnectRequest(conn, CarrierDisconnectReason::DISCONNECT_HANDSHAKE_REJECTED);
                            }
                        } break;
                        case Carrier::CST_CONNECTED:
                        {
                            if (!m_handshake->OnConfirmAck(conn, rb))
                            {
                                // This should be really impossible to be a bad case... since we ACK on requested and we should
                                // catch earlier the async.
                                DisconnectRequest(conn, CarrierDisconnectReason::DISCONNECT_HANDSHAKE_REJECTED);
                            }
                        } break;
                        case Carrier::CST_DISCONNECTED:
                        case Carrier::CST_DISCONNECTING:
                        {
                        } break;
                        }
                    } break;
                    case SM_DISCONNECT:
                    {
                        CarrierDisconnectReason reason = CarrierDisconnectReason::DISCONNECT_BAD_PACKETS; // initializing to bad packets in case Read will fail
                        rb.Read(reason);

                        //AZ_TracePrintf("GridMate","Disconnect %s\n",conn->fullAddress.c_str());
                        // Called to execute the necessary callbacks, otherwise the connection will be torn down instantly.
                        DisconnectRequest(conn, reason);

                        //////////////////////////////////////////////////////////////////////////
                        // Delete connection
                        conn->m_state = Carrier::CST_DISCONNECTED;
                        // unbind from the thread connection and inform carrier thread to delete it.
                        ThreadConnection* threadConn = conn->m_threadConn.exchange(NULL);
                        ThreadMessage* ctm = aznew ThreadMessage(CTM_DELETE_CONNECTION);
                        ctm->m_connection = conn;
                        ctm->m_threadConnection = threadConn;
                        ctm->m_disconnectReason = reason;
                        m_thread->PushCarrierThreadMessage(ctm);
                        EBUS_EVENT(Debug::CarrierDrillerBus, OnConnectionStateChanged, this, conn, conn->m_state);
                        //////////////////////////////////////////////////////////////////////////
                    } break;
                    case SM_CLOCK_SYNC:
                    {
                        AZ_Warning("GridMate", m_clockSyncInterval == InvalidSyncInterval, "You have received clock sync, while send clock sync too! Only one peer can send clock sync messages!");
                        if (m_clockSyncInterval == InvalidSyncInterval)
                        {
                            AZ::u32 time;
                            rb.Read(time);
                            OnReceivedTime(conn, time);
                        }
                    } break;
                    default:
                        break;
                    }
                }
                else
                {
                    AZ_Assert(result.m_state != ReceiveResult::UNSUFFICIENT_BUFFER_SIZE, "System messages should not be bigger than %d\n", m_thread->m_maxDataGramSizeBytes);
                    break; /// no more system messages
                }
            }
        }

        m_thread->FreeMessageData(systemMessageBuffer, m_thread->m_maxDataGramSizeBytes);
    }
    //////////////////////////////////////////////////////////////////////////
}

//=========================================================================
// QueryStatistics
// [11/10/2010]
//=========================================================================
Carrier::ConnectionStates
CarrierImpl::QueryStatistics(ConnectionID id, TrafficControl::Statistics* lastSecond, TrafficControl::Statistics* lifetime,
    TrafficControl::Statistics* effectiveLastSecond, TrafficControl::Statistics* effectiveLifetime,
    FlowInformation* flowInformation)
{
    AZ_Assert(id != InvalidConnectionID && id != AllConnections, "You need to specify only one valid connection!");
    if (id == InvalidConnectionID)
    {
        return Carrier::CST_DISCONNECTED;
    }

    Connection* conn = static_cast<Connection*>(id);
    bool validConnection = AZStd::find(m_connections.begin(), m_connections.end(), conn) != m_connections.end();
    AZ_Assert(validConnection, "This connection ID is not valid! Not in the list 0x%08x", conn);
    if (!validConnection)
    {
        return Carrier::CST_DISCONNECTED;
    }
    {
        AZStd::lock_guard<AZStd::mutex> statsLock(conn->m_statsLock);
        if (lastSecond)
        {
            *lastSecond = conn->m_statsLastSecond;
        }
        if (lifetime)
        {
            *lifetime = conn->m_statsLifetime;
        }
        if (effectiveLastSecond)
        {
            *effectiveLastSecond = conn->m_statsEffectiveLastSecond;
        }
        if (effectiveLifetime)
        {
            *effectiveLifetime = conn->m_statsEffectiveLifetime;
        }
        if (flowInformation)
        {
            {
                AZStd::lock_guard<AZStd::mutex> sendLock(conn->m_toSendLock);
                flowInformation->m_numToSendMessages = 0;
                for (int priority = 0; priority < Carrier::PRIORITY_MAX; ++priority)
                {
                    flowInformation->m_numToSendMessages += conn->m_toSend[priority].size();
                }
            }
            {
                AZStd::lock_guard<AZStd::mutex> receiveLock(conn->m_toReceiveLock);
                flowInformation->m_numToReceiveMessages = 0;
                for (int channel = 0; channel < m_maxNumberOfChannels; ++channel)
                {
                    flowInformation->m_numToReceiveMessages += conn->m_toReceive[channel].size();
                }
            }

            flowInformation->m_dataInTransfer = conn->m_congestionState.m_dataInTransfer;
            flowInformation->m_congestionWindow = conn->m_congestionState.m_congestionWindow;
        }
    }
    return conn->m_state;
}

//inline const char* ToString(bool b)
//{
//  return b ? "yes" : "no";
//}

//=========================================================================
// DebugStatusReport
// [1/4/2011]
//=========================================================================
void
CarrierImpl::DebugStatusReport(ConnectionID id, unsigned char channel)
{
    (void)id;
    (void)channel;
    /*  AZ_Assert(id!=InvalidConnectionID && id!=AllConnections,"You need to specify only one valid connection!");
    Connection* conn = static_cast<Connection*>(id);

    TimeStamp now = AZStd::chrono::system_clock::now();
    AZStd::chrono::milliseconds timeElapsed;

    AZ_TracePrintf("GridMate","=========================================================\n");
    AZ_TracePrintf("GridMate","Connection %d status for channel %d\n",id,channel);
    AZ_TracePrintf("GridMate","IsReady: %s isDisconn: %s\n",ToString(conn->isReady),ToString(conn->isDisconnecting));
    AZ_TracePrintf("GridMate","Send: seqNum: %d reliableSeqNum: %d\n",conn->m_sendSeqNum[channel],conn->m_sendReliableSeqNum[channel]);
    AZ_TracePrintf("GridMate","Is data to send %s (sorted on send priority):\n",ToString(HasDataToSend(conn)));
    for(AZStd::size_t i = 0; i < PRIORITY_MAX; ++i)
    {
    for(MessageDataListType::const_iterator iter = conn->toSend[i].begin(); iter != conn->toSend[i].end();++iter)
    {
    AZ_TracePrintf("GridMate","  Msg mode: %d ch: %d chunks: %d seqNum: %d relSeqNum: %d dataSize: %d\n",iter->reliability,iter->channel,iter->m_numChunks,iter->sequenceNumber,iter->reliableSeqNum,iter->dataSize);
    }
    }
    AZ_TracePrintf("GridMate","Receive: seqNum: %d reliableSeqNum: %d\n",conn->m_receivedSeqNum[channel],conn->receivedReliableSeqNum[channel]);
    for(MessageDataListType::const_iterator iter = conn->m_toReceive[channel].begin(); iter != conn->m_toReceive[channel].end();++iter)
    {
    AZ_TracePrintf("GridMate","  Msg mode: %d chunks: %d seqNum: %d relSeqNum: %d dataSize: %d\n",iter->reliability,iter->m_numChunks,iter->sequenceNumber,iter->reliableSeqNum,iter->dataSize);
    }
    AZ_TracePrintf("GridMate","Datagrams - current sequence number: %d\n",conn->dataGramSeqNum);
    AZ_TracePrintf("GridMate","Send dgrams %d:\n",conn->sendDataGrams.size());

    for(DatagramDataListType::const_iterator iter = conn->sendDataGrams.begin(); iter != conn->sendDataGrams.end();++iter)
    {
    timeElapsed = now - iter->flowControl.time;
    AZ_TracePrintf("GridMate","  Dgram %d payloadSize: %d resendSize: %d time %d ms ago\n",iter->sequenceNumber,iter->payloadDataSize,iter->m_resendDataSize,timeElapsed.count());
    for(AZStd::size_t i = 0; i < PRIORITY_MAX; ++i)
    {
    for(MessageDataListType::const_iterator msgIter = iter->toResend[i].begin(); msgIter != iter->toResend[i].end();++msgIter)
    {
    AZ_TracePrintf("GridMate","     ResendMsg ch: %d chunks: %d seqNum: %d relSeqNum: %d dataSize: %d\n",msgIter->channel,msgIter->m_numChunks,msgIter->sequenceNumber,msgIter->reliableSeqNum,msgIter->dataSize);
    }
    }
    }
    AZ_TracePrintf("GridMate","Received dgrams %d:\n",conn->receivedDataGramsIDs.size());
    for(Connection::DataGramIDHistory::const_iterator iter = conn->receivedDataGramsIDs.begin(); iter != conn->receivedDataGramsIDs.end();++iter)
    {
    AZ_TracePrintf("GridMate","  Dgram %d\n",*iter);
    }
    timeElapsed = now - conn->lastReceivedDatagramTime;
    AZ_TracePrintf("GridMate","Time since last dgram %d ms\n",timeElapsed.count());
    AZ_TracePrintf("GridMate","Time since connection start %d s\n",AZStd::chrono::seconds(now-conn->m_createTime).count());

    AZ_TracePrintf("GridMate","=========================================================\n");*/
}

//=========================================================================
// SendSystemMessage
// [11/8/2010]
//=========================================================================
void
CarrierImpl::SendSystemMessage(SystemMessageId id, WriteBuffer& wb, ConnectionID target, DataReliability reliability, bool isForValidConnectionsOnly)
{
    unsigned char byteId = static_cast<unsigned char>(id);
    /*
     * On the receiving end, carrier reads this message id at the end of the buffer and
     * assumes it's last byte in the messages, thus, if we have been bit packing, we have to make sure we are byte alignment as we message id.
     */
    wb.WriteWithByteAlignment(byteId);

#if defined(GM_CARRIER_MESSAGE_CRC)
    {
        AZ::u32 dataCrc = AZ::Crc32(wb.Get(), wb.Size());
        wb.WriteRaw(&dataCrc, sizeof(dataCrc));
    }
#endif // AZ_DEBUG_BUILD

    if (target == AllConnections)
    {
        for (AZStd::size_t i = 0; i < m_connections.size(); ++i)
        {
            if (!isForValidConnectionsOnly || m_connections[i]->m_state == Carrier::CST_CONNECTED)
            {
                GenerateSendMessages(wb.Get(), static_cast<unsigned int>(wb.Size()), m_connections[i], reliability, PRIORITY_SYSTEM, m_systemChannel);
            }
        }
    }
    else
    {
        if (!isForValidConnectionsOnly || static_cast<Connection*>(target)->m_state == Carrier::CST_CONNECTED)
        {
            GenerateSendMessages(wb.Get(), static_cast<unsigned int>(wb.Size()), target, reliability, PRIORITY_SYSTEM, m_systemChannel);
        }
    }
}

//=========================================================================
// ReceiveSystemMessage
// [11/8/2010]
//=========================================================================
bool CarrierImpl::ReceiveSystemMessage(const char* srcData, unsigned int srcDataSize, SystemMessageId& msgId, ReadBuffer& rb)
{
    AZ_Assert(srcDataSize >= 1, "System message size is at least 1 byte (message msgId)!");
    if (srcDataSize > 0)
    {
        msgId = static_cast<SystemMessageId>(*(srcData + srcDataSize - 1));
        rb = ReadBuffer(kCarrierEndian, srcData, srcDataSize - 1);
        return true;
    }
    return false;
}

//=========================================================================
// StartClockSync
// [12/22/2010]
//=========================================================================
void
CarrierImpl::StartClockSync(unsigned int syncInterval, bool isReset)
{
    if (isReset)
    {
        m_lastReceivedTime = 0;
        m_lastUsedTime = 0;
        m_lastReceivedTimeStamp = AZStd::chrono::system_clock::now();
    }
    m_lastSyncTimeStamp = TimeStamp(TimeStamp::duration::zero());
    m_clockSyncInterval = syncInterval;
}

//=========================================================================
// StopClockSync
// [12/22/2010]
//=========================================================================
void
CarrierImpl::StopClockSync()
{
    m_clockSyncInterval = InvalidSyncInterval;
}

//=========================================================================
// SendSyncTime
// [2/4/2011]
//=========================================================================
void
CarrierImpl::SendSyncTime()
{
    if (m_clockSyncInterval != InvalidSyncInterval)
    {
        AZ::u32 time = GetTime();
        WriteBufferStatic<> writeBuffer(kCarrierEndian);
        writeBuffer.Write(time);
        SendSystemMessage(SM_CLOCK_SYNC, writeBuffer, AllConnections, SEND_UNRELIABLE, true /* only for established connections */);
        m_lastSyncTimeStamp = m_currentTime;
    }
}

//=========================================================================
// OnReceivedTime
// [12/22/2010]
//=========================================================================
void
CarrierImpl::OnReceivedTime(ConnectionID fromId, AZ::u32 time)
{
    TrafficControl::Statistics lastSecond;
    // take the simple stats for the last 1 sec
    QueryStatistics(fromId, &lastSecond);
    // adjust received time with half the RTT
    //AZ_TracePrintf("GridMate","[%p] Received time %d rtt: %.2f\n",this,time,stats.traffic.rtt);
    m_lastReceivedTime = time + static_cast<AZ::u32>(lastSecond.m_rtt * 0.5f);
    m_lastReceivedTimeStamp = m_currentTime;
}

//=========================================================================
// GetTime
// [12/22/2010]
//=========================================================================
AZ::u32
CarrierImpl::GetTime()
{
    AZ::u32 currentTime = m_lastReceivedTime;
    AZ::u32 correction = static_cast<AZ::u32>(AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now() - m_lastReceivedTimeStamp).count());
    currentTime += correction;
    if (m_lastUsedTime < currentTime)
    {
        m_lastUsedTime = currentTime;
    }
    else
    {
        currentTime = m_lastUsedTime; // Never allow to go back in time
    }
    return currentTime;
}

//=========================================================================
// DebugDeleteConnection
// [5/10/2011]
//=========================================================================
void
CarrierImpl::DebugDeleteConnection(ConnectionID id)
{
    if (id == InvalidConnectionID && m_thread != NULL)
    {
        return;
    }

    Connection* conn = static_cast<Connection*>(id);
    CarrierDisconnectReason reason = CarrierDisconnectReason::DISCONNECT_DEBUG_DELETE_CONNECTION;
    AZ_TracePrintf("GridMate", "DebugDeleteConnection %s - SHOULD BE CALLED IN DEBUG TESTS ONLY!\n", conn->m_fullAddress.c_str());
    // Called to execute the necessary callbacks, otherwise the connection will be torn down instantly.
    DisconnectRequest(conn, reason);

    //////////////////////////////////////////////////////////////////////////
    // Delete connection
    conn->m_state = Carrier::CST_DISCONNECTED;
    // unbind from the thread connection and inform carrier thread to delete it.
    ThreadConnection* threadConn = conn->m_threadConn.exchange(NULL);
    ThreadMessage* ctm = aznew ThreadMessage(CTM_DELETE_CONNECTION);
    ctm->m_connection = conn;
    ctm->m_threadConnection = threadConn;
    ctm->m_disconnectReason = reason;
    m_thread->PushCarrierThreadMessage(ctm);
    EBUS_EVENT(Debug::CarrierDrillerBus, OnConnectionStateChanged, this, conn, conn->m_state);
    //////////////////////////////////////////////////////////////////////////
}

//=========================================================================
// DebugEnableDisconnectDetection
// [9/14/2012]
//=========================================================================
void
CarrierImpl::DebugEnableDisconnectDetection(bool isEnabled)
{
    // This is not thread safe, but this should not be a problem. If we need to be
    // we can add a new CTM_XXX (Carrier Thread Message)
    m_thread->m_enableDisconnectDetection = isEnabled;
}

//=========================================================================
// DebugIsEnableDisconnectDetection
// [9/14/2012]
//=========================================================================
bool
CarrierImpl::DebugIsEnableDisconnectDetection() const
{
    // This is not thread safe, but this should not be a problem. If we need to be
    // we can add a new CTM_XXX (Carrier Thread Message)
    return m_thread->m_enableDisconnectDetection;
}

//=========================================================================
// CreateDefaultCarrier
// [10/28/2010]
//=========================================================================
Carrier*
DefaultCarrier::Create(const CarrierDesc& desc, IGridMate* gridMate)
{
    return aznew CarrierImpl(desc, gridMate);
}

//=========================================================================
// ReasonToString
// [4/11/2011]
//=========================================================================
string
CarrierEventsBase::ReasonToString(CarrierDisconnectReason reason)
{
    const char* reasonStr = 0;
    switch (reason)
    {
    case CarrierDisconnectReason::DISCONNECT_USER_REQUESTED:
        reasonStr = "User requested";
        break;
    case CarrierDisconnectReason::DISCONNECT_BAD_CONNECTION:
        reasonStr = "Bad connection";
        break;
    case CarrierDisconnectReason::DISCONNECT_BAD_PACKETS:
        reasonStr = "Bad data packets";
        break;
    case CarrierDisconnectReason::DISCONNECT_DRIVER_ERROR:
        reasonStr = "Driver error";
        break;
    case CarrierDisconnectReason::DISCONNECT_HANDSHAKE_REJECTED:
        reasonStr = "Handshake rejected";
        break;
    case CarrierDisconnectReason::DISCONNECT_HANDSHAKE_TIMEOUT:
        reasonStr = "Handshake timeout";
        break;
    case CarrierDisconnectReason::DISCONNECT_WAS_ALREADY_CONNECTED:
        reasonStr = "Already connected to that user";
        break;
    case CarrierDisconnectReason::DISCONNECT_SHUTTING_DOWN:
        reasonStr = "Carrier is shutting down";
        break;
    case CarrierDisconnectReason::DISCONNECT_DEBUG_DELETE_CONNECTION:
        reasonStr = "Debug delete connection - DO NOT USE THIS FUNCTION!";
        break;
    case CarrierDisconnectReason::DISCONNECT_VERSION_MISMATCH:
        reasonStr = "Version mismatch when establishing a connection.";
        break;
    default:
        reasonStr = "Unknown reason";
    }

    return string(reasonStr);
}

#endif // #ifndef AZ_UNITY_BUILD
