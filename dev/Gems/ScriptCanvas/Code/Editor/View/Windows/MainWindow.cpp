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

#include <precompiled.h>

#include <Editor/View/Windows/MainWindow.h>

#include <QSplitter>
#include <QListView>
#include <QFileDialog>
#include <QShortcut>
#include <QKeySequence>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneEvent>
#include <QMimeData>
#include <QCoreApplication>
#include <QMessageBox>

#include <Editor/Undo/ScriptCanvasUndoCache.h>
#include <Editor/Undo/ScriptCanvasUndoManager.h>
#include <Editor/View/Dialogs/UnsavedChangesDialog.h>
#include <Editor/View/Dialogs/PreviewMessage.h>
#include <Editor/View/Dialogs/Settings.h>
#include <Editor/View/Widgets/NodeFavorites.h>
#include <Editor/View/Widgets/NodePalette.h>
#include <Editor/View/Widgets/Debugging.h>
#include <Editor/View/Widgets/PropertyGrid.h>
#include <Editor/View/Widgets/NodeOutliner.h>
#include <Editor/View/Widgets/CommandLine.h>
#include <Editor/View/Widgets/GraphTabBar.h>
#include <Editor/View/Widgets/NodeFavorites.h>
#include <Editor/View/Widgets/CanvasWidget.h>
#include <Editor/View/Widgets/LogPanel.h>

#include <Editor/View/Windows/ui_MainWindow.h>

#include <Editor/Model/EntityMimeDataHandler.h>

#include <Editor/Utilities/RecentAssetPath.h>

#include <Editor/Metrics.h>
#include <Editor/Settings.h>
#include <Editor/Nodes/NodeUtils.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasDocumentContext.h>

#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Widgets/CanvasGraphicsView/CanvasGraphicsView.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>

#include "ConnectionContextMenu.h"
#include "NodeContextMenu.h"
#include "SceneContextMenu.h"
#include "SlotContextMenu.h"
#include "CreateNodeContextMenu.h"
#include "EBusHandlerActionMenu.h"
#include "Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h"
#include "Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h"
#include "Editor/View/Windows/MainWindowBus.h"

namespace
{
    size_t s_scriptCanvasEditorDefaultNewNameCount = 0;
} // anonymous namespace.

namespace ScriptCanvasEditor
{
    using namespace AzToolsFramework;

    namespace
    {
        template <typename T>
        class ScopedVariableSetter
        {
        public:
            ScopedVariableSetter(T& value)
                : m_oldValue(value)
                , m_value(value)
            {
            }
            ScopedVariableSetter(T& value, const T& newValue)
                : m_oldValue(value)
                , m_value(value)
            {
                m_value = newValue;
            }

            ~ScopedVariableSetter()
            {
                m_value = m_oldValue;
            }

        private:
            T m_oldValue;
            T& m_value;
        };

        template<typename MimeDataDelegateHandler, typename ... ComponentArgs>
        AZ::EntityId CreateMimeDataDelegate(ComponentArgs... componentArgs)
        {
            AZ::Entity* mimeDelegateEntity = aznew AZ::Entity("MimeData Delegate");

            mimeDelegateEntity->CreateComponent<MimeDataDelegateHandler>(AZStd::forward<ComponentArgs>(componentArgs) ...);
            mimeDelegateEntity->Init();
            mimeDelegateEntity->Activate();

            return mimeDelegateEntity->GetId();
        }

        AZStd::string MakeTemporaryFilePathForSave(const char* targetFilename)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "File IO is not initialized.");

            AZStd::string devAssetPath = fileIO->GetAlias("@devassets@");
            AZStd::string userPath = fileIO->GetAlias("@user@");
            AZStd::string tempPath = targetFilename;
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, devAssetPath);
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, userPath);
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, tempPath);
            AzFramework::StringFunc::Replace(tempPath, "@devassets@", devAssetPath.c_str());
            AzFramework::StringFunc::Replace(tempPath, devAssetPath.c_str(), userPath.c_str());
            tempPath.append(ScriptCanvasAssetHandler::GetFileExtension());
            tempPath.append(".temp");

            return tempPath;
        }

        void CreateSaveDestinationDirectory()
        {
            AZStd::string unresolvedDestination("@devassets@/scriptcanvas");

            AZStd::array<char, AZ::IO::MaxPathLength> assetRoot;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(unresolvedDestination.c_str(), assetRoot.data(), assetRoot.size());

            // We just need the path to exist.
            QDir(assetRoot.data()).mkpath(".");
        }
    } // anonymous namespace.

    MainWindow::AssetGraphSceneData::AssetGraphSceneData(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
        : m_assetHolder(scriptCanvasAsset)
    {
        m_assetHolder.Init();

        m_tupleId.m_assetId = scriptCanvasAsset.GetId();
        AZ::Entity* sceneEntity = scriptCanvasAsset.IsReady() ? scriptCanvasAsset.Get()->GetScriptCanvasEntity() : nullptr;
        if (sceneEntity)
        {
            AZ::EntityId sceneId = sceneEntity->GetId();
            AZ::EntityId graphId;
            ScriptCanvas::SystemRequestBus::BroadcastResult(graphId, &ScriptCanvas::SystemRequests::FindGraphId, sceneEntity);
            m_tupleId.m_graphId = graphId;
            m_tupleId.m_sceneId = sceneId;
        }
    }

    void MainWindow::AssetGraphSceneMapper::Add(const AZ::Data::Asset<ScriptCanvasAsset> &scriptCanvasAsset)
    {
        if (scriptCanvasAsset.GetId().IsValid())
        {
            auto assetIt = m_assetIdToDataMap.find(scriptCanvasAsset.GetId());
            if (assetIt != m_assetIdToDataMap.end())
            {
                AssetGraphSceneData* assetGraphSceneData = assetIt->second.get();
                AZ::Entity* sceneEntity = scriptCanvasAsset.IsReady() ? scriptCanvasAsset.Get()->GetScriptCanvasEntity() : nullptr;
                AZ::EntityId sceneId = sceneEntity ? sceneEntity->GetId() : AZ::EntityId();
                AZ::EntityId graphId;
                ScriptCanvas::SystemRequestBus::BroadcastResult(graphId, &ScriptCanvas::SystemRequests::FindGraphId, sceneEntity);
                assetGraphSceneData->m_tupleId.m_graphId = graphId;
                assetGraphSceneData->m_tupleId.m_sceneId = sceneId;
            }
            else
            {
                AssetGraphSceneData* assetGraphSceneData = new AssetGraphSceneData(scriptCanvasAsset);
                m_assetIdToDataMap.emplace(scriptCanvasAsset.GetId(), assetGraphSceneData);
            }
        }
    }

    void MainWindow::AssetGraphSceneMapper::Add(AZStd::unique_ptr<AssetGraphSceneData> assetGraphSceneData)
    {
        if (assetGraphSceneData)
        {
            const AZ::Data::AssetId& assetId = assetGraphSceneData->m_tupleId.m_assetId;
            if (assetId.IsValid())
            {
                auto assetIt = m_assetIdToDataMap.find(assetId);
                if (assetIt == m_assetIdToDataMap.end())
                {
                    m_assetIdToDataMap.emplace(assetId, AZStd::move(assetGraphSceneData));
                }
                else
                {
                    m_assetIdToDataMap[assetId] = AZStd::move(assetGraphSceneData);
                }
            }
        }
    }

    void MainWindow::AssetGraphSceneMapper::Remove(const AZ::Data::AssetId& assetId)
    {
        m_assetIdToDataMap.erase(assetId);
    }

    void MainWindow::AssetGraphSceneMapper::Clear()
    {
        m_assetIdToDataMap.clear();
    }

    MainWindow::AssetGraphSceneData* MainWindow::AssetGraphSceneMapper::GetByAssetId(const AZ::Data::AssetId& assetId) const
    {
        if (!assetId.IsValid())
        {
            return nullptr;
        }

        auto assetToGraphIt = m_assetIdToDataMap.find(assetId);
        if (assetToGraphIt != m_assetIdToDataMap.end())
        {
            return assetToGraphIt->second.get();
        }

        return nullptr;
    }

    MainWindow::AssetGraphSceneData* MainWindow::AssetGraphSceneMapper::GetByGraphId(AZ::EntityId graphId) const
    {
        if (!graphId.IsValid())
        {
            return nullptr;
        }

        auto graphToAssetIt = AZStd::find_if(m_assetIdToDataMap.begin(), m_assetIdToDataMap.end(), [&graphId](const AZStd::pair<AZ::Data::AssetId, AZStd::unique_ptr<AssetGraphSceneData>>& assetPair)
        {
            return assetPair.second->m_tupleId.m_graphId == graphId;
        });

        if (graphToAssetIt != m_assetIdToDataMap.end())
        {
            return graphToAssetIt->second.get();
        }

        return nullptr;
    }

    MainWindow::AssetGraphSceneData* MainWindow::AssetGraphSceneMapper::GetBySceneId(AZ::EntityId sceneId) const
    {
        AssetGraphSceneId tupleId;
        if (!sceneId.IsValid())
        {
            return nullptr;
        }

        auto sceneToAssetIt = AZStd::find_if(m_assetIdToDataMap.begin(), m_assetIdToDataMap.end(), [sceneId](const AZStd::pair<AZ::Data::AssetId, AZStd::unique_ptr<AssetGraphSceneData>>& assetPair)
        {
            return assetPair.second->m_tupleId.m_sceneId == sceneId;
        });

        if (sceneToAssetIt != m_assetIdToDataMap.end())
        {
            return sceneToAssetIt->second.get();
        }
        return nullptr;
    }

    /////////////////
    // Event Filter
    /////////////////

    class EmptyCanvasDropFilter
        : public QObject
    {
    public:

        EmptyCanvasDropFilter(MainWindow* window)
            : m_window(window)
        {
        }

        bool eventFilter(QObject *object, QEvent *event)
        {
            switch (event->type())
            {
            case QEvent::DragEnter:
                m_window->OnEmptyCanvasDragEnter(static_cast<QDragEnterEvent*>(event));
                m_enterState = event->isAccepted();
                return m_enterState;
                break;
            case QEvent::DragMove:
                if (m_enterState)
                {
                    event->accept();
                }
                return m_enterState;
                break;
            case QEvent::DragLeave:
                m_enterState = false;
                break;
            case QEvent::Drop:
                if (m_enterState)
                {
                    m_window->OnEmptyCanvasDrop(static_cast<QDropEvent*>(event));
                    return m_enterState;
                }
                break;
            default:
                break;
            }

            return false;
        }

    private:
        MainWindow* m_window;

        bool m_enterState;
    };

    ////////////////
    // MainWindow
    ////////////////

    MainWindow::MainWindow()
        : QMainWindow(nullptr, Qt::Widget | Qt::WindowMinMaxButtonsHint)
        , ui(new Ui::MainWindow)
        , m_enterState(false)
        , m_ignoreSelection(false)
        , m_preventUndoStateUpdateCount(0)
    {
        AZStd::array<char, AZ::IO::MaxPathLength> unresolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@assets@/editor/translation/scriptcanvas_en_us.qm", unresolvedPath.data(), unresolvedPath.size());

        QString translationFilePath(unresolvedPath.data());
        if ( m_translator.load(QLocale::Language::English, translationFilePath) )
        {
            if ( !qApp->installTranslator(&m_translator) )
            {
                AZ_Warning("ScriptCanvas", false, "Error installing translation %s!", unresolvedPath.data());
            }
        }
        else
        {
            AZ_Warning("ScriptCanvas", false, "Error loading translation file %s", unresolvedPath.data());
        }

        ui->setupUi(this);

        CreateMenus();
        UpdateRecentMenu();

        m_host = new QWidget();

        m_layout = new QVBoxLayout();

        m_emptyCanvas = new QLabel(tr("Use the File Menu or drag out a node from the Node Palette to create a new script."), nullptr);
        m_emptyCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_emptyCanvas->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
        m_emptyCanvas->setBaseSize(size());
        m_emptyCanvas->setAcceptDrops(true);
        m_emptyCanvas->installEventFilter(new EmptyCanvasDropFilter(this));
        m_emptyCanvas->setObjectName("EmptyCanvas");

        // Tab bar and "+" button.
        {
            // This spacer keeps the "+" button right-aligned.
            m_plusButtonSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            m_horizontalTabBarLayout = new QHBoxLayout();

            m_tabBar = new Widget::GraphTabBar(m_host);
            m_tabBar->hide();
            connect(m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::OnTabCloseButtonPressed);
            connect(m_tabBar, &Widget::GraphTabBar::TabCloseNoButton, this, &MainWindow::OnTabCloseRequest);
            connect(m_tabBar, &Widget::GraphTabBar::TabInserted, this, &MainWindow::OnTabInserted);
            connect(m_tabBar, &Widget::GraphTabBar::TabRemoved, this, &MainWindow::OnTabRemoved);

            // We never use the m_tabBar and the m_plusButtonSpacer simultaneously.
            // We use the m_plusButtonSpacer to keep the "+" button right-aligned
            // when the m_tabBar is hidden.
            //
            // Both are added to the m_horizontalTabBarLayout because the initial
            // state (no tabs) requires the m_plusButtonSpacer, and m_tabBar is
            // hidden.
            m_horizontalTabBarLayout->addWidget(m_tabBar);
            m_horizontalTabBarLayout->addSpacerItem(m_plusButtonSpacer);

            QPushButton* plusButton = new QPushButton(tr("+"), this);
            plusButton->setFixedSize(20, 20);
            QObject::connect(plusButton, &QPushButton::clicked, this, &MainWindow::OnFileNew);
            m_horizontalTabBarLayout->addWidget(plusButton);
            m_layout->addLayout(m_horizontalTabBarLayout);
        }

        m_commandLine = new Widget::CommandLine(this);
        m_commandLine->setBaseSize(QSize(size().width(), m_commandLine->size().height()));
        m_commandLine->setObjectName("CommandLine");

        m_layout->addWidget(m_commandLine);
        m_layout->addWidget(m_emptyCanvas);

        m_nodeOutliner = new Widget::NodeOutliner(this);
        m_nodeOutliner->setObjectName("NodeOutliner");
        m_nodeOutliner->setMinimumHeight(40);

        m_propertyGrid = new Widget::PropertyGrid(this, "Node Inspector");
        m_propertyGrid->setObjectName("NodeInspector");

        // Disabled until debugger is implemented
        //m_debugging = new Widget::Debugging(this);
        //m_debugging->setObjectName("Debugging");
        //m_logPanel = new Widget::LogPanelWidget(this);
        //m_logPanel->setObjectName("LogPanel");

        m_nodePalette = new Widget::NodePalette(tr("Node Palette"), this);
        m_nodePalette->setObjectName("NodePalette");

        m_createNodeContextMenu = aznew CreateNodeContextMenu();
        m_ebusHandlerActionMenu = aznew EBusHandlerActionMenu();

        m_host->setLayout(m_layout);

        setCentralWidget(m_host);

        QTimer::singleShot(0, [this]() {
            SetDefaultLayout();

            RestoreWindowState();
            SaveWindowState();
        });

        m_entityMimeDelegateId = CreateMimeDataDelegate<ScriptCanvasEditor::EntityMimeDataHandler>();

        ScriptCanvasEditor::GeneralRequestBus::Handler::BusConnect();
        UIRequestBus::Handler::BusConnect();
        UndoNotificationBus::Handler::BusConnect();

        CreateUndoManager();

        UINotificationBus::Broadcast(&UINotifications::MainWindowCreationEvent, this);
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendEditorMetric, ScriptCanvasEditor::Metrics::Events::Editor::Open, m_activeAssetId);


        // Show the PREVIEW welcome message
        AZStd::intrusive_ptr<EditorSettings::PreviewSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::PreviewSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings && settings->m_showPreviewMessage)
        {
            QTimer::singleShot(1.f, this, [this]()
            {
                PreviewMessageDialog* previewMessage = aznew PreviewMessageDialog(this);

                QPoint centerPoint = frameGeometry().center();

                previewMessage->adjustSize();
                previewMessage->move(centerPoint.x() - previewMessage->width() / 2, centerPoint.y() - previewMessage->height() / 2);

                previewMessage->Show();
            });
        }
    }

    MainWindow::~MainWindow()
    {
        SaveWindowState();

        UndoNotificationBus::Handler::BusDisconnect();
        UIRequestBus::Handler::BusDisconnect();
        ScriptCanvasEditor::GeneralRequestBus::Handler::BusDisconnect();
        Clear();

        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendMetric, ScriptCanvasEditor::Metrics::Events::Editor::Close);
    }

    void MainWindow::CreateMenus()
    {
        // File menu        
        connect(ui->action_New_Script, &QAction::triggered, this, &MainWindow::OnFileNew);
        ui->action_New_Script->setShortcut(QKeySequence(QKeySequence::New));

        connect(ui->action_Open, &QAction::triggered, this, &MainWindow::OnFileOpen);
        ui->action_Open->setShortcut(QKeySequence(QKeySequence::Open));

        // List of recent files.
        {
            QMenu* recentMenu = new QMenu("Open &Recent");

            for (int i = 0; i < m_recentActions.size(); ++i)
            {
                QAction* action = new QAction(this);
                action->setVisible(false);
                m_recentActions[i] = AZStd::make_pair(action, QMetaObject::Connection());
                recentMenu->addAction(action);
            }

            recentMenu->addSeparator();

            // Clear Recent Files.
            {
                QAction* action = new QAction("&Clear Recent Files", this);

                QObject::connect(action,
                    &QAction::triggered,
                    [this](bool /*checked*/)
                {
                    ClearRecentFile();
                    UpdateRecentMenu();
                });

                recentMenu->addAction(action);
            }

            ui->menuFile->insertMenu(ui->action_Save, recentMenu);
            ui->menuFile->insertSeparator(ui->action_Save);
        }

        connect(ui->action_Save, &QAction::triggered, this, &MainWindow::OnFileSaveCaller);
        ui->action_Save->setShortcut(QKeySequence(QKeySequence::Save));

        connect(ui->action_Save_As, &QAction::triggered, this, &MainWindow::OnFileSaveAsCaller);
        ui->action_Save_As->setShortcut(QKeySequence(tr("Ctrl+Shift+S", "File|Save As...")));

        QObject::connect(ui->action_Close,
            &QAction::triggered,
            [this](bool /*checked*/)
        {
            m_tabBar->tabCloseRequested(m_tabBar->currentIndex());
        });
        ui->action_Close->setShortcut(QKeySequence(QKeySequence::Close));

        // Edit Menu
        SetupEditMenu();

        // View menu
        connect(ui->action_ViewNodePalette, &QAction::triggered, this, &MainWindow::OnViewNodePalette);
        connect(ui->action_ViewOutline, &QAction::triggered, this, &MainWindow::OnViewOutline);
        connect(ui->action_ViewProperties, &QAction::triggered, this, &MainWindow::OnViewProperties);
        connect(ui->action_ViewDebugger, &QAction::triggered, this, &MainWindow::OnViewDebugger);
        connect(ui->action_ViewCommandLine, &QAction::triggered, this, &MainWindow::OnViewCommandLine);
        connect(ui->action_ViewLog, &QAction::triggered, this, &MainWindow::OnViewLog);
        connect(ui->action_ViewRestoreDefaultLayout, &QAction::triggered, this, &MainWindow::OnRestoreDefaultLayout);
    }

    void MainWindow::SignalActiveSceneChanged(const AZ::EntityId& sceneId)
    {
        // The paste action refreshes based on the scene's mimetype
        RefreshPasteAction();

        MainWindowNotificationBus::Broadcast(&MainWindowNotifications::PreOnActiveSceneChanged);
        MainWindowNotificationBus::Broadcast(&MainWindowNotifications::OnActiveSceneChanged, sceneId);
        MainWindowNotificationBus::Broadcast(&MainWindowNotifications::PostOnActiveSceneChanged);
    }

    void MainWindow::UpdateRecentMenu()
    {
        QStringList recentFiles = ReadRecentFiles();

        int recentCount = 0;
        for (auto filename : recentFiles)
        {
            auto& recent = m_recentActions[recentCount++];

            recent.first->setText(QString("&%1 %2").arg(QString::number(recentCount), filename));
            recent.first->setData(filename);
            recent.first->setVisible(true);

            QObject::disconnect(recent.second);
            recent.second = QObject::connect(recent.first,
                &QAction::triggered,
                [this, filename](bool /*checked*/)
            {
                OpenFile(filename.toUtf8().data());
            });
        }

        for (int i = recentCount; i < m_recentActions.size(); ++i)
        {
            auto& recent = m_recentActions[recentCount++];
            recent.first->setVisible(false);
        }
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        SaveWindowState();

        // Close all unmodified files.
        for(auto assetIdDataPair = m_assetGraphSceneMapper.m_assetIdToDataMap.begin();
            assetIdDataPair != m_assetGraphSceneMapper.m_assetIdToDataMap.end();)
        {
            const AZ::Data::AssetId& assetId = assetIdDataPair->first;

            // We HAVE to do this here because CloseScriptCanvasAsset()
            // will invalidate our iterator.
            ++assetIdDataPair;

            // Get the state of the file.
            ScriptCanvasFileState fileState;
            DocumentContextRequestBus::BroadcastResult(fileState, &DocumentContextRequests::GetScriptCanvasAssetModificationState, assetId);
            if (fileState == ScriptCanvasFileState::UNMODIFIED)
            {
                CloseScriptCanvasAsset(assetId);
            }
        }

        // Get all the unsaved assets.
        // TOFIX LY-62249.
        for(auto assetIdDataPair = m_assetGraphSceneMapper.m_assetIdToDataMap.begin();
            assetIdDataPair != m_assetGraphSceneMapper.m_assetIdToDataMap.end();)
        {
            const AZ::Data::AssetId& assetId = assetIdDataPair->first;

            // We HAVE to do this here because CloseScriptCanvasAsset()
            // will invalidate our iterator.
            ++assetIdDataPair;

            // Get the state of the file.
            ScriptCanvasFileState fileState;
            DocumentContextRequestBus::BroadcastResult(fileState, &DocumentContextRequests::GetScriptCanvasAssetModificationState, assetId);
            AZ_Assert((fileState == ScriptCanvasFileState::NEW || fileState == ScriptCanvasFileState::MODIFIED),
                        "All UNMODIFIED files should have been CLOSED by now.");

            // Query the user.
            SetActiveAsset(assetId);
            QString tabName;
            QVariant data = GetTabData(assetId);
            if (data.isValid())
            {
                auto graphTabMetadata = data.value<Widget::GraphTabMetadata>();
                tabName = graphTabMetadata.m_tabName;
            }
            UnsavedChangesOptions shouldSaveResults = ShowSaveDialog(tabName);

            if (shouldSaveResults == UnsavedChangesOptions::SAVE)
            {
                SaveAsset(assetId,
                            DocumentContextRequests::SaveCB([this](const AZ::Data::AssetId& assetId, bool isSuccessful)
                            {
                                if (isSuccessful)
                                {
                                    int tabIndex = -1;
                                    if (IsTabOpen(assetId, tabIndex))
                                    {
                                        OnTabCloseRequest(tabIndex);
                                    }

                                    // Continue closing.
                                    qobject_cast<QWidget*>(parent())->close();
                                }
                                else
                                {
                                    // Abort closing.
                                    QMessageBox::critical(this, QString(), QObject::tr("Failed to save."));
                                }
                            }));
                event->ignore();
                return;
            }
            else if (shouldSaveResults == UnsavedChangesOptions::CANCEL_WITHOUT_SAVING)
            {
                event->ignore();
                return;
            }
            else if (shouldSaveResults == UnsavedChangesOptions::CONTINUE_WITHOUT_SAVING)
            {
                CloseScriptCanvasAsset(assetId);
            }
        }

        event->accept();
    }

    UnsavedChangesOptions MainWindow::ShowSaveDialog(const QString& filename)
    {
        UnsavedChangesOptions shouldSaveResults = UnsavedChangesOptions::INVALID;
        UnsavedChangesDialog dialog(filename, this);
        dialog.exec();
        shouldSaveResults = dialog.GetResult();

        return shouldSaveResults;
    }

    bool MainWindow::SaveAsset(const AZ::Data::AssetId& unsavedAssetId, const DocumentContextRequests::SaveCB& saveCB)
    {
        SetActiveAsset(unsavedAssetId);
        return OnFileSave(saveCB);
    }

    void MainWindow::CreateUndoManager()
    {
        m_undoManager = AZStd::make_unique<UndoManager>();
    }

    void MainWindow::TriggerUndo()
    {
        m_undoManager->Undo();
        SignalSceneDirty(GetActiveSceneId());
    }

    void MainWindow::TriggerRedo()
    {
        m_undoManager->Redo();
        SignalSceneDirty(GetActiveSceneId());
    }

    void MainWindow::PostUndoPoint(AZ::EntityId sceneId)
    {
        if (m_preventUndoStateUpdateCount == 0 && !m_undoManager->IsInUndoRedo())
        {
            ScopedUndoBatch scopedUndoBatch("Modify Graph Canvas Scene");
            AZ::Entity* scriptCanvasEntity{};
            AZ::ComponentApplicationBus::BroadcastResult(scriptCanvasEntity, &AZ::ComponentApplicationRequests::FindEntity, sceneId);
            m_undoManager->AddGraphItemChangeUndo(scriptCanvasEntity, "Graph Change");

            SignalSceneDirty(sceneId);
        }
    }

    void MainWindow::SignalSceneDirty(const AZ::EntityId& sceneId)
    {
        auto assetGraphSceneData = m_assetGraphSceneMapper.GetBySceneId(sceneId);
        if (assetGraphSceneData)
        {
            MarkAssetModified(assetGraphSceneData->m_tupleId.m_assetId);
        }
    }

    void MainWindow::PushPreventUndoStateUpdate()
    {
        ++m_preventUndoStateUpdateCount;
    }

    void MainWindow::PopPreventUndoStateUpdate()
    {
        if (m_preventUndoStateUpdateCount > 0)
        {
            --m_preventUndoStateUpdateCount;
        }
    }

    void MainWindow::ClearPreventUndoStateUpdate()
    {
        m_preventUndoStateUpdateCount = 0;
    }

    void MainWindow::MarkAssetModified(const AZ::Data::AssetId& assetId)
    {
        ScriptCanvasFileState fileState;
        DocumentContextRequestBus::BroadcastResult(fileState, &DocumentContextRequests::GetScriptCanvasAssetModificationState, assetId);
        if (fileState != ScriptCanvasFileState::NEW && fileState != ScriptCanvasFileState::MODIFIED)
        {
            DocumentContextRequestBus::Broadcast(&DocumentContextRequests::SetScriptCanvasAssetModificationState, assetId, ScriptCanvasFileState::MODIFIED);
        }
    }

    void MainWindow::MarkAssetUnmodified(const AZ::Data::AssetId& assetId)
    {
        ScriptCanvasFileState fileState;
        DocumentContextRequestBus::BroadcastResult(fileState, &DocumentContextRequests::GetScriptCanvasAssetModificationState, assetId);
        if (fileState != ScriptCanvasFileState::UNMODIFIED)
        {
            DocumentContextRequestBus::Broadcast(&DocumentContextRequests::SetScriptCanvasAssetModificationState, assetId, ScriptCanvasFileState::UNMODIFIED);
        }
    }

    void MainWindow::CreateScriptCanvasAssetHolder(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset, int tabIndex)
    {
        AssetGraphSceneData* assetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(scriptCanvasAsset.GetId());
        if (assetGraphSceneData)
        {
            assetGraphSceneData->m_assetHolder.SetAsset(scriptCanvasAsset);
            assetGraphSceneData->m_assetHolder.ActivateAssetData();
        }
        else
        {
            m_assetGraphSceneMapper.Add(AZStd::make_unique<AssetGraphSceneData>(scriptCanvasAsset));
            auto newAssetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(scriptCanvasAsset.GetId());

            AZStd::string tabName;
            if (scriptCanvasAsset.IsReady())
            {
                AzFramework::StringFunc::Path::GetFileName(scriptCanvasAsset.Get()->GetPath().data(), tabName);
            }

            MainWindowNotificationBus::Broadcast(&MainWindowNotifications::OnSceneLoaded, newAssetGraphSceneData->m_tupleId.m_sceneId);

            m_tabBar->InsertGraphTab(tabIndex, newAssetGraphSceneData->m_tupleId.m_sceneId, scriptCanvasAsset.GetId(),
                newAssetGraphSceneData->m_tupleId.m_graphId, tabName);
            EditorScriptCanvasAssetNotificationBus::MultiHandler::BusConnect(scriptCanvasAsset.GetId());
        }
    }

    void MainWindow::RefreshScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        // Update AssetMapper with new Scene and GraphId 
        auto assetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(scriptCanvasAsset.GetId());
        if (!assetGraphSceneData || !scriptCanvasAsset.IsReady())
        {
            return;
        }

        GraphCanvas::SceneNotificationBus::MultiHandler::BusDisconnect(assetGraphSceneData->m_tupleId.m_sceneId);
        GraphCanvas::SceneUIRequestBus::MultiHandler::BusDisconnect(assetGraphSceneData->m_tupleId.m_sceneId);

        AZ::Entity* sceneEntity = scriptCanvasAsset.Get()->GetScriptCanvasEntity();
        AZ::EntityId sceneId = sceneEntity ? sceneEntity->GetId() : AZ::EntityId();

        MainWindowNotificationBus::Broadcast(&MainWindowNotifications::OnSceneRefreshed, assetGraphSceneData->m_tupleId.m_sceneId, sceneId);

        AZ::EntityId graphId;
        ScriptCanvas::SystemRequestBus::BroadcastResult(graphId, &ScriptCanvas::SystemRequests::FindGraphId, sceneEntity);
        assetGraphSceneData->m_tupleId.m_graphId = graphId;
        assetGraphSceneData->m_tupleId.m_sceneId = sceneId;        

        AZStd::string tabName;
        AzFramework::StringFunc::Path::GetFileName(scriptCanvasAsset.Get()->GetPath().data(), tabName);
        m_tabBar->SetGraphTabData(scriptCanvasAsset.GetId(), sceneId, graphId, tabName);

        if (sceneId.IsValid())
        {
            GraphCanvas::SceneNotificationBus::MultiHandler::BusConnect(sceneId);
            GraphCanvas::SceneUIRequestBus::MultiHandler::BusConnect(sceneId);
            GraphCanvas::SceneMimeDelegateRequestBus::Event(sceneId, &GraphCanvas::SceneMimeDelegateRequests::AddDelegate, m_entityMimeDelegateId);

            LoadStyleSheet(sceneId);

            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::SetMimeType, Widget::NodePalette::GetMimeType());
            GraphCanvas::SceneMemberNotificationBus::Event(sceneId, &GraphCanvas::SceneMemberNotifications::OnSceneReady);
        }
    }

    int MainWindow::OpenScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset, int tabIndex)
    {
        if (0 != m_assetGraphSceneMapper.m_assetIdToDataMap.count(scriptCanvasAsset.GetId()))
        {
            return UpdateScriptCanvasAsset(scriptCanvasAsset);
        }

        CreateScriptCanvasAssetHolder(scriptCanvasAsset, tabIndex);

        int outTabIndex = -1;
        IsTabOpen(scriptCanvasAsset.GetId(), outTabIndex);

        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendGraphMetric, ScriptCanvasEditor::Metrics::Events::Canvas::OpenGraph, scriptCanvasAsset.GetId());
        
        return outTabIndex;
    }

    int MainWindow::UpdateScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        int outTabIndex = -1;

        RefreshScriptCanvasAsset(scriptCanvasAsset);
        if (IsTabOpen(scriptCanvasAsset.GetId(), outTabIndex))
        {
            RefreshActiveAsset();
        }

        return outTabIndex;
    }

    void MainWindow::RemoveScriptCanvasAsset(const AZ::Data::AssetId& assetId)
    {
        EditorScriptCanvasAssetNotificationBus::MultiHandler::BusDisconnect(assetId);
        auto assetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(assetId);
        if (assetGraphSceneData)
        {

            auto& tupleId = assetGraphSceneData->m_tupleId;
            GraphCanvas::SceneNotificationBus::MultiHandler::BusDisconnect(tupleId.m_sceneId);

            m_assetGraphSceneMapper.Remove(assetId);
            MainWindowNotificationBus::Broadcast(&MainWindowNotifications::OnSceneUnloaded, tupleId.m_sceneId);
        }
    }

    int MainWindow::CloseScriptCanvasAsset(const AZ::Data::AssetId& assetId)
    {
        int tabIndex = -1;
        if (IsTabOpen(assetId, tabIndex))
        {
            OnTabCloseRequest(tabIndex);
            Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendGraphMetric, ScriptCanvasEditor::Metrics::Events::Canvas::CloseGraph, assetId);
        }

        return tabIndex;
    }

    void MainWindow::LoadStyleSheet(const AZ::EntityId& sceneId)
    {
        QFile resource(":/ScriptCanvasEditorResources/graphcanvas_style.json");
        Q_ASSERT(resource.exists());
        if (resource.exists())
        {
            resource.open(QIODevice::ReadOnly);
            rapidjson::Document style;
            style.Parse(resource.readAll().data());

            if (style.HasParseError())
            {
                rapidjson::ParseErrorCode errCode = style.GetParseError();
                QString errMessage = QString("Parse Error: %1 at offset: %2").arg(errCode).arg(style.GetErrorOffset());
                AZ_Warning("GraphCanvas", false, errMessage.toUtf8().data());
            }
            else
            {
                GraphCanvas::SceneRequestBus::Event(sceneId,
                    static_cast<void (GraphCanvas::SceneRequests::*)(const rapidjson::Document&)>(&GraphCanvas::SceneRequests::SetStyleSheet), style);
            }
        }
    }

    //! Hook for receiving context menu events for each QGraphicsScene
    void MainWindow::OnSceneContextMenuEvent(const AZ::EntityId& sceneId, QGraphicsSceneContextMenuEvent* event)
    {
        auto graphId = GetGraphId(sceneId);
        if (!graphId.IsValid())
        {
            // Nothing to do.
            return;
        }

        m_createNodeContextMenu->RefreshActions(sceneId);
        QAction* action = m_createNodeContextMenu->exec(QPoint(event->screenPos().x(), event->screenPos().y()));

        if (action == nullptr)
        {
            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_createNodeContextMenu->GetNodePalette()->GetContextMenuEvent();

            bool isValid = false;
            NodeIdPair finalNode = ProcessCreateNodeMimeEvent(mimeEvent, sceneId, AZ::Vector2(event->scenePos().x(), event->scenePos().y()));

            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::ClearSelection);

            if (finalNode.m_graphCanvasId.IsValid())
            {
                event->accept();
                GraphCanvas::SceneNotificationBus::Event(sceneId, &GraphCanvas::SceneNotifications::PostCreationEvent);
            }
        }
        else
        {
            CreateNodeAction* nodeAction = qobject_cast<CreateNodeAction*>(action);

            if (nodeAction)
            {
                PushPreventUndoStateUpdate();
                AZ::Vector2 mousePoint(event->scenePos().x(), event->scenePos().y());
                bool triggerUndo = nodeAction->TriggerAction(sceneId, mousePoint);
                PopPreventUndoStateUpdate();

                if (triggerUndo)
                {
                    PostUndoPoint(sceneId);
                }
            }
        }
    }

    void MainWindow::OnSceneDoubleClickEvent(const AZ::EntityId& sceneId, QGraphicsSceneMouseEvent* event)
    {
    }

    //! Hook for receiving context menu events for each QGraphicsScene
    void MainWindow::OnNodeContextMenuEvent(const AZ::EntityId& nodeId, QGraphicsSceneContextMenuEvent* event)
    {
        NodeContextMenu contextMenu(event->scenePos(), nodeId);
        if (!contextMenu.actions().empty())
        {
            contextMenu.exec(event->screenPos());
            event->accept();
        }
    }

    void MainWindow::OnSlotContextMenuEvent(const AZ::EntityId& slotId, QGraphicsSceneContextMenuEvent* event)
    {
        SlotContextMenu contextMenu(slotId);
        if (!contextMenu.actions().empty())
        {
            contextMenu.exec(event->screenPos());
            event->accept();
        }
    }

    void MainWindow::OnConnectionContextMenuEvent(const AZ::EntityId& connectionId, QGraphicsSceneContextMenuEvent* event)
    {
        ConnectionContextMenu contextMenu(connectionId);
        if (!contextMenu.actions().empty())
        {
            contextMenu.exec(event->screenPos());
            event->accept();
        }
    }


    AZStd::string MainWindow::GetSuggestedFullFilenameToSaveAs(const AZ::Data::AssetId& assetId)
    {
        AZStd::array<char, AZ::IO::MaxPathLength> assetRoot;
        AZStd::string suggestedFilePath("@devassets@/");

        auto scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset<ScriptCanvasAsset>(assetId);
        AZStd::string assetPath = scriptCanvasAsset.IsReady() ? scriptCanvasAsset.Get()->GetPath() : AZStd::string();

        if (assetPath.empty())
        {
            int tabIndex = -1;
            if (IsTabOpen(assetId, tabIndex))
            {
                QVariant data = m_tabBar->tabData(tabIndex);
                if (data.isValid())
                {
                    auto graphTabMetadata = data.value<Widget::GraphTabMetadata>();
                    suggestedFilePath = AZStd::string::format("scriptcanvas/%s.%s", graphTabMetadata.m_tabName.toUtf8().data(), ScriptCanvasAssetHandler::GetFileExtension());
                }
            }
        }
        else
        {
            suggestedFilePath += assetPath;
        }

        AZ::IO::FileIOBase::GetInstance()->ResolvePath(suggestedFilePath.data(), assetRoot.data(), assetRoot.size());
        return assetRoot.data();
    }

    void MainWindow::SaveScriptCanvasAsset(const char* filename, AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset, const DocumentContextRequests::SaveCB& saveCB)
    {
        if (!scriptCanvasAsset.IsReady())
        {
            return;
        }

        auto saveAsset = scriptCanvasAsset;
        AZ::Data::AssetInfo suppliedAssetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(suppliedAssetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, scriptCanvasAsset.GetId());

        AZ::Data::AssetType assetType = ScriptCanvasAssetHandler::GetAssetTypeStatic();
        AZ::Data::AssetId catalogAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(catalogAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, filename, assetType, false);

        AZ::Data::AssetInfo diskAssetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(diskAssetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, catalogAssetId);

        // Generate a new Asset id if the current asset has a valid id and the filename does not refer to an existing asset
        // This is the case when the existing asset on disk is being saved to a new file
        if (suppliedAssetInfo.m_assetId != diskAssetInfo.m_assetId)
        {
            // This will either create an asset with the specific Id if it is not loaded or return the asset if it is loaded.
            // What is important here is that a valid Asset with the supplied Id is returned as the data will be overridden
            DocumentContextRequestBus::BroadcastResult(saveAsset, &DocumentContextRequests::CreateScriptCanvasAsset,
                diskAssetInfo.m_assetId.IsValid() ? diskAssetInfo.m_assetId : AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        }

        CopyAssetForSave(saveAsset, scriptCanvasAsset, filename, saveCB);
    }

    void MainWindow::OpenFile(const char* fullPath)
    {
        AZStd::string relativePath;
        bool foundPath = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundPath, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, fullPath, relativePath);
        if (foundPath)
        {
            AZ::Data::AssetType assetType = ScriptCanvasAssetHandler::GetAssetTypeStatic();
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, relativePath.data(), assetType, false);
            if (assetId.IsValid() && m_assetGraphSceneMapper.m_assetIdToDataMap.count(assetId) == 0)
            {
                SetRecentAssetId(assetId);

                OpenScriptCanvasAsset({ assetId, assetType });
                AddRecentFile(fullPath);
                UpdateRecentMenu();
            }
        }
    }

    AZ::Data::AssetInfo MainWindow::CopyAssetForSave(AZ::Data::Asset<ScriptCanvasAsset> newAsset, AZ::Data::Asset<ScriptCanvasAsset> oldAsset, const char* filename, const DocumentContextRequests::SaveCB& saveCB)
    {
        if (newAsset != oldAsset)
        {
            auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
            serializeContext->CloneObjectInplace(newAsset.Get()->GetScriptCanvasData(), &oldAsset.Get()->GetScriptCanvasData());
            AZStd::unordered_map<AZ::EntityId, AZ::EntityId> entityIdMap;
            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&newAsset.Get()->GetScriptCanvasData(), entityIdMap, serializeContext);
        }

        newAsset.Get()->SetPath(filename);
        AZ::Data::AssetInfo diskAssetInfo;
        diskAssetInfo.m_assetId = newAsset.GetId();
        diskAssetInfo.m_assetType = ScriptCanvasAssetHandler::GetAssetTypeStatic();
        diskAssetInfo.m_relativePath = filename;
        DocumentContextRequestBus::Broadcast(&DocumentContextRequests::SaveScriptCanvasAsset, diskAssetInfo, newAsset, saveCB);

        if (newAsset != oldAsset)
        {
            // If the newAsset is already open in the ScriptCanvas Editor, then close the asset being saved,
            // And refresh the newAsset by closing and reopening it at the same tab index
            // otherwise close the asset being saved and open up the new Asset at the old tab index
            int newTabIndex = -1;
            if (IsTabOpen(newAsset.GetId(), newTabIndex))
            {
                CloseScriptCanvasAsset(oldAsset.GetId());
                OpenScriptCanvasAsset(newAsset, CloseScriptCanvasAsset(newAsset.GetId()));
            }
            else
            {
                OpenScriptCanvasAsset(newAsset, CloseScriptCanvasAsset(oldAsset.GetId()));
            }
        }

        return diskAssetInfo;
    }

    void MainWindow::OnFileNew()
    {
        AZStd::string newAssetName = AZStd::string::format("Untitled-%i", ++s_scriptCanvasEditorDefaultNewNameCount);

        AZ::Data::Asset<ScriptCanvasAsset> newAsset;
        DocumentContextRequestBus::BroadcastResult(newAsset, &DocumentContextRequests::CreateScriptCanvasAsset, AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        newAsset.Get()->SetPath(AZStd::string::format("scriptcanvas/%s.%s", newAssetName.data(), ScriptCanvasAssetHandler::GetFileExtension()));

        OpenScriptCanvasAsset(newAsset);
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendMetric, ScriptCanvasEditor::Metrics::Events::Editor::NewFile);
    }

    bool MainWindow::OnFileSave(const DocumentContextRequests::SaveCB& saveCB)
    {
        bool saveAttempt = false;
        if (m_activeAssetId.IsValid())
        {
            auto& assetId = m_activeAssetId;
            ScriptCanvasFileState fileState;
            DocumentContextRequestBus::BroadcastResult(fileState, &DocumentContextRequests::GetScriptCanvasAssetModificationState, assetId);
            if (fileState == ScriptCanvasFileState::NEW)
            {
                saveAttempt = OnFileSaveAs(saveCB);
            }
            else
            {
                auto scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset<ScriptCanvasAsset>(assetId);
                if (scriptCanvasAsset.IsReady())
                {
                    SaveScriptCanvasAsset(scriptCanvasAsset.Get()->GetPath().data(), scriptCanvasAsset, saveCB);
                    saveAttempt = true;
                }
            }
        }
        return saveAttempt;
    }

    bool MainWindow::OnFileSaveAs(const DocumentContextRequests::SaveCB& saveCB)
    {
        bool saveAttempt = false;
        if (m_activeAssetId.IsValid())
        {
            auto& assetId = m_activeAssetId;
            CreateSaveDestinationDirectory();
            AZStd::string suggestedFilename = GetSuggestedFullFilenameToSaveAs(assetId);
            QString filter = tr("Script Canvas Files (%1)").arg(ScriptCanvasAssetHandler::GetFileFilter());
            QString selectedFile = QFileDialog::getSaveFileName(this, tr("Save As..."), suggestedFilename.data(), filter);
            if (!selectedFile.isEmpty())
            {
                AZStd::string relativePath(selectedFile.toUtf8().data());
                bool foundPath;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundPath, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, selectedFile.toUtf8().data(), relativePath);
                if (foundPath)
                {
                    auto scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset(assetId);
                    SaveScriptCanvasAsset(relativePath.data(), scriptCanvasAsset, saveCB);
                    saveAttempt = true;
                }
            }
        }

        return saveAttempt;
    }

    void MainWindow::OnFileOpen()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

        AZ::Data::AssetId openId = ReadRecentAssetId();

        AZStd::string assetRoot;
        {
            AZStd::array<char, AZ::IO::MaxPathLength> assetRootChar;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@devassets@", assetRootChar.data(), assetRootChar.size());
            assetRoot = assetRootChar.data();
        }

        AZStd::string assetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, openId);
        assetPath = AZStd::string::format("%s/%s", assetRoot.c_str(), assetPath.c_str());

        if (!openId.IsValid() || !QFile::exists(assetPath.c_str()))
        {
            assetPath = AZStd::string::format("%s/scriptcanvas", assetRoot.c_str());
        }

        QString filter = tr("Script Canvas Files (%1)").arg(ScriptCanvasAssetHandler::GetFileFilter());

        QFileDialog dialog(nullptr, tr("Open..."), assetPath.c_str(), filter);
        dialog.setFileMode(QFileDialog::ExistingFiles);

        if (dialog.exec() == QDialog::Accepted) 
        {
            for (const QString& open : dialog.selectedFiles())
            {
                OpenFile(open.toUtf8().data());
            }
        }
    }

    void MainWindow::SetupEditMenu()
    {
        ui->action_Undo->setShortcut(QKeySequence::Undo);
        ui->action_Cut->setShortcut(QKeySequence(QKeySequence::Cut));
        ui->action_Copy->setShortcut(QKeySequence(QKeySequence::Copy));
        ui->action_Paste->setShortcut(QKeySequence(QKeySequence::Paste));
        ui->action_Delete->setShortcut(QKeySequence(QKeySequence::Delete));

        connect(ui->menuEdit, &QMenu::aboutToShow, this, &MainWindow::OnEditMenuShow);

        connect(ui->action_Undo, &QAction::triggered, this, &MainWindow::TriggerUndo);
        connect(ui->action_Redo, &QAction::triggered, this, &MainWindow::TriggerRedo);
        connect(ui->action_Cut, &QAction::triggered, this, &MainWindow::OnEditCut);
        connect(ui->action_Copy, &QAction::triggered, this, &MainWindow::OnEditCopy);
        connect(ui->action_Paste, &QAction::triggered, this, &MainWindow::OnEditPaste);
        connect(ui->action_Duplicate, &QAction::triggered, this, &MainWindow::OnEditDuplicate);
        connect(ui->action_Delete, &QAction::triggered, this, &MainWindow::OnEditDelete);
        connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::RefreshPasteAction);

        connect(ui->action_GlobalPreferences, &QAction::triggered, [this]() 
        {
            AZStd::intrusive_ptr<EditorSettings::PreviewSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::PreviewSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
            bool originalSetting = settings && settings->m_showExcludedNodes;           

            ScriptCanvasEditor::SettingsDialog(ui->action_GlobalPreferences->text(), AZ::EntityId(), this).exec();

            bool settingChanged = settings && settings->m_showExcludedNodes;

            if (originalSetting != settingChanged)
            {
                m_nodePalette->ResetModel();
            }

        });

        connect(ui->action_GraphPreferences, &QAction::triggered, [this]() {
            AZ::EntityId graphId = GetActiveGraphId();
            if (!graphId.IsValid())
            {
                return;
            }

            ScriptCanvasEditor::SettingsDialog(ui->action_GlobalPreferences->text(), graphId, this).exec();
        });
    }

    void MainWindow::OnEditMenuShow()
    {
        RefreshGraphPreferencesAction();
    }

    void MainWindow::RefreshPasteAction()
    {
        AZStd::string copyMimeType;
        GraphCanvas::SceneRequestBus::EventResult(copyMimeType, GetActiveSceneId(),
            &GraphCanvas::SceneRequests::GetCopyMimeType);
        bool pasteableClipboard = QApplication::clipboard()->mimeData()->hasFormat(copyMimeType.c_str());
        ui->action_Paste->setEnabled(pasteableClipboard);
    }

    void MainWindow::RefreshGraphPreferencesAction()
    {
        ui->action_GraphPreferences->setEnabled(GetActiveGraphId().IsValid());
    }

    void MainWindow::OnEditCut()
    {
        AZ::EntityId sceneId = GetActiveSceneId();
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::CutSelection);
    }

    void MainWindow::OnEditCopy()
    {
        AZ::EntityId sceneId = GetActiveSceneId();
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::CopySelection);
    }

    void MainWindow::OnEditPaste()
    {
        AZ::EntityId sceneId = GetActiveSceneId();
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Paste);
    }

    void MainWindow::OnEditDuplicate()
    {
        AZ::EntityId sceneId = GetActiveSceneId();
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::DuplicateSelection);
    }

    void MainWindow::OnEditDelete()
    {
        AZ::EntityId sceneId = GetActiveSceneId();
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::DeleteSelection);
    }

    void MainWindow::OnScriptCanvasAssetUnloaded(const AZ::Data::AssetId& assetId)
    {
        CloseScriptCanvasAsset(assetId);
    }

    void MainWindow::OnScriptCanvasAssetSaved(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset, bool isSuccessful)
    {
        AZ::Data::AssetStreamInfo assetStreamInfo = AZ::Data::AssetManager::Instance().GetSaveStreamInfoForAsset(scriptCanvasAsset.GetId(), scriptCanvasAsset.GetType());
        AddRecentFile(assetStreamInfo.m_streamName.data());
        UpdateRecentMenu();
    }

    void MainWindow::OnScriptCanvasAssetActivated(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        UpdateScriptCanvasAsset(scriptCanvasAsset);
        AZ::Entity* scriptCanvasEntity = scriptCanvasAsset.Get()->GetScriptCanvasEntity();
        if (scriptCanvasEntity)
        {
            UndoCache* undoCache = m_undoManager->GetUndoCache();

            if (undoCache)
            {
                undoCache->UpdateCache(scriptCanvasEntity->GetId());
            }
        }
    }

    void MainWindow::OnCanUndoChanged(bool canUndo)
    {
        ui->action_Undo->setEnabled(canUndo);
    }

    void MainWindow::OnCanRedoChanged(bool canRedo)
    {
        ui->action_Redo->setEnabled(canRedo);
    }

    //! GeneralRequestBus
    void MainWindow::OnChangeActiveGraphTab(const Widget::GraphTabMetadata& graphMetadata)
    {
        SetActiveAsset(graphMetadata.m_assetId);
    }

    AZ::EntityId MainWindow::GetActiveGraphId() const
    {
        AssetGraphSceneData* assetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(m_activeAssetId);
        return assetGraphSceneData ? assetGraphSceneData->m_tupleId.m_graphId : AZ::EntityId();
    }

    AZ::EntityId MainWindow::GetActiveSceneId() const
    {
        AssetGraphSceneData* assetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(m_activeAssetId);
        return assetGraphSceneData ? assetGraphSceneData->m_tupleId.m_sceneId : AZ::EntityId();
    }

    AZ::EntityId MainWindow::GetGraphId(const AZ::EntityId& sceneId) const
    {
        AssetGraphSceneData* assetGraphSceneData = m_assetGraphSceneMapper.GetBySceneId(sceneId);
        return assetGraphSceneData ? assetGraphSceneData->m_tupleId.m_graphId : AZ::EntityId();
    }

    AZ::EntityId MainWindow::GetSceneId(const AZ::EntityId& graphId) const
    {
        AssetGraphSceneData* assetGraphSceneData = m_assetGraphSceneMapper.GetByGraphId(graphId);
        return assetGraphSceneData ? assetGraphSceneData->m_tupleId.m_sceneId : AZ::EntityId();
    }

    bool MainWindow::eventFilter(QObject* object, QEvent* event)
    {
        // Will need to check the object for the correct element if we start filtering more then one object.
        // For now, we can bypass that.
        switch (event->type())
        {
        case QEvent::DragEnter:
            OnEmptyCanvasDragEnter(static_cast<QDragEnterEvent*>(event));
            m_enterState = event->isAccepted();
            return m_enterState;
            break;
        case QEvent::DragMove:
            if (m_enterState)
            {
                event->accept();
            }
            return m_enterState;
            break;
        case QEvent::DragLeave:
            m_enterState = false;
            break;
        case QEvent::Drop:
            if (m_enterState)
            {
                OnEmptyCanvasDrop(static_cast<QDropEvent*>(event));
                return m_enterState;
            }
            break;
        default:
            break;
        }

        return false;
    }

    void MainWindow::OnEmptyCanvasDragEnter(QDragEnterEvent* dragEnterEvent)
    {
        const QMimeData* mimeData = dragEnterEvent->mimeData();

        if (mimeData->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()) || mimeData->hasFormat(Widget::NodePalette::GetMimeType()))
        {
            dragEnterEvent->accept();
        }
    }

    void MainWindow::OnEmptyCanvasDrop(QDropEvent* dropEvent)
    {
        const QMimeData* mimeData = dropEvent->mimeData();

        if (mimeData->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()) || mimeData->hasFormat(Widget::NodePalette::GetMimeType()))
        {
            OnFileNew();

            if (m_activeAssetId.IsValid())
            {
                AZ::EntityId sceneId = GetActiveSceneId();

                AZ::EntityId viewId;
                GraphCanvas::SceneRequestBus::EventResult(viewId, sceneId, &GraphCanvas::SceneRequests::GetViewId);

                AZ::Vector2 scenePos = AZ::Vector2(dropEvent->pos().x(), dropEvent->pos().y());
                GraphCanvas::ViewRequestBus::EventResult(scenePos, viewId, &GraphCanvas::ViewRequests::MapToScene, scenePos);

                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::DispatchSceneDropEvent, scenePos, mimeData);
            }
        }
    }

    QVariant MainWindow::GetTabData(const AZ::Data::AssetId& assetId)
    {
        for (int tabIndex = 0; tabIndex < m_tabBar->count(); ++tabIndex)
        {
            QVariant data = m_tabBar->tabData(tabIndex);
            if (data.isValid())
            {
                auto graphTabMetadata = data.value<Widget::GraphTabMetadata>();
                if (graphTabMetadata.m_assetId == assetId)
                {
                    return data;
                }
            }
        }
        return QVariant();
    }

    bool MainWindow::IsTabOpen(const AZ::Data::AssetId& assetId, int& outTabIndex) const
    {
        int tabIndex = m_tabBar->FindTab(assetId);
        if (-1 != tabIndex)
        {
            outTabIndex = tabIndex;
            return true;
        }
        return false;
    }


    void MainWindow::SetActiveAsset(const AZ::Data::AssetId& assetId)
    {
        if (m_activeAssetId == assetId)
        {
            return;
        }

        m_tabBar->SelectTab(assetId);

        if (m_activeAssetId.IsValid())
        {
            QVariant graphTabMetadataVariant = GetTabData(m_activeAssetId);
            if (graphTabMetadataVariant.isValid())
            {
                auto hostMetadata = graphTabMetadataVariant.value<Widget::GraphTabMetadata>();
                if (hostMetadata.m_hostWidget)
                {
                    hostMetadata.m_hostWidget->hide();
                    m_layout->removeWidget(hostMetadata.m_hostWidget);
                }
            }
        }

        if (assetId.IsValid())
        {
            m_activeAssetId = assetId;
            RefreshActiveAsset();
        }
        else
        {
            m_activeAssetId.SetInvalid();
            m_emptyCanvas->show();
            SignalActiveSceneChanged(AZ::EntityId());
        }

        RefreshSelection();
    }

    void MainWindow::RefreshActiveAsset()
    {
        if (m_activeAssetId.IsValid())
        {
            auto assetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(m_activeAssetId);
            if (!assetGraphSceneData)
            {
                AZ_Assert(assetGraphSceneData, "Asset %s does not belong to this window", m_activeAssetId.ToString<AZStd::string>().data());
                return;
            }

            auto scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset<ScriptCanvasAsset>(m_activeAssetId);
            if (scriptCanvasAsset.IsReady() && scriptCanvasAsset.Get()->GetScriptCanvasEntity()->GetState() == AZ::Entity::ES_ACTIVE)
            {
                const AssetGraphSceneId& tupleId = assetGraphSceneData->m_tupleId;
                QVariant graphTabMetadataVariant = GetTabData(m_activeAssetId);
                if (graphTabMetadataVariant.isValid())
                {
                    auto hostMetadata = graphTabMetadataVariant.value<Widget::GraphTabMetadata>();
                    if (hostMetadata.m_hostWidget)
                    {
                        if (auto canvasWidget = qobject_cast<Widget::CanvasWidget*>(hostMetadata.m_hostWidget))
                        {
                            canvasWidget->ShowScene(tupleId.m_sceneId);
                        }
                        hostMetadata.m_hostWidget->show();
                        m_layout->addWidget(hostMetadata.m_hostWidget);
                    }
                }

                m_emptyCanvas->hide();

                SignalActiveSceneChanged(tupleId.m_sceneId);
            }
        }
    }

    void MainWindow::Clear()
    {
        m_tabBar->RemoveAllBars();

        m_assetGraphSceneMapper.m_assetIdToDataMap;
        AZStd::vector<AZ::Data::AssetId> assetIds;
        for (const auto& assetIdDataPair : m_assetGraphSceneMapper.m_assetIdToDataMap)
        {
            assetIds.push_back(assetIdDataPair.first);
        }

        for (const AZ::Data::AssetId& assetId : assetIds)
        {
            RemoveScriptCanvasAsset(assetId);
        }
        SetActiveAsset({});
    }

    void MainWindow::OnTabInserted(int index)
    {
        // This is invoked AFTER a new tab has been inserted.

        if (m_tabBar->count() == 1)
        {
            // The first tab has been added.
            m_tabBar->show();

            // We DON'T need the spacer keep
            // the "+" button right-aligned.
            m_horizontalTabBarLayout->removeItem(m_plusButtonSpacer);
        }
    }

    void MainWindow::OnTabRemoved(int index)
    {
        // This is invoked AFTER an existing tab has been removed.

        if (m_tabBar->count() == 0)
        {
            // The last tab has been removed.
            m_tabBar->hide();

            // We NEED the spacer keep the
            // "+" button right-aligned.
            m_horizontalTabBarLayout->insertSpacerItem(0, m_plusButtonSpacer);
        }
    }

    void MainWindow::OnTabCloseButtonPressed(int index)
    {
        QVariant data = m_tabBar->tabData(index);
        if (data.isValid())
        {
            auto graphTabMetadata = data.value<Widget::GraphTabMetadata>();

            UnsavedChangesOptions saveDialogResults = UnsavedChangesOptions::CONTINUE_WITHOUT_SAVING;
            if (graphTabMetadata.m_fileState == ScriptCanvasFileState::NEW || graphTabMetadata.m_fileState == ScriptCanvasFileState::MODIFIED)
            {
                SetActiveAsset(graphTabMetadata.m_assetId);
                saveDialogResults = ShowSaveDialog(graphTabMetadata.m_tabName);
            }

            if (saveDialogResults == UnsavedChangesOptions::SAVE)
            {
                SaveAsset(graphTabMetadata.m_assetId,
                            DocumentContextRequests::SaveCB([this](const AZ::Data::AssetId& assetId, bool isSuccessful)
                            {
                                if (isSuccessful)
                                {
                                    int tabIndex = -1;
                                    if (IsTabOpen(assetId, tabIndex))
                                    {
                                        OnTabCloseRequest(tabIndex);
                                    }
                                }
                                else
                                {
                                    QMessageBox::critical(this, QString(), QObject::tr("Failed to save."));
                                }
                            }));
            }
            else if (saveDialogResults == UnsavedChangesOptions::CONTINUE_WITHOUT_SAVING)
            {
                OnTabCloseRequest(index);
            }
        }
    }

    void MainWindow::OnTabCloseRequest(int index)
    {
        QVariant data = m_tabBar->tabData(index);
        if (data.isValid())
        {
            auto graphTabMetadata = data.value<Widget::GraphTabMetadata>();
            if (graphTabMetadata.m_assetId == m_activeAssetId)
            {
                SetActiveAsset({});
            }

            if (graphTabMetadata.m_hostWidget)
            {
                graphTabMetadata.m_hostWidget->hide();
            }

            m_tabBar->CloseTab(index);
            m_tabBar->update();
            RemoveScriptCanvasAsset(graphTabMetadata.m_assetId);

            if (m_tabBar->count() == 0)
            {
                // The last tab has been removed.
                SetActiveAsset({});
            }
        }
    }

    void MainWindow::OnSelectionChanged()
    {
        // Selection will be ignored when a delete operation is are taking place to prevent slowdown from processing
        // too many events at once.
        if (!m_ignoreSelection)
        {
            RefreshSelection();
        }
    }

    void MainWindow::OnNodeAdded(const AZ::EntityId&)
    {
        RefreshSelection();
    }

    void MainWindow::OnNodeRemoved(const AZ::EntityId&)
    {
        m_propertyGrid->ClearSelection();

        RefreshSelection();
    }

    void MainWindow::OnNodePositionChanged(const AZ::EntityId& nodeId, const AZ::Vector2&)
    {
        m_propertyGrid->OnNodeUpdate(nodeId);
    }

    void MainWindow::OnKeyReleased(QKeyEvent* event)
    {
        AZ::EntityId sceneId = (*GraphCanvas::SceneNotificationBus::GetCurrentBusId());

        switch (event->key())
        {
        case Qt::Key_F4:
        {
            QString text = QApplication::clipboard()->text();
            GraphCanvas::SceneRequestBus::Event(sceneId, static_cast<void (GraphCanvas::SceneRequests::*)(const AZStd::string&)>(&GraphCanvas::SceneRequests::SetStyleSheet), AZStd::string(text.toUtf8()));
        }
        break;
        default:
            break;
        }
    }

    void MainWindow::SetDefaultLayout()
    {
        // Disable updates while we restore the layout to avoid temporary glitches
        // as the panes are moved around
        setUpdatesEnabled(false);

        if (m_commandLine)
        {
            m_commandLine->hide();
        }

        if (m_nodePalette)
        {
            addDockWidget(Qt::LeftDockWidgetArea, m_nodePalette);
            m_nodePalette->setFloating(false);
            m_nodePalette->show();
        }

        if (m_propertyGrid)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_propertyGrid);
            m_propertyGrid->setFloating(false);
            m_propertyGrid->hide();
        }

        if (m_nodeOutliner)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_nodeOutliner);
            m_nodeOutliner->setFloating(false);
            m_nodeOutliner->hide();
        }

        // Disabled until debugger is implemented
        // addDockWidget(Qt::BottomDockWidgetArea, m_debugging);

        if (m_logPanel)
        {
            addDockWidget(Qt::BottomDockWidgetArea, m_logPanel);
            m_logPanel->setFloating(false);
            m_logPanel->show();
        }

        resizeDocks(
        { m_nodePalette, m_propertyGrid },
        { static_cast<int>(size().width() * 0.15f), static_cast<int>(size().width() * 0.2f) },
            Qt::Horizontal);

        // Disabled until debugger is implemented
        //resizeDocks({ m_logPanel }, { static_cast<int>(size().height() * 0.1f) }, Qt::Vertical);

        // Re-enable updates now that we've finished adjusting the layout
        setUpdatesEnabled(true);

        m_defaultLayout = saveState();
    }

    void MainWindow::RefreshSelection()
    {
        AZ::EntityId sceneId = GetActiveSceneId();
        bool hasNodesSelection = false;

        if (m_activeAssetId.IsValid())
        {
            AZStd::vector<AZ::EntityId> selectedEntities;
            if (sceneId.IsValid())
            {
                // Get the selected nodes.
                GraphCanvas::SceneRequestBus::EventResult(selectedEntities, sceneId, &GraphCanvas::SceneRequests::GetSelectedNodes);
                hasNodesSelection = !selectedEntities.empty();
            }

            if (!selectedEntities.empty())
            {
                m_propertyGrid->SetSelection(selectedEntities);
            }
            else
            {
                m_propertyGrid->ClearSelection();
            }
        }
        else
        {
            m_propertyGrid->ClearSelection();
        }

        // cut, copy and duplicate only works for nodes
        ui->action_Cut->setEnabled(hasNodesSelection);
        ui->action_Copy->setEnabled(hasNodesSelection);
        ui->action_Duplicate->setEnabled(hasNodesSelection);

        bool hasSelection = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelection, sceneId, &GraphCanvas::SceneRequests::HasSelectedItems);
        ui->action_Delete->setEnabled(hasSelection);
    }

    void MainWindow::OnViewNodePalette()
    {
        if (m_nodePalette)
        {
            m_nodePalette->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewOutline()
    {
        if (m_nodeOutliner)
        {
            m_nodeOutliner->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewProperties()
    {
        if (m_propertyGrid)
        {
            m_propertyGrid->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewDebugger()
    {
        if (m_debugging)
        {
            m_debugging->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewCommandLine()
    {
        if (m_commandLine->isVisible())
        {
            m_commandLine->hide();
        }
        else
        {
            m_commandLine->show();
        }
    }

    void MainWindow::OnViewLog()
    {
        if (m_logPanel)
        {
            m_logPanel->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnRestoreDefaultLayout()
    {
        if (!m_defaultLayout.isEmpty())
        {
            restoreState(m_defaultLayout);
        }
    }

    void MainWindow::DeleteNodes(const AZ::EntityId& sceneId, const AZStd::vector<AZ::EntityId>& nodes)
    {
        ScopedVariableSetter<bool> scopedIgnoreSelection(m_ignoreSelection, true);
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{ nodes.begin(), nodes.end() });
    }

    void MainWindow::DeleteConnections(const AZ::EntityId& sceneId, const AZStd::vector<AZ::EntityId>& connections)
    {
        ScopedVariableSetter<bool> scopedIgnoreSelection(m_ignoreSelection, true);
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{ connections.begin(), connections.end() });
    }

    void MainWindow::DisconnectEndpoints(const AZ::EntityId& sceneId, const AZStd::vector<GraphCanvas::Endpoint>& endpoints)
    {
        AZStd::unordered_set<AZ::EntityId> connections;
        for (const auto& endpoint : endpoints)
        {
            AZStd::vector<AZ::EntityId> endpointConnections;
            GraphCanvas::SceneRequestBus::EventResult(endpointConnections, sceneId, &GraphCanvas::SceneRequests::GetConnectionsForEndpoint, endpoint);
            connections.insert(endpointConnections.begin(), endpointConnections.end());
        }
        DeleteConnections(sceneId, { connections.begin(), connections.end() });
    }

    void MainWindow::SaveWindowState()
    {
        auto state = AZ::UserSettings::CreateFind<EditorSettings::WindowSavedState>(AZ_CRC("ScriptCanvasEditorWindowState", 0x10c47d36), AZ::UserSettings::CT_LOCAL);
        if (state)
        {
            state->Init(saveState(), saveGeometry());
        }
    }

    void MainWindow::RestoreWindowState()
    {
        auto state = AZ::UserSettings::Find<EditorSettings::WindowSavedState>(AZ_CRC("ScriptCanvasEditorWindowState", 0x10c47d36), AZ::UserSettings::CT_LOCAL);
        if (state)
        {
            state->Restore(this);
        }
    }

    NodeIdPair MainWindow::ProcessCreateNodeMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, const AZ::EntityId& sceneId, AZ::Vector2 nodeCreationPos)
    {
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::ClearSelection);

        NodeIdPair retVal;

        if (azrtti_istypeof<CreateNodeMimeEvent>(mimeEvent))
        {
            CreateNodeMimeEvent* createEvent = static_cast<CreateNodeMimeEvent*>(mimeEvent);            

            if (createEvent->ExecuteEvent(nodeCreationPos, nodeCreationPos, sceneId))
            {
                retVal = createEvent->GetCreatedPair();
            }
        }
        else if (azrtti_istypeof<SpecializedCreateNodeMimeEvent>(mimeEvent))
        {
            SpecializedCreateNodeMimeEvent* ebusHandlerEvent = static_cast<SpecializedCreateNodeMimeEvent*>(mimeEvent);
            retVal = ebusHandlerEvent->ConstructNode(sceneId, nodeCreationPos);
        }

        return retVal;
    }

    GraphCanvas::Endpoint MainWindow::CreateNodeForProposal(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        PushPreventUndoStateUpdate();

        GraphCanvas::Endpoint retVal;

        AZ::EntityId sceneId = (*GraphCanvas::SceneUIRequestBus::GetCurrentBusId());

        m_createNodeContextMenu->FilterForSourceSlot(sceneId, endpoint.GetSlotId());
        m_createNodeContextMenu->RefreshActions(sceneId);
        m_createNodeContextMenu->DisableCreateActions();

        QAction* action = m_createNodeContextMenu->exec(screenPoint);

        // If the action returns null. We need to check if it was our widget, or just a close command.
        if (action == nullptr)
        {
            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_createNodeContextMenu->GetNodePalette()->GetContextMenuEvent();

            if (mimeEvent)
            {
                bool isValid = false;
                NodeIdPair finalNode = ProcessCreateNodeMimeEvent(mimeEvent, sceneId, AZ::Vector2(scenePoint.x(), scenePoint.y()));

                GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_Invalid;
                GraphCanvas::SlotRequestBus::EventResult(connectionType, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetConnectionType);

                // Create the node
                AZStd::vector<AZ::EntityId> targetSlotIds;
                GraphCanvas::NodeRequestBus::EventResult(targetSlotIds, finalNode.m_graphCanvasId, &GraphCanvas::NodeRequests::GetSlotIds);
                for (const auto& targetSlotId : targetSlotIds)
                {
                    GraphCanvas::Endpoint proposedEndpoint(finalNode.m_graphCanvasId, targetSlotId);

                    bool canCreate = false;

                    if (connectionType == GraphCanvas::ConnectionType::CT_Output)
                    {
                        GraphCanvas::ConnectionSceneRequestBus::EventResult(canCreate, sceneId, &GraphCanvas::ConnectionSceneRequests::IsValidConnection, endpoint, proposedEndpoint);
                    }
                    else if (connectionType == GraphCanvas::ConnectionType::CT_Input)
                    {
                        GraphCanvas::ConnectionSceneRequestBus::EventResult(canCreate, sceneId, &GraphCanvas::ConnectionSceneRequests::IsValidConnection, proposedEndpoint, endpoint);
                    }

                    if (canCreate)
                    {
                        if (connectionType == GraphCanvas::ConnectionType::CT_Output)
                        {
                            GraphCanvas::ConnectionSceneRequestBus::EventResult(isValid, sceneId, &GraphCanvas::ConnectionSceneRequests::CreateConnection, connectionId, endpoint, proposedEndpoint);
                        }
                        else if (connectionType == GraphCanvas::ConnectionType::CT_Input)
                        {
                            GraphCanvas::ConnectionSceneRequestBus::EventResult(isValid, sceneId, &GraphCanvas::ConnectionSceneRequests::CreateConnection, connectionId, proposedEndpoint, endpoint);
                        }

                        if (isValid)
                        {
                            retVal = proposedEndpoint;
                            break;
                        }
                    }
                }

                if (retVal.IsValid())
                {
                    GraphCanvas::SceneNotificationBus::Event(sceneId, &GraphCanvas::SceneNotifications::PostCreationEvent);
                }
                else
                {
                    DeleteNodes(sceneId, AZStd::vector<AZ::EntityId>{finalNode.m_graphCanvasId});
                }
            }
        }

        PopPreventUndoStateUpdate();

        return retVal;
    }

    void MainWindow::OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect&, const QPointF&, const QPoint& screenPoint)
    {
        AZ::EntityId sceneId = (*GraphCanvas::SceneUIRequestBus::GetCurrentBusId());

        m_ebusHandlerActionMenu->SetEbusHandlerNode(wrapperNode);

        // We don't care about the result, since the actions are done on demand with the menu
        m_ebusHandlerActionMenu->exec(screenPoint);
    }

    void MainWindow::RequestUndoPoint()
    {
        AZ::EntityId sceneId = (*GraphCanvas::SceneUIRequestBus::GetCurrentBusId());

        PostUndoPoint(sceneId);
    }

    void MainWindow::RequestPushPreventUndoStateUpdate()
    {
        PushPreventUndoStateUpdate();
    }

    void MainWindow::RequestPopPreventUndoStateUpdate()
    {
        PopPreventUndoStateUpdate();
    }

#include <Editor/View/Windows/MainWindow.moc>
}
