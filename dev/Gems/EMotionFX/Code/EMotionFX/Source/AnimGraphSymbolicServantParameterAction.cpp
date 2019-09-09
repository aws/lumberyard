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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphSymbolicServantParameterAction.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeFloat.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphSymbolicServantParameterAction, AnimGraphAllocator, 0)

    AnimGraphSymbolicServantParameterAction::AnimGraphSymbolicServantParameterAction()
        : AnimGraphTriggerAction()
    {
    }


    AnimGraphSymbolicServantParameterAction::AnimGraphSymbolicServantParameterAction(AnimGraph* animGraph)
        : AnimGraphSymbolicServantParameterAction()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphSymbolicServantParameterAction::~AnimGraphSymbolicServantParameterAction()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }


    bool AnimGraphSymbolicServantParameterAction::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTriggerAction::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        return true;
    }


    const char* AnimGraphSymbolicServantParameterAction::GetPaletteName() const
    {
        return "Symbolic Servant Parameter Action";
    }


    void AnimGraphSymbolicServantParameterAction::TriggerAction(AnimGraphInstance* animGraphInstance) const
    {
        MCore::Attribute* masterAttribute = animGraphInstance->FindParameter(m_masterParameterName);
        if (!masterAttribute)
        {
            AZ_Assert(false, "Can't find a parameter named %s in the master graph.", m_masterParameterName.c_str());
            return;
        }

        const AZStd::vector<AnimGraphInstance*>& servantGraphs = animGraphInstance->GetServantGraphs();

        for (AnimGraphInstance* servantGraph : servantGraphs)
        {
            MCore::Attribute* servantAttribute = servantGraph->FindParameter(m_servantParameterName);
            if (servantAttribute)
            {
                if (servantAttribute->InitFrom(masterAttribute))
                {
                    // If the name matches and the type matches, we sync the attribute from master to servant.
                    AZ::Outcome<size_t> index = servantGraph->FindParameterIndex(m_servantParameterName);
                    const ValueParameter* valueParameter = servantGraph->GetAnimGraph()->FindValueParameter(index.GetValue());
                    AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnParameterActionTriggered, valueParameter);
                }
                else
                {
                    // If the name matches and but type doesn't, warns the user.
                    AZ_Warning("EMotionFX", "Servant parameter %s does not match master parameter %s", m_servantParameterName.c_str(), m_masterParameterName.c_str());
                }

            }
        }
    }


    // Construct and output the information summary string for this object
    void AnimGraphSymbolicServantParameterAction::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Servant Parameter Name='%s, Master Parameter Name='%s.", RTTI_GetTypeName(), m_servantParameterName.c_str(), m_masterParameterName.c_str());
    }


    // Construct and output the tooltip for this object
    void AnimGraphSymbolicServantParameterAction::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // Add the action type
        columnName = "Action Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"120\"><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // Add servant parameter
        columnName = "Servant Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_servantParameterName.c_str());

        // Add master parameter
        columnName = "Master Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_masterParameterName.c_str());
    }


    AnimGraph* AnimGraphSymbolicServantParameterAction::GetRefAnimGraph() const
    {
        if (m_refAnimGraphAsset.GetId().IsValid() && m_refAnimGraphAsset.IsReady())
        {
            return m_refAnimGraphAsset.Get()->GetAnimGraph();
        }
        return nullptr;
    }


    void AnimGraphSymbolicServantParameterAction::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphSymbolicServantParameterAction, AnimGraphTriggerAction>()
            ->Version(1)
            ->Field("animGraphAsset", &AnimGraphSymbolicServantParameterAction::m_refAnimGraphAsset)
            ->Field("servantParameterName", &AnimGraphSymbolicServantParameterAction::m_servantParameterName)
            ->Field("masterParameterName", &AnimGraphSymbolicServantParameterAction::m_masterParameterName)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphSymbolicServantParameterAction>("Servant Parameter Action", "Servant parameter action attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphSymbolicServantParameterAction::m_refAnimGraphAsset, "Servant anim graph", "Servant anim graph that we want to pick a parameter from")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphSymbolicServantParameterAction::OnAnimGraphAssetChanged)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("AnimGraphParameter", 0x778af55a), &AnimGraphSymbolicServantParameterAction::m_servantParameterName, "Servant parameter", "The servant parameter that we want to sync to.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC("AnimGraph", 0x0d53d4b3), &AnimGraphSymbolicServantParameterAction::GetRefAnimGraph)
            ->DataElement(AZ_CRC("AnimGraphParameter", 0x778af55a), &AnimGraphSymbolicServantParameterAction::m_masterParameterName, "Master parameter", "The master parameter that we want to sync from.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC("AnimGraph", 0x0d53d4b3), &AnimGraphSymbolicServantParameterAction::GetAnimGraph)
            ;
    }


    void AnimGraphSymbolicServantParameterAction::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_refAnimGraphAsset)
        {
            m_refAnimGraphAsset = asset;

            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);

            OnAnimGraphAssetReady();
        }
    }


    void AnimGraphSymbolicServantParameterAction::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_refAnimGraphAsset)
        {
            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);
            m_refAnimGraphAsset = asset;

            OnAnimGraphAssetReady();
        }
    }

    void AnimGraphSymbolicServantParameterAction::OnAnimGraphAssetChanged()
    {
        LoadAnimGraphAsset();
    }

    void AnimGraphSymbolicServantParameterAction::LoadAnimGraphAsset()
    {
        if (m_refAnimGraphAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_refAnimGraphAsset.GetId());

            if (!m_refAnimGraphAsset.IsLoading()) // someone else is loading it, we will get notified
            {
                if (!m_refAnimGraphAsset.IsReady())
                {
                    if (!m_refAnimGraphAsset.QueueLoad())
                    {
                        // When switching between assets I have seen that the asset is not ready, we call QueueLoad, it fails
                        // because the asset is already loaded, and at that point it assigns the right m_assetData
                        AZ_Assert(m_refAnimGraphAsset.IsReady(), "Expecting a ready anim graph asset");
                        OnAnimGraphAssetReady();
                    }
                }
                else
                {
                    // Asset is ready
                    OnAnimGraphAssetReady();
                }
            }
        }
    }

    void AnimGraphSymbolicServantParameterAction::OnAnimGraphAssetReady()
    {
        // Verify if the servant parameter is valid in the ref anim graph
        AnimGraph* refAnimGraph = GetRefAnimGraph();
        if (refAnimGraph)
        {
            if (!refAnimGraph->FindParameterByName(m_servantParameterName))
            {
                m_servantParameterName.clear();
            }
        }

        // Verify if the master parameter is valid in the master anim graph
        AnimGraph* masterAnimGraph = GetAnimGraph();
        if (masterAnimGraph)
        {
            if (!refAnimGraph->FindParameterByName(m_masterParameterName))
            {
                m_masterParameterName.clear();
            }
        }
    }
} // namespace EMotionFX