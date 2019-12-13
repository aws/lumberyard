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

#include <IVegetationPoolManager.h>
#include <TPool.h>

class CVegetation;

namespace LegacyProceduralVegetation
{

#define MAX_PROC_OBJ_CHUNKS_NUM 128

    //! A VegetionChunk owns a fixed size array of CVegetation pointers.
    //! In other words, it owns a Chunk of pointers.
    struct VegetationChunk
        : public Cry3DEngineBase
    {
        CVegetation* m_pInstances;
        
        VegetationChunk()
        {
            const int maxVegetationInstancesPerChunk = GetCVars()->e_ProcVegetationMaxObjectsInChunk;
            m_pInstances = new CVegetation[maxVegetationInstancesPerChunk];
        }

        ~VegetationChunk()
        {
            delete[] m_pInstances;
        }

        void GetMemoryUsage(ICrySizer* pSizer) const;
    };

    typedef TPool<VegetationChunk> VegetationChunksPool;

    //! A VegetationSector owns a set of VegetationChunk pointers.
    //! Each VegetationChunk these pointers are pointing to are stored
    //! In a pool of VegetationChunks
    class VegetationSector
        : public Cry3DEngineBase
    {
    public:
        VegetationSector()
        {
            m_allocatedVegetationObjectsCount = 0;
            m_vegetationChunks.PreAllocate(32);
        }

        ~VegetationSector();


        CVegetation* AllocateVegetationInstance(VegetationChunksPool& vegetationChunksPool);
        void ReleaseAllVegetationInstances(VegetationChunksPool& vegetationChunksPool);
        
        //! Returns the number of allocated CVegetation instances inside this sector.
        //! In addition returns the number of VegetationChunks assigned to this sector
        //! in @vegetationChunksCount
        int GetVegetationInstancesCount(int& vegetationChunksCount)
        {
            vegetationChunksCount = m_vegetationChunks.Count();
            return m_allocatedVegetationObjectsCount;
        }
        
        void GetMemoryUsage(ICrySizer* pSizer) const;

    protected:
        //! The VegetationChunks that are assigned to this sector.
        PodArray<VegetationChunk*> m_vegetationChunks;

        //! How many CVegetation objects have been allocated in this sector so far.
        int m_allocatedVegetationObjectsCount;
    };

    typedef TPool<VegetationSector> VegetationSectorsPool;

    class VegetationPoolManager : public IVegetationPoolManager
        , public Cry3DEngineBase
    {
    public:
        VegetationPoolManager();
        virtual ~VegetationPoolManager();

        ///////////////////////////////////////////////////////////////////////
        //   IVegetationPoolManager START
        bool IsProceduralVegetationEnabled() const override
        {
            const int isEnabled = GetCVars()->e_ProcVegetation;
            return isEnabled != 0;
        }

        int GetMaxSectors() const override
        { 
            const int maxSectorsInCache = GetCVars()->e_ProcVegetationMaxSectorsInCache;
            return maxSectorsInCache;
        }

        int GetMaxVegetationInstancesPerSector() const override
        {
            const int maxSectorsInCache = GetCVars()->e_ProcVegetationMaxSectorsInCache;
            const int maxChunksPersector = MAX_PROC_OBJ_CHUNKS_NUM / maxSectorsInCache;
            const int maxVegetationInstancesPerChunk = GetCVars()->e_ProcVegetationMaxObjectsInChunk;
            return maxChunksPersector * maxVegetationInstancesPerChunk;
        }

        int GetMaxVegetationChunksPerSector() const override
        {
            const int maxSectorsInCache = GetCVars()->e_ProcVegetationMaxSectorsInCache;
            return MAX_PROC_OBJ_CHUNKS_NUM / maxSectorsInCache;
        }

        int GetUsedVegetationChunksCount(int& allCount) const override
        { 
            const int count = m_vegetationChunksPool->GetUsedInstancesCount(allCount);
            return count;
        }

        VegetationSector* GetNextAvailableVegetationSector() override
        {
            VegetationSector* sector = m_vegetationSectorsPool->GetObject();
            return sector;
        }

        bool AddVegetationInstanceToSector(VegetationSector* sector, float scale, Vec3 worldPosition, int nGroupId, byte angle) override;

        void ReleaseVegetationSector(VegetationSector* sector) override
        {
            sector->ReleaseAllVegetationInstances(*m_vegetationChunksPool);
            m_vegetationSectorsPool->ReleaseObject(sector);
        }

        //   IVegetationPoolManager END
        ///////////////////////////////////////////////////////////////////////

        void GetMemoryUsage(ICrySizer* pSizer) const;
        VegetationSectorsPool& GetVegetationSectorPool() { return *m_vegetationSectorsPool; }
        VegetationChunksPool& GetVegetationChunksPool() { return *m_vegetationChunksPool; }

    private:
        VegetationSectorsPool* m_vegetationSectorsPool;
        VegetationChunksPool* m_vegetationChunksPool;
 
        //! Exists for the single purpose of enabling ~VegetationSector to fully
        //! release allocated memory.
        friend class VegetationSector;
        static VegetationPoolManager* s_poolManagerPtr;
    };

} //namespace LegacyProceduralVegetation
