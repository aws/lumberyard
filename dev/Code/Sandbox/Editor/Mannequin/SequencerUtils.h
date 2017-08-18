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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERUTILS_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERUTILS_H
#pragma once

#pragma once

#include "ISequencerSystem.h"

//////////////////////////////////////////////////////////////////////////
class CSequencerUtils
{
public:
    struct SelectedKey
    {
        enum EKeyTimeType
        {
            eKeyBegin = 0,
            eKeyBlend,
            eKeyLoop,
            eKeyEnd
        };
        SelectedKey()
            : pNode(NULL)
            , pTrack(NULL)
            , nKey(-1)
            , fTime(0.0f)
            , eTimeType(eKeyBegin) {}
        SelectedKey(CSequencerNode* node, CSequencerTrack* track, int key, float time, EKeyTimeType timeType)
            : pNode(node)
            , pTrack(track)
            , nKey(key)
            , fTime(time)
            , eTimeType(timeType)
        {
        }

        CSequencerNode* pNode;
        CSequencerTrack* pTrack;
        int                         nKey;
        float                       fTime;
        EKeyTimeType        eTimeType;
    };
    struct SelectedKeys
    {
        std::vector<SelectedKey> keys;
    };

    struct SelectedTrack
    {
        CSequencerNode* pNode;
        CSequencerTrack* pTrack;
        int m_nSubTrackIndex;
    };
    struct SelectedTracks
    {
        std::vector<SelectedTrack> tracks;
    };

    // Return array of selected keys from the given sequence.
    static int GetSelectedKeys(CSequencerSequence* pSequence, SelectedKeys& selectedKeys);
    static int GetAllKeys(CSequencerSequence* pSequence, SelectedKeys& selectedKeys);
    static int GetSelectedTracks(CSequencerSequence* pSequence, SelectedTracks& selectedTracks);
    // Check whether only one track or subtracks belong to one same track is/are selected.
    static bool IsOneTrackSelected(const SelectedTracks& selectedTracks);
    static int GetKeysInTimeRange(const CSequencerSequence* pSequence, SelectedKeys& selectedKeys, const float t0, const float t1);
    static bool CanAnyKeyBeMoved(const SelectedKeys& selectedKeys);
};


//////////////////////////////////////////////////////////////////////////
struct ISequencerEventsListener
{
    // Called when Key selection changes.
    virtual void OnKeySelectionChange() = 0;
};


#endif // CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERUTILS_H
