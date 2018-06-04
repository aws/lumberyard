#pragma once

#include <CameraFramework/ICameraZoomBehavior.h>
#include <StartingPointCamera/StartingPointCameraZoomBehaviorBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>

namespace AZ
{
	class ReflectContext;
}

namespace Camera
{
	class DefaultZoomBehavior
		: public Camera::ICameraZoomBehavior
		, private AZ::TickBus::Handler
        , private Camera::CameraZoomBehaviorBus::Handler
	{
	public:
        ~DefaultZoomBehavior() override = default;
		AZ_RTTI( DefaultZoomBehavior, "{EFDA60A6-16B3-4199-BE31-1C7F105BFF25}", Camera::ICameraZoomBehavior );
        AZ_CLASS_ALLOCATOR(DefaultZoomBehavior, AZ::SystemAllocator, 0);
		static void Reflect( AZ::ReflectContext* reflection );

        //////////////////////////////////////////////////////////////////////////
        // ICameraZoomBehavior
		void ModifyZoom( const AZ::EntityId& targetEntityId, float& inOutZoom ) override;
		void Activate( AZ::EntityId ) override;
		void Deactivate() override;

	private:

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler
		void OnTick( float deltaTime, AZ::ScriptTimePoint time ) override;

        //////////////////////////////////////////////////////////////////////////
        // Camera::CameraZoomBehaviorBus::Handler
        void SetZoomTarget(float zoomTarget) override;
        void SetTimeToZoom(float timeToZoom) override;
        void StartZoom() override;
        void StopZoom() override;

        static float const k_defaultTimeToZoom;
        static float const k_defaultZoom;

        bool m_zoomOnActivate   = false;
        float m_timeToZoom      = k_defaultTimeToZoom;
		float m_zoom			= k_defaultZoom;			// Current zoom modifier to apply to the camera
		float m_zoomTarget		= k_defaultZoom;			// The zoom modifier that is being interpolated to
		float m_zoomVelocity	= 0.0f;						// The velocity for modifying the zoom 
                                                            // note that we use velocity and not a current time calc, 
                                                            // as then if the zoom in is interrupted then the zoom out time is correctly shorter.
	};
}
