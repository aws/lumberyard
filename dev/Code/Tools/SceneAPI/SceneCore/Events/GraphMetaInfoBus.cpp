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

#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            Crc32 GraphMetaInfo::s_ignoreVirtualType = AZ_CRC("Ignore", 0x0d88d6e2);

            GraphMetaInfo::GraphMetaInfo()
            {
            }

            void GraphMetaInfo::GetIconPath(AZStd::string& /*iconPath*/, const DataTypes::IGraphObject* /*target*/)
            {
            }

            void GraphMetaInfo::GetToolTip(AZStd::string& /*toolTip*/, const DataTypes::IGraphObject* /*target*/)
            {
            }

            void GraphMetaInfo::GetVirtualTypes(AZStd::set<Crc32>& /*types*/, const Containers::Scene& /*scene*/,
                Containers::SceneGraph::NodeIndex /*node*/)
            {
            }

            void GraphMetaInfo::GetAllVirtualTypes(AZStd::set<Crc32>& /*types*/)
            {
            }

            void GraphMetaInfo::GetVirtualTypeName(AZStd::string& /*name*/, Crc32 /*type*/)
            {
            }
        } // Events
    } // SceneAPI
} // AZ
