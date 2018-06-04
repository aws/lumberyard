#pragma once
#include "CameraFramework/ICameraSubComponent.h"

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    // This class is responsible for modifying the fov of a camera
    //////////////////////////////////////////////////////////////////////////
    class ICameraZoomBehavior
        : public ICameraSubComponent
    {
    public:
        AZ_RTTI(ICameraZoomBehavior, "{D7CED09E-B6B0-42CE-995F-B4FD235C2EC3}", ICameraSubComponent);
        virtual ~ICameraZoomBehavior() = default;
        
        // Modify the zoom (passes in the current zoom, this value should be modified by the behaviour)
		virtual void ModifyZoom( const AZ::EntityId& targetEntityId, float& inOutZoom ) = 0;
    };
} //namespace Camera 