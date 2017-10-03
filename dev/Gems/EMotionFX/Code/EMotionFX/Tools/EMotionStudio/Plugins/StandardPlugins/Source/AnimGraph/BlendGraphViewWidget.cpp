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
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "BlendGraphWidget.h"
#include "BlendGraphViewWidget.h"
#include "BlendSpace2DNodeWidget.h"
#include "BlendSpace1DNodeWidget.h"
#include "AnimGraphPlugin.h"
#include "AttributesWindow.h"
#include "NavigateWidget.h"
#include "ParameterWindow.h"
#include "GraphNode.h"
#include "NodePaletteWidget.h"

// qt includes
#include <QDropEvent>
#include <QLabel>
#include <QMenuBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QIcon>
#include <QSettings>
#include <QDropEvent>

// emfx and core includes
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>

// emstudio SDK
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"


namespace EMStudio
{
    // constructor
    BlendGraphViewWidget::BlendGraphViewWidget(AnimGraphPlugin* plugin, QWidget* parentWidget)
        : QWidget(parentWidget)
    {
        mParentPlugin = plugin;
    }


    void BlendGraphViewWidget::Init(BlendGraphWidget* blendGraphWidget)
    {
        NavigateWidget*     navigateWidget  = mParentPlugin->GetNavigateWidget();
        BlendGraphWidget*   graphWidget     = mParentPlugin->GetGraphWidget();

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

        // create the top widget with the menu and the toolbar
        QWidget* topWidget = new QWidget();
        topWidget->setMinimumHeight(23); // menu fix (to get it working with the Ly style)
        QHBoxLayout* horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSizeConstraint(QLayout::SetMaximumSize);
        horizontalLayout->setSpacing(2);
        horizontalLayout->setMargin(0);
        topWidget->setLayout(horizontalLayout);

        // create menu
        mMenu = new QMenuBar(this);
        mMenu->setStyleSheet("QMenuBar { min-height: 5px;}"); // menu fix (to get it working with the Ly style)
        mMenu->setNativeMenuBar(false);
        mMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        horizontalLayout->addWidget(mMenu);

        // create the toolbar
        QWidget* toolbarWidget = new QWidget();
        mToolbarLayout = new QHBoxLayout();
        mToolbarLayout->setSizeConstraint(QLayout::SetFixedSize);
        mToolbarLayout->setSpacing(0);
        mToolbarLayout->setMargin(0);
        toolbarWidget->setLayout(mToolbarLayout);

        horizontalLayout->addWidget(toolbarWidget);

        QMenu* fileMenu         = mMenu->addMenu("&File");
        QMenu* selectionMenu    = mMenu->addMenu("&Selection");
        QMenu* navigationMenu   = mMenu->addMenu("&Navigation");
        QMenu* vizMenu          = mMenu->addMenu("&Visualization");
        QMenu* windowsMenu      = mMenu->addMenu("&Window");

        // file section
        CreateEntry(fileMenu, mToolbarLayout, "Open",                           "Images/Icons/Open.png",            true,   false, FILE_OPEN /*, QKeySequence::Open*/);
        CreateEntry(fileMenu, mToolbarLayout, "Save",                           "Images/Menu/FileSave.png",         true,   false, FILE_SAVE /*, QKeySequence::Save*/);
        CreateEntry(fileMenu, mToolbarLayout, "Save As",                        "Images/Menu/FileSaveAs.png",       false,  false, FILE_SAVEAS /*, QKeySequence::SaveAs*/);

        connect(mActions[FILE_OPEN], SIGNAL(triggered()), mParentPlugin, SLOT(OnFileOpen()));
        connect(mActions[FILE_SAVEAS], SIGNAL(triggered()), mParentPlugin, SLOT(OnFileSaveAs()));
        connect(mActions[FILE_SAVE], SIGNAL(triggered()), mParentPlugin, SLOT(OnFileSave()));

        AddSeparator();

        // selection section
        CreateEntry(selectionMenu, mToolbarLayout, "Align Left",                "Images/AnimGraphPlugin/AlignLeft.png",    true,   false, SELECTION_ALIGNLEFT);
        CreateEntry(selectionMenu, mToolbarLayout, "Align Right",               "Images/AnimGraphPlugin/AlignRight.png",   true,   false, SELECTION_ALIGNRIGHT);
        CreateEntry(selectionMenu, mToolbarLayout, "Align Top",                 "Images/AnimGraphPlugin/AlignTop.png",     true,   false, SELECTION_ALIGNTOP);
        CreateEntry(selectionMenu, mToolbarLayout, "Align Bottom",              "Images/AnimGraphPlugin/AlignBottom.png",  true,   false, SELECTION_ALIGNBOTTOM);
        AddSeparator();
        selectionMenu->addSeparator();
        CreateEntry(selectionMenu, mToolbarLayout, "Zoom Selection",            "Images/AnimGraphPlugin/FitSelected.png",  true,   false, SELECTION_ZOOMSELECTION);
        CreateEntry(selectionMenu, mToolbarLayout, "Zoom Entire",               "Images/AnimGraphPlugin/FitAll.png",       true,   false, SELECTION_ZOOMALL);
        AddSeparator();
        selectionMenu->addSeparator();
        CreateEntry(selectionMenu, mToolbarLayout, "Cut",                       "Images/Icons/Cut.png",                     true,   false, SELECTION_CUT);
        CreateEntry(selectionMenu, mToolbarLayout, "Copy",                      "Images/Icons/Copy.png",                    true,   false, SELECTION_COPY);
        AddSeparator();
        selectionMenu->addSeparator();
        CreateEntry(selectionMenu, mToolbarLayout, "Select All",                "Images/Icons/SelectAll.png",               false,  false, SELECTION_SELECTALL);
        CreateEntry(selectionMenu, mToolbarLayout, "Unselect All",              "Images/Icons/UnselectAll.png",             false,  false, SELECTION_UNSELECTALL);
        selectionMenu->addSeparator();
        CreateEntry(selectionMenu, mToolbarLayout, "Add Wildcard Transition",   "Images/Icons/Plus.png",                    false,  false, SELECTION_ADDWILDCARDTRANSITION);
        CreateEntry(selectionMenu, mToolbarLayout, "Delete Nodes",              "Images/Icons/Remove.png",                  true,   false, SELECTION_DELETENODES);
        AddSeparator();
        selectionMenu->addSeparator();
        CreateEntry(selectionMenu, mToolbarLayout, "Activate State",            "Images/AnimGraphPlugin/Run.png",          true,   false, SELECTION_ACTIVATESTATE /*, Qt::CTRL + Qt::Key_J*/);
        CreateEntry(selectionMenu, mToolbarLayout, "Set As Entry State",        "Images/AnimGraphPlugin/EntryState.png",   true,   false, SELECTION_SETASENTRYNODE /*, Qt::CTRL + Qt::Key_J*/);

        connect(mActions[SELECTION_ALIGNLEFT], SIGNAL(triggered()), this, SLOT(AlignLeft()));
        connect(mActions[SELECTION_ALIGNRIGHT], SIGNAL(triggered()), this, SLOT(AlignRight()));
        connect(mActions[SELECTION_ALIGNTOP], SIGNAL(triggered()), this, SLOT(AlignTop()));
        connect(mActions[SELECTION_ALIGNBOTTOM], SIGNAL(triggered()), this, SLOT(AlignBottom()));
        connect(mActions[SELECTION_ZOOMSELECTION], SIGNAL(triggered()), this, SLOT(ZoomSelected()));
        connect(mActions[SELECTION_ZOOMALL], SIGNAL(triggered()), this, SLOT(ZoomAll()));

        // visualization section
        //CreateEntry( vizMenu, mToolbarLayout, "Debug Visualization",          "Images/AnimGraphPlugin/ShowProcessed.png",true,   true,  SELECTION_SHOWPROCESSED, Qt::CTRL + Qt::Key_P);

        CreateEntry(vizMenu, mToolbarLayout, "Display Play Speeds",        "", false,  true,  VISUALIZATION_PLAYSPEEDS);
        CreateEntry(vizMenu, mToolbarLayout, "Display Global Weights",     "", false,  true,  VISUALIZATION_GLOBALWEIGHTS);
        CreateEntry(vizMenu, mToolbarLayout, "Display Sync Status",        "", false,  true,  VISUALIZATION_SYNCSTATUS);
        CreateEntry(vizMenu, mToolbarLayout, "Display Play Positions",     "", false,  true,  VISUALIZATION_PLAYPOSITIONS);

        //connect( mActions[SELECTION_SHOWPROCESSED], SIGNAL(triggered()), this, SLOT(OnShowProcessed()) );
        connect(mActions[VISUALIZATION_PLAYSPEEDS], SIGNAL(triggered()), this, SLOT(OnDisplayPlaySpeeds()));
        connect(mActions[VISUALIZATION_GLOBALWEIGHTS], SIGNAL(triggered()), this, SLOT(OnDisplayGlobalWeights()));
        connect(mActions[VISUALIZATION_SYNCSTATUS], SIGNAL(triggered()), this, SLOT(OnDisplaySyncStatus()));
        connect(mActions[VISUALIZATION_PLAYPOSITIONS], SIGNAL(triggered()), this, SLOT(OnDisplayPlayPositions()));

        // windows section
        CreateEntry(windowsMenu, mToolbarLayout, "Parameter Window",            "",     false, true, WINDOWS_PARAMETERWINDOW);
        CreateEntry(windowsMenu, mToolbarLayout, "Hierarchy Window",            "",     false, true, WINDOWS_HIERARCHYWINDOW);
        CreateEntry(windowsMenu, mToolbarLayout, "Attribute Window",            "",     false, true, WINDOWS_ATTRIBUTEWINDOW);
        CreateEntry(windowsMenu, mToolbarLayout, "Node Group Window",           "",     false, true, WINDOWS_NODEGROUPWINDOW);
        CreateEntry(windowsMenu, mToolbarLayout, "Palette Window",              "",     false, true, WINDOWS_PALETTEWINDOW);
        CreateEntry(windowsMenu, mToolbarLayout, "Game Controller Window",      "",     false, true, WINDOWS_GAMECONTROLLERWINDOW);

        connect(mActions[WINDOWS_PARAMETERWINDOW], SIGNAL(triggered()), this, SLOT(UpdateWindowVisibility()));
        connect(mActions[WINDOWS_HIERARCHYWINDOW], SIGNAL(triggered()), this, SLOT(UpdateWindowVisibility()));
        connect(mActions[WINDOWS_ATTRIBUTEWINDOW], SIGNAL(triggered()), this, SLOT(UpdateWindowVisibility()));
        connect(mActions[WINDOWS_NODEGROUPWINDOW], SIGNAL(triggered()), this, SLOT(UpdateWindowVisibility()));
        connect(mActions[WINDOWS_PALETTEWINDOW], SIGNAL(triggered()), this, SLOT(UpdateWindowVisibility()));
        connect(mActions[WINDOWS_GAMECONTROLLERWINDOW], SIGNAL(triggered()), this, SLOT(UpdateWindowVisibility()));


        /////////////////////////////////////////////////////////////////////////////////////
        // hierarchy navigation
        /////////////////////////////////////////////////////////////////////////////////////
        QHBoxLayout* navigationLayout = new QHBoxLayout();
        navigationLayout->setSizeConstraint(QLayout::SetMaximumSize);
        navigationLayout->setSpacing(2);
        navigationLayout->setMargin(0);

        // navigation section
        CreateEntry(navigationMenu, navigationLayout, "Open Root Node",     "Images/AnimGraphPlugin/GoToRoot.png",     false,  false, NAVIGATION_ROOT);
        CreateEntry(navigationMenu, navigationLayout, "Open Selected Node", "Images/AnimGraphPlugin/OpenNode.png",     false,  false, NAVIGATION_OPENSELECTEDNODE);
        CreateEntry(navigationMenu, navigationLayout, "Back",               "Images/AnimGraphPlugin/StepBack.png",     true,   false, NAVIGATION_BACK, 0, false);
        CreateEntry(navigationMenu, navigationLayout, "Forward",            "Images/AnimGraphPlugin/StepForward.png",  true,   false, NAVIGATION_FORWARD, 0, false);

        // add the hierarchy navigation
        mNavigationLink = new NavigationLinkWidget(mParentPlugin, this);
        navigationLayout->addWidget(mNavigationLink);

        CreateEntry(navigationMenu, navigationLayout, "Open Parent Node",       "Images/AnimGraphPlugin/MoveUp.png",   true,   false, NAVIGATION_PARENT, 0, false);

        // add a dummy widget between the navigation items and the search bar
        /*QWidget* dummyWidget = new QWidget();
        dummyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        dummyWidget->setMinimumWidth(0);
        navigationLayout->addWidget(dummyWidget);

        mSearchButton = new SearchButton( nullptr, MysticQt::GetMysticQt()->FindIcon("Images/Icons/SearchClearButton.png") );
        mSearchButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        navigationLayout->addWidget(mSearchButton);*/

        connect(mActions[NAVIGATION_FORWARD],          SIGNAL(triggered()), this, SLOT(NavigateForward()));
        connect(mActions[NAVIGATION_BACK],             SIGNAL(triggered()), this, SLOT(NavigateBackward()));
        connect(mActions[NAVIGATION_ROOT],             SIGNAL(triggered()), this, SLOT(NavigateToRoot()));
        connect(mActions[NAVIGATION_OPENSELECTEDNODE], SIGNAL(triggered()), this, SLOT(NavigateToNode()));
        connect(mActions[NAVIGATION_PARENT],           SIGNAL(triggered()), this, SLOT(NavigateToParent()));

        // add the top widget with the menu and toolbar and the gl widget to the vertical layout
        verticalLayout->addWidget(topWidget);
        verticalLayout->addLayout(navigationLayout);
        verticalLayout->addWidget(&mViewportStack);

        mViewportStack.addWidget(blendGraphWidget);

        Reset();
        Update();

        connect(mActions[SELECTION_SETASENTRYNODE],            SIGNAL(triggered()), navigateWidget,    SLOT(OnSetAsEntryState()));
        connect(mActions[SELECTION_ACTIVATESTATE],             SIGNAL(triggered()), navigateWidget,    SLOT(OnActivateState()));
        connect(mActions[SELECTION_CUT],                       SIGNAL(triggered()), navigateWidget,    SLOT(Cut()));
        connect(mActions[SELECTION_COPY],                      SIGNAL(triggered()), navigateWidget,    SLOT(Copy()));
        connect(mActions[SELECTION_SELECTALL],                 SIGNAL(triggered()), this,              SLOT(SelectAll()));
        connect(mActions[SELECTION_UNSELECTALL],               SIGNAL(triggered()), this,              SLOT(UnselectAll()));
        connect(mActions[SELECTION_DELETENODES],               SIGNAL(triggered()), graphWidget,       SLOT(DeleteSelectedItems()));
        connect(mActions[SELECTION_ADDWILDCARDTRANSITION],     SIGNAL(triggered()), navigateWidget,    SLOT(OnAddWildCardTransition()));
    }


    BlendGraphViewWidget::~BlendGraphViewWidget()
    {
        for (auto entry : mNodeTypeToWidgetMap)
        {
            AnimGraphNodeWidget* widget = entry.second;
            delete widget;
        }
    }


    void BlendGraphViewWidget::UpdateWindowVisibility()
    {
        mParentPlugin->GetParameterDock()->setVisible(GetOptionFlag(WINDOWS_PARAMETERWINDOW));
        mParentPlugin->GetNavigationDock()->setVisible(GetOptionFlag(WINDOWS_HIERARCHYWINDOW));
        mParentPlugin->GetAttributeDock()->setVisible(GetOptionFlag(WINDOWS_ATTRIBUTEWINDOW));
        mParentPlugin->GetNodeGroupDock()->setVisible(GetOptionFlag(WINDOWS_NODEGROUPWINDOW));
        mParentPlugin->GetNodePaletteDock()->setVisible(GetOptionFlag(WINDOWS_PALETTEWINDOW));

    #ifdef HAS_GAME_CONTROLLER
        mParentPlugin->GetGameControllerDock()->setVisible(GetOptionFlag(WINDOWS_GAMECONTROLLERWINDOW));
    #endif
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
            //      SetOptionEnabled(SELECTION_SHOWPROCESSED, false);
        }
        else
        {
            SetOptionEnabled(FILE_SAVE, true);
            SetOptionEnabled(FILE_SAVEAS, true);
            SetOptionEnabled(NAVIGATION_ROOT, true);
            //      SetOptionEnabled(SELECTION_SHOWPROCESSED, true);
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
            SetOptionEnabled(SELECTION_SELECTALL, GraphNotEmpty);
            SetOptionEnabled(SELECTION_ZOOMALL, GraphNotEmpty);
        }
        else
        {
            // select all and zoom all menus can not be enabled if one graph is not valid
            SetOptionEnabled(SELECTION_SELECTALL, false);
            SetOptionEnabled(SELECTION_ZOOMALL, false);
        }

        SetOptionEnabled(NAVIGATION_OPENSELECTEDNODE, false);
        SetOptionEnabled(SELECTION_ACTIVATESTATE, false);
        SetOptionEnabled(SELECTION_SETASENTRYNODE, false);
        SetOptionEnabled(SELECTION_ADDWILDCARDTRANSITION, false);
        //SetOptionEnabled(SELECTION_REMOVEWILDCARDTRANSITION, false);
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

                    if (parentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
                    {
                        // type cast the parent node to a state machine
                        //EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

                        SetOptionEnabled(SELECTION_ACTIVATESTATE, true);
                        SetOptionEnabled(SELECTION_SETASENTRYNODE, true);
                        SetOptionEnabled(SELECTION_ADDWILDCARDTRANSITION, true);

                        // check if the node already has a wildcard transition
                        //if (stateMachine->HasWildcardTransition(animGraphNode) == false)
                        //  SetOptionEnabled(SELECTION_REMOVEWILDCARDTRANSITION, true);
                    }
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

        if (numSelectedNodes > 0)
        {
            SetOptionEnabled(SELECTION_UNSELECTALL, true);
            SetOptionEnabled(SELECTION_ZOOMSELECTION, true);
            SetOptionEnabled(SELECTION_CUT, true);
            SetOptionEnabled(SELECTION_COPY, true);

            // check if we need to disable the delete nodes option as an undeletable node is selected
            bool canDelete = true;
            const uint32 numNodes = nodeGraph->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                if (nodeGraph->GetNode(i)->GetIsSelected() && nodeGraph->GetNode(i)->GetIsDeletable() == false)
                {
                    canDelete = false;
                    break;
                }
            }
            SetOptionEnabled(SELECTION_DELETENODES, canDelete);
        }
        else
        {
            SetOptionEnabled(SELECTION_UNSELECTALL, false);
            SetOptionEnabled(SELECTION_ZOOMSELECTION, false);
            SetOptionEnabled(SELECTION_CUT, false);
            SetOptionEnabled(SELECTION_COPY, false);
            SetOptionEnabled(SELECTION_DELETENODES, false);
        }
    }

    AnimGraphNodeWidget* BlendGraphViewWidget::GetWidgetForNode(const EMotionFX::AnimGraphNode* node)
    {
        if (!node)
        {
            return nullptr;
        }

        const uint32 nodeType = node->GetType();
        AnimGraphNodeWidget* widget = nullptr;

        AZStd::unordered_map<uint32, AnimGraphNodeWidget*>::iterator it =
            mNodeTypeToWidgetMap.find(nodeType);
        if (it != mNodeTypeToWidgetMap.end())
        {
            widget = it->second;
        }
        else
        {
            switch (nodeType)
            {
            case EMotionFX::BlendSpace2DNode::TYPE_ID:
                widget = new BlendSpace2DNodeWidget(mParentPlugin);
                mNodeTypeToWidgetMap[nodeType] = widget;
                break;
            case EMotionFX::BlendSpace1DNode::TYPE_ID:
                widget = new BlendSpace1DNodeWidget(mParentPlugin);
                mNodeTypeToWidgetMap[nodeType] = widget;
                break;
            default:
                break;
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
        mActions[actionIndex] = menu->addAction(entryName);
        mActions[actionIndex]->setVisible(addToMenu);
        mActions[actionIndex]->setCheckable(checkable);
        mActions[actionIndex]->setShortcut(shortcut);

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

            QSize buttonSize = QSize(20, 20);
            toolbarButton->setMinimumSize(buttonSize);
            toolbarButton->setMaximumSize(buttonSize);
            toolbarButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

            toolbarLayout->addWidget(toolbarButton);
            toolbarButton->setIconSize(buttonSize - QSize(2, 2));

            // connect the menu entry with the toolbar button
            connect(toolbarButton, SIGNAL(clicked()), mActions[actionIndex], SLOT(trigger()));
        }
    }


    void BlendGraphViewWidget::AddSeparator()
    {
        QWidget* toolbarButton = new QWidget();

        QSize buttonSize = QSize(10, 20);
        toolbarButton->setMinimumSize(buttonSize);
        toolbarButton->setMaximumSize(buttonSize);
        toolbarButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        mToolbarLayout->addWidget(toolbarButton);
    }


    void BlendGraphViewWidget::Reset()
    {
        SetOptionFlag(WINDOWS_PARAMETERWINDOW,          true);
        SetOptionFlag(WINDOWS_HIERARCHYWINDOW,          true);
        SetOptionFlag(WINDOWS_ATTRIBUTEWINDOW,          true);
        SetOptionFlag(WINDOWS_PALETTEWINDOW,            true);
        SetOptionFlag(WINDOWS_GAMECONTROLLERWINDOW,     true);
        SetOptionFlag(WINDOWS_NODEGROUPWINDOW,          true);
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
            EMotionFX::AnimGraphNode*  animGraphNode  = animGraph->RecursiveFindNode(graphNode->GetName());
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

                alignedXPos = xPos;
                alignedYPos = yPos;
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
        MCore::String command;
        MCore::String outResult;
        MCore::CommandGroup commandGroup("Align anim graph nodes");

        // iterate over all nodes
        for (i = 0; i < numNodes; ++i)
        {
            GraphNode* node = nodeGraph->GetNode(i);

            if (node->GetIsSelected())
            {
                command.Format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" ", animGraph->GetID(), node->GetName());

                switch (mode)
                {
                case SELECTION_ALIGNLEFT:
                    command.FormatAdd("-xPos %i", alignedXPos);
                    break;
                case SELECTION_ALIGNRIGHT:
                    command.FormatAdd("-xPos %i", alignedXPos - node->CalcRequiredWidth());
                    break;
                case SELECTION_ALIGNTOP:
                    command.FormatAdd("-yPos %i", alignedYPos);
                    break;
                case SELECTION_ALIGNBOTTOM:
                    command.FormatAdd("-yPos %i", alignedYPos - node->CalcRequiredHeight());
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


    void BlendGraphViewWidget::AlignLeft()          { AlignNodes(SELECTION_ALIGNLEFT); }
    void BlendGraphViewWidget::AlignRight()         { AlignNodes(SELECTION_ALIGNRIGHT); }
    void BlendGraphViewWidget::AlignTop()           { AlignNodes(SELECTION_ALIGNTOP); }
    void BlendGraphViewWidget::AlignBottom()        { AlignNodes(SELECTION_ALIGNBOTTOM); }
    void BlendGraphViewWidget::ZoomSelected()       { mParentPlugin->FitActiveSelectionOnScreen(); }
    void BlendGraphViewWidget::ZoomAll()            { mParentPlugin->FitActiveGraphOnScreen(); }
    void BlendGraphViewWidget::SelectAll()          { mParentPlugin->SelectAll(); }
    void BlendGraphViewWidget::UnselectAll()        { mParentPlugin->UnselectAll(); }
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
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.moc>
