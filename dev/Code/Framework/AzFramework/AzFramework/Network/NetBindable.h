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

#ifndef AZFRAMEWORK_NET_BINDABLE_H
#define AZFRAMEWORK_NET_BINDABLE_H

#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Replica/ReplicaChunkInterface.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/RemoteProcedureCall.h>

namespace AZ
{
    class ReflectContext;
    namespace Internal
    {
        template <class FieldType>
        class AzFrameworkNetBindableFieldContainer;
    }
}

namespace AzFramework
{
    using GridMate::DataSetBase;
    using GridMate::DataSet;
    using GridMate::Marshaler;
    using GridMate::BasicThrottle;
    using GridMate::RpcBase;
    using GridMate::TimeContext;
    using GridMate::RpcContext;
    using GridMate::RpcDefaultTraits;

    /**
    * Components that want to be synchronized over the network should implement NetBindable.
    * The NetBindable interface is obtained via AZ_RTTI so components need to make sure to
    * declare NetBindable as a base class in their AZ_RTTI declaration (or AZ_COMPONENT declaration),
    * as well as to declare both AZ::Component and NetBindable as base classes in the reflection.
    */
    class NetBindable
        : public GridMate::ReplicaChunkInterface
    {
    public:
        AZ_RTTI(NetBindable, "{80206665-D429-4703-B42E-94434F82F381}");

        NetBindable();
        virtual ~NetBindable() {}

        void NetInit();        

        //! Called during network binding on the master. The default implementation will use the
        //! NetworkContext to create a chunk. User implementations should create and return a new binding.
        virtual GridMate::ReplicaChunkPtr GetNetworkBinding();

        //! Called during network binding on proxies.
        virtual void SetNetworkBinding(GridMate::ReplicaChunkPtr chunk) = 0;

        //! Called when network is unbound. Implementations should release their references to the binding.
        virtual void UnbindFromNetwork() = 0;

        static void Reflect(AZ::ReflectContext* reflection);

        template <class DataType, typename MarshalerType = Marshaler<DataType>, typename ThrottlerType = BasicThrottle<DataType> >
        class Field;

        template <class DataType, class InterfaceType, void (InterfaceType::*)(const DataType&, const TimeContext&), typename MarshalerType = Marshaler<DataType>, typename ThrottlerType = BasicThrottle<DataType> >
        class BoundField;

        template <typename ... Args>
        class Rpc;

        inline bool IsSyncEnabled() const { return m_isSyncEnabled; }
        //! Can be used to disabled net sync on a per component basis
        inline void SetSyncEnabled(bool enabled) { m_isSyncEnabled = enabled; }
    protected:
        bool m_isSyncEnabled;
    };

    class NetBindableFieldBase
    {
    public:
        virtual ~NetBindableFieldBase() = default;
        virtual void Bind(DataSetBase* dataSet) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    /// NetBindable::Field should be used for any field in a NetBindable that you want
    /// to be able to replicate
    ///////////////////////////////////////////////////////////////////////////
    template <class DataType, typename MarshalerType, typename ThrottlerType>
    class NetBindable::Field
        : public NetBindableFieldBase
    {
        friend class AZ::Internal::AzFrameworkNetBindableFieldContainer<NetBindable::Field<DataType, MarshalerType, ThrottlerType> >;
    public:
        AZ_TYPE_INFO(Field, "{00D56FA7-F8BD-402B-97FB-0E2599897056}", DataType, MarshalerType, ThrottlerType);
        using DataSetType = DataSet<DataType, MarshalerType, ThrottlerType>;
        using ValueType = DataType;

    public:
        Field(const DataType& value = DataType())
            : m_dataSet(nullptr)
            , m_value(value)
        {}
        ~Field() override = default;

        operator const DataType&() const
        {
            return m_dataSet ? m_dataSet->Get() : m_value;
        }

        Field& operator=(const DataType& val)
        {
            if (m_dataSet)
            {
                m_dataSet->Set(val);
            }
            else
            {
                m_value = val;
            }
            return *this;
        }

        Field& operator=(const DataType&& val)
        {
            if (m_dataSet)
            {
                m_dataSet->Set(AZStd::forward<const DataType>(val));
            }
            else
            {
                m_value = AZStd::move(val);
            }
            return *this;
        }

        void Bind(DataSetBase* dataSet) override
        {
            BindDataSet(static_cast<DataSetType*>(dataSet));
        }

        static void ConstructDataSet(void* mem, const char* name)
        {
            new (mem) DataSetType(name, DataType(), MarshalerType(), ThrottlerType());
        }

        static void DestructDataSet(void* mem)
        {
            DataSetType* dataSet = reinterpret_cast<DataSetType*>(mem);
            dataSet->~DataSetType();
        }

    protected:
        template <class DST>
        void BindDataSet(DST* dataSet)
        {
            if (m_dataSet)
            {
                m_value = m_dataSet->Get();
            }
            m_dataSet = dataSet;
            if (m_dataSet)
            {
                m_dataSet->Set(AZStd::move(m_value));
                m_value = DataType();
            }
        }

        DataType* CacheValue()
        {
            if (m_dataSet)
            {
                m_value = m_dataSet->Get();
            }
            return &m_value;
        }

        const DataType& GetCachedValue() const
        {
            return m_value;
        }

    private:
        DataSet<DataType, MarshalerType, ThrottlerType>* m_dataSet;
        DataType m_value;
    };

    template <class DataType, class InterfaceType, void (InterfaceType::* FuncPtr)(const DataType&, const TimeContext&), typename MarshalerType, typename ThrottlerType>
    class NetBindable::BoundField
        : public NetBindable::Field<DataType, MarshalerType, ThrottlerType>
    {
        friend class AZ::Internal::AzFrameworkNetBindableFieldContainer<NetBindable::BoundField<DataType, InterfaceType, FuncPtr, MarshalerType, ThrottlerType> >;
    public:
        AZ_TYPE_INFO(BoundField, "{5151CEAF-6AC0-45D7-AEDF-8B6C46CE07B9}", DataType, InterfaceType, MarshalerType, ThrottlerType);
        using DataSetType = typename DataSet<DataType, MarshalerType, ThrottlerType>::template BindInterface<InterfaceType, FuncPtr>;

    public:
        BoundField(const DataType& value = DataType())
            : NetBindable::Field<DataType, MarshalerType, ThrottlerType>(value)
        {}
        ~BoundField() override = default;

        void Bind(DataSetBase* dataSet) override
        {
            this->BindDataSet(static_cast<DataSetType*>(dataSet));
        }

        static void ConstructDataSet(void* mem, const char* name)
        {
            new (mem) DataSetType(name, DataType(), MarshalerType(), ThrottlerType());
        }

        static void DestructDataSet(void* mem)
        {
            DataSetType* dataSet = reinterpret_cast<DataSetType*>(mem);
            dataSet->~DataSetType();
        }
    };

    class NetBindableRpcBase
    {
    public:
        virtual ~NetBindableRpcBase() = default;
        virtual void Bind(RpcBase* rpc) = 0;
        virtual void Bind(NetBindable* handler) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    /// NetBindable::Rpc::Binder should be used for any RPC in a NetBindable that you want
    /// to be able to call remotely. If the object is not network bound, RPC
    /// calls will dispatch directly, as if the object were authoritative
    ///////////////////////////////////////////////////////////////////////////
    template <typename ... Args>
    class NetBindable::Rpc
    {
    public:
        template<class InterfaceType, bool (InterfaceType::* FuncPtr)(Args..., const RpcContext&), class Traits = RpcDefaultTraits>
        class Binder
            : public NetBindableRpcBase
        {
            friend class NetworkContext;
        public:
            using BindInterfaceType = typename GridMate::Rpc<GridMate::RpcArg<Args>...>::template BindInterface<InterfaceType, FuncPtr, Traits>;

            Binder()
                : m_rpc(nullptr)
                , m_instance(nullptr)
            {}

            void Bind(RpcBase* rpc) override
            {
                m_rpc = static_cast<BindInterfaceType*>(rpc);
                m_instance = nullptr;
            }

            void Bind(NetBindable* bindable)
            {
                m_instance = static_cast<InterfaceType*>(bindable);
                m_rpc = nullptr;
            }

            template <typename ... CallArgs>
            void operator()(CallArgs&& ... args)
            {
                AZ_Assert(m_instance || m_rpc, "Cannot call an RPC without either a local instance or a network bound handler, did you forget to call NetBindable::NetInit()?");
                if (m_rpc) // connected to network
                {
                    (*m_rpc)(AZStd::forward<CallArgs>(args) ...);
                }
                else if (m_instance) // local dispatch
                {
                    (*m_instance.*FuncPtr)(AZStd::forward<CallArgs>(args) ..., RpcContext());
                }
            }

        protected:
            static void ConstructRpc(void* mem, const char* name)
            {
                new (mem) BindInterfaceType(name);
            }

            static void DestructRpc(void*) { }

        private:
            BindInterfaceType* m_rpc;
            InterfaceType*     m_instance;
        };

        Rpc() = delete;
    };
}   // namespace AzFramework

namespace AZ
{
    namespace Internal
    {
        template <class FieldType>
        class AzFrameworkNetBindableFieldContainer
            : public SerializeContext::IDataContainer
        {
            using ValueType = typename FieldType::ValueType;
        public:
            AzFrameworkNetBindableFieldContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_dataSize = sizeof(ValueType);
                m_classElement.m_offset = 0;
                m_classElement.m_azRtti = GetRttiHelper<ValueType>();
                m_classElement.m_flags = AZStd::is_pointer<ValueType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                m_classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueType>::GetGenericInfo();
                m_classElement.m_typeId = SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
                m_classElement.m_editData = nullptr;
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            virtual const SerializeContext::ClassElement* GetElement(AZ::u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array
            virtual void EnumElements(void* instance, const ElementCB& cb) override
            {
                FieldType* field = reinterpret_cast<FieldType*>(instance);
                // We can't mess with the internal storage of the dataset safely, so we copy it into
                // the field's local value cache temporarily, then hand that to the callback
                // This will modify the local value cache, but that shouldn't matter as it will never
                // be used as long as a dataset is bound
                // If this turns out to be a perf problem due to copies of complex types, then
                // the easy solution is to get DataSets to expose a pointer to their underlying
                // data storage, and then we can return a pointer to that and modify it directly
                // if the field is bound to the network
                ValueType* valPtr = field->CacheValue();
                cb(valPtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement);
                // Ensure that the dataset is updated if changes happened
                *field = *valPtr;
            }

            /// Return number of elements in the container.
            virtual size_t  Size(void*) const override
            {
                return 1;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            virtual size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 1;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            virtual bool    IsStableElements() const override { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            virtual bool    IsFixedSize() const override { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            virtual bool    IsFixedCapacity() const override            { return true; }

            /// Returns true if the container is a smart pointer.
            virtual bool    IsSmartPointer() const override { return true; }

            /// Returns true if the container elements can be addressed by index, otherwise false.
            virtual bool    CanAccessElementsByIndex() const override { return false; }

            /// Reserve element
            virtual void*   ReserveElement(void* instance, const  SerializeContext::ClassElement*) override
            {
                FieldType* field = reinterpret_cast<FieldType*>(instance);
                *field = ValueType();
                return field->CacheValue(); // return the local value, should be accurate as the field will be unbound at serialization time
            }

            /// Get an element's address by its index (called before the element is loaded).
            virtual void*   GetElementByIndex(void*, const SerializeContext::ClassElement*, size_t) override
            {
                return nullptr;
            }

            /// Store element
            virtual void    StoreElement(void* instance, void*) override
            {
                // force store the value again, just in case the field is bound to a dataset
                FieldType* field = reinterpret_cast<FieldType*>(instance);
                *field = field->GetCachedValue();
            }

            /// Remove element in the container.
            virtual bool    RemoveElement(void* instance, const void*, SerializeContext*) override
            {
                FieldType* field = reinterpret_cast<FieldType*>(instance);
                *field = ValueType();
                return false; // you can't remove element from this container.
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            virtual size_t  RemoveElements(void* instance, const void**, size_t, SerializeContext*) override
            {
                RemoveElement(instance, nullptr, nullptr);
                return 0; // you can't remove elements from this container.
            }

            /// Clear elements in the instance.
            virtual void    ClearElements(void* instance, SerializeContext*) override
            {
                RemoveElement(instance, nullptr, nullptr);
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };
    }

    template <class DataType, typename MarshalerType, typename ThrottlerType>
    struct SerializeGenericTypeInfo< AzFramework::NetBindable::Field<DataType, MarshalerType, ThrottlerType> >
    {
        typedef typename AzFramework::NetBindable::Field<DataType, MarshalerType, ThrottlerType>    ContainerType;

        class GenericClassNetBindableField
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassNetBindableField, "{C1D4DD97-5DD7-42ED-969C-7435F27F5D8C}");
            GenericClassNetBindableField()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AzFramework::NetBindable::Field", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t) override
            {
                return SerializeGenericTypeInfo<DataType>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassNetBindableField* Instance()
            {
                static GenericClassNetBindableField s_instance;
                return &s_instance;
            }

        protected:
            Internal::AzFrameworkNetBindableFieldContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassNetBindableField::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassNetBindableField::Instance()->GetClassData()->m_typeId;
        }
    };

    template <class DataType, class InterfaceType, void (InterfaceType::* FuncPtr)(const DataType&, const AzFramework::TimeContext&), typename MarshalerType, typename ThrottlerType>
    struct SerializeGenericTypeInfo< typename AzFramework::NetBindable::BoundField<DataType, InterfaceType, FuncPtr, MarshalerType, ThrottlerType> >
    {
        typedef typename AzFramework::NetBindable::BoundField<DataType, InterfaceType, FuncPtr, MarshalerType, ThrottlerType>    ContainerType;

        class GenericClassNetBindableBoundField
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassNetBindableBoundField, "{EFD64FE7-9432-401A-B7A1-1767F4C5A7F0}");
            GenericClassNetBindableBoundField()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AzFramework::NetBindable::BoundField", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t) override
            {
                return SerializeGenericTypeInfo<DataType>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassNetBindableBoundField* Instance()
            {
                static GenericClassNetBindableBoundField s_instance;
                return &s_instance;
            }

        protected:
            Internal::AzFrameworkNetBindableFieldContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassNetBindableBoundField::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassNetBindableBoundField::Instance()->GetClassData()->m_typeId;
        }
    };
}

#endif // AZFRAMEWORK_NET_BINDABLE_H
#pragma once
