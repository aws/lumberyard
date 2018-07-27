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

#include <ISystem.h>
#include <IConsole.h>

#include <Editor/View/Windows/MainWindow.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

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
#include <QDir>
#include <QDirIterator>
#include <QProgressDialog>

#include <Editor/Undo/ScriptCanvasUndoCache.h>
#include <Editor/Undo/ScriptCanvasUndoManager.h>
#include <Editor/View/Dialogs/UnsavedChangesDialog.h>
#include <Editor/View/Dialogs/PreviewMessage.h>
#include <Editor/View/Dialogs/Settings.h>
#include <Editor/View/Widgets/NodeFavorites.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/Debugging.h>
#include <Editor/View/Widgets/PropertyGrid.h>
#include <Editor/View/Widgets/NodeOutliner.h>
#include <Editor/View/Widgets/CommandLine.h>
#include <Editor/View/Widgets/GraphTabBar.h>
#include <Editor/View/Widgets/NodeFavorites.h>
#include <Editor/View/Widgets/CanvasWidget.h>
#include <Editor/View/Widgets/LogPanel.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

#include <Editor/View/Windows/ui_mainwindow.h>

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

#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasDocumentContext.h>

#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>

#include <GraphCanvas/Styling/Parser.h>
#include <GraphCanvas/Styling/Style.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

#include "ConnectionContextMenu.h"
#include "NodeContextMenu.h"
#include "SceneContextMenu.h"
#include "SlotContextMenu.h"
#include "CreateNodeContextMenu.h"
#include "EBusHandlerActionMenu.h"
#include "Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h"
#include "Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h"

// Save Format Conversion
#include <AzCore/Component/EntityUtils.h>
#include <Editor/Include/ScriptCanvas/Components/EditorGraph.h>
////

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

        void CreateSaveDestinationDirectory()
        {
            AZStd::string unresolvedDestination("@devassets@/scriptcanvas");

            AZStd::array<char, AZ::IO::MaxPathLength> assetRoot;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(unresolvedDestination.c_str(), assetRoot.data(), assetRoot.size());

            // We just need the path to exist.
            QDir(assetRoot.data()).mkpath(".");
        }
    } // anonymous namespace.

      ///////////////////////////////
      // ScriptCanvasBatchConverter
      ///////////////////////////////
    class ScriptCanvasBatchConverter
        : public GraphCanvas::AssetEditorNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasBatchConverter, AZ::SystemAllocator, 0);

        ScriptCanvasBatchConverter(MainWindow* mainWindow)
            : m_mainWindow(mainWindow)
            , m_processing(false)
            , m_triggerSave(false)
        {
        }

        ~ScriptCanvasBatchConverter()
        {
            ISystem* system = nullptr;
            CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);

            ICVar* cvar = system->GetIConsole()->GetCVar("ed_KeepEditorActive");

            if (cvar)
            {
                cvar->Set(m_originalActive);
            }

            delete m_progressDialog;

            for (QDirIterator* dirIterator : m_directoryIterators)
            {
                delete dirIterator;
            }
        }

        void BatchConvert(QStringList directories)
        {
            ISystem* system = nullptr;
            CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);

            ICVar* cvar = system->GetIConsole()->GetCVar("ed_KeepEditorActive");

            if (cvar)
            {
                m_originalActive = cvar->GetIVal();
                cvar->Set(1);
            }

            GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

            m_currentTab = m_mainWindow->m_tabBar->currentIndex();

            m_progressDialog = new QProgressDialog(m_mainWindow);

            m_progressDialog->setWindowFlags(m_progressDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
            m_progressDialog->setLabelText("Running Batch Conversion...");
            m_progressDialog->setWindowModality(Qt::WindowModal);
            m_progressDialog->setMinimum(0);
            m_progressDialog->setMaximum(0);
            m_progressDialog->setMinimumDuration(0);
            m_progressDialog->setAutoClose(false);
            m_progressDialog->setCancelButton(nullptr);

            m_progressDialog->show();

            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

            for (QString directory : directories)
            {
                QDirIterator* directoryIterator = new QDirIterator(directory, QDirIterator::Subdirectories);

                m_directoryIterators.push_back(directoryIterator);
            }

            TickIterator();
        }

        // GraphCanvas::AssetEditorNotifications
        void PostOnActiveGraphChanged() override
        {
            if (m_assetId.IsValid() && m_triggerSave)
            {
                m_triggerSave = false;

                m_mainWindow->SaveAsset(m_assetId, [this](bool isSuccessful)
                {
                    m_processing = false;

                    AZ::Data::AssetId assetId = m_assetId;
                    m_assetId.SetInvalid();
                    m_mainWindow->CloseScriptCanvasAsset(assetId);
                    TickIterator();
                });
            }
        }

    private:

        void BatchConvertDirectory(QDir directory)
        {
            QDirIterator* directoryIterator = new QDirIterator(directory, QDirIterator::Subdirectories);

            m_directoryIterators.insert(m_directoryIterators.begin(), directoryIterator);
        }

        bool BatchConvertFile(const QString& fileName)
        {
            bool tickIterator = true;

            QByteArray utf8FileName = fileName.toUtf8();
            const bool loadBlocking = true;
            AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset;
            DocumentContextRequestBus::BroadcastResult(scriptCanvasAsset, &DocumentContextRequests::LoadScriptCanvasAsset, utf8FileName.data(), loadBlocking);
            if (scriptCanvasAsset.IsReady())
            {
                if (m_mainWindow->m_tabBar->FindTab(scriptCanvasAsset.GetId()) < 0)
                {
                    m_assetId = scriptCanvasAsset.GetId();
                    m_triggerSave = true;
                    m_processing = true;

                    m_progressDialog->setLabelText(QString("Converting %1...\n").arg(fileName));
                    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

                    m_mainWindow->OpenScriptCanvasAsset(scriptCanvasAsset);

                    tickIterator = false;
                }
            }

            return tickIterator;
        }

        void TickIterator()
        {
            while (!m_directoryIterators.empty())
            {
                QDirIterator* dirIterator = m_directoryIterators.back();

                while (dirIterator->hasNext())
                {
                    QString newElement = dirIterator->next();

                    if (newElement.endsWith("."))
                    {
                        continue;
                    }
                    else if (newElement.endsWith(".scriptcanvas"))
                    {
                        if (!BatchConvertFile(newElement))
                        {
                            break;
                        }
                    }
                    else
                    {
                        QFileInfo fileInfo(newElement);

                        if (fileInfo.isDir())
                        {
                            BatchConvertDirectory(QDir(newElement));
                        }
                    }
                }

                if (!dirIterator->hasNext() && !m_processing)
                {
                    delete dirIterator;
                    m_directoryIterators.pop_back();
                }
                else
                {
                    break;
                }
            }

            if (m_directoryIterators.empty() && !m_processing)
            {
                if (m_progressDialog)
                {
                    m_progressDialog->hide();
                    delete m_progressDialog;
                    m_progressDialog = nullptr;
                }

                m_mainWindow->m_tabBar->setCurrentIndex(m_currentTab);

                // Essentially a delete this. From this point on this element is garbage.
                m_mainWindow->SignalConversionComplete();
            }
        }

        AZStd::vector< QDirIterator* > m_directoryIterators;

        MainWindow* m_mainWindow;

        int m_originalActive;

        int m_currentTab;
        bool m_processing;
        bool m_triggerSave;

        QProgressDialog* m_progressDialog;
        AZ::Data::AssetId m_assetId;
    };

    ///////////////
    // MainWindow
    ///////////////

    MainWindow::AssetGraphSceneData::AssetGraphSceneData(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
        : m_asset(scriptCanvasAsset)
    {
        m_tupleId.m_assetId = m_asset.GetId();
        AZ::Entity* sceneEntity = scriptCanvasAsset.Get() ? m_asset.Get()->GetScriptCanvasEntity() : nullptr;
        if (sceneEntity)
        {
            m_tupleId.m_sceneId = sceneEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(m_tupleId.m_graphId, &ScriptCanvas::SystemRequests::FindGraphId, sceneEntity);
        }
    }

    void MainWindow::AssetGraphSceneData::Set(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        m_asset = scriptCanvasAsset;
        m_tupleId.m_assetId = scriptCanvasAsset.GetId();
        AZ::Entity* sceneEntity = scriptCanvasAsset.Get() ? m_asset.Get()->GetScriptCanvasEntity() : nullptr;
        if (sceneEntity)
        {
            m_tupleId.m_sceneId = sceneEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(m_tupleId.m_graphId, &ScriptCanvas::SystemRequests::FindGraphId, sceneEntity);
        }
    }

    AZ::Outcome<void, AZStd::string> MainWindow::AssetGraphSceneMapper::Add(AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset)
    {
        if (scriptCanvasAsset.GetId().IsValid())
        {
            auto assetIt = m_assetIdToDataMap.find(scriptCanvasAsset.GetId());
            if (assetIt != m_assetIdToDataMap.end())
            {
                assetIt->second->Set(scriptCanvasAsset);
            }
            else
            {
                m_assetIdToDataMap.emplace(scriptCanvasAsset.GetId(), AZStd::make_unique<AssetGraphSceneData>(scriptCanvasAsset));
            }

            return AZ::Success();
        }

        return AZ::Failure(AZStd::string("Script Canvas asset has invalid asset id"));
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

    ////////////////
    // MainWindow
    ////////////////

    MainWindow::MainWindow()
        : QMainWindow(nullptr, Qt::Widget | Qt::WindowMinMaxButtonsHint)
        , ui(new Ui::MainWindow)
        , m_enterState(false)
        , m_ignoreSelection(false)
        , m_preventUndoStateUpdateCount(0)
        , m_converter(nullptr)
        , m_styleManager(ScriptCanvasEditor::AssetEditorId, "ScriptCanvas/StyleSheet/graphcanvas_style.json")
    {
        VariablePaletteRequestBus::Handler::BusConnect();

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

        m_emptyCanvas = aznew GraphCanvas::GraphCanvasEditorCentralWidget(this);
        m_emptyCanvas->SetDragTargetText(tr("Use the File Menu or drag out a node from the Node Palette to create a new script.").toStdString().c_str());

        m_emptyCanvas->SetEditorId(ScriptCanvasEditor::AssetEditorId);

        m_emptyCanvas->RegisterAcceptedMimeType(Widget::NodePaletteDockWidget::GetMimeType());
        m_emptyCanvas->RegisterAcceptedMimeType(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

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

        m_minimap = aznew GraphCanvas::MiniMapDockWidget(ScriptCanvasEditor::AssetEditorId, this);
        m_minimap->setObjectName("MiniMapDockWidget");

        m_variableDockWidget = new VariableDockWidget(this);
        m_variableDockWidget->setObjectName("VariableManager");
        m_variableDockWidget->setMinimumHeight(40);

        m_propertyGrid = new Widget::PropertyGrid(this, "Node Inspector");
        m_propertyGrid->setObjectName("NodeInspector");

        m_bookmarkDockWidget = aznew GraphCanvas::BookmarkDockWidget(ScriptCanvasEditor::AssetEditorId, this);

        // Disabled until debugger is implemented
        //m_debugging = new Widget::Debugging(this);
        //m_debugging->setObjectName("Debugging");
        //m_logPanel = new Widget::LogPanelWidget(this);
        //m_logPanel->setObjectName("LogPanel");

        m_nodePalette = aznew Widget::NodePaletteDockWidget(tr("Node Palette"), this);
        m_nodePalette->setObjectName("NodePalette");

        // This needs to happen after the node palette is created, because we scrape for the variable data from inside
        // of there.
        m_variableDockWidget->PopulateVariablePalette(m_variablePaletteTypes);

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
        ScriptCanvasEditor::AutomationRequestBus::Handler::BusConnect();

        UIRequestBus::Handler::BusConnect();
        UndoNotificationBus::Handler::BusConnect();
        GraphCanvas::AssetEditorRequestBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
        GraphCanvas::AssetEditorSettingsRequestBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
        ScriptCanvas::BatchOperationNotificationBus::Handler::BusConnect();

        CreateUndoManager();

        UINotificationBus::Broadcast(&UINotifications::MainWindowCreationEvent, this);
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendEditorMetric, ScriptCanvasEditor::Metrics::Events::Editor::Open, m_activeAssetId);

        // Show the PREVIEW welcome message
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
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

        connect(m_nodePalette, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_nodeOutliner, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_minimap, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_propertyGrid, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_bookmarkDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_variableDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
    }

    MainWindow::~MainWindow()
    {
        SaveWindowState();

        ScriptCanvas::BatchOperationNotificationBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorRequestBus::Handler::BusDisconnect();
        UndoNotificationBus::Handler::BusDisconnect();
        UIRequestBus::Handler::BusDisconnect();
        ScriptCanvasEditor::GeneralRequestBus::Handler::BusDisconnect();

        Clear();

        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendMetric, ScriptCanvasEditor::Metrics::Events::Editor::Close);

        delete m_converter;
    }

    void MainWindow::CreateMenus()
    {
        // File menu
        connect(ui->action_New_Script, &QAction::triggered, this, &MainWindow::OnFileNew);
        ui->action_New_Script->setShortcut(QKeySequence(QKeySequence::New));

        connect(ui->action_Open, &QAction::triggered, this, &MainWindow::OnFileOpen);
        ui->action_Open->setShortcut(QKeySequence(QKeySequence::Open));

        connect(ui->action_BatchConversion, &QAction::triggered, this, &MainWindow::RunBatchConversion);
        ui->action_BatchConversion->setVisible(true);

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

        // Disabling the Minimap since it does not play nicely with the Qt caching solution
        // And causing some weird visual issues.
        connect(ui->action_ViewMiniMap, &QAction::triggered, this, &MainWindow::OnViewMiniMap);
        ui->action_ViewMiniMap->setVisible(false);

        connect(ui->action_ViewProperties, &QAction::triggered, this, &MainWindow::OnViewProperties);
        connect(ui->action_ViewBookmarks, &QAction::triggered, this, &MainWindow::OnBookmarks);

        connect(ui->action_ViewVariableManager, &QAction::triggered, this, &MainWindow::OnVariableManager);
        connect(m_variableDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);

        connect(ui->action_ViewDebugger, &QAction::triggered, this, &MainWindow::OnViewDebugger);
        connect(ui->action_ViewCommandLine, &QAction::triggered, this, &MainWindow::OnViewCommandLine);
        connect(ui->action_ViewLog, &QAction::triggered, this, &MainWindow::OnViewLog);        

        connect(ui->action_ViewRestoreDefaultLayout, &QAction::triggered, this, &MainWindow::OnRestoreDefaultLayout);        
    }

    void MainWindow::SignalActiveSceneChanged(const AZ::EntityId& scriptCanvasGraphId)
    {
        // The paste action refreshes based on the scene's mimetype
        RefreshPasteAction();

        AZ::EntityId graphCanvasGraphId = GetGraphCanvasGraphId(scriptCanvasGraphId);

        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::PreOnActiveGraphChanged);
        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::OnActiveGraphChanged, graphCanvasGraphId);
        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::PostOnActiveGraphChanged);
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

    void MainWindow::OnViewVisibilityChanged(bool visibility)
    {
        UpdateViewMenu();
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
                DocumentContextRequests::SaveCB saveCB = [this, assetId](bool isSuccessful)
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
                };
                SaveAsset(assetId, saveCB);
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
        SignalSceneDirty(GetActiveScriptCanvasGraphId());
    }

    void MainWindow::TriggerRedo()
    {
        m_undoManager->Redo();
        SignalSceneDirty(GetActiveScriptCanvasGraphId());
    }

    void MainWindow::RegisterVariableType(const ScriptCanvas::Data::Type& variableType)
    {        
        m_variablePaletteTypes.insert(ScriptCanvas::Data::ToAZType(variableType));
    }

    void MainWindow::PostUndoPoint(AZ::EntityId scriptCanvasGraphId)
    {
        if (m_preventUndoStateUpdateCount == 0 && !m_undoManager->IsInUndoRedo())
        {
            ScopedUndoBatch scopedUndoBatch("Modify Graph Canvas Scene");
            AZ::Entity* scriptCanvasEntity{};
            AZ::ComponentApplicationBus::BroadcastResult(scriptCanvasEntity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasGraphId);
            m_undoManager->AddGraphItemChangeUndo(scriptCanvasEntity, "Graph Change");

            SignalSceneDirty(scriptCanvasGraphId);
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
        AZ::EntityId scriptCanvasGraphId = sceneEntity ? sceneEntity->GetId() : AZ::EntityId();

        GraphCanvas::GraphId graphCanvsGraphId = GetGraphCanvasGraphId(scriptCanvasGraphId);

        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::OnGraphRefreshed, assetGraphSceneData->m_tupleId.m_graphId, graphCanvsGraphId);

        assetGraphSceneData->Set(scriptCanvasAsset);

        AZ::Outcome<ScriptCanvasAssetFileInfo, AZStd::string> assetInfoOutcome = AZ::Failure(AZStd::string());
        DocumentContextRequestBus::BroadcastResult(assetInfoOutcome, &DocumentContextRequests::GetFileInfo, scriptCanvasAsset.GetId());
        if (assetInfoOutcome)
        {
            const auto& assetFileInfo = assetInfoOutcome.GetValue();
            AZStd::string tabName;
            AzFramework::StringFunc::Path::GetFileName(assetFileInfo.m_absolutePath.data(), tabName);
            int tabIndex = -1;
            if (IsTabOpen(scriptCanvasAsset.GetId(), tabIndex))
            {
                auto tabVariant = m_tabBar->tabData(tabIndex);
                if (tabVariant.isValid())
                {
                    auto graphMetadata = tabVariant.value<Widget::GraphTabMetadata>();
                    graphMetadata.m_fileState = assetFileInfo.m_fileModificationState;
                    graphMetadata.m_tabName = tabName.data();
                    m_tabBar->SetGraphTabData(tabIndex, graphMetadata);
                    m_tabBar->setTabToolTip(tabIndex, assetFileInfo.m_absolutePath.data());
                }
            }
        }

        if (scriptCanvasGraphId.IsValid())
        {
            AZ::EntityId graphCanvasGraphId = GetGraphCanvasGraphId(scriptCanvasGraphId);

            GraphCanvas::SceneNotificationBus::MultiHandler::BusConnect(graphCanvasGraphId);
            GraphCanvas::SceneUIRequestBus::MultiHandler::BusConnect(graphCanvasGraphId);
            GraphCanvas::SceneMimeDelegateRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneMimeDelegateRequests::AddDelegate, m_entityMimeDelegateId);

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SetMimeType, Widget::NodePaletteDockWidget::GetMimeType());
            GraphCanvas::SceneMemberNotificationBus::Event(graphCanvasGraphId, &GraphCanvas::SceneMemberNotifications::OnSceneReady);
        }
    }

    AZ::Outcome<int, AZStd::string> MainWindow::CreateScriptCanvasAsset(AZStd::string_view assetPath, int tabIndex)
    {
        AZ::Data::Asset<ScriptCanvasAsset> newAsset;
        DocumentContextRequestBus::BroadcastResult(newAsset, &DocumentContextRequests::CreateScriptCanvasAsset, assetPath);
        m_assetGraphSceneMapper.Add(newAsset);

        int outTabIndex = -1;
        {
            // Insert tab block
            AZStd::string tabName;
            AzFramework::StringFunc::Path::GetFileName(assetPath.data(), tabName);
            m_tabBar->InsertGraphTab(tabIndex, newAsset.GetId(), tabName);
            if (!IsTabOpen(newAsset.GetId(), outTabIndex))
            {
                return AZ::Failure(AZStd::string::format("Unable to open new Script Canvas Asset with id %s in the Script Canvas Editor", newAsset.ToString<AZStd::string>().c_str()));
            }

            m_tabBar->setTabToolTip(outTabIndex, assetPath.data());
        }

        DocumentContextNotificationBus::MultiHandler::BusConnect(newAsset.GetId());
        ActivateAssetEntity(newAsset);

        return AZ::Success(outTabIndex);
    }

    AZ::Outcome<int, AZStd::string> MainWindow::OpenScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset, int tabIndex, AZ::EntityId contextId)
    {
        const AZ::Data::AssetId& assetId = scriptCanvasAsset.GetId();

        if (!assetId.IsValid())
        {
            return AZ::Failure(AZStd::string("Unable to open asset with invalid asset id"));
        }

        // Reuse the tab if available
        int outTabIndex = -1;
        auto assetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(assetId);
        if (assetGraphSceneData)
        {
            outTabIndex = m_tabBar->FindTab(assetId);
            m_tabBar->setCurrentIndex(outTabIndex);
            return AZ::Success(outTabIndex);
        }
        else
        {
            m_tabBar->InsertGraphTab(tabIndex, assetId);
            outTabIndex = m_tabBar->FindTab(assetId);
        }
        if (outTabIndex == -1)
        {
            return AZ::Failure(AZStd::string::format("Unable to open existing Script Canvas Asset with id %s in the Script Canvas Editor", scriptCanvasAsset.ToString<AZStd::string>().c_str()));
        }
        else
        {
            // asset successfully opened, add it to the recent file list
            AZStd::string rootFolder;
            AzFramework::AssetSystem::SourceAssetInfoResponse response;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(response.m_found, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID, assetId.m_guid, response.m_assetInfo, rootFolder);

            if (response.m_found)
            {
                AZStd::string fullPath;
                AzFramework::StringFunc::Path::Join(rootFolder.c_str(), response.m_assetInfo.m_relativePath.c_str(), fullPath);
                AddRecentFile(QString::fromUtf8(fullPath.c_str()));
                UpdateRecentMenu();
            }
        }

        AZ::Data::Asset<ScriptCanvasAsset> newAsset;
        if (scriptCanvasAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
        {
            ScriptCanvasAssetFileInfo scFileInfo;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string rootFilePath;
            AzToolsFramework::AssetSystemRequestBus::Broadcast(&AzToolsFramework::AssetSystemRequestBus::Events::GetAssetInfoById, assetId, azrtti_typeid<ScriptCanvasAsset>(), assetInfo, rootFilePath);
            AzFramework::StringFunc::Path::Join(rootFilePath.data(), assetInfo.m_relativePath.data(), scFileInfo.m_absolutePath);
            scFileInfo.m_fileModificationState = ScriptCanvasFileState::UNMODIFIED;
            DocumentContextRequestBus::Broadcast(&DocumentContextRequests::RegisterScriptCanvasAsset, assetId, scFileInfo);
            newAsset = scriptCanvasAsset;
        }
        else
        {
            DocumentContextRequestBus::BroadcastResult(newAsset, &DocumentContextRequests::LoadScriptCanvasAssetById, assetId, false);
        }

        AZ::Outcome<ScriptCanvasAssetFileInfo, AZStd::string> assetInfoOutcome = AZ::Failure(AZStd::string());
        DocumentContextRequestBus::BroadcastResult(assetInfoOutcome, &DocumentContextRequests::GetFileInfo, newAsset.GetId());
        AZStd::string assetPath = assetInfoOutcome ? assetInfoOutcome.GetValue().m_absolutePath : "";

        AZStd::string tabName;
        AzFramework::StringFunc::Path::GetFileName(assetPath.data(), tabName);
        Widget::GraphTabMetadata graphMetadata = m_tabBar->tabData(outTabIndex).value<Widget::GraphTabMetadata>();
        DocumentContextRequestBus::BroadcastResult(graphMetadata.m_fileState, &DocumentContextRequests::GetScriptCanvasAssetModificationState, graphMetadata.m_assetId);
        graphMetadata.m_tabName = tabName.data();

        m_tabBar->SetGraphTabData(outTabIndex, graphMetadata);
        m_tabBar->setTabToolTip(outTabIndex, assetPath.data());

        DocumentContextNotificationBus::MultiHandler::BusConnect(newAsset.GetId());
        // IsReady also checks the ReadyPreNotify state which signifies that the asset is ready
        // but the AssetBus::OnAssetReady event has not been dispatch
        // Here we only want to invoke the method if the OnAsseteReady event has been dispatch
        if (newAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
        {
            CloneAssetEntity(newAsset);
        }

        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendGraphMetric, ScriptCanvasEditor::Metrics::Events::Canvas::OpenGraph, newAsset.GetId());

        return AZ::Success(outTabIndex);
    }

    AZ::Outcome<int, AZStd::string> MainWindow::UpdateScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        int outTabIndex = -1;

        PushPreventUndoStateUpdate();
        RefreshScriptCanvasAsset(scriptCanvasAsset);
        if (IsTabOpen(scriptCanvasAsset.GetId(), outTabIndex))
        {
            RefreshActiveAsset();
        }
        PopPreventUndoStateUpdate();

        if (outTabIndex == -1)
        {
            return AZ::Failure(AZStd::string::format("Script Canvas Asset %s is not open in a tab", scriptCanvasAsset.ToString<AZStd::string>().c_str()));
        }
        return AZ::Success(outTabIndex);
    }

    void MainWindow::RemoveScriptCanvasAsset(const AZ::Data::AssetId& assetId)
    {
        DocumentContextRequestBus::Broadcast(&DocumentContextRequests::UnregisterScriptCanvasAsset, assetId);
        DocumentContextNotificationBus::MultiHandler::BusDisconnect(assetId);
        auto assetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(assetId);
        if (assetGraphSceneData)
        {
            auto& tupleId = assetGraphSceneData->m_tupleId;
            GraphCanvas::SceneNotificationBus::MultiHandler::BusDisconnect(tupleId.m_sceneId);

            m_assetGraphSceneMapper.Remove(assetId);
            GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::OnGraphUnloaded, tupleId.m_graphId);
        }
    }

    void MainWindow::MoveScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& newAsset, const ScriptCanvasAssetFileInfo& newAssetFileInfo)
    {
        m_assetGraphSceneMapper.Add(newAsset);
        GraphCanvas::SceneNotificationBus::MultiHandler::BusConnect(newAsset.Get()->GetScriptCanvasEntity()->GetId());

        DocumentContextRequestBus::Broadcast(&DocumentContextRequests::RegisterScriptCanvasAsset, newAsset.GetId(), newAssetFileInfo);

        DocumentContextNotificationBus::MultiHandler::BusConnect(newAsset.GetId());
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

    //! Hook for receiving context menu events for each QGraphicsScene
    void MainWindow::OnSceneContextMenuEvent(const AZ::EntityId& graphCanvasGraphId, QGraphicsSceneContextMenuEvent* event)
    {
        if (!graphCanvasGraphId.IsValid())
        {
            // Nothing to do.
            return;
        }

        AZStd::any* userData = nullptr;
        GraphCanvas::SceneRequestBus::EventResult(userData, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetUserData);

        AZ::EntityId scriptCanvasGraphId;

        if (userData && userData->is<AZ::EntityId>())
        {
            scriptCanvasGraphId = (*AZStd::any_cast<AZ::EntityId>(userData));
        }

        m_createNodeContextMenu->ResetSourceSlotFilter();
        m_createNodeContextMenu->RefreshActions(scriptCanvasGraphId);
        QAction* action = m_createNodeContextMenu->exec(QPoint(event->screenPos().x(), event->screenPos().y()));

        if (action == nullptr)
        {
            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_createNodeContextMenu->GetNodePalette()->GetContextMenuEvent();

            bool isValid = false;
            NodeIdPair finalNode = ProcessCreateNodeMimeEvent(mimeEvent, graphCanvasGraphId, AZ::Vector2(event->scenePos().x(), event->scenePos().y()));

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

            if (finalNode.m_graphCanvasId.IsValid())
            {
                event->accept();
                GraphCanvas::SceneNotificationBus::Event(graphCanvasGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
            }
        }
        else
        {
            CreateNodeAction* nodeAction = qobject_cast<CreateNodeAction*>(action);

            if (nodeAction)
            {
                PushPreventUndoStateUpdate();
                AZ::Vector2 mousePoint(event->scenePos().x(), event->scenePos().y());
                bool triggerUndo = nodeAction->TriggerAction(scriptCanvasGraphId, mousePoint);
                PopPreventUndoStateUpdate();

                if (triggerUndo)
                {
                    PostUndoPoint(scriptCanvasGraphId);
                }
            }
        }
    }

    void MainWindow::OnSceneDoubleClickEvent(const AZ::EntityId& graphCanvasGraphId, QGraphicsSceneMouseEvent* event)
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
        AZ::Outcome<ScriptCanvasAssetFileInfo, AZStd::string> assetInfoOutcome = AZ::Failure(AZStd::string());
        DocumentContextRequestBus::BroadcastResult(assetInfoOutcome, &DocumentContextRequests::GetFileInfo, assetId);
        AZStd::string assetPath;
        if (assetInfoOutcome)
        {
            const auto& assetFileInfo = assetInfoOutcome.GetValue();
            assetPath = assetInfoOutcome.GetValue().m_absolutePath;
        }

        if (assetPath.empty())
        {
            int tabIndex = -1;
            if (IsTabOpen(assetId, tabIndex))
            {
                QVariant data = m_tabBar->tabData(tabIndex);
                if (data.isValid())
                {
                    auto graphTabMetadata = data.value<Widget::GraphTabMetadata>();
                    assetPath = AZStd::string::format("@devassets@/scriptcanvas/%s.%s", graphTabMetadata.m_tabName.toUtf8().data(), ScriptCanvasAssetHandler::GetFileExtension());
                }
            }
        }

        AZStd::array<char, AZ::IO::MaxPathLength> resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(assetPath.data(), resolvedPath.data(), resolvedPath.size());
        return resolvedPath.data();
    }

    void MainWindow::SaveScriptCanvasAsset(AZStd::string_view filename, AZ::Data::Asset<ScriptCanvasAsset> sourceAsset, const DocumentContextRequests::SaveCB& saveCB)
    {
        if (!sourceAsset.IsReady())
        {
            return;
        }

        AZStd::string watchFolder;
        AZ::Data::AssetInfo catalogAssetInfo;
        bool sourceInfoFound{};
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, filename.data(), catalogAssetInfo, watchFolder);
        auto saveAssetId = sourceInfoFound ? catalogAssetInfo.m_assetId : AZ::Data::AssetId(AZ::Uuid::CreateRandom());

        AZ::Data::Asset<ScriptCanvasAsset> saveAsset = CopyAssetForSave(saveAssetId, sourceAsset);
        DocumentContextRequests::SourceFileChangedCB idChangedCB = [this, saveAssetId, sourceAsset, saveAsset](AZStd::string relPath, AZStd::string scanFolder, const AZ::Data::AssetId& newAssetId)
        {
            SourceFileChanged(saveAssetId, relPath, scanFolder, newAssetId);
        };

        ////
        // Version Conversion for Save File Revisions: Introduced in 1.14
        bool forceReload = false;

        {
            AZ::Entity* clonedScriptCanvasEntity = saveAsset.GetAs<ScriptCanvasAsset>()->GetScriptCanvasEntity();

            ScriptCanvasEditor::Graph* graphComponent = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasEditor::Graph>(clonedScriptCanvasEntity);

            if (graphComponent->NeedsSaveConversion())
            {
                bool isActive = clonedScriptCanvasEntity->GetState() == AZ::Entity::State::ES_ACTIVE;

                if (isActive)
                {
                    clonedScriptCanvasEntity->Deactivate();
                }
                else if (clonedScriptCanvasEntity->GetState() != AZ::Entity::State::ES_INIT)
                {
                    // Entity needs to be init'd in order for the internal component states
                    // to be valid
                    clonedScriptCanvasEntity->Init();
                }

                graphComponent->ConvertSaveFormat();

                forceReload = true;
            }
        }
        ////

        AZStd::string tabName;
        AzFramework::StringFunc::Path::GetFileName(filename.data(), tabName);
        QString capturedFileName(QString::fromUtf8(filename.to_string().c_str()));
        DocumentContextRequests::SaveCB wrappedSaveCB = [this, capturedFileName, sourceAsset, saveAsset, tabName, forceReload, saveCB](bool saveSuccess)
        {
            if (sourceAsset != saveAsset || forceReload)
            {
                int saveTabIndex = m_tabBar->FindTab(saveAsset.GetId());
                if (saveTabIndex == -1)
                {
                    saveTabIndex = m_tabBar->FindTab(sourceAsset.GetId());
                }

                CloseScriptCanvasAsset(sourceAsset.GetId());
                CloseScriptCanvasAsset(saveAsset.GetId());
                auto openOutcome = OpenScriptCanvasAsset(saveAsset, saveTabIndex);
                if (openOutcome)
                {
                    QVariant data = m_tabBar->tabData(openOutcome.GetValue());
                    if (data.isValid())
                    {
                        auto graphMetadata = data.value<Widget::GraphTabMetadata>();
                        m_tabBar->SetTabText(openOutcome.GetValue(), tabName.data(), graphMetadata.m_fileState);
                    }
                }
            }
            if (saveCB)
            {
                saveCB(saveSuccess);
            }

            if (saveSuccess)
            {
                AddRecentFile(capturedFileName);
                UpdateRecentMenu();
            }
        };

        DocumentContextRequestBus::Broadcast(&DocumentContextRequests::SaveScriptCanvasAsset, filename, saveAsset, wrappedSaveCB, idChangedCB);
    }

    void MainWindow::OpenFile(const char* fullPath)
    {
        AZStd::string watchFolder;
        AZ::Data::AssetInfo assetInfo;
        bool sourceInfoFound{};
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, fullPath, assetInfo, watchFolder);
        if (sourceInfoFound && m_assetGraphSceneMapper.m_assetIdToDataMap.count(assetInfo.m_assetId) == 0)
        {
            SetRecentAssetId(assetInfo.m_assetId);

            AZ::Data::Asset<ScriptCanvasAsset> newAsset;
            DocumentContextRequestBus::BroadcastResult(newAsset, &DocumentContextRequests::LoadScriptCanvasAssetById, assetInfo.m_assetId, false);
            auto openOutcome = OpenScriptCanvasAsset(newAsset);
            if (!openOutcome)
            {
                AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
            }
        }
    }

    AZ::Data::Asset<ScriptCanvasAsset> MainWindow::CopyAssetForSave(const AZ::Data::AssetId& assetId, AZ::Data::Asset<ScriptCanvasAsset> oldAsset)
    {
        AZ::Data::Asset<ScriptCanvasAsset> newAsset = oldAsset;
        if (assetId != oldAsset.GetId())
        {
            newAsset = aznew ScriptCanvasAsset(assetId, AZ::Data::AssetData::AssetStatus::Ready);
            auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
            serializeContext->CloneObjectInplace(newAsset.Get()->GetScriptCanvasData(), &oldAsset.Get()->GetScriptCanvasData());
            AZStd::unordered_map<AZ::EntityId, AZ::EntityId> entityIdMap;
            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&newAsset.Get()->GetScriptCanvasData(), entityIdMap, serializeContext);
            AZStd::unordered_map<ScriptCanvas::VariableId, ScriptCanvas::VariableId> variableIdMap;
            AZ::IdUtils::Remapper<ScriptCanvas::VariableId>::GenerateNewIdsAndFixRefs(&newAsset.Get()->GetScriptCanvasData(), variableIdMap, serializeContext);
        }

        return newAsset;
    }

    void MainWindow::SourceFileChanged(const AZ::Data::AssetId& saveAssetId, AZStd::string relPath, AZStd::string scanFolder, const AZ::Data::AssetId& newAssetId)
    {
        if (saveAssetId == newAssetId)
        {
            return;
        }

        // Fixes up the asset Ids associated with an open Script Canvas asset 
        auto saveAssetMapper = m_assetGraphSceneMapper.GetByAssetId(saveAssetId);
        if (saveAssetMapper)
        {
            bool isActiveAsset = saveAssetId == m_activeAssetId;

            // Move over the old asset data to a new ScriptCanvasAsset with the newAssetId
            AZ::Data::Asset<ScriptCanvasAsset> remappedAsset = aznew ScriptCanvasAsset(newAssetId, saveAssetMapper->m_asset.GetStatus());
            remappedAsset.Get()->GetScriptCanvasData() = AZStd::move(saveAssetMapper->m_asset.Get()->GetScriptCanvasData());
            
            // Extracts the Scene UndoState from the Undo Manager before it can be deleted by RemoveScriptCanvasAsset
            // Removes the ScriptCanvasAsset from the MainWindow which disconnects all EBuses and unloads the scene which would delete the Scene UndoState
            AZStd::unique_ptr<SceneUndoState> sceneUndoState = m_undoManager->ExtractSceneUndoState(remappedAsset.Get()->GetScriptCanvasEntity()->GetId());
            RemoveScriptCanvasAsset(saveAssetId);
            m_undoManager->InsertUndoState(remappedAsset.Get()->GetScriptCanvasEntity()->GetId(), AZStd::move(sceneUndoState));

            ScriptCanvasAssetFileInfo assetFileInfo;
            AzFramework::StringFunc::Path::Join(scanFolder.data(), relPath.data(), assetFileInfo.m_absolutePath);
            assetFileInfo.m_fileModificationState = ScriptCanvasFileState::UNMODIFIED;
            MoveScriptCanvasAsset(remappedAsset, assetFileInfo);

            // Re-purpose tab index with the new asset data

            AZStd::string tabName;
            AzFramework::StringFunc::Path::GetFileName(assetFileInfo.m_absolutePath.data(), tabName);
            int saveTabIndex = m_tabBar->FindTab(saveAssetId);
            QVariant data = m_tabBar->tabData(saveTabIndex);
            if (data.isValid())
            {   
                auto graphMetadata = data.value<Widget::GraphTabMetadata>();
                graphMetadata.m_tabName = tabName.data();
                graphMetadata.m_assetId = remappedAsset.GetId();
                graphMetadata.m_fileState = assetFileInfo.m_fileModificationState;
                m_tabBar->SetGraphTabData(saveTabIndex, graphMetadata);
                m_tabBar->setTabToolTip(saveTabIndex, assetFileInfo.m_absolutePath.data());

                if (isActiveAsset)
                {
                    SetActiveAsset(newAssetId);
                }
            }
        }
    }

    void MainWindow::OnFileNew()
    {
        AZStd::string newAssetName = AZStd::string::format("Untitled-%i", ++s_scriptCanvasEditorDefaultNewNameCount);

        AZStd::array<char, AZ::IO::MaxPathLength> assetRootArray;
        if (!AZ::IO::FileIOBase::GetInstance()->ResolvePath("@devassets@/scriptcanvas", assetRootArray.data(), assetRootArray.size()))
        {
            AZ_ErrorOnce("Script Canvas", false, "Unable to resolve @devassets@ path");
        }

        AZStd::string assetPath;
        AzFramework::StringFunc::Path::Join(assetRootArray.data(), (newAssetName + "." + ScriptCanvasAssetHandler::GetFileExtension()).data(), assetPath);
        auto createOutcome = CreateScriptCanvasAsset(assetPath);
        if (createOutcome)
        {
            Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendMetric, ScriptCanvasEditor::Metrics::Events::Editor::NewFile);
        }
        else
        {
            AZ_Warning("Script Canvas", createOutcome, "%s", createOutcome.GetError().data());
        }
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
                PrepareActiveAssetForSave();

                auto assetMapper = m_assetGraphSceneMapper.GetByAssetId(m_activeAssetId);
                if (assetMapper && assetMapper->m_asset.IsReady())
                {
                    auto scriptCanvasAsset = assetMapper->m_asset;
                    AZ::Outcome<ScriptCanvasAssetFileInfo, AZStd::string> assetInfoOutcome = AZ::Failure(AZStd::string());
                    DocumentContextRequestBus::BroadcastResult(assetInfoOutcome, &DocumentContextRequests::GetFileInfo, scriptCanvasAsset.GetId());
                    if (assetInfoOutcome)
                    {
                        SaveScriptCanvasAsset(assetInfoOutcome.GetValue().m_absolutePath, scriptCanvasAsset, saveCB);
                        saveAttempt = true;
                    }
                    else
                    {
                        AZ_Warning("Script Canvas", assetInfoOutcome, "%s", assetInfoOutcome.GetError().data());
                    }
                }
            }
        }
        return saveAttempt;
    }

    bool MainWindow::OnFileSaveAs(const DocumentContextRequests::SaveCB& saveCB)
    {
        if (!m_activeAssetId.IsValid())
        {
            return false;
        }

        PrepareActiveAssetForSave();

        CreateSaveDestinationDirectory();
        AZStd::string suggestedFilename = GetSuggestedFullFilenameToSaveAs(m_activeAssetId);
        QString filter = tr("Script Canvas Files (%1)").arg(ScriptCanvasAssetHandler::GetFileFilter());
        QString selectedFile = QFileDialog::getSaveFileName(this, tr("Save As..."), suggestedFilename.data(), filter);
        if (!selectedFile.isEmpty())
        {
            auto assetMapper = m_assetGraphSceneMapper.GetByAssetId(m_activeAssetId);
            if (assetMapper && assetMapper->m_asset.IsReady())
            {
                auto scriptCanvasAsset = assetMapper->m_asset;
                SaveScriptCanvasAsset(selectedFile.toUtf8().data(), scriptCanvasAsset, saveCB);
                return true;
            }
        }

        return false;
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
        if (!assetPath.empty())
        {
            assetPath = AZStd::string::format("%s/%s", assetRoot.c_str(), assetPath.c_str());
        }

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
            AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
            bool originalSetting = settings && settings->m_showExcludedNodes;

            ScriptCanvasEditor::SettingsDialog(ui->action_GlobalPreferences->text(), AZ::EntityId(), this).exec();

            bool settingChanged = settings && settings->m_showExcludedNodes;

            if (originalSetting != settingChanged)
            {
                m_nodePalette->ResetModel();
                
                delete m_createNodeContextMenu;
                m_createNodeContextMenu = aznew CreateNodeContextMenu();

                SignalActiveSceneChanged(GetActiveScriptCanvasGraphId());
            }

        });

        connect(ui->action_GraphPreferences, &QAction::triggered, [this]() {
            AZ::EntityId scriptCanvasSceneId = GetActiveScriptCanvasGraphId();
            if (!scriptCanvasSceneId.IsValid())
            {
                return;
            }

            ScriptCanvasEditor::SettingsDialog(ui->action_GlobalPreferences->text(), scriptCanvasSceneId, this).exec();
        });
    }

    void MainWindow::OnEditMenuShow()
    {
        RefreshGraphPreferencesAction();
    }

    void MainWindow::RefreshPasteAction()
    {
        AZStd::string copyMimeType;
        GraphCanvas::SceneRequestBus::EventResult(copyMimeType, GetActiveGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetCopyMimeType);
        const bool pasteableClipboard = !copyMimeType.empty() && QApplication::clipboard()->mimeData()->hasFormat(copyMimeType.c_str());

        ui->action_Paste->setEnabled(pasteableClipboard);
    }

    void MainWindow::RefreshGraphPreferencesAction()
    {
        ui->action_GraphPreferences->setEnabled(GetActiveGraphCanvasGraphId().IsValid());
    }

    void MainWindow::OnEditCut()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::CutSelection);
    }

    void MainWindow::OnEditCopy()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::CopySelection);
    }

    void MainWindow::OnEditPaste()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Paste);
    }

    void MainWindow::OnEditDuplicate()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::DuplicateSelection);
    }

    void MainWindow::OnEditDelete()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::DeleteSelection);
    }

    void MainWindow::OnScriptCanvasAssetReady(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        CloneAssetEntity(scriptCanvasAsset);
    }

    void MainWindow::CloneAssetEntity(AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset)
    {
        // The ScriptCanvas Asset loaded in the Script Canvas Editor should not share the memory with the version in the AssetManager
        // This is to allow a modified asset in the SC Editor to close without modifying the canonical asset
        ScriptCanvasAsset* assetData = aznew ScriptCanvasAsset(scriptCanvasAsset.GetId(), AZ::Data::AssetData::AssetStatus::Ready);
        auto& scriptCanvasData = assetData->GetScriptCanvasData();

        // Clone asset data into SC Editor asset
        auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        serializeContext->CloneObjectInplace(scriptCanvasData, &scriptCanvasAsset.Get()->GetScriptCanvasData());
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> entityIdMap;
        AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&scriptCanvasData, entityIdMap, serializeContext);

        ActivateAssetEntity(assetData);
    }

    void MainWindow::ActivateAssetEntity(AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset)
    {
        if (AZ::Entity* scriptCanvasEntity{ scriptCanvasAsset.Get()->GetScriptCanvasEntity() })
        {
            if (scriptCanvasEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
            {
                scriptCanvasEntity->Init();
            }
            if (scriptCanvasEntity->GetState() == AZ::Entity::ES_INIT)
            {
                scriptCanvasEntity->Activate();
            }

            m_assetGraphSceneMapper.Add(scriptCanvasAsset);
            AssetGraphSceneData* assetMapper = m_assetGraphSceneMapper.GetByAssetId(scriptCanvasAsset.GetId());
            
            SetActiveAsset(scriptCanvasAsset.GetId());
            UpdateScriptCanvasAsset(scriptCanvasAsset);

            GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId(scriptCanvasEntity->GetId());
            GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::OnGraphLoaded, graphCanvasGraphId);

            UndoCache* undoCache = m_undoManager->GetSceneUndoCache(scriptCanvasEntity->GetId());
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

    AZ::EntityId MainWindow::GetActiveGraphCanvasGraphId() const
    {
        return GetGraphCanvasGraphId(GetActiveScriptCanvasGraphId());
    }

    AZ::EntityId MainWindow::GetActiveScriptCanvasGraphId() const
    {
        AssetGraphSceneData* assetGraphSceneData = m_assetGraphSceneMapper.GetByAssetId(m_activeAssetId);
        return assetGraphSceneData ? assetGraphSceneData->m_tupleId.m_sceneId : AZ::EntityId();
    }

    GraphCanvas::GraphId MainWindow::GetGraphCanvasGraphId(const AZ::EntityId& scriptCanvasGraphId) const
    {
        AZ::EntityId graphCanvasId;
        EditorGraphRequestBus::EventResult(graphCanvasId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

        return graphCanvasId;
    }

    AZ::EntityId MainWindow::GetScriptCanvasGraphId(const GraphCanvas::GraphId& graphCanvasGraphId) const
    {
        const AZStd::any* userData = nullptr;
        GraphCanvas::SceneRequestBus::EventResult(userData, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetUserDataConst);

        if (userData && userData->is<AZ::EntityId>())
        {
            return (*AZStd::any_cast<AZ::EntityId>(userData));
        }

        return AZ::EntityId();
    }

    bool MainWindow::IsInUndoRedo(const AZ::EntityId& graphCanvasGraphId) const
    {
        if (GetActiveGraphCanvasGraphId() == graphCanvasGraphId)
        {
            return m_undoManager->IsInUndoRedo();
        }

        return false;
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
                AZ_Error("Script Canvas", assetGraphSceneData, "Asset %s does not belong to this window", m_activeAssetId.ToString<AZStd::string>().data());
                return;
            }

            auto scriptCanvasAsset = assetGraphSceneData->m_asset;
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
                        m_layout->addWidget(hostMetadata.m_hostWidget);
                        hostMetadata.m_hostWidget->show();
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
                const AZ::Data::AssetId assetId = graphTabMetadata.m_assetId;
                auto saveCB = [this, assetId](bool isSuccessful)
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
                };
                SaveAsset(graphTabMetadata.m_assetId, saveCB);
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

    void MainWindow::OnNodePositionChanged(const AZ::EntityId& nodeId, const AZ::Vector2&)
    {
        m_propertyGrid->OnNodeUpdate(nodeId);
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

        if (m_minimap)
        {
            addDockWidget(Qt::LeftDockWidgetArea, m_minimap);
            m_minimap->setFloating(false);
            m_minimap->hide();
        }

        if (m_nodePalette)
        {
            addDockWidget(Qt::LeftDockWidgetArea, m_nodePalette);
            m_nodePalette->setFloating(false);
            m_nodePalette->show();
        }

        if (m_variableDockWidget)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_variableDockWidget);
            m_variableDockWidget->setFloating(false);
            m_variableDockWidget->show();
        }

        if (m_variableDockWidget)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_variableDockWidget);
            m_variableDockWidget->setFloating(false);
            m_variableDockWidget->show();
        }

        if (m_propertyGrid)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_propertyGrid);
            m_propertyGrid->setFloating(false);
            m_propertyGrid->show();
        }

        if (m_bookmarkDockWidget)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_bookmarkDockWidget);
            m_bookmarkDockWidget->setFloating(false);
            m_bookmarkDockWidget->hide();
        }

        if (m_nodeOutliner)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_nodeOutliner);
            m_nodeOutliner->setFloating(false);
            m_nodeOutliner->hide();
        }

        if (m_variableDockWidget)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_variableDockWidget);
            m_variableDockWidget->setFloating(false);
            m_variableDockWidget->show();
        }

        if (m_minimap)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_minimap);
            m_minimap->setFloating(false);
            m_minimap->hide();
        }

        // Disabled until debugger is implemented
        // addDockWidget(Qt::BottomDockWidgetArea, m_debugging);

        if (m_logPanel)
        {
            addDockWidget(Qt::BottomDockWidgetArea, m_logPanel);
            m_logPanel->setFloating(false);
            m_logPanel->hide();
        }

        resizeDocks(
        { m_nodePalette, m_propertyGrid },
        { static_cast<int>(size().width() * 0.15f), static_cast<int>(size().width() * 0.2f) },
            Qt::Horizontal);

        resizeDocks({ m_nodePalette, m_minimap },
        { static_cast<int>(size().height() * 0.70f), static_cast<int>(size().height() * 0.30f) },
            Qt::Vertical);

        resizeDocks({ m_propertyGrid, m_variableDockWidget },
        { static_cast<int>(size().height() * 0.70f), static_cast<int>(size().height() * 0.30f) },
            Qt::Vertical);

        // Disabled until debugger is implemented
        //resizeDocks({ m_logPanel }, { static_cast<int>(size().height() * 0.1f) }, Qt::Vertical);

        // Re-enable updates now that we've finished adjusting the layout
        setUpdatesEnabled(true);

        m_defaultLayout = saveState();

        UpdateViewMenu();
    }

    void MainWindow::RefreshSelection()
    {
        AZ::EntityId scriptCanvasGraphId = GetActiveScriptCanvasGraphId();

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

        bool hasCopiableSelection = false;
        bool hasSelection = false;

        if (m_activeAssetId.IsValid())
        {
            if (graphCanvasGraphId.IsValid())
            {
                // Get the selected nodes.
                GraphCanvas::SceneRequestBus::EventResult(hasCopiableSelection, graphCanvasGraphId, &GraphCanvas::SceneRequests::HasCopiableSelection);
            }

            AZStd::vector< AZ::EntityId > selection;
            GraphCanvas::SceneRequestBus::EventResult(selection, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetSelectedItems);

            if (!selection.empty())
            {
                hasSelection = true;
                m_propertyGrid->SetSelection(selection);
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

        // cut, copy and duplicate only works for specified items
        ui->action_Cut->setEnabled(hasCopiableSelection);
        ui->action_Copy->setEnabled(hasCopiableSelection);
        ui->action_Duplicate->setEnabled(hasCopiableSelection);

        // Delete will work for anything that is selectable
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

    void MainWindow::OnViewMiniMap()
    {
        if (m_minimap)
        {
            m_minimap->toggleViewAction()->trigger();
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

    void MainWindow::OnBookmarks()
    {
        if (m_bookmarkDockWidget)
        {
            m_bookmarkDockWidget->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnVariableManager()
    {
        if (m_variableDockWidget)
        {
            m_variableDockWidget->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnRestoreDefaultLayout()
    {
        if (!m_defaultLayout.isEmpty())
        {
            restoreState(m_defaultLayout);
            UpdateViewMenu();
        }
    }

    void MainWindow::UpdateViewMenu()
    {
        if (ui->action_ViewBookmarks->isChecked() != m_bookmarkDockWidget->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewBookmarks);

            ui->action_ViewBookmarks->setChecked(m_bookmarkDockWidget->isVisible());
        }

        if (ui->action_ViewMiniMap->isChecked() != m_minimap->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewMiniMap);

            ui->action_ViewMiniMap->setChecked(m_minimap->isVisible());
        }

        if (ui->action_ViewNodePalette->isChecked() != m_nodePalette->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewNodePalette);

            ui->action_ViewNodePalette->setChecked(m_nodePalette->isVisible());
        }

        if (ui->action_ViewOutline->isChecked() != m_nodeOutliner->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewOutline);

            ui->action_ViewOutline->setChecked(m_nodeOutliner->isVisible());
        }

        if (ui->action_ViewProperties->isChecked() != m_propertyGrid->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewProperties);

            ui->action_ViewProperties->setChecked(m_propertyGrid->isVisible());
        }

        if (ui->action_ViewVariableManager->isChecked() != m_variableDockWidget->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewVariableManager);

            ui->action_ViewVariableManager->setChecked(m_variableDockWidget->isVisible());
        }
    }

    void MainWindow::DeleteNodes(const AZ::EntityId& graphCanvasGraphId, const AZStd::vector<AZ::EntityId>& nodes)
    {
        // clear the selection then delete the nodes that were selected
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{ nodes.begin(), nodes.end() });
    }

    void MainWindow::DeleteConnections(const AZ::EntityId& graphCanvasGraphId, const AZStd::vector<AZ::EntityId>& connections)
    {
        ScopedVariableSetter<bool> scopedIgnoreSelection(m_ignoreSelection, true);
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{ connections.begin(), connections.end() });
    }

    void MainWindow::DisconnectEndpoints(const AZ::EntityId& graphCanvasGraphId, const AZStd::vector<GraphCanvas::Endpoint>& endpoints)
    {
        AZStd::unordered_set<AZ::EntityId> connections;
        for (const auto& endpoint : endpoints)
        {
            AZStd::vector<AZ::EntityId> endpointConnections;
            GraphCanvas::SceneRequestBus::EventResult(endpointConnections, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetConnectionsForEndpoint, endpoint);
            connections.insert(endpointConnections.begin(), endpointConnections.end());
        }
        DeleteConnections(graphCanvasGraphId, { connections.begin(), connections.end() });
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

            UpdateViewMenu();
        }
    }

    void MainWindow::RunBatchConversion()
    {
        if (m_converter == nullptr)
        {
            QFileDialog directoryDialog(this);
            directoryDialog.setFileMode(QFileDialog::FileMode::Directory);

            if (directoryDialog.exec())
            {
                QStringList directories = directoryDialog.selectedFiles();

                m_converter = aznew ScriptCanvasBatchConverter(this);
                m_converter->BatchConvert(directories);
            }
        }
    }

    void MainWindow::BatchConvertDirectory(QDir directory)
    {
    }

    void MainWindow::BatchConvertFile(const QString& fileName)
    {
    }

    NodeIdPair MainWindow::ProcessCreateNodeMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, const AZ::EntityId& graphCanvasGraphId, AZ::Vector2 nodeCreationPos)
    {
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

        NodeIdPair retVal;

        if (azrtti_istypeof<CreateNodeMimeEvent>(mimeEvent))
        {
            CreateNodeMimeEvent* createEvent = static_cast<CreateNodeMimeEvent*>(mimeEvent);            

            if (createEvent->ExecuteEvent(nodeCreationPos, nodeCreationPos, graphCanvasGraphId))
            {
                retVal = createEvent->GetCreatedPair();
            }
        }
        else if (azrtti_istypeof<SpecializedCreateNodeMimeEvent>(mimeEvent))
        {
            SpecializedCreateNodeMimeEvent* specializedCreationEvent = static_cast<SpecializedCreateNodeMimeEvent*>(mimeEvent);
            retVal = specializedCreationEvent->ConstructNode(graphCanvasGraphId, nodeCreationPos);
        }

        return retVal;
    }

    const GraphCanvas::GraphCanvasTreeItem* MainWindow::GetNodePaletteRoot() const
    {
        return m_nodePalette->GetTreeRoot();
    }

    GraphCanvas::Endpoint MainWindow::CreateNodeForProposal(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        PushPreventUndoStateUpdate();

        GraphCanvas::Endpoint retVal;

        AZ::EntityId graphCanvasGraphId = (*GraphCanvas::SceneUIRequestBus::GetCurrentBusId());

        m_createNodeContextMenu->FilterForSourceSlot(graphCanvasGraphId, endpoint.GetSlotId());
        m_createNodeContextMenu->RefreshActions(graphCanvasGraphId);
        m_createNodeContextMenu->DisableCreateActions();

        QAction* action = m_createNodeContextMenu->exec(screenPoint);

        // If the action returns null. We need to check if it was our widget, or just a close command.
        if (action == nullptr)
        {
            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_createNodeContextMenu->GetNodePalette()->GetContextMenuEvent();

            if (mimeEvent)
            {
                bool isValid = false;
                NodeIdPair finalNode = ProcessCreateNodeMimeEvent(mimeEvent, graphCanvasGraphId, AZ::Vector2(scenePoint.x(), scenePoint.y()));

                if (finalNode.m_graphCanvasId.IsValid())
                {
                    GraphCanvas::VisualRequestBus::Event(finalNode.m_graphCanvasId, &GraphCanvas::VisualRequests::SetVisible, false);

                    GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_Invalid;
                    GraphCanvas::SlotRequestBus::EventResult(connectionType, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetConnectionType);

                    // Create the node
                    AZStd::vector<AZ::EntityId> targetSlotIds;
                    GraphCanvas::NodeRequestBus::EventResult(targetSlotIds, finalNode.m_graphCanvasId, &GraphCanvas::NodeRequests::GetSlotIds);

                    AZStd::list< GraphCanvas::Endpoint > endpoints;

                    for (const auto& targetSlotId : targetSlotIds)
                    {
                        GraphCanvas::Endpoint proposedEndpoint(finalNode.m_graphCanvasId, targetSlotId);

                        bool canCreate = false;

                        if (connectionType == GraphCanvas::ConnectionType::CT_Output)
                        {
                            GraphCanvas::GraphModelRequestBus::EventResult(canCreate, graphCanvasGraphId, &GraphCanvas::GraphModelRequests::IsValidConnection, endpoint, proposedEndpoint);
                        }
                        else if (connectionType == GraphCanvas::ConnectionType::CT_Input)
                        {
                            GraphCanvas::GraphModelRequestBus::EventResult(canCreate, graphCanvasGraphId, &GraphCanvas::GraphModelRequests::IsValidConnection, proposedEndpoint, endpoint);
                        }

                        if (canCreate)
                        {
                            endpoints.push_back(proposedEndpoint);
                        }
                    }

                    if (!endpoints.empty())
                    {
                        if (endpoints.size() == 1)
                        {
                            retVal = endpoints.front();
                        }
                        else
                        {
                            QMenu menu;

                            for (GraphCanvas::Endpoint proposedEndpoint : endpoints)
                            {
                                QAction* action = aznew EndpointSelectionAction(proposedEndpoint);
                                menu.addAction(action);
                            }

                            QAction* result = menu.exec(screenPoint);

                            if (result != nullptr)
                            {
                                EndpointSelectionAction* selectedEnpointAction = static_cast<EndpointSelectionAction*>(result);
                                retVal = selectedEnpointAction->GetEndpoint();
                            }
                            else
                            {
                                retVal.Clear();
                            }
                        }

                        if (retVal.IsValid())
                        {
                            if (connectionType == GraphCanvas::ConnectionType::CT_Output)
                            {
                                GraphCanvas::GraphModelRequestBus::EventResult(isValid, graphCanvasGraphId, &GraphCanvas::GraphModelRequests::CreateConnection, connectionId, endpoint, retVal);
                            }
                            else if (connectionType == GraphCanvas::ConnectionType::CT_Input)
                            {
                                GraphCanvas::GraphModelRequestBus::EventResult(isValid, graphCanvasGraphId, &GraphCanvas::GraphModelRequests::CreateConnection, connectionId, retVal, endpoint);
                            }

                            if (!isValid)
                            {
                                retVal.Clear();
                            }
                        }
                    }

                    if (retVal.IsValid())
                    {
                        GraphCanvas::VisualRequestBus::Event(finalNode.m_graphCanvasId, &GraphCanvas::VisualRequests::SetVisible, true);

                        AZ::Vector2 position;
                        GraphCanvas::GeometryRequestBus::EventResult(position, retVal.GetNodeId(), &GraphCanvas::GeometryRequests::GetPosition);

                        QPointF connectionPoint;
                        GraphCanvas::SlotUIRequestBus::EventResult(connectionPoint, retVal.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);

                        qreal verticalOffset = connectionPoint.y() - position.GetY();
                        position.SetY(position.GetY() - verticalOffset);

                        GraphCanvas::GeometryRequestBus::Event(retVal.GetNodeId(), &GraphCanvas::GeometryRequests::SetPosition, position);

                        GraphCanvas::SceneNotificationBus::Event(graphCanvasGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
                    }
                    else
                    {
                        DeleteNodes(graphCanvasGraphId, AZStd::vector<AZ::EntityId>{finalNode.m_graphCanvasId});
                    }
                }
            }
        }

        PopPreventUndoStateUpdate();

        return retVal;
    }

    void MainWindow::OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        AZ::EntityId sceneId = (*GraphCanvas::SceneUIRequestBus::GetCurrentBusId());

        if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(wrapperNode) != nullptr)
        {
            m_ebusHandlerActionMenu->SetEbusHandlerNode(wrapperNode);

            // We don't care about the result, since the actions are done on demand with the menu
            m_ebusHandlerActionMenu->exec(screenPoint);
        }
        else if (ScriptCanvasWrapperNodeDescriptorRequestBus::FindFirstHandler(wrapperNode) != nullptr)
        {
            ScriptCanvasWrapperNodeDescriptorRequestBus::Event(wrapperNode, &ScriptCanvasWrapperNodeDescriptorRequests::OnWrapperAction, actionWidgetBoundingRect, scenePoint, screenPoint);
        }
    }

    AZ::EntityId MainWindow::CreateNewGraph()
    {
        AZ::EntityId graphId;

        OnFileNew();

        if (m_activeAssetId.IsValid())
        {
            graphId = GetActiveGraphCanvasGraphId();
        }

        return graphId;
    }

    void MainWindow::OnCommandStarted(AZ::Crc32)
    {
        PushPreventUndoStateUpdate();
    }

    void MainWindow::OnCommandFinished(AZ::Crc32)
    {
        PopPreventUndoStateUpdate();
    }

    void MainWindow::SignalConversionComplete()
    {
        delete m_converter;
        m_converter = nullptr;
    }

    void MainWindow::PrepareActiveAssetForSave()
    {
        AZ::EntityId scriptCanvasGraphId = GetActiveScriptCanvasGraphId();
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::GraphModelRequestBus::Event(graphCanvasGraphId, &GraphCanvas::GraphModelRequests::OnSaveDataDirtied, scriptCanvasGraphId);
        GraphCanvas::GraphModelRequestBus::Event(graphCanvasGraphId, &GraphCanvas::GraphModelRequests::OnSaveDataDirtied, graphCanvasGraphId);
    }

    double MainWindow::GetSnapDistance() const
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            return settings->m_snapDistance;
        }

        return 10.0;
    }

    bool MainWindow::IsBookmarkViewportControlEnabled() const
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            return settings->m_allowBookmarkViewpointControl;
        }

        return false;
    }

    bool MainWindow::IsDragNodeCouplingEnabled() const
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            return settings->m_enableNodeDragCoupling;
        }

        return false;
    }

    AZStd::chrono::milliseconds MainWindow::GetDragCouplingTime() const
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            return AZStd::chrono::milliseconds(settings->m_dragNodeCouplingTimeMS);
        }

        return AZStd::chrono::milliseconds(500);
    }

    bool MainWindow::IsDragConnectionSpliceEnabled() const
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            return settings->m_enableNodeDragConnectionSplicing;
        }

        return false;
    }

    AZStd::chrono::milliseconds MainWindow::GetDragConnectionSpliceTime() const
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            return AZStd::chrono::milliseconds(settings->m_dragNodeConnectionSplicingTimeMS);
        }

        return AZStd::chrono::milliseconds(500);
    }

    bool MainWindow::IsDropConnectionSpliceEnabled() const
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            return settings->m_enableNodeDropConnectionSplicing;
        }

        return false;
    }

    AZStd::chrono::milliseconds MainWindow::GetDropConnectionSpliceTime() const
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            return AZStd::chrono::milliseconds(settings->m_dropNodeConnectionSplicingTimeMS);
        }

        return AZStd::chrono::milliseconds(500);
    }

#include <Editor/View/Windows/MainWindow.moc>
}
