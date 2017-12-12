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

#include "StdAfx.h"
#include "SplineComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace LmbrCentral
{
    using SplineComboBoxVec = AZStd::vector<AZStd::pair<size_t, AZStd::string> >;
    static SplineComboBoxVec PopulateSplineTypeList()
    {
        return SplineComboBoxVec
        {
            AZStd::make_pair(AZ::LinearSpline::RTTI_Type().GetHash(), "Linear"),
            AZStd::make_pair(AZ::BezierSpline::RTTI_Type().GetHash(), "Bezier"),
            AZStd::make_pair(AZ::CatmullRomSpline::RTTI_Type().GetHash(), "Catmull-Rom")
        };
    }

    static AZ::SplinePtr MakeSplinePtr(AZ::u64 splineType)
    {
        if (splineType == AZ::LinearSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::LinearSpline>();
        }
        
        if (splineType == AZ::BezierSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::BezierSpline>();
        }
        
        if (splineType == AZ::CatmullRomSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::CatmullRomSpline>();
        }
        
        AZ_Assert(false, "Unhandled spline type %d in %s", splineType, __FUNCTION__);

        return nullptr;
    }

    static AZ::SplinePtr CopySplinePtr(AZ::u64 splineType, const AZ::SplinePtr& spline)
    {
        if (splineType == AZ::LinearSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::LinearSpline>(*spline);
        }
        
        if (splineType == AZ::BezierSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::BezierSpline>(*spline);
        }
        
        if (splineType == AZ::CatmullRomSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::CatmullRomSpline>(*spline);
        }

        AZ_Assert(false, "Unhandled spline type %d in %s", splineType, __FUNCTION__);

        return nullptr;
    }

    SplineCommon::SplineCommon()
    {
        m_spline = MakeSplinePtr(m_splineType);
    }

    void SplineCommon::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineCommon>()
                ->Version(1)
                ->Field("Spline Type", &SplineCommon::m_splineType)
                ->Field("Spline", &SplineCommon::m_spline);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SplineCommon>("Configuration", "Spline configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SplineCommon::m_splineType, "Spline Type", "Interpolation style to use between points.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &PopulateSplineTypeList)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SplineCommon::OnChangeSplineType)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SplineCommon::m_spline, "Spline", "Data representing the path, in the entity's local coordinate space.")
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void SplineCommon::ChangeSplineType(AZ::u64 splineType)
    {
        m_splineType = splineType;
        OnChangeSplineType();
    }

    void SplineCommon::SetCallbacks(const AZStd::function<void()>& OnChangeElement, const AZStd::function<void()>& OnChangeContainer)
    {
        m_onChangeElement = OnChangeElement;
        m_onChangeContainer = OnChangeContainer;

        m_spline->SetCallbacks(OnChangeElement, OnChangeContainer);
    }

    AZ::u32 SplineCommon::OnChangeSplineType()
    {
        AZ::u32 ret = AZ::Edit::PropertyRefreshLevels::None;

        if (m_spline->RTTI_GetType().GetHash() != m_splineType)
        {
            m_spline = CopySplinePtr(m_splineType, m_spline);
            m_spline->SetCallbacks(m_onChangeElement, m_onChangeContainer);

            ret = AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        return ret;
    }

    /**
     * BehaviorContext forwarder for SplineComponentNotificationBus
     */
    class BehaviorSplineComponentNotificationBusHandler
        : public SplineComponentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorSplineComponentNotificationBusHandler, "{05816EA4-A4F0-4FB4-A82B-D6537B215D25}", AZ::SystemAllocator, OnSplineChanged);

        void OnSplineChanged() override
        {
            Call(FN_OnSplineChanged);
        }
    };

    void SplineComponent::Reflect(AZ::ReflectContext* context)
    {
        SplineCommon::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &SplineComponent::m_splineCommon);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SplineComponentNotificationBus>("SplineComponentNotificationBus")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)->
                Handler<BehaviorSplineComponentNotificationBusHandler>();

            behaviorContext->EBus<SplineComponentRequestBus>("SplineComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)
                ->Event("GetSpline", &SplineComponentRequestBus::Events::GetSpline)
                ->Event("SetClosed", &SplineComponentRequestBus::Events::SetClosed)
                ->Event("AddVertex", &SplineComponentRequestBus::Events::AddVertex)
                ->Event("UpdateVertex", &SplineComponentRequestBus::Events::UpdateVertex)
                ->Event("InsertVertex", &SplineComponentRequestBus::Events::InsertVertex)
                ->Event("RemoveVertex", &SplineComponentRequestBus::Events::RemoveVertex)
                ->Event("SetVertices", &SplineComponentRequestBus::Events::SetVertices)
                ->Event("ClearVertices", &SplineComponentRequestBus::Events::ClearVertices)
                ;
        }
    }

    void SplineComponent::Activate()
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        SplineComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SplineComponent::Deactivate()
    {
        SplineComponentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void SplineComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
    }

    AZ::ConstSplinePtr SplineComponent::GetSpline()
    {
        return m_splineCommon.GetSpline();
    }

    void SplineComponent::ChangeSplineType(AZ::u64 splineType)
    {
        m_splineCommon.ChangeSplineType(splineType);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void SplineComponent::UpdateVertex(size_t index, const AZ::Vector3& vertex)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.UpdateVertex(index, vertex);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void SplineComponent::AddVertex(const AZ::Vector3& vertex)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.AddVertex(vertex);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void SplineComponent::InsertVertex(size_t index, const AZ::Vector3& vertex)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.InsertVertex(index, vertex);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void SplineComponent::RemoveVertex(size_t index)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.RemoveVertex(index);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void SplineComponent::SetVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.SetVertices(vertices);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void SplineComponent::ClearVertices()
    {
        m_splineCommon.GetSpline()->m_vertexContainer.Clear();
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void SplineComponent::SetClosed(bool closed)
    {
        m_splineCommon.GetSpline()->SetClosed(closed);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }
} // namespace LmbrCentral