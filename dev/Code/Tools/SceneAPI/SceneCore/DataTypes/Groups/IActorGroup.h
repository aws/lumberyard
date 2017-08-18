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

#include <AzCore/RTTI/Rtti.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IActorGroup
                : public ISceneNodeGroup
            {
            public:
                AZ_RTTI(IActorGroup, "{C86945A8-AEE8-4CFC-8FBF-A20E9BC71348}", ISceneNodeGroup);

                ~IActorGroup() override = default;

                virtual const AZStd::string& GetSelectedRootBone() const = 0;
                virtual bool GetLoadMorphTargets() const = 0;
                virtual bool GetAutoCreateTrajectoryNode() const = 0;

                virtual void SetSelectedRootBone(const AZStd::string& selectedRootBone) = 0;
                virtual void SetLoadMorphTargets(bool loadMorphTargets) = 0;
                virtual void SetAutoCreateTrajectoryNode(bool autoCreateTrajectoryNode) = 0;
            };
        }
    }
}