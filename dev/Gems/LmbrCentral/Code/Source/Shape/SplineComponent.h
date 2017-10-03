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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>

namespace LmbrCentral
{
    /**
     * Common functionality and data for the SplineComponent.
     */
    class SplineCommon
    {
    public:
        AZ_CLASS_ALLOCATOR(SplineCommon, AZ::SystemAllocator, 0);
        AZ_RTTI(SplineCommon, "{91A31D7E-F63A-4AA8-BC50-909B37F0AD8B}");

        SplineCommon();
        virtual ~SplineCommon() = default;

        static void Reflect(AZ::ReflectContext* context);

        void ChangeSplineType(AZ::u64 splineType);
        void SetCallbacks(const AZStd::function<void()>& OnChangeElement, const AZStd::function<void()>& OnChangeContainer);

        AZ::SplinePtr& GetSpline() { return m_spline; }

    private:
        AZ::u32 OnChangeSplineType();

        AZ::u64 m_splineType = AZ::LinearSpline::RTTI_Type().GetHash(); ///< The currently set spline type (default to Linear)
        AZ::SplinePtr m_spline; ///< Reference to the underlying spline data.

        AZStd::function<void()> m_onChangeElement = nullptr;
        AZStd::function<void()> m_onChangeContainer = nullptr;
    };

    /**
     * Component interface to core spline implementation.
     */
    class SplineComponent
        : public AZ::Component
        , private SplineComponentRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        friend class EditorSplineComponent;

        AZ_COMPONENT(SplineComponent, "{F0905297-1E24-4044-BFDA-BDE3583F1E57}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SplineComponentRequestBus handler
        AZ::ConstSplinePtr GetSpline() override;
        void ChangeSplineType(AZ::u64 splineType) override;
        void SetClosed(bool closed) override;
        
        void AddVertex(const AZ::Vector3& vertex) override;
        void UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        void InsertVertex(size_t index, const AZ::Vector3& vertex) override;
        void RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override;
        void ClearVertices() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus listener
        // Called when the local transform of the entity has changed. Local transform update always implies world transform change too.
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SplineService", 0x2b674d3c));
            provided.push_back(AZ_CRC("VertexContainerService", 0x22cf8e10));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SplineService", 0x2b674d3c));
            incompatible.push_back(AZ_CRC("VertexContainerService", 0x22cf8e10));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        SplineCommon m_splineCommon; ///< Stores common spline functionality and properties.
        AZ::Transform m_currentTransform; ///< Caches the current transform for the entity on which this component lives.
    };

} // namespace LmbrCentral
