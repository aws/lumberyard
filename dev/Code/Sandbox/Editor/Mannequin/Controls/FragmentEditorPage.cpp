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

#include "StdAfx.h"
#include "FragmentEditorPage.h"

#include <ICryMannequinEditor.h>

#include "FragmentBrowser.h"
#include "PreviewerPage.h"
#include "../FragmentTrack.h"
#include "../FragmentEditor.h"
#include "../MannKeyPropertiesDlgFE.h"
#include "../MannequinDialog.h"
#include "../MannequinModelViewport.h"
#include "../MannequinNodes.h"
#include "../MannequinPlayback.h"
#include "../MannDebugOptionsDialog.h"
// For persistent debugging
#include "IGameFramework.h"

#include <QMenu>
#include <QShortcut>
#include "QtUtil.h"

//////////////////////////////////////////////////////////////////////////

CFragmentEditorPage::CFragmentEditorPage(QWidget* pParent)
    : CMannequinEditorPage(pParent)
    , m_ui(new Ui::FragmentEditorPage)
    , m_playShortcut(new QShortcut(QKeySequence(Qt::Key_Space), this))
    , m_goToStartShortcut(new QShortcut(QKeySequence(Qt::Key_Home), this))
    , m_contexts(NULL)
    , m_bTimeWasChanged(false)
{
    OnInitDialog();
}

CFragmentEditorPage::~CFragmentEditorPage()
{
    QSettings settings;
    settings.beginGroup("Settings");
    settings.beginGroup("Mannequin");
    settings.beginGroup("FragmentEditorLayout");
    settings.setValue("TimelineUnits", m_ui->m_wndTrackPanel->GetTickDisplayMode());
    OnDestroy();
}

void CFragmentEditorPage::OnInitDialog()
{
    m_ui->setupUi(this);
    m_ui->m_tagsPanel->Setup();

    m_contexts = CMannequinDialog::GetCurrentInstance()->Contexts();

    m_fMaxTime = 0.0f;
    m_fTime = 0.0f;
    m_playSpeed = 1.0f;
    m_bPlay = false;
    m_bLoop = false;
    m_draggingTime = false;
    m_bRestartingSequence = false;
    m_bRefreshingTagCtrl = false;
    m_bReadingTagCtrl = false;

    m_fragmentPlayback = NULL;
    m_sequencePlayback = NULL;

    m_tagVars.reset(new CVarBlock());

    setWindowTitle(tr("Fragment Editor Page"));

    // Track view
    m_ui->m_wndTrackPanel->SetMannequinContexts(m_contexts);
    m_ui->m_wndTrackPanel->SetTimeRange(0, 2.0f);
    m_ui->m_wndTrackPanel->SetTimeScale(100, 0);
    m_ui->m_wndTrackPanel->SetCurrTime(0);

    // Model viewport
    m_modelViewport.reset(new CMannequinModelViewport(eMEM_FragmentEditor, this));
    m_ui->m_modelViewportFrame->layout()->addWidget(m_modelViewport.get());
    m_modelViewport->SetType(ET_ViewportModel);
    m_modelViewport->SetTimelineUnits(m_ui->m_wndTrackPanel->GetTickDisplayMode());

    // Nodes tree view
    m_ui->m_wndNodes->SetKeyListCtrl(m_ui->m_wndTrackPanel);
    m_ui->m_wndNodes->SetSequence(m_ui->m_wndTrackPanel->GetSequence());

    // Properties panel
    m_ui->m_wndKeyProperties->SetKeysCtrl(m_ui->m_wndTrackPanel);
    m_ui->m_wndKeyProperties->SetSequence(m_ui->m_wndTrackPanel->GetSequence());

    // Tags panel
    m_ui->m_tagsPanel->SetTitle(tr("Preview Filter Tags"));

    // Toolbar
    InitToolbar();

    // Set up timeline units
    QSettings settings;
    settings.beginGroup("Settings");
    settings.beginGroup("Mannequin");
    settings.beginGroup("FragmentEditorLayout");

    int nTimelineUnits = settings.value("TimelineUnits", m_ui->m_wndTrackPanel->GetTickDisplayMode()).toInt();
    if (nTimelineUnits != m_ui->m_wndTrackPanel->GetTickDisplayMode())
    {
        OnToggleTimelineUnits();
    }


    connect(m_playShortcut.data(), &QShortcut::activated, this, &CFragmentEditorPage::OnPlay);
    connect(m_goToStartShortcut.data(), &QShortcut::activated, this, &CFragmentEditorPage::OnGoToStart);

    connect(m_ui->buttonTvJumpStart, &QToolButton::clicked, this, &CFragmentEditorPage::OnGoToStart);
    connect(m_ui->buttonTvJumpEnd, &QToolButton::clicked, this, &CFragmentEditorPage::OnGoToEnd);
    connect(m_ui->buttonTvPlay, &QToolButton::clicked, this, &CFragmentEditorPage::OnPlay);
    connect(m_ui->buttonPlayLoop, &QToolButton::clicked, this, &CFragmentEditorPage::OnLoop);
    connect(m_ui->buttonToggle1P, &QToolButton::clicked, this, &CFragmentEditorPage::OnToggle1P);
    connect(m_ui->buttonShowDebugOptions, &QToolButton::clicked, this, &CFragmentEditorPage::OnShowDebugOptions);
    connect(m_ui->buttonSetTimeToKey, &QToolButton::clicked, this, &CFragmentEditorPage::OnSetTimeToKey);
    connect(m_ui->buttonToggleScrubUnits, &QToolButton::clicked, this, &CFragmentEditorPage::OnToggleTimelineUnits);
    connect(m_ui->buttonMannReloadAnims, &QToolButton::clicked, this, &CFragmentEditorPage::OnReloadAnimations);
    connect(m_ui->buttonMannLookAtToggle, &QToolButton::clicked, this, &CFragmentEditorPage::OnToggleLookat);
    connect(m_ui->buttonMannRotateLocator, &QToolButton::clicked, this, &CFragmentEditorPage::OnClickRotateLocator);
    connect(m_ui->buttonMannTranslateLocator, &QToolButton::clicked, this, &CFragmentEditorPage::OnClickTranslateLocator);
    connect(m_ui->buttonMannAttachToEntity, &QToolButton::clicked, this, &CFragmentEditorPage::OnToggleAttachToEntity);
    connect(m_ui->buttonMannShowSceneRoots, &QToolButton::clicked, this, &CFragmentEditorPage::OnToggleShowSceneRoots);

    connect(m_ui->m_cDlgToolBar, &CSequencerDopeSheetToolbar::TimeChanged, this, &CFragmentEditorPage::SetTime);

    OnUpdateGoToStart();
    OnUpdateGoToEnd();
    OnUpdatePlay();
    OnUpdateLoop();
    OnUpdateSetTimeToKey();
    OnUpdateTimeEdit();
}

void CFragmentEditorPage::InitToolbar()
{
    QMenu* playMenu = new QMenu(m_ui->buttonTvPlay);
    m_ui->buttonTvPlay->setMenu(playMenu);
    connect(playMenu, &QMenu::aboutToShow, this, &CFragmentEditorPage::OnPlayMenu);
    m_ui->m_cDlgToolBar->SetTime(m_fTime, m_ui->m_wndTrackPanel->GetSnapFps());

    ValidateToolbarButtonsState();
}

void CFragmentEditorPage::ValidateToolbarButtonsState()
{
    m_ui->buttonMannLookAtToggle->setChecked(m_modelViewport->IsLookingAtCamera());
    m_ui->buttonMannRotateLocator->setChecked(!m_modelViewport->IsTranslateLocatorMode());
    m_ui->buttonMannTranslateLocator->setChecked(m_modelViewport->IsTranslateLocatorMode());
    m_ui->buttonMannAttachToEntity->setChecked(m_modelViewport->IsAttachedToEntity());
    m_ui->buttonMannShowSceneRoots->setChecked(m_modelViewport->IsShowingSceneRoots());
    m_ui->buttonToggle1P->setChecked(m_modelViewport->IsCameraAttached());
}

void CFragmentEditorPage::OnDestroy()
{
    m_modelViewport.release();
}

void CFragmentEditorPage::SaveLayout()
{
    const QString strSection = QStringLiteral("Settings\\Mannequin\\FragmentEditorLayout");

    MannUtils::SaveSplitterToRegistry(strSection, QStringLiteral("SequencerSplitter"), m_ui->m_wndSplitterTracks);
    MannUtils::SaveSplitterToRegistry(strSection, QStringLiteral("VerticalSplitter"), m_ui->m_wndSplitterVert);
    MannUtils::SaveSplitterToRegistry(strSection, QStringLiteral("HorizontalSplitter"), m_ui->m_wndSplitterHorz);
}

void CFragmentEditorPage::LoadLayout()
{
    const QString strSection = QStringLiteral("Settings\\Mannequin\\FragmentEditorLayout");

    MannUtils::LoadSplitterFromRegistry(strSection, QStringLiteral("SequencerSplitter"), m_ui->m_wndSplitterTracks, CMannequinDialog::s_minPanelSize);
    MannUtils::LoadSplitterFromRegistry(strSection, QStringLiteral("VerticalSplitter"), m_ui->m_wndSplitterVert, CMannequinDialog::s_minPanelSize);
    MannUtils::LoadSplitterFromRegistry(strSection, QStringLiteral("HorizontalSplitter"), m_ui->m_wndSplitterHorz, CMannequinDialog::s_minPanelSize);
}

void CFragmentEditorPage::slotVisibilityChanged(bool visible)
{
    if (!visible)
    {
        return;
    }

    if (auto pMannequinDialog = CMannequinDialog::GetCurrentInstance())
    {
        m_ui->m_wndNodes->SyncKeyCtrl();

        pMannequinDialog->ShowPane<CFragmentBrowser*>();
    }
}

float CFragmentEditorPage::GetMarkerTimeStart() const
{
    return m_ui->m_wndTrackPanel->GetMarkedTime().start;
}

float CFragmentEditorPage::GetMarkerTimeEnd() const
{
    return m_ui->m_wndTrackPanel->GetMarkedTime().end;
}

void CFragmentEditorPage::InstallAction(float time)
{
    OnSequenceRestart(time);

    if (m_fragmentPlayback)
    {
        m_fragmentPlayback->Release();
    }
    ActionScopes fragmentScopeMask = m_ui->m_wndTrackPanel->GetFragmentScopeMask();
    m_fragmentPlayback = new CFragmentPlayback(fragmentScopeMask, 0);
    SFragTagState tagState = m_ui->m_wndTrackPanel->GetTagState();
    m_fragmentPlayback->SetFragment(m_ui->m_wndTrackPanel->GetFragmentID(), tagState.fragmentTags, m_ui->m_wndTrackPanel->GetFragmentOptionIdx(), m_ui->m_wndTrackPanel->GetMaxTime());
    m_contexts->viewData[eMEM_FragmentEditor].m_pActionController->Queue(*m_fragmentPlayback);

    m_sequencePlayback->SetTime(time, true);
    m_fragmentPlayback->SetTime(time);
    CreateLocators();
}

void CFragmentEditorPage::Update()
{
    const float lastTime = m_fTime;
    bool isDraggingTime = m_ui->m_wndTrackPanel->IsDraggingTime();
    float dsTime = m_ui->m_wndTrackPanel->GetTime();
    if (m_bTimeWasChanged)
    {
        dsTime = m_fTime;
    }

    float effectiveSpeed = m_playSpeed;
    if (isDraggingTime || !m_bPlay)
    {
        effectiveSpeed = 0.0f;
    }

    // important to call before InstallAction (for sound muting through m_bPaused)
    m_modelViewport->SetPlaybackMultiplier(effectiveSpeed);
    if (isDraggingTime || m_bTimeWasChanged)
    {
        if (m_fTime != dsTime || m_bTimeWasChanged)
        {
            InstallAction(dsTime);
        }

        m_fTime = dsTime;
        m_bTimeWasChanged = false;
    }

    m_ui->m_cDlgToolBar->SetTime(m_fTime, m_ui->m_wndTrackPanel->GetSnapFps());

    m_sequencePlayback->Update(gEnv->pTimer->GetFrameTime() * effectiveSpeed, ModelViewport());

    m_draggingTime = isDraggingTime;

    if (m_fragmentPlayback)
    {
        if (m_fragmentPlayback->ReachedEnd())
        {
            if (m_bLoop)
            {
                const float startTime = GetMarkerTimeStart();

                InstallAction(startTime);

                // Set silent mode off.
                m_bRestartingSequence = false;
            }
            else
            {
                m_bPlay = false;
                OnUpdatePlay();
            }
        }

        m_fTime = m_fragmentPlayback->GetTimeSinceInstall();
    }
    m_ui->m_wndTrackPanel->SetCurrTime(m_fTime);
    CheckForLocatorChanges(lastTime, m_fTime);

    if (MannUtils::IsSequenceDirty(m_contexts, m_ui->m_wndTrackPanel->GetSequence(), m_lastChange, eMEM_FragmentEditor))
    {
        m_ui->m_wndTrackPanel->OnAccept();

        InstallAction(m_fTime);

        m_ui->m_wndTrackPanel->SetCurrTime(m_fTime, true);
    }

    m_ui->m_wndKeyProperties->OnUpdate();

    m_bRestartingSequence = false;

    // update the button logic
    ValidateToolbarButtonsState();

    OnUpdateGoToStart();
    OnUpdateGoToEnd();
    OnUpdatePlay();
    OnUpdateLoop();
    OnUpdateSetTimeToKey();
    OnUpdateTimeEdit();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnGoToStart()
{
    SetTime(0.0f);
    m_bPlay = false;
    OnUpdatePlay();
    m_bTimeWasChanged = true;
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnGoToEnd()
{
    SetTime(m_fMaxTime);
    m_bPlay = false;
    OnUpdatePlay();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnPlay()
{
    m_bPlay = !m_bPlay;
    OnUpdatePlay();

    if (m_bPlay)
    {
        if (m_fragmentPlayback && m_fragmentPlayback->ReachedEnd())
        {
            const float startTime = GetMarkerTimeStart();
            m_fTime = startTime;
        }
        // important to trigger InstallAction for audio to play
        m_bTimeWasChanged = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnTimeChanged(float fTime)
{
    m_sequencePlayback->SetTime(fTime);
    Update();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnLoop()
{
    m_bLoop = !m_bLoop;
    OnUpdateLoop();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnToggle1P()
{
    m_modelViewport->ToggleCamera();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnShowDebugOptions()
{
    CMannequinDebugOptionsDialog dialog(ModelViewport(), this);
    dialog.exec();
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::SetTimeToSelectedKey()
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_ui->m_wndTrackPanel->GetSequence(), selectedKeys);

    if (selectedKeys.keys.size() == 1)
    {
        CSequencerUtils::SelectedKey& key = selectedKeys.keys[0];
        CRY_ASSERT(key.pTrack != NULL);

        CSequencerKey* pKey = key.pTrack->GetKey(key.nKey);
        CRY_ASSERT(pKey != NULL);

        SetTime(pKey->time);
    }
}


void CFragmentEditorPage::OnSetTimeToKey()
{
    SetTimeToSelectedKey();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::SetTime(float fTime)
{
    m_fTime = fTime;
    m_bTimeWasChanged = true;
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnToggleTimelineUnits()
{
    // Enum has 2 states, so (oldMode + 1) % 2
    m_ui->m_wndTrackPanel->SetTickDisplayMode((ESequencerTickMode)((m_ui->m_wndTrackPanel->GetTickDisplayMode() + 1) & 1));
    if (m_modelViewport)
    {
        m_modelViewport->SetTimelineUnits(m_ui->m_wndTrackPanel->GetTickDisplayMode());
    }
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnReloadAnimations()
{
    if (m_modelViewport == NULL)
    {
        return;
    }

    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetAllKeys(m_ui->m_wndTrackPanel->GetSequence(), selectedKeys);
    for (int i = 0; i < selectedKeys.keys.size(); ++i)
    {
        CSequencerUtils::SelectedKey& selectedKey = selectedKeys.keys[i];

        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_ANIMLAYER)
        {
            IEntity* pEntity = selectedKey.pNode->GetEntity();
            CRY_ASSERT(pEntity != NULL);

            ICharacterInstance* pCharInstance = pEntity->GetCharacter(0);
            CRY_ASSERT(pCharInstance != NULL);

            pCharInstance->GetISkeletonAnim()->StopAnimationsAllLayers();
            pCharInstance->ReloadCHRPARAMS();
        }
    }
    SetTime(0.0f);
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnToggleLookat()
{
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnClickRotateLocator()
{
    m_modelViewport->SetLocatorRotateMode();
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnClickTranslateLocator()
{
    m_modelViewport->SetLocatorTranslateMode();
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnToggleAttachToEntity()
{
    if (m_modelViewport->IsAttachedToEntity())
    {
        m_modelViewport->DetachFromEntity();
    }
    else
    {
        m_modelViewport->AttachToEntity();
    }
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnPlayMenu()
{
    struct SScales
    {
        const char* pszText;
        float fScale;
    };
    SScales Scales[] = {
        {" 2 ", 2.0f},
        {" 1 ", 1.0f},
        {"1/2", 0.5f},
        {"1/4", 0.25f},
        {"1/8", 0.125f},
    };

    QMenu* menu = m_ui->buttonTvPlay->menu();
    if (menu->actions().isEmpty())
    {
        int nScales = sizeof(Scales) / sizeof(SScales);
        for (int i = 0; i < nScales; i++)
        {
            QAction* action = menu->addAction(QString::fromLatin1(Scales[i].pszText));
            action->setCheckable(true);
            action->setData(Scales[i].fScale);
        }

        connect(menu, &QMenu::triggered, this, [&](QAction* action){ m_playSpeed = action->data().toDouble(); });
    }

    for (QAction* action : menu->actions())
    {
        action->setChecked(action->data().toDouble() == m_playSpeed);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnUpdateGoToStart()
{
    m_ui->buttonTvJumpStart->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnUpdateGoToEnd()
{
    m_ui->buttonTvJumpEnd->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnUpdatePlay()
{
    m_ui->buttonTvPlay->setChecked(m_bPlay);
    m_ui->buttonTvPlay->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnUpdateLoop()
{
    m_ui->buttonPlayLoop->setChecked(m_bLoop);

    m_ui->buttonPlayLoop->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

/*
void CFragmentEditorPage::OnUpdateReloadAnimations( CCmdUI* pCmdUI )
{
    pCmdUI->Enable(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}
*/

void CFragmentEditorPage::OnUpdateSetTimeToKey()
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_ui->m_wndTrackPanel->GetSequence(), selectedKeys);

    m_ui->buttonSetTimeToKey->setEnabled(selectedKeys.keys.size() == 1);
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnUpdateTimeEdit()
{
    m_ui->m_cDlgToolBar->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnToggleShowSceneRoots()
{
    m_modelViewport->SetShowSceneRoots(!m_modelViewport->IsShowingSceneRoots());
    ValidateToolbarButtonsState();
}

void CFragmentEditorPage::UpdateSelectedFragment()
{
    const SFragTagState tagState = m_ui->m_wndTrackPanel->GetTagState();
    SetTagState(m_ui->m_wndTrackPanel->GetFragmentID(), tagState);

    ActionScopes fragmentScopeMask = m_ui->m_wndTrackPanel->GetFragmentScopeMask();
    ActionScopes sequenceScopeMask = (~fragmentScopeMask) & m_contexts->viewData[eMEM_FragmentEditor].m_pActionController->GetActiveScopeMask();

    m_sequencePlayback->SetScopeMask(sequenceScopeMask);
    SetTime(0.0f);

    CMannequinDialog::GetCurrentInstance()->UpdateTagList(m_ui->m_wndTrackPanel->GetFragmentID());
    CMannequinDialog::GetCurrentInstance()->SetTagStateOnCtrl(tagState);

    m_ui->m_wndNodes->SetSequence(m_ui->m_wndTrackPanel->GetSequence());

    CreateLocators();
}

void CFragmentEditorPage::CreateLocators()
{
    m_modelViewport->ClearLocators();

    if (m_ui->m_wndTrackPanel->GetFragmentID() != FRAGMENT_ID_INVALID)
    {
        const CFragmentHistory& history = m_ui->m_wndTrackPanel->GetFragmentHistory().m_history;
        const uint32 numHistoryItems = history.m_items.size();
        for (uint32 h = 0; h < numHistoryItems; h++)
        {
            const CFragmentHistory::SHistoryItem& item = history.m_items[h];

            if ((item.type == CFragmentHistory::SHistoryItem::Param) && item.isLocation)
            {
                m_modelViewport->AddLocator(h, item.paramName.toUtf8().data(), item.param.value);
            }
        }

        CSequencerUtils::SelectedKeys selectedKeys;
        CSequencerUtils::GetSelectedKeys(m_ui->m_wndTrackPanel->GetSequence(), selectedKeys);

        for (int i = 0; i < selectedKeys.keys.size(); ++i)
        {
            CSequencerUtils::SelectedKey& selectedKey = selectedKeys.keys[i];

            const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
            if (paramType == SEQUENCER_PARAM_PROCLAYER)
            {
                CProcClipKey key;
                selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

                const float startTime = key.time;
                const float endTime = startTime + selectedKey.pTrack->GetKeyDuration(selectedKey.nKey);
                const bool currentlyActive = startTime <= m_fTime && m_fTime < endTime;

                if (currentlyActive && key.pParams)
                {
                    QuatT transform;

                    IEntity* pEntity = selectedKey.pNode->GetEntity();
                    CRY_ASSERT(pEntity != NULL);

                    IProceduralParamsEditor::SEditorCreateTransformGizmoResult result = key.pParams->OnEditorCreateTransformGizmo(*pEntity);
                    if (result.createGizmo)
                    {
                        m_modelViewport->AddLocator(numHistoryItems + i, "ProcClip", result.gizmoLocation, pEntity, result.jointId, result.pAttachment, result.paramCRC, result.helperName);
                    }
                }
            }
        }
    }

    OnUpdateGoToStart();
    OnUpdateGoToEnd();
    OnUpdateSetTimeToKey();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::CheckForLocatorChanges(const float lastTime, const float newTime)
{
    if (lastTime == newTime)
    {
        return;
    }

    const CSequencerSequence* pSequence = m_ui->m_wndTrackPanel->GetSequence();
    const int nNumNodes = pSequence->GetNodeCount();
    for (int nodeID = 0; nodeID < nNumNodes; ++nodeID)
    {
        const CSequencerNode* pNode = pSequence->GetNode(nodeID);
        const int nNumTracks = pNode->GetTrackCount();
        for (int trackID = 0; trackID < nNumTracks; ++trackID)
        {
            const CSequencerTrack* pTrack = pNode->GetTrackByIndex(trackID);
            const int nNumKeys = pTrack->GetNumKeys();
            for (int keyID = 0; keyID < nNumKeys; ++keyID)
            {
                const CSequencerKey* pKey = pTrack->GetKey(keyID);
                const float startTime = pKey->time;
                const float endTime = startTime + pTrack->GetKeyDuration(keyID);

                bool prevActive = startTime <= lastTime && lastTime < endTime;
                bool nowActive = startTime <= newTime && newTime < endTime;

                if (prevActive ^ nowActive)
                {
                    // There is at least one change - create the locator(s)!
                    CreateLocators();
                    return;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::SetTagState(const FragmentID fragID, const SFragTagState& tagState)
{
    // Remove scope tags from request so that separate scopes are still able to play distinct fragment options
    TagState tsRequest = tagState.globalTags;
    const uint32 numScopes = m_contexts->m_controllerDef->m_scopeDef.size();
    for (uint32 i = 0; i < numScopes; i++)
    {
        tsRequest = m_contexts->m_controllerDef->m_tags.GetDifference(tsRequest, m_contexts->m_controllerDef->m_scopeDef[i].additionalTags);
    }

    // Get the union of the requested TagState and the global override m_globalTags
    const CTagDefinition& tagDef = m_contexts->m_controllerDef->m_tags;
    const SFragTagState newTagState(tagDef.GetUnion(m_globalTags, tsRequest), tagState.fragmentTags);
    m_contexts->viewData[eMEM_FragmentEditor].m_pAnimContext->state = newTagState.globalTags;

    CMannequinDialog& mannDialog = *CMannequinDialog::GetCurrentInstance();

    //--- Updated associated contexts
    const uint32 numContexts = m_contexts->m_controllerDef->m_scopeContexts.GetNum();
    for (uint32 i = 0; i < numContexts; i++)
    {
        const SScopeContextData* contextData = m_contexts->GetContextDataForID(newTagState, fragID,  i, eMEM_FragmentEditor);
        if (contextData)
        {
            mannDialog.EnableContextData(eMEM_FragmentEditor, contextData->dataID);
        }
        else
        {
            mannDialog.ClearContextData(eMEM_FragmentEditor, i);
        }
    }

    //--- Ensure that the currently selected context is always active for editing
    mannDialog.EnableContextData(eMEM_FragmentEditor, mannDialog.FragmentBrowser()->GetScopeContextId());

    PopulateTagList();
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::SetTagStateOnCtrl(const SFragTagState& tagState)
{
    m_bRefreshingTagCtrl = true;
    m_tagControls.Set(tagState.globalTags);
    m_bRefreshingTagCtrl = false;
}

//////////////////////////////////////////////////////////////////////////
SFragTagState CFragmentEditorPage::GetTagStateFromCtrl() const
{
    SFragTagState ret;
    ret.globalTags = m_tagControls.Get();

    return ret;
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnInternalVariableChange(IVariable* pVar)
{
    if (!m_bRefreshingTagCtrl)
    {
        m_bReadingTagCtrl = true;
        const SFragTagState tagState = GetTagStateFromCtrl();
        m_globalTags = tagState.globalTags;
        UpdateSelectedFragment();
        m_bReadingTagCtrl = false;
    }
}


//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnSequenceRestart(float timePassed)
{
    m_modelViewport->OnSequenceRestart(timePassed);
    m_bRestartingSequence = true;
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::PopulateTagList()
{
    if (!m_bReadingTagCtrl)
    {
        const SFragTagState tagsStateSelected = m_ui->m_wndTrackPanel->GetTagState();
        const ActionScopes availableScopeMask = ~m_ui->m_wndTrackPanel->GetFragmentScopeMask();
        SFragTagState usedTags(TAG_STATE_EMPTY, TAG_STATE_EMPTY);

        const uint32 numScopes = m_contexts->m_controllerDef->m_scopeDef.size();
        SFragTagState queryGlobalTags = tagsStateSelected;
        for (uint32 i = 0; i < numScopes; i++)
        {
            queryGlobalTags.globalTags = m_contexts->m_controllerDef->m_tags.GetDifference(queryGlobalTags.globalTags, m_contexts->m_controllerDef->m_scopeDef[i].additionalTags);
        }

        const CFragmentHistory& history = m_ui->m_wndTrackPanel->GetFragmentHistory().m_history;
        const uint32 numItems = history.m_items.size();
        for (uint32 i = 0; i < numItems; i++)
        {
            const CFragmentHistory::SHistoryItem& item = history.m_items[i];
            FragmentID fragID = item.fragment;

            if ((fragID != FRAGMENT_ID_INVALID) && ((availableScopeMask & item.installedScopeMask) == item.installedScopeMask))
            {
                const uint32 numContexts = m_contexts->m_contextData.size();
                for (uint32 c = 0; c < numContexts; c++)
                {
                    const SScopeContextData& context = m_contexts->m_contextData[c];
                    if (context.database)
                    {
                        SFragTagState contextUsedTags;
                        context.database->QueryUsedTags(fragID, queryGlobalTags, contextUsedTags);

                        usedTags.globalTags = m_contexts->m_controllerDef->m_tags.GetUnion(usedTags.globalTags, contextUsedTags.globalTags);
                    }
                }
            }
        }

        TagState enabledControls = m_contexts->m_controllerDef->m_tags.GetDifference(usedTags.globalTags, tagsStateSelected.globalTags, true);

        m_tagVars->DeleteAllVariables();
        m_tagControls.Init(m_tagVars, m_contexts->viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetDef(), enabledControls);

        RefreshTagsPanel();

        // set this up with the initial values we've already got
        SFragTagState fragTagState;
        fragTagState.globalTags = m_globalTags;
        SetTagStateOnCtrl(fragTagState);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::RefreshTagsPanel()
{
    m_ui->m_tagsPanel->DeleteVars();
    m_ui->m_tagsPanel->SetVarBlock(m_tagVars.get(), functor(*this, &CFragmentEditorPage::OnInternalVariableChange));
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name)
{
    if (tagDef == &m_contexts->viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetDef())
    {
        m_tagControls.RenameTag(tagID, name);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnTagRemoved(const CTagDefinition* tagDef, TagID tagID)
{
    if (tagDef == &m_contexts->viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetDef())
    {
        m_tagControls.RemoveTag(m_tagVars, tagID);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::OnTagAdded(const CTagDefinition* tagDef, const QString& name)
{
    if (tagDef == &m_contexts->viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetDef())
    {
        m_tagControls.AddTag(m_tagVars, name);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::InitialiseToPreviewFile(const XmlNodeRef& xmlSequenceRoot)
{
    m_ui->m_wndTrackPanel->InitialiseToPreviewFile(xmlSequenceRoot);
    m_modelViewport->SetActionController(m_contexts->viewData[eMEM_FragmentEditor].m_pActionController);
    m_contexts->viewData[eMEM_FragmentEditor].m_pActionController->SetFlag(AC_NoTransitions, true);
    for (uint32 i = 0; i < m_contexts->m_contextData.size(); i++)
    {
        SScopeContextData& contextData = m_contexts->m_contextData[i];
        if (contextData.viewData[eMEM_FragmentEditor].enabled && contextData.viewData[eMEM_FragmentEditor].m_pActionController)
        {
            contextData.viewData[eMEM_FragmentEditor].m_pActionController->SetFlag(AC_NoTransitions, true);
        }
    }
    SAFE_DELETE(m_sequencePlayback);
    m_fragmentPlayback = NULL;
    m_sequencePlayback = new CFragmentSequencePlayback(m_ui->m_wndTrackPanel->GetFragmentHistory().m_history, *m_contexts->viewData[eMEM_FragmentEditor].m_pActionController, eMEM_FragmentEditor, 0);

    m_globalTags = m_contexts->viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetMask();
    PopulateTagList();

    OnUpdateGoToStart();
    OnUpdateGoToEnd();
    OnUpdateSetTimeToKey();
}


//////////////////////////////////////////////////////////////////////////
void CFragmentEditorPage::Reset()
{
    m_ui->m_wndTrackPanel->SetFragment(FRAGMENT_ID_INVALID);
    m_ui->m_wndKeyProperties->ForceUpdate();
}

#include <Mannequin/Controls/FragmentEditorPage.moc>
