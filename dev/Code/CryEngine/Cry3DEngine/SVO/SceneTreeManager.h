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

#ifndef __SCENETREEMANAGER_H__
#define __SCENETREEMANAGER_H__

#if defined(FEATURE_SVO_GI)

class CSvoManager
    : public Cry3DEngineBase
{
public:
    static void Update(const SRenderingPassInfo& passInfo, CCamera& newCam);
    static void UpdateSubSystems(const CCamera& _newCam, CCamera& newCam);
    static void OnFrameStart(const SRenderingPassInfo& passInfo);
    static void OnFrameComplete();
    static void Render();
    static char* GetStatusString(int nLine);
    static void OnDisplayInfo(float& fTextPosX, float& fTextPosY, float& fTextStepY, float fTextScale);
    static bool GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D);
    static void GetSvoBricksForUpdate(PodArray<I3DEngine::SSvoNodeInfo>& arrNodeInfo, float fNodeSize, PodArray<SVF_P3F_C4B_T2F>* pVertsOut);
    static void RegisterMovement(const AABB& objBox);
    static void Voxel_Paint(Vec3 vPaintPos, float fRadius, int nSurfaceTypeId, Vec3 vBaseColor, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, EVoxelEditTarget eTarget, PodArray<IRenderNode*>* pBrushes, float fVoxelSize);
    static void CheckAllocateGlobalCloud();
    static void Release();
};

#endif

#endif