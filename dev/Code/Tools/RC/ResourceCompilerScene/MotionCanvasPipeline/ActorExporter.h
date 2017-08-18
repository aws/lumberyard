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

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            class ExportEventContext;
        }
    }
}

namespace MotionCanvasPipeline
{
    namespace SceneEvents = AZ::SceneAPI::Events;

    class ActorExporter
        : public SceneEvents::CallProcessorBinder
    {
    public:
        ActorExporter();
        ~ActorExporter() override = default;

        SceneEvents::ProcessingResult ProcessContext(SceneEvents::ExportEventContext& context) const;

    private:
    };

} // MotionCanvasPipeline
#endif //MOTIONCANVAS_GEM_ENABLED