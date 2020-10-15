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
#include <I3DEngine.h>

#pragma warning( push )
#pragma warning( disable: 4373 )      // virtual function overrides differ only by const/volatile qualifiers, mock issue


class MergedMeshesManagerMock
    : public IMergedMeshesManager
{
public:
    MOCK_METHOD2(CompileSectors, bool(DynArray<SInstanceSector>& pSectors, std::vector<struct IStatInstGroup*>* pVegGroupTable));
    MOCK_METHOD2(CompileAreas, bool(DynArray<SMeshAreaCluster>& clusters, int flags));
    MOCK_METHOD3(QueryDensity, size_t(const Vec3& pos, ISurfaceType* (&surfaceTypes)[MMRM_MAX_SURFACE_TYPES], float(&density)[MMRM_MAX_SURFACE_TYPES]));
    MOCK_METHOD0(CalculateDensity, void());
    MOCK_METHOD1(GetUsedMeshes, bool(DynArray<string>& pMeshNames));
    MOCK_CONST_METHOD0(CurrentSizeInVram, size_t());
    MOCK_CONST_METHOD0(CurrentSizeInMainMem, size_t());
    MOCK_CONST_METHOD0(GeomSizeInMainMem, size_t());
    MOCK_CONST_METHOD0(InstanceSize, size_t());
    MOCK_CONST_METHOD0(SpineSize, size_t());
    MOCK_CONST_METHOD0(InstanceCount, size_t());
    MOCK_CONST_METHOD0(VisibleInstances, size_t());
    MOCK_METHOD1(PrepareSegmentData, void(const AABB& aabb));
    MOCK_METHOD0(GetSegmentNodeCount, int());
    MOCK_METHOD1(GetCompiledDataSize, int(uint32 index));
    MOCK_METHOD5(GetCompiledData, bool(uint32 index, byte* pData, int nSize, string* pName, std::vector<struct IStatInstGroup*>** ppStatInstGroupTable));
    MOCK_METHOD3(AddDynamicInstance, IRenderNode* (const SInstanceSample&, IRenderNode** ppNode, bool bRegister));
};

#pragma warning( pop )
