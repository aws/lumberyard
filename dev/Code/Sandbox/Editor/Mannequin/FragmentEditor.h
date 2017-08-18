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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_FRAGMENTEDITOR_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_FRAGMENTEDITOR_H
#pragma once


#include "Controls/MannDopeSheet.h"

#include "ICryMannequin.h"

#include "MannequinBase.h"

class CFragment;
class IAnimationDatabase;

// CMannFragmentEditor dialog
class CMannFragmentEditor
    : public CMannDopeSheet
{
    Q_OBJECT
public:
    CMannFragmentEditor(QWidget* parent = nullptr);
    ~CMannFragmentEditor();

    void InitialiseToPreviewFile(const XmlNodeRef& xmlSequenceRoot);

    void SetMannequinContexts(SMannequinContexts* contexts);
    CSequencerSequence* GetSequence() { return m_pSequence; };

    SFragmentHistoryContext& GetFragmentHistory()
    {
        return *m_fragmentHistory;
    }

    void SetCurrentFragment(bool bForceContextToCurrentTags = false, uint32 scopeID = 0, uint32 contextID = 0);
    void SetFragment(const FragmentID fragID, const SFragTagState& tagState = SFragTagState(), const uint32 fragOption = 0,
        bool bForceContextToCurrentTags = false, uint32 scopeID = 0, uint32 contextID = 0);

    ActionScopes GetFragmentScopeMask() const
    {
        return m_fragmentScopeMask;
    }

    FragmentID GetFragmentID() const
    {
        return m_fragID;
    }

    FragmentID GetFragmentOptionIdx() const
    {
        return m_fragOptionIdx;
    }

    void SetTagState(const SFragTagState& tagState)
    {
        m_tagState = tagState;
    }
    const SFragTagState& GetTagState() const
    {
        return m_tagState;
    }

    float GetMaxTime() const
    {
        return m_realTimeRange.end;
    }

    void OnEditorNotifyEvent(EEditorNotifyEvent event);
    void OnAccept();

    void FlushChanges()
    {
        if (m_bEditingFragment)
        {
            StoreChanges();
            m_bEditingFragment = false;
        }
    }

protected:
    //void DrawKeys( CSequencerTrack *track,QPainter *painter,QRect &rc,Range &timeRange );
    void UpdateFragmentClips();

    void StoreChanges();

    void OnInternalVariableChange(IVariable* pVar);

    SMannequinContexts* m_contexts;
    SFragmentHistoryContext* m_fragmentHistory;

    FragmentID m_fragID;
    uint32       m_fragOptionIdx;
    SFragTagState m_tagState;

    ActionScopes m_fragmentScopeMask;

    CFragment m_fragmentTEMP;

    float m_motionParams[eMotionParamID_COUNT];

    bool m_bEditingFragment;
};



#endif // CRYINCLUDE_EDITOR_MANNEQUIN_FRAGMENTEDITOR_H
