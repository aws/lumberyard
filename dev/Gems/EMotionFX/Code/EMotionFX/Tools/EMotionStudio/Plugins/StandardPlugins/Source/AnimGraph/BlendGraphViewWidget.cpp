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

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphNodeWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendSpace1DNodeWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendSpace2DNodeWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigateWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationHistory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationLinkWidget.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QKeyEvent>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>


namespace EMStudio
{   
    // constructor
    BlendGraphViewWidget::BlendGraphViewWidget(AnimGraphPlugin* plugin, QWidget* parentWidget)
        : QWidget(parentWidget)
        , mParentPlugin(plugin)
    {
        EMotionFX::ActorEditorRequestBus::Handler::BusConnect();
    }


    void BlendGraphViewWidget::Init(BlendGraphWidget* blendGraphWidget)
    {
        connect(&mParentPlugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &BlendGraphViewWidget::OnFocusChanged);
        connect(&mParentPlugin->GetAnimGraphModel().GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &BlendGraphViewWidget::UpdateSelection);
        connect(mParentPlugin->GetNavigationHistory(), &NavigationHistory::ChangedSteppingLimits, this, &BlendGraphViewWidget::UpdateNavigation);

        for (uint32 i = 0; i < NUM_OPTIONS; ++i)
        {
            mToolbarButtons[i] = nullptr;
            mActions[i] = nullptr;
        }

        // create the vertical layout with the menu and the graph widget as entries
        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        verticalLayout->setSpacing(0);
        verticalLayout->setMargin(2);

        // create the toolbar
        QWidget* toolbarWidget = new QWidget();
        mToolbarLayout = new QHBoxLayout();
        mToolbarLayout->setSpacing(6);
        mToolbarLayout->setContentsMargins(8, 2, 8, 2);
        toolbarWidget->setLayout(mToolbarLayout);
        toolbarWidget->setMinimumHeight(28);
        toolbarWidget->setObjectName("EMFXFauxToolbar");
        toolbarWidget->setStyleSheet("QWidget#EMFXFauxToolbar { border-bottom: 1px solid black; }");

        QMenu* fileMenu = nullptr;
        QMenu* selectionMenu = nullptr;
        QMenu* navigationMenu = nullptr;
        QSize buttonSize = QSize(24, 24);
        QSize dropdownButtonSize = QSize(30, 24);

        // file section
        CreateEntry(fileMenu, mToolbarLayout, "New", "Images/Icons/Plus.png", true, false, FILE_NEW, 0, false, false /*, QKeySequence::New*/);
        connect(mActions[FILE_NEW], &QAction::triggered, this, &BlendGraphViewWidget::OnCreateAnimGraph);

        QPushButton* openMenuButton = new QPushButton();
        mOpenMenu = new QMenu("&Open");
        openMenuButton->setMenu(mOpenMenu);
        openMenuButton->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.png"));
        openMenuButton->setToolTip("Open");
        openMenuButton->setMinimumSize(dropdownButtonSize);
        openMenuButton->setMaximumSize(dropdownButtonSize);
        openMenuButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        mToolbarLayout->addWidget(openMenuButton);
        openMenuButton->setIconSize(QSize(16, 16));
        openMenuButton->setObjectName("EMFXAnimGraphOpenButton");
        openMenuButton->setStyleSheet("QPushButton#EMFXAnimGraphOpenButton::menu-indicator \
                                      { \
                                        subcontrol-position: right bottom; \
                                        subcontrol-origin: padding; \
                                        left: +1px; \
                                        top: -4px; \
                                      } \
                                      QPushButton#EMFXAnimGraphOpenButton \
                                      { \
                                        border: 0px; \
                                      }");
        connect(mOpenMenu, &QMenu::aboutToShow, this, &BlendGraphViewWidget::BuildOpenMenu);
        BuildOpenMenu();

        QPushButton* saveMenuButton = new QPushButton();
        QMenu* saveMenu = new QMenu("&Save");
        saveMenuButton->setMenu(saveMenu);
        saveMenuButton->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Save.png"));
        saveMenuButton->setToolTip("Save");
        saveMenuButton->setMinimumSize(dropdownButtonSize);
        saveMenuButton->setMaximumSize(dropdownButtonSize);
        saveMenuButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        mToolbarLayout->addWidget(saveMenuButton);
        saveMenuButton->setIconSize(QSize(16, 16));
        saveMenuButton->setObjectName("EMFXAnimGraphSaveButton");
        saveMenuButton->setStyleSheet("QPushButton#EMFXAnimGraphSaveButton::menu-indicator \
                                      { \
                                        subcontrol-position: right bottom; \
                                        subcontrol-origin: padding; \
                                        left: +1px; \
                                        top: -4px; \
                                      } \
                                      QPushButton#EMFXAnimGraphSaveButton \
                                      { \
                                        border: 0px; \
                                      }");
        CreateEntry(saveMenu, mToolbarLayout, "Save", "", false, false, FILE_SAVE, 0, false);
        CreateEntry(saveMenu, mToolbarLayout, "Save As", "", false, false, FILE_SAVEAS, 0, false);
        connect(mActions[FILE_SAVEAS], &QAction::triggered, mParentPlugin, &AnimGraphPlugin::OnFileSaveAs);
        connect(mActions[FILE_SAVE], &QAction::triggered, mParentPlugin, &AnimGraphPlugin::OnFileSave);

        AddSeparator();
        // Activate Nodes Options
        CreateEntry(selectionMenu, mToolbarLayout, "Activate Animgraph/State", "Images/AnimGraphPLugin/Play.png", true, false, ACTIVATE_ANIMGRAPH, 0, false, false);
        connect(mActions[ACTIVATE_ANIMGRAPH], &QAction::triggered, this, &BlendGraphViewWidget::OnActivateAnimGraph);

        // Viz options
        AddSeparator();
        CreateEntry(selectionMenu, mToolbarLayout, "Zoom Selection", "Images/AnimGraphPlugin/ZoomSelected.png", true, false, SELECTION_ZOOMSELECTION, 0, false, false);
        connect(mActions[SELECTION_ZOOMSELECTION], &QAction::triggered, this, &BlendGraphViewWidget::ZoomSelected);

        // Create the viz settings dropdown button
        QPushButton* vizMenuButton = new QPushButton();
        QMenu* vizMenu = new QMenu("&Visualization");
        vizMenuButton->setMenu(vizMenu);
        vizMenuButton->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Visualization.png"));
        vizMenuButton->setToolTip("Visualization");
        vizMenuButton->setStyleSheet("border: 0px;");
        vizMenuButton->setMinimumSize(dropdownButtonSize);
        vizMenuButton->setMaximumSize(dropdownButtonSize);
        vizMenuButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        mToolbarLayout->addWidget(vizMenuButton);
        vizMenuButton->setIconSize(QSize(16, 16));
        vizMenuButton->setObjectName("EMFXAnimGraphVizButton");
        vizMenuButton->setStyleSheet("QPushButton#EMFXAnimGraphVizButton::menu-indicator \
                                      { \
                                        subcontrol-position: right bottom; \
                                        subcontrol-origin: padding; \
                                        left: +1px; \
                                        top: -4px; \
                                      } \
                                      QPushButton#EMFXAnimGraphVizButton \
                                      { \
                                        border: 0px; \
                                      }");

        mActions[VISUALIZATION_PLAYSPEEDS] = vizMenu->addAction("Display Play Speeds");
        mActions[VISUALIZATION_GLOBALWEIGHTS] = vizMenu->addAction("Display Global Weights");
        mActions[VISUALIZATION_SYNCSTATUS] = vizMenu->addAction("Display Sync Status");
        mActions[VISUALIZATION_PLAYPOSITIONS] = vizMenu->addAction("Display Play Positions");
        connect(mActions[VISUALIZATION_PLAYSPEEDS], &QAction::triggered, this, &BlendGraphViewWidget::OnDisplayPlaySpeeds);
        connect(mActions[VISUALIZATION_GLOBALWEIGHTS], &QAction::triggered, this, &BlendGraphViewWidget::OnDisplayGlobalWeights);
        connect(mActions[VISUALIZATION_SYNCSTATUS], &QAction::triggered, this, &BlendGraphViewWidget::OnDisplaySyncStatus);
        connect(mActions[VISUALIZATION_PLAYPOSITIONS], &QAction::triggered, this, &BlendGraphViewWidget::OnDisplayPlayPositions);

        AddSeparator();

        // Alignment Options
        CreateEntry(selectionMenu, mToolbarLayout, "Align Left", "Images/AnimGraphPlugin/AlignLeft.png", true, false, SELECTION_ALIGNLEFT, 0, false, false);
        CreateEntry(selectionMenu, mToolbarLayout, "Align Right", "Images/AnimGraphPlugin/AlignRight.png", true, false, SELECTION_ALIGNRIGHT, 0, false, false);
        CreateEntry(selectionMenu, mToolbarLayout, "Align Top", "Images/AnimGraphPlugin/AlignTop.png", true, false, SELECTION_ALIGNTOP, 0, false, false);
        CreateEntry(selectionMenu, mToolbarLayout, "Align Bottom", "Images/AnimGraphPlugin/AlignBottom.png", true, false, SELECTION_ALIGNBOTTOM, 0, false, false);
        connect(mActions[SELECTION_ALIGNLEFT], &QAction::triggered, this, &BlendGraphViewWidget::AlignLeft);
        connect(mActions[SELECTION_ALIGNRIGHT], &QAction::triggered, this, &BlendGraphViewWidget::AlignRight);
        connect(mActions[SELECTION_ALIGNTOP], &QAction::triggered, this, &BlendGraphViewWidget::AlignTop);
        connect(mActions[SELECTION_ALIGNBOTTOM], &QAction::triggered, this, &BlendGraphViewWidget::AlignBottom);

        QWidget* toolbarSpacerWidget = new QWidget();
        toolbarSpacerWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        mToolbarLayout->addWidget(toolbarSpacerWidget);

        /////////////////////////////////////////////////////////////////////////////////////
        // hierarchy navigation
        /////////////////////////////////////////////////////////////////////////////////////
        QHBoxLayout* navigationLayout = new QHBoxLayout();
        navigationLayout->setSizeConstraint(QLayout::SetMaximumSize);
        navigationLayout->setSpacing(2);
        navigationLayout->setMargin(0);

        // navigation section
        CreateEntry(navigationMenu, navigationLayout, "Back", "Images/AnimGraphPlugin/StepBack.png", true, false, NAVIGATION_BACK, 0, false, false);
        CreateEntry(navigationMenu, navigationLayout, "Forward", "Images/AnimGraphPlugin/StepForward.png", true, false, NAVIGATION_FORWARD, 0, false, false);
        connect(mActions[NAVIGATION_FORWARD], &QAction::triggered, mParentPlugin->GetNavigationHistory(), &NavigationHistory::StepForward);
        connect(mActions[NAVIGATION_BACK], &QAction::triggered, mParentPlugin->GetNavigationHistory(), &NavigationHistory::StepBackward);

        AddSeparator(navigationLayout);

        // add the hierarchy navigation
        mNavigationLink = new NavigationLinkWidget(mParentPlugin, this);
        mNavigationLink->setMinimumHeight(28);
        navigationLayout->addWidget(mNavigationLink);

        CreateEntry(navigationMenu, navigationLayout, "Toggle Navigation Pane", "Images/AnimGraphPlugin/HierarchyView.png", true, false, NAVIGATION_NAVPANETOGGLE, 0, false, false);
        connect(mActions[NAVIGATION_NAVPANETOGGLE], &QAction::triggered, this, &BlendGraphViewWidget::ToggleNavigationPane);

        // add the top widget with the menu and toolbar and the gl widget to the vertical layout
        verticalLayout->addWidget(toolbarWidget);
        verticalLayout->addLayout(navigationLayout);
        verticalLayout->addWidget(&mViewportStack);
        
        mViewportSplitter = new QSplitter(Qt::Horizontal, this);
        mViewportSplitter->addWidget(blendGraphWidget);
        mViewportSplitter->addWidget(mParentPlugin->GetNavigateWidget());
        mViewportSplitter->setCollapsible(0, false);
        QList<int> sizes = { this->width(), 0 };
        mViewportSplitter->setSizes(sizes);
        mViewportStack.addWidget(mViewportSplitter);
        
        UpdateNavigation();
        UpdateAnimGraphOptions();
        UpdateSelection();
    }


    BlendGraphViewWidget::~BlendGraphViewWidget()
    {
        for (auto entry : mNodeTypeToWidgetMap)
        {
            AnimGraphNodeWidget* widget = entry.second;
            delete widget;
        }
    }

    void BlendGraphViewWidget::UpdateNavigation()
    {
        mActions[NAVIGATION_BACK]->setEnabled(mParentPlugin->GetNavigationHistory()->CanStepBackward());
        mActions[NAVIGATION_FORWARD]->setEnabled(mParentPlugin->GetNavigationHistory()->CanStepForward());

        mToolbarButtons[NAVIGATION_BACK]->setEnabled(mParentPlugin->GetNavigationHistory()->CanStepBackward());
        mToolbarButtons[NAVIGATION_FORWARD]->setEnabled(mParentPlugin->GetNavigationHistory()->CanStepForward());
    }

    void BlendGraphViewWidget::UpdateAnimGraphOptions()
    {
        // get the anim graph that is currently selected in the resource widget
        EMotionFX::AnimGraph* animGraph = mParentPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            SetOptionEnabled(FILE_SAVE, false);
            SetOptionEnabled(FILE_SAVEAS, false);
        }
        else
        {
            SetOptionEnabled(FILE_SAVE, true);
            SetOptionEnabled(FILE_SAVEAS, true);
        }
    }

    void BlendGraphViewWidget::UpdateSelection()
    {
        // do we have any selection?
        const bool anySelection = mParentPlugin->GetAnimGraphModel().GetSelectionModel().hasSelection();
        SetOptionEnabled(SELECTION_ZOOMSELECTION, anySelection);
        
        QModelIndex firstSelectedNode;
        bool atLeastTwoNodes = false;
        const QModelIndexList selectedIndexes = mParentPlugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        for (const QModelIndex& selected : selectedIndexes)
        {
            const AnimGraphModel::ModelItemType itemType = selected.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
            if (itemType == AnimGraphModel::ModelItemType::NODE)
            {
                if (firstSelectedNode.isValid())
                {
                    atLeastTwoNodes = true;
                    break;
                }
                else
                {
                    firstSelectedNode = selected;
                }
            }
        }

        if (atLeastTwoNodes)
        {
            SetOptionEnabled(SELECTION_ALIGNLEFT, true);
            SetOptionEnabled(SELECTION_ALIGNRIGHT, true);
            SetOptionEnabled(SELECTION_ALIGNTOP, true);
            SetOptionEnabled(SELECTION_ALIGNBOTTOM, true);
        }
        else
        {
            SetOptionEnabled(SELECTION_ALIGNLEFT, false);
            SetOptionEnabled(SELECTION_ALIGNRIGHT, false);
            SetOptionEnabled(SELECTION_ALIGNTOP, false);
            SetOptionEnabled(SELECTION_ALIGNBOTTOM, false);
        }
    }

    AnimGraphNodeWidget* BlendGraphViewWidget::GetWidgetForNode(const EMotionFX::AnimGraphNode* node)
    {
        if (!node)
        {
            return nullptr;
        }

        const AZ::TypeId nodeType = azrtti_typeid(node);
        AnimGraphNodeWidget* widget = nullptr;

        AZStd::unordered_map<AZ::TypeId, AnimGraphNodeWidget*>::iterator it =
            mNodeTypeToWidgetMap.find(nodeType);
        if (it != mNodeTypeToWidgetMap.end())
        {
            widget = it->second;
        }
        else
        {
            if (nodeType == azrtti_typeid<EMotionFX::BlendSpace2DNode>())
            {
                widget = new BlendSpace2DNodeWidget(mParentPlugin);
                mNodeTypeToWidgetMap[nodeType] = widget;
            }
            else if (nodeType == azrtti_typeid<EMotionFX::BlendSpace1DNode>())
            {
                widget = new BlendSpace1DNodeWidget(mParentPlugin);
                mNodeTypeToWidgetMap[nodeType] = widget;
            }
        }
        return widget;
    }

    void BlendGraphViewWidget::OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent)
    {
        if (newFocusParent == oldFocusParent)
        {
            // not interested if the parent didn't change
            return;
        }

        // Reset the viewports to avoid dangling pointers.
        for (auto& item : mNodeTypeToWidgetMap)
        {
            AnimGraphNodeWidget* viewport = item.second;
            viewport->SetCurrentNode(nullptr);
        }

        if (newFocusParent.isValid())
        {
            EMotionFX::AnimGraphNode* node = newFocusParent.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            AnimGraphNodeWidget* widget = GetWidgetForNode(node);
            if (widget)
            {
                int index = mViewportStack.indexOf(widget);
                if (index == -1)
                {
                    mViewportStack.addWidget(widget);
                    mViewportStack.setCurrentIndex(mViewportStack.count() - 1);
                }
                else
                {
                    mViewportStack.setCurrentIndex(index);
                }

                widget->SetCurrentNode(node);
                widget->SetCurrentModelIndex(newFocusParent);
            }
            else
            {
                // Show the default widget.
                mViewportStack.setCurrentIndex(0);
            }
        }
    }
    
    void BlendGraphViewWidget::SetOptionFlag(EOptionFlag option, bool isEnabled)
    {
        const uint32 optionIndex = (uint32)option;

        if (mToolbarButtons[optionIndex])
        {
            mToolbarButtons[optionIndex]->setChecked(isEnabled);
        }

        if (mActions[optionIndex])
        {
            mActions[optionIndex]->setChecked(isEnabled);
        }
    }


    void BlendGraphViewWidget::SetOptionEnabled(EOptionFlag option, bool isEnabled)
    {
        const uint32 optionIndex = (uint32)option;

        if (mToolbarButtons[optionIndex])
        {
            mToolbarButtons[optionIndex]->setEnabled(isEnabled);
        }

        if (mActions[optionIndex])
        {
            mActions[optionIndex]->setEnabled(isEnabled);
        }
    }


    void BlendGraphViewWidget::CreateEntry(QMenu* menu, QHBoxLayout* toolbarLayout, const char* entryName, const char* toolbarIconFileName, bool addToToolbar, bool checkable, int32 actionIndex, const QKeySequence& shortcut, bool border, bool addToMenu)
    {
        // menu entry
        if (addToMenu)
        {
            mActions[actionIndex] = menu->addAction(entryName);
            mActions[actionIndex]->setVisible(addToMenu);
            mActions[actionIndex]->setCheckable(checkable);
            mActions[actionIndex]->setShortcut(shortcut);
        }
        else
        {
            mActions[actionIndex] = new QAction(entryName, this);
        }

        if (strcmp(toolbarIconFileName, "") != 0 && checkable == false)
        {
            mActions[actionIndex]->setIcon(MysticQt::GetMysticQt()->FindIcon(toolbarIconFileName));
        }

        // toolbar button
        if (addToToolbar && strcmp(toolbarIconFileName, "") != 0)
        {
            QPushButton* toolbarButton = new QPushButton();
            mToolbarButtons[actionIndex] = toolbarButton;
            toolbarButton->setIcon(MysticQt::GetMysticQt()->FindIcon(toolbarIconFileName));
            toolbarButton->setCheckable(checkable);
            toolbarButton->setToolTip(entryName);

            if (border == false)
            {
                toolbarButton->setStyleSheet("border: 0px;");
            }

            QSize buttonSize = QSize(24, 24);
            toolbarButton->setMinimumSize(buttonSize);
            toolbarButton->setMaximumSize(buttonSize);
            toolbarButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

            toolbarLayout->addWidget(toolbarButton);
            toolbarButton->setIconSize(buttonSize - QSize(2, 2));

            // connect the menu entry with the toolbar button
            connect(toolbarButton, &QPushButton::clicked, mActions[actionIndex], &QAction::trigger);
        }
    }

    void BlendGraphViewWidget::BuildOpenMenu()
    {        
        mOpenMenu->clear();

        mActions[FILE_OPEN] = mOpenMenu->addAction("Open...");
        connect(mActions[FILE_OPEN], &QAction::triggered, mParentPlugin, &AnimGraphPlugin::OnFileOpen);

        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        if (numAnimGraphs > 0)
        {
            mOpenMenu->addSeparator();
            for (uint32 i = 0; i < numAnimGraphs; ++i)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
                if (animGraph->GetIsOwnedByRuntime() == false)
                {
                    QString itemName = "<Unsaved Animgraph>";
                    if (!animGraph->GetFileNameString().empty())
                    {
                        // convert full absolute paths to friendlier relative paths + folder they're found in.
                        // GetSourceInfoBySourcePath works on relative paths and absolute paths and doesn't need to wait for
                        // cached products to exist in order to function, so it is orders of magnitude faster than asking about product files.
                        itemName = QString::fromUtf8(animGraph->GetFileName());
                        bool success = false;
                        AZStd::string watchFolder;
                        AZ::Data::AssetInfo assetInfo;
                        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, animGraph->GetFileName(), assetInfo, watchFolder);
                        if (success)
                        {
                            itemName = QString::fromUtf8(assetInfo.m_relativePath.c_str());
                        }
                    }
                    QAction* openItem = mOpenMenu->addAction(itemName);
                    connect(openItem, &QAction::triggered, [this, animGraph]() { OpenAnimGraph(animGraph); });
                    openItem->setData(animGraph->GetID());
                }
            }
        }
    }

    void BlendGraphViewWidget::OpenAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        if (animGraph)
        {
            EMotionFX::MotionSet* motionSet = nullptr;
            EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
            mParentPlugin->GetActionManager().ActivateGraphForSelectedActors(animGraph, motionSet);
        }
    }

    void BlendGraphViewWidget::AddSeparator(QLayout* layout)
    {
        if (layout == nullptr)
        {
            layout = mToolbarLayout;
        }

        QLabel* spacer = new QLabel();

        QSize spacerSize = QSize(2, 18);
        spacer->setMinimumSize(spacerSize);
        spacer->setMaximumSize(spacerSize);
        QPixmap img = MysticQt::GetMysticQt()->FindIcon("Images/Icons/divider.png").pixmap(spacerSize);
        spacer->setPixmap(img);
        spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        layout->addWidget(spacer);
    }


    void BlendGraphViewWidget::AlignNodes(uint32 mode)
    {
        NodeGraph* nodeGraph = mParentPlugin->GetGraphWidget()->GetActiveGraph();
        if (!nodeGraph)
        {
            return;
        }

        int32 alignedXPos = 0;
        int32 alignedYPos = 0;
        int32 maxGraphNodeHeight = 0;
        int32 maxGraphNodeWidth = 0;

        // iterate over all graph nodes
        bool firstSelectedNode = true;
        const QModelIndexList selectedItems = mParentPlugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        AZStd::vector<GraphNode*> alignedGraphNodes;

        for (const QModelIndex& selected : selectedItems)
        {
            if (selected.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE)
            {
                GraphNode* graphNode = nodeGraph->FindGraphNode(selected);
                if (graphNode) // otherwise it does not belong to the current active graph
                {
                    alignedGraphNodes.push_back(graphNode);

                    EMotionFX::AnimGraphNode* animGraphNode = selected.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                    const int32 xPos = animGraphNode->GetVisualPosX();
                    const int32 yPos = animGraphNode->GetVisualPosY();
                    const int32 graphNodeHeight = graphNode->CalcRequiredHeight();
                    const int32 graphNodeWidth = graphNode->CalcRequiredWidth();

                    if (firstSelectedNode)
                    {
                        alignedXPos = xPos;
                        alignedYPos = yPos;
                        maxGraphNodeHeight = graphNodeHeight;
                        maxGraphNodeWidth = graphNodeWidth;
                        firstSelectedNode = false;
                    }

                    if (graphNodeHeight > maxGraphNodeHeight)
                    {
                        maxGraphNodeHeight = graphNodeHeight;
                    }

                    if (graphNodeWidth > maxGraphNodeWidth)
                    {
                        maxGraphNodeWidth = graphNodeWidth;
                    }

                    switch (mode)
                    {
                    case SELECTION_ALIGNLEFT:
                    {
                        if (xPos < alignedXPos)
                        {
                            alignedXPos = xPos;
                        }
                        break;
                    }
                    case SELECTION_ALIGNRIGHT:
                    {
                        if (xPos + graphNodeWidth > alignedXPos)
                        {
                            alignedXPos = xPos + graphNodeWidth;
                        }
                        break;
                    }
                    case SELECTION_ALIGNTOP:
                    {
                        if (yPos < alignedYPos)
                        {
                            alignedYPos = yPos;
                        }
                        break;
                    }
                    case SELECTION_ALIGNBOTTOM:
                    {
                        if (yPos + graphNodeHeight > alignedYPos)
                        {
                            alignedYPos = yPos + graphNodeHeight;
                        }
                        break;
                    }
                    default:
                        MCORE_ASSERT(false);
                    }
                }
            }
        }

        if (alignedGraphNodes.size() > 1)
        {
            // create the command group
            AZStd::string command;
            AZStd::string outResult;
            MCore::CommandGroup commandGroup("Align anim graph nodes");

            const QModelIndex& modelIndex = nodeGraph->GetModelIndex();
            EMotionFX::AnimGraphNode* parentNode = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            AZ_Assert(parentNode, "Expected the parent to be a node");
            EMotionFX::AnimGraph* animGraph = parentNode->GetAnimGraph();

            // iterate over all nodes
            for (GraphNode* node : alignedGraphNodes)
            {
                command = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" ", animGraph->GetID(), node->GetName());

                switch (mode)
                {
                case SELECTION_ALIGNLEFT:
                    command += AZStd::string::format("-xPos %i", alignedXPos);
                    break;
                case SELECTION_ALIGNRIGHT:
                    command += AZStd::string::format("-xPos %i", alignedXPos - node->CalcRequiredWidth());
                    break;
                case SELECTION_ALIGNTOP:
                    command += AZStd::string::format("-yPos %i", alignedYPos);
                    break;
                case SELECTION_ALIGNBOTTOM:
                    command += AZStd::string::format("-yPos %i", alignedYPos - node->CalcRequiredHeight());
                    break;
                default:
                    MCORE_ASSERT(false);
                }

                commandGroup.AddCommandString(command);
            }

            // execute the group command
            GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult);
        }
        
    }

    void BlendGraphViewWidget::OnCreateAnimGraph()
    {
        // get the current selection list and the number of actor instances selected
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

        // Activate the new anim graph automatically (The shown anim graph should always be the activated one).
        if (numActorInstances > 0)
        {
            // create the command group
            MCore::CommandGroup commandGroup("Create an anim graph");

            // add the create anim graph command
            commandGroup.AddCommandString("CreateAnimGraph");

            // get the correct motion set
            // nullptr can only be <no motion set> because it's the first anim graph so no one is activated
            // if no motion set selected but one is possible, use the first possible
            // if no motion set selected and no one created, use no motion set
            // if one already selected, use the already selected
            EMotionFX::MotionSet* motionSet = nullptr;
            EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
            if (!motionSet)
            {
                if (EMotionFX::GetMotionManager().GetNumMotionSets() > 0)
                {
                    motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);
                }
            }

            if (motionSet)
            {
                // Activate anim graph on all actor instances in case there is a motion set.
                for (uint32 i = 0; i < numActorInstances; ++i)
                {
                    EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                    commandGroup.AddCommandString(AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %%LASTRESULT%% -motionSetID %d", actorInstance->GetID(), motionSet->GetID()));
                }
            }

            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
        else
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommand("CreateAnimGraph", result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }

    void BlendGraphViewWidget::AlignLeft()          { AlignNodes(SELECTION_ALIGNLEFT); }
    void BlendGraphViewWidget::AlignRight()         { AlignNodes(SELECTION_ALIGNRIGHT); }
    void BlendGraphViewWidget::AlignTop()           { AlignNodes(SELECTION_ALIGNTOP); }
    void BlendGraphViewWidget::AlignBottom()        { AlignNodes(SELECTION_ALIGNBOTTOM); }

    void BlendGraphViewWidget::ZoomSelected()
    { 
        BlendGraphWidget* blendGraphWidget = mParentPlugin->GetGraphWidget();
        if (blendGraphWidget)
        {
            NodeGraph* nodeGraph = blendGraphWidget->GetActiveGraph();
            if (nodeGraph)
            {
                // try zooming on the selection rect
                const QRect selectionRect = nodeGraph->CalcRectFromSelection(true);
                if (selectionRect.isEmpty() == false)
                {
                    nodeGraph->ZoomOnRect(selectionRect, geometry().width(), blendGraphWidget->geometry().height(), true);
                }
                else // zoom on the full scene
                {
                    nodeGraph->FitGraphOnScreen(blendGraphWidget->geometry().width(), blendGraphWidget->geometry().height(), blendGraphWidget->GetMousePos(), true);
                }
            }
        }
    }

    void BlendGraphViewWidget::OnActivateAnimGraph()
    {
        // Activate the anim graph for visual feedback of button click.
        EMotionFX::MotionSet* motionSet = nullptr;
        EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
        if (!motionSet)
        {
            // In case no motion set was selected yet, use the first available. The activate graph callback will update the UI.
            const AZ::u32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (AZ::u32 i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* currentMotionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
                if (!currentMotionSet->GetIsOwnedByRuntime())
                {
                    motionSet = currentMotionSet;
                    break;
                }
            }
        }

        EMotionFX::AnimGraph* animGraph = mParentPlugin->GetAnimGraphModel().GetFocusedAnimGraph();
        if (animGraph)
        {
            mParentPlugin->GetActionManager().ActivateGraphForSelectedActors(animGraph, motionSet);
        }
    }

    void BlendGraphViewWidget::OnActivateState()
    {
        // Transition to the selected state.
        const QModelIndexList currentModelIndexes = mParentPlugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (!currentModelIndexes.empty())
        {
            const QModelIndex currentModelIndex = currentModelIndexes.front();
            const AnimGraphModel::ModelItemType itemType = currentModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
            if (itemType == AnimGraphModel::ModelItemType::NODE)
            {
                EMotionFX::AnimGraphNode* selectedNode = currentModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                EMotionFX::AnimGraphNode* parentNode = selectedNode->GetParentNode();
                if (parentNode && azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                {
                    EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                    EMotionFX::AnimGraphInstance* animGraphInstance = currentModelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
                    if (animGraphInstance)
                    {
                        stateMachine->TransitionToState(animGraphInstance, selectedNode);
                    }
                }
            }
        }
    }


    void BlendGraphViewWidget::NavigateToRoot()
    {
        const QModelIndex nodeModelIndex = mParentPlugin->GetGraphWidget()->GetActiveGraph()->GetModelIndex();
        if (nodeModelIndex.isValid())
        {
            mParentPlugin->GetAnimGraphModel().Focus(nodeModelIndex);
        }
    }


    void BlendGraphViewWidget::NavigateToParent()
    {
        const QModelIndex parentFocus = mParentPlugin->GetAnimGraphModel().GetParentFocus();
        if (parentFocus.isValid())
        {
            QModelIndex newParentFocus = parentFocus.model()->parent(parentFocus);
            if (newParentFocus.isValid())
            {
                mParentPlugin->GetAnimGraphModel().Focus(newParentFocus);
            }
        }
    }

    void BlendGraphViewWidget::ToggleNavigationPane()
    {        
        QList<int> sizes = mViewportSplitter->sizes(); 
        if (sizes[1] == 0)
        {
            // the nav pane is hidden if the width is 0, so set the width to 25%
            sizes[0] = (this->width() * 75) / 100;
            sizes[1] = this->width() - sizes[0];
        }
        else
        {
            // hide the nav pane
            sizes[0] = this->width();
            sizes[1] = 0;
        }
        mViewportSplitter->setSizes(sizes);
    }

    void BlendGraphViewWidget::NavigateToNode()
    {
        const QModelIndexList currentModelIndexes = mParentPlugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (!currentModelIndexes.empty())
        {
            const QModelIndex currentModelIndex = currentModelIndexes.front();
            mParentPlugin->GetAnimGraphModel().Focus(currentModelIndex);
        }
    }
    
    // toggle playspeed viz
    void BlendGraphViewWidget::OnDisplayPlaySpeeds()
    {
        const bool showPlaySpeeds = !mParentPlugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYSPEED);
        mParentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYSPEED, showPlaySpeeds);
        SetOptionFlag(VISUALIZATION_PLAYSPEEDS, showPlaySpeeds);
    }


    // toggle sync status viz
    void BlendGraphViewWidget::OnDisplaySyncStatus()
    {
        const bool show = !mParentPlugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_SYNCSTATUS);
        mParentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_SYNCSTATUS, show);
        SetOptionFlag(VISUALIZATION_SYNCSTATUS, show);
    }


    // toggle global weights viz
    void BlendGraphViewWidget::OnDisplayGlobalWeights()
    {
        const bool displayGlobalWeights = !mParentPlugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_GLOBALWEIGHT);
        mParentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_GLOBALWEIGHT, displayGlobalWeights);
        SetOptionFlag(VISUALIZATION_GLOBALWEIGHTS, displayGlobalWeights);
    }


    // toggle play position viz
    void BlendGraphViewWidget::OnDisplayPlayPositions()
    {
        const bool show = !mParentPlugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYPOSITION);
        mParentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYPOSITION, show);
        SetOptionFlag(VISUALIZATION_PLAYPOSITIONS, show);
    }


    // on keypress
    void BlendGraphViewWidget::keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Backspace:
        {
            mParentPlugin->GetNavigationHistory()->StepBackward();
            event->accept();
            break;
        }

        default:
            event->ignore();
        }
    }


    // on key release
    void BlendGraphViewWidget::keyReleaseEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Backspace:
        {
            event->accept();
            break;
        }

        default:
            event->ignore();
        }
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.moc>
