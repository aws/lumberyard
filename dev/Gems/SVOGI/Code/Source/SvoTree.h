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

#ifndef __SCENETREE_H__
#define __SCENETREE_H__
#include <AzCore/std/containers/unordered_map.h>
#if defined(FEATURE_SVO_GI)

#pragma pack(push,4)

#define SCENE_SCAN_FILE_VERSION 2
#define SVO_FILE_VERSION 1
#define GLOBAL_CLOUD_TREE_FILE_NAME "TREE.DAT"
#define GLOBAL_CLOUD_MESH_FILE_NAME "CCM.DAT"
#define GLOBAL_CLOUD_PVS_FILE_NAME  "PVS.DAT"

typedef AZStd::unordered_map<uint64, std::pair<byte, byte> > PvsMap;
typedef std::set<class CVoxelSegment*> VsSet;

const int nVoxStreamQueueMaxSize = 12;
const float fMaxScanDistance = 256;

class CSvoNode
{
public:

    CSvoNode(const AABB& box, CSvoNode* pParent);
    ~CSvoNode();

    static void* operator new (size_t);
    static void operator delete(void* ptr);

    void CheckAllocateChilds();
    void DeleteChilds();
    void Render(PodArray<struct SPvsItem>* pSortedPVS, uint64 nNodeKey, CRenderObject* pObj, int nTreeLevel, PodArray<SVF_P3F_C4B_T2F>& arrVertsOut, PodArray<class CVoxelSegment*> arrForStreaming[nVoxStreamQueueMaxSize][nVoxStreamQueueMaxSize]);
    bool IsStreamingInProgress();
    void GetTrisInAreaStats(int& nTrisCount, int& nVertCount, int& nTrisBytes, int& nVertBytes, int& nMaxVertPerArea, int& nMatsCount);
    void GetVoxSegMemUsage(int& nAllocated);
    AABB GetChildBBox(int nChildId);
    bool TryInsertPoint(const Vec3& vPos, const ColorB& vNormalB, float fAmbFactor, const ColorB& vColor, int32& nInserted, float fDot, PodArray<Vec3>& arrScanPoints, bool bImportDVB = false);
    void CheckAllocateSegment(int nLod);
    void RayTrace(Lineseg& rLS, Vec3 vLineDir, VsSet& visitedNodes, ColorB& hitColor, ColorB& hitNormal);
    void OnStatLightsChanged(const AABB& objBox);
    class CVoxelSegment* AllocateSegment(int nCloudId, int nStationId, int nLod, EFileStreamingStatus eStreamingStatus, bool bDroppedOnDisk);
    void ExportRenderData(int& nNodesProcessed, int nNodesAll, FILE* pFileToWrite, const int nTreeLevel, const int nTreeLevelToExport);
    void TestRefCount(int& nLeaks);
    void Load(int*& pData, int& nNodesCreated, CSvoNode* pParent, AABB* pAreaBox);
    void Save(PodArray<int>& arrData, AABB* pAreaBox);
    void CutChilds();
    bool CheckReadyForRendering(int nTreeLevel, PodArray<CVoxelSegment*> arrForStreaming[nVoxStreamQueueMaxSize][nVoxStreamQueueMaxSize]);
    void CollectResetVisNodes(PvsMap& arrNodes, uint64 nNodeKey);
    void StartBuildAOTasks(int& nNodesProcessed, int nNodesAll);
    void ComputeAO(int nTID);
    void Task_Compute(int nTID) { ComputeAO(nTID); }
    void Task_Store();
    static void Task_OnTaskConveyorFinish() {}
    void BuildMips(int& nNodesProcessed, int nNodesAll);
    void BuildParentMips();
    void FixGaps(int& nNodesProcessed, int nNodesAll);
    void FixSeams(int& nNodesProcessed, int nNodesAll, int nTreeLevelCur);
    void FixSeamsFromNeigbours(int nTreeLevelCur);
    CSvoNode* FindNodeByPosition(const Vec3& vPosWS, int nTreeLevelToFind, int nTreeLevelCur);
    void FixGapsInChilds();
    bool IsNodeGoodForExport();
    void UpdateNodeRenderDataPtrs();
    void RegisterMovement(const AABB& objBox);
    Vec3i GetStatGeomCheckSumm();
    CSvoNode* FindNodeByPoolAffset(int nAllocatedAtlasOffset);

    AABB m_nodeBox;
    CSvoNode** m_ppChilds;
    CSvoNode* m_pParent;
    CVoxelSegment* m_pSeg;
    uint m_nRequestSegmentUpdateFrametId;
    bool m_arrChildNotNeeded[8];
    bool m_bForceRecreate;
};

class CPointTreeNode
{
public:
    bool TryInsertPoint(int nId, const Vec3& vPos, const AABB& nodeBox, int nRecursionLevel = 0);
    bool IsThereAnyPointInTheBox(const AABB& testBox, const AABB& nodeBox);
    bool GetAllPointsInTheBox(const AABB& testBox, const AABB& nodeBox, PodArray<int>& arrIds);
    AABB GetChildBBox(int nChildId, const AABB& nodeBox);
    void Clear();
    CPointTreeNode() { m_ppChilds = 0; m_pPoints = 0; }
    ~CPointTreeNode() { Clear(); }

    struct SPointInfo
    {
        Vec3 vPos;
        int nId;
    };
    PodArray<SPointInfo>* m_pPoints;
    CPointTreeNode** m_ppChilds;
};

class CSvoEnv
    : public Cry3DEngineBase
{
public:

    CSvoEnv(const AABB& worldBox);
    ~CSvoEnv();
    bool GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D);
    void GetSvoBricksForUpdate(PodArray<I3DEngine::SSvoNodeInfo>& arrNodeInfo, float fNodeSize, PodArray<SVF_P3F_C4B_T2F>* pVertsOut);
    bool Render();
    void CheckUpdateMeshPools();
    int GetWorstPointInSubSet(const int nStart, const int eEnd);
    void StartupStreamingTimeTest(bool bDone);
    void OnLevelGeometryChanged();
    void DrawLightProbeDebug(Vec3 vPos, int nTexId);
    void ReconstructTree(bool bMultiPoint);
    void LoadTree(int& nNodesCreated);
    void DetectMovement_StaticGeom();
    void DetectMovement_StatLights();
    void CollectLights();

    PodArray<I3DEngine::SLightTI> m_lightsTI_S, m_lightsTI_D;
    AABB m_aabbLightsTI_D;
    ITexture* m_pGlobalSpecCM;
    float m_fGlobalSpecCM_Mult;
    CSvoNode* m_pSvoRoot;
    bool m_bReady;
    PodArray<CVoxelSegment*> m_arrForStreaming[nVoxStreamQueueMaxSize][nVoxStreamQueueMaxSize];
    int m_nDebugDrawVoxelsCounter;
    int m_nNodeCounter;
    int m_nDynNodeCounter;
    int m_nDynNodeCounter_DYNL;
    PodArray<CVoxelSegment*> m_arrForBrickUpdate[16];
    CryCriticalSection m_csLockTree;
    float m_fStreamingStartTime;
    float m_fSvoFreezeTime;
    int m_arrVoxelizeMeshesCounter[2];
    Matrix44 m_matDvrTm;
    AABB m_worldBox;
    PodArray<SVF_P3F_C4B_T2F> m_arrSvoProxyVertices;
    double m_prevCheckVal;
    bool m_bFirst_SvoFreezeTime;
    bool m_bFirst_StartStreaming;
    bool m_bStreamingDonePrev;

    int m_nTexOpasPoolId;
    int m_nTexNodePoolId;

#ifdef FEATURE_SVO_GI_ALLOW_HQ
#ifdef FEATURE_SVO_GI_USE_MESH_RT
    int m_nTexTrisPoolId;
#endif
    int m_nTexRgb0PoolId;
    int m_nTexRgb1PoolId;
    int m_nTexDynlPoolId;
    int m_nTexRgb2PoolId;
    int m_nTexRgb3PoolId;
    int m_nTexRgb4PoolId;
    int m_nTexNormPoolId;
    int m_nTexAldiPoolId;
#endif

    ETEX_Format m_nVoxTexFormat;
    TDoublyLinkedList<CVoxelSegment> m_arrSegForUnload;
    CPoolAllocator<struct SCpuBrickItem, 128> m_cpuBricksAllocator;
    CPoolAllocator<CSvoNode, 128> m_nodeAllocator;

#ifdef FEATURE_SVO_GI_ALLOW_HQ
    PodArrayRT<ColorB> m_arrRTPoolTexs;
    PodArrayRT<Vec4>   m_arrRTPoolTris;
    PodArrayRT<ColorB> m_arrRTPoolInds;
#endif
};

#pragma pack(pop)

#endif

#endif
