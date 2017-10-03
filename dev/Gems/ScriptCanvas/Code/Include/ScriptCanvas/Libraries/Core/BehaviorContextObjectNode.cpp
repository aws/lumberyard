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

#include "BehaviorContextObjectNode.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/string_view.h>
#include <Libraries/Core/MethodUtility.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            namespace Internal
            {
                static const AZ::u32 k_thisParamIndex(0);
                static const AZ::u32 k_setValueParamIndex(1);
                static const AZ::u32 k_firstFirstPropertyDataSlotIndex(2);
                static const AZ::u32 k_valueArgumentIndex(1);

                AZ_INLINE void CallGetter(AZ::u32 index, AZ::u32 slotIndex, Node& node, const AZStd::vector<AZ::BehaviorMethod*>& getters, AZStd::array<AZ::BehaviorValueParameter, BehaviorContextObjectNode::ParameterCount::Setter>& params)
                {
                    AZ::BehaviorMethod* getter = getters[index];
                    AZ_Error("Script Canvas", getter, "BehaviorContextObject %s getter not initialized", node.GetDebugName().data());

                    if (getter)
                    {
                        auto resultSlot = node.GetSlots()[slotIndex].GetId();
                        BehaviorContextMethodHelper::Call(node, getter, params.begin(), params.begin() + BehaviorContextObjectNode::ParameterCount::Getter, resultSlot);
                    }
                }

                AZ_INLINE bool InitializeGetterParams
                    ( AZStd::array<AZ::BehaviorValueParameter, BehaviorContextObjectNode::ParameterCount::Setter>& params
                    , AZ::BehaviorMethod* getter
                    , Node& node
                    , const Datum& thisDatum)
                {
                    AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> thisObjectParam = thisDatum.ToBehaviorValueParameter(*getter->GetArgument(Internal::k_thisParamIndex));
                    if (!thisObjectParam.IsSuccess())
                    {
                        SCRIPTCANVAS_REPORT_ERROR((node), "BehaviorContextObject %s couldn't be turned into a BehaviorValueParameter for getter: %s", node.GetDebugName().data(), thisObjectParam.GetError().data());
                        return false;
                    }
                    params[Internal::k_thisParamIndex].Set(thisObjectParam.GetValue());
                    return true;
                }

                AZ_INLINE bool InitializeSetterParams
                    ( AZStd::array<AZ::BehaviorValueParameter, BehaviorContextObjectNode::ParameterCount::Setter>& params
                    , AZ::BehaviorMethod* setter
                    , const Datum& input
                    , Node& node
                    , const Datum& thisDatum)
                {
                    AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> setValueParam = input.ToBehaviorValueParameter(*setter->GetArgument(k_setValueParamIndex));
                    if (!setValueParam.IsSuccess())
                    {
                        SCRIPTCANVAS_REPORT_ERROR((node), "BehaviorContextObject %s couldn't be turned into a BehaviorValueParameter for getter: %s", node.GetDebugName().data(), setValueParam.GetError().data());
                        return false;
                    }
                    params[Internal::k_setValueParamIndex].Set(setValueParam.GetValue());

                    return InitializeGetterParams(params, setter, node, thisDatum);
                }
            } // namespace Internal

            AZStd::string BehaviorContextObjectNode::GetDebugName() const
            {
                if (auto input = GetInput(GetSlotId(k_setThis)))
                {
                    return AZStd::string::format("%s", Data::GetName(input->GetType()));
                }

                return "Invalid";
            }

            void BehaviorContextObjectNode::InitializeObject(const AZ::Uuid& azType)
            {
                // \todo this works with basic types, and it probably shouldn't
                if (auto bcClass = AZ::BehaviorContextHelper::GetClass(azType))
                {
                    InitializeObject(*bcClass);
                }
                else
                {
                    const void* defaultValue = nullptr;
                    if (Data::IsEntityID(azType))
                    {
                        defaultValue = &ScriptCanvas::SelfReferenceId;
                    }
                    AddInputAndOutputTypeSlot(Data::FromBehaviorContextType(azType), defaultValue);
                }
            }

            void BehaviorContextObjectNode::InitializeObject(const AZStd::string& classNameString)
            {
                if (auto bcClass = AZ::BehaviorContextHelper::GetClass(classNameString))
                {
                    InitializeObject(*bcClass);
                }
            }

            void BehaviorContextObjectNode::InitializeObject(const AZ::BehaviorClass& behaviorClass)
            {
                m_className = behaviorClass.m_name;
                InitializeProperties(behaviorClass);
                ConfigureProperties(behaviorClass);
            }

            void BehaviorContextObjectNode::InitializeObject(const Data::Type& type)
            {
                const void* defaultValue = nullptr;
                if (Data::IsEntityID(type))
                {
                    defaultValue = &ScriptCanvas::SelfReferenceId;
                }
                AddInputAndOutputTypeSlot(type, defaultValue);
            }

            void BehaviorContextObjectNode::InitializeProperties(const AZ::BehaviorClass& behaviorClass)
            {
                const void* defaultValue = nullptr;
                if (Data::IsEntityID(behaviorClass.m_typeId))
                {
                    defaultValue = &ScriptCanvas::SelfReferenceId;
                }
                AddInputAndOutputTypeSlot(Data::FromBehaviorContextType(behaviorClass.m_typeId), defaultValue);

                for (const auto& iter : behaviorClass.m_properties)
                {
                    const AZStd::string_view name = iter.first;
                    const AZ::BehaviorProperty* bcProperty = iter.second;

                    SlotId setterSlotId;

                    if (bcProperty->m_setter)
                    {
                        AZ_Assert(bcProperty->m_setter->GetNumArguments() == ParameterCount::Setter, "Invalid setter");

                        if (const AZ::BehaviorParameter* argument = bcProperty->m_setter->GetArgument(Internal::k_valueArgumentIndex))
                        {
                            Data::Type inputType(Data::FromBehaviorContextType(argument->m_typeId));
                            const AZStd::string argName = AZStd::string::format("%s: %s", Data::GetName(inputType), bcProperty->m_name.data());
                            const AZStd::string* argumentTooltipPtr = bcProperty->m_setter->GetArgumentToolTip(Internal::k_valueArgumentIndex);

                            setterSlotId = AddInputTypeSlot(argName, argumentTooltipPtr ? argumentTooltipPtr->c_str() : "", AZStd::move(inputType), InputTypeContract::DatumType);
                        }
                    }

                    // \todo any property with a get, and NOT a set must get a full method node
                    if (bcProperty->m_getter && bcProperty->m_setter)
                    {
                        if (bcProperty->m_getter->HasResult())
                        {
                            if (const AZ::BehaviorParameter* result = bcProperty->m_getter->GetResult())
                            {
                                Data::Type outputType(Data::FromBehaviorContextType(result->m_typeId));
                                // multiple outs will need out value names
                                const AZStd::string resultSlotName(AZStd::string::format("%s: %s", bcProperty->m_name.data(), Data::GetName(outputType)));
                                auto outputTypeSlot = AddOutputTypeSlot(resultSlotName, "", AZStd::move(outputType), OutputStorage::Optional);
                            }
                        }
                    }
                }
            }

            void BehaviorContextObjectNode::ConfigureProperties(const AZ::BehaviorClass& behaviorClass)
            {
                if (IsConfigured())
                {
                    return;
                }

                AZ::u32 slotIndex(Internal::k_firstFirstPropertyDataSlotIndex);
                for (const auto& iter : behaviorClass.m_properties)
                {
                    const AZStd::string_view name = iter.first;
                    const AZ::BehaviorProperty* bcProperty = iter.second;

                    SlotId setterSlotId;

                    if (bcProperty->m_setter)
                    {
                        AZ_Assert(bcProperty->m_setter->GetNumArguments() == ParameterCount::Setter, "Invalid setter");

                        if (const AZ::BehaviorParameter* argument = bcProperty->m_setter->GetArgument(Internal::k_valueArgumentIndex))
                        {
                            Data::Type inputType(Data::FromBehaviorContextType(argument->m_typeId));
                            const AZStd::string argName = AZStd::string::format("%s: %s", Data::GetName(inputType), bcProperty->m_name.data());
                            if (SlotExists(argName, SlotType::DataIn, setterSlotId))
                            {
                                m_propertyAccount.m_settersByInputSlot.insert(AZStd::make_pair(setterSlotId, bcProperty->m_setter));
                                ++slotIndex;
                            }
                        }
                    }

                    // \todo any property with a get, and NOT a set must get a full method node
                    if (bcProperty->m_getter && bcProperty->m_setter)
                    {
                        if (bcProperty->m_getter->HasResult())
                        {
                            if (const AZ::BehaviorParameter* result = bcProperty->m_getter->GetResult())
                            {
                                Data::Type outputType(Data::FromBehaviorContextType(result->m_typeId));
                                // multiple outs will need out value names
                                const AZStd::string resultSlotName(AZStd::string::format("%s: %s", bcProperty->m_name.data(), Data::GetName(outputType)));
                                auto outputTypeSlot = AddOutputTypeSlot(resultSlotName.c_str(), "", AZStd::move(outputType), OutputStorage::Optional);
                                if (SlotExists(resultSlotName, SlotType::DataOut))
                                {
                                    AZ::u32 getterIndex = aznumeric_caster(m_propertyAccount.m_getters.size());
                                    m_propertyAccount.m_getters.push_back(bcProperty->m_getter);
                                    m_propertyAccount.m_getterIndexByInputSlot.insert({ setterSlotId, getterIndex });
                                    m_propertyAccount.m_getterSlotIndices.push_back(slotIndex);
                                    ++slotIndex;
                                }
                            }
                        }
                    }
                }

                m_configured = true;
            }

            void BehaviorContextObjectNode::OnInit()
            {
                if (auto input = GetInput(GetSlotId(k_setThis)))
                {
                    if (!input->Empty())
                    {
                        AddInputTypeAndOutputTypeSlot(input->GetType());
                    }
                }
            }

            void BehaviorContextObjectNode::OnWriteEnd()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                auto bcClass = AZ::BehaviorContextHelper::GetClass(m_className);

                if (bcClass)
                {
                    ConfigureProperties(*bcClass);
                }
            }

            void BehaviorContextObjectNode::SetInput(const Datum& input, const SlotId& id)
            {
                if (IsSetThisSlot(id))
                {
                    // push this value, as usual
                    Node::SetInput(input, id);

                    // now, call every getter, as every property has (presumably) been changed
                    AZStd::array<AZ::BehaviorValueParameter, ParameterCount::Setter> params;
                    if (!m_propertyAccount.m_getters.empty() 
                    && m_propertyAccount.m_getters[0]
                    && Internal::InitializeGetterParams(params, m_propertyAccount.m_getters[0], *this, m_inputData[Internal::k_thisParamIndex]))
                    {
                        for (AZ::u32 i(0), sentinel(aznumeric_caster(m_propertyAccount.m_getters.size())); i != sentinel; ++i)
                        {
                            Internal::CallGetter(i, m_propertyAccount.m_getterSlotIndices[i], *this, m_propertyAccount.m_getters, params);
                        }
                    }
                }
                else
                {
                    SetProperty(input, id);
                }
            }

            void BehaviorContextObjectNode::Reflect(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<BehaviorContextObjectNode, Node>()
                        ->Version(1)
                        ->EventHandler<SerializeContextEventHandlerDefault<BehaviorContextObjectNode>>()
                        ->Field("m_className", &BehaviorContextObjectNode::m_className)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<BehaviorContextObjectNode>("BehaviorContextObjectNode", "BehaviorContextObjectNode")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                            ;
                    }
                }
            }

            void BehaviorContextObjectNode::SetProperty(const Datum& input, const SlotId& id)
            {
                // find method
                auto methodBySlotIter = m_propertyAccount.m_settersByInputSlot.find(id);
                if (methodBySlotIter != m_propertyAccount.m_settersByInputSlot.end())
                {
                    if (methodBySlotIter->second)
                    {
                        AZ::BehaviorMethod* setter = methodBySlotIter->second;
                        AZ_Error("Script Canvas", setter, "BehaviorContextObject %s setter not initialized", GetDebugName().data());
                        AZStd::array<AZ::BehaviorValueParameter, ParameterCount::Setter> params;

                        if (setter && Internal::InitializeSetterParams(params, setter, input, *this, m_inputData[PureData::k_thisDatumIndex]))
                        {
                            BehaviorContextMethodHelper::Call(*this, setter, params.begin(), params.begin() + ParameterCount::Setter, SlotId());

                            PushThis();

                            auto getterIndexIter = m_propertyAccount.m_getterIndexByInputSlot.find(id);
                            if (getterIndexIter != m_propertyAccount.m_getterIndexByInputSlot.end())
                            {
                                Internal::CallGetter(getterIndexIter->second, m_propertyAccount.m_getterSlotIndices[getterIndexIter->second], *this, m_propertyAccount.m_getters, params);
                            }
                        }
                        else
                        {
                            AZ_Error("Script Canvas", false, "BehaviorContextObject SlotId %s setter param initialization failed", id.m_id.ToString<AZStd::string>().data());
                        }
                    }
                    else
                    {
                        AZ_Error("Script Canvas", false, "BehaviorContextObject SlotId %s did not route to a valid setter", id.m_id.ToString<AZStd::string>().data());
                    }
                }
                else
                {              
                    AZ_Error("Script Canvas", false, "BehaviorContextObject SlotId %s did not route to a setter", id.m_id.ToString<AZStd::string>().data());
                }
            }

        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas