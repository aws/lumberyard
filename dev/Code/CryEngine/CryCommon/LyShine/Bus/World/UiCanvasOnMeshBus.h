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

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <Cry_Math.h>

struct ray_hit;

namespace AzFramework
{
    namespace RenderGeometry
    {
        struct RayResult;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// UiCanvasOnMeshBus
class UiCanvasOnMeshInterface
    : public AZ::ComponentBus
{
public:
    virtual ~UiCanvasOnMeshInterface() {}

#if ENABLE_CRY_PHYSICS
    //! Convert the input collision point into a canvas space position and pass the event and that position
    //! to the UI canvas to handle as a positional input event
    //! \return true if the event was handled
    virtual bool ProcessCollisionInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, int triangleIndex, Vec3 hitPoint) = 0;

    //! Convert the input ray collision into a canvas space position and pass the event and that position
    //! to the UI canvas to handle as a positional input event
    //! \return true if the event was handled
    virtual bool ProcessRayHitInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, const ray_hit& rayHit) = 0;
#else

    //! Convert the input ray collision into a canvas space position and pass the event and that position
    //! to the UI canvas to handle as a positional input event
    //! \return true if the event was handled
    virtual bool ProcessHitInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, const AzFramework::RenderGeometry::RayResult& rayResult) = 0;
#endif // ENABLE_CRY_PHYSICS
};

using UiCanvasOnMeshBus = AZ::EBus<UiCanvasOnMeshInterface>;
