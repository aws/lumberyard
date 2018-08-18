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

#ifndef CRYINCLUDE_CRYANIMATION_GLOBALANIMATIONHEADERAIM_H
#define CRYINCLUDE_CRYANIMATION_GLOBALANIMATIONHEADERAIM_H
#pragma once

#include "GlobalAnimationHeader.h"
#include "Controller.h"
#include "ControllerPQLog.h"
#include "ControllerTCB.h"

#include "IStreamEngine.h"
#include <NameCRCHelper.h>

#define AIM_POSES (1)
#define PRECISION (0.001)

class CDefaultSkeleton;

struct AimIKPose
{
    DynArray<Quat> m_arrRotation;
    DynArray<Vec3> m_arrPosition;

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_arrRotation);
        pSizer->AddObject(m_arrPosition);
    }
};


struct QuadIndices
{
    uint8 i0, i1, i2, i3;
    Vec4    w0, w1, w2, w3;
    ColorB col;
    Vec3 height;
};



//////////////////////////////////////////////////////////////////////////
// this is the animation information on the module level (not on the per-model level)
// it doesn't know the name of the animation (which is model-specific), but does know the file name
// Implements some services for bone binding and ref counting
struct GlobalAnimationHeaderAIM
    : public GlobalAnimationHeader
{
    friend class CAnimationManager;
    friend class CAnimationSet;

    GlobalAnimationHeaderAIM ()
    {
        m_FilePathCRC32     =   0;
        m_nRef_by_Model     = 0;
        m_nRef_at_Runtime = 0;
        m_nTouchedCounter = 0;

        m_fStartSec             = -1;       // Start time in seconds.
        m_fEndSec                   = -1;       // End time in seconds.
        m_fTotalDuration    = -1;       //asset-features

        m_nControllers  = 0;
        m_AnimTokenCRC32 = 0;
        m_MiddleAimPoseRot.SetIdentity();
        m_MiddleAimPose.SetIdentity();
        m_nExist                =   0;

        m_fSampleRate           = ANIMATION_30Hz;
    }


    ~GlobalAnimationHeaderAIM()
    {
        ClearControllers();
    };



    void AddRef()
    {
        ++m_nRef_by_Model;
    }

    void Release()
    {
        if (!--m_nRef_by_Model)
        {
            m_nControllers = 0;
            m_arrController.clear();
        }
    }


#ifdef _DEBUG
    // returns the maximum reference counter from all controllers. 1 means that nobody but this animation
    // structure refers to them
    int MaxControllerRefCount()
    {
        if (m_arrController.size() == 0)
        {
            return 0;
        }
        int nMax = m_arrController[0]->NumRefs();
        for (int i = 0; i < m_nControllers; ++i)
        {
            if (m_arrController[i]->NumRefs() > nMax)
            {
                nMax = m_arrController[i]->NumRefs();
            }
        }
        return nMax;
    }
#endif


    ILINE uint32 IsAssetLoaded() const
    {
        return m_nControllers;
    }
    ILINE uint32 IsAssetOnDemand() const
    {
        return 0;
    }

    f32 GetSampleRate() const { return m_fSampleRate; }

    //---------------------------------------------------------------
    IController* GetControllerByJointCRC32(uint32 nControllerID) const
    {
        for (uint32 i = 0; i < m_nControllers; i++)
        {
            uint32 nCID = m_arrController[i]->GetID();
            if (nControllerID == nCID)
            {
                return m_arrController[i];
            }
        }
        return nullptr;
    }

    // Convert time basis from interpolation time [0,1] to Key Time
    // Note: ntime outside the range [0,1] generates extrapolated key time
    f32 NTime2KTime(f32 ntime) const
    {
        return GetSampleRate() * (m_fStartSec + ntime * (m_fEndSec - m_fStartSec));
    }

    //count how many position controllers this animation has
    uint32 GetTolalPosKeys() const
    {
        uint32 pos = 0;
        for (uint32 i = 0; i < m_nControllers; i++)
        {
            pos += (m_arrController[i]->GetPositionKeysNum() != 0);
        }
        return pos;
    }

    //count how many rotation controllers this animation has
    uint32 GetTolalRotKeys() const
    {
        uint32 rot = 0;
        for (uint32 i = 0; i < m_nControllers; i++)
        {
            rot += (m_arrController[i]->GetRotationKeysNum() != 0);
        }
        return rot;
    }

    size_t SizeOfAIM() const
    {
        size_t nSize = 0;//sizeof(*this);
        size_t nTemp00 = m_FilePath.capacity();
        nSize += nTemp00;
        size_t nTemp08 = m_arrAimIKPosesAIM.get_alloc_size();
        nSize += nTemp08;
        uint32 numAimPoses = m_arrAimIKPosesAIM.size();
        for (size_t i = 0; i < numAimPoses; ++i)
        {
            nSize += m_arrAimIKPosesAIM[i].m_arrRotation.get_alloc_size();
            nSize += m_arrAimIKPosesAIM[i].m_arrPosition.get_alloc_size();
        }

        size_t nTemp09 = m_arrController.get_alloc_size();
        nSize += nTemp09;
        for (uint16 i = 0; i < m_nControllers; ++i)
        {
            nSize += m_arrController[i]->SizeOfController();
        }
        return nSize;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_FilePath);
        pSizer->AddObject(m_arrController);
        pSizer->AddObject(m_arrAimIKPosesAIM);

        for (int i = 0; i < m_nControllers; ++i)
        {
            pSizer->AddObject(m_arrController[i].get());
        }
    }


    size_t GetControllersCount()
    {
        return m_nControllers;
    }

    void ClearControllers()
    {
        ClearAssetRequested();
        m_nControllers = 0;
        m_arrController.clear();
    }


    uint32 LoadAIM();
    bool LoadChunks(IChunkFile* pChunkFile);
    bool ReadMotionParameters (IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadController(IChunkFile::ChunkDesc* pChunkDesc, uint32 bLoadOldChunks);
    bool ReadTiming (IChunkFile::ChunkDesc* pChunkDesc);

    void ProcessDirectionalPoses(const CDefaultSkeleton* pDefaultSkeleton, const DynArray<DirectionalBlends>& rDirBlends, const DynArray<SJointsAimIK_Rot>& rRot, const DynArray<SJointsAimIK_Pos>& rPos);
    void ProcessPoses(const IDefaultSkeleton* pDefaultSkeleton, const char* szAnimationName);




    Vec3 Debug_PolarCoordinate(const Quat& q) const;
    uint32 Debug_AnnotateExamples2(uint32 numPoses, QuadIndices* arrQuat) const;
    void Debug_NLerp2AimPose(const CDefaultSkeleton* pDefaultSkeleton, const SJointsAimIK_Rot* rRot, uint32 numRotJoints, const SJointsAimIK_Pos* rPos, uint32 numPosJoints, const QuatT* arrRelPose0, const QuatT* arrRelPose1, f32 t, QuatT* arrAbsPose) const;
    void Debug_Blend4AimPose(const CDefaultSkeleton* pDefaultSkeleton, const SJointsAimIK_Rot* rRot, uint32 numRotJoints, const SJointsAimIK_Pos* rPos, uint32 numPosJoints, int8 i0, int8 i1, int8 i2, int8 i3, const Vec4& w, QuatT* arrRelPose, QuatT* arrAbsPose) const;

private:
    uint32 FlagsSanityFilter(uint32 flags);

public:
    uint16 m_nRef_by_Model;    //counter how many models are referencing this animation
    uint16 m_nRef_at_Runtime;  //counter how many times we use this animation at run-time at the same time (need for streaming)
    uint16 m_nTouchedCounter;  //do we use this asset at all?
    uint16 m_nControllers;
    f32 m_fStartSec;                    //Start time in seconds.
    f32 m_fEndSec;                      //End time in seconds.
    f32 m_fTotalDuration;           //asset-feature: total duration in seconds.
    DynArray<IController_AutoPtr> m_arrController;
    uint32 m_AnimTokenCRC32;
    DynArray<AimIKPose> m_arrAimIKPosesAIM; //if this animation contains aim-poses, we store them here

    float m_fSampleRate;

    uint64 m_nExist;
    Quat m_MiddleAimPoseRot;
    Quat m_MiddleAimPose;
    CHUNK_GAHAIM_INFO::VirtualExample m_PolarGrid[CHUNK_GAHAIM_INFO::XGRID * CHUNK_GAHAIM_INFO::YGRID];
    DynArray<CHUNK_GAHAIM_INFO::VirtualExampleInit2> VE2;
};




struct VExampleInit
{
    struct VirtualExampleInit1
    {
        f32 m_fSmalest;
        uint8 i0, i1, i2, i3;
        f64  w0, w1, w2, w3;
    };

    f64 m_fSmallest;
    uint32 m_nIterations;
    Vec4d m_Weight;

    int32  m_arrParentIdx[MAX_JOINT_AMOUNT];
    QuatT  m_arrDefaultRelPose[MAX_JOINT_AMOUNT];
    uint32 m_arrJointCRC32[MAX_JOINT_AMOUNT];

    QuatTd m_arrRelPose[MAX_JOINT_AMOUNT];
    QuatTd m_arrAbsPose[MAX_JOINT_AMOUNT];
    QuatTd m_arrRelPose0[MAX_JOINT_AMOUNT];
    QuatTd m_arrAbsPose0[MAX_JOINT_AMOUNT];
    QuatTd m_arrRelPose1[MAX_JOINT_AMOUNT];
    QuatTd m_arrAbsPose1[MAX_JOINT_AMOUNT];
    QuatTd m_arrRelPose2[MAX_JOINT_AMOUNT];
    QuatTd m_arrAbsPose2[MAX_JOINT_AMOUNT];
    QuatTd m_arrRelPose3[MAX_JOINT_AMOUNT];
    QuatTd m_arrAbsPose3[MAX_JOINT_AMOUNT];
    VirtualExampleInit1 PolarGrid[CHUNK_GAHAIM_INFO::XGRID * CHUNK_GAHAIM_INFO::YGRID];

    void Init(const CDefaultSkeleton* pDefaultSkeleton, const DynArray<SJointsAimIK_Rot>& rRot, const DynArray<SJointsAimIK_Pos>& rPos, GlobalAnimationHeaderAIM& rAIM, int nWeaponBoneIdx, uint32 numAimPoses);
    void CopyPoses2(const DynArray<SJointsAimIK_Rot>& rRot, const DynArray<SJointsAimIK_Pos>& rPos, GlobalAnimationHeaderAIM& rAIM, uint32 numPoses, uint32 skey);
    void RecursiveTest(const Vec2d& ControlPoint, GlobalAnimationHeaderAIM& rAIM, const DynArray<SJointsAimIK_Rot>& rRot, const DynArray<SJointsAimIK_Pos>& rPos, int nWBone, int i0, int i1, int i2, int i3, const Vec4d& w0, const Vec4d& w1, const Vec4d& w2, const Vec4d& w3);
    uint32 PointInQuat(const Vec2d& ControlPoint, GlobalAnimationHeaderAIM& rAIM, const DynArray<SJointsAimIK_Rot>& rRot, const DynArray<SJointsAimIK_Pos>& rPos, int nWBone, int i0, int i1, int i2, int i3, const Vec4d& w0, const Vec4d& w1, const Vec4d& w2, const Vec4d& w3);

    uint32 LinesegOverlap2D(const Planed& plane0, const Vec2d& ls0, const Vec2d& le0,   const Vec2d& tp0, const Vec2d& tp1);

    void ComputeAimPose(GlobalAnimationHeaderAIM& rAIM, const CDefaultSkeleton* pDefaultSkeleton, const DynArray<SJointsAimIK_Rot>& rRot, const DynArray<SJointsAimIK_Pos>& rPos, QuatTd* arrAbsPose, uint32 nAimPose);
    void Blend4AimPose(GlobalAnimationHeaderAIM& rAIM, const DynArray<SJointsAimIK_Rot>& rRot, const DynArray<SJointsAimIK_Pos>& rPos, int8 i0, int8 i1, int8 i2, int8 i3, const Vec4d& w, QuatTd* arrRelPose, QuatTd* arrAbsPose);
    void NLerp2AimPose(const DynArray<SJointsAimIK_Rot>& rRot, const DynArray<SJointsAimIK_Pos>& rPos, QuatTd* arrRelPose0, QuatTd* arrRelPose1, f64 t, QuatTd* arrAbsPose);
    uint32 AnnotateExamples(uint32 numPoses, QuadIndices* arrQuat);
    Vec3d PolarCoordinate(const Quatd& q);
};




#endif // CRYINCLUDE_CRYANIMATION_GLOBALANIMATIONHEADERAIM_H
