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

#include "CloudsGem.h"

namespace CloudsGem
{
    /**
     * The CloudsGemEditorModule module class extends the \ref CloudsGemModule
     * by defining editor versions of many components.
     *
     * This is the module used when working in the Editor.
     * The \ref CloudsGemModule is used by the standalone game.
     */
    class CloudsGemEditorModule
        : public CloudsGemModule
    {
    public:
        AZ_RTTI(CloudsGemEditorModule, "{7D7FFD86-8BF7-4083-A8C4-BC8B982CA97F}", CloudsGemModule);

        CloudsGemEditorModule();
        ~CloudsGemEditorModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace LmbrCentral