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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/set.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace DataTypes
        {
            class IGraphObject;
        }
        namespace Events
        {
            class GraphMetaInfo
                : public AZ::EBusTraits
            {
            public:
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
                SCENE_CORE_API static Crc32 s_ignoreVirtualType;

                SCENE_CORE_API GraphMetaInfo();

                virtual ~GraphMetaInfo() = 0;

                // Gets the path to the icon associated with the given object.
                SCENE_CORE_API virtual void GetIconPath(AZStd::string& iconPath, const DataTypes::IGraphObject* target);

                // Provides a short description of the type.
                SCENE_CORE_API virtual void GetToolTip(AZStd::string& toolTip, const DataTypes::IGraphObject* target);

                // Provides a list of string CRCs that indicate the virtual type the given node can act as.
                //      Virtual types are none custom types that are different interpretations of existing types based on
                //      their name or attributes.
                SCENE_CORE_API virtual void GetVirtualTypes(AZStd::set<Crc32>& types, const Containers::Scene& scene, 
                    Containers::SceneGraph::NodeIndex node);

                // Provides a list of string CRCs that indicate all available virtual types.
                SCENE_CORE_API virtual void GetAllVirtualTypes(AZStd::set<Crc32>& types);

                // Converts the virtual type hashed name into a readable name.
                SCENE_CORE_API virtual void GetVirtualTypeName(AZStd::string& name, Crc32 type);
            };

            inline GraphMetaInfo::~GraphMetaInfo() = default;

            using GraphMetaInfoBus = AZ::EBus<GraphMetaInfo>;
        } // Events
    } // SceneAPI
} // AZ
