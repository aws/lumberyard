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

#pragma once

#include <Data/Data.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/PureData.h>
#include <AzCore/std/typetraits/function_traits.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        template<typename t_Node, typename t_Datum>
        class NativeDatumNode
            : public PureData
        {
        public:
            using t_ThisType = NativeDatumNode<t_Node, t_Datum>;

            AZ_RTTI(((NativeDatumNode<t_Node, t_Datum>), "{B7D8D8D6-B2F1-481A-A712-B07D1C19555F}", t_Node, t_Datum), Node, AZ::Component);
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(NativeDatumNode);
            AZ_COMPONENT_BASE(NativeDatumNode, Node);

            static void Reflect(AZ::ReflectContext* reflection) 
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                {
                    serializeContext->Class<t_ThisType, PureData>()
                        ->Version(0)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<t_ThisType>("NativeDatumNode", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ;
                    }
                }
            }

        protected:
            virtual void AddProperties() {}

            template<typename t_Getter, typename t_Setter>
            void AddProperty(t_Getter getter, t_Setter setter, const char* name)
            {
                using t_GetterThisType = std::remove_cv_t<std::remove_reference_t< typename AZStd::function_traits<t_Getter>::class_type>>;
                static_assert(AZStd::is_same<t_GetterThisType, std::remove_cv_t<std::remove_reference_t< typename AZStd::function_traits<t_Setter>::class_type>>>::value, "nuh-uh");

                SlotId setterSlotId = AddSetter<t_GetterThisType>(setter, name);
                SlotId getterSlotId = AddGetter<t_GetterThisType>(getter, name);
                if (setterSlotId.IsValid() && getterSlotId.IsValid())
                {
                    m_propertyAccount.m_getterSetterIdPairs.emplace_back(getterSlotId, setterSlotId);
                }
            }

            template<typename t_GetterThisType, typename t_Function>
            SlotId AddGetter(t_Function function, const AZStd::string_view& slotName)
            {
                using t_PropertyType = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits<t_Function>::result_type>>;
                if (!function)
                {
                    return {};
                }

                auto getterFunc = [function](const Datum& thisDatum) -> Datum
                {
                    return Datum::CreateInitializedCopy((thisDatum.GetAs<t_Datum>()->*function)());
                };


                Data::Type type(Data::FromAZType<t_PropertyType>());
                SlotId getterSlotId = AddOutputTypeSlot(AZStd::string::format("%s: %s", slotName.begin(), Data::GetName(type)).c_str(), "", AZStd::move(type), OutputStorage::Optional);
                m_propertyAccount.m_getters.insert({getterSlotId, getterFunc});

                return getterSlotId;
            }

            template<typename t_SetterThisType, typename t_Function>
            SlotId AddSetter(t_Function function, const AZStd::string_view& slotName)
            {
                using t_PropertyType = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits_get_arg_t<t_Function, 0>>>;
                if (!function)
                {
                    return {};
                }

                auto setter = ([function](Datum& thisDatum, const Datum& setValue)
                {
                    (thisDatum.ModAs<t_Datum>()->*function)(*setValue.GetAs<t_PropertyType>());
                });

                Data::Type type(Data::FromAZType<t_PropertyType>());
                SlotId slotId(AddInputTypeSlot(AZStd::string::format("%s: %s", Data::GetName(type), slotName.begin()).c_str(), "", AZStd::move(type), InputTypeContract::DatumType));
                m_propertyAccount.m_settersByInputSlot.insert({ slotId, setter });
                return slotId;
            }

            AZ_INLINE void CallGetter(SlotId getterSlotId)
            {
                auto getterFuncIt = m_propertyAccount.m_getters.find(getterSlotId);
                Slot* getterSlot = GetSlot(getterSlotId);
                if (getterSlot && getterFuncIt != m_propertyAccount.m_getters.end())
                {
                    AZStd::vector<AZStd::pair<Node*, const SlotId>> outputNodes(ModConnectedNodes(*getterSlot));

                    if (!outputNodes.empty())
                    {
                        Datum getResult(getterFuncIt->second(m_inputData[k_thisDatumIndex]));

                        for (auto& nodePtrSlot : outputNodes)
                        {
                            if (nodePtrSlot.first)
                            {
                                Node::SetInput(*nodePtrSlot.first, nodePtrSlot.second, getResult);
                            }
                        }
                    }
                }
            }

            void OnInit() override
            {
                AddInputAndOutputTypeSlot(Data::FromAZType<t_Datum>());
                static_cast<t_Node*>(this)->AddProperties();
            }

            void OnActivate() override
            {
                PureData::OnActivate();
                for (const auto& getterSetterIdPair : m_propertyAccount.m_getterSetterIdPairs)
                {
                    CallGetter(getterSetterIdPair.first);
                }
            }

            template<typename t_NativeDatumNodeType>
            struct SetInputHelper
            {
                AZ_INLINE static void Help(t_NativeDatumNodeType& node, const Datum& input, const SlotId& id)
                {
                    if (node.IsSetThisSlot(id))
                    {
                        // push this value, as usual
                        node.Node::SetInput(input, id);

                        // push the (possibly new) value to all the connected getters
                        for (const auto& getterSetterIdPair : node.m_propertyAccount.m_getterSetterIdPairs)
                        {
                            node.CallGetter(getterSetterIdPair.first);
                        }
                    }
                    else
                    {
                        node.SetProperty(input, id);
                    }
                }
            };

            void SetInput(const Datum& input, const SlotId& id) override
            {
                SetInputHelper<t_ThisType>::Help(*this, input, id);
            }

            AZ_INLINE void SetProperty(const Datum& input, const SlotId& setterId)
            {
                auto methodBySlotIter = m_propertyAccount.m_settersByInputSlot.find(setterId);
                if (methodBySlotIter != m_propertyAccount.m_settersByInputSlot.end())
                {
                    if (methodBySlotIter->second)
                    {
                        methodBySlotIter->second(m_inputData[k_thisDatumIndex], input);
                        PushThis();

                        auto getterSetterIter = AZStd::find_if(m_propertyAccount.m_getterSetterIdPairs.begin(), m_propertyAccount.m_getterSetterIdPairs.end(),
                            [&setterId](const AZStd::pair<SlotId, SlotId>& getterSetterIdPair)
                        {
                            return setterId == getterSetterIdPair.second;
                        });
                        if (getterSetterIter != m_propertyAccount.m_getterSetterIdPairs.end())
                        {
                            CallGetter(getterSetterIter->first);
                        }
                    }
                }
            }

        private:
            PropertyAccount<GetterFunction, SetterFunction> m_propertyAccount;
        };
    }
}