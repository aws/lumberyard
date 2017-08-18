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

#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <AzCore/RTTI/Rtti.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IEFXMotionGroup
                : public IGroup
            {
            public:
                AZ_RTTI(IEFXMotionGroup, "{1CA400A8-2C3E-423D-B8A3-C457EF88E533}", IGroup);

                ~IEFXMotionGroup() override = default;

                // Ability to specify root bone can be useful if there are multiple skeletons 
                // stored in an fbx file or if the user wants to override the root bone automatically
                // selected by the code.
                virtual const AZStd::string& GetSelectedRootBone() const = 0;
                virtual uint32_t GetStartFrame() const = 0;
                virtual uint32_t GetEndFrame() const = 0;

                virtual void SetSelectedRootBone(const AZStd::string& selectedRootBone) = 0;
                virtual void SetStartFrame(uint32_t frame) = 0;
                virtual void SetEndFrame(uint32_t frame) = 0;
            };
        }
    }
}