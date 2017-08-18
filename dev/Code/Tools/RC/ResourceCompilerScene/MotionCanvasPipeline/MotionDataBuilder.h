#pragma once

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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <SceneAPI/SceneCore/Events/CallProcessorBinder.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace MotionCanvasPipeline
{
    struct MotionDataBuilderContext;

    class MotionDataBuilder
        : public AZ::SceneAPI::Events::CallProcessorBinder
    {
    public:
        MotionDataBuilder();
        ~MotionDataBuilder() override = default;

        AZ::SceneAPI::Events::ProcessingResult BuildMotionData(MotionDataBuilderContext& context);
    };

} // MotionCanvasPipeline
#endif //MOTIONCANVAS_GEM_ENABLED