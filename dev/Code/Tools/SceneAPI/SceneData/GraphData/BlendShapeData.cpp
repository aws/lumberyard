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

#include <AzCore/Casting/numeric_cast.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            namespace DataTypes = SceneAPI::DataTypes;

            BlendShapeData::~BlendShapeData() = default;

            void BlendShapeData::AddPosition(const Vector3& position)
            {
                m_positions.push_back(position);
            }

            unsigned int BlendShapeData::GetVertexCount() const
            {
                return static_cast<unsigned int>(m_positions.size());
            }

            const Vector3& BlendShapeData::GetPosition(unsigned int index) const
            {
                AZ_Assert(index < m_positions.size(), "GetPosition index not in range");
                return m_positions[index];
            }
        } // GraphData
    } // SceneData
} // AZ