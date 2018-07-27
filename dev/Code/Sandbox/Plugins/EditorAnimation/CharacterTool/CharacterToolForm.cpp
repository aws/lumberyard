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

#include "pch.h"
#include "CharacterToolForm.h"
#include <QAction>
#include <QBoxLayout>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRegExp>
#include <QSplitter>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QProgressDialog>

#include "../EditorCommon/QPropertyTree/QPropertyTree.h"
#include "../EditorCommon/QViewport.h"
#include "../EditorCommon/DockTitleBarWidget.h"
#include "../EditorCommon/UnsavedChangesDialog.h"
#include <AzQtComponents/Components/StyledDockWidget.h>
#include "SplitViewport.h"
#include "CharacterDocument.h"
#include "AnimationList.h"
#include "Explorer.h"
#include "ExplorerPanel.h"
#include "PlaybackPanel.h"
#include "SkeletonList.h"
#include "CharacterList.h"
#include "EditorCompressionPresetTable.h"
#include <IEditor.h>
#include <Util/PathUtil.h> // for getting game folder
#include <ActionOutput.h>

#include "Expected.h"
#include "Serialization.h"
#include "Serialization/JSONOArchive.h"
#include "Serialization/JSONIArchive.h"

#include "AnimationTagList.h"
#include "ModeCharacter.h"
#include "Serialization/Decorators/INavigationProvider.h"
#include "GizmoSink.h"
#include "BlendSpacePreview.h"
#include "SceneParametersPanel.h"
#include "PropertiesPanel.h"
#include "AnimEventPresetPanel.h"

#include "DisplayParametersPanel.h"

#include "TransformPanel.h"
#include "CharacterToolSystem.h"
#include "SceneContent.h"
#include "DockWidgetManager.h"
#include "CharacterGizmoManager.h"

#include <QtViewPaneManager.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

extern CharacterTool::System* g_pCharacterToolSystem;

namespace CharacterTool {
    struct ViewportPlaybackHotkeyConsumer
        : public QViewportConsumer
    {
        PlaybackPanel* playbackPanel;

        ViewportPlaybackHotkeyConsumer(PlaybackPanel* playbackPanel)
            : playbackPanel(playbackPanel)
        {
        }

        void OnViewportKey(const SKeyEvent& ev) override
        {
            if (ev.type == SKeyEvent::PRESS && ev.key != Qt::Key_Delete && ev.key != Qt::Key_D && ev.key != Qt::Key_Z)
            {
                playbackPanel->HandleKeyEvent(ev.key);
            }
        }

        bool ProcessesViewportKey(const QKeySequence& key) override
        {
            // test the same keys as in OnViewportKey
            if (key != QKeySequence(Qt::Key_Delete) && key != QKeySequence(Qt::Key_D) && key != QKeySequence(Qt::Key_Z))
            {
                // let the playback panel decide
                return playbackPanel->ProcessesKey(key);
            }

            return false;
        }
    };

    static string GetStateFilename()
    {
        string result = GetIEditor()->GetResolvedUserFolder().toUtf8().data();
        result += "\\CharacterTool\\State.json";
        return result;
    }

    // ---------------------------------------------------------------------------

    struct CharacterToolForm::SPrivate
        : IEditorNotifyListener
    {
        SPrivate(CharacterToolForm* form, CharacterDocument* document)
            : form(form)
        {
            GetIEditor()->RegisterNotifyListener(this);
        }

        ~SPrivate()
        {
            GetIEditor()->UnregisterNotifyListener(this);
        }

        void OnEditorNotifyEvent(EEditorNotifyEvent event) override
        {
            if (event == eNotify_OnIdleUpdate)
            {
                form->OnIdleUpdate();
            }
            else if (event == eNotify_OnQuit)
            {
                form->SaveState(GetStateFilename(), false);
            }
        }

        CharacterToolForm* form;
    };


    CharacterToolForm::CharacterToolForm(QWidget* parent)
        : QMainWindow(parent)
        , m_system(g_pCharacterToolSystem)
        , m_stateLoaded(false)
        , m_dockWidgetManager(new DockWidgetManager(this, g_pCharacterToolSystem))
        , m_closed(false)
        , m_isRunningAsyncSave(false)
    {
        m_displayParametersSplitterWidths[0] = 400;
        m_displayParametersSplitterWidths[1] = 200;

        Initialize();
    }

    struct ActionCreator
    {
        QMenu* menu;
        QObject* handlerObject;
        bool checkable;

        ActionCreator(QMenu* menu, QObject* handlerObject, bool checkable)
            : menu(menu)
            , handlerObject(handlerObject)
            , checkable(checkable)
        {
        }


        QAction* operator()(const char* name, const char* slot)
        {
            QAction* action = menu->addAction(name);
            action->setCheckable(checkable);
            if (!EXPECTED(QObject::connect(action, SIGNAL(triggered()), handlerObject, slot)))
            {
                return 0;
            }
            return action;
        }
        QAction* operator()(const char* name, const char* slot, const QVariant& actionData)
        {
            QAction* action = menu->addAction(name);
            action->setCheckable(checkable);
            action->setData(actionData);
            if (!EXPECTED(QObject::connect(action, SIGNAL(triggered()), handlerObject, slot)))
            {
                return 0;
            }
            return action;
        }
    };

    void CharacterToolForm::UpdatePanesMenu()
    {
        if (m_menuView)
        {
            menuBar()->removeAction(m_menuView->menuAction());
        }
        delete m_menuView;
        m_menuView = createPopupMenu();
        m_menuView->setParent(this);
        m_menuView->setTitle("&View");
        menuBar()->insertMenu(menuBar()->actions()[1], m_menuView);
    }

    void CharacterToolForm::UpdateLayoutMenu()
    {
        m_menuLayout->clear();

        ActionCreator addToLayout(m_menuLayout, this, false);
        vector<string> layouts = FindLayoutNames();
        for (size_t i = 0; i < layouts.size(); ++i)
        {
            QAction* action = addToLayout(layouts[i].c_str(), SLOT(OnLayoutSet()), QVariant(layouts[i].c_str()));
        }
        if (!layouts.empty())
        {
            m_menuLayout->addSeparator();
        }
        m_actionLayoutSaveState = addToLayout("Save Layout...", SLOT(OnLayoutSave()));
        QMenu* removeMenu = m_menuLayout->addMenu("Remove");
        if (layouts.empty())
        {
            removeMenu->setEnabled(false);
        }
        else
        {
            for (size_t i = 0; i < layouts.size(); ++i)
            {
                QAction* action = removeMenu->addAction(layouts[i].c_str());
                action->setData(QVariant(QString(layouts[i].c_str())));
                connect(action, SIGNAL(triggered()), this, SLOT(OnLayoutRemove()));
            }
        }
        m_menuLayout->addSeparator();
        m_actionLayoutReset = addToLayout("&Reset to Default", SLOT(OnLayoutReset()));
    }

    void CharacterToolForm::Initialize()
    {
        m_private.reset(new SPrivate(this, m_system->document.get()));

        m_modeCharacter.reset(new ModeCharacter());

        setDockNestingEnabled(true);

        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalExplorerSelectionChanged()), this, SLOT(OnExplorerSelectionChanged())));
        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalCharacterLoaded()), this, SLOT(OnCharacterLoaded())));
        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalDisplayOptionsChanged(const DisplayOptions&)), this, SLOT(OnDisplayOptionsChanged(const DisplayOptions&))));

        QWidget* centralWidget = new QWidget();
        setCentralWidget(centralWidget);
        {
            centralWidget->setContentsMargins(0, 0, 0, 0);

            QBoxLayout* centralLayout = new QBoxLayout(QBoxLayout::TopToBottom, centralWidget);
            centralLayout->setMargin(0);
            centralLayout->setSpacing(0);

            {
                QBoxLayout* topLayout = new QBoxLayout(QBoxLayout::LeftToRight, 0);
                topLayout->setMargin(0);

                m_modeToolBar = new QToolBar();
                m_modeToolBar->setIconSize(QSize(32, 32));
                m_modeToolBar->setStyleSheet("QToolBar { border: 0px }");
                m_modeToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                m_modeToolBar->setFloatable(false);
                topLayout->addWidget(m_modeToolBar, 0);
                topLayout->addStretch(1);

                m_transformPanel = new TransformPanel();
                topLayout->addWidget(m_transformPanel);

                m_displayParametersButton = new QToolButton();
                m_displayParametersButton->setText("Display Options");
                m_displayParametersButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                m_displayParametersButton->setCheckable(true);
                m_displayParametersButton->setIcon(QIcon("Editor/Icons/animation/display_options.png"));
                EXPECTED(connect(m_displayParametersButton, SIGNAL(toggled(bool)), this, SLOT(OnDisplayParametersButton())));
                topLayout->addWidget(m_displayParametersButton);

                centralLayout->addLayout(topLayout, 0);
            }

            m_displayParametersSplitter = new QSplitter(Qt::Horizontal);
            centralLayout->addWidget(m_displayParametersSplitter, 1);

            m_splitViewport = new QSplitViewport(0);
            EXPECTED(connect(m_splitViewport->OriginalViewport(), SIGNAL(SignalPreRender(const SRenderContext&)), this, SLOT(OnPreRenderOriginal(const SRenderContext&))));
            EXPECTED(connect(m_splitViewport->OriginalViewport(), SIGNAL(SignalRender(const SRenderContext&)), this, SLOT(OnRenderOriginal(const SRenderContext&))));
            EXPECTED(connect(m_splitViewport->CompressedViewport(), SIGNAL(SignalPreRender(const SRenderContext&)), this, SLOT(OnPreRenderCompressed(const SRenderContext&))));
            EXPECTED(connect(m_splitViewport->CompressedViewport(), SIGNAL(SignalRender(const SRenderContext&)), this, SLOT(OnRenderCompressed(const SRenderContext&))));
            EXPECTED(connect(m_splitViewport->CompressedViewport(), SIGNAL(SignalUpdate()), this, SLOT(OnViewportUpdate())));
            m_displayParametersSplitter->addWidget(m_splitViewport);

            m_displayParametersPanel = new DisplayParametersPanel(0, m_system->document.get(), m_system->contextList->Tail());
            m_displayParametersPanel->setVisible(false);
            m_displayParametersSplitter->addWidget(m_displayParametersPanel);
            m_displayParametersSplitter->setStretchFactor(0, 1);
            m_displayParametersSplitter->setStretchFactor(1, 0);
            QList<int> sizes;
            sizes.push_back(400);
            sizes.push_back(200);
            m_displayParametersSplitter->setSizes(sizes);
        }

        m_splitViewport->CompressedViewport()->installEventFilter(this);

        QMenuBar* menu = new QMenuBar(this);
        QMenu* fileMenu = menu->addMenu("&File");
        EXPECTED(connect(fileMenu->addAction("&New Character..."), SIGNAL(triggered()), this, SLOT(OnFileNewCharacter())));
        EXPECTED(connect(fileMenu->addAction("&Open Character..."), SIGNAL(triggered()), this, SLOT(OnFileOpenCharacter())));
        QMenu* menuFileRecent = fileMenu->addMenu("&Recent Characters");
        QObject::connect(menuFileRecent, &QMenu::aboutToShow, this, [=]() { OnFileRecentAboutToShow(menuFileRecent); });
        EXPECTED(connect(fileMenu->addAction("&Save All"), SIGNAL(triggered()), this, SLOT(OnFileSaveAll())));

        m_menuLayout = menu->addMenu("&Layout");
        UpdateLayoutMenu();

        setMenuBar(menu);

        m_animEventPresetPanel = new AnimEventPresetPanel(this, m_system);

        m_playbackPanel = new PlaybackPanel(this, m_system, m_animEventPresetPanel);
        m_sceneParametersPanel = new SceneParametersPanel(this, m_system);
        m_blendSpacePreview = new BlendSpacePreview(this, m_system->document.get());

        m_viewportPlaybackHotkeyConsumer.reset(new ViewportPlaybackHotkeyConsumer(m_playbackPanel));
        m_splitViewport->OriginalViewport()->AddConsumer(m_viewportPlaybackHotkeyConsumer.get());
        m_splitViewport->CompressedViewport()->AddConsumer(m_viewportPlaybackHotkeyConsumer.get());

        InstallMode(m_modeCharacter.data(), m_system->document->GetActiveCharacterEntry());

        EXPECTED(connect(m_dockWidgetManager.get(), SIGNAL(SignalChanged()), SLOT(OnDockWidgetsChanged())));
        m_dockWidgetManager->AddDockWidgetType<ExplorerPanel>("Explorer", Qt::LeftDockWidgetArea, "");
        m_dockWidgetManager->AddDockWidgetType<PropertiesPanel>("Properties", Qt::RightDockWidgetArea, "display");

        ResetLayout();

        ReadViewportOptions(m_system->document->GetViewportOptions(), m_system->document->GetDisplayOptions()->animation);

        m_system->LoadGlobalData();
        m_animEventPresetPanel->LoadPresets();

        m_system->document->EnableAudio(true);
    }

    void CharacterToolForm::ResetLayout()
    {
        m_system->document->ScrubTime(0.0f, false);
        m_system->document->GetViewportOptions() = ViewportOptions();
        m_system->document->DisplayOptionsChanged();
        m_system->document->GetPlaybackOptions() = PlaybackOptions();
        m_system->document->PlaybackOptionsChanged();
        m_splitViewport->CompressedViewport()->ResetCamera();
        m_splitViewport->OriginalViewport()->ResetCamera();

        m_dockWidgetManager->ResetToDefault();

        CreateDefaultDockWidgets();
    }

    vector<string> CharacterToolForm::FindLayoutNames()
    {
        vector<string> result;
        QDir dir(GetIEditor()->GetResolvedUserFolder() + "\\CharacterTool\\Layouts");
        QStringList layouts = dir.entryList(QDir::Files, QDir::Name);
        for (size_t i = 0; i < layouts.size(); ++i)
        {
            QString name = layouts[i];
            name.replace(QRegExp("\\.layout$"), "");
            result.push_back(name.toLocal8Bit().data());
        }

        return result;
    }

    static QDockWidget* CreateDockWidget(std::vector<QDockWidget*>* dockWidgets, const char* title, const QString& name, QWidget* widget, Qt::DockWidgetArea area, CharacterToolForm* window)
    {
        QDockWidget* dock = new AzQtComponents::StyledDockWidget(title, window);
        dock->setObjectName(name);
        dock->setWidget(widget);
        CDockTitleBarWidget* titleBar = new CDockTitleBarWidget(dock);
        dock->setTitleBarWidget(titleBar);
        window->addDockWidget(area, dock);
        dockWidgets->push_back(dock);
        return dock;
    }

    void CharacterToolForm::CreateDefaultDockWidgets()
    {
        for (size_t i = 0; i < m_dockWidgets.size(); ++i)
        {
            QDockWidget* widget = m_dockWidgets[i];
            widget->setParent(0);
            removeDockWidget(widget);
            widget->deleteLater();
        }
        m_dockWidgets.clear();

        CreateDockWidget(&m_dockWidgets, "Playback", "playback", m_playbackPanel, Qt::BottomDockWidgetArea, this);
        QDockWidget* sceneDock = CreateDockWidget(&m_dockWidgets, "Scene Parameters", "scene_parameters", m_sceneParametersPanel, Qt::RightDockWidgetArea, this);
        sceneDock->raise();

        QDockWidget* dock = new AzQtComponents::StyledDockWidget("Blend Space Preview", this);
        dock->setObjectName("blend_space_preview");
        dock->setWidget(m_blendSpacePreview);
        addDockWidget(Qt::RightDockWidgetArea, dock);
        dock->hide();
        m_dockWidgets.push_back(dock);

        CreateDockWidget(&m_dockWidgets, "Animation Event Presets", "anim_event_presets", m_animEventPresetPanel, Qt::RightDockWidgetArea, this)->hide();

        m_dockWidgetManager->AddDockWidgetsToMainWindow(true);

        UpdatePanesMenu();
    }

    void CharacterToolForm::Serialize(Serialization::IArchive& ar)
    {
        if (ar.Filter(SERIALIZE_STATE))
        {
            ar(*m_system, "system");

            ar(*m_dockWidgetManager, "dockWidgetManager");
            if (ar.IsInput())
            {
                m_dockWidgetManager->AddDockWidgetsToMainWindow(false);
            }

            int windowStateVersion = 0;
            QByteArray windowState;
            if (ar.IsOutput())
            {
                windowState = saveState(windowStateVersion);
            }
            ar(windowState, "windowState");
            if (ar.IsInput() && !windowState.isEmpty())
            {
                restoreState(windowState, windowStateVersion);
            }

            ar(*m_displayParametersPanel, "displayPanel");

            if (ar.IsOutput() && m_displayParametersPanel->isVisible())
            {
                m_displayParametersSplitterWidths[0] = m_displayParametersSplitter->sizes()[0];
                m_displayParametersSplitterWidths[1] = m_displayParametersSplitter->sizes()[1];
            }
            ar(m_displayParametersSplitterWidths, "displayParameterSplittersWidths");
            bool displayOptionsVisible = !m_displayParametersPanel->isHidden();
            ar(displayOptionsVisible, "displayOptionsVisible");
            if (ar.IsInput())
            {
                m_displayParametersPanel->setVisible(displayOptionsVisible);
                QList<int> widths;
                widths.push_back(m_displayParametersSplitterWidths[0]);
                widths.push_back(m_displayParametersSplitterWidths[1]);
                m_displayParametersSplitter->setSizes(widths);
                m_displayParametersButton->setChecked(displayOptionsVisible);
            }


            if (m_playbackPanel)
            {
                ar(*m_playbackPanel, "playbackPanel");
            }
            if (m_blendSpacePreview)
            {
                ar(*m_blendSpacePreview, "blendSpacePreview");
            }
            if (m_animEventPresetPanel)
            {
                ar(*m_animEventPresetPanel, "animEventPresetPanel");
            }

            ar (m_recentCharacters, "recentCharacters");
            if (ar.IsInput())
            {
                UpdatePanesMenu();
            }
        }
    }

    void CharacterToolForm::OnIdleUpdate()
    {
        if (m_splitViewport)
        {
            m_splitViewport->OriginalViewport()->Update();
            m_splitViewport->CompressedViewport()->Update();
        }

        m_system->document->IdleUpdate();

        if (m_blendSpacePreview && m_blendSpacePreview->isVisible())
        {
            m_blendSpacePreview->IdleUpdate();
        }
    }

    void CharacterToolForm::OnFileSaveAll()
    {
        RunSaveAll();
    }

    void CharacterToolForm::RunSaveAll()
    {
        RunSaveAll(
            [](bool success)
            {
                if (success)
                {
                    CryLog("Geppetto - SaveAll success");
                }
                else
                {
                    CryLog("Geppetto - SaveAll failed");
                }
            }
            );
    }
    void CharacterToolForm::RunSaveAll(AZStd::function<void(bool)> onComplete)
    {
        if (m_isRunningAsyncSave)
        {
            return;
        }

        // Disable this control while saving
        setEnabled(false);

        AZStd::shared_ptr<AZ::ActionOutput> output = AZStd::make_shared<AZ::ActionOutput>();
        m_isRunningAsyncSave = true;
        m_system->explorer->SaveAll(output,
            [this, output, onComplete](bool success)
            {
                SaveState(GetStateFilename(), false);
                m_animEventPresetPanel->SavePresets();

                if (output->HasAnyErrors())
                {
                    QMessageBox::critical(this, "Geppetto - Save All Failed", output->BuildErrorMessage().c_str());
                }

                m_isRunningAsyncSave = false;
                setEnabled(true);

                if (onComplete)
                {
                    onComplete(success);
                }
            }
            );
    }

    void CharacterToolForm::OnFileNewCharacter()
    {
        QString filter = "Character Definitions (*.cdf)";
        QString absoluteFilename = QFileDialog::getSaveFileName(this, "Create Character...",
                QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str()).toLocal8Bit().data(), filter, &filter).toLocal8Bit().data();
        if (absoluteFilename.isEmpty())
        {
            return;
        }

        QDir dir(QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str()));

        AZStd::shared_ptr<AZ::ActionOutput> actionOutput = AZStd::make_shared<AZ::ActionOutput>();
        string filename = dir.relativeFilePath(absoluteFilename).toLocal8Bit().data();
        m_system->characterList->AddAndSaveEntry(filename.c_str(), actionOutput,
            [this, filename, actionOutput](bool success)
            {
                if (!success)
                {
                    QMessageBox::critical(this, "Geppetto - Add And Save Failed", actionOutput->BuildErrorMessage().c_str());
                    return;
                }

                m_system->scene->characterPath = filename;
                m_system->scene->SignalCharacterChanged();

                ExplorerEntry* entry = m_system->explorer->FindEntryByPath(SUBTREE_CHARACTERS, filename.c_str());
                if (entry)
                {
                    m_system->document->SetSelectedExplorerEntries(ExplorerEntries(1, entry), 0);
                }
            }
            );
    }

    void CharacterToolForm::OnFileOpenCharacter()
    {
        string currentScene = m_system->scene->characterPath;
        AZStd::string path;
        if (currentScene.empty())
        {
            path = "Objects";
        }
        else
        {
            path = currentScene;
        }

        AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Character Definition");
        AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
        if (!selection.IsValid())
        {
            return;
        }

        m_system->scene->characterPath = Path::FullPathToGamePath(selection.GetResult()->GetFullPath().c_str());
        m_system->scene->SignalCharacterChanged();
    }

    void CharacterToolForm::OnLayoutReset()
    {
        ResetLayout();
    }

    void CharacterToolForm::OnLayoutSet()
    {
        QAction* action = qobject_cast<QAction*>(sender());
        if (action)
        {
            QString name = action->data().toString();
            if (!name.isEmpty())
            {
                LoadLayout(name.toLocal8Bit().data());
            }
        }
    }

    void CharacterToolForm::OnLayoutRemove()
    {
        QAction* action = qobject_cast<QAction*>(sender());
        if (action)
        {
            QString name = action->data().toString();
            if (!name.isEmpty())
            {
                RemoveLayout(name.toLocal8Bit().data());
            }
        }
    }

    void CharacterToolForm::ReadViewportOptions(const ViewportOptions& options, const DisplayAnimationOptions& animationOptions)
    {
    }


    void CharacterToolForm::OnDisplayParametersButton()
    {
        bool visible = m_displayParametersButton->isChecked();
        if (!visible)
        {
            m_displayParametersSplitterWidths[0] = m_displayParametersSplitter->sizes()[0];
            m_displayParametersSplitterWidths[1] = m_displayParametersSplitter->sizes()[1];
        }

        m_displayParametersPanel->setVisible(visible);
        if (visible)
        {
            QList<int> widths;
            widths.push_back(m_displayParametersSplitterWidths[0]);
            widths.push_back(m_displayParametersSplitterWidths[1]);
            m_displayParametersSplitter->setSizes(widths);
        }
    }

    void CharacterToolForm::OnDisplayOptionsChanged(const DisplayOptions& settings)
    {
        SViewportSettings vpSettings(settings.viewport);
        m_splitViewport->CompressedViewport()->SetSettings(vpSettings);
        m_splitViewport->OriginalViewport()->SetSettings(vpSettings);

        bool isOriginalAnimationShown = m_splitViewport->IsSplit();
        bool newShowOriginalAnimation = settings.animation.compressionPreview != COMPRESSION_PREVIEW_COMPRESSED;
        if (newShowOriginalAnimation != isOriginalAnimationShown)
        {
            m_splitViewport->SetSplit(newShowOriginalAnimation);
            if (newShowOriginalAnimation)
            {
                m_system->document->SyncPreviewAnimations();
            }
        }

        // deactivate grid
        vpSettings.grid.showGrid = false;
        m_blendSpacePreview->GetViewport()->SetSettings(vpSettings);
    }

    void CharacterToolForm::OnPreRenderCompressed(const SRenderContext& context)
    {
        m_system->document->PreRender(context);
    }

    void CharacterToolForm::OnRenderCompressed(const SRenderContext& context)
    {
        m_system->document->Render(context);
    }

    void CharacterToolForm::OnPreRenderOriginal(const SRenderContext& context)
    {
        m_system->document->PreRenderOriginal(context);
    }

    void CharacterToolForm::OnRenderOriginal(const SRenderContext& context)
    {
        m_system->document->RenderOriginal(context);
    }


    void CharacterToolForm::OnCharacterLoaded()
    {
        ICharacterInstance* character = m_system->document->CompressedCharacter();

        const char* filename = m_system->document->LoadedCharacterFilename();
        for (size_t i = 0; i < m_recentCharacters.size(); ++i)
        {
            if (azstricmp(m_recentCharacters[i].c_str(), filename) == 0)
            {
                m_recentCharacters.erase(m_recentCharacters.begin() + i);
                --i;
            }
        }

        enum
        {
            NUM_RECENT_FILES = 10
        };

        m_recentCharacters.insert(m_recentCharacters.begin(), filename);
        if (m_recentCharacters.size() > NUM_RECENT_FILES)
        {
            m_recentCharacters.resize(NUM_RECENT_FILES);
        }
        SaveState(GetStateFilename(), false);

        ExplorerEntries entries;
        if (ExplorerEntry* charEntry = m_system->document->GetActiveCharacterEntry())
        {
            entries.push_back(charEntry);
        }
        m_system->document->SetSelectedExplorerEntries(entries, 0);
    }

    void CharacterToolForm::UpdateViewportMode(ExplorerEntry* newEntry)
    {
        if (m_modeEntry == newEntry)
        {
            return;
        }
        if (!newEntry)
        {
            return;
        }

        if (!m_mode)
        {
            InstallMode(m_modeCharacter.data(), newEntry);
        }
    }


    void CharacterToolForm::InstallMode(IViewportMode* mode, ExplorerEntry* modeEntry)
    {
        if (m_mode)
        {
            m_splitViewport->CompressedViewport()->RemoveConsumer(m_mode);
            m_mode->LeaveMode();
            m_mode = 0;
        }

        m_system->gizmoSink->SetScene(0);
        m_mode = mode;
        m_modeEntry = modeEntry;

        if (!m_modeToolBar)
        {
            return;
        }

        m_modeToolBar->clear();

        if (m_mode)
        {
            SModeContext ctx;
            ctx.window = this;
            ctx.system = m_system;
            ctx.document = m_system->document.get();
            ctx.character = m_system->document->CompressedCharacter();
            ctx.transformPanel = m_transformPanel;
            ctx.toolbar = m_modeToolBar;
            ctx.layerPropertyTrees.resize(GIZMO_LAYER_COUNT);
            for (int layer = 0; layer < GIZMO_LAYER_COUNT; ++layer)
            {
                ctx.layerPropertyTrees[layer] = m_system->characterGizmoManager->Tree((GizmoLayer)layer);
            }

            m_mode->EnterMode(ctx);

            m_splitViewport->CompressedViewport()->AddConsumer(m_mode);
        }
        else
        {
            // dummy toolbar to keep panel recognizable
            QAction* action = m_modeToolBar->addAction(QIcon("Editor/Icons/animation/tool_move.png"), QString());
            action->setEnabled(false);
            action->setPriority(QAction::LowPriority);
            action = m_modeToolBar->addAction(QIcon("Editor/Icons/animation/tool_rotate.png"), QString());
            action->setEnabled(false);
            action->setPriority(QAction::LowPriority);
            action = m_modeToolBar->addAction(QIcon("Editor/Icons/animation/tool_scale.png"), QString());
            action->setEnabled(false);
            action->setPriority(QAction::LowPriority);
        }
    }


    void CharacterToolForm::OnExplorerSelectionChanged()
    {
        ExplorerEntries entries;
        m_system->document->GetSelectedExplorerEntries(&entries);

        if (entries.size() == 1)
        {
            ExplorerEntry* selectedEntry = entries[0];
            if (m_system->document->IsActiveInDocument(selectedEntry))
            {
                InstallMode(m_modeCharacter.data(), selectedEntry);
            }
        }
    }

    void CharacterToolForm::OnViewportOptionsChanged()
    {
        ReadViewportOptions(m_system->document->GetViewportOptions(), m_system->document->GetDisplayOptions()->animation);
    }

    void CharacterToolForm::OnViewportUpdate()
    {
        // This is called when viewport is asking for redraw, e.g. during the resize
        OnIdleUpdate();
    }

    void CharacterToolForm::OnAnimEventPresetPanelPutEvent()
    {
    }

    bool CharacterToolForm::event(QEvent* ev)
    {
        if (ev->type() == QEvent::WindowActivate)
        {
            GetIEditor()->SetActiveView(0);

            m_system->document->EnableAudio(true);
            if (m_splitViewport)
            {
                m_splitViewport->OriginalViewport()->SetForegroundUpdateMode(true);
                m_splitViewport->CompressedViewport()->SetForegroundUpdateMode(true);
            }
        }
        else if (ev->type() == QEvent::WindowDeactivate)
        {
            m_system->document->EnableAudio(false);
            if (m_splitViewport)
            {
                m_splitViewport->OriginalViewport()->SetForegroundUpdateMode(false);
                m_splitViewport->CompressedViewport()->SetForegroundUpdateMode(false);
            }
        }

        return QMainWindow::event(ev);
    }


    void CharacterToolForm::SaveState(const char* filename, bool layoutOnly)
    {
        QDir().mkdir(GetIEditor()->GetResolvedUserFolder() + "\\CharacterTool");

        Serialization::JSONOArchive oa;
        Serialization::SContext<CharacterToolForm> formContext(oa, this);
        if (layoutOnly)
        {
            oa.SetFilter(SERIALIZE_STATE | SERIALIZE_LAYOUT);
        }
        else
        {
            oa.SetFilter(SERIALIZE_STATE);
        }
        oa(*this, "state");
        oa.save(filename);
    }

    void CharacterToolForm::LoadState(const char* filename, bool layoutOnly)
    {
        Serialization::JSONIArchive ia;
        if (layoutOnly)
        {
            ia.SetFilter(SERIALIZE_STATE | SERIALIZE_LAYOUT);
        }
        else
        {
            ia.SetFilter(SERIALIZE_STATE);
        }
        Serialization::SContext<CharacterToolForm> formContext(ia, this);
        if (!ia.load(filename))
        {
            return;
        }
        ia(*this, "state");

        OnViewportOptionsChanged();
        UpdatePanesMenu();
    }

    static string GetLayoutFilename(const char* name)
    {
        string filename = string(GetIEditor()->GetResolvedUserFolder().toUtf8().data());
        filename += "\\CharacterTool\\Layouts\\";
        filename += name;
        filename += ".layout";

        return filename;
    }

    void CharacterToolForm::LoadLayout(const char* name)
    {
        string filename = GetLayoutFilename(name);

        LoadState(filename.c_str(), true);
    }


    void CharacterToolForm::SaveLayout(const char* name)
    {
        string filename = string(GetIEditor()->GetResolvedUserFolder().toUtf8().data());
        filename += "\\CharacterTool\\Layouts\\";
        QDir().mkdir(filename.c_str());

        filename += name;
        filename += ".layout";

        SaveState(filename.c_str(), true);
        UpdateLayoutMenu();
    }

    void CharacterToolForm::OnFileRecentAboutToShow(QMenu* recentMenu)
    {
        recentMenu->clear();

        if (m_recentCharacters.empty())
        {
            recentMenu->addAction("No characters were open recently")->setEnabled(false);
            return;
        }

        for (size_t i = 0; i < m_recentCharacters.size(); ++i)
        {
            const char* fullPath = m_recentCharacters[i].c_str();
            string path;
            string file;
            PathUtil::Split(fullPath, path, file);
            QString text;
            if (i < 10 - 1)
            {
                text.sprintf("&%d. %s\t%s", int(i + 1), file.c_str(), path.c_str());
            }
            else if (i == 10 - 1)
            {
                text.sprintf("1&0. %s\t%s", file.c_str(), path.c_str());
            }
            else
            {
                text.sprintf("%d. %s\t%s", int(i + 1), file.c_str(), path.c_str());
            }
            QAction* action = recentMenu->addAction(text);
            action->setData(QString::fromLocal8Bit(fullPath));
            EXPECTED(connect(action, SIGNAL(triggered()), this, SLOT(OnFileRecent())));
        }
    }

    void CharacterToolForm::RemoveLayout(const char* name)
    {
        string filename = GetLayoutFilename(name);

        QFile::remove(filename.c_str());
        UpdateLayoutMenu();
    }

    void CharacterToolForm::OnLayoutSave()
    {
        QString name = QInputDialog::getText(this, "Save Layout...", "Layout Name:");
        name = name.replace('.', "_");
        name = name.replace(':', "_");
        name = name.replace('\\', "_");
        name = name.replace('/', "_");
        name = name.replace('?', "_");
        name = name.replace('*', "_");
        if (!name.isEmpty())
        {
            SaveLayout(name.toLocal8Bit().data());
        }
        UpdateLayoutMenu();
    }

    CharacterToolForm::~CharacterToolForm()
    {
        delete m_modeToolBar;
        m_modeToolBar = nullptr;
        InstallMode(0, 0);
        m_dockWidgetManager->Clear();
    }


    void CharacterToolForm::ExecuteExplorerAction(const ExplorerAction& _action, const ExplorerEntries& entries)
    {
        ExplorerAction action = _action;
        if (!EXPECTED(action.func))
        {
            return;
        }

        ActionContext x;
        x.window = this;
        x.entries.assign(entries.begin(), entries.end());

        _action.func(x);

        if (!x.isAsync && x.output->HasAnyErrors())
        {
            QMessageBox::critical(this, "Error", x.output->BuildErrorMessage().c_str());
        }
    }

    void CharacterToolForm::closeEvent(QCloseEvent* ev)
    {
        if (m_closed)
        {
            return;
        }

        // Stop all close operations if we are currently saving. We need to wait until saving is complete.
        if (m_isRunningAsyncSave ||
            m_system->animationList->IsRunningAsyncSaveOperation() ||
            m_system->characterList->IsRunningAsyncSaveOperation() ||
            m_system->skeletonList->IsRunningAsyncSaveOperation())
        {
            QMessageBox::information(this, "Geppetto - Saving", "Geppetto is still running save operations. Please wait until complete before closing.", QMessageBox::StandardButton::Ok);
            ev->ignore();
            return;
        }

        // If there is nothing to save, then we're good
        ExplorerEntries unsavedEntries;
        m_system->explorer->GetUnsavedEntries(&unsavedEntries);
    
        if (unsavedEntries.size() == 0)
        {
            m_closed = true;
            return;
        }

        QMessageBox::StandardButton result =
            QMessageBox::question(this,
                "Geppetto - Unsaved changes",
                "Would you like to save all changes before closing?\n\n\nNote: If you close the editor, all your changes will be lost!",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        switch (result)
        {
        case QMessageBox::Cancel:
            ev->ignore();
            break;

        case QMessageBox::Yes:
            QSharedPointer<QProgressDialog> dlg(new QProgressDialog("Geppetto: Saving all unsaved changes...", QString(), 0, 0, nullptr));
            dlg->setWindowModality(Qt::WindowModality::NonModal);
            dlg->setModal(false);
            dlg->show();

            bool hasSaveAllFinished = false;
            bool wasSaveAllSuccessfull = false;
            RunSaveAll(
                [dlg, &hasSaveAllFinished, &wasSaveAllSuccessfull](bool wasSuccessfull)
                {
                    hasSaveAllFinished = true;
                    wasSaveAllSuccessfull = wasSuccessfull;
                    dlg->close();
                    if (wasSuccessfull)
                    {
                        QtViewPaneManager::instance()->ClosePane(LyViewPane::LegacyGeppetto);
                    }
                }
                );

            // At this point, the save could only be finished if if there were errors prior to kicking off ansync source control operations

            // Stop the close operation if:
            //  - The save is processing in the background. In this case, we can't close the window because it is processing the save, and because
            //    if the save fails the user may have some followup to do).
            //  - The save is done, but it failed. This would happen if the failure occurred before entering into async negotiations with source control.
            if (!hasSaveAllFinished || !wasSaveAllSuccessfull)
            {
                ev->ignore();
                break;
            }

            // If we've gotten here then there was no save operation to do, so we're going to continue with the close operation
            m_closed = true;
            break;
        }
    }

    void CharacterToolForm::resizeEvent(QResizeEvent* ev)
    {
        if (!m_stateLoaded)
        {
            LoadState(GetStateFilename(), false);
            m_stateLoaded = true;
        }
    }

    bool CharacterToolForm::eventFilter(QObject* sender, QEvent* ev)
    {
        if (sender == m_splitViewport->CompressedViewport())
        {
            if (ev->type() == QEvent::KeyPress)
            {
                QKeyEvent* press = (QKeyEvent*)(ev);

                static QViewport* fullscreenViewport;
                static QWidget* previousParent;
                if (press->key() == Qt::Key_F11)
                {
                    QViewport* viewport = m_splitViewport->CompressedViewport();
                    if (fullscreenViewport == viewport)
                    {
                        fullscreenViewport = 0;
                        viewport->showFullScreen();
                        m_splitViewport->FixLayout();
                    }
                    else
                    {
                        previousParent = (QWidget*)viewport->parent();
                        fullscreenViewport = viewport;
                        viewport->setParent(0);
                        //      viewport->resize(960, 1080);
                        viewport->resize(1920, 1080);
                        viewport->setWindowFlags(Qt::FramelessWindowHint);
                        viewport->move(0, 0);
                        viewport->show();
                        //viewport->showFullScreen();
                    }
                }
            }
#if 0
            if (ev->type() == QEvent::KeyPress)
            {
                QKeyEvent* press = (QKeyEvent*)(ev);
                if (press->key() == QKeySequence(Qt::Key_F1))
                {
                    static int index;
                    char buf[128] = "";
                    sprintf_s(buf, "Objects/%i/test.chr", index);
                    m_system->document->CharacterList()->AddCharacterEntry(buf);
                    ++index;
                    return true;
                }
            }
#endif
        }
        return false;
    }

    void CharacterToolForm::OnFileRecent()
    {
        if (QAction* action = qobject_cast<QAction*>(sender()))
        {
            QString path = action->data().toString();
            if (!path.isEmpty())
            {
                m_system->scene->characterPath = path.toLocal8Bit().data();
                m_system->scene->SignalCharacterChanged();
            }
        }
    }

    void CharacterToolForm::OnDockWidgetsChanged()
    {
        UpdatePanesMenu();
    }

    void CharacterToolForm::GenerateUnsavedFileTracking(FilenameToEntryMapping& unsavedFileTracking)
    {
        ExplorerEntries unsavedEntries;

        m_system->explorer->GetUnsavedEntries(&unsavedEntries);

        vector<string> unsavedFilenames;
        m_system->explorer->GetSaveFilenames(&unsavedFilenames, unsavedEntries);

        for (size_t i = 0; i < unsavedFilenames.size(); ++i)
        {
            unsavedFileTracking[unsavedFilenames[i]].push_back(unsavedEntries[i]);
        }
    }
}
#include <CharacterTool/CharacterToolForm.moc>
