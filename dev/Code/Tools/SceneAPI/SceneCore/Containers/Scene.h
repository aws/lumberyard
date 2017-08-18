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
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            // Scenes are used to store the scene's graph the hierarchy and the manifest for meta data as well
            //      as a history of the files used to construct both.
            class Scene
            {
            public:
                AZ_TYPE_INFO(Scene, "{1F2E6142-B0D8-42C6-A6E5-CD726DAA9EF0}");
                
                SCENE_CORE_API explicit Scene(const AZStd::string& name);
                SCENE_CORE_API explicit Scene(AZStd::string&& name);

                SCENE_CORE_API void SetSourceFilename(const AZStd::string& name);
                SCENE_CORE_API void SetSourceFilename(AZStd::string&& name);
                SCENE_CORE_API const AZStd::string& GetSourceFilename() const;

                SCENE_CORE_API void SetManifestFilename(const AZStd::string& name);
                SCENE_CORE_API void SetManifestFilename(AZStd::string&& name);
                SCENE_CORE_API const AZStd::string& GetManifestFilename() const;

                SCENE_CORE_API SceneGraph& GetGraph();
                SCENE_CORE_API const SceneGraph& GetGraph() const;

                SCENE_CORE_API SceneManifest& GetManifest();
                SCENE_CORE_API const SceneManifest& GetManifest() const;

                SCENE_CORE_API const AZStd::string& GetName() const;

            private:
                AZStd::string m_name;
                AZStd::string m_manifestFilename;
                AZStd::string m_sourceFilename;
                SceneGraph m_graph;
                SceneManifest m_manifest;
            };
        } // Containers
    } // SceneAPI
} // AZ
