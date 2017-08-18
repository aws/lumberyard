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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_FRAGMENTEDITORPAGE_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_FRAGMENTEDITORPAGE_H
#pragma once


#include <ICryMannequin.h>

#include "MannequinEditorPage.h"
#include "../MannequinBase.h"
#include "../MannequinNodes.h"
#include "../MannKeyPropertiesDlgFE.h"
#include "../FragmentEditor.h"
#include "../SequencerDopeSheetToolbar.h"

class QShortcut;

struct SMannequinContexts;
class CMannequinModelViewport;
class CFragmentPlayback;
class CFragmentSequencePlayback;

#include <Mannequin/Controls/ui_FragmentEditorPage.h>

class CFragmentEditorPage
    : public CMannequinEditorPage
{
    Q_OBJECT
public:
    CFragmentEditorPage(QWidget* pParent = nullptr);
    ~CFragmentEditorPage();

    virtual CMannequinModelViewport* ModelViewport() const { return m_modelViewport.get(); }
    virtual CMannFragmentEditor* TrackPanel() { return m_ui->m_wndTrackPanel; }
    virtual CMannNodesWidget* Nodes() { return m_ui->m_wndNodes; }
    CMannKeyPropertiesDlgFE* KeyProperties() { return m_ui->m_wndKeyProperties; }

    void SaveLayout();
    void LoadLayout();

    virtual void ValidateToolbarButtonsState();
    void Update();
    void UpdateSelectedFragment();
    void CreateLocators();
    void CheckForLocatorChanges(const float lastTime, const float newTime);
    void OnSequenceRestart(float timePassed);
    void InitialiseToPreviewFile(const XmlNodeRef& xmlSequenceRoot);
    void Reset();

    void SetTimeToSelectedKey();
    void SetTime(float fTime);

    float GetMarkerTimeStart() const;
    float GetMarkerTimeEnd() const;

    void SetTagState(const FragmentID fragID, const SFragTagState& tagState);
    TagState GetGlobalTags() const {return m_globalTags; }

public slots:
    void OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name);
    void OnTagRemoved(const CTagDefinition* tagDef, TagID tagID);
    void OnTagAdded(const CTagDefinition* tagDef, const QString& name);

protected slots:
    void OnInitDialog();

    void OnGoToStart();
    void OnGoToEnd();
    void OnPlay();
    void OnLoop();
    void OnToggle1P();
    void OnShowDebugOptions();
    void OnSetTimeToKey();
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
    void OnUpdateSetTimeToKey();
    //  void OnUpdateReloadAnimations();
    void OnUpdateTimeEdit();

    void slotVisibilityChanged(bool visible);
    void OnDestroy();

    void OnPlayMenu();

    void OnTimeChanged(float fTime);

private:
    void InitToolbar();
    void InstallAction(float time);
    void PopulateTagList();
    void RefreshTagsPanel();
    void SetTagStateOnCtrl(const SFragTagState& tagState);
    SFragTagState GetTagStateFromCtrl() const;
    void OnInternalVariableChange(IVariable* pVar);

    QScopedPointer<Ui::FragmentEditorPage> m_ui;
    QScopedPointer<QShortcut> m_playShortcut;
    QScopedPointer<QShortcut> m_goToStartShortcut;
    std::unique_ptr<CMannequinModelViewport>    m_modelViewport;

    SMannequinContexts* m_contexts;

    float m_fTime;
    float m_fMaxTime;
    float m_playSpeed;
    bool m_bPlay;
    bool m_bLoop;
    bool m_draggingTime;
    bool m_bRestartingSequence;
    bool m_bRefreshingTagCtrl;
    bool m_bReadingTagCtrl;
    bool m_bTimeWasChanged;

    SLastChange m_lastChange;
    TagState m_globalTags;
    TSmartPtr<CVarBlock> m_tagVars;
    CTagControl m_tagControls;

    CFragmentPlayback* m_fragmentPlayback;
    CFragmentSequencePlayback* m_sequencePlayback;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_FRAGMENTEDITORPAGE_H
