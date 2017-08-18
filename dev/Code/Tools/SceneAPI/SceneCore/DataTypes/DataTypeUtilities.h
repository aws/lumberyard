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
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class SceneManifest;
        }
        namespace DataTypes
        {
            namespace Utilities
            {

                // Checks if the given name is already in use by another manifest entry of a (derived) type.
                template<typename T>
                bool IsNameAvailable(const AZStd::string& name, const Containers::SceneManifest& manifest);
                // Checks if the given name is already in use by another manifest entry of a (derived) type.
                SCENE_CORE_API bool IsNameAvailable(const AZStd::string& name, const Containers::SceneManifest& manifest, const Uuid& type);

                // Creates a unique name for given (derived) type starting with the given base name.
                template<typename T>
                AZStd::string CreateUniqueName(const AZStd::string& baseName, const Containers::SceneManifest& manifest);
                // Creates a unique name for given (derived) type starting with the given base name.
                SCENE_CORE_API AZStd::string CreateUniqueName(const AZStd::string& baseName, const Containers::SceneManifest& manifest, const Uuid& type);

            } // Utilities
        } // DataTypes
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.inl>