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
#include "PreviewerPage.h"

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
#include "QtUtil.h"
#include "QtUtilWin.h"
#include <QShortcut>

#include <QDir>
#include <QMenu>
#include <QMessageBox>

#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

namespace
{
    static const float MIN_TIME_RANGE = 100.0f;
    static const int CHAR_BUFFER_SIZE = 512;
};

// returns a game root relative path like "/C3/Animations/FragmentSequences/"
QString GetFragmentSequenceFolder()
{
    ICVar* pSequencePathCVar = gEnv->pConsole->GetCVar("mn_sequence_path");
    return QtUtil::ToQString(Path::GetEditingGameDataFolder().c_str()) + "/" + (pSequencePathCVar ? pSequencePathCVar->GetString() : "");
}

#define SEQUENCE_FILE_FILTER "Sequence XML Files (*.xml)|*.xml"
#define SEQUENCE_FILE_FILTER_QT "Sequence XML Files (*.xml)"

//////////////////////////////////////////////////////////////////////////

CPreviewerPage::CPreviewerPage(QWidget* pParent)
    : CMannequinEditorPage(pParent)
    , m_ui(new Ui::PreviewerPage)
    , m_playShortcut(new QShortcut(QKeySequence(Qt::Key_Space), this))
    , m_goToStartShortcut(new QShortcut(QKeySequence(Qt::Key_Home), this))
    , m_contexts(NULL)
    , m_modelViewport(NULL)
    , m_sequence(NULL)
    , m_seed(0)
{
    OnInitDialog();
}

CPreviewerPage::~CPreviewerPage()
{
    QSettings settings;
    settings.beginGroup("Settings");
    settings.beginGroup("Mannequin");
    settings.beginGroup("PreviewerLayout");
    settings.setValue("TimelineUnits", m_ui->m_wndTrackPanel->GetTickDisplayMode());
}

void CPreviewerPage::OnInitDialog()
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

    setWindowTitle(QStringLiteral("Previewer Page"));

    const char* SEQUENCE_NAME = "Preview";
    m_sequence = new CSequencerSequence();
    m_sequence->SetName(SEQUENCE_NAME);

    // Model viewport
    m_modelViewport = new CMannequinModelViewport(eMEM_Previewer, this);
    m_ui->m_modelViewportFrame->layout()->addWidget(m_modelViewport);
    m_modelViewport->SetType(ET_ViewportModel);

    // Track view
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
    m_ui->m_tagsPanel->SetTitle("Preview Filter Tags");

    // Toolbar
    InitToolbar();

    // Set up timeline units
    QSettings settings;
    settings.beginGroup("Settings");
    settings.beginGroup("Mannequin");
    settings.beginGroup("PreviewerLayout");

    int nTimelineUnits = settings.value("TimelineUnits", m_ui->m_wndTrackPanel->GetTickDisplayMode()).toInt();
    if (nTimelineUnits != m_ui->m_wndTrackPanel->GetTickDisplayMode())
    {
        OnToggleTimelineUnits();
    }

    connect(m_playShortcut.data(), &QShortcut::activated, this, &CPreviewerPage::OnPlay);
    connect(m_goToStartShortcut.data(), &QShortcut::activated, this, &CPreviewerPage::OnGoToStart);

    connect(m_ui->buttonTvJumpStart, &QToolButton::clicked, this, &CPreviewerPage::OnGoToStart);
    connect(m_ui->buttonTvJumpEnd, &QToolButton::clicked, this, &CPreviewerPage::OnGoToEnd);
    connect(m_ui->buttonTvPlay, &QToolButton::clicked, this, &CPreviewerPage::OnPlay);
    connect(m_ui->buttonPlayLoop, &QToolButton::clicked, this, &CPreviewerPage::OnLoop);
    connect(m_ui->buttonToggle1P, &QToolButton::clicked, this, &CPreviewerPage::OnToggle1P);
    connect(m_ui->buttonShowDebugOptions, &QToolButton::clicked, this, &CPreviewerPage::OnShowDebugOptions);
    connect(m_ui->buttonPreviewerNewSequence, &QToolButton::clicked, this, &CPreviewerPage::OnNewSequence);
    connect(m_ui->buttonPreviewerSaveSequence, &QToolButton::clicked, this, &CPreviewerPage::OnSaveSequence);
    connect(m_ui->buttonPreviewerLoadSequence, &QToolButton::clicked, this, &CPreviewerPage::OnLoadSequence);
    connect(m_ui->buttonTogglePreviewUnits, &QToolButton::clicked, this, &CPreviewerPage::OnToggleTimelineUnits);
    connect(m_ui->buttonMannReloadAnims, &QToolButton::clicked, this, &CPreviewerPage::OnReloadAnimations);
    connect(m_ui->buttonMannLookAtToggle, &QToolButton::clicked, this, &CPreviewerPage::OnToggleLookat);
    connect(m_ui->buttonMannRotateLocator, &QToolButton::clicked, this, &CPreviewerPage::OnClickRotateLocator);
    connect(m_ui->buttonMannTranslateLocator, &QToolButton::clicked, this, &CPreviewerPage::OnClickTranslateLocator);
    connect(m_ui->buttonMannAttachToEntity, &QToolButton::clicked, this, &CPreviewerPage::OnToggleAttachToEntity);
    connect(m_ui->buttonMannShowSceneRoots, &QToolButton::clicked, this, &CPreviewerPage::OnToggleShowSceneRoots);

    connect(m_ui->m_cDlgToolBar, &CSequencerDopeSheetToolbar::TimeChanged, this, &CPreviewerPage::OnTimeChanged);

    OnUpdateGoToStart();
    OnUpdateGoToEnd();
    OnUpdatePlay();
    OnUpdateLoop();
    OnUpdateNewSequence();
    OnUpdateLoadSequence();
    OnUpdateSaveSequence();
    OnUpdateTimeEdit();
}

void CPreviewerPage::InitToolbar()
{
    m_ui->m_cDlgToolBar->SetTime(m_fTime, m_ui->m_wndTrackPanel->GetSnapFps());

    QMenu* playMenu = new QMenu(m_ui->buttonTvPlay);
    m_ui->buttonTvPlay->setMenu(playMenu);
    connect(playMenu, &QMenu::aboutToShow, this, &CPreviewerPage::OnPlayMenu);
    m_ui->m_cDlgToolBar->SetTime(m_fTime, m_ui->m_wndTrackPanel->GetSnapFps());

    ValidateToolbarButtonsState();
}

void CPreviewerPage::ValidateToolbarButtonsState()
{
    m_ui->buttonMannLookAtToggle->setChecked(m_modelViewport->IsLookingAtCamera());
    m_ui->buttonMannRotateLocator->setChecked(!m_modelViewport->IsTranslateLocatorMode());
    m_ui->buttonMannTranslateLocator->setChecked(m_modelViewport->IsTranslateLocatorMode());
    m_ui->buttonMannAttachToEntity->setChecked(m_modelViewport->IsAttachedToEntity());
    m_ui->buttonMannShowSceneRoots->setChecked(m_modelViewport->IsShowingSceneRoots());
    m_ui->buttonToggle1P->setChecked(m_modelViewport->IsCameraAttached());
}

void CPreviewerPage::SaveLayout()
{
    const QString strSection = QStringLiteral("Settings\\Mannequin\\PreviewerLayout");

    MannUtils::SaveSplitterToRegistry(strSection, QStringLiteral("SequencerSplitter"), m_ui->m_wndSplitterTracks);
    MannUtils::SaveSplitterToRegistry(strSection, QStringLiteral("VerticalSplitter"), m_ui->m_wndSplitterVert);
    MannUtils::SaveSplitterToRegistry(strSection, QStringLiteral("HorizontalSplitter"), m_ui->m_wndSplitterHorz);
}

void CPreviewerPage::LoadLayout()
{
    const QString strSection = QStringLiteral("Settings\\Mannequin\\PreviewerLayout");

    MannUtils::LoadSplitterFromRegistry(strSection, QStringLiteral("SequencerSplitter"), m_ui->m_wndSplitterTracks, CMannequinDialog::s_minPanelSize);
    MannUtils::LoadSplitterFromRegistry(strSection, QStringLiteral("VerticalSplitter"), m_ui->m_wndSplitterVert, CMannequinDialog::s_minPanelSize);
    MannUtils::LoadSplitterFromRegistry(strSection, QStringLiteral("HorizontalSplitter"), m_ui->m_wndSplitterHorz, CMannequinDialog::s_minPanelSize);
}

void CPreviewerPage::focusInEvent(QFocusEvent* event)
{
    CMannequinEditorPage::focusInEvent(event);
    m_ui->m_wndNodes->SyncKeyCtrl();
}

float CPreviewerPage::GetMarkerTimeStart() const
{
    return m_ui->m_wndTrackPanel->GetMarkedTime().start;
}

float CPreviewerPage::GetMarkerTimeEnd() const
{
    return m_ui->m_wndTrackPanel->GetMarkedTime().end;
}

void CPreviewerPage::Update()
{
    bool isDraggingTime = m_ui->m_wndTrackPanel->IsDraggingTime();
    float dsTime = m_ui->m_wndTrackPanel->GetTime();

    float effectiveSpeed = m_playSpeed;
    if (isDraggingTime)
    {
        m_sequencePlayback->SetTime(dsTime);
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

    m_ui->m_wndTrackPanel->SetCurrTime(m_fTime);

    if (MannUtils::IsSequenceDirty(m_contexts, m_sequence.get(), m_lastChange, eMEM_Previewer))
    {
        OnUpdateTV();
    }

    m_ui->m_wndKeyProperties->OnUpdate();
    m_bRestartingSequence = false;

    // update the button logic
    ValidateToolbarButtonsState();

    OnUpdateGoToStart();
    OnUpdateGoToEnd();
    OnUpdatePlay();
    OnUpdateLoop();
    OnUpdateNewSequence();
    OnUpdateLoadSequence();
    OnUpdateSaveSequence();
    OnUpdateTimeEdit();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnGoToStart()
{
    m_fTime = 0.0f;
    m_bPlay = false;
    OnUpdatePlay();

    m_sequencePlayback->SetTime(0.0f);
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnGoToEnd()
{
    m_fTime = m_fMaxTime;
    m_bPlay = false;
    OnUpdatePlay();

    m_sequencePlayback->SetTime(m_sequence->GetTimeRange().end);
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnPlay()
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
            m_sequencePlayback->SetTime(startTime);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnLoop()
{
    m_bLoop = !m_bLoop;
    OnUpdateLoop();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnPlayMenu()
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

        connect(menu, &QMenu::triggered, [&](QAction* action){ m_playSpeed = action->data().toDouble(); });
    }

    for (QAction* action : menu->actions())
    {
        action->setChecked(action->data().toDouble() == m_playSpeed);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnUpdateGoToStart()
{
    m_ui->buttonTvJumpStart->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnUpdateGoToEnd()
{
    m_ui->buttonTvJumpEnd->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnUpdatePlay()
{
    m_ui->buttonTvPlay->setChecked(m_bPlay);
    m_ui->buttonTvPlay->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnUpdateLoop()
{
    m_ui->buttonPlayLoop->setChecked(m_bLoop);
    m_ui->buttonPlayLoop->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////

void CPreviewerPage::OnUpdateReloadAnimations()
{
    m_ui->buttonMannReloadAnims->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnUpdateNewSequence()
{
    m_ui->buttonPreviewerNewSequence->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnUpdateLoadSequence()
{
    m_ui->buttonPreviewerLoadSequence->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnUpdateSaveSequence()
{
    m_ui->buttonPreviewerSaveSequence->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnUpdateTimeEdit()
{
    m_ui->m_cDlgToolBar->setEnabled(CMannequinDialog::GetCurrentInstance()->PreviewFileLoaded());
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnToggle1P()
{
    m_modelViewport->ToggleCamera();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnShowDebugOptions()
{
    CMannequinDebugOptionsDialog dialog(m_modelViewport, this);
    dialog.exec();
    ValidateToolbarButtonsState();
}


//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnToggleTimelineUnits()
{
    // Enum has 2 states, so (oldMode + 1) % 2
    m_ui->m_wndTrackPanel->SetTickDisplayMode((ESequencerTickMode)((m_ui->m_wndTrackPanel->GetTickDisplayMode() + 1) & 1));
    if (m_modelViewport)
    {
        m_modelViewport->SetTimelineUnits(m_ui->m_wndTrackPanel->GetTickDisplayMode());
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnReloadAnimations()
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
void CPreviewerPage::OnToggleLookat()
{
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnClickRotateLocator()
{
    m_modelViewport->SetLocatorRotateMode();
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnClickTranslateLocator()
{
    m_modelViewport->SetLocatorTranslateMode();
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnToggleShowSceneRoots()
{
    m_modelViewport->SetShowSceneRoots(!m_modelViewport->IsShowingSceneRoots());
    ValidateToolbarButtonsState();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnToggleAttachToEntity()
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
float CPreviewerPage::PopulateClipTracks(CSequencerNode* node, const int scopeID)
{
    IActionController* pActionController = m_contexts->viewData[eMEM_Previewer].m_pActionController;
    IScope* scope = pActionController->GetScope(scopeID);

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

    // cache the frag Key indices of transitions,
    // they are - potentially - restored at the end of this method
    std::vector<int> selectedTransitions;
    //--- Strip temporary transition clips
    const int numFragKeysInitial = fragTrack->GetNumKeys();
    for (int i = numFragKeysInitial - 1; i >= 0; i--)
    {
        CFragmentKey fragKey;
        fragTrack->GetKey(i, &fragKey);
        if (fragKey.transition)
        {
            if (fragTrack->IsKeySelected(i))
            {
                selectedTransitions.push_back(i);
            }
            fragTrack->RemoveKey(i);
        }
    }

    const ActionScopes scopeFlag = (1 << scopeID);

    float maxTime = MIN_TIME_RANGE;
    SClipTrackContext trackContext;
    trackContext.tagState.globalTags = m_tagState;
    trackContext.context = m_contexts->m_scopeData[scopeID].context[eMEM_Previewer];

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

            trackContext.context = m_contexts->GetContextDataForID(trackContext.tagState.globalTags, contextID, eMEM_Previewer);
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
                            item.fragOptionIdxSel = m_contexts->viewData[eMEM_Previewer].m_pAnimContext->randGenerator.GenerateUint32();
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

                    blendQuery.forceBlendUid = m_forcedBlendUid;

                    SFragmentSelection fragmentSelection;
                    SFragmentData fragData;
                    const bool hasFragment = trackContext.context->database->Query(fragData, blendQuery, item.fragOptionIdxSel, trackContext.context->animSet, &fragmentSelection);
                    MannUtils::AdjustFragDataForVEGs(fragData, *trackContext.context, eMEM_Previewer, motionParams);
                    SBlendQueryResult blend1, blend2;
                    trackContext.context->database->FindBestBlends(blendQuery, blend1, blend2);
                    const SFragmentBlend* pFirstBlend = blend1.pFragmentBlend;
                    const bool hasTransition = (pFirstBlend != NULL);

                    float blendStartTime = 0.0f; // in 'track time'
                    float blendSelectTime = 0.0f; // in 'track time'
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

                                        float cycleIncrement = 0.0f;
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
                        const int id = fragTrack->CreateKey(transitionTime);
                        CFragmentKey fragKey;
                        fragTrack->GetKey(id, &fragKey);
                        fragKey.SetFromQuery(blend1);
                        fragKey.context                 = trackContext.context;
                        fragKey.clipDuration        = duration;
                        if (!blend2.pFragmentBlend)
                        {
                            fragKey.clipDuration += firstBlendDuration;
                        }
                        fragKey.tranSelectTime  = blendSelectTime;
                        fragKey.tranLastClipEffectiveStart = transitionClipEffectiveStartTime;
                        fragKey.tranLastClipDuration = transitionLastClipDuration;
                        fragKey.historyItem         =   trackContext.historyItem;
                        fragKey.fragIndex               = partID++;
                        fragTrack->SetKey(id, &fragKey);
                        transitionTime += duration;
                    }
                    if (blend2.pFragmentBlend)
                    {
                        const float duration = fragData.duration[partID];
                        const int id = fragTrack->CreateKey(transitionTime);
                        CFragmentKey fragKey;
                        fragTrack->GetKey(id, &fragKey);
                        fragKey.SetFromQuery(blend2);
                        fragKey.context                 = trackContext.context;
                        fragKey.clipDuration        = duration + firstBlendDuration;
                        fragKey.tranSelectTime  = blendSelectTime;
                        fragKey.tranLastClipEffectiveStart = transitionClipEffectiveStartTime;
                        fragKey.tranLastClipDuration = transitionLastClipDuration;
                        fragKey.historyItem         =   trackContext.historyItem;
                        fragKey.fragIndex               = partID++;
                        fragTrack->SetKey(id, &fragKey);
                        transitionTime += duration;
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

    fragTrack->SortKeys();
    for (uint32 i = 0; i < selectedTransitions.size(); i++)
    {
        if (fragTrack->GetNumKeys() > selectedTransitions[i])
        {
            fragTrack->SelectKey(selectedTransitions[i], true);
        }
    }

    return maxTime;
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnTimeChanged(float fTime)
{
    m_sequencePlayback->SetTime(fTime);
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::PopulateAllClipTracks()
{
    if (!m_fragmentHistory.get())
    {
        return;
    }

    m_contexts->viewData[eMEM_Previewer].m_pAnimContext->randGenerator.seed(m_seed);

    uint32 numScopes = m_contexts->m_scopeData.size();
    float maxTime = MIN_TIME_RANGE;
    for (uint32 i = 0; i < numScopes; i++)
    {
        CSequencerNode* animNode = m_contexts->m_scopeData[i].animNode[eMEM_Previewer];
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
void CPreviewerPage::SetUIFromHistory()
{
    if (!m_fragmentHistory.get())
    {
        return;
    }

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
        bool hasPossibleContext =  (m_contexts->potentiallyActiveScopes & (1 << scopeData.scopeID)) != 0;
        if (hasPossibleContext)
        {
            CScopeNode* animNode = new CScopeNode(m_sequence.get(), &scopeData, eMEM_Previewer);
            m_sequence->AddNode(animNode);

            if (m_contexts->m_scopeData[i].context[eMEM_Previewer])
            {
                animNode->SetName((m_contexts->m_scopeData[i].name + " (" + m_contexts->m_scopeData[i].context[eMEM_Previewer]->name + ")").toUtf8().data());
            }
            else
            {
                animNode->SetName((m_contexts->m_scopeData[i].name + " (none)").toUtf8().data());
            }

            float maxTime = MIN_TIME_RANGE;
            MannUtils::InsertFragmentTrackFromHistory(animNode, *m_fragmentHistory, 0.0f, 0.0f, maxTime, m_contexts->m_scopeData[i]);
            m_contexts->m_scopeData[i].fragTrack[eMEM_Previewer] = (CFragmentTrack*)animNode->GetTrackForParameter(SEQUENCER_PARAM_FRAGMENTID);// fragTrack;

            m_contexts->m_scopeData[i].animNode[eMEM_Previewer] = animNode;
        }
    }

    PopulateAllClipTracks();

    m_ui->m_wndNodes->SetSequence(m_sequence.get());

    if (m_sequencePlayback)
    {
        m_sequencePlayback->SetTime(m_fTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::SetHistoryFromUI()
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
void CPreviewerPage::OnUpdateTV()
{
    SetHistoryFromUI();

    PopulateAllClipTracks();

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

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::NewSequence()
{
    ResetForcedBlend();
    m_fragmentHistory->m_history.m_items.clear();
    SetUIFromHistory();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnNewSequence()
{
    NewSequence();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::LoadSequence(const QString& szFilename)
{
    ResetForcedBlend();

    QString filename;

    if (szFilename.isNull())
    {
        AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Mannequin Preview");
        AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
        if (!selection.IsValid())
        {
            return;
        }
        filename = selection.GetResult()->GetFullPath().c_str();
    }
    else
    {
        filename = szFilename;
    }


    if (m_fragmentHistory != nullptr)
    {
        m_fragmentHistory->m_history.LoadSequence(filename);
        m_fragmentHistory->m_history.UpdateScopeMasks(m_contexts, m_tagState);

        m_sequencePlayback->SetTime(0.0f);

        SetUIFromHistory();

        m_ui->m_wndNodes->SetSequence(m_sequence.get());
        m_ui->m_wndKeyProperties->SetSequence(m_sequence.get());
    }
}

void CPreviewerPage::LoadSequence(const int scopeContextIdx, const FragmentID fromID, const FragmentID toID, const SFragTagState fromFragTag, const SFragTagState toFragTag, const SFragmentBlendUid blendUid)
{
    m_forcedBlendUid = blendUid;

    // load the blend into the history
    m_fragmentHistory->m_history.LoadSequence(scopeContextIdx, fromID, toID, fromFragTag, toFragTag, blendUid, m_tagState);

    // this sets the keys onto each scope (track?)
    m_fragmentHistory->m_history.UpdateScopeMasks(m_contexts, m_tagState);

    m_sequencePlayback->SetTime(0.0f);

    // this goes away and populates everything like magic
    SetUIFromHistory();

    m_ui->m_wndNodes->SetSequence(m_sequence.get());
    m_ui->m_wndKeyProperties->SetSequence(m_sequence.get());
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnLoadSequence()
{
    LoadSequence();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::SaveSequence(const QString& szFilename)
{
    OnUpdateTV();

    QString fullFolder = ::GetFragmentSequenceFolder();
    Path::ConvertSlashToBackSlash(fullFolder);
    CFileUtil::CreateDirectory(fullFolder.toUtf8().data());

    QString filename = QStringLiteral("test.xml");
    if (!szFilename.isNull() || CFileUtil::SelectSaveFile(SEQUENCE_FILE_FILTER_QT, "xml", fullFolder, filename))
    {
        if (!szFilename.isNull())
        {
            filename = szFilename;
        }

        m_fragmentHistory->m_history.SaveSequence(filename);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnSaveSequence()
{
    SaveSequence();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnSequenceRestart(float newTime)
{
    m_modelViewport->OnSequenceRestart(newTime);
    m_bRestartingSequence = true;
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::PopulateTagList()
{
    m_tagVars->DeleteAllVariables();
    m_tagControls.Init(m_tagVars, m_contexts->viewData[eMEM_Previewer].m_pAnimContext->state.GetDef());

    RefreshTagsPanel();

    // set this up with the initial values we've already got
    SFragTagState fragTagState;
    fragTagState.globalTags = m_tagState;
    SetTagStateOnCtrl(fragTagState);
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::RefreshTagsPanel()
{
    m_ui->m_tagsPanel->DeleteVars();
    m_ui->m_tagsPanel->SetVarBlock(m_tagVars.get(), functor(*this, &CPreviewerPage::OnInternalVariableChange));
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name)
{
    if (tagDef == &m_contexts->viewData[eMEM_Previewer].m_pAnimContext->state.GetDef())
    {
        m_tagControls.RenameTag(tagID, name);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnTagRemoved(const CTagDefinition* tagDef, TagID tagID)
{
    if (tagDef == &m_contexts->viewData[eMEM_Previewer].m_pAnimContext->state.GetDef())
    {
        m_tagControls.RemoveTag(m_tagVars, tagID);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnTagAdded(const CTagDefinition* tagDef, const QString& name)
{
    if (tagDef == &m_contexts->viewData[eMEM_Previewer].m_pAnimContext->state.GetDef())
    {
        m_tagControls.AddTag(m_tagVars, name);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::OnInternalVariableChange(IVariable* pVar)
{
    if (!m_bRefreshingTagCtrl)
    {
        SFragTagState tagState = GetTagStateFromCtrl();

        const uint32 numContexts = m_contexts->m_controllerDef->m_scopeContexts.GetNum();
        for (uint32 i = 0; i < numContexts; i++)
        {
            const SScopeContextData* contextData = m_contexts->GetContextDataForID(tagState.globalTags, i, eMEM_Previewer);
            if (contextData)
            {
                const bool res = CMannequinDialog::GetCurrentInstance()->EnableContextData(eMEM_Previewer, contextData->dataID);
                if (!res)
                {
                    // Write out error msg via MessageBox
                    QMessageBox::critical(this, tr("Mannequin Error"), tr("Cannot set that value as it has no parent context.\nIs it a scope/barrel attachment that needs a gun tag setting first?"));

                    // reset the displayed info?
                    CMannequinDialog::GetCurrentInstance()->ClearContextData(eMEM_Previewer, i);
                    tagState.globalTags = m_tagState;
                    SetTagStateOnCtrl(tagState);
                }
            }
            else
            {
                CMannequinDialog::GetCurrentInstance()->ClearContextData(eMEM_Previewer, i);
            }
        }

        SetUIFromHistory();

        m_tagState = tagState.globalTags;
        m_contexts->viewData[eMEM_Previewer].m_pAnimContext->state = tagState.globalTags;
        OnUpdateTV();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::ResetForcedBlend()
{
    m_forcedBlendUid = SFragmentBlendUid();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::SetTagStateOnCtrl(const SFragTagState& tagState)
{
    m_bRefreshingTagCtrl = true;
    m_tagControls.Set(tagState.globalTags);
    m_bRefreshingTagCtrl = false;
}

//////////////////////////////////////////////////////////////////////////
SFragTagState CPreviewerPage::GetTagStateFromCtrl() const
{
    SFragTagState ret;
    ret.globalTags = m_tagControls.Get();

    return ret;
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::InitialiseToPreviewFile()
{
    ResetForcedBlend();

    m_modelViewport->SetActionController(m_contexts->viewData[eMEM_Previewer].m_pActionController);

    m_fragmentHistory.reset(new SFragmentHistoryContext(*m_contexts->viewData[eMEM_Previewer].m_pActionController));

    m_tagState = m_contexts->viewData[eMEM_Previewer].m_pAnimContext->state.GetMask();

    m_fMaxTime = m_fragmentHistory->m_history.m_endTime - m_fragmentHistory->m_history.m_firstTime;

    m_sequencePlayback = new CFragmentSequencePlayback(m_fragmentHistory->m_history, *m_contexts->viewData[eMEM_Previewer].m_pActionController, eMEM_Previewer);
    //m_sequencePlayback = new CActionSequencePlayback(ACTION_SCOPES_ALL, 0, 0, m_fragmentHistory->m_history, true);
    //m_sequencePlayback->SetMaxTime(m_fMaxTime);
    //m_contexts->viewData[eMEM_Previewer].m_pActionController->Queue(m_sequencePlayback);

    SetUIFromHistory();
    PopulateTagList();

    OnUpdateGoToStart();
    OnUpdateGoToEnd();
    OnUpdateNewSequence();
    OnUpdateLoadSequence();
    OnUpdateSaveSequence();
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::SetTagState(const TagState& tagState)
{
    const CTagDefinition& tagDef = m_contexts->m_controllerDef->m_tags;
    TagState tsNew = tagDef.GetUnion(m_tagState, tagState);
    m_contexts->viewData[eMEM_Previewer].m_pAnimContext->state = tsNew;

    //--- Updated associated contexts
    const uint32 numContexts = m_contexts->m_controllerDef->m_scopeContexts.GetNum();
    for (uint32 i = 0; i < numContexts; i++)
    {
        const SScopeContextData* contextData = m_contexts->GetContextDataForID(tsNew, i, eMEM_Previewer);
        if (contextData)
        {
            CMannequinDialog::GetCurrentInstance()->EnableContextData(eMEM_Previewer, contextData->dataID);
        }
        else
        {
            CMannequinDialog::GetCurrentInstance()->ClearContextData(eMEM_Previewer, i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPreviewerPage::SetTime(float fTime)
{
    if (m_sequencePlayback != NULL)
    {
        m_sequencePlayback->SetTime(fTime, true);
    }

    m_ui->m_wndTrackPanel->SetCurrTime(fTime, true);
}

#include <Mannequin/Controls/PreviewerPage.moc>
