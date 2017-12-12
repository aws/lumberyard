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
#include <AzCore/std/string/string.h>

namespace MotionCanvasPipeline
{
    struct MotionGroupExportContext;

    class MotionGroupExporter
        : public AZ::SceneAPI::Events::CallProcessorBinder
    {
    public:
        explicit MotionGroupExporter();
        ~MotionGroupExporter() override = default;

        AZ::SceneAPI::Events::ProcessingResult ProcessContext(MotionGroupExportContext& context) const;

    protected:
        static const AZStd::string  s_fileExtension;
    };

} // MotionCanvasPipeline
#endif //MOTIONCANVAS_GEM_ENABLED