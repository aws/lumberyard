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
#include "ParameterWindow.h"
#include "AnimGraphPlugin.h"
#include "GraphNode.h"
#include "NodeGraph.h"
#include "BlendTreeVisualNode.h"
#include "BlendGraphWidget.h"
#include "GameControllerWindow.h"
#include "ParameterCreateEditDialog.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QHeaderView>
#include <QPushButton>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphParameterGroup.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphGameControllerSettings.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterGroupCommands.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>


namespace EMStudio
{
    // constructor
    ParameterCreateRenameWindow::ParameterCreateRenameWindow(const char* windowTitle, const char* topText, const char* defaultName, const char* oldName, const AZStd::vector<AZStd::string>& invalidNames, QWidget* parent)
        : QDialog(parent)
    {
        // store values
        mOldName = oldName;
        mInvalidNames = invalidNames;

        // update title of the about dialog
        setWindowTitle(windowTitle);

        // set the minimum width
        setMinimumWidth(300);

        // create the about dialog's layout
        QVBoxLayout* layout = new QVBoxLayout();

        // add the top text if valid
        if (topText)
        {
            layout->addWidget(new QLabel(topText));
        }

        // add the line edit
        mLineEdit = new QLineEdit(defaultName);
        connect(mLineEdit, SIGNAL(textChanged(QString)), this, SLOT(NameEditChanged(QString)));
        mLineEdit->selectAll();

        // create the button layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mCancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        // set the layout
        layout->addWidget(mLineEdit);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        // connect the buttons
        connect(mOKButton, SIGNAL(clicked()), this, SLOT(accept()));
        connect(mCancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    }


    // destructor
    ParameterCreateRenameWindow::~ParameterCreateRenameWindow()
    {
    }


    // check for duplicate names upon editing
    void ParameterCreateRenameWindow::NameEditChanged(const QString& text)
    {
        const AZStd::string convertedNewName = text.toUtf8().data();
        if (text.isEmpty())
        {
            mOKButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }
        else if (mOldName == convertedNewName)
        {
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
        else
        {
            // Is there a parameter with the given name already?
            if (AZStd::find(mInvalidNames.begin(), mInvalidNames.end(), convertedNewName) != mInvalidNames.end())
            {
                mOKButton->setEnabled(false);
                GetManager()->SetWidgetAsInvalidInput(mLineEdit);
                return;
            }

            // no duplicate name found
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
    }


    // constructor
    ParameterWindow::ParameterWindow(AnimGraphPlugin* plugin)
        : QWidget()
    {
        mPlugin = plugin;
        mEnsureVisibility = false;
        mLockSelection = false;

        // hook the callbacks to the commands
        mCreateCallback         = new CommandCreateBlendParameterCallback(false);
        mRemoveCallback         = new CommandRemoveBlendParameterCallback(false);
        mAdjustCallback         = new CommandAdjustBlendParameterCallback(false);
        mAddGroupCallback       = new CommandAnimGraphAddParameterGroupCallback(false);
        mRemoveGroupCallback    = new CommandAnimGraphRemoveParameterGroupCallback(false);
        mAdjustGroupCallback    = new CommandAnimGraphAdjustParameterGroupCallback(false);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateParameter", mCreateCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameter", mRemoveCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameter", mAdjustCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddParameterGroup", mAddGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameterGroup", mRemoveGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameterGroup", mAdjustGroupCallback);

        // add the add button
        mAddButton = new QPushButton("");
        EMStudioManager::MakeTransparentButton(mAddButton, "/Images/Icons/Plus.png", "Add new parameter");
        connect(mAddButton, SIGNAL(clicked()), this, SLOT(OnAddParameter()));

        // add the remove button
        mRemoveButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mRemoveButton, "/Images/Icons/Minus.png", "Remove selected parameters");
        connect(mRemoveButton, SIGNAL(clicked()), this, SLOT(OnRemoveButton()));

        // add the clear button
        mClearButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mClearButton, "/Images/Icons/Clear.png", "Remove all parameters");
        connect(mClearButton, SIGNAL(clicked()), this, SLOT(OnClearButton()));

        // add edit button
        mEditButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mEditButton, "/Images/Icons/Edit.png", "Edit selected parameter");
        connect(mEditButton, SIGNAL(clicked()), this, SLOT(OnEditButton()));

        // add move up button
        mMoveUpButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mMoveUpButton, "/Images/Icons/UpArrow.png", "Move selected parameter up");
        connect(mMoveUpButton, SIGNAL(clicked()), this, SLOT(OnMoveParameterUp()));

        // add move down button
        mMoveDownButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mMoveDownButton, "/Images/Icons/DownArrow.png", "Move selected parameter down");
        connect(mMoveDownButton, SIGNAL(clicked()), this, SLOT(OnMoveParameterDown()));

        // add the search filter button
        mSearchButton = new MysticQt::SearchButton(this, MysticQt::GetMysticQt()->FindIcon("Images/Icons/SearchClearButton2.png"));
        connect(mSearchButton->GetSearchEdit(), SIGNAL(textChanged(const QString&)), this, SLOT(FilterStringChanged(const QString&)));

        // add the buttons to the layout
        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        buttonsLayout->setSpacing(0);
        buttonsLayout->setAlignment(Qt::AlignLeft);
        buttonsLayout->addWidget(mAddButton);
        buttonsLayout->addWidget(mRemoveButton);
        buttonsLayout->addWidget(mClearButton);
        buttonsLayout->addWidget(mMoveUpButton);
        buttonsLayout->addWidget(mMoveDownButton);
        buttonsLayout->addWidget(mEditButton);

        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        buttonsLayout->addWidget(spacerWidget);

        QHBoxLayout* searchLayout = new QHBoxLayout();
        searchLayout->addWidget(new QLabel("Find:"), 0, Qt::AlignRight);
        searchLayout->addWidget(mSearchButton);
        buttonsLayout->addLayout(searchLayout);
        searchLayout->setSpacing(6);

        // create the parameter tree widget
        mTreeWidget = new QTreeWidget();
        mTreeWidget->setObjectName("AnimGraphParamWindow");
        mTreeWidget->header()->setObjectName("AnimGraphParamWindow");

        // adjust selection mode and enable some other helpful things
        mTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTreeWidget->setAlternatingRowColors(true);
        mTreeWidget->setExpandsOnDoubleClick(true);
        mTreeWidget->setAnimated(true);
        mTreeWidget->setUniformRowHeights(true);

        // add the header labels
        mTreeWidget->setColumnCount(3);
        QStringList headerList;
        headerList.append("Name");
        headerList.append("Viz");
        headerList.append("Data");
        mTreeWidget->setHeaderLabels(headerList);

        // disable the move of section to have column order fixed
        mTreeWidget->header()->setSectionsMovable(false);

        // set the name column width
        mTreeWidget->setColumnWidth(0, 150);

        // set the viz section fixed
        mTreeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);
        mTreeWidget->setColumnWidth(1, 24);

        // connect the tree widget
        connect(mTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(OnSelectionChanged()));
        connect(mTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(OnGroupCollapsed(QTreeWidgetItem*)));
        connect(mTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(OnGroupExpanded(QTreeWidgetItem*)));

        // create and fill the vertical layout
        mVerticalLayout = new QVBoxLayout();
        mVerticalLayout->setObjectName("StyledWidget");
        mVerticalLayout->setSpacing(2);
        mVerticalLayout->setMargin(0);
        mVerticalLayout->setAlignment(Qt::AlignTop);
        mVerticalLayout->addLayout(buttonsLayout);
        mVerticalLayout->addWidget(mTreeWidget);

        // set the object name
        setObjectName("StyledWidget");

        // create the fake widget and layout
        QWidget* fakeWidget = new QWidget();
        fakeWidget->setObjectName("StyledWidget");
        fakeWidget->setLayout(mVerticalLayout);

        QVBoxLayout* fakeLayout = new QVBoxLayout();
        fakeLayout->setMargin(0);
        fakeLayout->setSpacing(0);
        fakeLayout->setObjectName("StyledWidget");
        fakeLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
        fakeLayout->addWidget(fakeWidget);

        // set the layout
        setLayout(fakeLayout);

        // set the focus policy
        setFocusPolicy(Qt::ClickFocus);

        // init
        Init();
    }


    // destructor
    ParameterWindow::~ParameterWindow()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mCreateCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAddGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustGroupCallback, false);
        delete mCreateCallback;
        delete mRemoveCallback;
        delete mAdjustCallback;
        delete mAddGroupCallback;
        delete mRemoveGroupCallback;
        delete mAdjustGroupCallback;
    }


    // check if the gamepad control mode is enabled for the given parameter and if its actually being controlled or not
    void ParameterWindow::GetGamepadState(EMotionFX::AnimGraph* animGraph, MCore::AttributeSettings* attributeSettings, bool* outIsActuallyControlled, bool* outIsEnabled)
    {
        *outIsActuallyControlled = false;
        *outIsEnabled = false;

        // get the game controller settings and its active preset
        EMotionFX::AnimGraphGameControllerSettings* gameControllerSettings = animGraph->GetGameControllerSettings();
        EMotionFX::AnimGraphGameControllerSettings::Preset* preset = gameControllerSettings->GetActivePreset();

        // get access to the game controller and check if it is valid
        bool isGameControllerValid = false;
#ifdef HAS_GAME_CONTROLLER
        GameControllerWindow* gameControllerWindow = mPlugin->GetGameControllerWindow();
        if (gameControllerWindow)
        {
            isGameControllerValid = gameControllerWindow->GetIsGameControllerValid();
        }
#endif

        // only update in case a preset is selected and the game controller is valid
        if (preset && isGameControllerValid)
        {
            // check if the given parameter is controlled by a joystick
            EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* controllerParameterInfo = preset->FindParameterInfo(attributeSettings->GetName());
            if (controllerParameterInfo)
            {
                // set the gamepad controlled enable flag
                if (controllerParameterInfo->mEnabled)
                {
                    *outIsEnabled = true;
                }

                // when the axis is not set to "None"
                if (controllerParameterInfo->mAxis != MCORE_INVALIDINDEX8)
                {
                    *outIsActuallyControlled = true;
                }
            }

            // check if the given parameter is controlled by a gamepad button
            if (preset->CheckIfIsParameterButtonControlled(attributeSettings->GetName()))
            {
                *outIsActuallyControlled = true;
            }
            if (preset->CheckIfIsButtonEnabled(attributeSettings->GetName()))
            {
                *outIsEnabled = true;
            }
        }
    }


    // helper function to update all parameter and button infos
    void ParameterWindow::SetGamepadState(EMotionFX::AnimGraph* animGraph, MCore::AttributeSettings* attributeSettings, bool isEnabled)
    {
        // get the game controller settings and its active preset
        EMotionFX::AnimGraphGameControllerSettings* gameControllerSettings = animGraph->GetGameControllerSettings();
        EMotionFX::AnimGraphGameControllerSettings::Preset* preset = gameControllerSettings->GetActivePreset();

        // get access to the game controller and check if it is valid
        bool isGameControllerValid = false;

#ifdef HAS_GAME_CONTROLLER
        GameControllerWindow* gameControllerWindow = mPlugin->GetGameControllerWindow();
        if (gameControllerWindow)
        {
            isGameControllerValid = gameControllerWindow->GetIsGameControllerValid();
        }
#endif

        // only update in case a preset is selected and the game controller is valid
        if (preset && isGameControllerValid)
        {
            // check if the given parameter is controlled by a joystick and set its enabled state
            EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* controllerParameterInfo = preset->FindParameterInfo(attributeSettings->GetName());
            if (controllerParameterInfo)
            {
                controllerParameterInfo->mEnabled = isEnabled;
            }

            // do the same for the button infos
            preset->SetButtonEnabled(attributeSettings->GetName(), isEnabled);
        }
    }


    // add a parameter to the interface
    void ParameterWindow::AddParameterToInterface(EMotionFX::AnimGraph* animGraph, uint32 parameterIndex, QTreeWidgetItem* parameterGroupItem)
    {
        MCORE_ASSERT(parameterIndex < animGraph->GetNumParameters());
        MCore::AttributeSettings* attributeSettings = animGraph->GetParameter(parameterIndex);

        // make sure we only show the parameters that are wanted after the name are filtering
        if (!mFilterString.empty() && !attributeSettings->GetNameString().Contains(mFilterString.c_str()))
        {
            return;
        }

        // apply attributes for all actor instances
        MCore::Array<MCore::Attribute*> attributes;
        const uint32 numInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances();
        for (uint32 j = 0; j < numInstances; ++j)
        {
            // get the anim graph instance
            EMotionFX::AnimGraphInstance* animGraphInstance = GetCommandManager()->GetCurrentSelection().GetActorInstance(j)->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // only update actor instances using this anim graph
            if (animGraphInstance->GetAnimGraph() != animGraph)
            {
                continue;
            }

            // add the attribute
            attributes.Add(animGraphInstance->GetParameterValue(parameterIndex));
        }

        // add the parameter to the tree widget
        QTreeWidgetItem* item = new QTreeWidgetItem(parameterGroupItem);
        item->setText(0, attributeSettings->GetName());
        item->setToolTip(0, attributeSettings->GetDescription());
        item->setExpanded(true);
        parameterGroupItem->addChild(item);

        // check if the given parameter is selected
        if (GetIsParameterSelected(attributeSettings->GetName()))
        {
            item->setSelected(true);

            if (mEnsureVisibility)
            {
                mTreeWidget->scrollToItem(item);
                mEnsureVisibility = false;
            }
        }

        // check if the interface item needs to be read only or not
        bool isActuallyControlled, isEnabled;
        GetGamepadState(animGraph, attributeSettings, &isActuallyControlled, &isEnabled);

        // create the attribute and add it to the layout
        const bool readOnly = (isActuallyControlled && isEnabled);
        MysticQt::AttributeWidget* attributeWidget = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->CreateAttributeWidget(attributes, attributeSettings, nullptr, readOnly);
        attributeWidget->setToolTip(attributeSettings->GetDescription());
        mAttributeWidgets.push_back(attributeWidget);
        mTreeWidget->setItemWidget(item, 2, attributeWidget);

        // create the gizmo widget in case the parameter is currently not being controlled by the gamepad
        QWidget* gizmoWidget = nullptr;
        if (isActuallyControlled)
        {
            QPushButton* gizmoButton = new QPushButton();
            gizmoButton->setCheckable(true);
            gizmoButton->setChecked(isEnabled);
            SetGamepadButtonTooltip(gizmoButton);
            gizmoButton->setProperty("attributeInfo", attributeSettings->GetName());
            gizmoWidget = gizmoButton;
            connect(gizmoButton, SIGNAL(clicked()), this, SLOT(OnGamepadControlToggle()));
        }
        else
        {
            gizmoWidget = attributeWidget->CreateGizmoWidget();
        }

        if (gizmoWidget)
        {
            mTreeWidget->setItemWidget(item, 1, gizmoWidget);
        }
    }


    // set the tooltip for a checkable gamepad gizmo button based on the state
    void ParameterWindow::SetGamepadButtonTooltip(QPushButton* button)
    {
        if (button->isChecked())
        {
            EMStudioManager::MakeTransparentButton(button, "Images/Icons/Gamepad.png", "Parameter is currently being controlled by the gamepad", 20, 17);
        }
        else
        {
            EMStudioManager::MakeTransparentButton(button, "Images/Icons/GamepadDisabled.png", "Click button to enable gamepad control", 20, 17);
        }
    }


    // triggered when pressing one of the gamepad gizmo buttons
    void ParameterWindow::OnGamepadControlToggle()
    {
        if (EMotionFX::GetRecorder().GetIsInPlayMode() && EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            return;
        }

        MCORE_ASSERT(sender()->inherits("QPushButton"));
        QPushButton* button = qobject_cast<QPushButton*>(sender());
        SetGamepadButtonTooltip(button);

        const AZStd::string attributeInfoName = button->property("attributeInfo").toString().toUtf8().data();

        MCore::AttributeSettings* attributeSettings = mAnimGraph->FindParameter(attributeInfoName.c_str());
        if (attributeSettings)
        {
            // update the game controller settings
            SetGamepadState(mAnimGraph, attributeSettings, button->isChecked());

            // update the interface
            MysticQt::AttributeWidget* attributeWidget = FindAttributeWidget(attributeSettings);
            if (attributeWidget)
            {
                attributeWidget->SetReadOnly(button->isChecked());
            }
        }
    }


    // enable/disable recording/playback mode
    void ParameterWindow::OnRecorderStateChanged()
    {
        const bool readOnly = (EMotionFX::GetRecorder().GetIsInPlayMode()); // disable when in playback mode, enable otherwise
        if (mAnimGraph)
        {
            const uint32 numParams = mAnimGraph->GetNumParameters();
            for (uint32 i = 0; i < numParams; ++i)
            {
                MCore::AttributeSettings* attributeSettings = mAnimGraph->GetParameter(i);

                // update the interface
                MysticQt::AttributeWidget* attributeWidget = FindAttributeWidget(attributeSettings);
                if (attributeWidget)
                {
                    attributeWidget->SetReadOnly(readOnly);
                }
            }

            // update parameter values
            UpdateParameterValues();
        }

        // update the interface
        UpdateInterface();
    }


    // update the interface attribute widgets with current parameter values
    void ParameterWindow::UpdateParameterValues()
    {
        // get the selected actor instance
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            return;
        }

        if (mAnimGraph == nullptr)
        {
            return;
        }

        // get the anim graph instance for the selected actor instance
        EMotionFX::AnimGraphInstance* animGraphInstance = nullptr;
        animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            if (animGraphInstance->GetAnimGraph() != mAnimGraph) // if the selected anim graph instance isn't equal to the one of the actor instance
            {
                return;
            }
        }
        else
        {
            return;
        }

        // get the number of parameters and iterate through them
        const uint32 numParameters = mAnimGraph->GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            // get the attribute value
            MCore::Attribute* attribute = animGraphInstance->GetParameterValue(i);

            // update the interface
            MCore::AttributeSettings* attributeSettings = mAnimGraph->GetParameter(i);
            MysticQt::AttributeWidget* attributeWidget = FindAttributeWidget(attributeSettings);
            if (attributeWidget)
            {
                attributeWidget->SetValue(attribute);
            }
        }
    }


    // reset the name column width
    void ParameterWindow::ResetNameColumnWidth()
    {
        mTreeWidget->setColumnWidth(0, 150);
    }


    // resize the name column to contents
    void ParameterWindow::ResizeNameColumnToContents()
    {
        mTreeWidget->resizeColumnToContents(0);
    }


    // create the list of parameters
    void ParameterWindow::Init()
    {
        uint32 i;

        mLockSelection = true;

        // clear the parameter tree and the widget table
        mWidgetTable.clear();
        mTreeWidget->clear();
        mAttributeWidgets.clear();

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        mAnimGraph = animGraph;
        if (animGraph == nullptr)
        {
            // update the interface and return
            UpdateInterface();
            mLockSelection = false;
            return;
        }

        // only allow one actor or none instance to be selected
        if (GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances() > 1)
        {
            // update the interface and return
            UpdateInterface();
            mLockSelection = false;
            return;
        }

        //MCore::LOG("PreInit:: param=%s, group=%s", mSelectedParameterName.AsChar(), mSelectedParameterGroupName.AsChar());

        // get the selected actor instance
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        EMotionFX::AnimGraphInstance* animGraphInstance = nullptr;
        if (actorInstance)
        {
            animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance)
            {
                if (animGraphInstance->GetAnimGraph() != animGraph) // if the selected anim graph instance isn't equal to the one of the actor instance
                {
                    animGraphInstance = nullptr;
                }
            }
        }

        // default parameter group
        QTreeWidgetItem* defaultGroupItem = new QTreeWidgetItem(mTreeWidget);
        defaultGroupItem->setText(0, "Default");
        defaultGroupItem->setText(1, "");
        defaultGroupItem->setExpanded(true);
        mTreeWidget->addTopLevelItem(defaultGroupItem);

        // add all parameters that belong to no parameter group
        const uint32 numParameters = animGraph->GetNumParameters();
        for (i = 0; i < numParameters; ++i)
        {
            if (animGraph->FindParameterGroupForParameter(i))
            {
                continue;
            }

            AddParameterToInterface(animGraph, i, defaultGroupItem);
        }

        // get the number of parameter groups and iterate through them
        AZStd::string tempString;
        const uint32 numGroups = animGraph->GetNumParameterGroups();
        for (uint32 g = 0; g < numGroups; ++g)
        {
            EMotionFX::AnimGraphParameterGroup* group = animGraph->GetParameterGroup(g);

            // add the group item to the tree widget
            QTreeWidgetItem* groupItem = new QTreeWidgetItem(mTreeWidget);

            groupItem->setText(0, group->GetName());

            groupItem->setExpanded(!group->GetIsCollapsed());
            tempString = AZStd::string::format("%d Parameters", group->GetNumParameters());
            groupItem->setToolTip(1, tempString.c_str());
            mTreeWidget->addTopLevelItem(groupItem);

            // check if the given parameter is selected
            if (GetIsParameterGroupSelected(group->GetName()))
            {
                groupItem->setSelected(true);

                if (mEnsureVisibility)
                {
                    mTreeWidget->scrollToItem(groupItem);
                    mEnsureVisibility = false;
                }
            }

            // add all parameters that belong to the given group
            for (i = 0; i < numParameters; ++i)
            {
                if (animGraph->FindParameterGroupForParameter(i) != group)
                {
                    continue;
                }

                AddParameterToInterface(animGraph, i, groupItem);
            }
        }

        mLockSelection = false;

        UpdateInterface();
    }


    void ParameterWindow::SingleSelectParameter(const char* parameterName, bool ensureVisibility, bool updateInterface)
    {
        mSelectedParameterNames.clear();
        mSelectedParameterNames.push_back(parameterName);

        mSelectedParameterGroupNames.clear();

        mEnsureVisibility = ensureVisibility;

        if (updateInterface)
        {
            UpdateInterface();
        }
    }


    void ParameterWindow::SingleSelectParameterGroup(const char* groupName, bool ensureVisibility, bool updateInterface)
    {
        mSelectedParameterNames.clear();

        mSelectedParameterGroupNames.clear();
        mSelectedParameterGroupNames.push_back(groupName);

        mEnsureVisibility = ensureVisibility;

        if (updateInterface)
        {
            UpdateInterface();
        }
    }



    void ParameterWindow::FilterStringChanged(const QString& text)
    {
        mFilterString = text.toUtf8().data();
        Init();
    }


    void ParameterWindow::OnSelectionChanged()
    {
        // update the local arrays which store the selected parameter groups and parameter names
        UpdateSelectionArrays();

        // update the interface
        UpdateInterface();
    }


    void ParameterWindow::CanMove(bool* outMoveUpPossible, bool* outMoveDownPossible)
    {
        // init the values
        *outMoveUpPossible = false;
        *outMoveDownPossible = false;

        // get the active anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // check if we have the default group selected
        // on this case we do nothing more because the default group can not be moved
        if (GetIsDefaultParameterGroupSingleSelected())
        {
            return;
        }

        // the parameter case
        const uint32 parameterIndex = GetSingleSelectedParameterIndex();
        if (parameterIndex != MCORE_INVALIDINDEX32)
        {
            // find the parameter group index where the parameter is inside, invalid index if the parameter is in the default group
            EMotionFX::AnimGraphParameterGroup*    parameterGroup      = animGraph->FindParameterGroupForParameter(parameterIndex);
            const uint32                            parameterGroupIndex = animGraph->FindParameterGroupIndex(parameterGroup);

            // move up possible only if a neighbor is found or if the parameter is not in the first parameter group
            // the default group is always the first so we can simply check if the parameter is not inside it using the invalid index
            if (GetNeighborParameterIndex(animGraph, parameterIndex, true) != MCORE_INVALIDINDEX32 || parameterGroupIndex != MCORE_INVALIDINDEX32)
            {
                *outMoveUpPossible = true;
            }
            else
            {
                *outMoveUpPossible = false;
            }

            // move down possible only if a neighbor is found or if the parameter is not in the last parameter group, it's needed to check the case where the parameter is inside the default group, on this case we simply check if one parameter group exists
            if (GetNeighborParameterIndex(animGraph, parameterIndex, false) != MCORE_INVALIDINDEX32 || (parameterGroupIndex == MCORE_INVALIDINDEX32 && animGraph->GetNumParameterGroups() > 0) || parameterGroupIndex < (animGraph->GetNumParameterGroups() - 1))
            {
                *outMoveDownPossible = true;
            }
            else
            {
                *outMoveDownPossible = false;
            }
        }

        // the parameter group case
        const AZStd::string parameterGroupName = GetSingleSelectedParameterGroupName();
        if (!parameterGroupName.empty())
        {
            // find the parameter group index by name
            const uint32 parameterGroupIndex = animGraph->FindParameterGroupIndexByName(parameterGroupName.c_str());

            // check if the parameter group index is found
            if (parameterGroupIndex != MCORE_INVALIDINDEX32)
            {
                // move up possible only if the parameter group is not the first
                if (parameterGroupIndex > 0)
                {
                    *outMoveUpPossible = true;
                }

                // move down possible only if the parameter group is not the last
                if (parameterGroupIndex < (animGraph->GetNumParameterGroups() - 1))
                {
                    *outMoveDownPossible = true;
                }
            }
        }
    }


    // update the interface
    void ParameterWindow::UpdateInterface()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr || EMotionFX::GetRecorder().GetIsInPlayMode() || EMotionFX::GetRecorder().GetIsRecording())
        {
            mAddButton->setEnabled(false);
            mRemoveButton->setEnabled(false);
            mClearButton->setEnabled(false);
            mEditButton->setEnabled(false);
            mMoveUpButton->setEnabled(false);
            mMoveDownButton->setEnabled(false);
            return;
        }

        const bool isDefaultGroupSingleSelected = GetIsDefaultParameterGroupSingleSelected();

        // always allow to add a parameter when there is a anim graph selected
        mAddButton->setEnabled(true);

        // enable the clear button in case we have more than zero parameters
        const uint32 numParameters      = animGraph->GetNumParameters();
        const uint32 numParameterGroups = animGraph->GetNumParameterGroups();
        if (numParameters > 0 || numParameterGroups > 0)
        {
            mClearButton->setEnabled(true);
        }
        else
        {
            mClearButton->setEnabled(false);
        }

        // only disable the remove button if we single selected the default group or have nothing selected
        mRemoveButton->setEnabled(true);
        if (isDefaultGroupSingleSelected || (mSelectedParameterNames.empty() && mSelectedParameterGroupNames.empty()))
        {
            mRemoveButton->setEnabled(false);
        }

        // check if we can move up/down the currently single selected item
        bool moveUpPossible, moveDownPossible;
        CanMove(&moveUpPossible, &moveDownPossible);

        mMoveUpButton->setEnabled(moveUpPossible);
        mMoveDownButton->setEnabled(moveDownPossible);

        // enable the edit button in case we have a parameter selected
        if (GetSingleSelectedParameterIndex() != MCORE_INVALIDINDEX32 && isDefaultGroupSingleSelected == false)
        {
            mEditButton->setEnabled(true);
        }
        else
        {
            mEditButton->setEnabled(false);
        }
    }


    // add a new parameter
    void ParameterWindow::OnAddParameter()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // generate a unique parameter name
        MCore::String uniqueParameterName;
        uniqueParameterName.GenerateUniqueString("Parameter",   [&](const MCore::String& value)
            {
                return (animGraph->FindParameter(value.AsChar()) == nullptr);
            });

        // show the create parameter dialog
        ParameterCreateEditDialog dialog(mPlugin, this);
        dialog.SetName(uniqueParameterName.AsChar());
        dialog.GetAttributeSettings()->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_FLOATSLIDER);
        dialog.Init();
        if (dialog.exec() == QDialog::Rejected)
        {
            return;
        }

        //------------------------
        MCore::String       commandResult;
        AZStd::string       commandString;
        MCore::CommandGroup commandGroup("Add parameter");

        // Construct the create parameter command and add it to the command group.
        CommandSystem::ConstructCreateParameterCommand(commandString,
            animGraph,
            dialog.GetName().c_str(),
            dialog.GetAttributeSettings()->GetInterfaceType(),
            dialog.GetMinValue(),
            dialog.GetMaxValue(),
            dialog.GetDefaultValue(),
            dialog.GetDescription(),
            MCORE_INVALIDINDEX32,
            dialog.GetIsScalable());
        commandGroup.AddCommandString(commandString);

        const AZStd::string selectedParameterName   = GetSingleSelectedParameterName();
        const AZStd::string selectedGroupName       = GetSingleSelectedParameterGroupName();

        // if we have a group selected add the new parameter to this group
        if (!selectedGroupName.empty())
        {
            commandString = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %d -name \"%s\" -parameterNames \"%s\" -action \"add\"", animGraph->GetID(), selectedGroupName.c_str(), dialog.GetName().c_str());
            commandGroup.AddCommandString(commandString);
        }
        else if (!selectedParameterName.empty())
        {
            EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupForParameter(GetSingleSelectedParameterIndex());
            if (parameterGroup)
            {
                commandString = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %d -name \"%s\" -parameterNames \"%s\" -action \"add\"", animGraph->GetID(), parameterGroup->GetName(), dialog.GetName().c_str());
                commandGroup.AddCommandString(commandString);
            }
        }

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // edit a new parameter
    void ParameterWindow::OnEditButton()
    {
        // get the selected parameter index and make sure it is valid
        const uint32 parameterIndex = GetSingleSelectedParameterIndex();
        if (parameterIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the parameter info
        MCore::AttributeSettings* parameter = animGraph->GetParameter(parameterIndex);
        const MCore::String oldName = parameter->GetName();

        // create and init the dialog
        ParameterCreateEditDialog dialog(mPlugin, this, true);
        dialog.SetName(parameter->GetName());
        dialog.SetDescription(parameter->GetDescription());
        dialog.SetNumAttributes(3);
        dialog.SetDefaultValue(parameter->GetDefaultValue()->Clone());
        dialog.SetMinValue(parameter->GetMinValue()->Clone());
        dialog.SetMaxValue(parameter->GetMaxValue()->Clone());

        // show the dialog
        dialog.Init();
        if (dialog.exec() == QDialog::Rejected)
        {
            return;
        }

        //------------------------
        AZStd::string commandString;
        MCore::String resultString;
        MCore::String minValue;
        MCore::String maxValue;
        MCore::String defaultValue;

        // convert the values to strings
        dialog.GetMinValue()->ConvertToString(minValue);
        dialog.GetMaxValue()->ConvertToString(maxValue);
        dialog.GetDefaultValue()->ConvertToString(defaultValue);

        // convert the interface type into a string
        uint32 interfaceType = dialog.GetAttributeSettings()->GetInterfaceType();

        // Build the command string and execute it.
        if (dialog.GetDescription().empty())
        {
            commandString = AZStd::string::format("AnimGraphAdjustParameter -animGraphID %i -name \"%s\" -newName \"%s\" -interfaceType %i -minValue \"%s\" -maxValue \"%s\" -defaultValue \"%s\" -isScalable %s", animGraph->GetID(), oldName.AsChar(), dialog.GetName().c_str(), interfaceType, minValue.AsChar(), maxValue.AsChar(), defaultValue.AsChar(), dialog.GetIsScalable() ? "true" : "false");
        }
        else
        {
            commandString = AZStd::string::format("AnimGraphAdjustParameter -animGraphID %i -name \"%s\" -newName \"%s\" -description \"%s\" -interfaceType %i -minValue \"%s\" -maxValue \"%s\" -defaultValue \"%s\" -isScalable %s", animGraph->GetID(), oldName.AsChar(), dialog.GetName().c_str(), dialog.GetDescription().c_str(), interfaceType, minValue.AsChar(), maxValue.AsChar(), defaultValue.AsChar(), dialog.GetIsScalable() ? "true" : "false");
        }

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    const char* ParameterWindow::GetSingleSelectedParameterGroupName() const
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return "";
        }

        // make sure we only have exactly one selected item
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        if (selectedItems.count() != 1)
        {
            return "";
        }

        // get the selected item and make sure we haven't selected a group
        QTreeWidgetItem* selectedItem = selectedItems[0];
        if (selectedItem->parent())
        {
            return "";
        }

        if (selectedItem->text(0) == "Default")
        {
            return "";
        }

        // find the parameter group and return its name
        EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupByName(FromQtString(selectedItem->text(0)).AsChar());
        if (parameterGroup)
        {
            return parameterGroup->GetName();
        }

        // return failure
        return "";
    }


    void ParameterWindow::UpdateSelectionArrays()
    {
        // only update the selection in case it is not locked
        if (mLockSelection)
        {
            return;
        }

        // get the anim graph and clear the selection
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        mSelectedParameterNames.clear();
        mSelectedParameterGroupNames.clear();
        if (!animGraph)
        {
            return;
        }

        // make sure we only have exactly one selected item
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        int32 numSelectedItems = selectedItems.count();

        for (int32 i = 0; i < numSelectedItems; ++i)
        {
            // get the selected item
            QTreeWidgetItem* selectedItem = selectedItems[i];

            // in case the item has no parent we are dealing with a parameter group
            if (!selectedItem->parent())
            {
                mSelectedParameterGroupNames.push_back(selectedItem->text(0).toUtf8().data());
            }
            else // normal parameter
            {
                mSelectedParameterNames.push_back(selectedItem->text(0).toUtf8().data());
            }
        }
    }


    // get the index of the selected parameter
    uint32 ParameterWindow::GetSingleSelectedParameterIndex() const
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        // find and return the index of the parameter in the anim graph
        return animGraph->FindParameterIndex(GetSingleSelectedParameterName());
    }


    // remove the selected parameters and groups
    void ParameterWindow::OnRemoveButton()
    {
        {
            // log the parameters and the parameter groups
            EMotionFX::AnimGraph* logAnimGraph = mPlugin->GetActiveAnimGraph();
            const uint32 logNumParams = logAnimGraph->GetNumParameters();
            MCore::LogInfo("=================================================");
            MCore::LogInfo("Parameters: (%i)", logNumParams);
            for (uint32 p = 0; p < logNumParams; ++p)
            {
                MCore::LogInfo("Parameter #%i: Name='%s'", p, logAnimGraph->GetParameter(p)->GetName());
            }
            const uint32 logNumGroups = logAnimGraph->GetNumParameterGroups();
            MCore::LogInfo("Parameter Groups: (%i)", logNumGroups);
            for (uint32 g = 0; g < logNumGroups; ++g)
            {
                EMotionFX::AnimGraphParameterGroup* paramGroup = logAnimGraph->GetParameterGroup(g);
                MCore::LogInfo("Parameter Group #%i: Name='%s'", g, paramGroup->GetName());
                const uint32 numGroupParams = paramGroup->GetNumParameters();
                for (uint32 u = 0; u < numGroupParams; ++u)
                {
                    MCore::LogInfo("   + Parameter: Index=%i, Name='%s'", paramGroup->GetParameter(u), logAnimGraph->GetParameter(paramGroup->GetParameter(u))->GetName());
                }
            }
        }

        int32 i;

        // check if the anim graph is valid
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Remove parameters/groups");

        AZStd::vector<AZStd::string> selectedParameterNames = mSelectedParameterNames;
        bool askToDeleteParamsOfGroup = false;

        // get the number of selected parameter groups and iterate through them
        const int32 numSelectedGroups = static_cast<int32>(mSelectedParameterGroupNames.size());
        if (numSelectedGroups > 0)
        {
            for (i = numSelectedGroups - 1; i >= 0; i--)
            {
                // remove the parameter group, invalid group is the default group, on this case it's not possible to remove the group and not needed to move parameters because they are already in the default group
                EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupByName(mSelectedParameterGroupNames[static_cast<size_t>(i)].c_str());
                if (parameterGroup)
                {
                    // remove the parameter group
                    CommandSystem::RemoveParameterGroup(animGraph, parameterGroup, false, &commandGroup);

                    // check if we have selected all parameters inside the group
                    // if not we should ask if we want to remove them along with the group
                    const uint32 numParametersInGroup = parameterGroup->GetNumParameters();
                    for (uint32 p = 0; p < numParametersInGroup; ++p)
                    {
                        const uint32    parameterIndex  = parameterGroup->GetParameter(p);
                        const char*     parameterName   = animGraph->GetParameter(parameterIndex)->GetName();
                        if (AZStd::find(selectedParameterNames.begin(), selectedParameterNames.end(), parameterName) == selectedParameterNames.end())
                        {
                            askToDeleteParamsOfGroup = true;
                        }
                    }
                }
            }
        }

        if (askToDeleteParamsOfGroup)
        {
            if (QMessageBox::question(this, "Remove Parameters Along With The Groups?", "Would you also like to remove the parameters inside the group? Clicking no will move them into the default parameter group.", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
            {
                for (i = 0; i < numSelectedGroups; ++i)
                {
                    // remove the parameter group
                    EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupByName(mSelectedParameterGroupNames[i].c_str());
                    if (parameterGroup)
                    {
                        const uint32 numParametersInGroup = parameterGroup->GetNumParameters();
                        for (uint32 p = 0; p < numParametersInGroup; ++p)
                        {
                            const uint32    parameterIndex  = parameterGroup->GetParameter(p);
                            const char*     parameterName   = animGraph->GetParameter(parameterIndex)->GetName();
                            if (AZStd::find(selectedParameterNames.begin(), selectedParameterNames.end(), parameterName) == selectedParameterNames.end())
                            {
                                selectedParameterNames.push_back(parameterName);
                            }
                        }
                    }
                }
            }
        }

        // get the number of selected parameters and iterate through them
        CommandSystem::RemoveParametersCommand(animGraph, selectedParameterNames, &commandGroup);

        // Execute the command group.
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // remove all parameters and groups
    void ParameterWindow::OnClearButton()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // ask the user if he really wants to remove all parameters
        if (QMessageBox::question(this, "Remove All Groups And Parameters?", "Are you sure you want to remove all parameters and all parameter groups from the anim graph?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Clear parameters/groups");

        // add the commands to remove all groups and parameters
        CommandSystem::ClearParameterGroups(animGraph, &commandGroup);
        CommandSystem::ClearParametersCommand(animGraph, &commandGroup);

        // Execute the command group.
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // move the parameter up in the list
    void ParameterWindow::OnMoveParameterUp()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();

        // get the selected parameter index and make sure it is valid
        const uint32 parameterIndex = GetSingleSelectedParameterIndex();
        if (parameterIndex != MCORE_INVALIDINDEX32)
        {
            uint32 switchParameterIndex = GetNeighborParameterIndex(animGraph, parameterIndex, true);
            if (switchParameterIndex != MCORE_INVALIDINDEX32)
            {
                MCore::AttributeSettings* parameterA = animGraph->GetParameter(parameterIndex);
                MCore::AttributeSettings* parameterB = animGraph->GetParameter(switchParameterIndex);

                // Construct and execute the swap parameters command
                AZStd::string result;
                const AZStd::string commandString = AZStd::string::format("AnimGraphSwapParameters -animGraphID %d -what \"%s\" -with \"%s\"", animGraph->GetID(), parameterA->GetName(), parameterB->GetName());
                if (!EMStudio::GetCommandManager()->ExecuteCommand(commandString, result))
                {
                    AZ_Error("EMotionFX", false, result.c_str());
                }
            }
            else
            {
                // get the parameter group to which the selected parameter belongs to
                MCore::AttributeSettings* parameter = animGraph->GetParameter(parameterIndex);
                EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupForParameter(parameterIndex);
                if (parameterGroup)
                {
                    const uint32 parameterGroupIndex = animGraph->FindParameterGroupIndex(parameterGroup);
                    MCORE_ASSERT(parameterGroupIndex != MCORE_INVALIDINDEX32);

                    // build the command string
                    AZStd::string commandString;
                    AZStd::string commandResult;

                    AZStd::string newGroupName;
                    if (parameterGroupIndex > 0)
                    {
                        newGroupName = animGraph->GetParameterGroup(parameterGroupIndex - 1)->GetName();
                    }

                    if (!newGroupName.empty())
                    {
                        commandString = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %d -name \"%s\" -parameterNames \"%s\" -action \"add\"", animGraph->GetID(), newGroupName.c_str(), parameter->GetName());
                    }
                    else
                    {
                        commandString = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %d -parameterNames \"%s\" -action \"remove\"", animGraph->GetID(), parameter->GetName());
                    }

                    // Execute the command.
                    AZStd::string result;
                    if (!EMStudio::GetCommandManager()->ExecuteCommand(commandString, result))
                    {
                        AZ_Error("EMotionFX", false, result.c_str());
                    }
                }
            }
        }

        // get the currently selected parameter group
        const AZStd::string parameterGroupName = GetSingleSelectedParameterGroupName();
        if (!parameterGroupName.empty())
        {
            // find the parameter group index by name
            const uint32 parameterGroupIndex = animGraph->FindParameterGroupIndexByName(parameterGroupName.c_str());
            if (parameterGroupIndex != MCORE_INVALIDINDEX32)
            {
                CommandSystem::MoveParameterGroupCommand(animGraph, parameterGroupIndex, parameterGroupIndex - 1);
            }
        }
    }


    // move parameter down in the list
    void ParameterWindow::OnMoveParameterDown()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();

        // get the selected parameter index and make sure it is valid
        const uint32 parameterIndex = GetSingleSelectedParameterIndex();
        if (parameterIndex != MCORE_INVALIDINDEX32)
        {
            uint32 switchParameterIndex = GetNeighborParameterIndex(animGraph, parameterIndex, false);
            if (switchParameterIndex != MCORE_INVALIDINDEX32)
            {
                MCore::AttributeSettings* parameterA = animGraph->GetParameter(parameterIndex);
                MCore::AttributeSettings* parameterB = animGraph->GetParameter(switchParameterIndex);

                // Construct and execute the swap parameters command.
                AZStd::string result;
                const AZStd::string command = AZStd::string::format("AnimGraphSwapParameters -animGraphID %d -what \"%s\" -with \"%s\"", animGraph->GetID(), parameterA->GetName(), parameterB->GetName());
                if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
                {
                    AZ_Error("EMotionFX", false, result.c_str());
                }
            }
            else
            {
                // get the parameter group to which the selected parameter belongs to
                MCore::AttributeSettings* parameter = animGraph->GetParameter(parameterIndex);
                EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupForParameter(parameterIndex);

                AZStd::string newGroupName;

                if (parameterGroup)
                {
                    const uint32 parameterGroupIndex = animGraph->FindParameterGroupIndex(parameterGroup);
                    MCORE_ASSERT(parameterGroupIndex != MCORE_INVALIDINDEX32);

                    if (parameterGroupIndex < animGraph->GetNumParameterGroups() - 1)
                    {
                        newGroupName = animGraph->GetParameterGroup(parameterGroupIndex + 1)->GetName();
                    }
                }
                else
                {
                    if (animGraph->GetNumParameterGroups() > 0)
                    {
                        newGroupName = animGraph->GetParameterGroup(0)->GetName();
                    }
                }

                if (!newGroupName.empty())
                {
                    const AZStd::string command = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %d -name \"%s\" -parameterNames \"%s\" -action \"add\"", animGraph->GetID(), newGroupName.c_str(), parameter->GetName());
                    
                    // Execute the command.
                    AZStd::string result;
                    if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
                    {
                        AZ_Error("EMotionFX", false, result.c_str());
                    }
                }
            }

            return;
        }

        // get the currently selected parameter group
        const AZStd::string parameterGroupName = GetSingleSelectedParameterGroupName();
        if (!parameterGroupName.empty())
        {
            // find the parameter group index by name
            const uint32 parameterGroupIndex = animGraph->FindParameterGroupIndexByName(parameterGroupName.c_str());
            if (parameterGroupIndex != MCORE_INVALIDINDEX32)
            {
                CommandSystem::MoveParameterGroupCommand(animGraph, parameterGroupIndex, parameterGroupIndex + 1);
            }
        }
    }


    uint32 ParameterWindow::GetNeighborParameterIndex(EMotionFX::AnimGraph* animGraph, uint32 parameterIndex, bool upper)
    {
        // in case there are no parameters inside the anim graph, return directly
        if (animGraph->GetNumParameters() == 0)
        {
            return MCORE_INVALIDINDEX32;
        }

        // get the parameter group to which the selected parameter belongs to
        EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupForParameter(parameterIndex);
        if (parameterGroup == nullptr)
        {
            if (upper)
            {
                // find the upper parameter inside the default group
                //          const uint32 numParameters = animGraph->GetNumParameters();
                for (int32 i = parameterIndex - 1; i >= 0; i--)
                {
                    if (animGraph->FindParameterGroupForParameter(i) == nullptr)
                    {
                        return i;
                    }
                }
            }
            else
            {
                // find the lower parameter inside the default group
                const uint32 numParameters = animGraph->GetNumParameters();
                for (uint32 i = parameterIndex + 1; i < numParameters; i++)
                {
                    if (animGraph->FindParameterGroupForParameter(i) == nullptr)
                    {
                        return i;
                    }
                }
            }
        }
        else
        {
            // get the local parameter group index of the parameter
            const uint32 localIndex = parameterGroup->FindLocalParameterIndex(parameterIndex);
            if (localIndex == MCORE_INVALIDINDEX32)
            {
                MCORE_ASSERT(localIndex != MCORE_INVALIDINDEX32);
                return MCORE_INVALIDINDEX32;
            }

            if (upper)
            {
                // in case there is a parameter up the current one in the group
                if (localIndex > 0)
                {
                    return parameterGroup->GetParameter(localIndex - 1);
                }
            }
            else
            {
                // in case there is a parameter down the current one in the group
                if (localIndex < parameterGroup->GetNumParameters() - 1)
                {
                    return parameterGroup->GetParameter(localIndex + 1);
                }
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    bool ParameterWindow::GetIsDefaultParameterGroupSingleSelected()
    {
        // check if we have the default group selected, return false directly in this case
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        if (selectedItems.count() == 1)
        {
            QTreeWidgetItem* selectedItem = selectedItems[0];
            if (selectedItem->parent() == nullptr && selectedItem->text(0) == "Default")
            {
                return true;
            }
        }

        return false;
    }


    void ParameterWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        EMotionFX::AnimGraph*  animGraph  = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr || EMotionFX::GetRecorder().GetIsInPlayMode() || EMotionFX::GetRecorder().GetIsRecording())
        {
            return;
        }

        // create the context menu
        QMenu menu(this);

        const bool isDefaultGroupSingleSelected = GetIsDefaultParameterGroupSingleSelected();

        // get the selected parameter index and make sure it is valid
        const uint32 parameterIndex = GetSingleSelectedParameterIndex();
        if (parameterIndex != MCORE_INVALIDINDEX32 && isDefaultGroupSingleSelected == false)
        {
            // make the current value the default value for this parameter
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance && actorInstance->GetAnimGraphInstance())
            {
                QAction* makeDefaultAction = menu.addAction("Make Default Value");
                makeDefaultAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/Refresh.png")); // TODO: replace with a new icon?
                connect(makeDefaultAction, SIGNAL(triggered()), this, SLOT(OnMakeDefaultValue()));
            }

            // edit action
            QAction* editAction = menu.addAction("Edit");
            editAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Edit.png"));
            connect(editAction, SIGNAL(triggered()), this, SLOT(OnEditButton()));
        }
        else
        {
            const AZStd::string selectedGroupName = GetSingleSelectedParameterGroupName();
            EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupByName(selectedGroupName.c_str());
            if (parameterGroup && isDefaultGroupSingleSelected == false)
            {
                // rename group action
                QAction* renameGroupAction = menu.addAction("Rename Group");
                renameGroupAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Edit.png"));
                connect(renameGroupAction, SIGNAL(triggered()), this, SLOT(OnRenameGroup()));
            }
        }

        if (!mSelectedParameterNames.empty())
        {
            menu.addSeparator();

            // select parameter group action
            QMenu* groupMenu = new QMenu("Assign To Group", &menu);
            QAction* noneGroupAction = groupMenu->addAction("Default");
            noneGroupAction->setCheckable(true);

            if (animGraph->FindParameterGroupForParameter(parameterIndex) == nullptr && parameterIndex != MCORE_INVALIDINDEX32)
            {
                noneGroupAction->setChecked(true);
            }
            else
            {
                noneGroupAction->setChecked(false);
            }

            connect(noneGroupAction, SIGNAL(triggered()), this, SLOT(OnParameterGroupSelected()));


            // get the number of parameter groups and iterate through them
            const uint32 numGroups = animGraph->GetNumParameterGroups();
            for (uint32 i = 0; i < numGroups; ++i)
            {
                // get the parameter group
                EMotionFX::AnimGraphParameterGroup* group = animGraph->GetParameterGroup(i);

                QAction* groupAction = groupMenu->addAction(group->GetName());
                groupAction->setCheckable(true);

                if (animGraph->FindParameterGroupForParameter(parameterIndex) == group)
                {
                    groupAction->setChecked(true);
                }
                else
                {
                    groupAction->setChecked(false);
                }

                connect(groupAction, SIGNAL(triggered()), this, SLOT(OnParameterGroupSelected()));
            }

            menu.addMenu(groupMenu);
        }

        menu.addSeparator();

        // check if we can move up/down the currently single selected item
        bool moveUpPossible, moveDownPossible;
        CanMove(&moveUpPossible, &moveDownPossible);

        // move up action
        if (moveUpPossible)
        {
            QAction* moveUpAction = menu.addAction("Move Up");
            moveUpAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/UpArrow.png"));
            connect(moveUpAction, SIGNAL(triggered()), this, SLOT(OnMoveParameterUp()));
        }

        // move down action
        if (moveDownPossible)
        {
            QAction* moveDownAction = menu.addAction("Move Down");
            moveDownAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/DownArrow.png"));
            connect(moveDownAction, SIGNAL(triggered()), this, SLOT(OnMoveParameterDown()));
        }

        menu.addSeparator();

        // add group action
        QAction* addGroupAction = menu.addAction("Add Group");
        addGroupAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
        connect(addGroupAction, SIGNAL(triggered()), this, SLOT(OnAddGroupButton()));

        menu.addSeparator();

        // remove action
        if (!(isDefaultGroupSingleSelected || (mSelectedParameterNames.empty() && mSelectedParameterGroupNames.empty())))
        {
            QAction* removeAction = menu.addAction("Remove");
            removeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Remove.png"));
            connect(removeAction, SIGNAL(triggered()), this, SLOT(OnRemoveButton()));
        }

        // clear action
        if (animGraph->GetNumParameters() > 0 || animGraph->GetNumParameterGroups() > 0)
        {
            QAction* clearAction = menu.addAction("Clear");
            clearAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Clear.png"));
            connect(clearAction, SIGNAL(triggered()), this, SLOT(OnClearButton()));
        }

        // show the menu at the given position
        if (menu.isEmpty() == false)
        {
            menu.exec(event->globalPos());
        }
    }


    void ParameterWindow::OnParameterGroupSelected()
    {
        assert(sender()->inherits("QAction"));
        QAction* action = qobject_cast<QAction*>(sender());

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the number of selected parameters and return directly in case there aren't any selected
        const size_t numSelectedParameters = mSelectedParameterNames.size();
        if (numSelectedParameters == 0)
        {
            return;
        }

        // construct the name of the parameter group
        AZStd::string commandGroupName;
        if (numSelectedParameters == 1)
        {
            commandGroupName = "Assign parameter to group";
        }
        else
        {
            commandGroupName = "Assign parameters to group";
        }

        AZStd::string commandString;
        MCore::CommandGroup commandGroup(commandGroupName.c_str());

        // target parameter group
        const AZStd::string parameterGroupName = action->text().toUtf8().data();
        EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupByName(parameterGroupName.c_str());

        // iterate through the selected parameters and
        for (uint32 i = 0; i < numSelectedParameters; ++i)
        {
            // get the index of the selected parameter
            const uint32 parameterIndex = animGraph->FindParameterIndex(mSelectedParameterNames[i].c_str());
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                continue;
            }

            // get the parameter group name from the action and search the parameter group
            MCore::AttributeSettings* attributeSettings = animGraph->GetParameter(parameterIndex);

            if (parameterGroup)
            {
                commandString = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %d -name \"%s\" -parameterNames \"%s\" -action \"add\"", animGraph->GetID(), parameterGroup->GetName(), attributeSettings->GetName());
            }
            else
            {
                commandString = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %d -parameterNames \"%s\" -action \"clear\"", animGraph->GetID(), attributeSettings->GetName());
            }

            commandGroup.AddCommandString(commandString);
        }

        // Execute the command group.
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // make it the default value
    void ParameterWindow::OnMakeDefaultValue()
    {
        const uint32 parameterIndex = GetSingleSelectedParameterIndex();
        if (parameterIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the current selected anim graph
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            QMessageBox::warning(this, "Selection Issue", "We cannot perform this operation while you have multiple actor instances selected!");
            return;
        }

        // get the attribute info
        const AZStd::string selectedParamName = GetSingleSelectedParameterName();
        const uint32 animGraphParamIndex = animGraph->FindParameterIndex(selectedParamName.c_str());
        MCore::AttributeSettings* attributeSettings = animGraph->GetParameter(animGraphParamIndex);

        // get the anim graph instance if there is any
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (!animGraphInstance)
        {
            return;
        }

        // init the parameter default value from the current value
        attributeSettings->GetDefaultValue()->InitFrom(animGraphInstance->GetParameterValue(animGraphParamIndex));
        animGraph->SetDirtyFlag(true);
    }


    void ParameterWindow::OnAddGroupButton()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // fill in the invalid names array
        AZStd::vector<AZStd::string> invalidNames;
        invalidNames.push_back("Default");
        const uint32 numParameterGroups = animGraph->GetNumParameterGroups();
        for (uint32 i = 0; i < numParameterGroups; ++i)
        {
            invalidNames.push_back(animGraph->GetParameterGroup(i)->GetName());
        }

        // generate a unique group name
        MCore::String uniqueGroupName;
        uniqueGroupName.GenerateUniqueString("ParameterGroup",  [&](const MCore::String& value)
            {
                return (animGraph->FindParameterGroupByName(value.AsChar()) == nullptr);
            });

        // show the create window
        ParameterCreateRenameWindow createWindow("Create Group", "Please enter the group name:", uniqueGroupName.AsChar(), "", invalidNames, this);
        if (createWindow.exec() != QDialog::Accepted)
        {
            return;
        }

        // Construct command.
        const AZStd::string command = AZStd::string::format("AnimGraphAddParameterGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), createWindow.GetName().c_str());

        // select our new group directly (this needs UpdateInterface() to be called, but the command does that internally)
        SingleSelectParameterGroup(createWindow.GetName().c_str(), true);

        // Execute command.
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void ParameterWindow::OnRenameGroup()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            return;
        }

        // get the currently selected parameter group, return in case none is selected
        const AZStd::string parameterGroupName = GetSingleSelectedParameterGroupName();
        if (parameterGroupName.empty())
        {
            return;
        }

        // find the parameter group by name
        EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupByName(parameterGroupName.c_str());
        if (!parameterGroup)
        {
            return;
        }

        // fill in the invalid names array
        AZStd::vector<AZStd::string> invalidNames;
        invalidNames.push_back("Default");
        const uint32 numParameterGroups = animGraph->GetNumParameterGroups();
        for (uint32 i = 0; i < numParameterGroups; ++i)
        {
            if (animGraph->GetParameterGroup(i) != parameterGroup)
            {
                invalidNames.push_back(animGraph->GetParameterGroup(i)->GetName());
            }
        }

        // show the rename window
        ParameterCreateRenameWindow renameWindow("Rename Group", "Please enter the new group name:", parameterGroup->GetName(), parameterGroup->GetName(), invalidNames, this);
        if (renameWindow.exec() != QDialog::Accepted)
        {
            return;
        }

        // Build and execute command.
        const AZStd::string command = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %i -name \"%s\" -newName \"%s\"", animGraph->GetID(), parameterGroup->GetName(), renameWindow.GetName().c_str());
        
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void ParameterWindow::OnGroupExpanded(QTreeWidgetItem* item)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // find the parameter group
        EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupByName(FromQtString(item->text(0)).AsChar());
        if (parameterGroup)
        {
            parameterGroup->SetIsCollapsed(false);
        }
    }


    void ParameterWindow::OnGroupCollapsed(QTreeWidgetItem* item)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // find the parameter group
        EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupByName(FromQtString(item->text(0)).AsChar());
        if (parameterGroup)
        {
            parameterGroup->SetIsCollapsed(true);
        }
    }


    void ParameterWindow::keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Delete:
        {
            OnRemoveButton();
            event->accept();
            break;
        }
        case Qt::Key_PageUp:
        {
            OnMoveParameterUp();
            event->accept();
            break;
        }
        case Qt::Key_PageDown:
        {
            OnMoveParameterDown();
            event->accept();
            break;
        }
        default:
            event->ignore();
        }
    }


    void ParameterWindow::keyReleaseEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Delete:
        {
            event->accept();
            break;
        }
        case Qt::Key_PageUp:
        {
            event->accept();
            break;
        }
        case Qt::Key_PageDown:
        {
            event->accept();
            break;
        }
        default:
            event->ignore();
        }
    }


    // Find the attribute widget for a given attribute settings.
    MysticQt::AttributeWidget* ParameterWindow::FindAttributeWidget(MCore::AttributeSettings* attributeSettings)
    {
        for (MysticQt::AttributeWidget* attributeWidget : mAttributeWidgets)
        {
            if (attributeWidget->GetAttributeInfo() == attributeSettings)
            {
                return attributeWidget;
            }
        }

        return nullptr;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // Command callbacks
    //----------------------------------------------------------------------------------------------------------------------------------

    bool ParameterWindowCommandCallback()
    {
        // get the plugin object
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        EMotionFX::AnimGraph* animGraph = animGraphPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return false;
        }

        //bool syncHappened = false;

        // get the number of visual graphs and iterate through them
        const uint32 numGraphs = animGraphPlugin->GetNumGraphInfos();
        for (uint32 i = 0; i < numGraphs; ++i)
        {
            // get the current visual graph
            AnimGraphPlugin::GraphInfo*    graphInfo   = animGraphPlugin->GetGraphInfo(i);
            NodeGraph*                      visualGraph = graphInfo->mVisualGraph;

            // check if the visual graph belongs to the given anim graph, if not skip it
            if (animGraph != graphInfo->mAnimGraph)
            {
                continue;
            }

            // get the number of nodes inside the graph and iterate through them
            const uint32 numNodes = visualGraph->GetNumNodes();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                // get the current node and check if this is a blend graph node
                GraphNode* curNode = visualGraph->GetNode(n);
                if (curNode->GetType() != BlendTreeVisualNode::TYPE_ID)
                {
                    continue;
                }

                // check if it is a parameter node and sync the node with the emfx one
                BlendTreeVisualNode* blendTreeVisualNode = static_cast<BlendTreeVisualNode*>(curNode);
                if (blendTreeVisualNode->GetEMFXNode()->GetType() == EMotionFX::BlendTreeParameterNode::TYPE_ID)
                {
                    blendTreeVisualNode->Sync();
                    //syncHappened = true;
                }
            }
        }

        // update the parameter window and resize the name column
        animGraphPlugin->GetParameterWindow()->Init();
        animGraphPlugin->GetParameterWindow()->ResizeNameColumnToContents();
        return true;
    }

    bool ParameterWindow::CommandCreateBlendParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandCreateBlendParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the remove parameter command which calls the callback already

    bool ParameterWindow::CommandRemoveBlendParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandRemoveBlendParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the create parameter command which calls the callback already

    bool ParameterWindow::CommandAdjustBlendParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandAdjustBlendParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the adjust parameter command which calls the callback already

    bool ParameterWindow::CommandAnimGraphAddParameterGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandAnimGraphAddParameterGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)          { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the remove parameter group command which calls the callback already

    bool ParameterWindow::CommandAnimGraphRemoveParameterGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandAnimGraphRemoveParameterGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the create parameter group command which calls the callback already

    bool ParameterWindow::CommandAnimGraphAdjustParameterGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandAnimGraphAdjustParameterGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the adjust parameter group command which calls the callback already
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.moc>