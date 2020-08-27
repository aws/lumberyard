#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class MotionExtractionHandlerRequests
            : public AZ::ComponentBus
        {
        public:

            virtual void HandleMotionExtraction(const AZ::Vector3& deltaPosition, float deltaTime) = 0;
        };
        using MotionExtractionHandlerRequestBus = AZ::EBus<MotionExtractionHandlerRequests>;
    }
}