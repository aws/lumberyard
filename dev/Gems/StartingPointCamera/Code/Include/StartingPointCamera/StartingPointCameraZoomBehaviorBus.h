#pragma once
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

namespace Camera
{
    class CameraZoomBehaviorRequest
        : public AZ::ComponentBus 
    {
    public: 
        virtual ~CameraZoomBehaviorRequest() = default;

        virtual void SetZoomTarget(float zoomTarget) = 0;
        virtual void SetTimeToZoom(float timeToZoom) = 0;
        virtual void StartZoom() = 0;
        virtual void StopZoom() = 0;
    };
    using CameraZoomBehaviorBus = AZ::EBus<CameraZoomBehaviorRequest>;
} //namespace Camera