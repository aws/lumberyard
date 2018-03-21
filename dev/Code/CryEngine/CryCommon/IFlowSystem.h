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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_IFLOWSYSTEM_H
#define CRYINCLUDE_CRYCOMMON_IFLOWSYSTEM_H
#pragma once


#include <BoostHelpers.h>
#include <CrySizer.h>
#include "SerializeFwd.h"
#include <ISerialize.h>
#include <AzCore/std/typetraits/type_id.h>
#include <AzCore/std/utils.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include "MathConversion.h"
#include <AzCore/Casting/numeric_cast.h>

#define _UICONFIG(x) x

struct IFlowGraphModuleManager;
struct IFlowGraphDebugger;

typedef uint8 TFlowPortId;
typedef uint16 TFlowNodeId;
typedef uint16 TFlowNodeTypeId;
typedef uint32 TFlowGraphId;
typedef int TFlowSystemContainerId;

static const TFlowNodeId InvalidFlowNodeId = ~TFlowNodeId(0);
static const TFlowPortId InvalidFlowPortId = ~TFlowPortId(0);
static const TFlowNodeTypeId InvalidFlowNodeTypeId = 0; // must be 0! FlowSystem.cpp relies on it
static const TFlowGraphId InvalidFlowGraphId = ~TFlowGraphId(0);
static const TFlowSystemContainerId InvalidContainerId = ~TFlowSystemContainerId(0);

struct FlowEntityId final
{
public:

    using StorageType = AZ::u64;
    static const StorageType s_invalidFlowEntityID = 0;

    FlowEntityId()
        : m_id(s_invalidFlowEntityID)
    {}

    explicit FlowEntityId(AZ::EntityId id)
    {
        m_id = static_cast<AZ::u64>(id);
    }

    explicit FlowEntityId(EntityId id)
    {
        m_id = static_cast<StorageType>(id);
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    void Serialize(TSerialize ser)
    {
    }

    FlowEntityId& operator = (const AZ::EntityId id)
    {
        m_id = static_cast<StorageType>(id);
        return *this;
    }

    FlowEntityId& operator = (StorageType id)
    {
        m_id = id;
        return *this;
    }

    operator AZ::EntityId() const
    {
        const AZ::EntityId id = static_cast<AZ::EntityId>(m_id);
        return id;
    }

    operator EntityId() const
    {
        const /*Cry*/ EntityId id = static_cast<EntityId>(m_id);
        return id;
    }

    const StorageType& GetId() const { return m_id; }

private:

    StorageType m_id;
};


inline bool operator==(const FlowEntityId& a, const FlowEntityId& b)
{
    return a.GetId() == b.GetId();
}

// Notes:
//   This is a special type which means "no input data"
struct SFlowSystemVoid
{
    void Serialize(TSerialize ser) {}
    void GetMemoryUsage(ICrySizer* pSizer) const{}
};

inline bool operator==(const SFlowSystemVoid& a, const SFlowSystemVoid& b)
{
    return true;
}

struct FlowVec3
    : public Vec3
{
    FlowVec3()
        : Vec3(ZERO) {}
    FlowVec3(float x, float y, float z)
        : Vec3(x, y, z) {}
};

inline bool operator!=(const AZ::Vector3& v, const int& i)
{
    return false;
}

struct SFlowSystemPointer
{
    void* m_pointer;
    SFlowSystemPointer()
        : m_pointer(nullptr) {}
    SFlowSystemPointer(void* ptr)
        : m_pointer(ptr) {}
    void Serialize(TSerialize ser) {}
    void GetMemoryUsage(ICrySizer* pSizer) const{}
};

inline bool operator==(const SFlowSystemPointer& a, const SFlowSystemPointer& b)
{
    return a.m_pointer == b.m_pointer;
}

// The FlowCustomData class implements a "generic" flow node port type. This
// allows a flow node implementation to use a class of their own definition for
// the value of the port without needing to make any changes to existing
// engine or editor code.
//
// To create a custom port type, first define a class that extends the
// FlowCustomData::Base class. For example:
//
//     class MyData : public FlowCustomData::Base { ... }
//
// Objects of this class will be copied as the value moves from port to port.
// See FlowCustomData::Base information on how to implement conversions and
// serialization for your custom port type.
//
// To define ports that uses your type, include the following in your input or
// output port definitions:
//
//     InputPortConfig_CustomData<MyData>( ... )
//     OutputPortConfig_CustomData<MyData>( ... )
//
// To get your data from an input port, use:
//
//     GetPortCustomData<MyData>()
//
// To activate an output port using your data, use:
//
//    ActivateCustomDataOutput<MyData>(...)
//
class FlowCustomData
{
public:

    // Custom data types may extend this class to get default implementations
    // of all the required methods, but this is not required. All methods are
    // called directly against the type specified when the port is defined.
    // Name hiding, not virtual methods, is used to customize behavior in the
    // derived class.
    class Base
    {
    public:

        // Implement the following methods in your derived class to
        // enable the needed conversions.

        bool SetFromVoid(const SFlowSystemVoid& from) { return false; }
        bool SetFromInt(const int& from) { return false; };
        bool SetFromFloat(const float& from) { return false; };
        bool SetFromDouble(const double& from) { return false; };
        bool SetFromEntityId(const EntityId& from) { return false; };
        bool SetFromFlowEntityId(const FlowEntityId& from) { return false; };
        bool SetFromAZVector3(const AZ::Vector3& from) { return false; };
        bool SetFromVec3(const Vec3& from) { return false; };
        bool SetFromString(const string& from) { return false; };
        bool SetFromBool(const bool& from) { return false; };
        bool SetFromObject(const FlowCustomData& from) { return false; };
        bool SetFromPointer(const SFlowSystemPointer& from) { return false; };

        bool GetAsVoid(SFlowSystemVoid& to) const { return true; };
        bool GetAsInt(int& to) const { return false; };
        bool GetAsFloat(float& to) const { return false; };
        bool GetAsDouble(double& to) const { return false; };
        bool GetAsEntityId(EntityId& to) const { return false; };
        bool GetAsFlowEntityId(FlowEntityId& to) const { return false; };
        bool GetAsAZVector3(AZ::Vector3& to) const { return false; };
        bool GetAsVec3(Vec3& to) const { return false; };
        bool GetAsString(string& to) const { return false; };
        bool GetAsBool(bool& to) const { return false; };
        bool GetAsObject(FlowCustomData& to) const { return false; };
        bool GetAsPointer(SFlowSystemPointer& to) const { return false; };

        bool operator==(const Base& other) const
        {
            // By default two custom data objects are never equal, create
            // a more specialized operator to implement equality for your
            // type.
            //
            // Currently the flow graph framework requires that this operator be defined
            // but it isn't clear when it is actually used.
            return false;
        }

        void Serialize(TSerialize ser)
        {
            // Used to serialize game tokens. Game tokens are not currently
            // supported by custom port data, but such support could be added
            // in the future.
        }

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            // Size of *this has already been added. Implement this method
            // in your class to count heap resources owned by your object.
        }
    };

    // When working with "Any" ports, the follow methods can be used to
    // manipulate FlowCustomData values.

    // Determine if the FlowCustomData contains a value of the specified type.
    template<typename ValueType>
    bool IsType() const
    {
        return m_pWrapper->IsType<ValueType>();
    }

    // Get the value contained by the FlowCustomData. The specified type must
    // be the type of the value contained by the FlowCustomData. No conversion
    // is performed by this method.
    template<typename ValueType>
    const ValueType& Get() const
    {
        return m_pWrapper->Get<ValueType>();
    }

    // Set the value contained by the FlowCustomData using copy semantics.
    // The specified type must be the type of the value already contained
    // by the FlowCustomData. No conversion is performed by this method.
    template<typename ValueType>
    void Set(const ValueType& value)
    {
        m_pWrapper->Set<ValueType>(value);
    }

    // Set the value contained by the FlowCustomData using move semantics.
    // The specified type must be the type of the value already contained
    // by the FlowCustomData. No conversion is performed by this method.
    template<typename ValueType>
    void Set(ValueType&& value)
    {
        m_pWrapper->Set<ValueType>(std::forward<ValueType>(value));
    }

    // Get the value contained by the FlowCustomData converted to the specified
    // type. True is returned if the conversion was successful. False is
    // returned if the value could not be converted.
    template<typename To>
    bool GetAs(To& to) const
    {
        return m_pWrapper->GetAs(to);
    }

    // Set the value contained by the FlowCustomData by converting a value of
    // the specified type. True is returned if the conversion was successful.
    // False is returned if the value could not be converted.
    template<typename From>
    bool SetFrom(const From& from)
    {
        return m_pWrapper->SetFrom(from);
    }

    // Everything below this is an "implementation detail".

    FlowCustomData()
        : m_pWrapper(std::make_shared<Wrapper<Empty> >(Empty()))
    {
    }

    template<
        typename ValueType,
        typename AZStd::Utils::enable_if_c<!std::is_base_of<FlowCustomData, typename AZStd::decay<ValueType>::type>::value>::type
        >
    FlowCustomData(ValueType&& value)
        : m_pWrapper(std::make_shared<Wrapper<ValueType> >(std::forward<ValueType>(value)))
    {
    }

    template<typename ValueType>
    FlowCustomData(const ValueType& value)
        : m_pWrapper(std::make_shared<Wrapper<ValueType> >(value))
    {
    }

    FlowCustomData(const FlowCustomData& other)
        : m_pWrapper(other.m_pWrapper)
    {
    }

    FlowCustomData(FlowCustomData&& other)
        : m_pWrapper(std::move(other.m_pWrapper))
    {
    }

    FlowCustomData& operator=(const FlowCustomData& other)
    {
        m_pWrapper = other.m_pWrapper;
        return *this;
    };

    FlowCustomData& operator=(FlowCustomData&& other)
    {
        m_pWrapper = std::move(other.m_pWrapper);
        return *this;
    }

    ~FlowCustomData() = default;

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        m_pWrapper->GetMemoryUsage(pSizer);
    }

    void Serialize(TSerialize ser)
    {
        m_pWrapper->Serialize(ser);
    }

    bool operator==(const FlowCustomData& other) const
    {
        return *m_pWrapper == *other.m_pWrapper;
    }

    bool IsSameType(const FlowCustomData& other) const
    {
        return m_pWrapper->IsSameType(*other.m_pWrapper);
    }

private:

    class WrapperBase;

    // We use a shared ptr to allow a single custom value to be shared by
    // multiple inputs. However, this means it is important that the input
    // not be modified in place by the node implemenation.
    std::shared_ptr<WrapperBase> m_pWrapper;

    // This class is used only by the FlowCustomData's default constructor.
    class Empty
        : public FlowCustomData::Base
    {
    };

    // The WrapperBase and Wrapper<ValueType> classes work together to
    // capture the custom port type at compile time. This allows us to
    // execute the conversion, serialization, and sizing methods on the
    // custom port data object using template expansion instead of requring
    // the type to extend a base class and using virtual methods.
    //
    // It also allows use to use the correct value type constructors and
    // assignment operators, and we use the captured type when we need to
    // verify that the Set and Get operations specify the correct type.
    //
    // Currently we still end up with virtual methods in the wrapper for
    // some operations, but this is preferable to putting them on the
    // custom port type for a couple of reasons:
    //
    // 1) Instances of the custom port type are copied from port to port,
    // and not requringing these to have any virtual methods means they
    // don't have to have a vtable so this copying is just a little bit
    // less expensive.
    //
    // 2) Future changes to the flow graph implemenation could elmininate
    // the need for the virtual methods in the wrapper, without impacting
    // existing custom port types. Specifically, if the FlowCustomData type
    // were parameterized with ValueType, then the wrapper wouldn't be
    // necessary. But that approach won't work with the existing use of
    // boost:varient to store port values.

    class WrapperBase
    {
    public:

        virtual ~WrapperBase() {}

        template<typename ValueType>
        const ValueType& Get() const
        {
            assert(IsType<ValueType>());
            return static_cast<const Wrapper<ValueType>*>(this)->Get();
        }

        template<typename ValueType>
        void Set(const ValueType& value)
        {
            assert(IsType<ValueType>());
            static_cast<Wrapper<ValueType>*>(this)->Set(value);
        }

        template<typename ValueType>
        void Set(ValueType&& value)
        {
            assert(IsType<ValueType>());
            static_cast<Wrapper<ValueType>*>(this)->Set(std::forward<ValueType>(value));
        }

        virtual bool SetFrom(const SFlowSystemVoid& from) = 0;
        virtual bool SetFrom(const int& from) = 0;
        virtual bool SetFrom(const float& from) = 0;
        virtual bool SetFrom(const double& from) = 0;
        virtual bool SetFrom(const EntityId& from) = 0;
        virtual bool SetFrom(const FlowEntityId& from) = 0;
        virtual bool SetFrom(const AZ::Vector3& from) = 0;
        virtual bool SetFrom(const Vec3& from) = 0;
        virtual bool SetFrom(const string& from) = 0;
        virtual bool SetFrom(const bool& from) = 0;
        virtual bool SetFrom(const FlowCustomData& from) = 0;
        virtual bool SetFrom(const SFlowSystemPointer& from) = 0;

        virtual bool GetAs(SFlowSystemVoid& to) const = 0;
        virtual bool GetAs(int& to) const = 0;
        virtual bool GetAs(float& to) const = 0;
        virtual bool GetAs(double& to) const = 0;
        virtual bool GetAs(EntityId& to) const = 0;
        virtual bool GetAs(FlowEntityId& to) const = 0;
        virtual bool GetAs(AZ::Vector3& to) const = 0;
        virtual bool GetAs(Vec3& to) const = 0;
        virtual bool GetAs(string& to) const = 0;
        virtual bool GetAs(bool& to) const = 0;
        virtual bool GetAs(FlowCustomData& to) const = 0;
        virtual bool GetAs(SFlowSystemPointer& to) const = 0;

        bool operator==(const WrapperBase& other) const
        {
            if (!IsSameType(other))
            {
                return false;
            }
            else
            {
                return ValueEquals(other);
            }
        }

        bool IsSameType(const WrapperBase& other) const
        {
            return m_type == other.m_type;
        }

        template<typename ValueType>
        bool IsType() const
        {
            return m_type == GetType<ValueType>();
        }

        template<typename T>
        static const AZStd::type_id& GetType()
        {
            // Using AZCore RTTI implementation
            static const AZStd::type_id& TypeId = aztypeid(T);
            return TypeId;
        }

        virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
        virtual void Serialize(TSerialize ser) = 0;

    protected:

        WrapperBase(const AZStd::type_id& type)
            : m_type(type) {}

        virtual bool ValueEquals(const WrapperBase& other) const = 0;

    private:

        const AZStd::type_id& m_type;
    };

    template<typename ValueType>
    class Wrapper
        : public WrapperBase
    {
    public:

        Wrapper(const ValueType& value)
            : WrapperBase(GetType<ValueType>())
            , m_value(value)
        {
        }

        Wrapper(ValueType&& value)
            : WrapperBase(GetType<ValueType>())
            , m_value(std::move(value))
        {
        }

        // no copy semantics
        Wrapper(const Wrapper&) = delete;
        Wrapper(Wrapper&&) = delete;
        Wrapper& operator=(Wrapper&) = delete;
        Wrapper& operator=(Wrapper&&) = delete;

        const ValueType& Get() const
        {
            return m_value;
        }

        void Set(const ValueType& other)
        {
            m_value = other;
        }

        void Set(ValueType&& other)
        {
            m_value = std::move(other);
        }

        bool SetFrom(const SFlowSystemVoid& from) override
        {
            return m_value.SetFromVoid(from);
        }

        bool SetFrom(const int& from) override
        {
            return m_value.SetFromInt(from);
        }

        bool SetFrom(const float& from) override
        {
            return m_value.SetFromFloat(from);
        }

        bool SetFrom(const double& from) override
        {
            return m_value.SetFromDouble(from);
        }

        bool SetFrom(const EntityId& from) override
        {
            return m_value.SetFromEntityId(from);
        }

        bool SetFrom(const FlowEntityId& from) override
        {
            return m_value.SetFromFlowEntityId(from);
        }

        bool SetFrom(const AZ::Vector3& from) override
        {
            return m_value.SetFromAZVector3(from);
        }

        bool SetFrom(const Vec3& from) override
        {
            return m_value.SetFromVec3(from);
        }

        bool SetFrom(const string& from) override
        {
            return m_value.SetFromString(from);
        }

        bool SetFrom(const bool& from) override
        {
            return m_value.SetFromBool(from);
        }

        bool SetFrom(const FlowCustomData& from) override
        {
            return m_value.SetFromObject(from);
        }

        bool SetFrom(const SFlowSystemPointer& from) override
        {
            return m_value.SetFromPointer(from);
        }

        bool GetAs(SFlowSystemVoid& to) const override
        {
            return m_value.GetAsVoid(to);
        }

        bool GetAs(int& to) const override
        {
            return m_value.GetAsInt(to);
        }

        bool GetAs(float& to) const override
        {
            return m_value.GetAsFloat(to);
        }

        bool GetAs(double& to) const override
        {
            return m_value.GetAsDouble(to);
        }

        bool GetAs(EntityId& to) const override
        {
            return m_value.GetAsEntityId(to);
        }

        bool GetAs(FlowEntityId& to) const override
        {
            return m_value.GetAsFlowEntityId(to);
        }

        bool GetAs(AZ::Vector3& to) const override
        {
            return m_value.GetAsAZVector3(to);
        }

        bool GetAs(Vec3& to) const override
        {
            return m_value.GetAsVec3(to);
        }

        bool GetAs(string& to) const override
        {
            return m_value.GetAsString(to);
        }

        bool GetAs(bool& to) const override
        {
            return m_value.GetAsBool(to);
        }

        bool GetAs(FlowCustomData& to) const override
        {
            return m_value.GetAsObject(to);
        }

        bool GetAs(SFlowSystemPointer& to) const override
        {
            return m_value.GetAsPointer(to);
        }

        void GetMemoryUsage(ICrySizer* pSizer) const override
        {
            pSizer->AddObjectSize(this);
            m_value.GetMemoryUsage(pSizer);
        }

        void Serialize(TSerialize ser) override
        {
            m_value.Serialize(ser);
        }

    private:

        bool ValueEquals(const WrapperBase& other) const
        {
            return m_value == static_cast<const Wrapper<ValueType>&>(other).m_value;
        }

        ValueType m_value;
    };
};

// Notes:
//   By adding types to this list, we can extend the flow system to handle
//   new data types. However, consider using the FlowCustomData type, which
//   allows you to pass custom opaque data from node to node without adding
//   a type to this list and making all the other changes that would require.
//   Important: If types need to be added, append at the end. Otherwise it breaks serialization!
// Remarks:
//   OTHER THINGS THAT NEED UPDATING HOWEVER INCLUDE:
//   CFlowData::ConfigureInputPort
// See also:
//   CFlowData::ConfigureInputPort
typedef boost::mpl::vector<
    SFlowSystemVoid,
    int,
    float,
    EntityId,
    Vec3,
    string,
    bool,
    FlowCustomData,
    SFlowSystemPointer,
    double,
    FlowEntityId,
    AZ::Vector3
    > TFlowSystemDataTypes;

// Notes:
//   Default conversion uses C++ rules.
template <class From, class To>
struct SFlowSystemConversion
{
    static ILINE bool ConvertValue(const From& from, To& to)
    {
        to = (To)(from);
        return true;
    }
};

#define FLOWSYSTEM_NO_CONVERSION(T)                                                      \
    template <>                                                                          \
    struct SFlowSystemConversion<T, T> {                                                 \
        static ILINE bool ConvertValue(const T&from, T & to) { to = from; return true; } \
    }
FLOWSYSTEM_NO_CONVERSION(SFlowSystemVoid);
FLOWSYSTEM_NO_CONVERSION(int);
FLOWSYSTEM_NO_CONVERSION(uint64);
FLOWSYSTEM_NO_CONVERSION(float);
FLOWSYSTEM_NO_CONVERSION(double);
FLOWSYSTEM_NO_CONVERSION(EntityId);
FLOWSYSTEM_NO_CONVERSION(FlowEntityId);
FLOWSYSTEM_NO_CONVERSION(AZ::Vector3);
FLOWSYSTEM_NO_CONVERSION(Vec3);
FLOWSYSTEM_NO_CONVERSION(string);
FLOWSYSTEM_NO_CONVERSION(bool);
FLOWSYSTEM_NO_CONVERSION(SFlowSystemPointer);
#undef FLOWSYSTEM_NO_CONVERSION

// Notes:
//   Specialization for converting to bool to avoid compiler warnings.
template <class From>
struct SFlowSystemConversion<From, bool>
{
    static ILINE bool ConvertValue(const From& from, bool& to)
    {
        to = (from != 0);
        return true;
    }
};

inline bool Is32BitId(const FlowEntityId& id)
{
    return (id.GetId() >> 32) == 0;
}

template <>
struct SFlowSystemConversion<FlowEntityId, float>
{
    static ILINE bool ConvertValue(const FlowEntityId& from, float& to)
    {
        AZ_STATIC_ASSERT(sizeof(FlowEntityId::StorageType) == sizeof(AZ::u64), "FlowEntityId storage is expectd to be 64-bit for flow graph numeric conversions.");
        if (Is32BitId(from))
        {
            to = static_cast<float>(from.GetId());
        }
        else
        {
            AZ_Error("FlowGraph", false, "Illegal conversion from 64-bit entityId to float.");
            return false;
        }

        return true;
    }
};

template <>
struct SFlowSystemConversion<float, FlowEntityId>
{
    static ILINE bool ConvertValue(const float& from, FlowEntityId& to)
    {
        AZ_Warning("FlowGraph", false, "Unsafe conversion from float to entity Id");
        to = static_cast<FlowEntityId::StorageType>(from);
        return true;
    }
};

template <>
struct SFlowSystemConversion<FlowEntityId, int>
{
    static ILINE bool ConvertValue(const FlowEntityId& from, int& to)
    {
        AZ_STATIC_ASSERT(sizeof(FlowEntityId::StorageType) == sizeof(AZ::u64), "FlowEntityId storage is expectd to be 64-bit for flow graph numeric conversions.");
        if (Is32BitId(from))
        {
            to = static_cast<int>(from.GetId());
        }
        else
        {
            AZ_Error("FlowGraph", false, "Illegal conversion from 64-bit entityId to 32-bit int.");
            return false;
        }

        return true;
    }
};

template <>
struct SFlowSystemConversion<int, FlowEntityId>
{
    static ILINE bool ConvertValue(const int& from, FlowEntityId& to)
    {
        FlowEntityId::StorageType temp = static_cast<FlowEntityId::StorageType>(from);
        if (temp == FlowEntityId::s_invalidFlowEntityID)
        {
            // Allow conversion without warning for values matching "invalid flow entity ID".
            to = FlowEntityId();
        }
        else
        {
            AZ_Warning("FlowGraph", false, "Unsafe conversion from int to entityId");
            to = temp;
        }
        return true;
    }
};

template <>
struct SFlowSystemConversion<FlowEntityId, double>
{
    static ILINE bool ConvertValue(const FlowEntityId& from, double& to)
    {
        to = static_cast<double>(from.GetId());
        return true;
    }
};

template <>
struct SFlowSystemConversion<double, FlowEntityId>
{
    static ILINE bool ConvertValue(const double& from, FlowEntityId& to)
    {
        to = static_cast<FlowEntityId::StorageType>(from);
        return true;
    }
};

template <>
struct SFlowSystemConversion<EntityId, double>
{
    static ILINE bool ConvertValue(const EntityId& from, double& to)
    {
        to = static_cast<double>(from);
        return true;
    }
};

template <>
struct SFlowSystemConversion<double, EntityId>
{
    static ILINE bool ConvertValue(const double& from, EntityId& to)
    {
        to = static_cast<EntityId>(from);
        return true;
    }
};


//////////////////////////////////////////////////////////////////////////
// To AZ::Vector3 conversions
//////////////////////////////////////////////////////////////////////////

template <>
struct SFlowSystemConversion < int, AZ::Vector3 >
{
    static ILINE bool ConvertValue(const int& from, AZ::Vector3& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < float, AZ::Vector3 >
{
    static ILINE bool ConvertValue(const float& from, AZ::Vector3& to)
    {
        to.Set(from);
        return true;
    }
};

template <>
struct SFlowSystemConversion < EntityId, AZ::Vector3 >
{
    static ILINE bool ConvertValue(const EntityId& from, AZ::Vector3& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < Vec3, AZ::Vector3 >
{
    static ILINE bool ConvertValue(const Vec3& from, AZ::Vector3& to)
    {
        to = LYVec3ToAZVec3(from);
        return true;
    }
};

template <>
struct SFlowSystemConversion < string, AZ::Vector3 >
{
    static ILINE bool ConvertValue(const string& from, AZ::Vector3& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < bool, AZ::Vector3 >
{
    static ILINE bool ConvertValue(const bool& from, AZ::Vector3& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < FlowCustomData, AZ::Vector3 >
{
    static ILINE bool ConvertValue(const FlowCustomData& from, AZ::Vector3& to)
    {
        return from.GetAs(to);
    }
};

template <>
struct SFlowSystemConversion < double, AZ::Vector3 >
{
    static ILINE bool ConvertValue(const double& from, AZ::Vector3& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < FlowEntityId, AZ::Vector3 >
{
    static ILINE bool ConvertValue(const FlowEntityId& from, AZ::Vector3& to)
    {
        return false;
    }
};

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// From AZ::Vector3 conversions
//////////////////////////////////////////////////////////////////////////

template <>
struct SFlowSystemConversion < AZ::Vector3, int >
{
    static ILINE bool ConvertValue(const AZ::Vector3& from, int& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < AZ::Vector3, float >
{
    static ILINE bool ConvertValue(const AZ::Vector3& from, float& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < AZ::Vector3, EntityId >
{
    static ILINE bool ConvertValue(const AZ::Vector3& from, EntityId& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < AZ::Vector3, Vec3 >
{
    static ILINE bool ConvertValue(const AZ::Vector3& from, Vec3& to)
    {
        to = AZVec3ToLYVec3(from);
        return true;
    }
};

template <>
struct SFlowSystemConversion < AZ::Vector3, string >
{
    static ILINE bool ConvertValue(const AZ::Vector3& from, string& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < AZ::Vector3, FlowCustomData >
{
    static ILINE bool ConvertValue(const AZ::Vector3& from, FlowCustomData& to)
    {
        return to.SetFrom(from);
    }
};

template <>
struct SFlowSystemConversion < AZ::Vector3, double >
{
    static ILINE bool ConvertValue(const AZ::Vector3& from, double& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion < AZ::Vector3, FlowEntityId >
{
    static ILINE bool ConvertValue(const AZ::Vector3& from, FlowEntityId& to)
    {
        return false;
    }
};

//////////////////////////////////////////////////////////////////////////

// Notes:
//   "Void" conversion needs some help
//   converting from void to anything yields default value of type.
template <class To>
struct SFlowSystemConversion<SFlowSystemVoid, To>
{
    static ILINE bool ConvertValue(SFlowSystemVoid, To& to)
    {
        to = To();
        return true;
    }
};
template <>
struct SFlowSystemConversion<SFlowSystemVoid, SFlowSystemPointer>
{
    static ILINE bool ConvertValue(const SFlowSystemVoid& from, SFlowSystemPointer& to)
    {
        to = SFlowSystemPointer();
        return true;
    }
};
template <>
struct SFlowSystemConversion<SFlowSystemVoid, bool>
{
    static ILINE bool ConvertValue(const SFlowSystemVoid& from, bool& to)
    {
        to = false;
        return true;
    }
};
template <>
struct SFlowSystemConversion<SFlowSystemVoid, Vec3>
{
    static ILINE bool ConvertValue(const SFlowSystemVoid& from, Vec3& to)
    {
        to = Vec3(0, 0, 0);
        return true;
    }
};
template <>
struct SFlowSystemConversion<Vec3, SFlowSystemVoid>
{
    static ILINE bool ConvertValue(const Vec3& from, SFlowSystemVoid&)
    {
        return true;
    }
};

template <>
struct SFlowSystemConversion < EntityId, SFlowSystemVoid >
{
    static ILINE bool ConvertValue(const EntityId& from, SFlowSystemVoid&)
    {
        return true;
    }
};

template <>
struct SFlowSystemConversion < FlowEntityId, SFlowSystemVoid >
{
    static ILINE bool ConvertValue(const FlowEntityId& from, SFlowSystemVoid&)
    {
        return true;
    }
};

// Notes:
//   Converting to void is trivial - just lose the value info.
template <class From>
struct SFlowSystemConversion<From, SFlowSystemVoid>
{
    static ILINE bool ConvertValue(const From& from, SFlowSystemVoid&)
    {
        return true;
    }
};

// Notes:
//   "Pointer" conversion needs some help
//   converting from void to anything yields default value of type.
template <class To>
struct SFlowSystemConversion<SFlowSystemPointer, To>
{
    static ILINE bool ConvertValue(SFlowSystemPointer, To& to)
    {
        to = To();
        return true;
    }
};
template <>
struct SFlowSystemConversion<SFlowSystemPointer, bool>
{
    static ILINE bool ConvertValue(const SFlowSystemPointer& from, bool& to)
    {
        to = false;
        return true;
    }
};
template <>
struct SFlowSystemConversion<SFlowSystemPointer, SFlowSystemVoid>
{
    static ILINE bool ConvertValue(const SFlowSystemPointer& from, SFlowSystemVoid& to)
    {
        to = SFlowSystemVoid();
        return true;
    }
};
template <>
struct SFlowSystemConversion<SFlowSystemPointer, Vec3>
{
    static ILINE bool ConvertValue(const SFlowSystemPointer& from, Vec3& to)
    {
        to = Vec3(0, 0, 0);
        return true;
    }
};
template <>
struct SFlowSystemConversion<Vec3, SFlowSystemPointer>
{
    static ILINE bool ConvertValue(const Vec3& from, SFlowSystemPointer&)
    {
        return false;
    }
};

// Notes:
//   Converting to void is trivial - just lose the value info.
template <class From>
struct SFlowSystemConversion<From, SFlowSystemPointer>
{
    static ILINE bool ConvertValue(const From& from, SFlowSystemPointer&)
    {
        return false;
    }
};

// Summary:
//   "Vec3" conversions...
template <>
struct SFlowSystemConversion<Vec3, EntityId>
{
    static ILINE bool ConvertValue(const Vec3& from, EntityId& to)
    {
        return false;
    }
};

template <>
struct SFlowSystemConversion<EntityId, Vec3>
{
    static ILINE bool ConvertValue(const EntityId& from, Vec3& to)
    {
        return false;
    }
};

template <class To>
struct SFlowSystemConversion<Vec3, To>
{
    static ILINE bool ConvertValue(const Vec3& from, To& to)
    {
        return SFlowSystemConversion<float, To>::ConvertValue(from.x, to);
    }
};

template <class From>
struct SFlowSystemConversion<From, Vec3>
{
    static ILINE bool ConvertValue(const From& from, Vec3& to)
    {
        float temp;
        if (!SFlowSystemConversion<From, float>::ConvertValue(from, temp))
        {
            return false;
        }
        to.x = to.y = to.z = temp;
        return true;
    }
};
template <>
struct SFlowSystemConversion<Vec3, bool>
{
    static ILINE bool ConvertValue(const Vec3& from, bool& to)
    {
        to = from.GetLengthSquared() > 0;
        return true;
    }
};

// FlowCustomData conversions

template <typename From>
struct SFlowSystemConversion < From, FlowCustomData >
{
    static ILINE bool ConvertValue(const From& from, FlowCustomData& to)
    {
        return to.SetFrom<From>(from);
    }
};

template <typename To>
struct SFlowSystemConversion < FlowCustomData, To >
{
    static ILINE bool ConvertValue(const FlowCustomData& from, To& to)
    {
        return from.GetAs(to);
    }
};

template <>
struct SFlowSystemConversion < FlowCustomData, FlowCustomData > // necessary to avoid template conflicts
{
    static ILINE bool ConvertValue(const FlowCustomData& from, FlowCustomData& to)
    {
        // Try converting both ways, this allows the conversion methods to be defined
        // on either of the custom port types.
        return to.SetFrom(from) || from.GetAs(to);
    }
};

template <>
struct SFlowSystemConversion < FlowCustomData, SFlowSystemVoid > // necessary to avoid template conflicts
{
    static ILINE bool ConvertValue(const FlowCustomData& from, SFlowSystemVoid& to)
    {
        return from.GetAs(to);
    }
};

template <>
struct SFlowSystemConversion < SFlowSystemVoid, FlowCustomData > // necessary to avoid template conflicts
{
    static ILINE bool ConvertValue(const SFlowSystemVoid& from, FlowCustomData& to)
    {
        return to.SetFrom(from);
    }
};

template <>
struct SFlowSystemConversion < Vec3, FlowCustomData > // necessary to avoid template conflicts
{
    static ILINE bool ConvertValue(const Vec3& from, FlowCustomData& to)
    {
        return to.SetFrom(from);
    }
};

template <>
struct SFlowSystemConversion < FlowCustomData, Vec3 > // necessary to avoid template conflicts
{
    static ILINE bool ConvertValue(const FlowCustomData& from, Vec3& to)
    {
        return from.GetAs(to);
    }
};

template <>
struct SFlowSystemConversion < FlowCustomData, bool > // necessary to avoid template conflicts
{
    static ILINE bool ConvertValue(const FlowCustomData& from, bool& to)
    {
        return from.GetAs(to);
    }
};

template <>
struct SFlowSystemConversion < FlowCustomData, SFlowSystemPointer > // necessary to avoid template conflicts
{
    static ILINE bool ConvertValue(const FlowCustomData& from, SFlowSystemPointer& to)
    {
        return from.GetAs(to);
    }
};

template <>
struct SFlowSystemConversion < SFlowSystemPointer, FlowCustomData > // necessary to avoid template conflicts
{
    static ILINE bool ConvertValue(const SFlowSystemPointer& from, FlowCustomData& to)
    {
        return to.SetFrom(from);
    }
};

#define FLOWSYSTEM_STRING_CONVERSION(type, fmt)                      \
    template <>                                                      \
    struct SFlowSystemConversion<type, string>                       \
    {                                                                \
        static ILINE bool ConvertValue(const type&from, string & to) \
        {                                                            \
            to.Format(fmt, from);                                    \
            return true;                                             \
        }                                                            \
    };                                                               \
    template <>                                                      \
    struct SFlowSystemConversion<string, type>                       \
    {                                                                \
        static ILINE bool ConvertValue(const string&from, type & to) \
        {                                                            \
            return 1 == azsscanf(from.c_str(), fmt, &to);            \
        }                                                            \
    };

FLOWSYSTEM_STRING_CONVERSION(int, "%d");
FLOWSYSTEM_STRING_CONVERSION(uint64, "%" PRIu64);
FLOWSYSTEM_STRING_CONVERSION(float, "%f");
FLOWSYSTEM_STRING_CONVERSION(double, "%lf");
FLOWSYSTEM_STRING_CONVERSION(EntityId, "%u");

template <>
struct SFlowSystemConversion<FlowEntityId, string>
{
    static ILINE bool ConvertValue(const FlowEntityId& from, string& to)
    {
        to.Format("%llu", from.GetId());
        return true;
    }
};

template <>
struct SFlowSystemConversion<string, FlowEntityId>
{
    static ILINE bool ConvertValue(const string& from, FlowEntityId& to)
    {
        FlowEntityId::StorageType id;
        if (1 == sscanf(from.c_str(), "%llu", &id))
        {
            to = id;
            return true;
        }

        return false;
    }
};

#undef FLOWSYSTEM_STRING_CONVERSION

template <>
struct SFlowSystemConversion<bool, string>
{
    static ILINE bool ConvertValue(const bool& from, string& to)
    {
        to.Format("%s", from ? "true" : "false");
        return true;
    }
};

template <>
struct SFlowSystemConversion<string, bool>
{
    // Description:
    //   Leaves 'to' unchanged in case of error.
    static ILINE bool ConvertValue(const string& from, bool& to)
    {
        int to_i;
        if (1 == sscanf(from.c_str(), "%d", &to_i))
        {
            to = !!to_i;
            return true;
        }
        if (0 == _stricmp (from.c_str(), "true"))
        {
            to = true;
            return true;
        }
        if (0 == _stricmp (from.c_str(), "false"))
        {
            to = false;
            return true;
        }
        return false;
    }
};

template <>
struct SFlowSystemConversion<Vec3, string>
{
    static ILINE bool ConvertValue(const Vec3& from, string& to)
    {
        to.Format("%g,%g,%g", from.x, from.y, from.z);
        return true;
    }
};

template <>
struct SFlowSystemConversion<string, Vec3>
{
    static ILINE bool ConvertValue(const string& from, Vec3& to)
    {
        return 3 == sscanf(from.c_str(), "%g,%g,%g", &to.x, &to.y, &to.z);
    }
};

enum EFlowDataTypes
{
    eFDT_Any = -1,
    eFDT_Void = boost::mpl::find<TFlowSystemDataTypes, SFlowSystemVoid>::type::pos::value,
    eFDT_Int = boost::mpl::find<TFlowSystemDataTypes, int>::type::pos::value,
    eFDT_Float = boost::mpl::find<TFlowSystemDataTypes, float>::type::pos::value,
    eFDT_EntityId = boost::mpl::find<TFlowSystemDataTypes, EntityId>::type::pos::value,
    eFDT_Vec3 = boost::mpl::find<TFlowSystemDataTypes, Vec3>::type::pos::value,
    eFDT_String = boost::mpl::find<TFlowSystemDataTypes, string>::type::pos::value,
    eFDT_Bool = boost::mpl::find<TFlowSystemDataTypes, bool>::type::pos::value,
    eFDT_CustomData = boost::mpl::find<TFlowSystemDataTypes, FlowCustomData>::type::pos::value,
    eFDT_Pointer = boost::mpl::find<TFlowSystemDataTypes, SFlowSystemPointer>::type::pos::value,
    eFDT_Double = boost::mpl::find<TFlowSystemDataTypes, double>::type::pos::value,
    eFDT_FlowEntityId = boost::mpl::find<TFlowSystemDataTypes, FlowEntityId>::type::pos::value,
    eFDT_AZVector3 = boost::mpl::find<TFlowSystemDataTypes, Vec3>::type::pos::value,
};


typedef boost::make_variant_over<TFlowSystemDataTypes>::type TFlowInputDataVariant;

template <class T>
struct DefaultInitialized
{
    T operator()() const { return T(); }
};

template <>
struct DefaultInitialized<Vec3>
{
    Vec3 operator()() const { return Vec3(ZERO); }
};

template <class Iter>
struct DefaultInitializedForTag
{
    bool operator()(int tag, TFlowInputDataVariant& v) const
    {
        if (tag == boost::mpl::distance<typename boost::mpl::begin<TFlowSystemDataTypes>::type, Iter>::type::value)
        {
            DefaultInitialized<typename boost::mpl::deref<Iter>::type> create;
            v = create();
            return true;
        }
        else
        {
            DefaultInitializedForTag<typename boost::mpl::next<Iter>::type> create;
            return create(tag, v);
        }
    }
};

template <>
struct DefaultInitializedForTag<typename boost::mpl::end<TFlowSystemDataTypes>::type>
{
    bool operator()(int tag, TFlowInputDataVariant& v) const
    {
        return false;
    }
};

class TFlowInputData;

// Declare, but not fully define, the IsSameType* classes so that
// TFlowInputData can use them. The classes can't be embedded in TFlowInputData
// because IsSameTypeExpl is specialized and only Visual Studio allows
// specialized templates to be declared in a class. Other compilers (like
// Clang) flag it as an error.
namespace TypeComparison
{
    class IsSameType
    {
        class StrictlyEqual
            : public boost::static_visitor<bool>
        {
        public:
            template <typename T, typename U>
            bool operator()(const T&, const U&) const
            {
                return false; // cannot compare different types
            }

            template <typename T>
            bool operator()(const T&, const T&) const
            {
                return true;
            }

            bool operator()(const FlowCustomData& a, const FlowCustomData& b)
            {
                // delegate to FlowCustomData to determine if the contained objects
                // have the same type
                return a.IsSameType(b);
            }
        };

    public:
        bool operator()(const TFlowInputData& a, const TFlowInputData& b) const;
    };

    template <typename Ref>
    class IsSameTypeExpl
    {
        class StrictlyEqual
            : public boost::static_visitor<bool>
        {
        public:
            template <typename T>
            bool operator()(const T&) const
            {
                return false; // cannot compare different types
            }

            bool operator()(const Ref&) const
            {
                return true;
            }
        };

    public:
        bool operator()(const TFlowInputData& a) const;
    };

    template <>
    class IsSameTypeExpl<FlowCustomData>
    {
        class StrictlyEqual
            : public boost::static_visitor<bool>
        {
        public:

            StrictlyEqual(const FlowCustomData& a)
                : static_visitor()
                , m_a(a)
            {
            }

            template <typename T>
            bool operator()(const T&) const
            {
                return false; // cannot compare different types
            }

            bool operator()(const FlowCustomData& b) const
            {
                return m_a.IsSameType(b);
            }

        private:

            const FlowCustomData& m_a;
        };

    public:
        bool operator()(const TFlowInputData& a, const FlowCustomData& other) const;
    };
}

class TFlowInputData
{
    class IsEqual
    {
        class StrictlyEqual
            : public boost::static_visitor<bool>
        {
        public:
            template <typename T, typename U>
            bool operator()(const T&, const U&) const
            {
                return false; // cannot compare different types
            }

            template <typename T>
            bool operator()(const T& lhs, const T& rhs) const
            {
                return lhs == rhs;
            }
        };

    public:
        bool operator()(const TFlowInputData& a, const TFlowInputData& b) const
        {
            return boost::apply_visitor(StrictlyEqual(), a.m_variant, b.m_variant);
        }
    };

    class ExtractType
        : public boost::static_visitor<EFlowDataTypes>
    {
    public:
        template <typename T>
        EFlowDataTypes operator()(const T& value) const
        {
            return (EFlowDataTypes) boost::mpl::find<TFlowSystemDataTypes, T>::type::pos::value;
        }
    };

    template <typename To>
    class ConvertType_Get
        : public boost::static_visitor<bool>
    {
    public:
        ConvertType_Get(To& to_)
            : to(to_) {}

        template <typename From>
        bool operator()(const From& from) const
        {
            return SFlowSystemConversion<From, To>::ConvertValue(from, to);
        }

        To& to;
    };

    template <typename From>
    class ConvertType_Set
        : public boost::static_visitor<bool>
    {
    public:
        ConvertType_Set(const From& from_)
            : from(from_) {}

        template <typename To>
        bool operator()(To& to) const
        {
            return SFlowSystemConversion<From, To>::ConvertValue(from, to);
        }

        const From& from;
    };

    class ConvertType_SetFlexi
        : public boost::static_visitor<bool>
    {
    public:
        ConvertType_SetFlexi(TFlowInputData& to_)
            : to(to_) {}

        template <typename From>
        bool operator()(const From& from) const
        {
            return boost::apply_visitor(ConvertType_Set<From>(from), to.m_variant);
        }

        TFlowInputData& to;
    };

    class WriteType
        : public boost::static_visitor<void>
    {
    public:
        WriteType(TSerialize ser, bool userFlag)
            : m_ser(ser)
            , m_userFlag(userFlag) {}

        template <class T>
        void operator()(T& v)
        {
            int tag = boost::mpl::find<TFlowSystemDataTypes, T>::type::pos::value;
            m_ser.Value("tag", tag);
            m_ser.Value("ud", m_userFlag);
            m_ser.Value("v", v);
        }

    private:
        TSerialize m_ser;
        bool m_userFlag;
    };

    class LoadType
        : public boost::static_visitor<void>
    {
    public:
        LoadType(TSerialize ser)
            : m_ser(ser) {}

        template <class T>
        void operator()(T& v)
        {
            m_ser.Value("v", v);
        }

    private:
        TSerialize m_ser;
    };

    class MemStatistics
        : public boost::static_visitor<void>
    {
    public:
        MemStatistics(ICrySizer* pSizer)
            : m_pSizer(pSizer) {}

        template <class T>
        void operator()(T& v)
        {
            m_pSizer->AddObject(v);
        }

    private:
        ICrySizer* m_pSizer;
    };

public:
    TFlowInputData()
        : m_variant()
        , m_userFlag(false)
        , m_locked(false)
    {}

    TFlowInputData(const TFlowInputData& rhs)
        : m_variant(rhs.m_variant)
        , m_userFlag(rhs.m_userFlag)
        , m_locked(rhs.m_locked)
    {}

    template <typename T>
    explicit TFlowInputData(const T& v, bool locked = false)
        : m_variant(v)
        , m_userFlag(false)
        , m_locked(locked)
    {}

    template <
        typename T,
        typename AZStd::Utils::enable_if_c<!std::is_base_of<TFlowInputData, typename AZStd::decay<T>::type>::value>::type
        >
    explicit TFlowInputData(T&& v, bool locked = false)
        : m_variant(std::forward<T>(v))
        , m_userFlag(false)
        , m_locked(locked)
    {}

    template <typename T>
    static TFlowInputData CreateDefaultInitialized(bool locked = false)
    {
        DefaultInitialized<T> create;
        return TFlowInputData(create(), locked);
    }

    static TFlowInputData CreateDefaultInitializedForTag(int tag, bool locked = false)
    {
        TFlowInputDataVariant v;
        DefaultInitializedForTag<boost::mpl::begin<TFlowSystemDataTypes>::type> create;
        create(tag, v);
        return TFlowInputData(v, locked);
    }

    TFlowInputData& operator=(const TFlowInputData& rhs)
    {
        if (this != &rhs)
        {
            m_variant = rhs.m_variant;
            m_userFlag = rhs.m_userFlag;
            m_locked = rhs.m_locked;
        }
        return *this;
    }

    bool operator==(const TFlowInputData& rhs) const
    {
        IsEqual isEqual;
        return isEqual(*this, rhs);
    }

    bool operator!=(const TFlowInputData& rhs) const
    {
        IsEqual isEqual;
        return !isEqual(*this, rhs);
    }

    bool SetDefaultForTag(int tag)
    {
        DefaultInitializedForTag<boost::mpl::begin<TFlowSystemDataTypes>::type> set;
        return set(tag, m_variant);
    }

    template<typename T>
    bool Set(const T& value)
    {
        TypeComparison::IsSameTypeExpl<T> isSameType;
        if (IsLocked() && !isSameType(*this))
        {
            return false;
        }
        else
        {
            m_variant = value;
            return true;
        }
    }

    bool Set(const FlowCustomData& value)
    {
        TypeComparison::IsSameTypeExpl<FlowCustomData> isSameType;
        if (IsLocked() && !isSameType(*this, value))
        {
            return false;
        }
        else
        {
            m_variant = value;
            return true;
        }
    }

    template <typename T>
    bool SetValueWithConversion(const T& value)
    {
        TypeComparison::IsSameTypeExpl<T> isSameType;
        if (IsLocked() && !isSameType(*this))
        {
            return boost::apply_visitor(ConvertType_Set<T>(value), m_variant);
        }
        else
        {
            m_variant = value;
            return true;
        }
    }

    bool SetValueWithConversion(const FlowCustomData& value)
    {
        TypeComparison::IsSameTypeExpl<FlowCustomData> isSameType;
        if (IsLocked() && !isSameType(*this, value))
        {
            return boost::apply_visitor(ConvertType_Set<FlowCustomData>(value), m_variant);
        }
        else
        {
            m_variant = value;
            return true;
        }
    }

    bool SetValueWithConversion(const TFlowInputData& value)
    {
        TypeComparison::IsSameType isSameType;
        if (IsLocked() && !isSameType(*this, value))
        {
            return boost::apply_visitor(ConvertType_SetFlexi(*this), value.m_variant);
        }
        else
        {
            m_variant = value.m_variant;
            return true;
        }
    }

    template<typename T>
    T* GetPtr() { return boost::get<T>(&m_variant); }
    template<typename T>
    const T* GetPtr() const { return boost::get<const T>(&m_variant); }

    const TFlowInputDataVariant& GetVariant() const { return m_variant; }

    template <typename T>
    bool GetValueWithConversion(T& value) const
    {
        TypeComparison::IsSameTypeExpl<T> isSameType;
        if (isSameType(*this))
        {
            value = *GetPtr<T>();
            return true;
        }
        else
        {
            return boost::apply_visitor(ConvertType_Get<T>(value), m_variant);
        }
    }

    EFlowDataTypes GetType() const { return boost::apply_visitor(ExtractType(), m_variant); }

    bool IsUserFlagSet() const { return m_userFlag; }
    void SetUserFlag(bool value) { m_userFlag = value; }
    void ClearUserFlag() { m_userFlag = false; }

    bool IsLocked() const { return m_locked; }
    void SetLocked() { m_locked = true; }
    void SetUnlocked() { m_locked = false; }

    void Serialize(TSerialize ser)
    {
        MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Configurable variant serialization");

        if (ser.IsWriting())
        {
            WriteType visitor(ser, IsUserFlagSet());
            boost::apply_visitor(visitor, m_variant);
        }
        else
        {
            int tag = -2; // should be safe
            ser.Value("tag", tag);

            SetDefaultForTag(tag);

            bool ud = m_userFlag;
            ser.Value("ud", ud);
            m_userFlag = ud;

            LoadType visitor(ser);
            boost::apply_visitor(visitor, m_variant);
        }
    }

    template <class Visitor>
    void Visit(Visitor& visitor)
    {
        boost::apply_visitor(visitor, m_variant);
    }

    template <class Visitor>
    void Visit(Visitor& visitor) const
    {
        boost::apply_visitor(visitor, m_variant);
    }

    void GetMemoryStatistics(ICrySizer* pSizer) const
    {
        MemStatistics visitor(pSizer);
        boost::apply_visitor(visitor, m_variant);
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        GetMemoryStatistics(pSizer);
    }

private:
    friend struct SEqualFlowInputData;

    TFlowInputDataVariant m_variant;

    bool m_userFlag : 1;
    bool m_locked : 1;
};

// Definitions of the operator() methods need to be after TFlowInputData class
// definition because they use methods defined in the class
namespace TypeComparison
{
    inline bool IsSameType::operator()(const TFlowInputData& a, const TFlowInputData& b) const
    {
        return boost::apply_visitor(StrictlyEqual(), a.GetVariant(), b.GetVariant());
    }

    template <typename Ref>
    inline bool IsSameTypeExpl<Ref>::operator()(const TFlowInputData& a) const
    {
        return boost::apply_visitor(StrictlyEqual(), a.GetVariant());
    }

    inline bool IsSameTypeExpl<FlowCustomData>::operator()(const TFlowInputData& a, const FlowCustomData& other) const
    {
        return boost::apply_visitor(StrictlyEqual(other), a.GetVariant());
    }
}

struct SFlowAddress
{
    SFlowAddress(TFlowNodeId _node, TFlowPortId _port, bool _isOutput)
    {
        this->node = _node;
        this->port = _port;
        this->isOutput = _isOutput;
    }
    SFlowAddress()
    {
        node = InvalidFlowNodeId;
        port = InvalidFlowPortId;
        isOutput = true;
    }
    bool operator ==(const SFlowAddress& n) const { return node == n.node && port == n.port && isOutput == n.isOutput; }
    bool operator !=(const SFlowAddress& n) const { return !(*this == n); }

    TFlowNodeId node;
    TFlowPortId port;
    bool isOutput;
};

//////////////////////////////////////////////////////////////////////////
// Summary:
//   Flags used by the flow system.
enum EFlowNodeFlags
{
    EFLN_TARGET_ENTITY         = 0x0001, // CORE FLAG: This node targets an entity, entity id must be provided.
    EFLN_HIDE_UI               = 0x0002, // CORE FLAG: This node cannot be selected by user for placement in flow graph UI.
    EFLN_DYNAMIC_OUTPUT        = 0x0004, // CORE FLAG: This node is setup for dynamic output port growth in runtime.
    EFLN_UNREMOVEABLE          = 0x0008,    // CORE FLAG: This node cannot be deleted by the user
    EFLN_CORE_MASK             = 0x000F, // CORE_MASK

    EFLN_APPROVED              = 0x0010, // CATEGORY:  This node is approved for designers.
    EFLN_ADVANCED              = 0x0020, // CATEGORY:  This node is slightly advanced and approved.
    EFLN_DEBUG                 = 0x0040, // CATEGORY:  This node is for debug purpose only.
    EFLN_OBSOLETE              = 0x0200, // CATEGORY:  This node is obsolete and is not available in the editor.
    EFLN_CATEGORY_MASK         = 0x0FF0, // CATEGORY_MASK.

    EFLN_ACTIVATION_INPUT      = 0x1000, // USAGE: This node uses an activation input port.
    EFLN_USAGE_MASK            = 0xF000, // USAGE_MASK.

    EFLN_AISEQUENCE_ACTION       = 0x010000,
    EFLN_AISEQUENCE_SUPPORTED    = 0x020000,
    EFLN_AISEQUENCE_END          = 0x040000,
    EFLN_AIACTION_START          = 0x080000,
    EFLN_AIACTION_END            = 0x100000,
    EFLN_TYPE_MASK               = 0xFF0000,
};

//////////////////////////////////////////////////////////////////////////
enum EFlowSpecialEntityId : int32
{
    EFLOWNODE_ENTITY_ID_GRAPH1 = -1,
    EFLOWNODE_ENTITY_ID_GRAPH2 = -2,
};

struct SInputPortConfig
{
    // Summary:
    //   Name of this port.
    const char* name;
    // Summary:
    //   Human readable name of this port (default: same as name).
    const char* humanName;
    // Summary:
    //   Human readable description of this port (help).
    const char* description;
    // Summary:
    //   UIConfig: enums for the variable
    // Example:
    //   "enum_string:a,b,c"
    //   "enum_string:something=a,somethingelse=b,whatever=c"
    //   "enum_int:something=0,somethingelse=10,whatever=20"
    //   "enum_float:something=1.0,somethingelse=2.0"
    //   "enum_global:GlobalEnumName"
    const char* sUIConfig;
    // Summary:
    //   Default data.
    TFlowInputData defaultData;

    void GetMemoryUsage(ICrySizer* pSizer) const{}
};

struct SOutputPortConfig
{
    // Summary:
    //   Name of this port.
    const char* name;
    // Summary:
    //   Human readable name of this port (default: same as name).
    const char* humanName;
    // Summary:
    //   Human readable description of this port (help).
    const char* description;
    // Summary:
    //   Type of our output (or -1 for "dynamic").
    int type;

    void GetMemoryUsage(ICrySizer* pSizer) const{}
};

struct SFlowNodeConfig
{
    SFlowNodeConfig()
        : pInputPorts(0)
        , pOutputPorts(0)
        , nFlags(EFLN_DEBUG)
        , sDescription(0)
        , sUIClassName(0) {}

    const SInputPortConfig* pInputPorts;
    const SOutputPortConfig* pOutputPorts;
    // Summary:
    //   Node configuration flags
    // See also:
    //   EFlowNodeFlags
    uint32 nFlags;
    const char* sDescription;
    const char* sUIClassName;

    ILINE void SetCategory(uint32 flags)
    {
        nFlags = (nFlags & ~EFLN_CATEGORY_MASK) | flags;
    }

    ILINE uint32 GetCategory() const
    {
        return /* static_cast<EFlowNodeFlags> */ (nFlags & EFLN_CATEGORY_MASK);
    }

    ILINE uint32 GetCoreFlags() const
    {
        return nFlags & EFLN_CORE_MASK;
    }

    ILINE uint32 GetUsageFlags() const
    {
        return nFlags & EFLN_USAGE_MASK;
    }

    ILINE uint32 GetTypeFlags() const
    {
        return nFlags & EFLN_TYPE_MASK;
    }
};

template <class T>
ILINE SOutputPortConfig OutputPortConfig(const char* name, const char* description = NULL, const char* humanName = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SOutputPortConfig result = {name, humanName, description, boost::mpl::find<TFlowSystemDataTypes, T>::type::pos::value};
    return result;
}

template <>
ILINE SOutputPortConfig OutputPortConfig<AZ::Vector3>(const char* name, const char* description, const char* humanName)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SOutputPortConfig result = { name, humanName, description, boost::mpl::find<TFlowSystemDataTypes, Vec3>::type::pos::value };
    return result;
}

ILINE SOutputPortConfig OutputPortConfig_AnyType(const char* name, const char* description = NULL, const char* humanName = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SOutputPortConfig result = {name, humanName, description, eFDT_Any};
    return result;
}

ILINE SOutputPortConfig OutputPortConfig_Void(const char* name, const char* description = NULL, const char* humanName = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SOutputPortConfig result = {name, humanName, description, eFDT_Void};
    return result;
}

template <typename ValueType>
// ValueType is not currently used
ILINE SOutputPortConfig OutputPortConfig_CustomData(const char* name, const char* description = nullptr, const char* humanName = nullptr)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SOutputPortConfig result = { name, humanName, description, eFDT_CustomData };
    return result;
}

ILINE SOutputPortConfig OutputPortConfig_Pointer(const char* name, const char* description = NULL, const char* humanName = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SOutputPortConfig result = {name, humanName, description, eFDT_Pointer};
    return result;
}

template <class T>
ILINE SInputPortConfig InputPortConfig(const char* name, const char* description = NULL, const char* humanName = NULL, const char* sUIConfig = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SInputPortConfig result = {name, humanName, description, sUIConfig, TFlowInputData::CreateDefaultInitialized<T>(true)};
    return result;
}

template <>
ILINE SInputPortConfig InputPortConfig<AZ::Vector3>(const char* name, const char* description, const char* humanName, const char* sUIConfig)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SInputPortConfig result = { name, humanName, description, sUIConfig, TFlowInputData::CreateDefaultInitialized<Vec3>(true) };
    return result;
}

template <class T, class ValueT>
ILINE SInputPortConfig InputPortConfig(const char* name, const ValueT& value, const char* description = NULL, const char* humanName = NULL, const char* sUIConfig = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SInputPortConfig result = {name, humanName, description, sUIConfig, TFlowInputData(T(value), true)};
    return result;
}

ILINE SInputPortConfig InputPortConfig_AnyType(const char* name, const char* description = NULL, const char* humanName = NULL, const char* sUIConfig = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SInputPortConfig result = {name, humanName, description, sUIConfig, TFlowInputData(0, false)};
    return result;
}

ILINE SInputPortConfig InputPortConfig_Void(const char* name, const char* description = NULL, const char* humanName = NULL, const char* sUIConfig = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SInputPortConfig result = {name, humanName, description, sUIConfig, TFlowInputData(SFlowSystemVoid(), false)};
    return result;
}

template<typename ValueType>
ILINE SInputPortConfig InputPortConfig_CustomData(const char* name, const ValueType& value, const char* description = NULL, const char* humanName = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SInputPortConfig result = { name, humanName, description, NULL, TFlowInputData(FlowCustomData(value), true) };
    return result;
}

template<typename ValueType>
ILINE SInputPortConfig InputPortConfig_CustomData(const char* name, ValueType&& value, const char* description = nullptr, const char* humanName = nullptr)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SInputPortConfig result = { name, humanName, description, nullptr, TFlowInputData(FlowCustomData(std::forward<ValueType>(value)), true) };
    return result;
}

ILINE SInputPortConfig InputPortConfig_Pointer(const char* name, const char* description = NULL, const char* humanName = NULL, const char* sUIConfig = NULL)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    SInputPortConfig result = {name, humanName, description, sUIConfig, TFlowInputData(SFlowSystemPointer(), false)};
    return result;
}

struct IAIAction;
struct ICustomAction;
struct IFlowGraph;

struct IFlowNode;
TYPEDEF_AUTOPTR(IFlowNode);
typedef IFlowNode_AutoPtr IFlowNodePtr;

struct IFlowNode
{
    struct SActivationInfo
    {
        SActivationInfo(IFlowGraph* graph = 0, TFlowNodeId id = 0, void* userData = 0, TFlowInputData* inputPorts = 0)
        {
            this->pGraph = graph;
            this->myID = id;
            this->m_pUserData = userData;
            this->pInputPorts = inputPorts;
            this->pEntity = 0;
            this->connectPort = InvalidFlowPortId;
            this->entityId = FlowEntityId::s_invalidFlowEntityID;
        }
        //! Graph this node belongs to
        IFlowGraph* pGraph;
        //! Id of this node
        TFlowNodeId myID;

        //! Entity Id of the entity attached to this node
        //! Can refer to component entity or cry entity
        FlowEntityId entityId;

        //! Actual Entity attached to this node (Only Cry Entities)
        IEntity* pEntity;
        TFlowPortId connectPort;
        TFlowInputData* pInputPorts;
        void* m_pUserData;
    };

    enum EFlowEvent
    {
        eFE_Update,
        eFE_Activate,               // Sent if one or more input ports have been activated.
        eFE_FinalActivate,          // Must be eFE_Activate+1 (same as activate, but at the end of FlowSystem:Update)
        eFE_PrecacheResources,  // Called after flow graph loading to precache resources inside flow nodes.
        eFE_Initialize,             // Sent once after level has been loaded. it is NOT called on Serialization!
        eFE_FinalInitialize,        // Must be eFE_Initialize+1
        eFE_SetEntityId,            // This event is send to set the entity of the FlowNode. Might also be sent in conjunction (pre) other events (like eFE_Initialize).
        eFE_Suspend,
        eFE_Resume,
        eFE_ConnectInputPort,
        eFE_DisconnectInputPort,
        eFE_ConnectOutputPort,
        eFE_DisconnectOutputPort,
        eFE_Uninitialize,
        eFE_DontDoAnythingWithThisPlease
    };

    // <interfuscator:shuffle>
    virtual ~IFlowNode(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual IFlowNodePtr Clone(SActivationInfo*) = 0;

    virtual void GetConfiguration(SFlowNodeConfig&) = 0;
    virtual bool SerializeXML(SActivationInfo*, const XmlNodeRef& root, bool reading) = 0;
    virtual void Serialize(SActivationInfo*, TSerialize ser) = 0;
    virtual void PostSerialize(SActivationInfo*) = 0;
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo*) = 0;

    virtual void GetMemoryUsage(ICrySizer* s) const = 0;

    virtual const char* GetClassTag() const { return ""; }
    virtual const char* GetUIName() const { return ""; }
    // Summary:
    //   Used to return the global UI enum name to use for the given input port, if the port defines a UI enum of type 'enum_global_def'
    //   Returns TRUE if a global enum name was determined and should be used. Otherwise the common name is used.
    // In:
    //   portId - The input port Id for which to determine the global enum name
    //   pNodeEntity - Current entity attached to the node
    //   szName - The common name defined with the port (enum_global_def:commonName)
    // Out:
    //   outGlobalEnum - The global enum name to use for this port
    virtual bool GetPortGlobalEnum(uint32 portId, IEntity* pNodeEntity, const char* szName, string& outGlobalEnum) const { return false; }
    // </interfuscator:shuffle>
};

// Summary:
//   Wraps IFlowNode for specific data
struct IFlowNodeData
{
    // <interfuscator:shuffle>
    virtual ~IFlowNodeData(){}
    // Summary:
    //   Gets a pointer to the node.
    virtual IFlowNode* GetNode() = 0;
    // Summary:
    //   Gets the ID of the node type.
    virtual TFlowNodeTypeId GetNodeTypeId() = 0;
    // Summary:
    //   Gets the node's name.
    virtual const char* GetNodeName() = 0;
    virtual void GetConfiguration(SFlowNodeConfig&) = 0;

    // Summary:
    //   Gets the amount of ports in the flownode
    virtual int GetNumInputPorts() const = 0;
    virtual int GetNumOutputPorts() const = 0;

    virtual EntityId GetCurrentForwardingEntity() const = 0;
    // </interfuscator:shuffle>
};

struct IFlowGraph;
TYPEDEF_AUTOPTR(IFlowGraph);
typedef IFlowGraph_AutoPtr IFlowGraphPtr;

struct IFlowGraphHook;
TYPEDEF_AUTOPTR(IFlowGraphHook);
typedef IFlowGraphHook_AutoPtr IFlowGraphHookPtr;

struct IFlowGraphHook
{
    enum EActivation
    {
        eFGH_Stop,
        eFGH_Pass,
        eFGH_Debugger_Input,
        eFGH_Debugger_Output,
    };
    // <interfuscator:shuffle>
    virtual ~IFlowGraphHook(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual IFlowNodePtr CreateNode(IFlowNode::SActivationInfo*, TFlowNodeTypeId typeId) = 0;
    // Description:
    //   Returns false to disallow this creation!!
    virtual bool CreatedNode(TFlowNodeId id, const char* name, TFlowNodeTypeId typeId, IFlowNodePtr pNode) = 0;
    // Description:
    //   This gets called if CreatedNode was called, and then cancelled later
    virtual void CancelCreatedNode(TFlowNodeId id, const char* name, TFlowNodeTypeId typeId, IFlowNodePtr pNode) = 0;

    virtual IFlowGraphHook::EActivation PerformActivation(IFlowGraphPtr pFlowgraph, TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const TFlowInputData* value) = 0;
    // </interfuscator:shuffle>
    void GetMemoryUsage(ICrySizer* pSizer) const{}
};
// Description:
//   Structure that permits to iterate through the node of the flowsystem.
struct IFlowNodeIterator
{
    // <interfuscator:shuffle>
    virtual ~IFlowNodeIterator(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual IFlowNodeData* Next(TFlowNodeId& id) = 0;
    // </interfuscator:shuffle>
};

// Description:
//   Structure that permits to iterate through the edge of the flowsystem.
struct IFlowEdgeIterator
{
    // Summary:
    //   Structure describing flowsystem edges.
    struct Edge
    {
        TFlowNodeId fromNodeId;     // ID of the starting node.
        TFlowPortId fromPortId;     // ID of the starting port.
        TFlowNodeId toNodeId;       // ID of the arrival node.
        TFlowPortId toPortId;       // ID of the arrival port.
    };

    // <interfuscator:shuffle>
    virtual ~IFlowEdgeIterator(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual bool Next(Edge& edge) = 0;
    // </interfuscator:shuffle>
};

TYPEDEF_AUTOPTR(IFlowNodeIterator);
typedef IFlowNodeIterator_AutoPtr IFlowNodeIteratorPtr;
TYPEDEF_AUTOPTR(IFlowEdgeIterator);
typedef IFlowEdgeIterator_AutoPtr IFlowEdgeIteratorPtr;

struct SFlowNodeActivationListener
{
    // <interfuscator:shuffle>
    virtual ~SFlowNodeActivationListener(){}
    virtual bool OnFlowNodeActivation(IFlowGraphPtr pFlowgraph, TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const char* value) = 0;
    // </interfuscator:shuffle>
};

namespace NFlowSystemUtils
{
    // Description:
    //   This class helps define IFlowGraph by declaring typed virtual
    //   functions corresponding to TFlowSystemDataTypes.
    // See also:
    //   IFlowGraph, TFlowSystemDataTypes

    template<typename T>
    struct Wrapper;

    template<>
    struct Wrapper<SFlowSystemVoid>
    {
        explicit Wrapper(const SFlowSystemVoid& v)
            : value(v) {}
        const SFlowSystemVoid& value;
    };
    template<>
    struct Wrapper<int>
    {
        explicit Wrapper(const int& v)
            : value(v) {}
        const int& value;
    };
    template<>
    struct Wrapper < float >
    {
        explicit Wrapper(const float& v)
            : value(v) {}
        const float& value;
    };
    template<>
    struct Wrapper < double >
    {
        explicit Wrapper(const double& v)
            : value(v) {}
        const double& value;
    };
    template<>
    struct Wrapper<FlowEntityId>
    {
        explicit Wrapper(const FlowEntityId& v)
            : value(v) {}
        const FlowEntityId& value;
    };
    template<>
    struct Wrapper < EntityId >
    {
        explicit Wrapper(const EntityId& v)
            : value(v) {}
        const EntityId& value;
    };
    template<>
    struct Wrapper<Vec3>
    {
        explicit Wrapper(const Vec3& v)
            : value(v) {}
        const Vec3& value;
    };
    template<>
    struct Wrapper<AZ::Vector3>
    {
        explicit Wrapper(const AZ::Vector3& v)
            : value(v) {}
        const AZ::Vector3& value;
    };
    template<>
    struct Wrapper<string>
    {
        explicit Wrapper(const string& v)
            : value(v) {}
        const string& value;
    };
    template<>
    struct Wrapper<bool>
    {
        explicit Wrapper(const bool& v)
            : value(v) {}
        const bool& value;
    };
    template<>
    struct Wrapper <FlowCustomData>
    {
        explicit Wrapper(const FlowCustomData& v)
            : value(v) {}
        const FlowCustomData& value;
    };
    template<>
    struct Wrapper<SFlowSystemPointer>
    {
        explicit Wrapper(const SFlowSystemPointer& v)
            : value(v) {}
        const SFlowSystemPointer& value;
    };


    struct IFlowSystemTyped
    {
        virtual ~IFlowSystemTyped() {}

        virtual void DoActivatePort(const SFlowAddress, const Wrapper<SFlowSystemVoid>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<int>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<float>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<double>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<EntityId>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<FlowEntityId>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<AZ::Vector3>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<Vec3>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<string>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<bool>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<FlowCustomData>& value) = 0;
        virtual void DoActivatePort(const SFlowAddress, const Wrapper<SFlowSystemPointer>& value) = 0;
    };
}

// Description:
//   Refers to NFlowSystemUtils::IFlowSystemTyped to see extra per-flow system type
//   functions that are added to this interface.
struct IFlowGraph
    : public NFlowSystemUtils::IFlowSystemTyped
{
    enum EFlowGraphType
    {
        eFGT_Default = 0,
        eFGT_AIAction,
        eFGT_Module,
        eFGT_CustomAction,
        eFGT_MaterialFx,
        eFGT_FlowGraphComponent
    };
    // <interfuscator:shuffle>
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual IFlowGraphPtr Clone() = 0;

    // Summary:
    //   Clears flow graph, deletes all nodes and edges.
    virtual void Clear() = 0;

    virtual void RegisterHook(IFlowGraphHookPtr) = 0;
    virtual void UnregisterHook(IFlowGraphHookPtr) = 0;

    virtual IFlowNodeIteratorPtr CreateNodeIterator() = 0;
    virtual IFlowEdgeIteratorPtr CreateEdgeIterator() = 0;

    // Summary:
    //   Assigns id of the default entity associated with this flow graph.
    virtual void SetGraphEntity(FlowEntityId id, int nIndex = 0) = 0;
    // Summary:
    //   Retrieves id of the default entity associated with this flow graph.
    virtual FlowEntityId GetGraphEntity(int nIndex) const = 0;

    // Summary:
    //   Enables/disables the flowgraph - used from editor.
    virtual void SetEnabled (bool bEnable) = 0;
    // Summary:
    //   Checks if flowgraph is currently enabled.
    virtual bool IsEnabled() const = 0;

    // Summary:
    //   Activates/deactivates the flowgraph - used from ComponentFlowGraph during Runtime.
    // See also:
    //   ComponentFlowGraph
    virtual void SetActive (bool bActive) = 0;
    // Summary:
    //   Checks if flowgraph is currently active.
    virtual bool IsActive() const = 0;

    virtual void UnregisterFromFlowSystem() = 0;

    virtual void SetType (IFlowGraph::EFlowGraphType type) = 0;
    virtual IFlowGraph::EFlowGraphType GetType() const = 0;

    // Primary game interface.

    //
    virtual void Update() = 0;
    virtual bool SerializeXML(const XmlNodeRef& root, bool reading) = 0;
    virtual void Serialize(TSerialize ser) = 0;
    virtual void PostSerialize() = 0;
    // Description:
    //   Similar to Update, but sends an eFE_Initialize instead of an eFE_Update
    virtual void InitializeValues() = 0;

    /**
    * Uninitializes this flowgraph by sending the eFE_Uninitialize event to all flowgraph nodes
    * any node that wishes to do shutdown related cleanup should implement a handler for
    * this event.
    */
    virtual void Uninitialize() = 0;

    // Description:
    //   Send eFE_PrecacheResources event to all flow graph nodes, so they can cache thierinternally used assets.
    virtual void PrecacheResources() = 0;

    virtual void EnsureSortedEdges() = 0;

    // Node manipulation.
    //##@{
    virtual SFlowAddress ResolveAddress(const char* addr, bool isOutput) = 0;
    virtual TFlowNodeId ResolveNode(const char* name) = 0;
    virtual TFlowNodeId CreateNode(TFlowNodeTypeId typeId, const char* name, void* pUserData = 0) = 0;
    virtual TFlowNodeId CreateNode(const char* typeName, const char* name, void* pUserData = 0) = 0; // deprecated
    virtual IFlowNodeData* GetNodeData(TFlowNodeId id) = 0;
    virtual bool SetNodeName(TFlowNodeId id, const char* sName) = 0;
    virtual const char* GetNodeName(TFlowNodeId id) = 0;
    virtual TFlowNodeTypeId GetNodeTypeId(TFlowNodeId id) = 0;
    virtual const char* GetNodeTypeName(TFlowNodeId id) = 0;
    virtual void RemoveNode(const char* name) = 0;
    virtual void RemoveNode(TFlowNodeId id) = 0;
    virtual void SetUserData(TFlowNodeId id, const XmlNodeRef& data) = 0;
    virtual XmlNodeRef GetUserData(TFlowNodeId id) = 0;
    virtual bool LinkNodes(SFlowAddress from, SFlowAddress to) = 0;
    virtual void UnlinkNodes(SFlowAddress from, SFlowAddress to) = 0;
    //##@}

    // Description:
    //   Registers hypergraph/editor listeners for visual flowgraph debugging.
    virtual void RegisterFlowNodeActivationListener(SFlowNodeActivationListener* listener) = 0;
    // Description:
    //   Removes HyperGraph/editor listeners for visual flowgraph debugging.
    virtual void RemoveFlowNodeActivationListener(SFlowNodeActivationListener* listener) = 0;

    //! HyperGraph connection
    //! Attaches this Flowgraph to the HyperGraph that is acting as the View (and controller) for this flowgraph
    virtual void SetControllingHyperGraphId(const AZ::Uuid& hypergraphId) = 0;
    virtual AZ::Uuid GetControllingHyperGraphId() const = 0;

    virtual bool NotifyFlowNodeActivationListeners(TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const char* value) = 0;

    virtual void SetEntityId(TFlowNodeId, FlowEntityId) = 0;
    virtual FlowEntityId GetEntityId(TFlowNodeId) = 0;

    virtual IFlowGraphPtr GetClonedFlowGraph() const = 0;

    // Summary:
    //   Gets node configuration.
    // Notes:
    //   Low level functions.
    virtual void GetNodeConfiguration(TFlowNodeId id, SFlowNodeConfig&) = 0;

    // node-graph interface
    virtual void SetRegularlyUpdated(TFlowNodeId, bool) = 0;
    virtual void RequestFinalActivation(TFlowNodeId) = 0;
    virtual void ActivateNode(TFlowNodeId) = 0;
    virtual void ActivatePortAny(SFlowAddress output, const TFlowInputData&) = 0;
    // Description:
    //   Special function to activate port from other dll module and pass
    //   const char* string to avoid passing string class across dll boundaries
    virtual void ActivatePortCString(SFlowAddress output, const char* cstr) = 0;

    virtual bool SetInputValue(TFlowNodeId node, TFlowPortId port, const TFlowInputData&) = 0;
    virtual bool IsOutputConnected(SFlowAddress output) = 0;
    virtual const TFlowInputData* GetInputValue(TFlowNodeId node, TFlowPortId port) = 0;
    virtual bool GetActivationInfo(const char* nodeName, IFlowNode::SActivationInfo& actInfo) = 0;

    // temporary solutions [ ask Dejan ]

    // Description:
    //   Suspended flow graphs were needed for AI Action flow graphs.
    //   Suspended flow graphs shouldn't be updated!
    //   Nodes in suspended flow graphs should ignore OnEntityEvent notifications!
    virtual void SetSuspended(bool suspend = true) = 0;
    //  Description:
    //   Checks if the flow graph is suspended.
    virtual bool IsSuspended() const = 0;

    // AI action related

    // Summary:
    //   Sets an AI Action
    virtual void SetAIAction(IAIAction* pAIAction) = 0;
    // Summary:
    //   Gets an AI Action
    virtual IAIAction* GetAIAction() const = 0;

    // Summary:
    //   Sets a Custom Action
    virtual void SetCustomAction(ICustomAction* pCustomAction) = 0;
    // Summary:
    //   Gets a Custom Action
    virtual ICustomAction* GetCustomAction() const = 0;

    virtual void GetMemoryUsage(ICrySizer* s) const = 0;
    // </interfuscator:shuffle>

    template <class T>
    ILINE void ActivatePort(const SFlowAddress output, const T& value)
    {
        DoActivatePort(output, NFlowSystemUtils::Wrapper<T>(value));
    }

    template<typename ValueType>
    ILINE void ActivateCustomDataPort(const SFlowAddress output, const ValueType& value)
    {
        DoActivatePort(output, NFlowSystemUtils::Wrapper<FlowCustomData>(FlowCustomData(value)));
    }

    template<typename ValueType>
    ILINE void ActivateCustomDataPort(const SFlowAddress output, int nPort, ValueType&& value)
    {
        DoActivatePort(output, NFlowSystemUtils::Wrapper<FlowCustomData>(FlowCustomData(std::forward<ValueType>(value))));
    }

    ILINE void ActivatePort(SFlowAddress output, const TFlowInputData& value)
    {
        ActivatePortAny(output, value);
    }

    // Graph tokens are gametokens which are unique to a particular flow graph
    struct SGraphToken
    {
        SGraphToken()
            : type(eFDT_Void) {}

        string name;
        EFlowDataTypes type;
    };
    virtual void RemoveGraphTokens() = 0;
    virtual bool AddGraphToken(const SGraphToken& token) = 0;
    virtual size_t GetGraphTokenCount() const = 0;
    virtual const IFlowGraph::SGraphToken* GetGraphToken(size_t index) const = 0;
    virtual string GetGlobalNameForGraphToken(const string& tokenName) const = 0;

    virtual TFlowGraphId GetGraphId() const = 0;
};

struct IFlowNodeFactory
{
    // <interfuscator:shuffle>
    virtual ~IFlowNodeFactory(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual IFlowNodePtr Create(IFlowNode::SActivationInfo*) = 0;
    virtual void GetMemoryUsage(ICrySizer* s) const = 0;
    virtual void Reset() = 0;
    virtual bool AllowOverride() const {return false; }
    // </interfuscator:shuffle>
};

TYPEDEF_AUTOPTR(IFlowNodeFactory);
typedef IFlowNodeFactory_AutoPtr IFlowNodeFactoryPtr;

// Description:
//   Structure that permits to iterate through the node type.
//   For each node type the ID and the name are stored.
struct IFlowNodeTypeIterator
{
    struct SNodeType
    {
        TFlowNodeTypeId     typeId;
        const char*         typeName;
    };
    // <interfuscator:shuffle>
    virtual ~IFlowNodeTypeIterator(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual bool Next(SNodeType& nodeType) = 0;
    // </interfuscator:shuffle>
};
TYPEDEF_AUTOPTR(IFlowNodeTypeIterator);
typedef IFlowNodeTypeIterator_AutoPtr IFlowNodeTypeIteratorPtr;

struct IFlowGraphInspector
{
    struct IFilter;
    TYPEDEF_AUTOPTR(IFilter);
    typedef IFilter_AutoPtr IFilterPtr;

    struct IFilter
    {
        enum EFilterResult
        {
            eFR_DontCare, // Lets others decide. [not used currently]
            eFR_Block,    // Blocks flow record.
            eFR_Pass      // Lets flow record pass.
        };
        // <interfuscator:shuffle>
        virtual ~IFilter(){}
        virtual void AddRef() = 0;
        virtual void Release() = 0;
        // Summary:
        //   Adds a new filter
        // Notes:
        //   Filter can have subfilters. (these are generally ANDed)
        virtual void AddFilter(IFilterPtr pFilter) = 0;
        // Summary:
        //   Removes the specified filter.
        virtual void RemoveFilter(IFilterPtr pFilter) = 0;
        // Summary:
        //   Applies filter(s).
        virtual IFlowGraphInspector::IFilter::EFilterResult Apply(IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to) = 0;
        // </interfuscator:shuffle>
    };

    // <interfuscator:shuffle>
    virtual ~IFlowGraphInspector(){}

    virtual void AddRef() = 0;
    virtual void Release() = 0;
    // Description:
    //   Called once per frame before IFlowSystem::Update
    // See also:
    //   IFlowSystem::Update
    virtual void PreUpdate(IFlowGraph* pGraph) = 0;
    // Description:
    //   Called once per frame after  IFlowSystem::Update
    // See also:
    //   IFlowSystem::Update
    virtual void PostUpdate(IFlowGraph* pGraph) = 0;
    virtual void NotifyFlow(IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to) = 0;
    virtual void NotifyProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo, IFlowNode* pImpl) = 0;
    virtual void AddFilter(IFlowGraphInspector::IFilterPtr pFilter) = 0;
    virtual void RemoveFilter(IFlowGraphInspector::IFilterPtr pFilter) = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    // </interfuscator:shuffle>
};

struct IFlowSystemContainer
{
    virtual ~IFlowSystemContainer() {}

    virtual void AddItem(TFlowInputData item) = 0;
    virtual void AddItemUnique(TFlowInputData item) = 0;

    virtual void RemoveItem(TFlowInputData item) = 0;

    virtual TFlowInputData GetItem(int i) = 0;
    virtual void Clear() = 0;

    virtual void RemoveItemAt(int i) = 0;
    virtual int GetItemCount() const = 0;
    virtual void GetMemoryUsage(ICrySizer* s) const = 0;
    virtual void Serialize(TSerialize ser) = 0;
};

TYPEDEF_AUTOPTR(IFlowGraphInspector);
typedef IFlowGraphInspector_AutoPtr IFlowGraphInspectorPtr;

typedef AZStd::shared_ptr<IFlowSystemContainer> IFlowSystemContainerPtr;

struct IFlowSystem
{
    // <interfuscator:shuffle>
    virtual ~IFlowSystem(){}

    virtual void Release() = 0;

    // Summary:
    //   Updates the flow system.
    // Notes:
    //   IGameFramework calls this every frame.
    virtual void Update() = 0;

    // Summary:
    //   Resets the flow system.
    virtual void Reset(bool unload) = 0;

    virtual void Enable(bool enable) = 0;
    // Summary:
    //   Reload all FlowNode templates; only in Sandbox!
    virtual void ReloadAllNodeTypes() = 0;

    // Summary:
    //   Creates a flowgraph.
    virtual IFlowGraphPtr CreateFlowGraph() = 0;

    // Summary:
    //   Type registration and resolving.
    virtual TFlowNodeTypeId RegisterType(const char* typeName, IFlowNodeFactoryPtr factory) = 0;

    // Summary:
    //   Unregister previously registered type, keeps internal type ids valid
    virtual bool UnregisterType(const char* typeName) = 0;

    // Summary:
    //   Gets a type name from a flow node type ID.
    virtual const char* GetTypeName(TFlowNodeTypeId typeId) = 0;
    // Summary:
    //   Gets a flow node type ID from a type name.
    virtual TFlowNodeTypeId GetTypeId(const char* typeName) = 0;
    // Summary:
    //   Creates an iterator to visit the node type.
    virtual IFlowNodeTypeIteratorPtr CreateNodeTypeIterator() = 0;

    // Summary:
    //   Gets a class tag from a UI name
    virtual string GetClassTagFromUIName(const string& uiName) const = 0;

    // Summary:
    //   Gets a UI Name from a class tag.
    virtual string GetUINameFromClassTag(const string& classTag) const = 0;
    // Inspecting flowsystem.

    // Summary:
    //   Registers an inspector for the flowsystem.
    virtual void RegisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph = 0) = 0;
    // Summary:
    //   Unregisters an inspector for the flowsystem.
    virtual void UnregisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph = 0) = 0;
    // Summary:
    //   Gets the default inspector for the flowsystem.
    virtual IFlowGraphInspectorPtr GetDefaultInspector() const = 0;
    // Summary:
    //   Enables/disables inspectors for the flowsystem.
    virtual void EnableInspecting(bool bEnable) = 0;
    // Summary:
    //   Returns true if inspectoring is enable, false otherwise.
    virtual bool IsInspectingEnabled() const = 0;

    // Summary:
    //  Returns the graph with the specified ID (NULL if not found)
    virtual IFlowGraph* GetGraphById(TFlowGraphId graphId) = 0;

    //  Notify the nodes that an entityId has changed on an existing entity - generates eFE_SetEntityId event
    virtual void OnEntityIdChanged(FlowEntityId oldId, FlowEntityId newId) = 0;

    // Gets memory statistics.
    virtual void GetMemoryUsage(ICrySizer* s) const = 0;

    // Gets the module manager
    virtual IFlowGraphModuleManager* GetIModuleManager() = 0;

    // Create, Delete and access flowsystem's global containers
    virtual bool CreateContainer(TFlowSystemContainerId id) = 0;
    virtual void DeleteContainer(TFlowSystemContainerId id) = 0;

    virtual IFlowSystemContainerPtr GetContainer(TFlowSystemContainerId id) = 0;

    virtual void Serialize(TSerialize ser) = 0;
    virtual TFlowGraphId RegisterGraph(IFlowGraphPtr pGraph, const char* debugName) = 0;
    virtual void UnregisterGraph(IFlowGraphPtr pGraph) = 0;

    /**
    * Uninitializes every Flowgraph being handled by the FlowSystem
    */
    virtual void Uninitialize() = 0;
    // </interfuscator:shuffle>
};

ILINE bool IsPortActive(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    return pActInfo->pInputPorts[nPort].IsUserFlagSet();
}

ILINE EFlowDataTypes GetPortType(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    return (EFlowDataTypes)pActInfo->pInputPorts[nPort].GetType();
}

ILINE const TFlowInputData& GetPortAny(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    return pActInfo->pInputPorts[nPort];
}

ILINE bool GetPortBool(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    bool* p_x = (pActInfo->pInputPorts[nPort].GetPtr<bool>());
    return (p_x != 0) ? *p_x : 0;
}

ILINE int GetPortInt(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    int* intPtr = pActInfo->pInputPorts[nPort].GetPtr<int>();
    if (intPtr)
    {
        return *intPtr;
    }
    else
    {
        return 0;
    }
}

ILINE FlowEntityId GetPortFlowEntityId(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    FlowEntityId* idPointer = pActInfo->pInputPorts[nPort].GetPtr<FlowEntityId>();
    if (idPointer)
    {
        return *idPointer;
    }
    else
    {
        return FlowEntityId();
    }
}

ILINE FlowEntityId GetPortEntityId(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    return GetPortFlowEntityId(pActInfo, nPort);
}

ILINE float GetPortFloat(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    float* floatPtr = pActInfo->pInputPorts[nPort].GetPtr<float>();
    if (floatPtr)
    {
        return *floatPtr;
    }
    else
    {
        return 0.0f;
    }
}

ILINE double GetPortDouble(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    double* doublePtr = pActInfo->pInputPorts[nPort].GetPtr<double>();
    if (doublePtr)
    {
        return *doublePtr;
    }
    else
    {
        return 0.0;
    }
}

ILINE Vec3 GetPortVec3(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    Vec3* vectorPtr = pActInfo->pInputPorts[nPort].GetPtr<Vec3>();
    if (vectorPtr)
    {
        return *vectorPtr;
    }
    else
    {
        return Vec3();
    }
}

ILINE const string& GetPortString(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    const string* p_x = (pActInfo->pInputPorts[nPort].GetPtr<string>());

    if (p_x)
    {
        return *p_x;
    }
    else
    {
        const static string empty ("");
        return empty;
    }
}

template<typename ValueType>
ILINE const ValueType& GetPortCustomData(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    const FlowCustomData* p_x = (pActInfo->pInputPorts[nPort].GetPtr<FlowCustomData>());

    if (p_x)
    {
        return p_x->Get<ValueType>();
    }
    else
    {
        const static ValueType empty;
        return empty;
    }
}

ILINE void* GetPortPointer(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    void* x = pActInfo->pInputPorts[nPort].GetPtr<SFlowSystemPointer>()->m_pointer;
    return x;
}

ILINE bool IsBoolPortActive(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    if (IsPortActive(pActInfo, nPort) && GetPortBool(pActInfo, nPort))
    {
        return true;
    }
    else
    {
        return false;
    }
}

template <class T>
ILINE void ActivateOutput(IFlowNode::SActivationInfo* pActInfo, int nPort, const T& value)
{
    uint8 portId = aznumeric_caster(nPort);
    SFlowAddress addr(pActInfo->myID, portId, true);
    pActInfo->pGraph->ActivatePort(addr, value);
}

template<typename ValueType>
ILINE void ActivateCustomDataOutput(IFlowNode::SActivationInfo* pActInfo, int nPort, const ValueType& value)
{
    uint8 portId = aznumeric_caster(nPort);
    SFlowAddress addr(pActInfo->myID, portId, true);
    pActInfo->pGraph->ActivatePort(addr, FlowCustomData(value));
}

template<typename ValueType>
ILINE void ActivateCustomDataOutput(IFlowNode::SActivationInfo* pActInfo, int nPort, ValueType&& value)
{
    uint8 portId = aznumeric_caster(nPort);
    SFlowAddress addr(pActInfo->myID, portId, true);
    pActInfo->pGraph->ActivatePort(addr, FlowCustomData(std::forward<ValueType>(value)));
}

ILINE bool IsOutputConnected(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
    uint8 portId = aznumeric_caster(nPort);
    SFlowAddress addr(pActInfo->myID, portId, true);
    return pActInfo->pGraph->IsOutputConnected(addr);
}

#endif // CRYINCLUDE_CRYCOMMON_IFLOWSYSTEM_H
