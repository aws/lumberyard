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

#include "Core.h"

#include <ScriptCanvas/Libraries/Libraries.h>
#include <Data/DataMacros.h>
#include <Data/DataTrait.h>
#include <ScriptCanvas/Core/Attributes.h>

namespace CoreCPP
{

    template<typename t_Type>
    struct BehaviorClassReflection
    {
        AZ_TYPE_INFO((BehaviorClassReflection<t_Type>), "{0EADF8F5-8AB8-42E9-9C50-F5C78255C817}", t_Type);
    };

    template<typename t_Type, bool isHashable = ScriptCanvas::Data::Traits<t_Type>::s_isKey>
    struct HashContainerReflector
    {
        static void Reflect(AZ::ReflectContext*)
        {
        }
    };

#define SCRIPT_CANVAS_REFLECT_SERIALIZATION_KEY_VALUE_TYPE(VALUE_TYPE, CONTAINER_TYPE)\
        if (auto genericClassInfo = AZ::SerializeGenericTypeInfo< CONTAINER_TYPE <t_Type, VALUE_TYPE##Type>>::GetGenericInfo())\
        {\
            genericClassInfo->Reflect(serializeContext);\
        }

#define SCRIPT_CANVAS_REFLECT_BEHAVIOR_KEY_VALUE_TYPE(VALUE_TYPE, CONTAINER_TYPE)\
        ->Method("Reflect" #VALUE_TYPE #CONTAINER_TYPE "Func", [](const CONTAINER_TYPE < t_Type, VALUE_TYPE##Type >&){})

    template<typename t_Type>
    struct HashContainerReflector<t_Type, true>
    {
        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            using namespace ScriptCanvas::Data;
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_set<t_Type>>::GetGenericInfo())
                {
                    genericClassInfo->Reflect(serializeContext);
                }

                SCRIPT_CANVAS_PER_DATA_TYPE_1(SCRIPT_CANVAS_REFLECT_SERIALIZATION_KEY_VALUE_TYPE, AZStd::unordered_map);
            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorContext, azrtti_typeid<BehaviorClassReflection<t_Type>>());
                AZ::BehaviorContext::ClassBuilder<BehaviorClassReflection<t_Type>> classBuilder (behaviorContext, behaviorClass);
                classBuilder->Method("ReflectSet", [](const AZStd::unordered_set<t_Type>&) {})
                    SCRIPT_CANVAS_PER_DATA_TYPE_1(SCRIPT_CANVAS_REFLECT_BEHAVIOR_KEY_VALUE_TYPE, AZStd::unordered_map)
                    ;
            }
        }
    };
#undef SCRIPT_CANVAS_REFLECT_SERIALIZATION_KEY_VALUE_TYPE
#undef SCRIPT_CANVAS_REFLECT_BEHAVIOR_KEY_VALUE_TYPE

    template<typename t_Type>
    struct TraitsReflector
    {
        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<t_Type>>::GetGenericInfo())
                {
                    genericClassInfo->Reflect(serializeContext);
                }
            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                behaviorContext->Class<BehaviorClassReflection<t_Type>>(AZStd::string::format("ReflectOnDemandTargets_%s", ScriptCanvas::Data::Traits<t_Type>::GetName().data()).data())
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Ignore, true)
                    ->Method("ReflectVector", [](const AZStd::vector<t_Type>&) {})
                    ;
            }

            HashContainerReflector<t_Type>::Reflect(reflectContext);
        }
    };


    #define SCRIPT_CANVAS_CALL_REFLECT_ON_TRAITS(TYPE)\
        TraitsReflector<TYPE##Type>::Reflect(reflectContext);

    using namespace AZStd;
    using namespace ScriptCanvas;

    // use this to reflect on demand reflection targets in the appropriate place
    class ReflectOnDemandTargets
    {

    public:
        AZ_TYPE_INFO(ReflectOnDemandTargets, "{FE658DB8-8F68-4E05-971A-97F398453B92}");
        AZ_CLASS_ALLOCATOR(ReflectOnDemandTargets, AZ::SystemAllocator, 0);

        ReflectOnDemandTargets() = default;
        ~ReflectOnDemandTargets() = default;

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            using namespace ScriptCanvas::Data;
            SCRIPT_CANVAS_PER_DATA_TYPE(SCRIPT_CANVAS_CALL_REFLECT_ON_TRAITS);
        }
    };

    #undef SCRIPT_CANVAS_CALL_REFLECT_ON_TRAITS

} // namespace CoreCPP

namespace ScriptCanvas
{
    namespace Library
    {
        void Core::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<Core, LibraryDefinition>()
                    ->Version(1)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Core>("Core", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Libraries/Core.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".time")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "TimeNodeTitlePalette")
                        ;
                }
            }

            Nodes::Core::EBusEventEntry::Reflect(reflection);
            Nodes::Core::Internal::ScriptEventEntry::Reflect(reflection);
            Nodes::Core::Internal::ScriptEventBase::Reflect(reflection);

            CoreCPP::ReflectOnDemandTargets::Reflect(reflection);

        }

        void Core::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Core;
            AddNodeToRegistry<Core, Assign>(nodeRegistry);
            AddNodeToRegistry<Core, Error>(nodeRegistry);
            AddNodeToRegistry<Core, ErrorHandler>(nodeRegistry);
            AddNodeToRegistry<Core, Method>(nodeRegistry);
            AddNodeToRegistry<Core, BehaviorContextObjectNode>(nodeRegistry);
            AddNodeToRegistry<Core, Start>(nodeRegistry);            
            AddNodeToRegistry<Core, ScriptCanvas::Nodes::Core::String>(nodeRegistry);
            AddNodeToRegistry<Core, EBusEventHandler>(nodeRegistry);
            AddNodeToRegistry<Core, ExtractProperty>(nodeRegistry);
            AddNodeToRegistry<Core, ForEach>(nodeRegistry);
            AddNodeToRegistry<Core, GetVariableNode>(nodeRegistry);
            AddNodeToRegistry<Core, SetVariableNode>(nodeRegistry);
            AddNodeToRegistry<Core, ReceiveScriptEvent>(nodeRegistry);
            AddNodeToRegistry<Core, SendScriptEvent>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Core::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Core::Assign::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Error::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ErrorHandler::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Method::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::BehaviorContextObjectNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Start::CreateDescriptor(),                
                ScriptCanvas::Nodes::Core::String::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::EBusEventHandler::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ExtractProperty::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ForEach::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::GetVariableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::SetVariableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ReceiveScriptEvent::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::SendScriptEvent::CreateDescriptor(),
            });
        }
    }
}
