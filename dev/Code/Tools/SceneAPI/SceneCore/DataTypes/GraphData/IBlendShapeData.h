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

#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IBlendShapeData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IBlendShapeData, "{55E7384D-9333-4C51-BC91-E90CAC2C30E2}", IGraphObject);

                virtual ~IBlendShapeData() override = default;

                virtual unsigned int GetVertexCount() const = 0;
                virtual const AZ::Vector3& GetPosition(unsigned int index) const = 0;

            };
        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ
