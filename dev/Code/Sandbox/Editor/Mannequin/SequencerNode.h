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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERNODE_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERNODE_H
#pragma once


#include "ISequencerSystem.h"
#include "MannequinBase.h"

#include <QMetaType>

struct SControllerDef;
class CSequencerSequence;
class CSequencerTrack;

namespace
{
    enum EMenuItem
    {
        eMenuItem_CopySelectedKeys = 1,
        eMenuItem_CopyKeys,
        eMenuItem_PasteKeys,
        eMenuItem_CopyLayer,
        eMenuItem_PasteLayer,
        eMenuItem_RemoveLayer,
        eMenuItem_MuteNode,
        eMenuItem_MuteAll,
        eMenuItem_UnmuteAll,
        eMenuItem_SoloNode,
        eMenuItem_CreateForCurrentTags,
        eMenuItem_AddLayer_First = 100, // Begins range Add Layer
        eMenuItem_AddLayer_Last = 120,  // Ends range Add Layer
        eMenuItem_ShowHide_First = 200, // Begins range Show/Hide Track
        eMenuItem_ShowHide_Last = 220   // Ends range Show/Hide Track
    };
}

class CSequencerNode
    : virtual public _i_reference_target_t
{
public:
    enum ESupportedParamFlags
    {
        PARAM_MULTIPLE_TRACKS = 0x01, // Set if parameter can be assigned multiple tracks.
    };

    struct SParamInfo
    {
        SParamInfo()
            : name("")
            , paramId(SEQUENCER_PARAM_UNDEFINED)
            , flags(0) {};
        SParamInfo(const char* _name, ESequencerParamType _paramId, int _flags)
            : name(_name)
            , paramId(_paramId)
            , flags(_flags) {};

        const char* name;
        ESequencerParamType paramId;
        int flags;
    };

public:
    CSequencerNode(CSequencerSequence* sequence, const SControllerDef& controllerDef);
    virtual ~CSequencerNode();

    void SetName(const char* name);
    const char* GetName();

    virtual ESequencerNodeType GetType() const;


    void SetSequence(CSequencerSequence* pSequence);
    CSequencerSequence* GetSequence();


    virtual IEntity* GetEntity();

    void UpdateKeys();

    int GetTrackCount() const;
    void AddTrack(ESequencerParamType param, CSequencerTrack* track);
    bool RemoveTrack(CSequencerTrack* pTrack);
    CSequencerTrack* GetTrackByIndex(int nIndex) const;

    virtual CSequencerTrack* CreateTrack(ESequencerParamType nParamId);
    CSequencerTrack* GetTrackForParameter(ESequencerParamType nParamId) const;
    CSequencerTrack* GetTrackForParameter(ESequencerParamType nParamId, uint32 index) const;


    void SetTimeRange(Range timeRange);

    bool GetStartExpanded() const
    {
        return m_startExpanded;
    }

    virtual void InsertMenuOptions(QMenu* menu);
    virtual void ClearMenuOptions(QMenu* menu);
    virtual void OnMenuOption(int menuOption);


    virtual bool CanAddTrackForParameter(ESequencerParamType nParamId) const;

    virtual int GetParamCount() const;
    virtual bool GetParamInfo(int nIndex, SParamInfo& info) const;

    bool GetParamInfoFromId(ESequencerParamType paramId, SParamInfo& info) const;
    bool IsParamValid(ESequencerParamType paramId) const;

    void Mute(bool bMute) { m_muted = bMute; }
    bool IsMuted() const { return m_muted; }
    void SetReadOnly(bool bReadOnly) { m_readOnly = bReadOnly; }
    bool IsReadOnly() const { return m_readOnly; }

    virtual void UpdateMutedLayerMasks(uint32 mutedAnimLayerMask, uint32 mutedProcLayerMask);

protected:
    string m_name;
    CSequencerSequence* m_sequence;
    const SControllerDef& m_controllerDef;

    bool m_muted;
    bool m_readOnly;

    bool m_startExpanded;

    // Tracks.
    struct TrackDesc
    {
        ESequencerParamType paramId; // Track parameter id.
        _smart_ptr<CSequencerTrack> track;
    };

    std::vector<TrackDesc> m_tracks;
};

Q_DECLARE_METATYPE(CSequencerNode*);


#endif // CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERNODE_H
