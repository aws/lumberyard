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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

namespace CD
{
    class FlexibleMesh
        : public CRefCountBase
    {
    public:
        std::vector<SVertex> vertexList;
        std::vector<BrushVec3> normalList;
        std::vector<SMeshFace> faceList;
        std::map<int, int> matId2SubsetMap;

        void Clear()
        {
            vertexList.clear();
            normalList.clear();
            faceList.clear();
            matId2SubsetMap.clear();
        }

        int FindMatIdFromSubsetNum(int nSubsetNum) const
        {
            std::map<int, int>::const_iterator ii = matId2SubsetMap.begin();
            for (; ii != matId2SubsetMap.end(); ++ii)
            {
                if (ii->second == nSubsetNum)
                {
                    return ii->first;
                }
            }
            return -1;
        }

        int AddMatID(int nMatID)
        {
            int nSubsetNumber = 0;
            if (matId2SubsetMap.find(nMatID) != matId2SubsetMap.end())
            {
                nSubsetNumber = matId2SubsetMap[nMatID];
            }
            else
            {
                nSubsetNumber = matId2SubsetMap.size();
                matId2SubsetMap[nMatID] = nSubsetNumber;
            }
            return nSubsetNumber;
        }

        void Reserve(int nSize)
        {
            vertexList.reserve(nSize);
            normalList.reserve(nSize);
            faceList.reserve(nSize);
        }

        bool IsValid() const
        {
            return !vertexList.empty() && !faceList.empty() && !normalList.empty();
        }

        bool IsPassed(const BrushVec3& raySrc, const BrushVec3& rayDir, BrushFloat& outT) const
        {
            for (int i = 0, iFaceCount(faceList.size()); i < iFaceCount; ++i)
            {
                const std::vector<SVertex>& v = vertexList;
                const SMeshFace& f = faceList[i];
                Vec3 vOut;
                if (Intersect::Ray_Triangle(Ray(ToVec3(raySrc), ToVec3(rayDir)), v[f.v[2]].pos, v[f.v[1]].pos, v[f.v[0]].pos, vOut) ||
                    Intersect::Ray_Triangle(Ray(ToVec3(raySrc), ToVec3(rayDir)), v[f.v[0]].pos, v[f.v[1]].pos, v[f.v[2]].pos, vOut))
                {
                    outT = raySrc.GetDistance(ToBrushVec3(vOut));
                    return true;
                }
            }
            return false;
        }
    };

    typedef _smart_ptr<FlexibleMesh> FlexibleMeshPtr;
};