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

#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBinder.h>

struct IConvertContext;
class CContentCGF;

namespace AZ
{
    namespace RC
    {
        struct SkinGroupExportContext;

        class SkinLodExporter
            : public AZ::SceneAPI::Events::CallProcessorBinder
        {
        public:
            SkinLodExporter(IAssetWriter* writer, IConvertContext* convertContext);
            ~SkinLodExporter() override = default;

            SceneAPI::Events::ProcessingResult ProcessContext(SkinGroupExportContext& context) const;

            static const AZStd::string s_fileExtension;

        protected:
            IAssetWriter* m_assetWriter;
            IConvertContext* m_convertContext;
        };
    }
}