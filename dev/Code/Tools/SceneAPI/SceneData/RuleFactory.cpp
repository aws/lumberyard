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

#include <iterator>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <SceneAPI/SceneData/RuleFactory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Groups/SkeletonGroup.h>
#include <SceneAPI/SceneData/Groups/SkinGroup.h>
#include <SceneAPI/SceneData/Rules/CommentRule.h>
#include <SceneAPI/SceneData/Rules/MeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/OriginRule.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneData/Rules/PhysicsRule.h>

#include <SceneAPI/SceneData/Rules/PropertyEditorViews/MeshAdvancedRuleViewModel.h>
#include <SceneAPI/SceneData/Rules/PropertyEditorViews/OriginRuleViewModel.h>
#include <SceneAPI/SceneData/Rules/PropertyEditorViews/PhysicsRuleViewModel.h>

#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

#define DEFAULT_RULE(name, type, allowMultiple) \
    { name, type ::TYPEINFO_Uuid(), [] { return AZStd::shared_ptr<DataTypes::IRule>(AZStd::make_shared< type >()); }, nullptr, nullptr, allowMultiple }
#define VIEWMODEL_RULE(name, type, allowMultiple, view) \
    { name, type ::TYPEINFO_Uuid(), [] { return AZStd::shared_ptr<DataTypes::IRule>(AZStd::make_shared< type >()); }, view, nullptr, allowMultiple }
#define WIDGET_RULE(name, type, allowMultiple, widget) \
    { name, type ::TYPEINFO_Uuid(), [] { return AZStd::shared_ptr<DataTypes::IRule>(AZStd::make_shared< type >()); }, nullptr, widget, allowMultiple }

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZStd::vector<RuleFactory::RuleData> RuleFactory::s_ruleTypes[static_cast<size_t>(GroupType::NumGroupTypes)];

            void RuleFactory::Reflect(AZ::ReflectContext* context)
            {
                MeshGroup::Reflect(context);
                SkeletonGroup::Reflect(context);
                SkinGroup::Reflect(context);
                CommentRule::Reflect(context);
                MeshAdvancedRule::Reflect(context);
                OriginRule::Reflect(context);
                MaterialRule::Reflect(context);
                PhysicsRule::Reflect(context);

                MeshAdvancedRuleViewModel::Reflect(context);
                OriginRuleViewModel::Reflect(context);
                PhysicsRuleViewModel::Reflect(context);
                SceneNodeSelectionList::Reflect(context);
            }

            RuleFactory::RuleFactory()
            {
                if (s_ruleTypes[static_cast<size_t>(GroupType::Mesh)].empty() ||
                    s_ruleTypes[static_cast<size_t>(GroupType::Skeleton)].empty() ||
                    s_ruleTypes[static_cast<size_t>(GroupType::Skin)].empty() ||
                    s_ruleTypes[static_cast<size_t>(GroupType::Animation)].empty())
                {
                    BuildRuleTypes();
                }
            }

            void RuleFactory::BuildRuleTypes()
            {
                s_ruleTypes[static_cast<size_t>(GroupType::Mesh)] =
                {
                    DEFAULT_RULE("Comment", CommentRule, AllowMultiple::Yes),
                    VIEWMODEL_RULE("Origin", OriginRule, AllowMultiple::No,
                        [](AZStd::shared_ptr<const Containers::Scene>& scene, QWidget*, const AZStd::shared_ptr<DataTypes::IManifestObject>& instance)
                        {
                            return AZStd::unique_ptr<DataTypes::IViewModel>(aznew OriginRuleViewModel(scene, instance));
                        }),
                    VIEWMODEL_RULE("Advanced", MeshAdvancedRule, AllowMultiple::No,
                        [](AZStd::shared_ptr<const Containers::Scene>& scene, QWidget*, const AZStd::shared_ptr<DataTypes::IManifestObject>& instance)
                        {
                            return AZStd::unique_ptr<DataTypes::IViewModel>(aznew MeshAdvancedRuleViewModel(scene, instance));
                        }),
                    DEFAULT_RULE("Materials", MaterialRule, AllowMultiple::No),
                    VIEWMODEL_RULE("Physics", PhysicsRule, AllowMultiple::No,
                        [](AZStd::shared_ptr<const Containers::Scene>& scene, QWidget*, const AZStd::shared_ptr<DataTypes::IManifestObject>& instance)
                        {
                            return AZStd::unique_ptr<DataTypes::IViewModel>(aznew PhysicsRuleViewModel(scene, instance));
                        })
                };
                s_ruleTypes[static_cast<size_t>(GroupType::Skeleton)] =
                {
                    DEFAULT_RULE("Comment", CommentRule, AllowMultiple::Yes)
                };
                s_ruleTypes[static_cast<size_t>(GroupType::Skin)] =
                {
                    DEFAULT_RULE("Comment", CommentRule, AllowMultiple::Yes),
                    VIEWMODEL_RULE("Advanced", MeshAdvancedRule, AllowMultiple::No,
                        [](AZStd::shared_ptr<const Containers::Scene>& scene, QWidget*, const AZStd::shared_ptr<DataTypes::IManifestObject>& instance)
                        {
                            return AZStd::unique_ptr<DataTypes::IViewModel>(aznew MeshAdvancedRuleViewModel(scene, instance));
                        }),
                    DEFAULT_RULE("Materials", MaterialRule, AllowMultiple::No),
                };
                s_ruleTypes[static_cast<size_t>(GroupType::Animation)] =
                {
                    DEFAULT_RULE("Comment", CommentRule, AllowMultiple::Yes)
                };
            }

            size_t RuleFactory::GetRuleCount(GroupType group) const
            {
                return s_ruleTypes[static_cast<size_t>(group)].size();
            }

            const AZStd::string& RuleFactory::GetRuleName(GroupType group, size_t index) const
            {
                if (index < s_ruleTypes[static_cast<size_t>(group)].size())
                {
                    return s_ruleTypes[static_cast<size_t>(group)][index].m_name;
                }
                else
                {
                    static AZStd::string emptyString;
                    return emptyString;
                }
            }

            const AZStd::string& RuleFactory::GetRuleName(GroupType group, const AZStd::shared_ptr<DataTypes::IRule>& instance) const
            {
                AZ_Assert(instance, "Invalid rule instance passed to RuleFactory.");

                const AZ::Uuid& typeId = instance->RTTI_GetType();
                for (auto& it : s_ruleTypes[static_cast<size_t>(group)])
                {
                    if (it.m_typeId == typeId)
                    {
                        return it.m_name;
                    }
                }

                static AZStd::string emptyString;
                return emptyString;
            }

            AZStd::shared_ptr<DataTypes::IRule> RuleFactory::CreateRule(GroupType group, const AZStd::string& name) const
            {
                for (auto& it : s_ruleTypes[static_cast<size_t>(group)])
                {
                    if (it.m_name == name)
                    {
                        return it.m_constructor();
                    }
                }
                return AZStd::shared_ptr<DataTypes::IRule>();
            }

            bool RuleFactory::AllowsMultipleRuleInstances(GroupType group, const AZStd::string& name) const
            {
                for (auto& it : s_ruleTypes[static_cast<size_t>(group)])
                {
                    if (it.m_name == name)
                    {
                        return it.m_limit == AllowMultiple::Yes;
                    }
                }
                return false;
            }

            bool RuleFactory::AllowsMultipleRuleInstances(GroupType group, const AZStd::shared_ptr<DataTypes::IRule>& instance) const
            {
                AZ_Assert(instance, "Invalid rule instance passed to RuleFactory.");

                const AZ::Uuid& typeId = instance->RTTI_GetType();
                for (auto& it : s_ruleTypes[static_cast<size_t>(group)])
                {
                    if (it.m_typeId == typeId)
                    {
                        return it.m_limit == AllowMultiple::Yes;
                    }
                }
                return false;
            }

            AZStd::unique_ptr<DataTypes::IViewModel> RuleFactory::CreateViewModel(
                AZStd::shared_ptr<const Containers::Scene>& scene, QWidget* parent, const AZStd::shared_ptr<DataTypes::IManifestObject>& instance) const
            {
                const AZ::Uuid& typeId = instance->RTTI_GetType();
                for (auto& group : s_ruleTypes)
                {
                    for (auto& entry : group)
                    {
                        if (entry.m_typeId == typeId)
                        {
                            if (entry.m_viewModelConstructor)
                            {
                                return entry.m_viewModelConstructor(scene, parent, instance);
                            }
                        }
                    }
                }

                return AZStd::unique_ptr<DataTypes::IViewModel>(nullptr);
            }

            QWidget* RuleFactory::CreateWidget(AZStd::shared_ptr<const Containers::Scene>& scene, QWidget* parent,
                AZ::SerializeContext& serializeContext, const AZStd::shared_ptr<DataTypes::IManifestObject>& instance) const
            {
                const AZ::Uuid& typeId = instance->RTTI_GetType();
                for (auto& group : s_ruleTypes)
                {
                    for (auto& entry : group)
                    {
                        if (entry.m_typeId == typeId)
                        {
                            if (entry.m_widgetConstructor)
                            {
                                return entry.m_widgetConstructor(scene, parent, serializeContext, instance);
                            }
                        }
                    }
                }

                return nullptr;
            }

            AZStd::shared_ptr<DataTypes::IGroup> RuleFactory::CreateGroup(GroupType group) const
            {
                switch (group)
                {
                case GroupType::Mesh:
                    return AZStd::make_shared<MeshGroup>();
                case GroupType::Skeleton:
                    return AZStd::make_shared<SkeletonGroup>();
                case GroupType::Skin:
                    return AZStd::make_shared<SkinGroup>();
                default:
                    return nullptr;
                }
            }
        }
    }
}

#undef DEFAULT_RULE
#undef VIEWMODEL_RULE
#undef WIDGET_RULE
