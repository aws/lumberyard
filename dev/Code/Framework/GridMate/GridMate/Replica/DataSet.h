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
#ifndef GM_DATASET_H
#define GM_DATASET_H

#include <AzCore/std/function/function_fwd.h>

#include <GridMate/Containers/vector.h>

#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Replica/Throttles.h>

#include <GridMate/Serialize/Buffer.h>


namespace AzFramework
{
    class NetworkContext;
}

namespace GridMate
{
    class ReplicaChunkDescriptor;
    class ReplicaChunkBase;
    class ReplicaMarshalTaskBase;

    /**
    * DataSetBase
    * Base type for all replica datasets
    */
    class DataSetBase
    {
        friend ReplicaChunkDescriptor;
        friend ReplicaChunkBase;
        friend AzFramework::NetworkContext;

    public:
        void SetMaxIdleTime(float dt) { m_maxIdleTicks = dt; }
        float GetMaxIdleTime() const { return m_maxIdleTicks; }
        bool CanSet() const;
        bool IsDefaultValue() const { return m_isDefaultValue; }
        void MarkNonDefaultValue() { m_isDefaultValue = false; }

    protected:
        explicit DataSetBase(const char* debugName);
        virtual ~DataSetBase() {}

        virtual PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags) = 0;
        virtual void Unmarshal(UnmarshalContext& mc) = 0;
        virtual void ResetDirty() = 0;
        virtual void DispatchChangedEvent(const TimeContext& tc) { (void) tc; }
        virtual void SetDirty();

        ReadBuffer GetMarshalData();

        float m_maxIdleTicks;
        WriteBufferDynamic m_streamCache;
        ReplicaChunkBase* m_replicaChunk; // raw pointer, assuming datasets do not exists without replica chunk
        unsigned int m_lastUpdateTime;

        bool m_isDefaultValue;
    };

    // This shim is here to temporarily allow support for Pointer type unmarshalling
    // (current approach of detecting differences doesn't readily allow for this behavior)
    // This is mainly to support ScriptProperties.
    class MarshalerShim
    {
    public:
        template<class MarshalerType, class DataType>
        static bool Unmarshal(UnmarshalContext& mc, MarshalerType& marshaler, DataType& sourceValue, AZStd::false_type /*isPointerType*/)
        {
            DataType value;

            // Expects a return of whether or not the value was actually read.
            if (mc.m_iBuf->Read(value, marshaler))
            {
                if (!(value == sourceValue))
                {
                    sourceValue = value;
                    return true;
                }
            }

            return false;
        }

        template<class MarshalerType, class DataType>
        static bool Unmarshal(UnmarshalContext& mc, MarshalerType& marshaler, DataType& sourceValue, AZStd::true_type /*isPointerType*/)
        {
            // Expects a return of whether or not the value was changed.
            return marshaler.UnmarshalToPointer(sourceValue,(*mc.m_iBuf));
        }
    };

    /**
        Declares a networked DataSet of type DataType. Optionally pass in a marshaler that
        can write the data to a stream. Otherwise the DataSet will expect to find a
        ForwardMarshaler specialized on type Type. Optionally pass in a throttler
        that can decide when the data has changed enough to send to the downstream proxies.
    **/
    template<typename DataType, typename MarshalerType = Marshaler<DataType>, typename ThrottlerType = BasicThrottle<DataType>>
    class DataSet
        : public DataSetBase
    {
    public:
        template<class C, void (C::* FuncPtr)(const DataType&, const TimeContext&)>
        class BindInterface;

        /**
            Constructs a DataSet.
        **/
        DataSet(const char* debugName, const DataType& value = DataType(), const MarshalerType& marshaler = MarshalerType(), const ThrottlerType& throttler = ThrottlerType())
            : DataSetBase(debugName)
            , m_value(value)
            , m_throttler(throttler)
            , m_marshaler(marshaler)
            , m_idleTicks(-1.f)
        {
            m_throttler.UpdateBaseline(value);
        }

        /**
            Modify the DataSet. Call this on the Master node to change the data,
            which will be propagated to all proxies.
        **/
        void Set(const DataType& v)
        {
            if (CanSet())
            {
                m_value = v;
                m_isDefaultValue = false;
                SetDirty();
            }
        }

        /**
            Modify the DataSet. Call this on the Master node to change the data,
            which will be propagated to all proxies.
        **/
        void Set(DataType&& v)
        {
            if (CanSet())
            {
                m_value = v;
                m_isDefaultValue = false;
                SetDirty();
            }
        }

        /**
            Modify the DataSet. Call this on the Master node to change the data,
            which will be propagated to all proxies.
        **/
        template <class ... Args>
        void SetEmplace(Args&& ... args)
        {
            if (CanSet())
            {
                m_value = DataType(AZStd::forward<Args>(args) ...);
                m_isDefaultValue = false;
                SetDirty();
            }
        }

        /**
            Modify the DataSet directly without copying it. Call this on the Master node,
            passing in a function object that takes the value by reference, optionally
            modifies the data, and returns true if the data was changed.
        **/
        template<typename FuncPtr>
        bool Modify(FuncPtr func)
        {
            static_assert(AZStd::is_same<decltype(func(m_value)), bool>::value, "Function object must return dirty status");

            bool dirty = false;
            if (CanSet())
            {
                dirty = func(m_value);
                if (dirty)
                {
                    m_isDefaultValue = false;
                    SetDirty();
                }
            }
            return dirty;
        }

        /**
            Returns the current value of the DataSet.
        **/
        const DataType& Get() const { return m_value; }

        /**
            Returns the last updated network time of the DataSet.
        **/
        unsigned int GetLastUpdateTime() const { return m_lastUpdateTime; }

        /**
            Returns the marshaler instance.
        **/
        MarshalerType& GetMarshaler() { return m_marshaler; }

        /**
            Returns the throttler instance.
        **/
        ThrottlerType& GetThrottler() { return m_throttler; }

        //@{
        /**
            Returns equality for values of the same type
        **/
        bool operator==(const DataType& other) const { return m_value == other; }
        bool operator==(const DataSet& other) const { return m_value == other.m_value; }

        bool operator!=(const DataType& other) const { return !(m_value == other); }
        bool operator!=(const DataSet& other) const { return !(m_value == other.m_value); }
        //@}

    protected:
        DataSet(const DataSet& rhs) AZ_DELETE_METHOD;
        DataSet& operator=(const DataSet&) AZ_DELETE_METHOD;

        void SetDirty() override
        {
            if (!IsWithinToleranceThreshold())
            {
                DataSetBase::SetDirty();
            }
        }

        PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags) override
        {
            PrepareDataResult pdr(false, false, false, false);

            bool isDirty = false;
            if (!IsWithinToleranceThreshold())
            {
                m_isDefaultValue = false;

                isDirty = true;
                m_idleTicks = 0.f;
            }
            else if (m_idleTicks < m_maxIdleTicks)
            {
                /*
                 * This logic sends updates unreliable for some time and then sends reliable update at the end.
                 * However, this is not necessary in the case of a value that is still a default one,
                 * since the new proxy event (that occurs prior) is always sent reliably.
                 */
                if (!m_isDefaultValue)
                {
                    isDirty = true;
                }

                m_idleTicks += 1.f;
            }

            if (isDirty)
            {
                m_throttler.UpdateBaseline(m_value);
                m_streamCache.Clear();
                m_streamCache.SetEndianType(endianType);
                m_streamCache.Write(m_value, m_marshaler);

                if (m_idleTicks >= m_maxIdleTicks)
                {
                    pdr.m_isDownstreamReliableDirty = true;
                }
                else
                {
                    pdr.m_isDownstreamUnreliableDirty = true;
                }
            }
            else if (marshalFlags & ReplicaMarshalFlags::ForceDirty)
            {
                /*
                 * If the dataset is not dirty but the current operation is forcing dirty then
                 * we need to prepare the stream cache by writing the current value in,
                 * otherwise, the marshalling logic will send nothing or an out of date value.
                 *
                 * This can occur with NewOwner command, for example.
                 */

                m_streamCache.Clear();
                m_streamCache.SetEndianType(endianType);
                m_streamCache.Write(m_value, m_marshaler);
            }
            return pdr;
        }

        void Unmarshal(UnmarshalContext& mc) override
        {
            if (MarshalerShim::Unmarshal(mc,m_marshaler,m_value,typename AZStd::is_pointer<DataType>::type()))
            {
                m_lastUpdateTime = mc.m_timestamp;
                m_replicaChunk->AddDataSetEvent(this);
            }
        }

        void ResetDirty() override
        {
            m_idleTicks = m_maxIdleTicks;
        }

        bool IsWithinToleranceThreshold()
        {
            return m_throttler.WithinThreshold(m_value);
        }

        DataType m_value;
        ThrottlerType m_throttler;
        MarshalerType m_marshaler;
        float m_idleTicks;
    };
    //-----------------------------------------------------------------------------

    /**
        Declares a DataSet with an event handler that is called when the DataSet is changed.
        Use BindInterface<Class, FuncPtr> to dispatch to a method on the ReplicaChunk's
        ReplicaChunkInterface event handler instance.
    **/
    template<typename DataType, typename MarshalerType, typename ThrottlerType>
    template<class C, void (C::* FuncPtr)(const DataType&, const TimeContext&)>
    class DataSet<DataType, MarshalerType, ThrottlerType>::BindInterface
        : public DataSet<DataType, MarshalerType, ThrottlerType>
    {
    public:
        BindInterface(const char* debugName,
            const DataType& value = DataType(),
            const MarshalerType& marshaler = MarshalerType(),
            const ThrottlerType& throttler = ThrottlerType())
            : DataSet(debugName,
                value,
                marshaler,
                throttler)
        { }

    protected:
        void DispatchChangedEvent(const TimeContext& tc) override
        {
            C* c = static_cast<C*>(m_replicaChunk->GetHandler());
            if (c)
            {
                TimeContext changeTime;
                changeTime.m_realTime = m_lastUpdateTime;
                changeTime.m_localTime = m_lastUpdateTime - (tc.m_realTime - tc.m_localTime);

                (*c.*FuncPtr)(m_value, changeTime);
            }
        }
    };

    /**
    * CtorContextBase
    */
    class CtorContextBase
    {
        friend class CtorDataSetBase;

        static CtorContextBase* s_pCur;

    protected:
        //-----------------------------------------------------------------------------
        class CtorDataSetBase
        {
        public:
            CtorDataSetBase();
            virtual ~CtorDataSetBase() { }

            virtual void Marshal(WriteBuffer& wb) = 0;
            virtual void Unmarshal(ReadBuffer& rb) = 0;
        };
        //-----------------------------------------------------------------------------
        template<typename DataType, typename MarshalerType = Marshaler<DataType> >
        class CtorDataSet
            : public CtorDataSetBase
        {
            MarshalerType m_marshaler;
            DataType m_value;

        public:
            CtorDataSet(const MarshalerType& marshaler = MarshalerType())
                : m_marshaler(marshaler) { }

            void Set(const DataType& val) { m_value = val; }
            const DataType& Get() const { return m_value; }

            virtual void Marshal(WriteBuffer& wb)
            {
                wb.Write(m_value, m_marshaler);
            }

            virtual void Unmarshal(ReadBuffer& rb)
            {
                rb.Read(m_value, m_marshaler);
            }
        };
        //-----------------------------------------------------------------------------

    private:
        typedef vector<CtorDataSetBase*> MembersArrayType;
        MembersArrayType m_members;

    public:
        CtorContextBase();

        void Marshal(WriteBuffer& wb);
        void Unmarshal(ReadBuffer& rb);
    };
    //-----------------------------------------------------------------------------
} // namespace GridMate

#endif // GM_DATASET_H
