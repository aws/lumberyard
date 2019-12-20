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

#include <Tests/SystemComponentFixture.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace EMotionFX
{
    class SampleGameFixture : public SystemComponentFixtureWithCatalog
    {
    protected:

        virtual AZStd::string GetAssetFolder() const override
        {
            // This path points to asset under sample's project.
            // If you want to test asset under different projects, you need to change the path in here.
            // execute path: (workspace)/dev/(test_outputfolder)
            // asset path(pc): (workspace)/dev/Cache/SamplesProject/pc/samplesproject/
            AZStd::string assetPath = mApp.GetExecutableFolder();
            const size_t pos = AzFramework::StringFunc::Find(assetPath.c_str(), '/', 1, true, false);
            if (pos != AZStd::string::npos)
            {
                assetPath.resize(pos);
                assetPath += "/Cache/SamplesProject/pc/samplesproject";
            }
            return assetPath;
        }
    };
}