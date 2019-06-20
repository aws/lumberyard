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

#include "Water_precompiled.h"
#include <Water/WaterModule.h>

namespace Water
{
    class WaterConverter;

    class WaterEditorModule final
        : public WaterModule
    {
    public:
        AZ_RTTI(WaterEditorModule, "{0C30109F-9D59-419A-86D6-27F51134D012}", WaterModule);
        AZ_CLASS_ALLOCATOR(WaterEditorModule, AZ::SystemAllocator, 0);

        WaterEditorModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams& params) override;
        void OnCrySystemShutdown(ISystem&) override;

    private:
        AZStd::unique_ptr<WaterConverter> m_waterConverter;
    };
}