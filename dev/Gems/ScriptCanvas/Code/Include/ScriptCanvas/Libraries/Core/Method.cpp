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

#include "precompiled.h"

#include "Method.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <Libraries/Core/MethodUtility.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            void Method::OnInputSignal(const SlotId&)
            {
                AZ_Error("Script Canvas", m_method, "Method node called with no initialized method!");
                
                if (m_method)
                {
                    AZStd::array<AZ::BehaviorValueParameter, BehaviorContextMethodHelper::MaxCount> params;
                    AZ::BehaviorValueParameter* paramFirst(params.begin());
                    AZ::BehaviorValueParameter* paramIter = paramFirst;
                    {
                        // all input should have been pushed into this node already
                        int argIndex(0);
                        for (Datum& input : m_inputData)
                        {
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> inputParameter = input.ToBehaviorValueParameter(*m_method->GetArgument(argIndex));
                            if (!inputParameter.IsSuccess())
                            {
                                SCRIPTCANVAS_REPORT_ERROR((*this), "BehaviorContext method input problem at parameter index %d: %s", argIndex, inputParameter.GetError().data());
                                return;
                            }

                            paramIter->Set(inputParameter.GetValue());
                            ++paramIter;
                            ++argIndex;
                        }

                        BehaviorContextMethodHelper::Call(*this, m_method, paramFirst, paramIter, m_resultSlotID);
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }

            void Method::InitializeMethod(AZ::BehaviorMethod& method, const AZStd::string* inputNameOverride)
            {
                ConfigureMethod(method);

                if (method.HasResult())
                {
                    if (const AZ::BehaviorParameter* result = method.GetResult())
                    {
                        Data::Type outputType(AZ::BehaviorContextHelper::IsStringParameter(*result) ? Data::Type::String() : Data::FromBehaviorContextType(result->m_typeId));
                        // multiple outs will need out value names
                        const AZStd::string resultSlotName(AZStd::string::format("Result: %s", Data::GetName(outputType)));
                        m_resultSlotID = AddOutputTypeSlot(resultSlotName.c_str(), "", AZStd::move(outputType), OutputStorage::Required);
                    }
                }

                // input slots
                AZ_Error("Script Canvas", method.GetNumArguments() <= BehaviorContextMethodHelper::MaxCount, "Too many arguments for a Script Canvas method");

                const AZ::BehaviorParameter* busIdArgument = method.GetBusIdArgument();

                for (size_t argIndex(0), sentinel(method.GetNumArguments()); argIndex != sentinel; ++argIndex)
                {
                    if (const AZ::BehaviorParameter* argument = method.GetArgument(argIndex))
                    {
                        const char* argumentTypeName = Data::GetName(AZ::BehaviorContextHelper::IsStringParameter(*argument) ? Data::Type::String() : Data::FromBehaviorContextType(argument->m_typeId));
                        const AZStd::string* argumentNamePtr = method.GetArgumentName(argIndex);
                        const AZStd::string argName = argumentNamePtr && !argumentNamePtr->empty()
                            ? *argumentNamePtr
                            : (AZStd::string::format("%s:%2d", argumentTypeName, argIndex));

                        const AZStd::string* argumentTooltipPtr = method.GetArgumentToolTip(argIndex);
                        const AZStd::string_view argumentTooltip = argumentTooltipPtr ? argumentTooltipPtr->c_str() : AZStd::string_view();

                        // Create a slot with a default value
                        if (argument->m_typeId == azrtti_typeid<AZ::EntityId>())
                        {
                            AddInputDatumSlot(argName, argumentTooltip, Datum::eOriginality::Copy, ScriptCanvas::SelfReferenceId, false);
                        }
                        else
                        {
                            SlotId addedSlot = AddInputDatumSlot(argName, argumentTooltip, *argument, Datum::eOriginality::Copy, false);

                            if (const AZ::BehaviorDefaultValue* defaultValue = method.GetDefaultValue(argIndex))
                            {
                                Datum* input = ModInput(*this, addedSlot);

                                if (input && Data::IsValueType(input->GetType()))
                                {
                                    *input = Datum::CreateFromBehaviorContextValue(defaultValue->m_value);
                                }
                            }
                        }
                    }
                }

                AddSlot("In", "", SlotType::ExecutionIn);
                AddSlot("Out", "", SlotType::ExecutionOut);
            }

            void Method::InitializeClassOrBus(const Namespaces& namespaces, const AZStd::string& className, const AZStd::string& methodName)
            {
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "Can't create the ebus sender without a behavior context!");
                    return;
                }

                const auto& ebusIterator = behaviorContext->m_ebuses.find(className);
                if (ebusIterator == behaviorContext->m_ebuses.end())
                {
                    InitializeClass(namespaces, className, methodName);
                }
                else
                {
                    InitializeEvent(namespaces, className, methodName);
                }
            }

            void Method::InitializeClass(const Namespaces& namespaces, const AZStd::string& className, const AZStd::string& methodName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
               
                AZ::BehaviorMethod* method{};
                if (FindClass(method, namespaces, className, methodName))
                {
                    InitializeMethod(*method);
                    InitializeLookUp(namespaces, className, methodName, MethodType::Member);
                }
            }

            void Method::InitializeEvent(const Namespaces& namespaces, const AZStd::string& ebusName, const AZStd::string& eventName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                AZ::BehaviorMethod* method{};
                if (FindEvent(method, namespaces, ebusName, eventName))
                {
                    InitializeMethod(*method, &eventName);
                    InitializeLookUp(namespaces, ebusName, eventName, MethodType::Event, &eventName);
                }
            }

            void Method::InitializeFree(const Namespaces& namespaces, const AZStd::string& methodName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
               
                AZ::BehaviorMethod* method{};
                if (FindFree(method, namespaces, methodName))
                {
                    InitializeMethod(*method);
                    InitializeLookUp(namespaces, m_className, methodName, MethodType::Free);
                }
            }

            void Method::InitializeLookUp(const Namespaces& namespaces, const AZStd::string& className, const AZStd::string& methodName, const MethodType methodType, const AZStd::string* nameOverride)
            {
                m_namespaces = namespaces;
                m_className = className;
                m_methodName = methodName;
                m_methodType = methodType;
                m_name = nameOverride ? *nameOverride : m_methodName;
            }

            void Method::ConfigureMethod(AZ::BehaviorMethod& method)
            {
                if (IsConfigured())
                {
                    return;
                }

                m_method = &method;
            }

            bool Method::FindClass(AZ::BehaviorMethod*& outMethod, const Namespaces& namespaces, AZStd::string_view className, AZStd::string_view methodName)
            {
                AZ::BehaviorContext* behaviorContext(nullptr);
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "A behavior context is required!");
                    return false;
                }

                auto classIter(behaviorContext->m_classes.find(className));
                if (classIter == behaviorContext->m_classes.end())
                {
                    AZ_Error("Script Canvas", false, "No class by name of %s in the behavior context!", className.data());
                    return false;
                }

                AZ::BehaviorClass* behaviorClass(classIter->second);
                AZ_Assert(behaviorClass, "BehaviorContext Class entry %s has no class pointer", className.data());

                const auto methodIter(behaviorClass->m_methods.find(methodName.data()));
                if (methodIter == behaviorClass->m_methods.end())
                {
                    AZ_Error("Script Canvas", false, "No method by name of %s in BehaviorContext class %s", methodName.data(), className.data());
                    return false;
                }

                // this argument is the first argument...so perhaps remove the distinction between class and member functions, since it probably won't follow polymorphism
                // if it will, keep the distinction, and add the first argument separately
                AZ::BehaviorMethod* method(methodIter->second);
                if (!method)
                {
                    AZ_Error("Script Canvas", false, "BehaviorContext Method entry %s has no method pointer", methodName.data());
                    return false;
                }

                outMethod = method;
                return true;
            }

            bool Method::FindEvent(AZ::BehaviorMethod*& outMethod, const Namespaces& namespaces, AZStd::string_view ebusName, AZStd::string_view eventName)
            {
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "Can't create the ebus sender without a behavior context!");
                    return false;
                }

                const auto& ebusIterator = behaviorContext->m_ebuses.find(ebusName);
                if (ebusIterator == behaviorContext->m_ebuses.end())
                {
                    AZ_Error("Script Canvas", false, "No ebus by name of %s in the behavior context!", ebusName.data());
                    return false;
                }

                AZ::BehaviorEBus* ebus = ebusIterator->second;
                AZ_Assert(ebus, "ebus == nullptr in %s", ebusName.data());

                const auto& sender = ebus->m_events.find(eventName);

                if (sender == ebus->m_events.end())
                {
                    AZ_Error("Script Canvas", false, "No event by name of %s found in the ebus %s", eventName.data(), ebusName.data());
                    return false;
                }

                AZ::EBusAddressPolicy addressPolicy
                    = ebus->m_idParam.m_typeId.IsNull()
                    ? AZ::EBusAddressPolicy::Single
                    : AZ::EBusAddressPolicy::ById;

                AZ::BehaviorMethod* method
                    = ebus->m_queueFunction
                    ? (addressPolicy == AZ::EBusAddressPolicy::ById ? sender->second.m_queueEvent : sender->second.m_queueBroadcast)
                    : (addressPolicy == AZ::EBusAddressPolicy::ById ? sender->second.m_event : sender->second.m_broadcast);

                if (!method)
                {
                    AZ_Error("Script Canvas", false, "Queue function mismatch in %s-%s", eventName.data(), ebusName.data());
                    return false;
                }

                outMethod = method;
                return true;
            }

            bool Method::FindFree(AZ::BehaviorMethod*& outMethod, const Namespaces& namespaces, AZStd::string_view methodName)
            {
                AZ::BehaviorContext* behaviorContext(nullptr);
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "A behavior context is required!");
                    return false;
                }

                const auto methodIter(behaviorContext->m_methods.find(methodName));
                if (methodIter == behaviorContext->m_methods.end())
                {
                    AZ_Error("Script Canvas", false, "No method by name of %s in the behavior context!", methodName.data());
                    return false;
                }

                AZ::BehaviorMethod* method(methodIter->second);
                if (!method)
                {
                    AZ_Error("Script Canvas", false, "BehaviorContext Method entry %s has no method pointer", methodName.data());
                    return false;
                }

                outMethod = method;
                return true;
            }

            void Method::OnWriteEnd()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
                
                AZ::BehaviorMethod* method{};
                switch (m_methodType)
                {
                case MethodType::Event:
                {
                    if (FindEvent(method, m_namespaces, m_className, m_methodName))
                    {
                        ConfigureMethod(*method);
                    }
                    break;
                }
                case MethodType::Free:
                {
                    if (FindFree(method, m_namespaces, m_methodName))
                    {
                        ConfigureMethod(*method);
                    }
                    break;
                }
                case MethodType::Member:
                {
                    if (FindClass(method, m_namespaces, m_className, m_methodName))
                    {
                        ConfigureMethod(*method);
                    }
                    break;
                }
                default:
                {
                    AZ_Error("Script Canvas", false, "unsupported method type in method");
                    break;
                }
                }
            }

            void Method::Reflect(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<Method, Node>()
                        ->Version(2)
                        ->EventHandler<SerializeContextEventHandlerDefault<Method>>()
                        ->Field("methodType", &Method::m_methodType)
                        ->Field("methodName", &Method::m_methodName)
                        ->Field("className", &Method::m_className)
                        ->Field("namespaces", &Method::m_namespaces)
                        ->Field("resultSlotID", &Method::m_resultSlotID)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<Method>("Method", "Method")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ;
                    }
                }
            }

        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas