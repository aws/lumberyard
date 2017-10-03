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
        template<typename t_Node, typename t_Datum, bool s_hasProperties=false>
        class NativeDatumNode
            : public PureData
        {
        public:
            using t_ThisType = NativeDatumNode<t_Node, t_Datum, s_hasProperties>;

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
            void AddProperties() {}

            template<typename t_Getter, typename t_Setter>
            void AddProperty(t_Getter getter, t_Setter setter, const char* name)
            {
                AZ_STATIC_ASSERT(s_hasProperties, "AddProperties cannot be invoked on nodes without properties");
                using t_GetterThisType = std::remove_cv_t<std::remove_reference_t< typename AZStd::function_traits<t_Getter>::class_type>>;
                static_assert(AZStd::is_same<t_GetterThisType, std::remove_cv_t<std::remove_reference_t< typename AZStd::function_traits<t_Setter>::class_type>>>::value, "nuh-uh");

                AZ::u32 slotIndex = aznumeric_caster(m_slotContainer.m_slots.size());
                const SlotId setterSlotId = AddSetter<t_GetterThisType >(setter, name);
                AZ::u32 getterIndex = aznumeric_caster(m_propertyAccount.m_getters.size());
                AddGetter<t_GetterThisType>(getter, name);
                m_propertyAccount.m_getterIndexByInputSlot.insert({ setterSlotId, getterIndex });
                m_propertyAccount.m_getterSlotIndices.push_back(slotIndex + 1);
            }

            template<typename t_GetterThisType, typename t_Function>
            SlotId AddGetter(t_Function function, const AZStd::string_view& slotName)
            {
                using t_PropertyType = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits<t_Function>::result_type>>;

                m_propertyAccount.m_getters.push_back([function](const Datum& thisDatum) -> Datum
                {
                    return Datum::CreateInitializedCopy((thisDatum.GetAs<t_Datum>()->*function)());
                });

                Data::Type type(Data::FromAZType<t_PropertyType>());
                return AddOutputTypeSlot(AZStd::string::format("%s: %s", slotName.begin(), Data::GetName(type)).c_str(), "", AZStd::move(type), OutputStorage::Optional);
            }

            template<typename t_SetterThisType, typename t_Function>
            SlotId AddSetter(t_Function function, const AZStd::string_view& slotName)
            {
                using t_PropertyType = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits_get_arg_t<t_Function, 0>>>;

                auto setter = ([function](Datum& thisDatum, const Datum& setValue)
                {
                    (thisDatum.ModAs<t_Datum>()->*function)(*setValue.GetAs<t_PropertyType>());
                });

                Data::Type type(Data::FromAZType<t_PropertyType>());
                SlotId slotId(AddInputTypeSlot(AZStd::string::format("%s: %s", Data::GetName(type), slotName.begin()).c_str(), "", AZStd::move(type), InputTypeContract::DatumType));
                m_propertyAccount.m_settersByInputSlot.insert({ slotId, setter });
                return slotId;
            }

            AZ_INLINE void CallGetter(AZ::u32 getterIndex)
            {
                AZStd::vector<AZStd::pair<Node*, const SlotId>> outputNodes(ModConnectedNodes(GetSlots()[m_propertyAccount.m_getterSlotIndices[getterIndex]]));

                if (!outputNodes.empty())
                {
                    Datum getResult(m_propertyAccount.m_getters[getterIndex](m_inputData[k_thisDatumIndex]));

                    for (auto& nodePtrSlot : outputNodes)
                    {
                        if (nodePtrSlot.first)
                        {
                            Node::SetInput(*nodePtrSlot.first, nodePtrSlot.second, getResult);
                        }
                    }
                }
            }

            void OnInit() override
            {
                AddInputAndOutputTypeSlot(Data::FromAZType<t_Datum>());
                static_cast<t_Node*>(this)->AddProperties();
            }

            template<typename t_NativeDatumNodeType = NativeDatumNode, bool = s_hasProperties>
            struct SetInputHelper
            {
                AZ_INLINE static void Help(t_NativeDatumNodeType& node, const Datum& input, const SlotId& id)
                {
                    node.Node::SetInput(input, id);
                }
            };

            template<typename t_NativeDatumNodeType>
            struct SetInputHelper<t_NativeDatumNodeType, true>
            {
                AZ_INLINE static void Help(t_NativeDatumNodeType& node, const Datum& input, const SlotId& id)
                {
                    if (node.IsSetThisSlot(id))
                    {
                        // push this value, as usual
                        node.Node::SetInput(input, id);

                        // push the (possibly new) value to all the connected getters
                        for (AZ::u32 getterIndex(0), sentinel(aznumeric_caster(node.m_propertyAccount.m_getters.size())); getterIndex != sentinel; ++getterIndex)
                        {
                            node.CallGetter(getterIndex);
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
                SetInputHelper<t_ThisType, s_hasProperties>::Help(*this, input, id);
            }

            AZ_INLINE void SetProperty(const Datum& input, const SlotId& id)
            {
                auto methodBySlotIter = m_propertyAccount.m_settersByInputSlot.find(id);
                if (methodBySlotIter != m_propertyAccount.m_settersByInputSlot.end())
                {
                    if (methodBySlotIter->second)
                    {
                        methodBySlotIter->second(m_inputData[k_thisDatumIndex], input);
                        PushThis();

                        auto getterIndexIter = m_propertyAccount.m_getterIndexByInputSlot.find(id);
                        if (getterIndexIter != m_propertyAccount.m_getterIndexByInputSlot.end())
                        {
                            CallGetter(getterIndexIter->second);
                        }
                    }
                }
            }

        private:
            PropertyAccount<GetterFunction, SetterFunction, s_hasProperties> m_propertyAccount;
        };
    }
}