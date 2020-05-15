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

#include "Source/LegacyTerrainModule.h"

namespace LegacyTerrain
{
    class LegacyTerrainEditorModule
        : public LegacyTerrainModule
    {
    public:
        AZ_RTTI(LegacyTerrainEditorModule, "{B09BE115-D5E7-479E-90FC-A2255B4BF3DB}", LegacyTerrainModule);
        AZ_CLASS_ALLOCATOR(LegacyTerrainEditorModule, AZ::SystemAllocator, 0);

        LegacyTerrainEditorModule();
    };
}
