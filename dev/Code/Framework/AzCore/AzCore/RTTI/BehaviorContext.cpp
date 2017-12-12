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

namespace AZ
{
    //=========================================================================
    // BehaviorMethod
    //=========================================================================
    BehaviorMethod::BehaviorMethod()
        : m_debugDescription(nullptr)
        , m_defaultArguments(nullptr)
    {
    }

    //=========================================================================
    // ~BehaviorMethod
    //=========================================================================
    BehaviorMethod::~BehaviorMethod()
    {
        delete m_defaultArguments;

        for (auto attrIt : m_attributes)
        {
            delete attrIt.second;
        }
    }

    //=========================================================================
    // GetMinNumberOfArguments
    //=========================================================================
    size_t BehaviorMethod::GetMinNumberOfArguments() const
    {
        return GetNumArguments() - (m_defaultArguments ? m_defaultArguments->GetNumValues() : 0);
    }

    //=========================================================================
    // BehaviorProperty
    //=========================================================================
    BehaviorProperty::BehaviorProperty()
        : m_getter(nullptr)
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
        for (auto propertyIt : m_events)
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

        for (auto attrIt : m_attributes)
        {
            delete attrIt.second;
        }

        delete m_createHandler;
        delete m_destroyHandler;

        delete m_queueFunction;
        delete m_getCurrentId;
    }

    //=========================================================================
    // BehaviorContext::GlobalMethodInfo
    //=========================================================================
    BehaviorContext::GlobalMethodInfo::GlobalMethodInfo(BehaviorContext* context, const char* methodName, BehaviorMethod* method)
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
    // BehaviorContext::~GlobalMethodInfo
    //=========================================================================
    BehaviorContext::GlobalMethodInfo::~GlobalMethodInfo()
    {
        // process all on demand queued reflections
        decltype (Base::m_context->m_toProcessOnDemandReflection) toProcess(AZStd::move(Base::m_context->m_toProcessOnDemandReflection));
        for (auto toReflectIt : toProcess)
        {
            toReflectIt.second(Base::m_context);
        }

        if (m_method)
        {
            BehaviorContextBus::Event(m_context, &BehaviorContextBus::Events::OnAddGlobalMethod, m_name, m_method);
        }
    }

    //=========================================================================
    // BehaviorContext::GlobalMethodInfo::operator->
    //=========================================================================
    BehaviorContext::GlobalMethodInfo* BehaviorContext::GlobalMethodInfo::operator->()
    {
        return this;
    }
    
    //=========================================================================
    // BehaviorContext::GlobalPropertyInfo
    //=========================================================================
    BehaviorContext::GlobalPropertyInfo::GlobalPropertyInfo(BehaviorContext* context, BehaviorProperty* prop)
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
    BehaviorContext::GlobalPropertyInfo::~GlobalPropertyInfo()
    {
        // process all on demand queued reflections
        decltype (Base::m_context->m_toProcessOnDemandReflection) toProcess(AZStd::move(Base::m_context->m_toProcessOnDemandReflection));
        for (auto toReflectIt : toProcess)
        {
            toReflectIt.second(Base::m_context);
        }

        if (m_prop)
        {
            BehaviorContextBus::Event(m_context, &BehaviorContextBus::Events::OnAddGlobalProperty, m_prop->m_name.c_str(), m_prop);
        }
    }

    //=========================================================================
    // BehaviorContext::GlobalPropertyInfo::operator->
    //=========================================================================
    BehaviorContext::GlobalPropertyInfo* BehaviorContext::GlobalPropertyInfo::operator->()
    {
        return this;
    }

    //=========================================================================
    // BehaviorContext
    //=========================================================================
    BehaviorContext::BehaviorContext()
    {        
    }

    //=========================================================================
    // ~BehaviorContext
    //=========================================================================
    BehaviorContext::~BehaviorContext()
    {
        for (auto methodIt : m_methods)
        {
            delete methodIt.second;
        }

        for (BehaviorMethod* method : m_attributeMethods)
        {
            delete method;
        }

        for (auto propertyIt : m_properties)
        {
            delete propertyIt.second;
        }

        for (auto classIt : m_classes)
        {
            delete classIt.second;
        }

        for (auto ebusIt : m_ebuses)
        {
            delete ebusIt.second;
        }
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
        for (BehaviorMethod* constructor : m_constructors)
        {
            delete constructor;
        }

        for (auto methodIt : m_methods)
        {
            delete methodIt.second;
        }

        for (BehaviorMethod* method : m_attributeMethods)
        {
            delete method;
        }

        for (auto propertyIt : m_properties)
        {
            delete propertyIt.second;
        }

        for (auto attrIt : m_attributes)
        {
            delete attrIt.second;
        }
    }

    //=========================================================================
    // Create
    //=========================================================================
    BehaviorObject BehaviorClass::Create() const
    {
        BehaviorObject result;
        if (m_defaultConstructor)
        {
            result.m_address = Allocate(); 
            if (result.m_address)
            {
                m_defaultConstructor(result.m_address, m_userData);
            }
            result.m_typeId = m_typeId;
        }
        return result;
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

    namespace BehaviorContextHelper
    {
        AZ::BehaviorClass* GetClass(BehaviorContext* behaviorContext, const AZ::Uuid& id)
        {
            const auto& classIterator = behaviorContext->m_typeToClassMap.find(id);
            if (classIterator != behaviorContext->m_typeToClassMap.end())
            {
                return classIterator->second;
            }
            return nullptr;
        }
    }

} // namespace AZ

#endif // AZ_UNITY_BUILD