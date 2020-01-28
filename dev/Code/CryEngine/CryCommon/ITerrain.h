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

#define COMPILED_TERRAIN_TEXTURE_FILE_NAME "terrain/cover.ctc"

#pragma pack(push,4)

struct IMaterial;

struct ITerrain
{
    virtual ~ITerrain() {}

    struct SurfaceWeight
    {
        static const int WeightCount = 3;
        static const uint8 Undefined = 127;
        static const uint8 Hole = 128;

        SurfaceWeight()
        {
            Ids[0] = Ids[1] = Ids[2] = Undefined;
            Weights[0] = Weights[1] = Weights[2] = 0;
        }

        inline uint8 PrimaryId() const
        {
            return Ids[0];
        }

        inline uint8 PrimaryWeight() const
        {
            return Weights[0];
        }

        uint8 Ids[WeightCount];

        uint8 Weights[WeightCount];

        AUTO_STRUCT_INFO
    };

    virtual void SetTerrainElevation(int left, int bottom, int areaSize, const float* heightmap, int weightmapSize, const SurfaceWeight* surfaceWeightSet) = 0;

    virtual float GetZ(int x, int y) const = 0;
    virtual float GetBilinearZ(float x1, float y1) const = 0;

    virtual SurfaceWeight GetSurfaceWeight(int x, int y) const = 0;

    virtual Vec3 GetTerrainSurfaceNormal(Vec3 vPos, float fRange) = 0;
    virtual void GetTerrainAlignmentMatrix(const Vec3& vPos, const float amount, Matrix33& matrix33) = 0;

    virtual void GetMaterials(AZStd::vector<_smart_ptr<IMaterial>>& materials) = 0;

    virtual bool IsHole(int x, int y) const = 0;
    virtual bool IsMeshQuadFlipped(const int x, const int y, const int nUnitSize) const = 0;

};

#pragma pack(pop)