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

#include <SceneAPI/SceneCore/Events/CallProcessorConnector.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>

struct IConvertContext;

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = SceneAPI::Events;

        class SkinExporter
            : public SceneEvents::CallProcessorConnector
        {
        public:
            explicit SkinExporter(IConvertContext* convertContext);
            ~SkinExporter() override = default;

            SceneEvents::ProcessingResult Process(SceneEvents::ICallContext* context) override;

        private:
            IConvertContext* m_convertContext;
        };
    }
}