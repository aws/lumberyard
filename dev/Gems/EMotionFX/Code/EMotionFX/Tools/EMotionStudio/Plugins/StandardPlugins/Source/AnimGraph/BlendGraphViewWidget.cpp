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

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <Editor/AnimGraphEditorBus.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "BlendGraphWidget.h"
#include "BlendGraphViewWidget.h"
#include "NodeGroupWindow.h"
#include "BlendSpace2DNodeWidget.h"
#include "BlendSpace1DNodeWidget.h"
#include "AnimGraphPlugin.h"
#include "AttributesWindow.h"
#include "NavigateWidget.h"
#include "ParameterWindow.h"
#include "GraphNode.h"
#include "NodePaletteWidget.h"

// qt includes
#include <QComboBox>
#include <QDropEvent>
#include <QLabel>
#include <QMenuBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QIcon>
#include <QSettings>
#include <QDropEvent>
#include <QSplitter>

// emfx and core includes
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSystem.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

// emstudio SDK
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"


namespace EMStudio
{   
    // constructor
    BlendGraphViewWidget::BlendGraphViewWidget(AnimGraphPlugin* plugin, QWidget* parentWidget)
        : QWidget(parentWidget)
    {
        mSelectedAnimGraphID = MCORE_INVALIDINDEX32;
        mParentPlugin = plugin;

        // create the command callbacks and register them
        mSaveAnimGraphCallback = new CommandSaveAnimGraphCallback(true);
        mCreateAnimGraphCallback = new CommandCreateAnimGraphCallback(true);
        mRemoveAnimGraphCallback = new CommandRemoveAnimGraphCallback(true);
        mSelectCallback = new CommandSelectCallback(false);
        mUnselectCallback = new CommandUnselectCallback(false);
        mClearSelectionCallback = new CommandClearSelectionCallback(false);
        mCreateMotionSetCallback = new CommandCreateMotionSetCallback(false);
        mRemoveMotionSetCallback = new CommandRemoveMotionSetCallback(false);
        mAdjustMotionSetCallback = new CommandAdjustMotionSetCallback(false);
        mSaveMotionSetCallback = new CommandSaveMotionSetCallback(false);
        mLoadMotionSetCallback = new CommandLoadMotionSetCallback(false);
        mActivateAnimGraphCallback = new CommandActivateAnimGraphCallback(false);
        mAnimGraphAddConditionCallback = new CommandAnimGraphAddConditionCallback(false);
        mAnimGraphRemoveConditionCallback = new CommandAnimGraphRemoveConditionCallback(false);
        mAnimGraphCreateConnectionCallback = new CommandAnimGraphCreateConnectionCallback(false);
        mAnimGraphRemoveConnectionCallback = new CommandAnimGraphRemoveConnectionCallback(false);
        mAnimGraphAdjustConnectionCallback = new CommandAnimGraphAdjustConnectionCallback(false);
        mAnimGraphCreateNodeCallback = new CommandAnimGraphCreateNodeCallback(false);
        mAnimGraphAdjustNodeCallback = new CommandAnimGraphAdjustNodeCallback(false);
        mAnimGraphRemoveNodeCallback = new CommandAnimGraphRemoveNodeCallback(false);
        mAnimGraphSetEntryStateCallback = new CommandAnimGraphSetEntryStateCallback(false);
        mAnimGraphAdjustNodeGroupCallback = new CommandAnimGraphAdjustNodeGroupCallback(false);
        mAnimGraphAddNodeGroupCallback = new CommandAnimGraphAddNodeGroupCallback(false);
        mAnimGraphRemoveNodeGroupCallback = new CommandAnimGraphRemoveNodeGroupCallback(false);
        mAnimGraphCreateParameterCallback = new CommandAnimGraphCreateParameterCallback(false);
        mAnimGraphRemoveParameterCallback = new CommandAnimGraphRemoveParameterCallback(false);
        mAnimGraphAdjustParameterCallback = new CommandAnimGraphAdjustParameterCallback(false);
        mAnimGraphSwapParametersCallback = new CommandAnimGraphSwapParametersCallback(false);
        mAnimGraphAdjustParameterGroupCallback = new CommandAnimGraphAdjustParameterGroupCallback(false);
        mAnimGraphAddParameterGroupCallback = new CommandAnimGraphAddParameterGroupCallback(false);
        mAnimGraphRemoveParameterGroupCallback = new CommandAnimGraphRemoveParameterGroupCallback(false);
        GetCommandManager()->RegisterCommandCallback("SaveAnimGraph", mSaveAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("CreateAnimGraph", mCreateAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveAnimGraph", mRemoveAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("CreateMotionSet", mCreateMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveMotionSet", mRemoveMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustMotionSet", mAdjustMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("SaveMotionSet", mSaveMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("LoadMotionSet", mLoadMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("ActivateAnimGraph", mActivateAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddCondition", mAnimGraphAddConditionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveCondition", mAnimGraphRemoveConditionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateConnection", mAnimGraphCreateConnectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveConnection", mAnimGraphRemoveConnectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustConnection", mAnimGraphAdjustConnectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateNode", mAnimGraphCreateNodeCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustNode", mAnimGraphAdjustNodeCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNode", mAnimGraphRemoveNodeCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphSetEntryState", mAnimGraphSetEntryStateCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustNodeGroup", mAnimGraphAdjustNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddNodeGroup", mAnimGraphAddNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNodeGroup", mAnimGraphRemoveNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateParameter", mAnimGraphCreateParameterCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameter", mAnimGraphRemoveParameterCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameter", mAnimGraphAdjustParameterCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphSwapParameters", mAnimGraphSwapParametersCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameterGroup", mAnimGraphAdjustParameterGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddParameterGroup", mAnimGraphAddParameterGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameterGroup", mAnimGraphRemoveParameterGroupCallback);

        EMotionFX::ActorEditorRequestBus::Handler::BusConnect();
    }


    void BlendGraphViewWidget::Init(BlendGraphWidget* blendGraphWidget)
    {
        NavigateWidget*     navigateWidget = mParentPlugin->GetNavigateWidget();
        BlendGraphWidget*   graphWidget = mParentPlugin->GetGraphWidget();

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
        connect(mActions[FILE_NEW], SIGNAL(triggered()), this, SLOT(OnCreateAnimGraph()));

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
        connect(mActions[FILE_OPEN], &QAction::triggered, mParentPlugin, &AnimGraphPlugin::OnFileOpen);
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
        connect(mActions[FILE_SAVEAS], SIGNAL(triggered()), mParentPlugin, SLOT(OnFileSaveAs()));
        connect(mActions[FILE_SAVE], SIGNAL(triggered()), mParentPlugin, SLOT(OnFileSave()));

        AddSeparator();
        // Activate Nodes Options
        CreateEntry(selectionMenu, mToolbarLayout, "Activate Animgraph/State", "Images/AnimGraphPLugin/Play.png", true, false, SELECTION_ACTIVATESTATE, 0, false, false);
        connect(mActions[SELECTION_ACTIVATESTATE], SIGNAL(triggered()), this, SLOT(OnActivateState()));

        // Viz options
        AddSeparator();
        CreateEntry(selectionMenu, mToolbarLayout, "Zoom Selection", "Images/AnimGraphPlugin/ZoomSelected.png", true, false, SELECTION_ZOOMSELECTION, 0, false, false);
        connect(mActions[SELECTION_ZOOMSELECTION], SIGNAL(triggered()), this, SLOT(ZoomSelected()));

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
        connect(mActions[VISUALIZATION_PLAYSPEEDS], SIGNAL(triggered()), this, SLOT(OnDisplayPlaySpeeds()));
        connect(mActions[VISUALIZATION_GLOBALWEIGHTS], SIGNAL(triggered()), this, SLOT(OnDisplayGlobalWeights()));
        connect(mActions[VISUALIZATION_SYNCSTATUS], SIGNAL(triggered()), this, SLOT(OnDisplaySyncStatus()));
        connect(mActions[VISUALIZATION_PLAYPOSITIONS], SIGNAL(triggered()), this, SLOT(OnDisplayPlayPositions()));

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
        connect(mActions[NAVIGATION_FORWARD], SIGNAL(triggered()), this, SLOT(NavigateForward()));
        connect(mActions[NAVIGATION_BACK], SIGNAL(triggered()), this, SLOT(NavigateBackward()));

        AddSeparator(navigationLayout);

        // add the hierarchy navigation
        mNavigationLink = new NavigationLinkWidget(mParentPlugin, this);
        mNavigationLink->setMinimumHeight(28);
        navigationLayout->addWidget(mNavigationLink);

        CreateEntry(navigationMenu, navigationLayout, "Toggle Navigation Pane", "Images/AnimGraphPlugin/HierarchyView.png", true, false, NAVIGATION_NAVPANETOGGLE, 0, false, false);
        connect(mActions[NAVIGATION_NAVPANETOGGLE],    SIGNAL(triggered()), this, SLOT(ToggleNavigationPane()));

        // add the top widget with the menu and toolbar and the gl widget to the vertical layout
        verticalLayout->addWidget(toolbarWidget);
        verticalLayout->addLayout(navigationLayout);
        verticalLayout->addWidget(&mViewportStack);
        
        mViewportSplitter = new QSplitter(Qt::Horizontal, this);
        mViewportSplitter->addWidget(blendGraphWidget);
        mViewportSplitter->addWidget(navigateWidget);
        mViewportSplitter->setCollapsible(0, false);
        QList<int> sizes = { this->width(), 0 };
        mViewportSplitter->setSizes(sizes);
        mViewportStack.addWidget(mViewportSplitter);

        Reset();
        Update();
    }


    BlendGraphViewWidget::~BlendGraphViewWidget()
    {
        EMotionFX::ActorEditorRequestBus::Handler::BusDisconnect();

        for (auto entry : mNodeTypeToWidgetMap)
        {
            AnimGraphNodeWidget* widget = entry.second;
            delete widget;
        }

        // unregister and destroy the command callbacks
        GetCommandManager()->RemoveCommandCallback(mSaveAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSaveMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mLoadMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mActivateAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAddConditionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveConditionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphCreateConnectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveConnectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustConnectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphCreateNodeCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustNodeCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveNodeCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphSetEntryStateCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAddNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphCreateParameterCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveParameterCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustParameterCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphSwapParametersCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustParameterGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAddParameterGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveParameterGroupCallback, false);

        // delete callbacks
        delete mSaveAnimGraphCallback;
        delete mCreateAnimGraphCallback;
        delete mRemoveAnimGraphCallback;
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mCreateMotionSetCallback;
        delete mRemoveMotionSetCallback;
        delete mAdjustMotionSetCallback;
        delete mSaveMotionSetCallback;
        delete mLoadMotionSetCallback;
        delete mActivateAnimGraphCallback;
        delete mAnimGraphAddConditionCallback;
        delete mAnimGraphRemoveConditionCallback;
        delete mAnimGraphCreateConnectionCallback;
        delete mAnimGraphRemoveConnectionCallback;
        delete mAnimGraphAdjustConnectionCallback;
        delete mAnimGraphCreateNodeCallback;
        delete mAnimGraphAdjustNodeCallback;
        delete mAnimGraphRemoveNodeCallback;
        delete mAnimGraphSetEntryStateCallback;
        delete mAnimGraphAdjustNodeGroupCallback;
        delete mAnimGraphAddNodeGroupCallback;
        delete mAnimGraphRemoveNodeGroupCallback;
        delete mAnimGraphCreateParameterCallback;
        delete mAnimGraphRemoveParameterCallback;
        delete mAnimGraphAdjustParameterCallback;
        delete mAnimGraphSwapParametersCallback;
        delete mAnimGraphAdjustParameterGroupCallback;
        delete mAnimGraphAddParameterGroupCallback;
        delete mAnimGraphRemoveParameterGroupCallback;
    }


    void BlendGraphViewWidget::Update()
    {
        mActions[NAVIGATION_BACK]->setEnabled(mParentPlugin->CanPopHistory());
        mActions[NAVIGATION_FORWARD]->setEnabled(mParentPlugin->CanStepForwardInHistory());

        mToolbarButtons[NAVIGATION_BACK]->setEnabled(mParentPlugin->CanPopHistory());
        mToolbarButtons[NAVIGATION_FORWARD]->setEnabled(mParentPlugin->CanStepForwardInHistory());

        // get the anim graph that is currently selected in the resource widget
        EMotionFX::AnimGraph* animGraph = mParentPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            SetOptionEnabled(FILE_SAVE, false);
            SetOptionEnabled(FILE_SAVEAS, false);
            SetOptionEnabled(NAVIGATION_ROOT, false);
        }
        else
        {
            SetOptionEnabled(FILE_SAVE, true);
            SetOptionEnabled(FILE_SAVEAS, true);
            SetOptionEnabled(NAVIGATION_ROOT, true);
        }

        SetOptionEnabled(NAVIGATION_PARENT, false);
        EMotionFX::AnimGraphNode* currentAnimGraphNode = mParentPlugin->GetGraphWidget()->GetCurrentNode();
        if (currentAnimGraphNode)
        {
            EMotionFX::AnimGraphNode* parentNode = currentAnimGraphNode->GetParentNode();
            if (parentNode)
            {
                SetOptionEnabled(NAVIGATION_PARENT, true);
            }
        }

        // update the navigation link
        mNavigationLink->Update(animGraph, currentAnimGraphNode);

        // get the number of selected nodes
        BlendGraphWidget* blendGraphWidget = mParentPlugin->GetGraphWidget();
        NodeGraph* nodeGraph = blendGraphWidget->GetActiveGraph();
        uint32 numSelectedNodes = 0;
        if (nodeGraph)
        {
            // get the number of selected nodes
            numSelectedNodes = nodeGraph->CalcNumSelectedNodes();

            // graph not empty flag
            const bool GraphNotEmpty = nodeGraph->GetNumNodes() > 0;

            // enable the select all menu only if at least one node is in the graph
            SetOptionEnabled(SELECTION_ZOOMSELECTION, true);
        }
        else
        {
            // select all and zoom all menus can not be enabled if one graph is not valid
            SetOptionEnabled(SELECTION_ZOOMSELECTION, false);
        }

        SetOptionEnabled(NAVIGATION_OPENSELECTEDNODE, false);
        if (numSelectedNodes == 1)
        {
            EMotionFX::AnimGraphNode* animGraphNode = blendGraphWidget->FindFirstSelectedAnimGraphNode();
            if (animGraphNode)
            {
                if (animGraphNode->GetHasVisualGraph())
                {
                    SetOptionEnabled(NAVIGATION_OPENSELECTEDNODE, true);
                }

                // get the parent node and check if it is valid
                EMotionFX::AnimGraphNode* parentNode = animGraphNode->GetParentNode();
                if (parentNode)
                {
                    SetOptionEnabled(NAVIGATION_PARENT, true);
                }
            }
        }

        if (numSelectedNodes > 1)
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

    void BlendGraphViewWidget::UpdateOpenMenu()
    {
        BuildOpenMenu();
    }


    EMotionFX::ActorInstance* BlendGraphViewWidget::GetSelectedActorInstance()
    {
        return GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
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

    void BlendGraphViewWidget::SetCurrentNode(EMotionFX::AnimGraphNode* node)
    {
        // Reset the viewports to avoid dangling pointers.
        for (auto& item : mNodeTypeToWidgetMap)
        {
            AnimGraphNodeWidget* viewport = item.second;
            viewport->SetCurrentNode(nullptr);
        }

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
        }
        else
        {
            // Show the default widget.
            mViewportStack.setCurrentIndex(0);
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
            connect(toolbarButton, SIGNAL(clicked()), mActions[actionIndex], SLOT(trigger()));
        }
    }

    void BlendGraphViewWidget::BuildOpenMenu()
    {        
        mOpenMenu->clear();
//        disconnect(mActions[FILE_OPEN], SIGNAL(triggered()), mParentPlugin, SLOT(OnFileOpen()));
        mActions[FILE_OPEN] = mOpenMenu->addAction("Open...");
        connect(mActions[FILE_OPEN], SIGNAL(triggered()), mParentPlugin, SLOT(OnFileOpen()));
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        if (numAnimGraphs > 0)
        {
//            /*mActions[FILE_OPEN] =*/ mOpenMenu->addAction("Manage AnimGraphs...");
            mOpenMenu->addSeparator();
            for (uint32 i = 0; i < numAnimGraphs; ++i)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
                if (animGraph->GetIsOwnedByRuntime() == false)
                {
                    QString itemName = "<Unsaved Animgraph>";
                    if (animGraph->GetFileNameString().size() > 0)
                    {
                        itemName = QString::fromUtf8(animGraph->GetFileName());
                        bool success = false;

                        AZStd::string relativeProductPath;
                        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, animGraph->GetFileName(), relativeProductPath);
                        if (success)
                        {
                            AZStd::string fullSourcePath;
                            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativeProductPath, fullSourcePath);
                            if (success)
                            {
                                AZStd::string watchFolder;
                                AZ::Data::AssetInfo assetInfo;
                                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, fullSourcePath.c_str(), assetInfo, watchFolder);
                                if (success)
                                {
                                    itemName = QString::fromUtf8(assetInfo.m_relativePath.c_str());
                                }
                            }
                        }
                    }
                    QAction* openItem = mOpenMenu->addAction(itemName);
                    connect(openItem, &QAction::triggered, this, &BlendGraphViewWidget::OpenAnimGraph);
                    openItem->setData(animGraph->GetID());
                }
            }
        }
    }

    void BlendGraphViewWidget::OpenAnimGraph()
    {
        QAction *action = qobject_cast<QAction *>(sender());
        if (action)
        {
            uint32 animGraphID = action->data().toUInt();
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);

            EMotionFX::MotionSet* motionSet = nullptr;
            EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);

            ActivateAnimGraphToSelection(animGraph, motionSet);
        }
    }

    // get the selected anim graph
    EMotionFX::AnimGraph* BlendGraphViewWidget::GetSelectedAnimGraph()
    {
        // check if the ID is not valid (nothing selected or multiple selection)
        if (mSelectedAnimGraphID == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        // get the anim graph by the ID
        return EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mSelectedAnimGraphID);
    }


    // activate anim graph to the selected actor instances
    void BlendGraphViewWidget::ActivateAnimGraphToSelection(EMotionFX::AnimGraph* animGraph, EMotionFX::MotionSet* motionSet)
    {
        if (!animGraph)
        {
            return;
        }

        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

        // Anim graph can only be activated in case there are actor instances.
        if (numActorInstances == 0)
        {
            mParentPlugin->SetActiveAnimGraph(animGraph);
            return;
        }

        MCore::CommandGroup commandGroup("Activate anim graph");

        // Activate the anim graph each selected actor instance.
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

            // Use the given motion set in case it is valid, elsewise use the one previously set to the actor instance.
            uint32 motionSetId = MCORE_INVALIDINDEX32;
            if (motionSet)
            {
                motionSetId = motionSet->GetID();
            }
            else
            {
                if (animGraphInstance)
                {
                    EMotionFX::MotionSet* animGraphInstanceMotionSet = animGraphInstance->GetMotionSet();
                    if (animGraphInstanceMotionSet)
                    {
                        motionSetId = animGraphInstanceMotionSet->GetID();
                    }
                }
            }

            commandGroup.AddCommandString(AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetID %d", actorInstance->GetID(), animGraph->GetID(), motionSetId));
        }

        if (commandGroup.GetNumCommands() > 0)
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
            mParentPlugin->SetActiveAnimGraph(animGraph);
            mParentPlugin->SetAnimGraphRunning(true);
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


    void BlendGraphViewWidget::Reset()
    {
    }


    void BlendGraphViewWidget::SaveOptions(QSettings* settings)
    {
        const uint32 numOptions = NUM_OPTIONS;
        for (uint32 i = 0; i < numOptions; ++i)
        {
            QString name = QString(i);
            if (mActions[i]->isCheckable())
            {
                settings->setValue(name, mActions[i]->isChecked());
            }
        }
    }


    void BlendGraphViewWidget::LoadOptions(QSettings* settings)
    {
        const uint32 numOptions = NUM_OPTIONS;
        for (uint32 i = 0; i < numOptions; ++i)
        {
            QString name = QString(i);
            if (mActions[i]->isCheckable())
            {
                const bool isEnabled = settings->value(name, mActions[i]->isChecked()).toBool();
                SetOptionFlag((EOptionFlag)i, isEnabled);
            }
        }
    }



    void BlendGraphViewWidget::AlignNodes(uint32 mode)
    {
        NodeGraph* nodeGraph = mParentPlugin->GetGraphWidget()->GetActiveGraph();
        if (nodeGraph == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraph* animGraph = mParentPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        uint32 i;
        int32 alignedXPos = 0;
        int32 alignedYPos = 0;
        int32 maxGraphNodeHeight = 0;
        int32 maxGraphNodeWidth = 0;

        // iterate over all graph nodes
        bool firstSelectedNode = true;
        const uint32 numNodes = nodeGraph->GetNumNodes();
        for (i = 0; i < numNodes; ++i)
        {
            // get the graph and the anim graph node
            GraphNode*                  graphNode       = nodeGraph->GetNode(i);
            EMotionFX::AnimGraphNode*  animGraphNode  = animGraph->RecursiveFindNodeByName(graphNode->GetName());
            if (animGraphNode == nullptr)
            {
                continue;
            }

            if (graphNode->GetIsSelected())
            {
                const int32 xPos            = animGraphNode->GetVisualPosX();
                const int32 yPos            = animGraphNode->GetVisualPosY();
                const int32 graphNodeHeight = graphNode->CalcRequiredHeight();
                const int32 graphNodeWidth  = graphNode->CalcRequiredWidth();

                if (firstSelectedNode)
                {
                    alignedXPos         = xPos;
                    alignedYPos         = yPos;
                    maxGraphNodeHeight  = graphNodeHeight;
                    maxGraphNodeWidth   = graphNodeWidth;
                    firstSelectedNode   = false;
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

        // create the command group
        AZStd::string command;
        AZStd::string outResult;
        MCore::CommandGroup commandGroup("Align anim graph nodes");

        // iterate over all nodes
        for (i = 0; i < numNodes; ++i)
        {
            GraphNode* node = nodeGraph->GetNode(i);

            if (node->GetIsSelected())
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
        }

        // execute the group command
        GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult);
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
            MCore::CommandGroup commandGroup("Create a anim graph");

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

            // get the motion set ID
            const uint32 motionSetID = (motionSet) ? motionSet->GetID() : MCORE_INVALIDINDEX32;

            // activate on each selected actor instance
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                m_tempString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %%LASTRESULT%% -motionSetID %d", actorInstance->GetID(), motionSetID);
                commandGroup.AddCommandString(m_tempString);
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
    void BlendGraphViewWidget::ZoomSelected()       { mParentPlugin->FitActiveSelectionOnScreen(); }


    void BlendGraphViewWidget::OnActivateState()
    {
        NavigateWidget* navigateWidget = mParentPlugin->GetNavigateWidget();
        BlendGraphWidget* blendGraphWidget = mParentPlugin->GetGraphWidget();
        if (!navigateWidget || !blendGraphWidget)
        {
            return;
        }

        EMotionFX::AnimGraph* animGraph = GetSelectedAnimGraph();
        NodeGraph* nodeGraph = blendGraphWidget->GetActiveGraph();
        if (!nodeGraph || !animGraph)
        {
            return;
        }

        // Reactivate the anim graph for visual feedback of button click.
        EMotionFX::MotionSet* motionSet = nullptr;
        EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
        ActivateAnimGraphToSelection(animGraph, motionSet);

        // Transition to the selected state.
        EMotionFX::AnimGraphNode* selectedNode = blendGraphWidget->FindFirstSelectedAnimGraphNode();
        if (selectedNode)
        {
            EMotionFX::AnimGraphNode* parentNode = selectedNode->GetParentNode();
            if (parentNode && azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

                // For all selected actor instances that are running the given anim graph.
                const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
                const AZ::u32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
                for (AZ::u32 i = 0; i < numSelectedActorInstances; ++i)
                {
                    EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                    EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

                    if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
                    {
                        stateMachine->TransitionToState(animGraphInstance, selectedNode);
                    }
                }
            }
        }
    }


    void BlendGraphViewWidget::NavigateForward()    { mParentPlugin->HistoryStepForward(); Update(); }
    void BlendGraphViewWidget::NavigateBackward()   { mParentPlugin->HistoryStepBack(); Update(); }


    void BlendGraphViewWidget::NavigateToRoot()
    {
        EMotionFX::AnimGraph* animGraph = mParentPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraphNode* rootNode = nullptr;
        mParentPlugin->GetNavigateWidget()->ShowGraph(rootNode, true);
    }


    void BlendGraphViewWidget::NavigateToParent()
    {
        EMotionFX::AnimGraphNode* animGraphNode = mParentPlugin->GetGraphWidget()->GetCurrentNode();
        if (animGraphNode == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraphNode* parentNode = animGraphNode->GetParentNode();
        mParentPlugin->GetNavigateWidget()->ShowGraph(parentNode, true);
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
        EMotionFX::AnimGraphNode* animGraphNode = mParentPlugin->GetGraphWidget()->FindFirstSelectedAnimGraphNode();
        if (animGraphNode == nullptr)
        {
            return;
        }

        mParentPlugin->GetNavigateWidget()->ShowGraph(animGraphNode, true);
    }


    // toggle visualization of processed nodes and connections
    /*void BlendGraphViewWidget::OnShowProcessed()
    {
        const bool showProcessed = !mParentPlugin->GetShowProcessed();
        mParentPlugin->SetShowProcessed( showProcessed );

        SetOptionFlag( SELECTION_SHOWPROCESSED, showProcessed );
    }*/


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
            NavigateBackward();
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


    //----------------------------------------------------------------------------------------------------------------------------------
    // command callbacks
    //----------------------------------------------------------------------------------------------------------------------------------

    bool UpdateOpenMenuBlendGraphViewWidget()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
        BlendGraphViewWidget* blendGraphViewWidget = animGraphPlugin->GetViewWidget();

        animGraphPlugin->RemoveAllUnusedGraphInfos();
        blendGraphViewWidget->UpdateOpenMenu();
        return true;
    }


    bool UpdateOpenMenuAndMotionSetBlendGraphViewWidget()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
        BlendGraphViewWidget* blendGraphViewWidget = animGraphPlugin->GetViewWidget();

        animGraphPlugin->RemoveAllUnusedGraphInfos();
        blendGraphViewWidget->UpdateOpenMenu();
        EMotionFX::AnimGraphEditorNotificationBus::Broadcast(&EMotionFX::AnimGraphEditorNotifications::UpdateMotionSetComboBox);
        return true;
    }


    bool UpdateAnimGraphPluginAfterActivate()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin*                animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
        animGraphPlugin->SetAnimGraphRunning(true);

        BlendGraphViewWidget*           blendGraphViewWidget = animGraphPlugin->GetViewWidget();
        EMStudio::ParameterWindow*      parameterWindow = animGraphPlugin->GetParameterWindow();
        EMStudio::AttributesWindow*     attributesWindow = animGraphPlugin->GetAttributesWindow();
        EMStudio::NodeGroupWindow*      nodeGroupWindow = animGraphPlugin->GetNodeGroupWidget();

        animGraphPlugin->RemoveAllUnusedGraphInfos();
        blendGraphViewWidget->UpdateOpenMenu();

        animGraphPlugin->UpdateStateMachineColors();

        parameterWindow->Init();
        nodeGroupWindow->Init();
        return true;
    }


    bool UpdateMotionSetComboBoxResourceWidget()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
        BlendGraphViewWidget* blendGraphViewWidget = animGraphPlugin->GetViewWidget();

        EMotionFX::AnimGraphEditorNotificationBus::Broadcast(&EMotionFX::AnimGraphEditorNotifications::UpdateMotionSetComboBox);
        return true;
    }


    bool BlendGraphViewWidget::CommandSaveAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandSaveAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateOpenMenuBlendGraphViewWidget(); }


    bool BlendGraphViewWidget::CommandActivateAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphPluginAfterActivate(); }
    bool BlendGraphViewWidget::CommandActivateAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphPluginAfterActivate(); }

    bool BlendGraphViewWidget::CommandCreateAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        CommandSystem::CommandCreateAnimGraph* createAnimGraphCommand = static_cast<CommandSystem::CommandCreateAnimGraph*>(command);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(createAnimGraphCommand->mPreviouslyUsedID);
        GetOutlinerManager()->AddItemToCategory("Anim Graphs", animGraph->GetID(), animGraph);
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandCreateAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateOpenMenuBlendGraphViewWidget(); }


    bool BlendGraphViewWidget::CommandRemoveAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        CommandSystem::CommandRemoveAnimGraph* removeAnimGraphCommand = static_cast<CommandSystem::CommandRemoveAnimGraph*>(command);
        for (const AZStd::pair<AZStd::string, uint32>& oldFilenameAndIds : removeAnimGraphCommand->m_oldFileNamesAndIds)
        {
            GetOutlinerManager()->RemoveItemFromCategory("Anim Graphs", oldFilenameAndIds.second);
        }
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandRemoveAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateOpenMenuBlendGraphViewWidget(); }


    bool BlendGraphViewWidget::CommandAnimGraphAddConditionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAddConditionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveConditionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveConditionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphCreateConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphCreateConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphCreateNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphCreateNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphSetEntryStateCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphSetEntryStateCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAddNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAddNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphCreateParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphCreateParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphSwapParametersCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphSwapParametersCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustParameterGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAdjustParameterGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAddParameterGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphAddParameterGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveParameterGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandAnimGraphRemoveParameterGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateOpenMenuBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false && CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateOpenMenuAndMotionSetBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false && CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateOpenMenuAndMotionSetBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false && CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateOpenMenuAndMotionSetBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateOpenMenuAndMotionSetBlendGraphViewWidget();
    }


    bool BlendGraphViewWidget::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateOpenMenuAndMotionSetBlendGraphViewWidget(); }
    bool BlendGraphViewWidget::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateOpenMenuAndMotionSetBlendGraphViewWidget(); }


    //////////////////////////////////////////////////////////////////////////////////////////////////
    // Motion set related callbacks
    //////////////////////////////////////////////////////////////////////////////////////////////////


    bool BlendGraphViewWidget::CommandAdjustMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (commandLine.CheckIfHasParameter("newName"))
        {
            return UpdateMotionSetComboBoxResourceWidget();
        }
        return true;
    }
    bool BlendGraphViewWidget::CommandAdjustMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (commandLine.CheckIfHasParameter("newName"))
        {
            return UpdateMotionSetComboBoxResourceWidget();
        }
        return true;
    }
    bool BlendGraphViewWidget::CommandCreateMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendGraphViewWidget::CommandCreateMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendGraphViewWidget::CommandRemoveMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendGraphViewWidget::CommandRemoveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendGraphViewWidget::CommandSaveMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendGraphViewWidget::CommandSaveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendGraphViewWidget::CommandLoadMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendGraphViewWidget::CommandLoadMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.moc>
