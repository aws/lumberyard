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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_PREVIEWERPAGE_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_PREVIEWERPAGE_H
#pragma once


#include <ICryMannequin.h>

#include "MannequinEditorPage.h"
#include "../MannequinBase.h"
#include "../MannequinNodes.h"
#include "../MannKeyPropertiesDlgFE.h"
#include "../SequencerDopeSheetToolbar.h"

#include "MannDopeSheet.h"

#include <QScopedPointer>

class QShortcut;

struct SMannequinContexts;
class CMannequinModelViewport;
class CSequencerSequence;
class CFragmentSequencePlayback;

#include <Mannequin/Controls/ui_PreviewerPage.h>

class CPreviewerPage
    : public CMannequinEditorPage
{
    Q_OBJECT
public:
    CPreviewerPage(QWidget* pParent = nullptr);
    ~CPreviewerPage();

    virtual CMannequinModelViewport* ModelViewport() const { return m_modelViewport; }
    virtual CMannDopeSheet* TrackPanel() { return m_ui->m_wndTrackPanel; }
    virtual CMannNodesWidget* Nodes() { return m_ui->m_wndNodes; }
    CMannKeyPropertiesDlgFE* KeyProperties() { return m_ui->m_wndKeyProperties; }

    CSequencerSequence* Sequence() const { return m_sequence.get(); }
    SFragmentHistoryContext* FragmentHistory() const { return m_fragmentHistory.get(); }

    void SaveLayout();
    void LoadLayout();
    virtual void ValidateToolbarButtonsState();

    void Update();
    void PopulateAllClipTracks();
    void SetUIFromHistory();
    void SetHistoryFromUI();
    void OnUpdateTV();
    void OnSequenceRestart(float newTime);
    void InitialiseToPreviewFile();
    void SetTagState(const TagState& tagState);

    void SetTime(float fTime);

    float GetMarkerTimeStart() const;
    float GetMarkerTimeEnd() const;

    void NewSequence();
    void LoadSequence(const QString& szFilename = QString());
    void SaveSequence(const QString& szFilename = QString());

    void LoadSequence(const int scopeContextIdx, const FragmentID fromID, const FragmentID toID, const SFragTagState fromFragTag, const SFragTagState toFragTag, const SFragmentBlendUid blendUid);

    void OnNewSequence();
    void OnLoadSequence();
    void OnSaveSequence();

public slots:
    void OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name);
    void OnTagRemoved(const CTagDefinition* tagDef, TagID tagID);
    void OnTagAdded(const CTagDefinition* tagDef, const QString& name);

protected:
    void OnInitDialog();

    void OnGoToStart();
    void OnGoToEnd();
    void OnPlay();
    void OnLoop();
    void OnToggle1P();
    void OnShowDebugOptions();
    void OnToggleTimelineUnits();
    void OnReloadAnimations();
    void OnToggleLookat();
    void OnClickRotateLocator();
    void OnClickTranslateLocator();
    void OnToggleShowSceneRoots();
    void OnToggleAttachToEntity();

    void OnUpdateGoToStart();
    void OnUpdateGoToEnd();
    void OnUpdatePlay();
    void OnUpdateLoop();
    void OnUpdateReloadAnimations();
    void OnUpdateNewSequence();
    void OnUpdateLoadSequence();
    void OnUpdateSaveSequence();
    void OnUpdateTimeEdit();

    void focusInEvent(QFocusEvent* event) override;

    void OnPlayMenu();
    float PopulateClipTracks(CSequencerNode* node, const int scopeID);

    void OnTimeChanged(float fTime);

private:
    void InitToolbar();

    void PopulateTagList();
    void RefreshTagsPanel();
    void SetTagStateOnCtrl(const SFragTagState& tagState);
    SFragTagState GetTagStateFromCtrl() const;
    void OnInternalVariableChange(IVariable* pVar);
    void ResetForcedBlend();

    QScopedPointer<Ui::PreviewerPage> m_ui;
    QScopedPointer<QShortcut> m_playShortcut;
    QScopedPointer<QShortcut> m_goToStartShortcut;
    CMannequinModelViewport* m_modelViewport;

    SMannequinContexts* m_contexts;
    CFragmentSequencePlayback* m_sequencePlayback;

    std::unique_ptr<SFragmentHistoryContext>    m_fragmentHistory;
    _smart_ptr<CSequencerSequence>                  m_sequence;

    float m_fTime;
    float m_fMaxTime;
    float m_playSpeed;
    uint32 m_seed;
    bool m_bPlay;
    bool m_bLoop;
    bool m_draggingTime;
    bool m_bRestartingSequence;

    SLastChange m_lastChange;
    TagState m_tagState;
    bool m_bRefreshingTagCtrl;
    TSmartPtr<CVarBlock> m_tagVars;
    CTagControl m_tagControls;

    SFragmentBlendUid m_forcedBlendUid;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_PREVIEWERPAGE_H
