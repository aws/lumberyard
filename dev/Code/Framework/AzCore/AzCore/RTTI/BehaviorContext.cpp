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

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/EntityBus.h>

namespace AZ
{
    //=========================================================================
    // BehaviorMethod
    //=========================================================================
    BehaviorMethod::BehaviorMethod(BehaviorContext* context)
        : OnDemandReflectionOwner(*context)
        , m_debugDescription(nullptr)
    {
    }

    //=========================================================================
    // ~BehaviorMethod
    //=========================================================================
    BehaviorMethod::~BehaviorMethod()
    {
        for (auto attrIt : m_attributes)
        {
            delete attrIt.second;
        }
        m_attributes.clear();
    }

    //=========================================================================
    // BehaviorProperty
    //=========================================================================
    BehaviorProperty::BehaviorProperty(BehaviorContext* context)
        : OnDemandReflectionOwner(*context)
        , m_getter(nullptr)
        , m_setter(nullptr)
    {
    }
    
    //=========================================================================
    // ~BehaviorProperty
    //=========================================================================
    BehaviorProperty::~BehaviorProperty()
    {
        delete m_getter;
        delete m_setter;

        for (auto attrIt : m_attributes)
        {
            delete attrIt.second;
        }
    }

    //=========================================================================
    // GetTypeId
    //=========================================================================
    const AZ::Uuid& BehaviorProperty::GetTypeId() const
    {
        if (m_getter)
        {
            // if we have result, we validate on reflection that we have a result, so no need to check.
            return m_getter->GetResult()->m_typeId;
        }
        else
        {
            // if the property is Write only we should have a setter and the setter last argument is the property type
            return m_setter->GetArgument(m_setter->GetNumArguments() - 1)->m_typeId;
        }        
    }

    //=========================================================================
    // BehaviorEBus
    //=========================================================================
    BehaviorEBus::BehaviorEBus()
        : m_createHandler(nullptr)
        , m_destroyHandler(nullptr)
        , m_queueFunction(nullptr)
        , m_getCurrentId(nullptr)
    {
        m_idParam.m_name = "BusIdType";
        m_idParam.m_typeId = AZ::Uuid::CreateNull();
        m_idParam.m_traits = BehaviorParameter::TR_REFERENCE;
        m_idParam.m_azRtti = nullptr;
    }

    //=========================================================================
    // ~BehaviorEBus
    //=========================================================================
    BehaviorEBus::~BehaviorEBus()
    {
        // Clear all lists of things now to prevent double deleting
        // (if they're found in the lists later, they'll be deleted again)
        auto events = AZStd::move(m_events);
        auto attributes = AZStd::move(m_attributes);

        // Actually delete everything
        for (auto propertyIt : events)
        {
            delete propertyIt.second.m_broadcast;
            delete propertyIt.second.m_event;
            delete propertyIt.second.m_queueBroadcast;
            delete propertyIt.second.m_queueEvent;
            for (auto attrIt : propertyIt.second.m_attributes)
            {
                delete attrIt.second;
            }
        }

        for (auto attrIt : attributes)
        {
            delete attrIt.second;
        }

        delete m_createHandler;
        delete m_destroyHandler;

        delete m_queueFunction;
        delete m_getCurrentId;
    }

    //=========================================================================
    // BehaviorContext::GlobalMethodBuilder
    //=========================================================================
    BehaviorContext::GlobalMethodBuilder::GlobalMethodBuilder(BehaviorContext* context, const char* methodName, BehaviorMethod* method)
        : Base::GenericAttributes(context)
        , m_name(methodName)
        , m_method(method)
    {
        if (method)
        {
            m_currentAttributes = &method->m_attributes;
        }
    }

    //=========================================================================
    // BehaviorContext::~GlobalMethodBuilder
    //=========================================================================
    BehaviorContext::GlobalMethodBuilder::~GlobalMethodBuilder()
    {
        // process all on demand queued reflections
        m_context->ExecuteQueuedOnDemandReflections();

        if (m_method)
        {
            BehaviorContextBus::Event(m_context, &BehaviorContextBus::Events::OnAddGlobalMethod, m_name, m_method);
        }
    }

    //=========================================================================
    // BehaviorContext::GlobalMethodBuilder::operator->
    //=========================================================================
    BehaviorContext::GlobalMethodBuilder* BehaviorContext::GlobalMethodBuilder::operator->()
    {
        return this;
    }
    
    //=========================================================================
    // BehaviorContext::GlobalPropertyBuilder
    //=========================================================================
    BehaviorContext::GlobalPropertyBuilder::GlobalPropertyBuilder(BehaviorContext* context, BehaviorProperty* prop)
        : Base::GenericAttributes(context)
        , m_prop(prop)
    {
        if (prop)
        {
            m_currentAttributes = &prop->m_attributes;
        }
    }

    //=========================================================================
    // BehaviorContext::~PropertyInfo
    //=========================================================================
    BehaviorContext::GlobalPropertyBuilder::~GlobalPropertyBuilder()
    {
        // process all on demand queued reflections
        m_context->ExecuteQueuedOnDemandReflections();

        if (m_prop)
        {
            BehaviorContextBus::Event(m_context, &BehaviorContextBus::Events::OnAddGlobalProperty, m_prop->m_name.c_str(), m_prop);
        }
    }

    //=========================================================================
    // BehaviorContext::GlobalPropertyBuilder::operator->
    //=========================================================================
    BehaviorContext::GlobalPropertyBuilder* BehaviorContext::GlobalPropertyBuilder::operator->()
    {
        return this;
    }

    //=========================================================================
    // BehaviorContext
    //=========================================================================
    BehaviorContext::BehaviorContext() = default;

    //=========================================================================
    // ~BehaviorContext
    //=========================================================================
    BehaviorContext::~BehaviorContext()
    {
        // Clear all lists of things now to prevent double deleting
        // (if they're found in the lists later, they'll be deleted again)
        auto methods = AZStd::move(m_methods);
        auto properties = AZStd::move(m_properties);
        auto classes = AZStd::move(m_classes);
        auto ebuses = AZStd::move(m_ebuses);
        m_typeToClassMap.clear();

        // Actually delete everything
        for (auto methodIt : methods)
        {
            delete methodIt.second;
        }

        for (auto propertyIt : properties)
        {
            delete propertyIt.second;
        }

        for (auto classIt : classes)
        {
            delete classIt.second;
        }

        for (auto ebusIt : ebuses)
        {
            delete ebusIt.second;
        }
    }

    bool BehaviorContext::IsTypeReflected(AZ::Uuid typeId) const
    {
        auto classTypeIt = m_typeToClassMap.find(typeId);
        return (classTypeIt != m_typeToClassMap.end());
    }

    //=========================================================================
    // BehaviorClass
    //=========================================================================
    BehaviorClass::BehaviorClass()
        : m_allocate(nullptr)
        , m_deallocate(nullptr)
        , m_defaultConstructor(nullptr)
        , m_destructor(nullptr)
        , m_cloner(nullptr)
        , m_equalityComparer(nullptr)
        , m_userData(nullptr)
        , m_typeId(Uuid::CreateNull())
        , m_alignment(0)
        , m_size(0)
        , m_unwrapper(nullptr)
        , m_unwrapperUserData(nullptr)
        , m_wrappedTypeId(Uuid::CreateNull())
    {
    }

    //=========================================================================
    // ~BehaviorClass
    //=========================================================================
    BehaviorClass::~BehaviorClass()
    {
        // Clear all lists of things now to prevent double deleting
        // (if they're found in the lists later, they'll be deleted again)
        auto constructors = AZStd::move(m_constructors);
        auto methods = AZStd::move(m_methods);
        auto properties = AZStd::move(m_properties);
        auto attributes = AZStd::move(m_attributes);

        // Actually delete everything
        for (BehaviorMethod* constructor : constructors)
        {
            delete constructor;
        }

        for (auto methodIt : methods)
        {
            delete methodIt.second;
        }

        for (auto propertyIt : properties)
        {
            delete propertyIt.second;
        }

        for (auto attrIt : attributes)
        {
            delete attrIt.second;
        }
    }

    //=========================================================================
    // Create
    //=========================================================================
    BehaviorObject BehaviorClass::Create() const
    {
        return Create(Allocate());
    }
    
    BehaviorObject BehaviorClass::Create(void* address) const
    {
        if (m_defaultConstructor && address)
        {
            m_defaultConstructor(address, m_userData);
        }

        return BehaviorObject(address, m_typeId);
    }

    //=========================================================================
    // Clone
    //=========================================================================
    BehaviorObject BehaviorClass::Clone(const BehaviorObject& object) const 
    {
        BehaviorObject result;
        if (m_cloner && object.m_typeId == m_typeId)
        {
            result.m_address = Allocate();
            if (result.m_address)
            {
                m_cloner(result.m_address, object.m_address, m_userData);
            }
            result.m_typeId = m_typeId;
        }
        return result;
    }

    //=========================================================================
    // Move
    //=========================================================================
    BehaviorObject BehaviorClass::Move(BehaviorObject&& object) const
    {
        BehaviorObject result;
        if (m_mover && object.m_typeId == m_typeId)
        {
            result.m_address = Allocate();
            if (result.m_address)
            {
                m_mover(result.m_address, object.m_address, m_userData);
                Destroy(object);
            }
            result.m_typeId = m_typeId;
        }
        return result;
    }

    //=========================================================================
    // Destroy
    //=========================================================================
    void BehaviorClass::Destroy(const BehaviorObject& object) const
    {
        if (object.m_typeId == m_typeId && m_destructor)
        {
            m_destructor(object.m_address, m_userData);
            Deallocate(object.m_address);
        }
    }

    //=========================================================================
    // Allocate
    //=========================================================================
    void* BehaviorClass::Allocate() const
    {
        if (m_allocate)
        {
            return m_allocate(m_userData);
        }
        else
        {
            return azmalloc(m_size, m_alignment, AZ::SystemAllocator, m_name.c_str());
        }
    }
    
    //=========================================================================
    // Deallocate
    //=========================================================================
    void  BehaviorClass::Deallocate(void* address) const
    {
        if (address == nullptr)
        {
            return;
        }

        if (m_deallocate)
        {
            m_deallocate(address, m_userData);
        }
        else
        {
            azfree(address, AZ::SystemAllocator, m_size, m_alignment);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    // InstallGenericHook
    //=========================================================================
    bool BehaviorEBusHandler::InstallGenericHook(int index, GenericHookType hook, void* userData)
    {
        if (index != -1)
        {
            // Check parameters
            m_events[index].m_isFunctionGeneric = true;
            m_events[index].m_function = reinterpret_cast<void*>(hook);
            m_events[index].m_userData = userData;
            return true;
        }

        return false;
    }

    //=========================================================================
    // InstallGenericHook
    //=========================================================================
    bool BehaviorEBusHandler::InstallGenericHook(const char* name, GenericHookType hook, void* userData)
    {
        return InstallGenericHook(GetFunctionIndex(name), hook, userData);
    }

    //=========================================================================
    // GetEvents
    //=========================================================================
    const BehaviorEBusHandler::EventArray& BehaviorEBusHandler::GetEvents() const
    {
        return m_events;
    }

    bool BehaviorEBusHandler::BusForwarderEvent::HasResult() const
    {
        return !m_parameters.empty() && !m_parameters.front().m_typeId.IsNull() && m_parameters.front().m_typeId != azrtti_typeid<void>();
    }

    namespace BehaviorContextHelper
    {
        AZ::BehaviorClass* GetClass(BehaviorContext* behaviorContext, const AZ::TypeId& id)
        {
            const auto& classIterator = behaviorContext->m_typeToClassMap.find(id);
            if (classIterator != behaviorContext->m_typeToClassMap.end())
            {
                return classIterator->second;
            }
            return nullptr;
        }

        const BehaviorClass* GetClass(const AZStd::string& classNameString)
        {
            const char* className = classNameString.c_str();
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("Behavior Context", false, "A behavior context is required!");
                return nullptr;
            }

            const auto classIter(behaviorContext->m_classes.find(className));
            if (classIter == behaviorContext->m_classes.end())
            {
                AZ_Warning("Behavior Context", false, "No class by name of %s in the behavior context!", className);
                return nullptr;
            }

            AZ_Assert(classIter->second, "BehaviorContext Class entry %s has no class pointer", className);
            return classIter->second;
        }

        const BehaviorClass* GetClass(const AZ::TypeId& typeID)
        {
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("Behavior Context", false, "A behavior context is required!");
                return nullptr;
            }

            const auto classIter(behaviorContext->m_typeToClassMap.find(typeID));
            if (classIter == behaviorContext->m_typeToClassMap.end())
            {
                AZ_Warning("Behavior Context", false, "No class by typeID of %s in the behavior context!", typeID.ToString<AZStd::string>().c_str());
                return nullptr;
            }

            AZ_Assert(classIter->second, "BehaviorContext class by typeID %s is nullptr in the behavior context!", typeID.ToString<AZStd::string>().c_str());
            return classIter->second;
        }
    
        AZ::TypeId GetClassType(const AZStd::string& classNameString)
        {
            const char* className = classNameString.c_str();
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("Behavior Context", false, "A behavior context is required!");
                return AZ::TypeId::CreateNull();
            }

            const auto classIter(behaviorContext->m_classes.find(className));
            if (classIter == behaviorContext->m_classes.end())
            {
                AZ_Error("Behavior Context", false, "No class by name of %s in the behavior context!", className);
                return AZ::TypeId::CreateNull();
            }

            AZ::BehaviorClass* behaviorClass(classIter->second);
            AZ_Assert(behaviorClass, "BehaviorContext Class entry %s has no class pointer", className);
            return behaviorClass->m_typeId;
        }

        bool IsStringParameter(const BehaviorParameter& parameter)
        {
            return (parameter.m_traits & AZ::BehaviorParameter::TR_STRING) == AZ::BehaviorParameter::TR_STRING;
        }
    }
 
} // namespace AZ

#endif // AZ_UNITY_BUILD
