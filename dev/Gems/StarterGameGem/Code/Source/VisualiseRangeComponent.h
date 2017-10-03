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
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
	class ReflectContext;
}

namespace StarterGameGem
{

	/*!
	* VisualiseRangeComponentRequests
	* Messages serviced by the VisualiseRangeComponent
	*/
	class VisualiseRangeComponentRequests
		: public AZ::ComponentBus
	{
	public:
		virtual ~VisualiseRangeComponentRequests() {}

		//! Sets the information for the sight range.
		virtual void SetSightRange(float aggroRange, float sightRange) = 0;
		//! Sets the information for the suspicion range.
		virtual void SetSuspicionRange(float range, float fov, float rearRange) = 0;

	};

	using VisualiseRangeComponentRequestsBus = AZ::EBus<VisualiseRangeComponentRequests>;


	class VisualiseRangeComponent
		: public AZ::Component
		, private AZ::TickBus::Handler
        , private AZ::TransformNotificationBus::Handler
		, private VisualiseRangeComponentRequestsBus::Handler
	{
	public:
		AZ_COMPONENT(VisualiseRangeComponent, "{3D76625F-1D2E-43C1-95A9-D385189F4CFE}");

		~VisualiseRangeComponent() override = default;

		//////////////////////////////////////////////////////////////////////////
		// AZ::Component interface implementation
		void Init() override;
		void Activate() override;
		void Deactivate() override;
		//////////////////////////////////////////////////////////////////////////

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		//////////////////////////////////////////////////////////////////////////
		// AZ::TickBus interface implementation
		void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
		//////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // Transform notification bus handler
        /// Called when the local transform of the entity has changed.
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;
        //////////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// VisualiseRangeComponentRequestsBus::Handler
		void SetSightRange(float aggroRange, float sightRange) override;
		void SetSuspicionRange(float range, float fov, float rearRange) override;

	protected:
        //! Stores the transform of the entity
        AZ::Transform m_currentEntityTransform;

	private:
		// For suspicion.
		float m_suspicionRange;
		float m_suspicionFoV;
		float m_suspicionRearRange;
		AZ::Vector4 m_colourSuspicion;

		// For combat.
		float m_rangeSight;
		float m_rangeAggro;
		AZ::Vector4 m_colourCombat;

	};

} // namespace StarterGameGem
