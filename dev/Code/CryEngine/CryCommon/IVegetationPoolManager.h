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

namespace LegacyProceduralVegetation
{
    //Represents an opaque data type of a sector that contains vegetation instances.
    class VegetationSector;

    class IVegetationPoolManager
    {
    public:
        virtual ~IVegetationPoolManager() {}

        virtual bool IsProceduralVegetationEnabled() const = 0;

        virtual int GetMaxSectors() const = 0 ;
        virtual int GetMaxVegetationInstancesPerSector() const = 0;
        virtual int GetMaxVegetationChunksPerSector() const = 0;
        virtual int GetUsedVegetationChunksCount(int& allCount) const = 0;

        virtual VegetationSector* GetNextAvailableVegetationSector() = 0;
        virtual bool AddVegetationInstanceToSector(VegetationSector* sector, float scale, Vec3 worldPosition, int nGroupId, byte angle) = 0;
        virtual void ReleaseVegetationSector(VegetationSector* sector) = 0;
    };
} //namespace LegacyProceduralVegetation