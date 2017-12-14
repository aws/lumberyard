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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include <AzCore/std/string/string.h>
#include <IXml.h>

#include "MobileDetectSpec.h"

namespace MobileSysInspect
{
    AZStd::unordered_map<AZStd::string, AZStd::string> device_spec_map;
    const float LOW_SPEC_RAM = 1.0f;
    const float MEDIUM_SPEC_RAM = 2.0f;
    const float HIGH_SPEC_RAM = 3.0f;

    namespace Internal
    {
        void LoadDeviceSpecMapping_impl(const char* fileName)
        {
            XmlNodeRef xmlNode = GetISystem()->LoadXmlFromFile(fileName);

            if (!xmlNode)
            {
                return;
            }

            const int fileCount = xmlNode->getChildCount();

            for (int i = 0; i < fileCount; ++i)
            {
                XmlNodeRef fileNode = xmlNode->getChild(i);
                AZStd::string file = fileNode->getAttr("file");

                if (!file.empty())
                {
                    const int modelCount = fileNode->getChildCount();

                    for (int j = 0; j < modelCount; ++j)
                    {
                        XmlNodeRef modelNode = fileNode->getChild(j);
                        AZStd::string model = modelNode->getAttr("model");

                        if (!model.empty())
                        {
                            device_spec_map[model] = file;
                        }
                    }
                }
            }
        }
    }
}
