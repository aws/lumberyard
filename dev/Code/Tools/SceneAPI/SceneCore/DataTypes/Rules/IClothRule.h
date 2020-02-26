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

#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            /// This class is the interface for the cloth rule (aka cloth modifier).
            /// It exposes functions to access the data the user can set in a cloth rule.
            class IClothRule
                : public IRule
            {
            public:
                AZ_RTTI(IClothRule, "{5185510A-50BF-418A-ACB4-1A9E014C7E43}", IRule);

                ~IClothRule() override = default;

                /// Returns the name of the mesh node inside the FBX that will be exported as cloth.
                virtual const AZStd::string& GetMeshNodeName() const = 0;

                /// Returns the name of the vertex color stream inside the FBX that contains the cloth inverse masses.
                virtual const AZStd::string& GetVertexColorStreamName() const = 0;

                /// Returns whether or not the vertex color stream was explicitly disabled by the user.
                virtual bool IsVertexColorStreamDisabled() const = 0;

            };
        } //namespace DataTypes
    } //namespace SceneAPI
}  // namespace AZ
