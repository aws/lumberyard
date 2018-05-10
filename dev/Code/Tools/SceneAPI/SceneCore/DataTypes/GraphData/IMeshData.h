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

#ifndef AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMESHDATA_H_
#define AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMESHDATA_H_

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMeshData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IMeshData, "{B94A59C0-F3A5-40A0-B541-7E36B6576C4A}", IGraphObject);

                struct Face
                {
                    unsigned int vertexIndex[3];

                    inline bool operator==(const Face& rhs) const
                    {
                        return (vertexIndex[0] == rhs.vertexIndex[0] && vertexIndex[1] == rhs.vertexIndex[1] &&
                            vertexIndex[2] == rhs.vertexIndex[2]);
                    }

                    inline bool operator!=(const Face& rhs) const
                    {
                        return (vertexIndex[0] != rhs.vertexIndex[0] || vertexIndex[1] != rhs.vertexIndex[1] ||
                            vertexIndex[2] != rhs.vertexIndex[2]);
                    }
                };

                virtual ~IMeshData() override = default;

                virtual unsigned int GetVertexCount() const = 0;
                virtual bool HasNormalData() const = 0;

                //1 to 1 mapping from position to normal (each corner of triangle represented)
                virtual const AZ::Vector3& GetPosition(unsigned int index) const = 0;
                virtual const AZ::Vector3& GetNormal(unsigned int index) const = 0;

                virtual unsigned int GetFaceCount() const = 0;
                virtual const Face& GetFaceInfo(unsigned int index) const = 0;
                virtual unsigned int GetFaceMaterialId(unsigned int index) const = 0;

                //  0 <= vertexIndex < GetVertexCount().
                virtual int GetControlPointIndex(int vertexIndex) const = 0;

                // Returns number of unique control points used in the mesh.  Here, "used"
                // means it is actually referenced by some polygon in the mesh.
                virtual size_t GetUsedControlPointCount() const = 0;

                // If the control point index specified is indeed used by the mesh, returns a unique value
                // in the range [0,  GetUsedControlPointCount()). Otherwise, returns -1.
                virtual int GetUsedPointIndexForControlPoint(int controlPointIndex) const = 0;

                virtual unsigned int GetVertexIndex(int faceIndex, int vertexIndexInFace) const = 0;
                static const int s_invalidMaterialId = 0;
            };
        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ

#endif // AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMESHDATA_H_