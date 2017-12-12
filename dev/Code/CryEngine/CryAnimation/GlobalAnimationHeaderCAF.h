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

#ifndef CRYINCLUDE_CRYANIMATION_GLOBALANIMATIONHEADERCAF_H
#define CRYINCLUDE_CRYANIMATION_GLOBALANIMATIONHEADERCAF_H
#pragma once

#include "GlobalAnimationHeader.h"
#include "Controller.h"
#include "ControllerPQLog.h"
#include "ControllerTCB.h"
#include "ControllerDefragHeap.h"

#include "IStreamEngine.h"
#include <NameCRCHelper.h>
#include "AnimEventList.h"

struct CInternalSkinningInfoDBA;
class ILoaderCAFListener;
class IControllerRelocatableChain;

#include <PoolAllocator.h>

#define CAF_MAX_SEGMENTS        (5)         //Maximum number of segments in a CAF

struct GlobalAnimationHeaderCAFStreamContent
    : public IStreamCallback
{
    int m_id;
    const char* m_pPath;

    uint32 m_nFlags;
    uint32 m_FilePathDBACRC32;

    f32     m_fStartSec;
    f32     m_fEndSec;
    f32     m_fTotalDuration;

    f32     m_fSampleRate;

    QuatT   m_StartLocation;

    DynArray<IController_AutoPtr> m_arrController;
    CControllerDefragHdl m_hControllerData;

    GlobalAnimationHeaderCAFStreamContent();
    ~GlobalAnimationHeaderCAFStreamContent();

    void* StreamOnNeedStorage(IReadStream* pStream, unsigned nSize, bool& bAbortOnFailToAlloc);
    void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
    void StreamOnComplete (IReadStream* pStream, unsigned nError);

    uint32 ParseChunkHeaders(IChunkFile* pChunkFile, bool& bLoadOldChunksOut, bool& bLoadInPlaceOut, size_t& nUsefulSizeOut);
    bool ParseChunkRange(IChunkFile* pChunkFile, uint32 min, uint32 max, bool bLoadOldChunks, char*& pStorage, IControllerRelocatableChain*& pRelocateHead);
    bool ReadMotionParameters (IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadController(IChunkFile::ChunkDesc* pChunkDesc, bool bLoadOldChunks, char*& pStorage, IControllerRelocatableChain*& pRelocateHead);
    bool ReadTiming (IChunkFile::ChunkDesc* pChunkDesc);
    uint32 FlagsSanityFilter(uint32 flags);

    f32 GetSampleRate() const { return m_fSampleRate; }

private:
    // Prevent copy construction
    GlobalAnimationHeaderCAFStreamContent(const GlobalAnimationHeaderCAFStreamContent&);
    GlobalAnimationHeaderCAFStreamContent& operator = (const GlobalAnimationHeaderCAFStreamContent&);
};

//////////////////////////////////////////////////////////////////////////
// this is the animation information on the module level (not on the per-model level)
// it doesn't know the name of the animation (which is model-specific), but does know the file name
// Implements some services for bone binding and ref counting
struct GlobalAnimationHeaderCAF
    : public GlobalAnimationHeader
{
    friend class CAnimationManager;
    friend class CAnimationSet;
    friend struct GlobalAnimationHeaderCAFStreamContent;

    GlobalAnimationHeaderCAF();
    ~GlobalAnimationHeaderCAF()
    {
        ClearControllers();
    };

    //! Duration of a segment.
    //! \param segment Id of the segment. Valid segments should be <= GetTotalSegments() and <= (CAF_MAX_SEGMENTS - 2).
    //! \returns Duration. Always returns 0 if the segment is out of range.
    ILINE f32 GetSegmentDuration(uint32 segmentIndex) const
    {
        if (segmentIndex > m_Segments || segmentIndex > (CAF_MAX_SEGMENTS - 2))
        {
            return 0;
        }
        f32 t0 = m_SegmentsTime[segmentIndex + 0];
        f32 t1 = m_SegmentsTime[segmentIndex + 1];
        f32 t       =   t1 - t0;
        return m_fTotalDuration * t;
    };

    ILINE f32 GetNTimeforEntireClip(uint32 segmentIndex, f32 fAnimTime) const
    {
        // Compute time of the current frame for the ENTIRE animation.
        int32 segtotal = m_Segments - 1;
        f32 totdur = m_fTotalDuration ? m_fTotalDuration : (1.0f / GetSampleRate());
        const f32 segdur = GetSegmentDuration(segmentIndex);
        const f32 segbase = m_SegmentsTime[segmentIndex];
        const f32 percent = segdur / totdur;
        return fAnimTime * percent + segbase; //this is a percentage value between 0-1 for the ENTIRE animation
    }

    ILINE uint32 GetTotalSegments() const
    {
        return m_Segments;
    }

    //! Convert time basis from ntime to ktime
    //! \param ntime Time to convert. Should be in the range 0 <= ntime <= 1
    //! \returns Converted time. Undefined for out of range inputs.
    f32 NTime2KTime(f32 ntime) const
    {
        CRY_ASSERT(ntime >= 0 && ntime <= 1);
        return GetSampleRate() * (m_fStartSec + ntime * (m_fEndSec - m_fStartSec));
    }

    void AddRef()
    {
        ++m_nRef_by_Model;
    }

    void Release()
    {
        if (!--m_nRef_by_Model)
        {
            ClearControllers();
        }
    }

#ifdef _DEBUG
    // returns the maximum reference counter from all controllers. 1 means that nobody but this animation structure refers to them
    int MaxControllerRefCount() const
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

    //count how many position controllers this animation has
    uint32 GetTotalPosKeys() const;

    //count how many rotation controllers this animation has
    uint32 GetTotalRotKeys() const;


    size_t SizeOfCAF(const bool bForceControllerCalcu = false) const;
    void LoadControllersCAF();
    void LoadDBA();

    void GetMemoryUsage(ICrySizer* pSizer) const;

    uint32 IsAssetLoaded() const
    {
        if (m_nControllers2)
        {
            return m_nControllers;
        }
        return m_FilePathDBACRC32 != -1; //return 0 in case of an ANM-file
    }

    ILINE uint32 IsAssetOnDemand() const
    {
        if (m_nFlags & CA_ASSET_TCB)
        {
            return 0;
        }
        return m_FilePathDBACRC32 == 0;
    }

#ifdef EDITOR_PCDEBUGCODE
    bool Export2HTR(const char* szAnimationName, const char* saveDirectory, const CDefaultSkeleton* pDefaultSkeleton);
    static bool SaveHTR(const char* szAnimationName, const char* saveDirectory, const std::vector<string>& jointNameArray, const std::vector<string>& jointParentArray, const std::vector< DynArray<QuatT> >& arrAnimation, const QuatT* parrDefJoints);
    static bool SaveICAF(const char* szAnimationName, const char* saveDirectory, const std::vector<string>& arrJointNames, const std::vector< DynArray<QuatT> >& arrAnimation, f32 sampleRate);
#endif

    //---------------------------------------------------------------
    IController* GetControllerByJointCRC32(uint32 nControllerID) const
    {
        if (!IsAssetCreated() || !IsAssetLoaded())
        {
            return nullptr;
        }

        int32 nSize = m_arrControllerLookupVector.size();


        IF (m_arrController.size() != nSize, false)
        {
            return nullptr;
        }

        IF (nSize == 0, false)  // don't try to search in empty arrays
        {
            return nullptr;
        }

        const uint32* arrControllerLookup = &m_arrControllerLookupVector[0];
        int32 low = 0;
        int32 high = nSize - 1;
        while (low <= high)
        {
            int32 mid = (low + high) / 2;
            if (arrControllerLookup[mid] > nControllerID)
            {
                high = mid - 1;
            }
            else if (arrControllerLookup[mid] < nControllerID)
            {
                low = mid + 1;
            }
            else
            {
                IController* pController = m_arrController[mid].get();
                CryPrefetch(pController);
                return pController;
            }
        }
        return nullptr; // not found
    }

    size_t GetControllersCount() const
    {
        return m_nControllers;
    }

    virtual void AddNewDynamicControllers(IAnimationSet* animationSet)
    {
        uint controllerCount = GetControllersCount();

        for (uint i = 0; i < controllerCount; ++i)
        {
            if (m_arrController[i]->GetFlags() & IController::DYNAMIC_CONTROLLER)
            {
                if (!animationSet->HasDynamicController(m_arrController[i]->GetID()))
                {
                    animationSet->AddDynamicController(m_arrController[i]->GetID());
                }
            }
        }
    }

    void ClearControllers();
    void ClearControllerData();
    uint32 LoadCAF();
    uint32 DoesExistCAF();
    void StartStreamingCAF();
    void ControllerInit();

    void ConnectCAFandDBA();

    void ResetSegmentTime(int segmentIndex);

    f32 GetSampleRate() const { return m_fSampleRate; }

    void InitControllerLookup(uint32 numControllers)
    {
        if (numControllers > 512)
        {
            CryLogAlways("ERROR, controller array size to big(size = %i", numControllers);
        }
        m_arrControllerLookupVector.resize(numControllers);
        for (uint32 i = 0; i < numControllers; ++i)
        {
            m_arrControllerLookupVector[i] = m_arrController[i]->GetID();
        }
    }


public:
    uint32 m_FilePathDBACRC32;              //hash value (if the file is comming from a DBA)
    IReadStreamPtr m_pStream;

    f32     m_fSampleRate;

    f32     m_fStartSec;                                //asset-feature: Start time in seconds.
    f32     m_fEndSec;                                  //asset-feature: End time in seconds.
    f32     m_fTotalDuration;                       //asset-feature: asset-feature: total duration in seconds.

    uint16 m_nControllers;
    uint16 m_nControllers2;


    DynArray<IController_AutoPtr> m_arrController;
    DynArray<uint32> m_arrControllerLookupVector; // used to speed up controller lookup (binary search on the CRC32 only, load controller from a associative array)
    CControllerDefragHdl m_hControllerData;

    CAnimEventList m_AnimEventsCAF;

    uint16  m_nRef_by_Model;                //counter how many models are referencing this animation
    uint16  m_nRef_at_Runtime;          //counter how many times we use this animation at run-time at the same time (needed for streaming)
    uint16  m_nTouchedCounter;          //for statistics: did we use this asset at all?
    uint8       m_bEmpty;                               //Loaded from database
    uint8       m_Segments;                         //asset-feature: amount of segments
    f32         m_SegmentsTime[CAF_MAX_SEGMENTS];           //asset-feature: normalized-time for each segment

    QuatT   m_StartLocation;                        //asset-feature: the original location of the animation in world-space

private:
    void CommitContent(GlobalAnimationHeaderCAFStreamContent& content);
    void FinishLoading(unsigned nError);
} _ALIGN(16);

// mark GlobalAnimationHeaderCAF as moveable, to prevent problems with missing copy-constructors
// and to not waste performance doing expensive copy-constructor operations
template<>
inline bool raw_movable<GlobalAnimationHeaderCAF>(GlobalAnimationHeaderCAF const& dest)
{
    return true;
}

#endif // CRYINCLUDE_CRYANIMATION_GLOBALANIMATIONHEADERCAF_H
