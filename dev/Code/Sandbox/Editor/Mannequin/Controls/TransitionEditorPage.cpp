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
#include "TransitionEditorPage.h"

#include <ICryMannequinEditor.h>
#include <IGameFramework.h>

#include "../FragmentEditor.h"
#include "../MannKeyPropertiesDlgFE.h"
#include "../MannequinDialog.h"
#include "../MannequinModelViewport.h"
#include "../MannequinNodes.h"
#include "../SequencerSequence.h"
#include "../MannequinPlayback.h"
#include "../FragmentTrack.h"
#include "../SequenceAnalyzerNodes.h"
#include "../MannDebugOptionsDialog.h"
#include "QtUtilWin.h"
#include <QShortcut>

#include <QMessageBox>

namespace
{
    static const float TRANSITION_MIN_TIME_RANGE = 100.0f;
    static const int TRANSITION_CHAR_BUFFER_SIZE = 512;
};

// returns a game root relative path like "/C3/Animations/FragmentSequences/"
QString GetFragmentSequenceFolder();

#define SEQUENCE_FILE_FILTER "Sequence XML Files (*.xml)|*.xml"

//////////////////////////////////////////////////////////////////////////

CTransitionEditorPage::CTransitionEditorPage(QWidget* pParent)
    : CMannequinEditorPage(pParent)
    , m_ui(new Ui::TransitionEditorPage)
    , m_playShortcut(new QShortcut(QKeySequence(Qt::Key_Space), this))
    , m_goToStartShortcut(new QShortcut(QKeySequence(Qt::Key_Home), this))
    , m_contexts(NULL)
    , m_modelViewport(NULL)
    , m_sequence(NULL)
    , m_seed(0)
    , m_fromID(FRAGMENT_ID_INVALID)
    , m_toID(FRAGMENT_ID_INVALID)
    , m_fromFragTag()
    , m_toFragTag()
    , m_scopeMaskFrom(ACTION_SCOPES_NONE)
    , m_scopeMaskTo(ACTION_SCOPES_NONE)
{
    OnInitDialog();
}

CTransitionEditorPage::~CTransitionEditorPage()
{
    QSettings settings;
    settings.beginGroup("Settings");
    settings.beginGroup("Mannequin");
    settings.beginGroup("TransitionEditorLayout");
    settings.setValue("TimelineUnits", m_ui->m_wndTrackPanel->GetTickDisplayMode());
}

void CTransitionEditorPage::OnInitDialog()
{
    m_ui->setupUi(this);
    m_ui->m_tagsPanel->Setup();

    m_contexts = CMannequinDialog::GetCurrentInstance()->Contexts();

    m_fTime = 0.0f;
    m_fMaxTime = 0.0f;
    m_playSpeed = 1.0f;
    m_bPlay = false;
    m_bLoop = false;
    m_draggingTime = false;
    m_bRefreshingTagCtrl = false;

    m_sequencePlayback = NULL;

    m_tagVars.reset(new CVarBlock());

    setWindowTitle(tr("TransitionEditor Page"));

    const char* SEQUENCE_NAME = "Transition Preview";
    m_sequence = new CSequencerSequence();
    m_sequence->SetName(SEQUENCE_NAME);

    // Model viewport
    m_modelViewport = new CMannequinModelViewport(eMEM_TransitionEditor, this);
    m_ui->m_modelViewportFrame->layout()->addWidget(m_modelViewport);
    m_modelViewport->SetType(ET_ViewportModel);

    // Track view
    //m_wndTrackPanel.SetDragAndDrop(false);
    m_ui->m_wndTrackPanel->SetTimeRange(0, m_fMaxTime);
    m_ui->m_wndTrackPanel->SetTimeScale(100, 0);
    m_ui->m_wndTrackPanel->SetCurrTime(0);
    m_ui->m_wndTrackPanel->SetSequence(m_sequence.get());

    // Nodes tree view
    m_ui->m_wndNodes->SetKeyListCtrl(m_ui->m_wndTrackPanel);
    m_ui->m_wndNodes->SetSequence(m_sequence.get());

    // Properties panel
    m_ui->m_wndKeyProperties->SetKeysCtrl(m_ui->m_wndTrackPanel);
    m_ui->m_wndKeyProperties->SetSequence(m_sequence.get());

    // Tags panel
    m_ui->m_tagsPanel->SetTitle(tr("Preview Filter Tags"));

    // Toolbar
    InitToolbar();

    connect(m_playShortcut.data(), &QShortcut::activated, this, &CTransitionEditorPage::OnPlay);
    connect(m_goToStartShortcut.data(), &QShortcut::activated, this, &CTransitionEditorPage::OnGoToStart);

    connect(m_ui->buttonTvJumpStart, &QToolButton::clicked, this, &CTransitionEditorPage::OnGoToStart);
    connect(m_ui->buttonTvJumpEnd, &QToolButton::clicked, this, &CTransitionEditorPage::OnGoToEnd);
    connect(m_ui->buttonTvPlay, &QToolButton::clicked, this, &CTransitionEditorPage::OnPlay);
    connect(m_ui->buttonPlayLoop, &QToolButton::clicked, this, &CTransitionEditorPage::OnLoop);
    connect(m_ui->buttonToggle1P, &QToolButton::clicked, this, &CTransitionEditorPage::OnToggle1P);
    connect(m_ui->buttonShowDebugOptions, &QToolButton::clicked, this, &CTransitionEditorPage::OnShowDebugOptions);
    connect(m_ui->buttonTogglePreviewUnits, &QToolButton::clicked, this, &CTransitionEditorPage::OnToggleTimelineUnits);
    connect(m_ui->buttonMannReloadAnims, &QToolButton::clicked, this, &CTransitionEditorPage::OnReloadAnimations);
    connect(m_ui->buttonMannLookAtToggle, &QToolButton::clicked, this, &CTransitionEditorPage::OnToggleLookat);
    connect(m_ui->buttonMannRotateLocator, &QToolButton::clicked, this, &CTransitionEditorPage::OnClickRotateLocator);
    connect(m_ui->buttonMannTranslateLocator, &QToolButton::clicked, this, &CTransitionEditorPage::OnClickTranslateLocator);
    connect(m_ui->buttonMannAttachToEntity, &QToolButton::clicked, this, &CTransitionEditorPage::OnToggleAttachToEntity);
    connect(m_ui->buttonMannShowSceneRoots, &QToolButton::clicked, this, &CTransitionEditorPage::OnToggleShowSceneRoots);

    connect(m_ui->m_cDlgToolBar, &CSequencerDopeSheetToolbar::TimeChanged, this, &CTransitionEditorPage::OnTimeChanged);

    OnUpdateTimeEdit();
}

void CTransitionEditorPage::InitToolbar()
{
    QMenu* playMenu = new QMenu(m_ui->buttonTvPlay);
    m_ui->buttonTvPlay->setMenu(playMenu);
    connect(playMenu, &QMenu::aboutToShow, this, &CTransitionEditorPage::OnPlayMenu);
    m_ui->m_cDlgToolBar->SetTime(m_fTime, m_ui->m_wndTrackPanel->GetSnapFps());

    ValidateToolbarButtonsState();
}

void CTransitionEditorPage::ValidateToolbarButtonsState()
{
    m_ui->buttonMannLookAtToggle->setChecked(m_modelViewport->IsLookingAtCamera());
    m_ui->buttonMannRotateLocator->setChecked(!m_modelViewport->IsTranslateLocatorMode());
    m_ui->buttonMannTranslateLocator->setChecked(m_modelViewport->IsTranslateLocatorMode());
    m_ui->buttonMannAttachToEntity->setChecked(m_modelViewport->IsAttachedToEntity());
    m_ui->buttonMannShowSceneRoots->setChecked(m_modelViewport->IsShowingSceneRoots());
    m_ui->buttonToggle1P->setChecked(m_modelViewport->IsCameraAttached());
}

void CTransitionEditorPage::SaveLayout()
{
    const QString strSection = QStringLiteral("Settings\\Mannequin\\TransitionEditorLayout");

    MannUtils::SaveSplitterToRegistry(strSection, QStringLiteral("SequencerSplitter"), m_ui->m_wndSplitterTracks);
    MannUtils::SaveSplitterToRegistry(strSection, QStringLiteral("VerticalSplitter"), m_ui->m_wndSplitterVert);
    MannUtils::SaveSplitterToRegistry(strSection, QStringLiteral("HorizontalSplitter"), m_ui->m_wndSplitterHorz);
}

void CTransitionEditorPage::LoadLayout()
{
    const QString strSection = QStringLiteral("Settings\\Mannequin\\TransitionEditorLayout");

    MannUtils::LoadSplitterFromRegistry(strSection, QStringLiteral("SequencerSplitter"), m_ui->m_wndSplitterTracks, CMannequinDialog::s_minPanelSize);
    MannUtils::LoadSplitterFromRegistry(strSection, QStringLiteral("VerticalSplitter"), m_ui->m_wndSplitterVert, CMannequinDialog::s_minPanelSize);
    MannUtils::LoadSplitterFromRegistry(strSection, QStringLiteral("HorizontalSplitter"), m_ui->m_wndSplitterHorz, CMannequinDialog::s_minPanelSize);
}

void CTransitionEditorPage::slotVisibilityChanged(bool visible)
{
    if (!visible)
    {
        return;
    }

    if (auto pMannequinDialog = CMannequinDialog::GetCurrentInstance())
    {
        m_ui->m_wndNodes->SyncKeyCtrl();

        pMannequinDialog->ShowPane<CTransitionBrowser*>();
    }
}

float CTransitionEditorPage::GetMarkerTimeStart() const
{
    return m_ui->m_wndTrackPanel->GetMarkedTime().start;
}

float CTransitionEditorPage::GetMarkerTimeEnd() const
{
    return m_ui->m_wndTrackPanel->GetMarkedTime().end;
}

void CTransitionEditorPage::Update()
{
    bool isDraggingTime = m_ui->m_wndTrackPanel->IsDraggingTime();
    float dsTime = m_ui->m_wndTrackPanel->GetTime();

    float effectiveSpeed = m_playSpeed;
    if (isDraggingTime)
    {
        if (m_sequencePlayback)
        {
            m_sequencePlayback->SetTime(dsTime);
        }
        m_fTime = dsTime;
        effectiveSpeed = 0.0f;
    }
    else if (m_draggingTime)
    {
    }

    if (!m_bPlay)
    {
        effectiveSpeed = 0.0f;
    }

    m_ui->m_cDlgToolBar->SetTime(m_fTime, m_ui->m_wndTrackPanel->GetSnapFps());

    m_modelViewport->SetPlaybackMultiplier(effectiveSpeed);

    m_draggingTime = isDraggingTime;

    float endTime = GetMarkerTimeEnd();
    if (endTime <= 0.0f)
    {
        endTime = m_fMaxTime;
    }

    if (m_sequencePlayback)
    {
        m_sequencePlayback->Update(gEnv->pTimer->GetFrameTime() * effectiveSpeed, m_modelViewport);

        m_fTime = m_sequencePlayback->GetTime();

        if (m_fTime > endTime)
        {
            if (m_bLoop)
            {
                const float startTime = GetMarkerTimeStart();

                m_fTime = startTime;
                m_sequencePlayback->SetTime(startTime);

                // Set silent mode off.
                m_bRestartingSequence = false;
            }
            else
            {
                m_bPlay = false;
                OnUpdatePlay();
            }
        }
    }

    m_ui->m_wndTrackPanel->SetCurrTime(m_fTime);

    if (MannUtils::IsSequenceDirty(m_contexts, m_sequence.get(), m_lastChange, eMEM_TransitionEditor))
    {
        if (!TrackPanel()->IsDragging())
        {
            OnUpdateTV();
        }
    }

    m_ui->m_wndKeyProperties->OnUpdate();
    m_bRestartingSequence = false;

    // Clean up any removed tracks from history. 
    // A ref count of 1 means history is the only remaining holder of the track.
    auto trackIter = m_fragmentHistory->m_tracks.begin();
    while (trackIter != m_fragmentHistory->m_tracks.end())
    {
        const _smart_ptr<CSequencerTrack>& track = *trackIter;

        if (1 == track->NumRefs())
        {
            trackIter = m_fragmentHistory->m_tracks.erase(trackIter);
        }
        else
        {
            ++trackIter;
        }
    }

    // update the button logic
    ValidateToolbarButtonsState();

    OnUpdatePlay();
    OnUpdateLoop();
    OnUpdateGoToStart();
    OnUpdateGoToEnd();
    OnUpdateReloadAnimations();
    OnUpdateTimeEdit();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnGoToStart()
{
    m_fTime = 0.0f;
    m_bPlay = false;
    OnUpdatePlay();

    if (m_sequencePlayback)
    {
        m_sequencePlayback->SetTime(0.0f);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnGoToEnd()
{
    m_fTime = m_fMaxTime;
    m_bPlay = false;
    OnUpdatePlay();

    if (m_sequencePlayback)
    {
        m_sequencePlayback->SetTime(m_sequence->GetTimeRange().end);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnPlay()
{
    m_bPlay = !m_bPlay;
    OnUpdatePlay();

    if (m_bPlay)
    {
        float endTime = GetMarkerTimeEnd();
        if (endTime <= 0.0f)
        {
            endTime = m_fMaxTime;
        }
        if (m_fTime > endTime)
        {
            const float startTime = GetMarkerTimeStart();

            m_fTime = startTime;
            if (m_sequencePlayback)
            {
                m_sequencePlayback->SetTime(startTime);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnLoop()
{
    m_bLoop = !m_bLoop;
    OnUpdateLoop();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnPlayMenu()
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
void CTransitionEditorPage::OnUpdateGoToStart()
{
    m_ui->buttonTvJumpStart->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnUpdateGoToEnd()
{
    m_ui->buttonTvJumpEnd->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnUpdatePlay()
{
    m_ui->buttonTvPlay->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
    m_ui->buttonTvPlay->setChecked(m_bPlay);
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnUpdateLoop()
{
    m_ui->buttonPlayLoop->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
    m_ui->buttonPlayLoop->setChecked(m_bLoop);
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnUpdateReloadAnimations()
{
    m_ui->buttonMannReloadAnims->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnUpdateTimeEdit()
{
    m_ui->m_cDlgToolBar->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnToggle1P()
{
    m_modelViewport->ToggleCamera();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnShowDebugOptions()
{
    CMannequinDebugOptionsDialog dialog(m_modelViewport, this);
    dialog.exec();
    ValidateToolbarButtonsState();
}


//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnToggleTimelineUnits()
{
    // Enum has 2 states, so (oldMode + 1) % 2
    m_ui->m_wndTrackPanel->SetTickDisplayMode((ESequencerTickMode)((m_ui->m_wndTrackPanel->GetTickDisplayMode() + 1) & 1));
    if (m_modelViewport)
    {
        m_modelViewport->SetTimelineUnits(m_ui->m_wndTrackPanel->GetTickDisplayMode());
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnReloadAnimations()
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

    OnSequenceRestart(0.0f);
    SetTime(0.0f);
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnToggleLookat()
{
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnClickRotateLocator()
{
    m_modelViewport->SetLocatorRotateMode();
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnClickTranslateLocator()
{
    m_modelViewport->SetLocatorTranslateMode();
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnToggleShowSceneRoots()
{
    m_modelViewport->SetShowSceneRoots(!m_modelViewport->IsShowingSceneRoots());
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnToggleAttachToEntity()
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
float CTransitionEditorPage::PopulateClipTracks(CSequencerNode* node, const int scopeID)
{
    IActionController* pActionController = m_contexts->viewData[eMEM_TransitionEditor].m_pActionController;
    IScope* scope = pActionController->GetScope(scopeID);

    // Find which keys are selected to restore later
    int numTracks = node->GetTrackCount();
    std::vector<std::vector<int> > selectedKeys;
    selectedKeys.reserve(numTracks);
    for (int trackID = 0; trackID < numTracks; ++trackID)
    {
        std::vector<int> tracksSelectedKeys;
        CSequencerTrack* pTrack = node->GetTrackByIndex(trackID);
        int numKeys = pTrack->GetNumKeys();
        tracksSelectedKeys.reserve(numKeys);

        for (int keyID = 0; keyID < numKeys; ++keyID)
        {
            const CSequencerKey* key = pTrack->GetKey(keyID);
            if (pTrack->IsKeySelected(keyID))
            {
                tracksSelectedKeys.push_back(keyID);
            }
        }

        selectedKeys.push_back(tracksSelectedKeys);
    }

    for (uint32 l = 0; l < node->GetTrackCount(); )
    {
        CSequencerTrack* track = node->GetTrackByIndex(l);
        switch (track->GetParameterType())
        {
        case SEQUENCER_PARAM_ANIMLAYER:
        case SEQUENCER_PARAM_PROCLAYER:
            node->RemoveTrack(track);
            break;
        default:
            l++;
            break;
        }
    }

    CFragmentTrack* fragTrack = (CFragmentTrack*)node->GetTrackForParameter(SEQUENCER_PARAM_FRAGMENTID);
    CTransitionPropertyTrack* propsTrack = (CTransitionPropertyTrack*)node->GetTrackForParameter(SEQUENCER_PARAM_TRANSITIONPROPS);

    //--- Strip transition properties track
    const int numPropKeysInitial = propsTrack->GetNumKeys();
    for (int i = numPropKeysInitial - 1; i >= 0; i--)
    {
        propsTrack->RemoveKey(i);
    }

    const ActionScopes scopeFlag = (1 << scopeID);

    float maxTime = TRANSITION_MIN_TIME_RANGE;
    SClipTrackContext trackContext;
    trackContext.tagState.globalTags = m_tagState;
    trackContext.context = m_contexts->m_scopeData[scopeID].context[eMEM_TransitionEditor];

    const TagState additionalTags = m_contexts->m_controllerDef->m_scopeDef[scopeID].additionalTags;
    const CTagDefinition& tagDef = m_contexts->m_controllerDef->m_tags;

    const uint32 contextID = scope->GetContextID();

    FragmentID lastfragment = FRAGMENT_ID_INVALID;
    SFragTagState   lastTagState;
    float                       lastTime = 0.0f; // last fragment's insertion time, in 'track time'
    float                       lastFragmentDuration = 0.0f;
    bool                        lastPrimaryInstall  = false;

    float motionParams[eMotionParamID_COUNT];
    memset(motionParams, 0, sizeof(motionParams));

    for (uint32 h = 0; h < m_fragmentHistory->m_history.m_items.size(); h++)
    {
        CFragmentHistory::SHistoryItem& item = m_fragmentHistory->m_history.m_items[h];

        if (item.type == CFragmentHistory::SHistoryItem::Tag)
        {
            trackContext.tagState.globalTags = tagDef.GetUnion(m_tagState, item.tagState.globalTags);

            trackContext.context = m_contexts->GetContextDataForID(trackContext.tagState.globalTags, contextID, eMEM_TransitionEditor);
        }
        else if (item.type == CFragmentHistory::SHistoryItem::Param)
        {
            EMotionParamID paramID = MannUtils::GetMotionParam(item.paramName.toUtf8().data());
            if (paramID != eMotionParamID_COUNT)
            {
                motionParams[paramID] = item.param.value.q.v.x;
            }
        }
        else if (item.type == CFragmentHistory::SHistoryItem::Fragment)
        {
            SFragmentQuery fragQuery(tagDef, item.fragment, SFragTagState(trackContext.tagState.globalTags, item.tagState.fragmentTags), additionalTags, item.fragOptionIdxSel);

            ActionScopes scopeMask = m_contexts->potentiallyActiveScopes & item.installedScopeMask;

            if (scopeMask & (1 << scopeID))
            {
                const int numFragKeys = fragTrack->GetNumKeys();
                for (int i = 0; i < numFragKeys; i++)
                {
                    CFragmentKey fragKey;
                    fragTrack->GetKey(i, &fragKey);
                    if (fragKey.historyItem == h)
                    {
                        fragKey.scopeMask   = item.installedScopeMask;
                        fragTrack->SetKey(i, &fragKey);
                    }
                }

                if (trackContext.context && trackContext.context->database)
                {
                    float nextTime = -1.0f;
                    for (uint32 n = h + 1; n < m_fragmentHistory->m_history.m_items.size(); n++)
                    {
                        const CFragmentHistory::SHistoryItem& nextItem = m_fragmentHistory->m_history.m_items[n];
                        if (nextItem.installedScopeMask & item.installedScopeMask)
                        {
                            //--- This fragment collides with the current one and so will force a flush of it
                            nextTime = nextItem.time - m_fragmentHistory->m_history.m_firstTime;
                            break;
                        }
                    }

                    trackContext.historyItem = h;
                    trackContext.scopeID = scopeID;

                    const float time = item.time - m_fragmentHistory->m_history.m_firstTime;


                    uint32 rootScope = 0;
                    for (uint32 i = 0; i <= scopeID; i++)
                    {
                        if (((1 << i) & scopeMask) != 0)
                        {
                            rootScope = i;
                            break;
                        }
                    }

                    bool primaryInstall = true;
                    //--- If this is not the root scope on a cross-scope fragment
                    if (rootScope != scopeID)
                    {
                        if (additionalTags == TAG_STATE_EMPTY)
                        {
                            //--- Check to see if it should go on an earlier context
                            const uint32 numScopes = pActionController->GetTotalScopes();
                            const uint32 contextID = pActionController->GetScope(scopeID)->GetContextID();
                            for (uint32 i = 0; (i < scopeID) && primaryInstall; i++)
                            {
                                const uint32 altContextID = pActionController->GetScope(i)->GetContextID();
                                primaryInstall = (contextID != altContextID);
                            }
                        }
                    }
                    else
                    {
                        item.fragOptionIdxSel = item.fragOptionIdx;
                        if (item.fragOptionIdx == OPTION_IDX_RANDOM)
                        {
                            item.fragOptionIdxSel = m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->randGenerator.GenerateUint32();
                        }
                    }

                    trackContext.tagState.fragmentTags = item.tagState.fragmentTags;

                    CClipKey lastClipKey;
                    CSequencerTrack* pSeqTrack = node->GetTrackForParameter(SEQUENCER_PARAM_ANIMLAYER, 0);
                    if (pSeqTrack)
                    {
                        const int numKeys = pSeqTrack->GetNumKeys();
                        if (numKeys > 0)
                        {
                            pSeqTrack->GetKey(numKeys - 1, &lastClipKey);
                        }
                    }


                    FragmentID installFragID = item.fragment;
                    const float fragmentTime = item.time - lastTime; // how much time has passed relative to the last fragment's insertion time, so 'fragment time'
                    SBlendQuery blendQuery;
                    blendQuery.fragmentFrom   = lastfragment;
                    blendQuery.fragmentTo         = installFragID;
                    blendQuery.tagStateFrom   = lastTagState;
                    blendQuery.tagStateTo         = fragQuery.tagState;
                    blendQuery.fragmentTime   = fragmentTime;
                    blendQuery.prevNormalisedTime = blendQuery.normalisedTime = lastClipKey.ConvertTimeToNormalisedTime(time);
                    blendQuery.additionalTags = additionalTags;
                    blendQuery.SetFlag(SBlendQuery::fromInstalled, lastPrimaryInstall);
                    blendQuery.SetFlag(SBlendQuery::toInstalled, primaryInstall);
                    blendQuery.SetFlag(SBlendQuery::higherPriority, item.trumpsPrevious);
                    blendQuery.SetFlag(SBlendQuery::noTransitions, !lastPrimaryInstall);

                    blendQuery.forceBlendUid = m_forcedBlendUid;

                    SFragmentSelection fragmentSelection;
                    SFragmentData fragData;
                    const bool hasFragment = trackContext.context->database->Query(fragData, blendQuery, item.fragOptionIdxSel, trackContext.context->animSet, &fragmentSelection);
                    MannUtils::AdjustFragDataForVEGs(fragData, *trackContext.context, eMEM_TransitionEditor, motionParams);
                    SBlendQueryResult blend1, blend2;
                    if (lastPrimaryInstall)
                    {
                        trackContext.context->database->FindBestBlends(blendQuery, blend1, blend2);
                    }
                    const SFragmentBlend* pFirstBlend = blend1.pFragmentBlend;
                    const bool hasTransition = (pFirstBlend != NULL);
                    //ADD STUFF

                    float blendStartTime = 0.0f; // in 'track time'
                    float blendSelectTime = 0.0f; // in 'track time'
                    float transitionStartTime = m_fragmentHistory->m_history.m_startTime;
                    float cycleIncrement = 0.0f;
                    if (rootScope == scopeID)
                    {
                        if (!item.trumpsPrevious)
                        {
                            // by default the blendStartTime is at the 'end' of the previous fragment
                            // (for oneshots lastFragmentDuration is at the end of the fragment,
                            // for looping frags it's at the end of the next-to-last clip)
                            blendStartTime = lastTime + lastFragmentDuration;
                            if (hasTransition)
                            {
                                if (pFirstBlend->flags & SFragmentBlend::Cyclic)
                                {
                                    // In a cyclic transition treat selectTime as normalised time
                                    // (we assume it's in the first cycle here and adjust it lateron in the CycleLocked case)
                                    blendSelectTime = lastClipKey.ConvertUnclampedNormalisedTimeToTime(pFirstBlend->selectTime);

                                    if (pFirstBlend->flags & SFragmentBlend::CycleLocked)
                                    {
                                        // Treat the startTime as normalised time.
                                        // Figure out which cycle we are currently in, and move
                                        // startTime & selectTime into the same cycle.

                                        blendStartTime = lastClipKey.ConvertUnclampedNormalisedTimeToTime(pFirstBlend->startTime);
                                        const float lastPrimeClipDuration = lastClipKey.GetOneLoopDuration();
                                        for (; blendStartTime + cycleIncrement <= time; cycleIncrement += lastPrimeClipDuration)
                                        {
                                            ;
                                        }

                                        if (blendSelectTime + cycleIncrement > time)
                                        {
                                            cycleIncrement = max(cycleIncrement - lastPrimeClipDuration, 0.0f);
                                        }

                                        blendStartTime += cycleIncrement;
                                        blendSelectTime += cycleIncrement;
                                    }
                                }
                                else
                                {
                                    blendStartTime += pFirstBlend->startTime;
                                    blendSelectTime = lastTime + pFirstBlend->selectTime;
                                }
                                transitionStartTime = pFirstBlend->startTime;
                            }
                        }

                        item.startTime = blendStartTime + fragData.duration[0]; // in 'track time' (duration[0] should probably be the total time of the transition instead)
                    }
                    else
                    {
                        blendStartTime = item.startTime - fragData.duration[0];
                    }


                    const float insertionTime   = max(time, blendStartTime); // in 'track time'

                    float lastInsertedClipEndTime = 0.0f;
                    const bool isLooping = MannUtils::InsertClipTracksToNode(node, fragData, insertionTime, nextTime, lastInsertedClipEndTime, trackContext);
                    maxTime = max(maxTime, lastInsertedClipEndTime);

                    float firstBlendDuration = 0.0f;
                    bool foundReferenceBlend = false;
                    if (fragData.animLayers.size() > 0)
                    {
                        const uint32 numClips = fragData.animLayers[0].size();
                        for (uint32 c = 0; c < numClips; c++)
                        {
                            const SAnimClip& animClip = fragData.animLayers[0][c];
                            if (animClip.part != animClip.blendPart)
                            {
                                firstBlendDuration = animClip.blend.duration;
                                foundReferenceBlend = true;
                            }
                        }
                    }
                    if ((fragData.procLayers.size() > 0) && !foundReferenceBlend)
                    {
                        const uint32 numClips = fragData.procLayers[0].size();
                        for (uint32 c = 0; c < numClips; c++)
                        {
                            const SProceduralEntry& procClip = fragData.procLayers[0][c];
                            if (procClip.part != procClip.blendPart)
                            {
                                firstBlendDuration = procClip.blend.duration;
                                foundReferenceBlend = true;
                            }
                        }
                    }

                    float transitionTime = insertionTime;
                    const float transitionClipEffectiveStartTime = lastClipKey.GetEffectiveAssetStartTime();
                    const float transitionLastClipDuration = lastClipKey.GetOneLoopDuration();
                    int partID = 0;
                    if (blend1.pFragmentBlend)
                    {
                        const float duration = fragData.duration[partID];
                        const int id = propsTrack->CreateKey(transitionTime);
                        CTransitionPropertyKey transitionKey;
                        propsTrack->GetKey(id, &transitionKey);
                        transitionKey.duration = duration;
                        if (!blend2.pFragmentBlend)
                        {
                            transitionKey.duration += firstBlendDuration;
                        }
                        transitionKey.blend = blend1;
                        transitionKey.prop = CTransitionPropertyKey::eTP_Transition;
                        transitionKey.prevClipStart = lastTime;
                        transitionKey.prevClipDuration = transitionLastClipDuration;
                        transitionKey.tranFlags = blend1.pFragmentBlend->flags;
                        transitionKey.toHistoryItem = h;
                        transitionKey.toFragIndex = partID;
                        transitionKey.sharedId = 0;
                        propsTrack->SetKey(id, &transitionKey);

                        const int idSelect = propsTrack->CreateKey(blendSelectTime);
                        CTransitionPropertyKey selectKey;
                        propsTrack->GetKey(idSelect, &selectKey);
                        selectKey.blend = blend1;
                        selectKey.prop = CTransitionPropertyKey::eTP_Select;
                        selectKey.refTime = lastTime;
                        selectKey.prevClipStart = lastTime;
                        selectKey.prevClipDuration = transitionLastClipDuration;
                        selectKey.tranFlags = blend1.pFragmentBlend->flags;
                        selectKey.toHistoryItem = h;
                        selectKey.toFragIndex = partID;
                        selectKey.sharedId = 0;
                        propsTrack->SetKey(idSelect, &selectKey);

                        CTransitionPropertyKey startKey;
                        const int idStart = propsTrack->CreateKey(cycleIncrement + lastTime + lastFragmentDuration + transitionStartTime);
                        propsTrack->GetKey(idStart, &startKey);
                        startKey.blend = blend1;
                        startKey.prop = CTransitionPropertyKey::eTP_Start;
                        startKey.refTime = lastTime + lastFragmentDuration;
                        startKey.duration = transitionTime - startKey.time;
                        startKey.prevClipStart = lastTime;
                        startKey.prevClipDuration = transitionLastClipDuration;
                        startKey.tranFlags = blend1.pFragmentBlend->flags;
                        startKey.toHistoryItem = h;
                        startKey.toFragIndex = partID;
                        startKey.sharedId = 0;
                        propsTrack->SetKey(idStart, &startKey);
                        transitionTime += duration;
                        ++partID;
                    }
                    if (blend2.pFragmentBlend)
                    {
                        const float duration = fragData.duration[partID];
                        const int id = propsTrack->CreateKey(transitionTime);
                        CTransitionPropertyKey transitionKey;
                        propsTrack->GetKey(id, &transitionKey);
                        transitionKey.duration = duration + firstBlendDuration;
                        transitionKey.blend = blend2;
                        transitionKey.prop = CTransitionPropertyKey::eTP_Transition;
                        transitionKey.prevClipStart = lastTime;
                        transitionKey.prevClipDuration = transitionLastClipDuration;
                        transitionKey.tranFlags = blend2.pFragmentBlend->flags;
                        transitionKey.toHistoryItem = h;
                        transitionKey.toFragIndex = partID;
                        transitionKey.sharedId = 1;
                        propsTrack->SetKey(id, &transitionKey);

                        const int idSelect = propsTrack->CreateKey(blendSelectTime);
                        CTransitionPropertyKey selectKey;
                        propsTrack->GetKey(idSelect, &selectKey);
                        selectKey.blend = blend2;
                        selectKey.prop = CTransitionPropertyKey::eTP_Select;
                        selectKey.refTime = lastTime;
                        selectKey.prevClipStart = lastTime;
                        selectKey.prevClipDuration = transitionLastClipDuration;
                        selectKey.tranFlags = blend2.pFragmentBlend->flags;
                        selectKey.toHistoryItem = h;
                        selectKey.toFragIndex = partID;
                        selectKey.sharedId = 1;
                        propsTrack->SetKey(idSelect, &selectKey);

                        const int idStart = propsTrack->CreateKey(cycleIncrement + lastTime + lastFragmentDuration + transitionStartTime);
                        CTransitionPropertyKey startKey;
                        propsTrack->GetKey(idStart, &startKey);
                        startKey.blend = blend2;
                        startKey.prop = CTransitionPropertyKey::eTP_Start;
                        startKey.refTime = lastTime + lastFragmentDuration;
                        startKey.duration = transitionTime - startKey.time;
                        startKey.prevClipStart = lastTime;
                        startKey.prevClipDuration = transitionLastClipDuration;
                        startKey.tranFlags = blend2.pFragmentBlend->flags;
                        startKey.toHistoryItem = h;
                        startKey.toFragIndex = partID;
                        startKey.sharedId = 1;
                        propsTrack->SetKey(idStart, &startKey);
                        transitionTime += duration;
                        ++partID;
                    }

                    lastFragmentDuration = ((fragData.duration[partID] + transitionTime) - insertionTime);

                    const float fragmentClipDuration = lastInsertedClipEndTime - insertionTime;
                    for (int i = 0; i < numFragKeys; i++)
                    {
                        CFragmentKey fragKey;
                        fragTrack->GetKey(i, &fragKey);
                        if (fragKey.historyItem == trackContext.historyItem)
                        {
                            const float duration = fragData.duration[partID];

                            fragKey.transitionTime  = firstBlendDuration;
                            fragKey.tagState                = fragmentSelection.tagState;
                            fragKey.tagStateFull      = fragQuery.tagState;
                            fragKey.context                 = trackContext.context;
                            fragKey.clipDuration        = fragmentClipDuration;
                            fragKey.tranStartTime       = blendStartTime;
                            fragKey.hasFragment         = hasFragment;
                            fragKey.isLooping               = isLooping;
                            fragKey.scopeMask               = item.installedScopeMask;
                            fragKey.fragIndex               = partID;
                            fragKey.tranStartTimeValue = transitionStartTime;
                            fragTrack->SetKey(i, &fragKey);
                        }
                    }

                    lastPrimaryInstall = primaryInstall;
                    lastfragment = installFragID;
                    lastTagState = fragQuery.tagState;
                    lastTime         = insertionTime;
                }
            }
        }
    }

    // Reselect the new keys
    for (uint32 trackID = 0; trackID < selectedKeys.size(); ++trackID)
    {
        CSequencerTrack* pTrack = node->GetTrackByIndex(trackID);
        if (pTrack == NULL)
        {
            continue;
        }

        // Sort keys
        pTrack->SortKeys();

        // Restore selection
        for (uint32 keyID = 0; keyID < selectedKeys[trackID].size(); ++keyID)
        {
            pTrack->SelectKey(selectedKeys[trackID][keyID], true);
        }
    }

    return maxTime;
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnTimeChanged(float fTime)
{
    m_sequencePlayback->SetTime(fTime);
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::PopulateAllClipTracks()
{
    if (!m_fragmentHistory.get())
    {
        return;
    }

    m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->randGenerator.seed(m_seed);

    uint32 numScopes = m_contexts->m_scopeData.size();
    float maxTime = TRANSITION_MIN_TIME_RANGE;
    for (uint32 i = 0; i < numScopes; i++)
    {
        CSequencerNode* animNode = m_contexts->m_scopeData[i].animNode[eMEM_TransitionEditor];
        if (animNode)
        {
            const float maxTimeCur = PopulateClipTracks(animNode, i);
            maxTime = max(maxTime, maxTimeCur);
        }
    }
    maxTime += 1.0f;
    maxTime = max(maxTime, (m_fragmentHistory->m_history.m_endTime - m_fragmentHistory->m_history.m_firstTime));

    Range timeRange(0.0f, maxTime);
    m_sequence->SetTimeRange(timeRange);
    m_ui->m_wndTrackPanel->SetTimeRange(0.0f, maxTime);
    m_fMaxTime = maxTime;


    //--- Create locators
    m_modelViewport->ClearLocators();

    const CFragmentHistory& history = m_fragmentHistory->m_history;
    const uint32 numHistoryItems = history.m_items.size();
    for (uint32 h = 0; h < numHistoryItems; h++)
    {
        const CFragmentHistory::SHistoryItem& item = history.m_items[h];

        if ((item.type == CFragmentHistory::SHistoryItem::Param) && item.isLocation)
        {
            m_modelViewport->AddLocator(h, item.paramName.toUtf8().data(), item.param.value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::InsertContextHistoryItems(const SScopeData& scopeData, const float keyTime)
{
    for (uint32 h = 0; h < m_contextHistory->m_history.m_items.size(); h++)
    {
        const CFragmentHistory::SHistoryItem& item = m_contextHistory->m_history.m_items[h];

        if (item.type == CFragmentHistory::SHistoryItem::Fragment)
        {
            ActionScopes scopeMask = item.installedScopeMask;
            scopeMask &= scopeData.mannContexts->potentiallyActiveScopes;

            uint32 rootScope = ACTION_SCOPES_ALL;
            for (uint32 i = 0; i <= scopeData.scopeID; i++)
            {
                if (((1 << i) & scopeMask) != 0)
                {
                    rootScope = i;
                    break;
                }
            }

            if (scopeData.scopeID == rootScope)
            {
                CFragmentHistory::SHistoryItem newItem(item);
                newItem.time = keyTime;

                m_fragmentHistory->m_history.m_items.push_back(newItem);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::SetUIFromHistory()
{
    if (!m_fragmentHistory.get())
    {
        return;
    }

    float transitionStartTime = 3.0f; // Default transition start time

    uint32 numScopes = m_contexts->m_scopeData.size();
    m_sequence->RemoveAll();
    m_fragmentHistory->m_tracks.clear();
    if (m_contexts->m_controllerDef)
    {
        CRootNode* rootNode = new CRootNode(m_sequence.get(), *m_contexts->m_controllerDef);
        rootNode->SetName("Globals");
        CTagTrack* tagTrack = (CTagTrack*)rootNode->CreateTrack(SEQUENCER_PARAM_TAGS);
        m_sequence->AddNode(rootNode);

        CParamTrack* paramTrack = (CParamTrack*)rootNode->CreateTrack(SEQUENCER_PARAM_PARAMS);

        m_fragmentHistory->m_tracks.push_back(tagTrack);
        m_fragmentHistory->m_tracks.push_back(paramTrack);

        const uint32 numHistoryItems = m_fragmentHistory->m_history.m_items.size();
        for (uint32 h = 0; h < numHistoryItems; h++)
        {
            const CFragmentHistory::SHistoryItem& item = m_fragmentHistory->m_history.m_items[h];

            if (item.type == CFragmentHistory::SHistoryItem::Tag)
            {
                float time = item.time - m_fragmentHistory->m_history.m_firstTime;
                int newKeyID = tagTrack->CreateKey(time);
                CTagKey newKey;
                newKey.time = time;
                newKey.tagState = item.tagState.globalTags;
                tagTrack->SetKey(newKeyID, &newKey);

                if (newKeyID > 0)
                {
                    // Store this tag's time as the start of the transition, there should only ever be two tags
                    transitionStartTime = time;
                }
            }
            else if (item.type == CFragmentHistory::SHistoryItem::Param)
            {
                float time = item.time - m_fragmentHistory->m_history.m_firstTime;
                int newKeyID = paramTrack->CreateKey(time);
                CParamKey newKey;
                newKey.time = time;
                newKey.name = item.paramName;
                newKey.parameter = item.param;
                newKey.isLocation = item.isLocation;
                newKey.historyItem = h;
                paramTrack->SetKey(newKeyID, &newKey);
            }
        }
    }
    for (uint32 i = 0; i < numScopes; i++)
    {
        SScopeData& scopeData = m_contexts->m_scopeData[i];

        if (!(m_scopeMaskFrom & (1 << i)))
        {
            InsertContextHistoryItems(scopeData, 0.0f);
        }
        else if (!(m_scopeMaskTo & (1 << i))) // If scope was in "from" then it is already applied - we don't want duplicates
        {
            InsertContextHistoryItems(scopeData, transitionStartTime);
        }

        CScopeNode* animNode = new CScopeNode(m_sequence.get(), &scopeData, eMEM_TransitionEditor);
        m_sequence->AddNode(animNode);

        if (scopeData.context[eMEM_TransitionEditor])
        {
            animNode->SetName((scopeData.name + " (" + scopeData.context[eMEM_TransitionEditor]->name + ")").toUtf8().data());
        }
        else
        {
            animNode->SetName((scopeData.name + " (none)").toUtf8().data());
        }

        float maxTime = TRANSITION_MIN_TIME_RANGE;
        MannUtils::InsertFragmentTrackFromHistory(animNode, *m_fragmentHistory, 0.0f, 0.0f, maxTime, scopeData);
        scopeData.fragTrack[eMEM_TransitionEditor] = (CFragmentTrack*)animNode->GetTrackForParameter(SEQUENCER_PARAM_FRAGMENTID);
        scopeData.animNode[eMEM_TransitionEditor] = animNode;
    }

    PopulateAllClipTracks();

    m_ui->m_wndNodes->SetSequence(m_sequence.get());

    if (m_sequencePlayback)
    {
        m_sequencePlayback->SetTime(m_fTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::SetHistoryFromUI()
{
    if (!m_fragmentHistory.get())
    {
        return;
    }
    MannUtils::GetHistoryFromTracks(*m_fragmentHistory);
    m_fragmentHistory->m_history.UpdateScopeMasks(m_contexts, m_tagState);

    Range range = m_sequence->GetTimeRange();
    m_fragmentHistory->m_history.m_startTime = range.start;
    m_fragmentHistory->m_history.m_endTime = range.end;
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnUpdateTV(bool forceUpdate)
{
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    bool anyChanges = false;

    const uint32 numScopes = m_contexts->m_scopeData.size();
    for (uint32 i = 0; i < numScopes; i++)
    {
        uint32 clipChangeCount = 0;
        uint32 fragChangeCount = 0;
        uint32 propChangeCount = 0;

        const SScopeData& scopeData = m_contexts->m_scopeData[i];
        CFragmentTrack* pFragTrack = NULL;
        CTransitionPropertyTrack* pPropsTrack = NULL;
        if (scopeData.animNode[eMEM_TransitionEditor])
        {
            const uint32 numTracks = scopeData.animNode[eMEM_TransitionEditor]->GetTrackCount();

            for (uint32 t = 0; t < numTracks; t++)
            {
                CSequencerTrack* seqTrack = scopeData.animNode[eMEM_TransitionEditor]->GetTrackByIndex(t);
                switch (seqTrack->GetParameterType())
                {
                case SEQUENCER_PARAM_ANIMLAYER:
                case SEQUENCER_PARAM_PROCLAYER:
                    clipChangeCount += seqTrack->GetChangeCount();
                    break;
                case SEQUENCER_PARAM_FRAGMENTID:
                    pFragTrack = (CFragmentTrack*)seqTrack;
                    fragChangeCount = pFragTrack->GetChangeCount();
                    break;
                case SEQUENCER_PARAM_TRANSITIONPROPS:
                    pPropsTrack = (CTransitionPropertyTrack*)seqTrack;
                    propChangeCount = pPropsTrack->GetChangeCount();
                    break;
                }
            }
        }

        // If there have been changes this time, take note
        if (forceUpdate || clipChangeCount || fragChangeCount || propChangeCount)
        {
            anyChanges = true;
        }

        //--- Transition properties have changed, rebuild any transitions
        if ((clipChangeCount || propChangeCount) && pPropsTrack)
        {
            std::vector<int> overriddenBlends;

            const int numKeys = pPropsTrack->GetNumKeys();
            for (uint32 k = 0; k < numKeys; ++k)
            {
                CTransitionPropertyKey key;
                pPropsTrack->GetKey(k, &key);
                if (std::find(overriddenBlends.begin(), overriddenBlends.end(), key.sharedId) != overriddenBlends.end())
                {
                    // Have hit a key pointing at this blend which is overriding the other properties
                    continue;
                }
                const SFragmentBlend* blend = pPropsTrack->GetBlendForKey(k);
                if (blend)
                {
                    bool updated = false;

                    SFragmentBlend blendCopy = *blend;

                    if (propChangeCount)
                    {
                        if (key.overrideProps)
                        {
                            // This key was updated from the properties panel
                            // Copy properties from it then ignore other keys for the same blend
                            updated = true;
                            overriddenBlends.push_back(key.sharedId);
                            key.overrideProps = false;

                            // Copy values directly from the key
                            blendCopy.flags = key.tranFlags;
                            blendCopy.selectTime = max(0.f, key.overrideSelectTime);
                            if (key.tranFlags & SFragmentBlend::Cyclic)
                            {
                                blendCopy.startTime = key.overrideStartTime - (float)(int)key.overrideStartTime;
                            }
                            else
                            {
                                blendCopy.startTime = min(0.f, key.overrideStartTime);
                            }
                            CryLogAlways("Set times from GUI: %f %f", blendCopy.selectTime, blendCopy.startTime);
                        }
                        else if (key.prop == CTransitionPropertyKey::eTP_Select)
                        {
                            // This is a select time key, calculate new select time from the key position
                            updated = true;
                            if (key.tranFlags & SFragmentBlend::Cyclic)
                            {
                                //clipSelectTime = (fragKey.tranSelectTime - fragKey.tranLastClipEffectiveStart) / fragKey.tranLastClipDuration;
                                blendCopy.selectTime = (key.time - key.prevClipStart) / key.prevClipDuration;
                                CryLogAlways("Pre-clip select time: %f", blendCopy.selectTime);
                                blendCopy.selectTime -= (float)(int)blendCopy.selectTime;
                                CryLogAlways("Set select time from key %f (%f - %f) / %f", blendCopy.selectTime, key.time, key.prevClipStart, key.prevClipDuration);
                            }
                            else
                            {
                                blendCopy.selectTime = key.time - key.prevClipStart;
                                CryLogAlways("Set select time from key %f (%f - %f)", blendCopy.selectTime, key.time, key.prevClipStart);
                            }
                        }
                        else if (key.prop == CTransitionPropertyKey::eTP_Start)
                        {
                            // This is a start time key, calculate new start time from the key position
                            updated = true;
                            if (key.tranFlags & SFragmentBlend::Cyclic)
                            {
                                //clipStartTime  = (tranStartTime - fragKey.tranLastClipEffectiveStart) / fragKey.tranLastClipDuration;
                                blendCopy.startTime = (key.time - key.refTime) / key.prevClipDuration;
                                CryLogAlways("Pre-clip start time: %f", blendCopy.startTime);
                                blendCopy.startTime -= (float)(int)blendCopy.startTime;
                                CryLogAlways("Set start time from key %f (%f - %f) / %f", blendCopy.startTime, key.refTime, key.time, key.prevClipDuration);
                            }
                            else
                            {
                                blendCopy.startTime = min(0.0f, key.time - key.refTime);
                                CryLogAlways("Set start time from key %f (%f - %f)", blendCopy.startTime, key.time, key.refTime);
                            }
                        }
                    }

                    CFragment fragmentNew;
                    if (clipChangeCount && key.prop == CTransitionPropertyKey::eTP_Transition)
                    {
                        MannUtils::GetFragmentFromClipTracks(fragmentNew, scopeData.animNode[eMEM_TransitionEditor], key.toHistoryItem, key.time, key.toFragIndex);
                        blendCopy.pFragment = &fragmentNew;
                        updated = true;
                    }

                    if (updated)
                    {
                        pPropsTrack->UpdateBlendForKey(k, blendCopy);
                    }
                }
                CRY_ASSERT(blend);
            }
        }

        //--- Clip has changed, rebuild any transition fragments
        if ((clipChangeCount || fragChangeCount) && pFragTrack)
        {
            const int numKeys = pFragTrack->GetNumKeys();
            float lastTime = 0.0f;
            for (uint32 k = 0; k < numKeys; k++)
            {
                CFragmentKey fragKey;
                pFragTrack->GetKey(k, &fragKey);

                if (fragKey.transition)
                {
                    CRY_ASSERT(fragKey.context);

                    bool alreadyDoneTransition = false;
                    if (fragKey.sharedID != 0)
                    {
                        for (uint32 s = 0, mask = 1; s < i; s++, mask <<= 1)
                        {
                            if (fragKey.scopeMask & mask)
                            {
                                alreadyDoneTransition = true;
                                break;
                            }
                        }
                    }

                    const float tranStartTime  = fragKey.time;
                    int prevKeyID = pFragTrack->GetPrecedingFragmentKey(k, true);
                    const bool hasPrevClip = (prevKeyID >= 0);
                    const SFragmentBlend* pFragmentBlend = fragKey.context->database->GetBlend(fragKey.tranFragFrom, fragKey.tranFragTo, fragKey.tranTagFrom, fragKey.tranTagTo, fragKey.tranBlendUid);
                    if (pFragmentBlend)
                    {
                        float clipSelectTime, clipStartTime;

                        if (hasPrevClip && !alreadyDoneTransition)
                        {
                            CFragmentKey prevFragKey;
                            pFragTrack->GetKey(prevKeyID, &prevFragKey);

                            if (fragKey.tranFlags & SFragmentBlend::Cyclic)
                            {
                                clipSelectTime = (fragKey.tranSelectTime - fragKey.tranLastClipEffectiveStart) / fragKey.tranLastClipDuration;
                                clipStartTime  = (tranStartTime - fragKey.tranLastClipEffectiveStart) / fragKey.tranLastClipDuration;
                                clipSelectTime -= (float)(int)clipSelectTime;
                                clipStartTime  -= (float)(int)clipStartTime;
                            }
                            else
                            {
                                clipSelectTime = fragKey.tranSelectTime - prevFragKey.time;
                                clipStartTime  = min(tranStartTime - prevFragKey.clipDuration - max(prevFragKey.tranStartTime, prevFragKey.time), 0.0f);
                            }
                        }
                        else
                        {
                            clipSelectTime = pFragmentBlend->selectTime;
                            clipStartTime  = pFragmentBlend->startTime;
                        }
                        clipStartTime -= fragKey.tranStartTimeRelative;

                        if (clipChangeCount)
                        {
                            CFragment fragmentNew;
                            MannUtils::GetFragmentFromClipTracks(fragmentNew, scopeData.animNode[eMEM_TransitionEditor], fragKey.historyItem, tranStartTime, fragKey.fragIndex);
                            SFragmentBlend fragBlend;
                            fragBlend.pFragment = &fragmentNew;
                            fragBlend.enterTime = 0.0f;
                            fragBlend.selectTime = clipSelectTime;
                            fragBlend.startTime  = clipStartTime;
                            fragBlend.flags = fragKey.tranFlags;
                            MannUtils::GetMannequinEditorManager().SetBlend(fragKey.context->database, fragKey.tranFragFrom, fragKey.tranFragTo, fragKey.tranTagFrom, fragKey.tranTagTo, fragKey.tranBlendUid, fragBlend);

                            CMannequinDialog& mannDialog = *CMannequinDialog::GetCurrentInstance();
                            mannDialog.TransitionBrowser()->Refresh();
                        }
                        else if (hasPrevClip)
                        {
                            SFragmentBlend fragBlendNew = *pFragmentBlend;
                            fragBlendNew.selectTime = clipSelectTime;
                            fragBlendNew.startTime  = clipStartTime;
                            fragBlendNew.flags = fragKey.tranFlags;
                            MannUtils::GetMannequinEditorManager().SetBlend(fragKey.context->database, fragKey.tranFragFrom, fragKey.tranFragTo, fragKey.tranTagFrom, fragKey.tranTagTo, fragKey.tranBlendUid, fragBlendNew);

                            CMannequinDialog& mannDialog = *CMannequinDialog::GetCurrentInstance();
                            mannDialog.TransitionBrowser()->Refresh();
                        }
                    }
                }
            }
        }
    }

    if (anyChanges)
    {
        SetHistoryFromUI();

        PopulateAllClipTracks();
    }

    if (m_sequence.get())
    {
        m_ui->m_wndNodes->SetSequence(m_sequence.get());
    }

    if (m_sequencePlayback)
    {
        m_sequencePlayback->SetTime(m_fTime, true);
    }

    //--- Reset the track based change counts
    const uint32 numNodes = m_sequence->GetNodeCount();
    for (uint32 i = 0; i < numNodes; i++)
    {
        CSequencerNode* seqNode = m_sequence->GetNode(i);
        const uint32 numTracks = seqNode->GetTrackCount();
        for (uint32 t = 0; t < numTracks; t++)
        {
            CSequencerTrack* seqTrack = seqNode->GetTrackByIndex(t);
            seqTrack->ResetChangeCount();
        }
    }
}

void CTransitionEditorPage::LoadSequence(const int scopeContextIdx, const FragmentID fromID, const FragmentID toID, const SFragTagState fromFragTag, const SFragTagState toFragTag, const SFragmentBlendUid blendUid)
{
    m_fromID = fromID;
    m_toID = toID;
    m_fromFragTag = fromFragTag;
    m_toFragTag = toFragTag;
    m_scopeMaskFrom = m_contexts->m_controllerDef->GetScopeMask(fromID, fromFragTag);
    m_scopeMaskTo = m_contexts->m_controllerDef->GetScopeMask(toID, toFragTag);

    m_forcedBlendUid = blendUid;

    // load the blend into the history
    m_fragmentHistory->m_history.LoadSequence(scopeContextIdx, fromID, toID, fromFragTag, toFragTag, blendUid, m_tagState);

    // this sets the keys onto each scope (track?)
    m_fragmentHistory->m_history.UpdateScopeMasks(m_contexts, m_tagState);

    if (m_sequencePlayback)
    {
        m_sequencePlayback->SetTime(0.0f);
    }

    // this goes away and populates everything like magic
    SetUIFromHistory();

    m_ui->m_wndNodes->SetSequence(m_sequence.get());
    m_ui->m_wndKeyProperties->SetSequence(m_sequence.get());

    PopulateTagList();

    OnUpdateTV(true);
    SelectFirstKey();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::ClearSequence()
{
    if (m_sequencePlayback != NULL)
    {
        m_sequencePlayback->SetTime(0.0f);
    }

    m_ui->m_wndNodes->SetSequence(NULL);
    m_ui->m_wndKeyProperties->SetSequence(NULL);
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnSequenceRestart(float newTime)
{
    m_modelViewport->OnSequenceRestart(newTime);
    m_bRestartingSequence = true;
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::PopulateTagList()
{
    // Make all tags that are used in the transition (from or to) non-zero
    SFragTagState transitionTags;
    transitionTags.globalTags = m_fromFragTag.globalTags | m_toFragTag.globalTags;
    transitionTags.fragmentTags = m_fromFragTag.fragmentTags | m_toFragTag.fragmentTags;

    // Store the current preview tag state
    const CTagDefinition& tagDefs   = m_contexts->m_controllerDef->m_tags;
    const TagState globalTags = m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->state.GetMask();

    SFragTagState usedTags(TAG_STATE_EMPTY, TAG_STATE_EMPTY);

    // Subtract scope tags from the transition tags
    const uint32 numScopes = m_contexts->m_controllerDef->m_scopeDef.size();
    SFragTagState queryGlobalTags = transitionTags;
    for (uint32 i = 0; i < numScopes; i++)
    {
        queryGlobalTags.globalTags = m_contexts->m_controllerDef->m_tags.GetDifference(queryGlobalTags.globalTags, m_contexts->m_controllerDef->m_scopeDef[i].additionalTags);
    }

    // Determine which tags are used in the history
    const CFragmentHistory& history = m_fragmentHistory->m_history;
    const uint32 numItems = history.m_items.size();
    for (uint32 i = 0; i < numItems; i++)
    {
        const CFragmentHistory::SHistoryItem& item = history.m_items[i];
        FragmentID fragID = item.fragment;

        if (fragID != FRAGMENT_ID_INVALID)
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

    // Enable tags that are used in the history but not forced by the transition tags
    TagState enabledControls = m_contexts->m_controllerDef->m_tags.GetDifference(usedTags.globalTags, transitionTags.globalTags, true);

    // Update the properties pane with the relevant tags
    m_tagVars->DeleteAllVariables();
    m_tagControls.Init(m_tagVars, m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->state.GetDef(), enabledControls);

    RefreshTagsPanel();

    // set this up with the initial values we've already got
    SFragTagState fragTagState;
    fragTagState.globalTags = m_tagState;
    SetTagStateOnCtrl(fragTagState);
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::RefreshTagsPanel()
{
    m_ui->m_tagsPanel->DeleteVars();
    m_ui->m_tagsPanel->SetVarBlock(m_tagVars.get(), functor(*this, &CTransitionEditorPage::OnInternalVariableChange));
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name)
{
    if (tagDef == &m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->state.GetDef())
    {
        m_tagControls.RenameTag(tagID, name);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnTagRemoved(const CTagDefinition* tagDef, TagID tagID)
{
    if (tagDef == &m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->state.GetDef())
    {
        m_tagControls.RemoveTag(m_tagVars, tagID);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnTagAdded(const CTagDefinition* tagDef, const QString& name)
{
    if (tagDef == &m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->state.GetDef())
    {
        m_tagControls.AddTag(m_tagVars, name);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::OnInternalVariableChange(IVariable* pVar)
{
    if (!m_bRefreshingTagCtrl)
    {
        SFragTagState tagState = GetTagStateFromCtrl();

        const uint32 numContexts = m_contexts->m_controllerDef->m_scopeContexts.GetNum();
        for (uint32 i = 0; i < numContexts; i++)
        {
            const SScopeContextData* contextData = m_contexts->GetContextDataForID(tagState.globalTags, i, eMEM_TransitionEditor);
            if (contextData)
            {
                const bool res = CMannequinDialog::GetCurrentInstance()->EnableContextData(eMEM_TransitionEditor, contextData->dataID);
                if (!res)
                {
                    // Write out error msg via MessageBox
                    QMessageBox::critical(this, tr("Mannequin Error"), tr("Cannot set that value as it has no parent context.\nIs it a scope/barrel attachment that needs a gun tag setting first?"));

                    // reset the displayed info?
                    CMannequinDialog::GetCurrentInstance()->ClearContextData(eMEM_TransitionEditor, i);
                    tagState.globalTags = m_tagState;
                    SetTagStateOnCtrl(tagState);
                }
            }
            else
            {
                CMannequinDialog::GetCurrentInstance()->ClearContextData(eMEM_TransitionEditor, i);
            }
        }

        SetUIFromHistory();

        m_tagState = tagState.globalTags;
        m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->state = tagState.globalTags;
        OnUpdateTV();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::ResetForcedBlend()
{
    m_forcedBlendUid = SFragmentBlendUid();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::SelectFirstKey()
{
    if (!m_ui->m_wndTrackPanel->SelectFirstKey(SEQUENCER_PARAM_TRANSITIONPROPS))
    {
        if (!m_ui->m_wndTrackPanel->SelectFirstKey(SEQUENCER_PARAM_ANIMLAYER))
        {
            if (!m_ui->m_wndTrackPanel->SelectFirstKey(SEQUENCER_PARAM_PROCLAYER))
            {
                m_ui->m_wndTrackPanel->SelectFirstKey();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::SetTagStateOnCtrl(const SFragTagState& tagState)
{
    m_bRefreshingTagCtrl = true;
    m_tagControls.Set(tagState.globalTags);
    m_bRefreshingTagCtrl = false;
}

//////////////////////////////////////////////////////////////////////////
SFragTagState CTransitionEditorPage::GetTagStateFromCtrl() const
{
    SFragTagState ret;
    ret.globalTags = m_tagControls.Get();

    return ret;
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::InitialiseToPreviewFile(const XmlNodeRef& xmlSequenceRoot)
{
    ResetForcedBlend();

    m_modelViewport->SetActionController(m_contexts->viewData[eMEM_TransitionEditor].m_pActionController);

    m_contextHistory.reset(new SFragmentHistoryContext(*m_contexts->viewData[eMEM_TransitionEditor].m_pActionController));
    m_contextHistory->m_history.LoadSequence(xmlSequenceRoot);
    m_contextHistory->m_history.UpdateScopeMasks(m_contexts, m_tagState);

    m_fragmentHistory.reset(new SFragmentHistoryContext(*m_contexts->viewData[eMEM_TransitionEditor].m_pActionController));

    m_tagState = m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->state.GetMask();

    m_fMaxTime = m_fragmentHistory->m_history.m_endTime - m_fragmentHistory->m_history.m_firstTime;

    m_sequencePlayback = new CFragmentSequencePlayback(m_fragmentHistory->m_history, *m_contexts->viewData[eMEM_TransitionEditor].m_pActionController, eMEM_TransitionEditor);
    //m_sequencePlayback = new CActionSequencePlayback(ACTPreviewFileION_SCOPES_ALL, 0, 0, m_fragmentHistory->m_history, true);
    //m_sequencePlayback->SetMaxTime(m_fMaxTime);
    //m_contexts->viewData[eMEM_TransitionEditor].m_pActionController->Queue(m_sequencePlayback);

    SetUIFromHistory();
    PopulateTagList();
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::SetTagState(const TagState& tagState)
{
    const CTagDefinition& tagDef = m_contexts->m_controllerDef->m_tags;
    TagState tsNew = tagDef.GetUnion(m_tagState, tagState);
    m_contexts->viewData[eMEM_TransitionEditor].m_pAnimContext->state = tsNew;

    //--- Updated associated contexts
    const uint32 numContexts = m_contexts->m_controllerDef->m_scopeContexts.GetNum();
    for (uint32 i = 0; i < numContexts; i++)
    {
        const SScopeContextData* contextData = m_contexts->GetContextDataForID(tsNew, i, eMEM_TransitionEditor);
        if (contextData)
        {
            CMannequinDialog::GetCurrentInstance()->EnableContextData(eMEM_TransitionEditor, contextData->dataID);
        }
        else
        {
            CMannequinDialog::GetCurrentInstance()->ClearContextData(eMEM_TransitionEditor, i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionEditorPage::SetTime(float fTime)
{
    if (m_sequencePlayback != NULL)
    {
        m_sequencePlayback->SetTime(fTime, true);
    }

    m_ui->m_wndTrackPanel->SetCurrTime(fTime, true);
}

#include <Mannequin/Controls/TransitionEditorPage.moc>
