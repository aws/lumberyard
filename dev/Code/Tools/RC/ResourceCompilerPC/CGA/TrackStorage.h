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

// Description : Storage for database of tracks.


#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_TRACKSTORAGE_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_TRACKSTORAGE_H
#pragma once

#include "AnimationLoader.h"
#include <AzCore/std/parallel/mutex.h>

struct DBStatistics
{
    DBStatistics()
        : m_iNumAnimations(0)
        , m_iNumControllers(0)
        , m_iNumKeyTimes(0)
        , m_iNumPositions(0)
        , m_iNumRotations(0)
        , m_iSavedBytes(0) {}
    int m_iNumAnimations;

    int m_iNumControllers;

    int m_iNumKeyTimes;
    int m_iNumPositions;
    int m_iNumRotations;


    int m_iSavedBytes;
};

struct GlobalAnimationHeaderCAF;

class CTrackStorage
{
public:
    CTrackStorage(bool bigEndianOutput);
    ~CTrackStorage();

    bool GetAnimFileTime(const string&  name, DWORD* ft);

    void AddAnimation(const GlobalAnimationHeaderCAF& header);

    void Analyze(uint32& TrackShader, uint32& SizeDataShared, uint32& TotalTracks, uint32& TotalMemory, CSkeletonInfo* currentSkeleton);

    void Clear()
    {
        m_arrGlobalAnimationHeaderCAF.clear();
        m_arrAnimNames.clear();
        m_arrKeyTimes.clear();
        m_arrRotationTracks.clear();
        m_arrPositionTracks.clear();
        m_arrKeyTimesRemap.clear();
        m_arrKeyPosRemap.clear();
        m_arrKeyRotRemap.clear();
    }

    uint32  FindKeyTimesTrack(KeyTimesInformationPtr& pKeyTimes)
    {
        const uint32 numKeys = pKeyTimes->GetNumKeys();
        TVectorRemap::iterator it = m_arrKeyTimesRemap.lower_bound(numKeys);
        TVectorRemap::iterator end = m_arrKeyTimesRemap.upper_bound(numKeys);

        for (; it != end; ++it)
        {
            if (IsKeyTimesIdentical(pKeyTimes, m_arrKeyTimes[it->second]))
            {
                return it->second;
            }
        }

        return -1;
    }

    uint32  FindPositionTrack(PositionControllerPtr& pKeys)
    {
        const uint32 numKeys = pKeys->GetNumKeys();
        TVectorRemap::iterator it = m_arrKeyPosRemap.lower_bound(numKeys);
        TVectorRemap::iterator end = m_arrKeyPosRemap.upper_bound(numKeys);

        for (; it != end; ++it)
        {
            if (IsPositionIdentical(pKeys, m_arrPositionTracks[it->second]))
            {
                return it->second;
            }
        }

        return -1;
    }

    uint32  FindRotationTrack(RotationControllerPtr& pKeys)
    {
        const uint32 numKeys = pKeys->GetNumKeys();
        TVectorRemap::iterator it = m_arrKeyRotRemap.lower_bound(numKeys);
        TVectorRemap::iterator end = m_arrKeyRotRemap.upper_bound(numKeys);

        for (; it != end; ++it)
        {
            if (IsRotationIdentical(pKeys, m_arrRotationTracks[it->second]))
            {
                return it->second;
            }
        }

        return -1;
    }

    void SaveDataBase905(const char* name, bool bPrepareForInPlaceStream, int pointerSize);

    const DBStatistics& GetStatistics()
    {
        return m_Statistics;
    }

protected:
    bool IsRotationIdentical(const RotationControllerPtr& track1, const TrackRotationStoragePtr&  track2);
    bool IsPositionIdentical(const PositionControllerPtr& track1, const TrackPositionStoragePtr& track2);
    bool IsKeyTimesIdentical(const KeyTimesInformationPtr& track1, const KeyTimesInformationPtr& track2);
    // get number animation from m_arrAnimations. If anim doesnt exist - new num created;
    uint32 FindOrAddAnimationHeader(const GlobalAnimationHeaderCAF& header, const string& name);
    uint32 FindAnimation(const string& name);

    void AnalyzeKeyTimes();

    void CreateBitsetKeyTimes(int k);
    void CreateStartStopKeyTimes(int k);
public:
    std::vector<GlobalAnimationHeaderCAF> m_arrGlobalAnimationHeaderCAF;
    typedef std::vector<string> TNamesVector;
    TNamesVector m_arrAnimNames;
    DynArray<KeyTimesInformationPtr> m_arrKeyTimes;
    DynArray<TrackRotationStoragePtr> m_arrRotationTracks;
    DynArray<TrackPositionStoragePtr> m_arrPositionTracks;

    typedef std::multimap<int, int> TVectorRemap;

    TVectorRemap m_arrKeyTimesRemap;
    TVectorRemap m_arrKeyPosRemap;
    TVectorRemap m_arrKeyRotRemap;

    bool m_bBigEndianOutput;

    DBStatistics m_Statistics;
    AZStd::mutex m_lock;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_TRACKSTORAGE_H
