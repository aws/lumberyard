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

#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            Scene::Scene(const AZStd::string& name)
                : m_name(name)
            {
            }

            Scene::Scene(AZStd::string&& name)
                : m_name(AZStd::move(name))
            {
            }

            void Scene::SetSourceFilename(const AZStd::string& name)
            {
                m_sourceFilename = name;
            }

            void Scene::SetSourceFilename(AZStd::string&& name)
            {
                m_sourceFilename = AZStd::move(name);
            }

            const AZStd::string& Scene::GetSourceFilename() const
            {
                return m_sourceFilename;
            }

            void Scene::SetManifestFilename(const AZStd::string& name)
            {
                m_manifestFilename = name;
            }

            void Scene::SetManifestFilename(AZStd::string&& name)
            {
                m_manifestFilename = AZStd::move(name);
            }

            const AZStd::string& Scene::GetManifestFilename() const
            {
                return m_manifestFilename;
            }

            SceneGraph& Scene::GetGraph()
            {
                return m_graph;
            }

            const SceneGraph& Scene::GetGraph() const
            {
                return m_graph;
            }

            SceneManifest& Scene::GetManifest()
            {
                return m_manifest;
            }

            const SceneManifest& Scene::GetManifest() const
            {
                return m_manifest;
            }

            const AZStd::string& Scene::GetName() const
            {
                return m_name;
            }
        } // Containers
    } // SceneAPI
} // AZ
