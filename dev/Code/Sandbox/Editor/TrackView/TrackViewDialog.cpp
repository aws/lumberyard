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

// Description : CTrackViewDialog Implementation file.


#include "StdAfx.h"
#include "TrackViewDialog.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Maestro/Bus/EditorSequenceComponentBus.h>

#include "ViewPane.h"
#include "StringDlg.h"
#include "TVSequenceProps.h"
#include "ViewManager.h"
#include "AnimationContext.h"
#include "AnimationSerializer.h"
#include "TrackViewFindDlg.h"
#include "TrackViewUndo.h"
#include "TrackViewAnimNode.h"
#include "TrackViewTrack.h"
#include "TrackViewSequence.h"
#include "TrackViewSequenceManager.h"
#include "SequenceBatchRenderDialog.h"
#include "TVCustomizeTrackColorsDlg.h"
#include "QtUtilWin.h"
#include <AzQtComponents/Components/StyledDockWidget.h>

#include "Objects/ObjectLayerManager.h"
#include "Objects/EntityObject.h"

#include "IViewPane.h"
#include "PluginManager.h"
#include "Util/3DConnexionDriver.h"
#include "TrackViewNewSequenceDialog.h"

#include <IMovieSystem.h>
#include <IEntitySystem.h>

#include "Util/BoostPythonHelpers.h"

#include "Util/EditorUtils.h"
#include "FBXExporterDialog.h"
#include "Maestro/Types/AnimParamType.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/SequenceType.h"

#include <QFileDialog>
#include <QSplitter>
#include <QToolBar>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QAction>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QToolButton>
#include <QSettings>
#include <QKeyEvent>
#include <QTimer>

//////////////////////////////////////////////////////////////////////////
namespace
{
    const char* s_kTrackViewLayoutSection = "TrackViewLayout";
    const char* s_kTrackViewSection = "DockingPaneLayouts\\TrackView";
    const char* s_kSplitterEntry = "Splitter";
    const char* s_kVersionEntry = "TrackViewLayoutVersion";

    const char* s_kTrackViewSettingsSection = "TrackView";
    const char* s_kSnappingModeEntry = "SnappingMode";
    const char* s_kFrameSnappingFPSEntry = "FrameSnappingFPS";
    const char* s_kTickDisplayModeEntry = "TickDisplayMode";
    const char* s_kDefaultTracksEntry = "DefaultTracks";

    const char* s_kRebarVersionEntry = "TrackViewReBarVersion";
    const char* s_kRebarBandEntryPrefix = "ReBarBand";

    const char* s_kNoSequenceComboBoxEntry = "--- No Sequence ---";

    const int s_kMinimumFrameSnappingFPS = 1;
    const int s_kMaximumFrameSnappingFPS = 120;

    const int TRACKVIEW_LAYOUT_VERSION = 0x0001; // Bump this up on every substantial pane layout change
    const int TRACKVIEW_REBAR_VERSION = 0x0002; // Bump this up on every substantial rebar change
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions opts;
    opts.shortcut = QKeySequence(Qt::Key_T);
    opts.sendViewPaneNameBackToAmazonAnalyticsServers = true;

    AzToolsFramework::RegisterViewPane<CTrackViewDialog>(LyViewPane::TrackView, LyViewPane::CategoryTools, opts);
    GetIEditor()->GetSettingsManager()->AddToolName(s_kTrackViewLayoutSection, LyViewPane::TrackView);
}

const GUID& CTrackViewDialog::GetClassID()
{
    static const GUID guid =
    {
        0xd21c9fe5, 0x22d3, 0x41e3, { 0xb8, 0x4b, 0xa3, 0x77, 0xaf, 0xa0, 0xa0, 0x5c }
    };
    return guid;
}


//////////////////////////////////////////////////////////////////////////
CTrackViewDialog* CTrackViewDialog::s_pTrackViewDialog = NULL;

//////////////////////////////////////////////////////////////////////////
CTrackViewDialog::CTrackViewDialog(QWidget* pParent /*=NULL*/)
    : QMainWindow(pParent)
{
    s_pTrackViewDialog = this;
    m_bRecord = false;
    m_bAutoRecord = false;
    m_bPause = false;
    m_bPlay = false;
    m_fLastTime = -1.0f;
    m_fAutoRecordStep = 0.5f;
    m_bNeedReloadSequence = false;
    m_bIgnoreUpdates = false;
    m_bDoingUndoOperation = false;

    m_findDlg = nullptr;

    m_lazyInitDone = false;
    m_bEditLock = false;

    m_pNodeForTracksToolBar = NULL;

    m_currentToolBarParamTypeId = 0;

    IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
    m_defaultTracksForEntityNode.resize(pMovieSystem->GetEntityNodeParamCount());
    for (int i = 0; i < pMovieSystem->GetEntityNodeParamCount(); ++i)
    {
        CAnimParamType paramType = pMovieSystem->GetEntityNodeParamType(i);

        if (paramType == AnimParamType::Position || paramType == AnimParamType::Rotation || paramType == AnimParamType::Event)
        {
            // Pos,Rot and Event tracks are on by very default.
            m_defaultTracksForEntityNode[i] = 1;
        }
        else
        {
            m_defaultTracksForEntityNode[i] = 0;
        }
    }

    OnInitDialog();

    GetIEditor()->RegisterNotifyListener(this);
    GetIEditor()->GetObjectManager()->GetLayersManager()->AddUpdateListener(functor(*this, &CTrackViewDialog::OnLayerUpdate));
    GetIEditor()->GetAnimation()->AddListener(this);
    GetIEditor()->GetSequenceManager()->AddListener(this);
    GetIEditor()->GetUndoManager()->AddListener(this);
}

CTrackViewDialog::~CTrackViewDialog()
{
    SaveLayouts();
    SaveMiscSettings();
    SaveTrackColors();

    if (m_findDlg)
    {
        m_findDlg->deleteLater();
        m_findDlg = nullptr;
    }
    s_pTrackViewDialog = 0;

    const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
    CTrackViewSequence* pSequence = pSequenceManager->GetSequenceByEntityId(m_currentSequenceEntityId);
    if (pSequence)
    {
        pSequence->RemoveListener(this);
        pSequence->RemoveListener(m_wndNodesCtrl);
        pSequence->RemoveListener(m_wndKeyProperties);
        pSequence->RemoveListener(m_wndCurveEditor);
        pSequence->RemoveListener(m_wndDopeSheet);
    }

    GetIEditor()->GetUndoManager()->RemoveListener(this);
    GetIEditor()->GetSequenceManager()->RemoveListener(this);
    GetIEditor()->GetAnimation()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
    GetIEditor()->GetObjectManager()->GetLayersManager()->RemoveUpdateListener(functor(*this, &CTrackViewDialog::OnLayerUpdate));
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddEntityNodeMenu()
{
    QAction* a = static_cast<QAction*>(sender());
    int cmd = a ? a->data().toInt() : 0;
    if (cmd > 0)
    {
        IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
        int index = cmd - 1;
        assert(index < pMovieSystem->GetEntityNodeParamCount());

        IAnimNode::ESupportedParamFlags flags = pMovieSystem->GetEntityNodeParamFlags(index);
        if (flags & IAnimNode::eSupportedParamFlags_MultipleTracks)
        {
            int tracks = m_defaultTracksForEntityNode[index];
            bool ok = false;
            tracks = QInputDialog::getInt(this, tr("Multi-track count"), QStringLiteral(""), tracks, 0, 10, 1, &ok);
            if (ok)
            {
                m_defaultTracksForEntityNode[index] = tracks;
            }
        }
        else
        {
            m_defaultTracksForEntityNode[index] = 1 - m_defaultTracksForEntityNode[index];
        }
    }
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDialog::OnInitDialog()
{
    InitToolbar();
    InitMenu();

    QWidget* w = new QWidget();
    QVBoxLayout* l = new QVBoxLayout;
    l->setMargin(0);

    m_wndSplitter = new QSplitter(w);
    m_wndSplitter->setOrientation(Qt::Horizontal);

    m_wndNodesCtrl = new CTrackViewNodesCtrl(this, this);
    m_wndNodesCtrl->SetTrackViewDialog(this);

    m_wndDopeSheet = new CTrackViewDopeSheetBase(this);
    m_wndDopeSheet->SetTimeRange(0, 20);
    m_wndDopeSheet->SetTimeScale(100, 0);

    m_wndDopeSheet->SetNodesCtrl(m_wndNodesCtrl);
    m_wndNodesCtrl->SetDopeSheet(m_wndDopeSheet);

    m_wndSplitter->addWidget(m_wndNodesCtrl);
    m_wndSplitter->addWidget(m_wndDopeSheet);
    m_wndSplitter->setStretchFactor(0, 1);
    m_wndSplitter->setStretchFactor(1, 10);
    l->addWidget(m_wndSplitter);
    w->setLayout(l);
    setCentralWidget(w);

    m_wndKeyProperties = new CTrackViewKeyPropertiesDlg(this);
    QDockWidget* dw = new AzQtComponents::StyledDockWidget(this);
    dw->setObjectName("m_wndKeyProperties");
    dw->setWindowTitle("Key");
    dw->setWidget(m_wndKeyProperties);
    addDockWidget(Qt::RightDockWidgetArea, dw);
    m_wndKeyProperties->PopulateVariables();
    m_wndKeyProperties->SetKeysCtrl(m_wndDopeSheet);

    m_wndCurveEditorDock = new AzQtComponents::StyledDockWidget(this);
    m_wndCurveEditorDock->setObjectName("m_wndCurveEditorDock");
    m_wndCurveEditorDock->setWindowTitle("Curve Editor");
    m_wndCurveEditor = new TrackViewCurveEditorDialog(this);
    m_wndCurveEditorDock->setWidget(m_wndCurveEditor);
    addDockWidget(Qt::BottomDockWidgetArea, m_wndCurveEditorDock);
    m_wndCurveEditor->SetPlayCallback([this] { OnPlay();
        });

    InitSequences();

    m_lazyInitDone = false;

    QTimer::singleShot(0, this, SLOT(ReadLayouts()));
    //  ReadLayouts();
    ReadMiscSettings();
    ReadTrackColors();

    QString cursorPosText = QString("0.000(%1fps)").arg(FloatToIntRet(m_wndCurveEditor->GetFPS()));
    m_cursorPos->setText(cursorPosText);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::FillAddSelectedEntityMenu()
{
    QMenu* menu = qobject_cast<QMenu*>(sender());
    menu->clear();
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    SequenceType sequenceType = pSequence ? pSequence->GetSequenceType() : SequenceType::Legacy;

    IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
    for (int i = 0; i < pMovieSystem->GetEntityNodeParamCount(); ++i)
    {
        CAnimParamType paramType = pMovieSystem->GetEntityNodeParamType(i);

        QString paramName = pMovieSystem->GetEntityNodeParamName(i);
        IAnimNode::ESupportedParamFlags paramFlags = pMovieSystem->GetEntityNodeParamFlags(i);
        bool checked = false;

        if (m_defaultTracksForEntityNode[i] > 0)
        {
            checked = true;
            if (paramFlags & IAnimNode::eSupportedParamFlags_MultipleTracks)
            {
                paramName = QString("%1 x%2").arg(paramName).arg(m_defaultTracksForEntityNode[i]);
            }
        }

        bool enabled = true;
        if (sequenceType == SequenceType::SequenceComponent)
        {
            // For AZ::Entities with components, we can't assume there will be any particular component available, save for the Transform component (most of the time)
            // So we only support default tracks of Pos/Rot/Scale

            if (!(paramType.GetType() == AnimParamType::Position || paramType.GetType() == AnimParamType::Rotation || paramType.GetType() == AnimParamType::Scale))
            {
                // disable and uncheck everything but pos/rot/scale
                enabled = false;
                checked = false;
            }
        }
        QAction* action = menu->addAction(paramName);
        action->setCheckable(true);
        action->setChecked(checked);
        action->setData(i + 1);
        action->setEnabled(enabled);

        connect(action, &QAction::triggered, this, &CTrackViewDialog::OnAddEntityNodeMenu);
    }
}

void CTrackViewDialog::InitToolbar()
{
    m_mainToolBar = addToolBar("Sequence/Node Toolbar");
    m_mainToolBar->setObjectName("m_mainToolBar");
    m_mainToolBar->setFloatable(false);
    m_mainToolBar->addWidget(new QLabel("Sequence/Node:"));
    QAction* qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-00.png"), "Add Sequence");
    qaction->setData(ID_TV_ADD_SEQUENCE);
    m_actions[ID_TV_ADD_SEQUENCE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAddSequence);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-01.png"), "Delete Sequence");
    qaction->setData(ID_TV_DEL_SEQUENCE);
    m_actions[ID_TV_DEL_SEQUENCE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnDelSequence);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-02.png"), "Edit Sequence Properties");
    qaction->setData(ID_TV_EDIT_SEQUENCE);
    m_actions[ID_TV_EDIT_SEQUENCE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnEditSequence);
    m_sequencesComboBox = new QComboBox(this);
    m_sequencesComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_sequencesComboBox->setToolTip("Select the sequence");
    connect(m_sequencesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSequenceComboBox()));
    m_mainToolBar->addWidget(m_sequencesComboBox);
    m_mainToolBar->addSeparator();

    QToolButton* toolButton = new QToolButton(m_mainToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/main/tvmain-03.png"), "Add Selected Node", this);
    qaction->setData(ID_ADDNODE);
    m_actions[ID_ADDNODE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAddSelectedNode);
    toolButton->setDefaultAction(qaction);
    {
        QMenu* buttonMenu = new QMenu(this);
        toolButton->setMenu(buttonMenu);
        connect(buttonMenu, &QMenu::aboutToShow, this, &CTrackViewDialog::FillAddSelectedEntityMenu);
    }
    m_mainToolBar->addWidget(toolButton);

    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-04.png"), "Add Director Node");
    qaction->setData(ID_ADDSCENETRACK);
    m_actions[ID_ADDSCENETRACK] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAddDirectorNode);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-05.png"), "Find");
    qaction->setData(ID_FIND);
    m_actions[ID_FIND] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnFindNode);
    m_mainToolBar->addSeparator();
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-06.png"), "Toggle Disable");
    qaction->setCheckable(true);
    qaction->setData(ID_TRACKVIEW_TOGGLE_DISABLE);
    m_actions[ID_TRACKVIEW_TOGGLE_DISABLE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnToggleDisable);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-07.png"), "Toggle Mute");
    qaction->setCheckable(true);
    qaction->setData(ID_TRACKVIEW_TOGGLE_MUTE);
    m_actions[ID_TRACKVIEW_TOGGLE_MUTE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnToggleMute);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-08.png"), "Mute Selected Tracks");
    qaction->setData(ID_TRACKVIEW_MUTE_ALL);
    m_actions[ID_TRACKVIEW_MUTE_ALL] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnMuteAll);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-09.png"), "Unmute Selected Tracks");
    qaction->setData(ID_TRACKVIEW_UNMUTE_ALL);
    m_actions[ID_TRACKVIEW_UNMUTE_ALL] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnUnmuteAll);
    m_mainToolBar->addSeparator();
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-10.png"), "Create Light Animation Set");
    qaction->setData(ID_TV_CREATE_LIGHT_ANIMATION_SET);
    m_actions[ID_TV_CREATE_LIGHT_ANIMATION_SET] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnCreateLightAnimationSet);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-11.png"), "Add Light Animation Node");
    qaction->setData(ID_TV_ADD_LIGHT_ANIMATION_NODE);
    m_actions[ID_TV_ADD_LIGHT_ANIMATION_NODE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAddLightAnimation);

    m_viewToolBar = addToolBar("View Toolbar");
    m_viewToolBar->setObjectName("m_viewToolBar");
    m_viewToolBar->setFloatable(false);
    m_viewToolBar->addWidget(new QLabel("View:"));
    qaction = m_viewToolBar->addAction(QIcon(":/Trackview/view/tvview-00.png"), "Track Editor");
    qaction->setData(ID_TV_MODE_DOPESHEET);
    qaction->setShortcut(QKeySequence("Ctrl+D"));
    qaction->setCheckable(true);
    qaction->setChecked(true);
    m_actions[ID_TV_MODE_DOPESHEET] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnModeDopeSheet);
    qaction = m_viewToolBar->addAction(QIcon(":/Trackview/view/tvview-01.png"), "Curve Editor");
    qaction->setData(ID_TV_MODE_CURVEEDITOR);
    qaction->setShortcut(QKeySequence("Ctrl+R"));
    qaction->setCheckable(true);
    m_actions[ID_TV_MODE_CURVEEDITOR] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnModeCurveEditor);
    qaction = m_viewToolBar->addAction(QIcon(":/Trackview/view/tvview-02.png"), "Both");
    qaction->setData(ID_TV_MODE_OPENCURVEEDITOR);
    qaction->setShortcut(QKeySequence("Ctrl+B"));
    m_actions[ID_TV_MODE_OPENCURVEEDITOR] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnOpenCurveEditor);

    m_playToolBar = addToolBar("Play Toolbar");
    m_playToolBar->setObjectName("m_playToolBar");
    m_playToolBar->setFloatable(false);
    m_playToolBar->addWidget(new QLabel("Play:"));
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-00.png"), "Go to start of sequence");
    qaction->setData(ID_TV_JUMPSTART);
    m_actions[ID_TV_JUMPSTART] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnGoToStart);

    toolButton = new QToolButton(m_playToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/play/tvplay-01.png"), "Play Animation", this);
    qaction->setData(ID_TV_PLAY);
    qaction->setCheckable(true);
    m_actions[ID_TV_PLAY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnPlay);
    qaction->setShortcut(QKeySequence(Qt::Key_Space));
    qaction->setShortcutContext(Qt::WindowShortcut);
    toolButton->setDefaultAction(qaction);
    {
        QMenu* buttonMenu = new QMenu(this);
        toolButton->setMenu(buttonMenu);
        QActionGroup* ag = new QActionGroup(buttonMenu);
        for (auto i : { .5, 1., 2., 4., 8. })
        {
            if (i == .5)
            {
                qaction = buttonMenu->addAction(" 2 ");
            }
            else if (i == 1.)
            {
                qaction = buttonMenu->addAction(" 1 ");
            }
            else
            {
                qaction = buttonMenu->addAction(QString("1/%1").arg((int)i));
            }
            qaction->setData(i);
            connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnPlaySetScale);
            qaction->setCheckable(true);
            qaction->setChecked(i == 1.);
            ag->addAction(qaction);
        }
        buttonMenu->addSeparator();
        qaction = buttonMenu->addAction("Sequence Camera");
        connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnPlayToggleCamera);
        ag->addAction(qaction);
    }
    m_playToolBar->addWidget(toolButton);

    toolButton = new QToolButton(m_playToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/play/tvplay-02.png"), "Stop", this);
    qaction->setData(ID_TV_STOP);
    m_actions[ID_TV_STOP] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnStop);
    toolButton->setDefaultAction(qaction);
    {
        QMenu* buttonMenu = new QMenu(this);
        toolButton->setMenu(buttonMenu);
        qaction = buttonMenu->addAction("Stop");
        connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnStop);
        toolButton->addAction(qaction);
        qaction = buttonMenu->addAction("Stop with Hard Reset");
        qaction->setData(true);
        connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnStopHardReset);
    }
    m_playToolBar->addWidget(toolButton);

    m_playToolBar->addSeparator();
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-03.png"), "Pause");
    qaction->setData(ID_TV_PAUSE);
    qaction->setCheckable(true);
    m_actions[ID_TV_PAUSE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnPause);
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-04.png"), "Go to end of sequence");
    qaction->setData(ID_TV_JUMPEND);
    m_actions[ID_TV_JUMPEND] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnGoToEnd);

    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-05.png"), "Start Animation Recording");
    qaction->setData(ID_TV_RECORD);
    qaction->setCheckable(true);
    m_actions[ID_TV_RECORD] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnRecord);

    toolButton = new QToolButton(m_playToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/play/tvplay-06.png"), "Start Auto Recording", this);
    toolButton->addAction(qaction);
    toolButton->setDefaultAction(qaction);
    qaction->setData(ID_TV_RECORD_AUTO);
    qaction->setCheckable(true);
    m_actions[ID_TV_RECORD_AUTO] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAutoRecord);
    {
        QMenu* buttonMenu = new QMenu(this);
        toolButton->setMenu(buttonMenu);
        QActionGroup* ag = new QActionGroup(buttonMenu);
        for (auto i : { 1, 2, 5, 10, 25, 50, 100 })
        {
            if (i == 1)
            {
                qaction = buttonMenu->addAction(" 1 sec");
            }
            else
            {
                qaction = buttonMenu->addAction(QString("1/%1 sec").arg(i));
            }
            qaction->setData(i);
            connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAutoRecordStep);
            qaction->setCheckable(true);
            qaction->setChecked(i == 1);
            ag->addAction(qaction);
        }
    }
    m_playToolBar->addWidget(toolButton);

    m_playToolBar->addSeparator();
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-07.png"), "Loop");
    qaction->setData(ID_PLAY_LOOP);
    qaction->setCheckable(true);
    m_actions[ID_PLAY_LOOP] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnLoop);
    m_playToolBar->addSeparator();
    m_cursorPos = new QLabel(this);
    m_playToolBar->addWidget(m_cursorPos);
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-08.png"), "Frame Rate");
    qaction->setData(ID_TV_SNAP_FPS);
    qaction->setCheckable(true);
    m_actions[ID_TV_SNAP_FPS] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapFPS);
    m_activeCamStatic = new QLabel(this);
    m_playToolBar->addWidget(m_activeCamStatic);
    m_playToolBar->addSeparator();
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-09.png"), "Undo");
    qaction->setData(ID_UNDO);
    m_actions[ID_UNDO] = qaction;
    connect(qaction, &QAction::triggered, this, []()
        {
		GetIEditor()->Undo();
	});
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-10.png"), "Redo");
    qaction->setData(ID_REDO);
    m_actions[ID_REDO] = qaction;
    connect(qaction, &QAction::triggered, this, []()
        {
		GetIEditor()->Redo();
	});

    addToolBarBreak(Qt::TopToolBarArea);

    m_keysToolBar = addToolBar("Keys Toolbar");
    m_keysToolBar->setObjectName("m_keysToolBar");
    m_keysToolBar->setFloatable(false);
    m_keysToolBar->addWidget(new QLabel("Keys:"));
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-00.png"), "Go to previous key");
    qaction->setData(ID_TV_PREVKEY);
    m_actions[ID_TV_PREVKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnGoToPrevKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-01.png"), "Go to next key");
    qaction->setData(ID_TV_NEXTKEY);
    m_actions[ID_TV_NEXTKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnGoToNextKey);
    m_keysToolBar->addSeparator();
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-02.png"), "Move Keys");
    qaction->setData(ID_TV_MOVEKEY);
    m_actions[ID_TV_MOVEKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnMoveKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-03.png"), "Slide Keys");
    qaction->setData(ID_TV_SLIDEKEY);
    m_actions[ID_TV_SLIDEKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSlideKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-04.png"), "Scale Keys");
    qaction->setData(ID_TV_SCALEKEY);
    m_actions[ID_TV_SCALEKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnScaleKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-05.png"), "Add Keys");
    qaction->setData(ID_TV_ADDKEY);
    m_actions[ID_TV_ADDKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAddKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-06.png"), "Delete Keys");
    qaction->setData(ID_TV_DELKEY);
    m_actions[ID_TV_DELKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnDelKey);
    m_keysToolBar->addSeparator();
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-07.png"), "No Snapping");
    qaction->setData(ID_TV_SNAP_NONE);
    m_actions[ID_TV_SNAP_NONE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapNone);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-08.png"), "Magnet Snapping");
    qaction->setData(ID_TV_SNAP_MAGNET);
    m_actions[ID_TV_SNAP_MAGNET] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapMagnet);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-09.png"), "Frame Snapping");
    qaction->setData(ID_TV_SNAP_FRAME);
    m_actions[ID_TV_SNAP_FRAME] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapFrame);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-10.png"), "Tick Snapping");
    qaction->setData(ID_TV_SNAP_TICK);
    m_actions[ID_TV_SNAP_TICK] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapTick);
    m_keysToolBar->addSeparator();
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-11.png"), "Sync Selected Entity Nodes to Base Position");
    qaction->setData(ID_TV_SYNC_TO_BASE);
    m_actions[ID_TV_SYNC_TO_BASE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSyncSelectedTracksToBase);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-12.png"), "Sync Selected Entity Nodes from Base Position");
    qaction->setData(ID_TV_SYNC_FROM_BASE);
    m_actions[ID_TV_SYNC_FROM_BASE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSyncSelectedTracksFromBase);
    QActionGroup* ag = new QActionGroup(this);
    ag->addAction(m_actions[ID_TV_ADDKEY]);
    ag->addAction(m_actions[ID_TV_MOVEKEY]);
    ag->addAction(m_actions[ID_TV_SLIDEKEY]);
    ag->addAction(m_actions[ID_TV_SCALEKEY]);
    foreach(QAction * qaction, ag->actions())
    qaction->setCheckable(true);
    m_actions[ID_TV_MOVEKEY]->setChecked(true);
    ag = new QActionGroup(this);
    ag->addAction(m_actions[ID_TV_SNAP_NONE]);
    ag->addAction(m_actions[ID_TV_SNAP_MAGNET]);
    ag->addAction(m_actions[ID_TV_SNAP_FRAME]);
    ag->addAction(m_actions[ID_TV_SNAP_TICK]);
    foreach(QAction * qaction, ag->actions())
    qaction->setCheckable(true);
    m_actions[ID_TV_SNAP_NONE]->setChecked(true);

    m_tracksToolBar = addToolBar("Tracks Toolbar");
    m_tracksToolBar->setObjectName("m_tracksToolBar");
    m_tracksToolBar->setFloatable(false);
    ClearTracksToolBar();

    m_bRecord = false;
    m_bPause = false;
    m_bPlay = false;
}

void CTrackViewDialog::InitMenu()
{
    QMenuBar* mb = this->menuBar();

    QMenu* m = mb->addMenu("&Sequence");
    QAction* a = m->addAction("New Sequence...");
    a->setData(ID_TV_SEQUENCE_NEW);
    m_actions[ID_TV_SEQUENCE_NEW] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnAddSequence);

    m = mb->addMenu("&View");
    m->addAction(m_actions[ID_TV_MODE_DOPESHEET]);
    m->addAction(m_actions[ID_TV_MODE_CURVEEDITOR]);
    m->addAction(m_actions[ID_TV_MODE_OPENCURVEEDITOR]);
    m->addSeparator();
    a = m->addAction("Tick in Seconds");
    a->setData(ID_VIEW_TICKINSECONDS);
    a->setCheckable(true);
    m_actions[ID_VIEW_TICKINSECONDS] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnViewTickInSeconds);
    a = m->addAction("Tick in Frames");
    a->setData(ID_VIEW_TICKINFRAMES);
    a->setCheckable(true);
    m_actions[ID_VIEW_TICKINFRAMES] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnViewTickInFrames);

    m = mb->addMenu("T&ools");
    a = m->addAction("Render Output...");
    a->setData(ID_TOOLS_BATCH_RENDER);
    m_actions[ID_TOOLS_BATCH_RENDER] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnBatchRender);
    a = m->addAction("Customize &Track Colors...");
    a->setData(ID_TV_TOOLS_CUSTOMIZETRACKCOLORS);
    m_actions[ID_TV_TOOLS_CUSTOMIZETRACKCOLORS] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnCustomizeTrackColors);
}

void CTrackViewDialog::UpdateActions()
{
    if (m_bIgnoreUpdates || m_actions.empty())
    {
        return;
    }

    if (GetIEditor()->GetAnimation()->IsRecordMode())
    {
        m_actions[ID_TV_RECORD]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_RECORD]->setChecked(false);
    }
    if (GetIEditor()->GetAnimation()->IsAutoRecording())
    {
        m_actions[ID_TV_RECORD_AUTO]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_RECORD_AUTO]->setChecked(false);
    }
    if (GetIEditor()->GetAnimation()->IsPlayMode())
    {
        m_actions[ID_TV_PLAY]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_PLAY]->setChecked(false);
    }
    if (GetIEditor()->GetAnimation()->IsPaused())
    {
        m_actions[ID_TV_PAUSE]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_PAUSE]->setChecked(false);
    }
    if (GetIEditor()->GetAnimation()->IsLoopMode())
    {
        m_actions[ID_PLAY_LOOP]->setChecked(true);
    }
    else
    {
        m_actions[ID_PLAY_LOOP]->setChecked(false);
    }
    if (m_wndDopeSheet->GetTickDisplayMode() == eTVTickMode_InSeconds)
    {
        m_actions[ID_VIEW_TICKINSECONDS]->setChecked(true);
    }
    else
    {
        m_actions[ID_VIEW_TICKINSECONDS]->setChecked(false);
    }
    if (m_wndDopeSheet->GetTickDisplayMode() == eTVTickMode_InFrames)
    {
        m_actions[ID_VIEW_TICKINFRAMES]->setChecked(true);
    }
    else
    {
        m_actions[ID_VIEW_TICKINFRAMES]->setChecked(false);
    }

    m_actions[ID_TV_DEL_SEQUENCE]->setEnabled(m_bEditLock ? false : true);

    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        bool bLightAnimationSetActive = (QString(pSequence->GetName()) == LIGHT_ANIMATION_SET_NAME)
            && (pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet);

        if (m_bEditLock || bLightAnimationSetActive)
        {
            m_actions[ID_TV_EDIT_SEQUENCE]->setEnabled(false);
        }
        else
        {
            m_actions[ID_TV_EDIT_SEQUENCE]->setEnabled(true);
        }

        CTrackViewAnimNodeBundle selectedNodes = pSequence->GetSelectedAnimNodes();
        CTrackViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();

        const unsigned int selectedNodeCount = selectedNodes.GetCount();
        const unsigned int selectedTrackCount = selectedTracks.GetCount();

        bool updated_ID_TRACKVIEW_TOGGLE_DISABLE = false;
        bool updated_ID_TRACKVIEW_TOGGLE_MUTE = false;
        if (selectedNodeCount + selectedTrackCount == 1)
        {
            if (selectedNodeCount == 1)
            {
                CTrackViewAnimNode* pAnimNode = selectedNodes.GetNode(0);

                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(true);
                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setChecked(pAnimNode->IsDisabled() ? true : false);
                updated_ID_TRACKVIEW_TOGGLE_DISABLE = true;
            }

            if (selectedTrackCount == 1)
            {
                CTrackViewTrack* pTrack = selectedTracks.GetTrack(0);

                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(true);
                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setChecked(pTrack->IsDisabled() ? true : false);
                updated_ID_TRACKVIEW_TOGGLE_DISABLE = true;
                m_actions[ID_TRACKVIEW_TOGGLE_MUTE]->setEnabled(true);
                m_actions[ID_TRACKVIEW_TOGGLE_MUTE]->setChecked(pTrack->IsMuted() ? true : false);
                updated_ID_TRACKVIEW_TOGGLE_MUTE = true;
            }
        }

        bool allSelectedTracksUseMute = true;
        for (int i = 0; i < selectedTrackCount; i++)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            if (pTrack && !pTrack->UsesMute())
            {
                allSelectedTracksUseMute = false;
                break;
            }
        }

        if (!updated_ID_TRACKVIEW_TOGGLE_DISABLE)
        {
            m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(false);
        }
        // disable toggle mute if don't have a single track selected or the all selected tracks does not use Mute
        if (!updated_ID_TRACKVIEW_TOGGLE_MUTE || !allSelectedTracksUseMute)
        {
            m_actions[ID_TRACKVIEW_TOGGLE_MUTE]->setEnabled(false);
        }

        m_actions[ID_TRACKVIEW_MUTE_ALL]->setEnabled(true);

        m_actions[ID_TV_ADD_LIGHT_ANIMATION_NODE]->setEnabled(bLightAnimationSetActive);
        m_actions[ID_ADDSCENETRACK]->setEnabled(!bLightAnimationSetActive);
        if (GetIEditor()->GetSelection()->IsEmpty() || bLightAnimationSetActive)
        {
            m_actions[ID_ADDNODE]->setEnabled(false);
        }
        else
        {
            m_actions[ID_ADDNODE]->setEnabled(true);
        }
    }
    else
    {
        m_actions[ID_TV_DEL_SEQUENCE]->setEnabled(false);
        m_actions[ID_TV_EDIT_SEQUENCE]->setEnabled(false);
        m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(false);
        m_actions[ID_TRACKVIEW_TOGGLE_MUTE]->setEnabled(false);
        m_actions[ID_TRACKVIEW_MUTE_ALL]->setEnabled(false);
        m_actions[ID_TV_ADD_LIGHT_ANIMATION_NODE]->setEnabled(false);
        m_actions[ID_ADDSCENETRACK]->setEnabled(false);
        m_actions[ID_ADDNODE]->setEnabled(false);
        m_actions[ID_TV_CREATE_LIGHT_ANIMATION_SET]->setEnabled(false);
    }

    if (pSequence != nullptr)
    {
        // enable the Create Light Animation Set button only for legacy sequences
        if (pSequence->GetSequenceType() == SequenceType::Legacy)
        {
            IAnimSequence* pAnimSequence = GetIEditor()->GetMovieSystem()->FindLegacySequenceByName(LIGHT_ANIMATION_SET_NAME);
            if (pAnimSequence && pAnimSequence->GetSequenceType() == SequenceType::Legacy)
            {
                m_actions[ID_TV_CREATE_LIGHT_ANIMATION_SET]->setDisabled(pAnimSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet);
            }
            else
            {
                m_actions[ID_TV_CREATE_LIGHT_ANIMATION_SET]->setEnabled(true);
            }
        }
        else
        {
            m_actions[ID_TV_CREATE_LIGHT_ANIMATION_SET]->setEnabled(false);
        }    
    }
    m_actions[ID_TOOLS_BATCH_RENDER]->setEnabled((GetIEditor()->GetMovieSystem()->GetNumSequences() > 0) ? true : false);
    m_actions[ID_TV_ADD_SEQUENCE]->setEnabled(GetIEditor()->GetDocument() && GetIEditor()->GetDocument()->IsDocumentReady());
    m_actions[ID_TV_SEQUENCE_NEW]->setEnabled(GetIEditor()->GetDocument() && GetIEditor()->GetDocument()->IsDocumentReady());    
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::InitSequences()
{
    ReloadSequences();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::InvalidateSequence()
{
    m_bNeedReloadSequence = true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::InvalidateDopeSheet()
{
    if (m_wndDopeSheet)
    {
        m_wndDopeSheet->update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::Update()
{
    if (m_bNeedReloadSequence)
    {
        m_bNeedReloadSequence = false;
        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* pSequence = pSequenceManager->GetSequenceByEntityId(m_currentSequenceEntityId);

        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        pAnimationContext->SetSequence(pSequence, true, false);
    }

    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    float fTime = pAnimationContext->GetTime();

    if (fTime != m_fLastTime)
    {
        m_fLastTime = fTime;
        SetCursorPosText(fTime);
    }

    // Display the name of the active camera in the static control, if any.
    // The active camera node means two conditions:
    // 1. Sequence camera is currently active.
    // 2. The camera which owns this node has been set as the current camera by the director node.
    bool bSequenceCamInUse = gEnv->pMovieSystem->GetCallback() == NULL ||
        gEnv->pMovieSystem->GetCallback()->IsSequenceCamUsed();
    AZ::EntityId camId = gEnv->pMovieSystem->GetCameraParams().cameraEntityId;
    if (camId.IsValid() && bSequenceCamInUse)
    {
        CEntityObject* pEditorEntity = CEntityObject::FindFromEntityId(camId);
        if (pEditorEntity)
        {
            m_activeCamStatic->setText(pEditorEntity->GetName());
        }
        else
        {
            m_activeCamStatic->setText("Active Camera");
        }
    }
    else
    {
        m_activeCamStatic->setText("Active Camera");
    }
}


//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnGoToPrevKey()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    CTrackViewSequence* pSequence = pAnimationContext->GetSequence();

    if (pSequence)
    {
        float time = pAnimationContext->GetTime();

        CTrackViewNode* pNode = pSequence->GetFirstSelectedNode();
        pNode = pNode ? pNode : pSequence;

        if (pNode->SnapTimeToPrevKey(time))
        {
            pAnimationContext->SetTime(time);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnGoToNextKey()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    CTrackViewSequence* pSequence = pAnimationContext->GetSequence();

    if (pSequence)
    {
        float time = pAnimationContext->GetTime();

        CTrackViewNode* pNode = pSequence->GetFirstSelectedNode();
        pNode = pNode ? pNode : pSequence;

        if (pNode->SnapTimeToNextKey(time))
        {
            pAnimationContext->SetTime(time);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddKey()
{
    m_wndDopeSheet->SetMouseActionMode(eTVActionMode_AddKeys);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnDelKey()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    CTrackViewSequence* pSequence = pAnimationContext->GetSequence();

    if (pSequence)
    {
        CUndo undo("Delete Keys");
        pSequence->DeleteSelectedKeys();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnMoveKey()
{
    m_wndDopeSheet->SetMouseActionMode(eTVActionMode_MoveKey);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSlideKey()
{
    m_wndDopeSheet->SetMouseActionMode(eTVActionMode_SlideKey);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnScaleKey()
{
    m_wndDopeSheet->SetMouseActionMode(eTVActionMode_ScaleKey);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSyncSelectedTracksToBase()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        pSequence->SyncSelectedTracksToBase();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSyncSelectedTracksFromBase()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        pSequence->SyncSelectedTracksFromBase();
    }
}

void CTrackViewDialog::OnExportFBXSequence()
{
    SaveCurrentSequenceToFBX();
}

void CTrackViewDialog::OnExportNodeKeysGlobalTime()
{
    CExportManager* pExportManager = static_cast<CExportManager*>(GetIEditor()->GetExportManager());

    if (pExportManager)
    {
        pExportManager->SaveNodeKeysTimeToXML();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddSequence()
{
    CTVNewSequenceDialog dlg(this);

    if (dlg.exec() == QDialog::Accepted)
    {
        QString sequenceName = dlg.GetSequenceName();

        if (sequenceName != s_kNoSequenceComboBoxEntry)
        {
            SequenceType sequenceType = dlg.GetSequenceType();

            CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
            CTrackViewSequence* newSequence = nullptr;

            QString newSequenceCmd = QStringLiteral("trackview.new_sequence '%1' %2").arg(sequenceName, QStringLiteral("%1").arg(static_cast<int>(sequenceType)));

            if (sequenceType == SequenceType::Legacy)
            {
                CUndo undo("Add Sequence");
                GetIEditor()->ExecuteCommand(newSequenceCmd);
                newSequence = sequenceManager->GetSequenceByName(sequenceName);
                AZ_Assert(newSequence, "Creating new sequence failed.");
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Create TrackView Director Node");
                GetIEditor()->ExecuteCommand(newSequenceCmd);
                newSequence = sequenceManager->GetSequenceByName(sequenceName);
                AZ_Assert(newSequence, "Creating new sequence failed.");
                undoBatch.MarkEntityDirty(newSequence->GetSequenceComponentEntityId());
            }

            // make it the currently selected sequence
            CAnimationContext* animationContext = GetIEditor()->GetAnimation();
            animationContext->SetSequence(newSequence, true, false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::ReloadSequences()
{
    if (!GetIEditor()->GetMovieSystem() || m_bIgnoreUpdates || m_bDoingUndoOperation)
    {
        return;
    }

    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
    CTrackViewSequenceNoNotificationContext context(pSequence);

    if (pSequence)
    {
        pSequence->UnBindFromEditorObjects();
    }

    ClearTracksToolBar();

    if (pAnimationContext->IsPlaying())
    {
        pAnimationContext->SetPlaying(false);
    }

    ReloadSequencesComboBox();

    SaveZoomScrollSettings();

    if (m_currentSequenceEntityId.IsValid())
    {
        CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        pSequence = pSequenceManager->GetSequenceByEntityId(m_currentSequenceEntityId);

        const float prevTime = pAnimationContext->GetTime();
        pAnimationContext->SetSequence(pSequence, true, true);
        pAnimationContext->SetTime(prevTime);
    }
    else
    {
        pAnimationContext->SetSequence(nullptr, true, false);
        m_sequencesComboBox->setCurrentIndex(0);
    }

    if (pSequence)
    {
        pSequence->BindToEditorObjects();
    }

    pAnimationContext->ForceAnimation();

    UpdateSequenceLockStatus();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::ReloadSequencesComboBox()
{
    m_sequencesComboBox->blockSignals(true);
    m_sequencesComboBox->clear();
    m_sequencesComboBox->addItem(QString(s_kNoSequenceComboBoxEntry));

    {
        CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        const unsigned int numSequences = pSequenceManager->GetCount();

        for (int k = 0; k < numSequences; ++k)
        {
            CTrackViewSequence* pSequence = pSequenceManager->GetSequenceByIndex(k);
            QString entityIdString = GetEntityIdAsString(pSequence->GetSequenceComponentEntityId());
            m_sequencesComboBox->addItem(pSequence->GetName(), entityIdString);
        }
    }

    if (!m_currentSequenceEntityId.IsValid())
    {
        m_sequencesComboBox->setCurrentIndex(0);
    }
    else
    {
        QString entityIdString = GetEntityIdAsString(m_currentSequenceEntityId);
        m_sequencesComboBox->setCurrentIndex(m_sequencesComboBox->findData(entityIdString));
    }
    m_sequencesComboBox->blockSignals(false);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::UpdateSequenceLockStatus()
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || pSequence->IsLayerLocked())
    {
        SetEditLock(true);
    }
    else
    {
        SetEditLock(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::SetEditLock(bool bLock)
{
    m_bEditLock = bLock;

    m_wndDopeSheet->SetEditLock(bLock);
    m_wndNodesCtrl->SetEditLock(bLock);
    m_wndNodesCtrl->update();

    m_wndCurveEditor->SetEditLock(bLock);
    m_wndCurveEditor->update();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnLayerUpdate(int event, CObjectLayer* pLayer)
{
    switch (event)
    {
    case CObjectLayerManager::ON_LAYER_MODIFY:
    {
        UpdateSequenceLockStatus();
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnDelSequence()
{
    if (m_sequencesComboBox->currentIndex() == 0)
    {
        return;
    }

    if (QMessageBox::question(this, LyViewPane::TrackView, "Delete current sequence?") == QMessageBox::Yes)
    {
        int sel = m_sequencesComboBox->currentIndex();
        if (sel != -1)
        {
            QString entityIdString = m_sequencesComboBox->currentData().toString();
            m_sequencesComboBox->removeItem(sel);
            m_sequencesComboBox->setCurrentIndex(0);

            OnSequenceComboBox();

            if (!entityIdString.isEmpty())
            {
                AZ::EntityId entityId = AZ::EntityId(entityIdString.toULongLong());
                if (entityId.IsValid())
                {
                    GetIEditor()->ExecuteCommand(QStringLiteral("trackview.delete_sequence '%1'").arg(entityIdString));
                }
            }

            UpdateActions();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnEditSequence()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (pSequence)
    {
        float fps = m_wndCurveEditor->GetFPS();
        CTVSequenceProps dlg(pSequence, fps, this);
        if (dlg.exec() == QDialog::Accepted)
        {
            // Sequence updated.
            ReloadSequences();
        }
        m_wndDopeSheet->update();
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSequenceComboBox()
{
    int sel = m_sequencesComboBox->currentIndex();
    if (sel == -1)
    {
        GetIEditor()->GetAnimation()->SetSequence(nullptr, false, false);
        return;
    }

    // Display current sequence.
    QString entityIdString = m_sequencesComboBox->currentData().toString();
    GetIEditor()->ExecuteCommand(QStringLiteral("trackview.set_current_sequence '%1'").arg(entityIdString));
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSequenceChanged(CTrackViewSequence* sequence)
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    // Remove listeners from previous sequence
    CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
    CTrackViewSequence* prevSequence = sequenceManager->GetSequenceByEntityId(m_currentSequenceEntityId);
    if (prevSequence)
    {
        prevSequence->RemoveListener(this);
        prevSequence->RemoveListener(m_wndNodesCtrl);
        prevSequence->RemoveListener(m_wndKeyProperties);
        prevSequence->RemoveListener(m_wndCurveEditor);
        prevSequence->RemoveListener(m_wndDopeSheet);
    }

    if (sequence)
    {
        m_currentSequenceEntityId = sequence->GetSequenceComponentEntityId();

        sequence->Reset(true);
        SaveZoomScrollSettings();

        UpdateDopeSheetTime(sequence);

        CSequenceObject* sequenceObject = sequence->GetSequenceObject();
        if (sequenceObject && sequenceObject->HasValidZoomScrollSettings())
        {
            CSequenceObject::CZoomScrollSettings settings;
            sequenceObject->GetZoomScrollSettings(settings);
            m_wndDopeSheet->SetTimeScale(settings.dopeSheetZoom, 0);
            m_wndDopeSheet->SetScrollOffset(settings.dopeSheetHScroll);
            m_wndNodesCtrl->RestoreVerticalScrollPos(settings.nodesListVScroll);
            m_wndCurveEditor->GetSplineCtrl().SetZoom(settings.curveEditorZoom);
            m_wndCurveEditor->GetSplineCtrl().SetScrollOffset(settings.curveEditorScroll);
            m_wndCurveEditor->GetSplineCtrl().SetEditLock(false);
        }

        m_sequencesComboBox->blockSignals(true);
        QString entityIdString = GetEntityIdAsString(m_currentSequenceEntityId);
        int sequenceIndex = m_sequencesComboBox->findData(entityIdString);
        m_sequencesComboBox->setCurrentIndex(sequenceIndex);
        m_sequencesComboBox->blockSignals(false);

        sequence->ClearSelection();

        sequence->AddListener(this);
        sequence->AddListener(m_wndNodesCtrl);
        sequence->AddListener(m_wndKeyProperties);
        sequence->AddListener(m_wndCurveEditor);
        sequence->AddListener(m_wndDopeSheet);
    }
    else
    {
        m_currentSequenceEntityId.SetInvalid();
        m_sequencesComboBox->setCurrentIndex(0);
        m_wndCurveEditor->GetSplineCtrl().SetEditLock(true);
    }

    m_wndNodesCtrl->OnSequenceChanged();
    m_wndKeyProperties->OnSequenceChanged(sequence);

    ClearTracksToolBar();

    GetIEditor()->GetAnimation()->ForceAnimation();

    m_wndNodesCtrl->update();
    m_wndDopeSheet->update();

    UpdateSequenceLockStatus();
    UpdateTracksToolBar();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnRecord()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetRecording(!pAnimationContext->IsRecording());
    m_wndDopeSheet->update();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAutoRecord()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetAutoRecording(!pAnimationContext->IsRecording(), m_fAutoRecordStep);
    m_wndDopeSheet->update();
    UpdateActions();
}

void CTrackViewDialog::OnAutoRecordStep()
{
    QAction* action = static_cast<QAction*>(sender());
    int factor = action->data().toInt();
    m_fAutoRecordStep = 1.f / factor;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnGoToStart()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    const float startTime = pAnimationContext->GetMarkers().start;

    pAnimationContext->SetTime(startTime);
    pAnimationContext->SetPlaying(false);
    pAnimationContext->SetRecording(false);

    CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
    if (pSequence)
    {
        // Reset sequence to the beginning.
        pSequence->Reset(true);
    }

    // notify explicit time changed and return to playback controls *after* the sequence is reset.
    pAnimationContext->TimeChanged(startTime);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnGoToEnd()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetTime(pAnimationContext->GetMarkers().end);
    pAnimationContext->SetPlaying(false);
    pAnimationContext->SetRecording(false);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnPlay()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    bool wasRecordMode = pAnimationContext->IsRecordMode();
    if (!pAnimationContext->IsPlaying())
    {
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (pSequence)
        {
            if (!pAnimationContext->IsAutoRecording())
            {
                if (wasRecordMode)
                {
                    pAnimationContext->SetRecording(false);
                }
            }
            GetIEditor()->ExecuteCommand("trackview.play_sequence");
        }
    }
    else
    {
        GetIEditor()->ExecuteCommand("trackview.stop_sequence");
    }
    UpdateActions();
}

void CTrackViewDialog::OnPlaySetScale()
{
    QAction* action = static_cast<QAction*>(sender());
    float v = action->data().toFloat();
    if (v > 0.f)
    {
        GetIEditor()->GetAnimation()->SetTimeScale(1.f / v);
    }
}

void CTrackViewDialog::OnPlayToggleCamera()
{
    CViewport* pRendView = GetIEditor()->GetViewManager()->GetViewport(ET_ViewportCamera);
    if (pRendView)
    {
        pRendView->ToggleCameraObject();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnStop()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();

    if (pAnimationContext->IsPlaying())
    {
        pAnimationContext->SetPlaying(false);
    }
    else
    {
        OnGoToStart();
    }
    pAnimationContext->SetRecording(false);
    UpdateActions();
}

void CTrackViewDialog::OnStopHardReset()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetTime(pAnimationContext->GetMarkers().start);
    pAnimationContext->SetPlaying(false);
    pAnimationContext->SetRecording(false);

    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        pSequence->ResetHard();
    }
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnPause()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    if (pAnimationContext->IsPaused())
    {
        pAnimationContext->Resume();
    }
    else
    {
        pAnimationContext->Pause();
    }
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnLoop()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetLoopMode(!pAnimationContext->IsLoopMode());
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginNewScene:
    case eNotify_OnBeginLoad:
    case eNotify_OnBeginSceneSave:
    case eNotify_OnBeginGameMode:
        m_bIgnoreUpdates = true;
        break;
    case eNotify_OnEndNewScene:
    case eNotify_OnEndLoad:
        m_bIgnoreUpdates = false;
        ReloadSequences();
        break;
    case eNotify_OnEndSceneSave:
    case eNotify_OnEndGameMode:
        m_bIgnoreUpdates = false;
        break;
    case eNotify_OnMissionChange:
        if (!m_bIgnoreUpdates)
        {
            ReloadSequences();
        }
        break;
    case eNotify_OnReloadTrackView:
        if (!m_bIgnoreUpdates)
        {
            ReloadSequences();
        }
        break;
    case eNotify_OnIdleUpdate:
        if (!m_bIgnoreUpdates)
        {
            Update();
        }
        break;
    case eNotify_OnBeginSimulationMode:
        SetEditLock(true);
        m_sequencesComboBox->setEnabled(false);
        UpdateActions();
        break;
    case eNotify_OnEndSimulationMode:
        SetEditLock(false);
        m_sequencesComboBox->setEnabled(true);
        UpdateActions();
        break;
    case eNotify_OnSelectionChange:
        UpdateActions();
        break;
    case eNotify_OnQuit:
        SaveLayouts();
        SaveMiscSettings();
        SaveTrackColors();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddSelectedNode()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (sequence)
    {
        // Try to paste to a selected group node, otherwise to sequence
        CTrackViewAnimNodeBundle selectedNodes = sequence->GetSelectedAnimNodes();
        CTrackViewAnimNode* animNode = (selectedNodes.GetCount() == 1) ? selectedNodes.GetNode(0) : sequence;
        animNode = (animNode->IsGroupNode() && animNode->GetType() != AnimNodeType::AzEntity) ? animNode : sequence;

        CTrackViewAnimNodeBundle addedNodes;
        if (sequence->GetSequenceType() != SequenceType::Legacy)
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Add Entities to TrackView");
            addedNodes = animNode->AddSelectedEntities(m_defaultTracksForEntityNode);
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
        else
        {
            CUndo undo("Add Entities to TrackView");
            addedNodes = animNode->AddSelectedEntities(m_defaultTracksForEntityNode);
        }

        if (addedNodes.GetCount() > 0)
        {
            // mark layer containing sequence as dirty
            sequence->MarkAsModified();
        }

        // check to make sure all nodes were added and notify user if they weren't
        if (addedNodes.GetCount() != GetIEditor()->GetSelection()->GetCount())
        {
            IMovieSystem* movieSystem = GetIEditor()->GetMovieSystem();

            QMessageBox::information(this, tr("Track View Warning"), tr(movieSystem->GetUserNotificationMsgs().c_str()));

            // clear the notification log now that we've consumed and presented them.
            movieSystem->ClearUserNotificationMsgs();
        }

        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddDirectorNode()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (sequence)
    {
        QString name = sequence->GetAvailableNodeNameStartingWith("Director");
        if (sequence->GetSequenceType() == SequenceType::Legacy)
        {
            CUndo undo("Create TrackView Director Node");
            sequence->CreateSubNode(name, AnimNodeType::Director);
        }
        else
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Create TrackView Director Node");
            sequence->CreateSubNode(name, AnimNodeType::Director);
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnFindNode()
{
    if (!m_findDlg)
    {
        m_findDlg = new CTrackViewFindDlg("Find Node in Track View", this);
        m_findDlg->Init(this);
        connect(m_findDlg, SIGNAL(finished(int)), m_wndNodesCtrl->findChild<QTreeView*>(), SLOT(setFocus()), Qt::QueuedConnection);
    }
    m_findDlg->FillData();
    m_findDlg->show();
    m_findDlg->raise();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::keyPressEvent(QKeyEvent* event)
{
    // HAVE TO INCLUDE CASES FOR THESE IN THE ShortcutOverride handler in ::event() below
    if (event->key() == Qt::Key_Space && event->modifiers() == Qt::NoModifier)
    {
        event->accept();
        GetIEditor()->GetAnimation()->TogglePlay();
    }
    return QMainWindow::keyPressEvent(event);
}

bool CTrackViewDialog::event(QEvent* e)
{
    if (e->type() == QEvent::ShortcutOverride)
    {
        // since we respond to the following things, let Qt know so that shortcuts don't override us
        bool respondsToEvent = false;

        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_Space && keyEvent->modifiers() == Qt::NoModifier)
        {
            respondsToEvent = true;
        }

        if (respondsToEvent)
        {
            e->accept();
            return true;
        }
    }

    return QMainWindow::event(e);
}

#if defined(AZ_PLATFORM_WINDOWS)
bool CTrackViewDialog::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    /* On Windows, eventType is set to "windows_generic_MSG" for messages sent to toplevel windows, and "windows_dispatcher_MSG" for
    system - wide messages such as messages from a registered hot key.In both cases, the message can be casted to a MSG pointer.
    The result pointer is only used on Windows, and corresponds to the LRESULT pointer.*/

    if (eventType == "windows_generic_MSG") {
        MSG* pMsg = static_cast<MSG*>(message);
        return processRawInput(pMsg);
    }

    return false;
}

bool CTrackViewDialog::processRawInput(MSG* pMsg)
{
    if (pMsg->message == WM_INPUT)
    {
        static C3DConnexionDriver* p3DConnexionDriver = 0;

        if (!p3DConnexionDriver)
        {
            p3DConnexionDriver = (C3DConnexionDriver*)GetIEditor()->GetPluginManager()->GetPluginByGUID("{AD109901-9128-4ffd-8E67-137CB2B1C41B}");
        }
        if (p3DConnexionDriver)
        {
            S3DConnexionMessage msg;
            if (p3DConnexionDriver->GetInputMessageData(pMsg->lParam, msg))
            {
                if (msg.bGotRotation)
                {
                    float fTime = GetIEditor()->GetAnimation()->GetTime();
                    float fDelta2 = msg.vRotate[2] * 0.1f;
                    fTime += fDelta2;

                    GetIEditor()->GetAnimation()->SetTime(fTime);
                    return true;
                }
            }
        }
    }
    return false;
}
#endif

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnModeDopeSheet()
{
    auto sizes = m_wndSplitter->sizes();
    m_wndCurveEditorDock->setVisible(false);
    m_wndCurveEditorDock->toggleViewAction()->setEnabled(false);
    if (m_wndCurveEditorDock->widget() != m_wndCurveEditor)
    {
        m_wndCurveEditorDock->setWidget(m_wndCurveEditor);
    }
    m_wndDopeSheet->show();
    m_wndSplitter->setSizes(sizes);
    m_actions[ID_TV_MODE_DOPESHEET]->setChecked(true);
    m_actions[ID_TV_MODE_CURVEEDITOR]->setChecked(false);
    m_wndCurveEditor->OnSequenceChanged(GetIEditor()->GetAnimation()->GetSequence());
    m_lastMode = ViewMode::TrackView;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnModeCurveEditor()
{
    auto sizes = m_wndSplitter->sizes();
    m_wndCurveEditorDock->setVisible(false);
    m_wndCurveEditorDock->toggleViewAction()->setEnabled(false);
    if (m_wndCurveEditorDock->widget() == m_wndCurveEditor)
    {
        m_wndSplitter->insertWidget(1, m_wndCurveEditor);
    }
    m_wndDopeSheet->hide();
    m_wndSplitter->setSizes(sizes);
    m_actions[ID_TV_MODE_DOPESHEET]->setChecked(false);
    m_actions[ID_TV_MODE_CURVEEDITOR]->setChecked(true);
    m_wndCurveEditor->OnSequenceChanged(GetIEditor()->GetAnimation()->GetSequence());
    m_lastMode = ViewMode::CurveEditor;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnOpenCurveEditor()
{
    OnModeDopeSheet();
    m_wndCurveEditorDock->show();
    m_wndCurveEditorDock->toggleViewAction()->setEnabled(true);
    m_actions[ID_TV_MODE_DOPESHEET]->setChecked(true);
    m_actions[ID_TV_MODE_CURVEEDITOR]->setChecked(true);
    m_wndCurveEditor->OnSequenceChanged(GetIEditor()->GetAnimation()->GetSequence());
    m_lastMode = ViewMode::Both;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapNone()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapNone);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapMagnet()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapMagnet);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapFrame()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapFrame);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapTick()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapTick);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapFPS()
{
    int fps = FloatToIntRet(m_wndCurveEditor->GetFPS());
    bool ok = false;
    fps = QInputDialog::getInt(this, tr("Frame rate for frame snapping"), QStringLiteral(""), fps, s_kMinimumFrameSnappingFPS, s_kMaximumFrameSnappingFPS, 1, &ok);
    if (ok)
    {
        m_wndDopeSheet->SetSnapFPS(fps);
        m_wndCurveEditor->SetFPS(fps);

        SetCursorPosText(GetIEditor()->GetAnimation()->GetTime());
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnViewTickInSeconds()
{
    m_wndDopeSheet->SetTickDisplayMode(eTVTickMode_InSeconds);
    m_wndCurveEditor->SetTickDisplayMode(eTVTickMode_InSeconds);
    SetCursorPosText(GetIEditor()->GetAnimation()->GetTime());
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnViewTickInFrames()
{
    m_wndDopeSheet->SetTickDisplayMode(eTVTickMode_InFrames);
    m_wndCurveEditor->SetTickDisplayMode(eTVTickMode_InFrames);
    SetCursorPosText(GetIEditor()->GetAnimation()->GetTime());
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::SaveMiscSettings() const
{
    QSettings settings;
    settings.beginGroup(s_kTrackViewSettingsSection);
    settings.setValue(s_kSnappingModeEntry, static_cast<int>(m_wndDopeSheet->GetSnappingMode()));
    float fps = m_wndCurveEditor->GetFPS();
    settings.setValue(s_kFrameSnappingFPSEntry, fps);
    settings.setValue(s_kTickDisplayModeEntry, static_cast<int>(m_wndDopeSheet->GetTickDisplayMode()));
    settings.setValue(s_kDefaultTracksEntry, QByteArray(reinterpret_cast<const char*>(m_defaultTracksForEntityNode.data()),
        m_defaultTracksForEntityNode.size() * sizeof(uint32)));
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::ReadMiscSettings()
{
    QSettings settings;
    settings.beginGroup(s_kTrackViewSettingsSection);
    ESnappingMode snapMode = (ESnappingMode)(settings.value(s_kSnappingModeEntry, eSnappingMode_SnapNone).toInt());
    m_wndDopeSheet->SetSnappingMode(snapMode);
    if (snapMode == eSnappingMode_SnapNone)
    {
        m_actions[ID_TV_SNAP_NONE]->setChecked(true);
    }
    else if (snapMode == eSnappingMode_SnapMagnet)
    {
        m_actions[ID_TV_SNAP_MAGNET]->setChecked(true);
    }
    else if (snapMode == eSnappingMode_SnapTick)
    {
        m_actions[ID_TV_SNAP_TICK]->setChecked(true);
    }
    else if (snapMode == eSnappingMode_SnapFrame)
    {
        m_actions[ID_TV_SNAP_FRAME]->setChecked(true);
    }

    if (settings.contains(s_kFrameSnappingFPSEntry))
    {
        float fps = settings.value(s_kFrameSnappingFPSEntry).toDouble();
        if (fps >= s_kMinimumFrameSnappingFPS && fps <= s_kMaximumFrameSnappingFPS)
        {
            m_wndDopeSheet->SetSnapFPS(FloatToIntRet(fps));
            m_wndCurveEditor->SetFPS(fps);
        }
    }

    ETVTickMode tickMode = (ETVTickMode)(settings.value(s_kTickDisplayModeEntry, eTVTickMode_InSeconds).toInt());
    m_wndDopeSheet->SetTickDisplayMode(tickMode);
    m_wndCurveEditor->SetTickDisplayMode(tickMode);

    if (settings.contains(s_kDefaultTracksEntry))
    {
        const QByteArray ba = settings.value(s_kDefaultTracksEntry).toByteArray();
        if (ba.size() == sizeof(uint32) * m_defaultTracksForEntityNode.size())
        {
            memcpy(&m_defaultTracksForEntityNode[0], ba.data(), ba.size());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::SaveLayouts()
{
    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup("TrackView");
    QByteArray data = this->saveState();
    settings.setValue("layout", data);
    settings.setValue("lastViewMode", (int)m_lastMode);
    QStringList sl;
    foreach(int i, m_wndSplitter->sizes())
    sl << QString::number(i);
    settings.setValue("splitter", sl.join(","));
    settings.endGroup();
    settings.sync();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::ReadLayouts()
{
    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup("TrackView");

    setViewMode(static_cast<ViewMode>(settings.value("lastViewMode").toInt()));

    if (settings.contains("layout"))
    {
        QByteArray data = settings.value("layout").toByteArray();
        if (!data.isEmpty())
        {
            restoreState(data);
        }
    }
    if (settings.contains("splitter"))
    {
        const QStringList sl = settings.value("splitter").toString().split(",");
        QList<int> szl;
        szl.reserve(sl.size());
        for (const QString& s : sl)
        {
            szl << s.toInt();
        }
        if (!sl.isEmpty())
        {
            m_wndSplitter->setSizes(szl);
        }
    }
}

void CTrackViewDialog::setViewMode(ViewMode mode)
{
    switch (mode)
    {
    case ViewMode::TrackView:
        OnModeDopeSheet();
        break;
    case ViewMode::CurveEditor:
        OnModeCurveEditor();
        break;
    case ViewMode::Both:
        OnOpenCurveEditor();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::SaveTrackColors() const
{
    CTVCustomizeTrackColorsDlg::SaveColors(s_kTrackViewSettingsSection);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::ReadTrackColors()
{
    CTVCustomizeTrackColorsDlg::LoadColors(s_kTrackViewSettingsSection);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::SetCursorPosText(float fTime)
{
    QString sText;
    int fps = FloatToIntRet(m_wndCurveEditor->GetFPS());
    int nMins = int ( fTime / 60.0f );
    int nSecs = int (fTime - float ( nMins ) * 60.0f);
    int nFrames = fps > 0 ? int(fTime * m_wndCurveEditor->GetFPS()) % fps : 0;

    sText = QString("%1:%2:%3 (%4fps)").arg(nMins).arg(nSecs, 2, 10, QLatin1Char('0')).arg(nFrames, 2, 10, QLatin1Char('0')).arg(fps);
    m_cursorPos->setText(sText);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnCustomizeTrackColors()
{
    CTVCustomizeTrackColorsDlg dlg(this);
    dlg.exec();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnBatchRender()
{
    CSequenceBatchRenderDialog dlg(m_wndCurveEditor->GetFPS(), this);
    dlg.exec();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::UpdateTracksToolBar()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        ClearTracksToolBar();

        CTrackViewAnimNodeBundle selectedNodes = pSequence->GetSelectedAnimNodes();

        if (selectedNodes.GetCount() == 1)
        {
            CTrackViewAnimNode* pAnimNode = selectedNodes.GetNode(0);
            SetNodeForTracksToolBar(pAnimNode);

            const AnimNodeType nodeType = pAnimNode->GetType();
            int paramCount = 0;
            IAnimNode::AnimParamInfos animatableProperties;
            CTrackViewNode* parentNode = pAnimNode->GetParentNode();

            // all AZ::Entity entities are animated through components. Component nodes always have a parent - the containing AZ::Entity
            if (nodeType == AnimNodeType::Component && parentNode)
            {
                // component node - query all the animatable tracks via an EBus request

                // all AnimNodeType::Component are parented to AnimNodeType::AzEntityNodes - get the parent to get it's AZ::EntityId to use for the EBus request
                if (parentNode->GetNodeType() == eTVNT_AnimNode)
                {
                    // this cast is safe because we check that the type is eTVNT_AnimNode
                    const AZ::EntityId azEntityId = static_cast<CTrackViewAnimNode*>(parentNode)->GetAzEntityId();

                    // query the animatable component properties from the Sequence Component
                    Maestro::EditorSequenceComponentRequestBus::Event(const_cast<CTrackViewAnimNode*>(pAnimNode)->GetSequence()->GetSequenceComponentEntityId(),
                        &Maestro::EditorSequenceComponentRequestBus::Events::GetAllAnimatablePropertiesForComponent,
                        animatableProperties, azEntityId, pAnimNode->GetComponentId());

                    // also add any properties handled in CryMovie
                    pAnimNode->AppendNonBehaviorAnimatableProperties(animatableProperties);

                    paramCount = animatableProperties.size();
                }
            }
            else
            {
                // legacy Entity
                paramCount = pAnimNode->GetParamCount();
            }

            for (int i = 0; i < paramCount; ++i)
            {
                CAnimParamType paramType;
                QString name;

                // get the animatable param name
                if (nodeType == AnimNodeType::Component)
                {
                    paramType = animatableProperties[i].paramType;
                }
                else
                {

                    paramType = pAnimNode->GetParamType(i);

                    if (paramType.GetType() == AnimParamType::Invalid)
                    {
                        continue;
                    }
                }

                CTrackViewTrack* pTrack = pAnimNode->GetTrackForParameter(paramType);
                if (pTrack && (!pAnimNode->GetParamFlags(paramType) & IAnimNode::eSupportedParamFlags_MultipleTracks))
                {
                    continue;
                }

                name = pAnimNode->GetParamName(paramType);

                QString sToolTipText("Add " + name + " Track");
                QIcon hIcon = m_wndNodesCtrl->GetIconForTrack(pTrack);
                AddButtonToTracksToolBar(paramType, hIcon, sToolTipText);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::ClearTracksToolBar()
{
    m_tracksToolBar->clear();
    m_tracksToolBar->addWidget(new QLabel("Tracks:"));

    m_pNodeForTracksToolBar = NULL;
    m_toolBarParamTypes.clear();
    m_currentToolBarParamTypeId = 0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::AddButtonToTracksToolBar(const CAnimParamType& paramId, const QIcon& hIcon, const QString& title)
{
    const int paramTypeToolBarID = ID_TV_TRACKS_TOOLBAR_BASE + m_currentToolBarParamTypeId;
    if (paramTypeToolBarID <= ID_TV_TRACKS_TOOLBAR_LAST)
    {
        m_toolBarParamTypes.push_back(paramId);
        ++m_currentToolBarParamTypeId;

        QAction* a = m_tracksToolBar->addAction(hIcon, title);
        a->setData(paramTypeToolBarID);
        connect(a, &QAction::triggered, this, &CTrackViewDialog::OnTracksToolBar);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnTracksToolBar()
{
    QAction* action = static_cast<QAction*>(sender());
    int nID = action->data().toInt();
    const unsigned int paramTypeToolBarID = nID - ID_TV_TRACKS_TOOLBAR_BASE;

    if (paramTypeToolBarID < m_toolBarParamTypes.size())
    {
        if (m_pNodeForTracksToolBar && m_toolBarParamTypes[paramTypeToolBarID].GetType() != AnimParamType::Invalid)
        {
            CTrackViewSequence* sequence = m_pNodeForTracksToolBar->GetSequence();
            AZ_Assert(sequence, "Expected valid sequence");

            if (sequence && sequence->GetSequenceType() != SequenceType::Legacy)
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Track Via Toolbar");
                m_pNodeForTracksToolBar->CreateTrack(m_toolBarParamTypes[paramTypeToolBarID]);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
            else
            {
                CUndo undo("Create TrackView Track");
                m_pNodeForTracksToolBar->CreateTrack(m_toolBarParamTypes[paramTypeToolBarID]);
            }
            UpdateTracksToolBar();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnToggleDisable()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        CTrackViewAnimNodeBundle selectedNodes = pSequence->GetSelectedAnimNodes();
        const unsigned int numSelectedNodes = selectedNodes.GetCount();
        for (unsigned int i = 0; i < numSelectedNodes; ++i)
        {
            CTrackViewAnimNode* pNode = selectedNodes.GetNode(i);
            pNode->SetDisabled(!pNode->IsDisabled());
        }

        CTrackViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetDisabled(!pTrack->IsDisabled());
        }
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnToggleMute()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        CTrackViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetMuted(!pTrack->IsMuted());
        }
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnMuteAll()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        CTrackViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetMuted(true);
        }
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnUnmuteAll()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        CTrackViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetMuted(false);
        }
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::SaveZoomScrollSettings()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (pSequence)
    {
        CSequenceObject* pSequenceObject = pSequence->GetSequenceObject();
        if (!pSequenceObject)
        {
            return;
        }

        CSequenceObject::CZoomScrollSettings settings;
        settings.dopeSheetZoom = m_wndDopeSheet->GetTimeScale();
        settings.dopeSheetHScroll = m_wndDopeSheet->GetScrollPos();
        settings.nodesListVScroll = m_wndNodesCtrl->SaveVerticalScrollPos();
        settings.curveEditorZoom = m_wndCurveEditor->GetSplineCtrl().GetZoom();
        settings.curveEditorScroll = m_wndCurveEditor->GetSplineCtrl().GetScrollOffset();
        pSequenceObject->SetZoomScrollSettings(settings);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnCreateLightAnimationSet()
{
    const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
    assert(pSequenceManager && pSequenceManager->GetLegacySequenceByName(LIGHT_ANIMATION_SET_NAME) == NULL);

    CUndo undo("Add Sequence");
    // LightAnimationSequences are for animating legacy lights, so it's implied that the sequence type is  SequenceType::Legacy
    GetIEditor()->ExecuteCommand(QStringLiteral("trackview.new_sequence '%1' %2").arg(LIGHT_ANIMATION_SET_NAME).arg(static_cast<int>(SequenceType::Legacy)));

    CTrackViewSequence* pSequence = pSequenceManager->GetLegacySequenceByName(LIGHT_ANIMATION_SET_NAME);
    assert(pSequence);
    pSequence->SetFlags(IAnimSequence::eSeqFlags_LightAnimationSet);
    pSequence->MarkAsModified();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddLightAnimation()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (pSequence)
    {
        assert((QString(pSequence->GetName()) == LIGHT_ANIMATION_SET_NAME)
            && (pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet));

        StringDlg dlg(tr("New Light Animation"));
        while (true)
        {
            if (dlg.exec() == QDialog::Accepted)
            {
                if (pSequence->GetAnimNodesByName(dlg.GetString().toUtf8().data()).GetCount() > 0)
                {
                    QMessageBox::warning(this, tr("New Light Animation"),
                        tr("The name '%1' already exists!").arg(dlg.GetString()));
                    continue;
                }

                break;
            }
            else
            {
                return;
            }
        }

        CUndo undo("Add Node");
        CTrackViewAnimNode* pLightNode = pSequence->CreateSubNode(dlg.GetString(), AnimNodeType::Light);

        if (pLightNode)
        {
            pLightNode->CreateTrack(AnimParamType::Position);
            pLightNode->CreateTrack(AnimParamType::Rotation);
            pLightNode->CreateTrack(AnimParamType::LightDiffuse);
            pLightNode->CreateTrack(AnimParamType::LightRadius);

            pLightNode->SetSelected(true);

            UpdateActions();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnNodeSelectionChanged(CTrackViewSequence* pSequence)
{
    CTrackViewSequence* pCurrentSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (pCurrentSequence && pCurrentSequence == pSequence)
    {
        UpdateTracksToolBar();
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnNodeRenamed(CTrackViewNode* pNode, const char* pOldName)
{
    // React to sequence name changes
    if (pNode->GetNodeType() == eTVNT_Sequence)
    {
        ReloadSequencesComboBox();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::UpdateDopeSheetTime(CTrackViewSequence* pSequence)
{
    Range timeRange = pSequence->GetTimeRange();
    m_wndDopeSheet->SetTimeRange(timeRange.start, timeRange.end);
    m_wndDopeSheet->SetStartMarker(timeRange.start);
    m_wndDopeSheet->SetEndMarker(timeRange.end);
    m_wndDopeSheet->SetTimeScale(m_wndDopeSheet->GetTimeScale(), 0);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSequenceSettingsChanged(CTrackViewSequence* pSequence)
{
    CTrackViewSequence* pCurrentSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (pCurrentSequence && pCurrentSequence == pSequence)
    {
        UpdateDopeSheetTime(pSequence);
        m_wndNodesCtrl->update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSequenceAdded(CTrackViewSequence* pSequence)
{
    ReloadSequencesComboBox();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSequenceRemoved(CTrackViewSequence* pSequence)
{
    ReloadSequencesComboBox();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnLegacySequencePostLoad(CTrackViewSequence* sequence, bool undo)
{
    // If this happened as the result of an undo, the id of the legacy entity might change.
    // That id is used to select sequences in the in the combo box. So we need to refresh the
    // combo box here to make sure all the id's are valid. This code can be deleted
    // with legacy sequences.
    if (undo)
    {
        ReloadSequencesComboBox();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::BeginUndoTransaction()
{
    m_bDoingUndoOperation = true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::EndUndoTransaction()
{
    m_bDoingUndoOperation = false;
}

void CTrackViewDialog::SaveCurrentSequenceToFBX()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return;
    }

    QString selectedSequenceFBXStr = QString(pSequence->GetName()) + ".fbx";
    CExportManager* pExportManager = static_cast<CExportManager*>(GetIEditor()->GetExportManager());
    const char szFilters[] = "FBX Files (*.fbx)";

    CFBXExporterDialog fpsDialog;

    CTrackViewTrackBundle allTracks = pSequence->GetAllTracks();

    for (int trackID = 0; trackID < allTracks.GetCount(); ++trackID)
    {
        CTrackViewTrack* pCurrentTrack = allTracks.GetTrack(trackID);

        if (!pCurrentTrack->GetParentNode()->IsSelected())
        {
            continue;
        }
    }

    QString filename = QFileDialog::getSaveFileName(this, tr("Export Selected Nodes To FBX File"), selectedSequenceFBXStr, szFilters);
    if (!filename.isEmpty())
    {
        pExportManager->SetBakedKeysSequenceExport(true);
        pExportManager->Export(filename.toUtf8().data(), "", "", false, false, false, false, true);
    }
}


#include <TrackView/TrackViewDialog.moc>
