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

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <RC/ResourceCompilerScene/Common/ExportContextGlobal.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IAnimationGroup;
        }
        namespace Events
        {
            class ExportProductList;
        }
    }

    namespace RC
    {
        // Called to export a specific Animation (Caf) Group
        struct CafGroupExportContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(CafGroupExportContext, "{43D58BC7-2AE4-49F6-BDEF-4A1297FEB97A}", SceneAPI::Events::ICallContext);

            CafGroupExportContext(SceneAPI::Events::ExportEventContext& parent,
                const SceneAPI::DataTypes::IAnimationGroup& group, Phase phase);
            CafGroupExportContext(SceneAPI::Events::ExportProductList& products, const SceneAPI::Containers::Scene& scene, 
                const AZStd::string& outputDirectory, const SceneAPI::DataTypes::IAnimationGroup& group, Phase phase);
            CafGroupExportContext(const CafGroupExportContext& copyContent, Phase phase);
            CafGroupExportContext(const CafGroupExportContext& copyContent) = delete;
            ~CafGroupExportContext() override = default;

            CafGroupExportContext& operator=(const CafGroupExportContext& other) = delete;

            SceneAPI::Events::ExportProductList& m_products;
            const SceneAPI::Containers::Scene& m_scene;
            const AZStd::string& m_outputDirectory;
            const SceneAPI::DataTypes::IAnimationGroup& m_group;
            const Phase m_phase;
        };
    }
}