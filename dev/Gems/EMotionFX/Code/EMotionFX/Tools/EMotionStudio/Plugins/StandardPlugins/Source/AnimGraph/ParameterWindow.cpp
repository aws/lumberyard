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
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphGroupParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphGameControllerSettings.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendTreeVisualNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GameControllerWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditDialog.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ParameterEditorFactory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ValueParameterEditor.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <EMotionFX/Source/Parameter/GroupParameter.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <QAction>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

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
        connect(mLineEdit, &QLineEdit::textChanged, this, &ParameterCreateRenameWindow::NameEditChanged);
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
        connect(mOKButton, &QPushButton::clicked, this, &ParameterCreateRenameWindow::accept);
        connect(mCancelButton, &QPushButton::clicked, this, &ParameterCreateRenameWindow::reject);
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
        mAddGroupCallback       = new CommandAnimGraphAddGroupParameterCallback(false);
        mRemoveGroupCallback    = new CommandAnimGraphRemoveGroupParameterCallback(false);
        mAdjustGroupCallback    = new CommandAnimGraphAdjustGroupParameterCallback(false);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateParameter", mCreateCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameter", mRemoveCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameter", mAdjustCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddGroupParameter", mAddGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveGroupParameter", mRemoveGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustGroupParameter", mAdjustGroupCallback);

        // add the add button
        mAddButton = new QPushButton("");
        EMStudioManager::MakeTransparentButton(mAddButton, "/Images/Icons/Plus.png", "Add new parameter");
        connect(mAddButton, &QPushButton::clicked, this, &ParameterWindow::OnAddParameter);

        // add the remove button
        mRemoveButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mRemoveButton, "/Images/Icons/Minus.png", "Remove selected parameters");
        connect(mRemoveButton, &QPushButton::clicked, this, &ParameterWindow::OnRemoveButton);

        // add the clear button
        mClearButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mClearButton, "/Images/Icons/Clear.png", "Remove all parameters");
        connect(mClearButton, &QPushButton::clicked, this, &ParameterWindow::OnClearButton);

        // add edit button
        mEditButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mEditButton, "/Images/Icons/Edit.png", "Edit selected parameter");
        connect(mEditButton, &QPushButton::clicked, this, &ParameterWindow::OnEditButton);

        // add move up button
        mMoveUpButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mMoveUpButton, "/Images/Icons/UpArrow.png", "Move selected parameter up");
        connect(mMoveUpButton, &QPushButton::clicked, this, &ParameterWindow::OnMoveParameterUp);

        // add move down button
        mMoveDownButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mMoveDownButton, "/Images/Icons/DownArrow.png", "Move selected parameter down");
        connect(mMoveDownButton, &QPushButton::clicked, this, &ParameterWindow::OnMoveParameterDown);

        // add the search filter button
        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &ParameterWindow::OnTextFilterChanged);
        m_searchWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        // add the buttons to the layout
        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        buttonsLayout->setSpacing(0);
        buttonsLayout->addWidget(mAddButton);
        buttonsLayout->addWidget(mRemoveButton);
        buttonsLayout->addWidget(mClearButton);
        buttonsLayout->addWidget(mMoveUpButton);
        buttonsLayout->addWidget(mMoveDownButton);
        buttonsLayout->addWidget(mEditButton);

        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        spacerWidget->setMinimumWidth(20);
        buttonsLayout->addWidget(spacerWidget);

        buttonsLayout->addWidget(m_searchWidget);
        
        // create the parameter tree widget
        mTreeWidget = new QTreeWidget();
        mTreeWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mTreeWidget->setObjectName("AnimGraphParamWindow");
        mTreeWidget->header()->setVisible(false);

        // adjust selection mode and enable some other helpful things
        mTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTreeWidget->setExpandsOnDoubleClick(true);
        mTreeWidget->setColumnCount(3);
        mTreeWidget->setUniformRowHeights(true);
        mTreeWidget->setIndentation(10);
        mTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        mTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        mTreeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);

        // connect the tree widget
        connect(mTreeWidget, &QTreeWidget::itemSelectionChanged, this, &ParameterWindow::OnSelectionChanged);
        connect(mTreeWidget, &QTreeWidget::itemCollapsed, this, &ParameterWindow::OnGroupCollapsed);
        connect(mTreeWidget, &QTreeWidget::itemExpanded, this, &ParameterWindow::OnGroupExpanded);

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

        setLayout(mVerticalLayout);

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
    void ParameterWindow::GetGamepadState(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, bool* outIsActuallyControlled, bool* outIsEnabled)
    {
        *outIsActuallyControlled = false;
        *outIsEnabled = false;

        // get the game controller settings and its active preset
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = animGraph->GetGameControllerSettings();
        EMotionFX::AnimGraphGameControllerSettings::Preset* preset = gameControllerSettings.GetActivePreset();

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
            EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* controllerParameterInfo = preset->FindParameterInfo(parameter->GetName().c_str());
            if (controllerParameterInfo)
            {
                // set the gamepad controlled enable flag
                if (controllerParameterInfo->m_enabled)
                {
                    *outIsEnabled = true;
                }

                // when the axis is not set to "None"
                if (controllerParameterInfo->m_axis != MCORE_INVALIDINDEX8)
                {
                    *outIsActuallyControlled = true;
                }
            }

            // check if the given parameter is controlled by a gamepad button
            if (preset->CheckIfIsParameterButtonControlled(parameter->GetName().c_str()))
            {
                *outIsActuallyControlled = true;
            }
            if (preset->CheckIfIsButtonEnabled(parameter->GetName().c_str()))
            {
                *outIsEnabled = true;
            }
        }
    }


    // helper function to update all parameter and button infos
    void ParameterWindow::SetGamepadState(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, bool isEnabled)
    {
        // get the game controller settings and its active preset
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = animGraph->GetGameControllerSettings();
        EMotionFX::AnimGraphGameControllerSettings::Preset* preset = gameControllerSettings.GetActivePreset();

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
            EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* controllerParameterInfo = preset->FindParameterInfo(parameter->GetName().c_str());
            if (controllerParameterInfo)
            {
                controllerParameterInfo->m_enabled = isEnabled;
            }

            // do the same for the button infos
            preset->SetButtonEnabled(parameter->GetName().c_str(), isEnabled);
        }
    }

    void ParameterWindow::AddParameterToInterface(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, QTreeWidgetItem* parentWidgetItem)
    {
        // Only filter value parameters
        if (!mFilterString.empty()
            && azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>()
            && AzFramework::StringFunc::Find(parameter->GetName().c_str(), mFilterString.c_str()) == AZStd::string::npos)
        {
            return;
        }

        QTreeWidgetItem* widgetItem = new QTreeWidgetItem(parentWidgetItem);
        widgetItem->setText(0, parameter->GetName().c_str());
        widgetItem->setExpanded(true);
        widgetItem->setData(0, Qt::UserRole, QString(parameter->GetName().c_str()));
        parentWidgetItem->addChild(widgetItem);

        // check if the given parameter is selected
        if (GetIsParameterSelected(parameter->GetName()))
        {
            widgetItem->setSelected(true);
            if (mEnsureVisibility)
            {
                mTreeWidget->scrollToItem(widgetItem);
                mEnsureVisibility = false;
            }
        }

        if (azrtti_typeid(parameter) == azrtti_typeid<EMotionFX::GroupParameter>())
        {
            const EMotionFX::GroupParameter* groupParameter = static_cast<const EMotionFX::GroupParameter*>(parameter);

            const AZStd::string tooltip = AZStd::string::format("%d Parameters", groupParameter->GetNumValueParameters());
            widgetItem->setToolTip(0, tooltip.c_str());
            widgetItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

            // add all parameters that belong to the given group
            const EMotionFX::ParameterVector& childParameters = groupParameter->GetChildParameters();
            for (const EMotionFX::Parameter* childParameter : childParameters)
            {
                AddParameterToInterface(animGraph, childParameter, widgetItem);
            }
        }
        else
        {
            const EMotionFX::ValueParameter* valueParameter = static_cast<const EMotionFX::ValueParameter*>(parameter);
            const AZ::Outcome<size_t> parameterIndex = animGraph->FindValueParameterIndex(valueParameter);
            AZ_Assert(parameterIndex.IsSuccess(), "Expected a parameter belonging to the the anim graph");

            // apply attributes for all actor instances
            AZStd::vector<MCore::Attribute*> attributes;
            const uint32 numInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances();
            for (uint32 j = 0; j < numInstances; ++j)
            {
                // only update actor instances using this anim graph
                EMotionFX::AnimGraphInstance* animGraphInstance = GetCommandManager()->GetCurrentSelection().GetActorInstance(j)->GetAnimGraphInstance();
                if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
                {
                    attributes.emplace_back(animGraphInstance->GetParameterValue(static_cast<uint32>(parameterIndex.GetValue())));
                }
            }

            // check if the interface item needs to be read only or not
            bool isActuallyControlled, isEnabled;
            GetGamepadState(animGraph, parameter, &isActuallyControlled, &isEnabled);

            // create the attribute and add it to the layout
            const bool readOnly = (isActuallyControlled && isEnabled) || attributes.empty();

            ParameterWidget parameterWidget;
            parameterWidget.m_valueParameterEditor.reset(ParameterEditorFactory::Create(animGraph, valueParameter, attributes));
            parameterWidget.m_valueParameterEditor->setIsReadOnly(readOnly);

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                return;
            }
            parameterWidget.m_propertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(mTreeWidget);
            parameterWidget.m_propertyEditor->SetSizeHintOffset(QSize(0, 0));
            parameterWidget.m_propertyEditor->SetAutoResizeLabels(false);
            parameterWidget.m_propertyEditor->SetLeafIndentation(0);
            parameterWidget.m_propertyEditor->setStyleSheet("QFrame, .QWidget, QSlider, QCheckBox { background-color: transparent }");

            parameterWidget.m_propertyEditor->AddInstance(parameterWidget.m_valueParameterEditor.get(), azrtti_typeid(parameterWidget.m_valueParameterEditor.get()));
            parameterWidget.m_propertyEditor->Setup(serializeContext, this, false, 0);
            parameterWidget.m_propertyEditor->SetSelectionEnabled(true);
            parameterWidget.m_propertyEditor->show();
            parameterWidget.m_propertyEditor->ExpandAll();
            parameterWidget.m_propertyEditor->InvalidateAll();

            mTreeWidget->setItemWidget(widgetItem, 2, parameterWidget.m_propertyEditor);

            // create the gizmo widget in case the parameter is currently not being controlled by the gamepad
            QWidget* gizmoWidget = nullptr;
            if (isActuallyControlled)
            {
                QPushButton* gizmoButton = new QPushButton();
                gizmoButton->setCheckable(true);
                gizmoButton->setChecked(isEnabled);
                SetGamepadButtonTooltip(gizmoButton);
                gizmoButton->setProperty("attributeInfo", parameter->GetName().c_str());
                gizmoWidget = gizmoButton;
                connect(gizmoButton, &QPushButton::clicked, this, &ParameterWindow::OnGamepadControlToggle);
            }
            else
            {
                AzToolsFramework::ReflectedPropertyEditor* rpe = parameterWidget.m_propertyEditor.data();
                gizmoWidget = parameterWidget.m_valueParameterEditor->CreateGizmoWidget(
                        [rpe]()
                        {
                            rpe->InvalidateValues();
                        }
                        );
            }
            if (gizmoWidget)
            {
                mTreeWidget->setItemWidget(widgetItem, 1, gizmoWidget);
                mTreeWidget->setColumnWidth(1, 20);
            }

            auto insertIt = m_parameterWidgets.emplace(parameter, AZStd::move(parameterWidget));
            if (!insertIt.second)
            {
                insertIt.first->second = AZStd::move(parameterWidget);
            }
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

    void ParameterWindow::BeforePropertyModified(AzToolsFramework::InstanceDataNode*)
    {
    }

    void ParameterWindow::AfterPropertyModified(AzToolsFramework::InstanceDataNode*)
    {
    }

    void ParameterWindow::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode*)
    {
    }

    void ParameterWindow::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode*)
    {
    }

    void ParameterWindow::SealUndoStack()
    {
    }

    void ParameterWindow::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint& point)
    {
        EMotionFX::AnimGraph*  animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr || EMotionFX::GetRecorder().GetIsInPlayMode() || EMotionFX::GetRecorder().GetIsRecording())
        {
            return;
        }

        // create the context menu
        QMenu menu(this);

        // get the selected parameter index and make sure it is valid
        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        if (parameter)
        {
            // make the current value the default value for this parameter
            if (azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>())
            {
                EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
                if (actorInstance && actorInstance->GetAnimGraphInstance())
                {
                    QAction* makeDefaultAction = menu.addAction("Make default value");
                    makeDefaultAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/Refresh.png")); // TODO: replace with a new icon?
                    connect(makeDefaultAction, &QAction::triggered, this, &ParameterWindow::OnMakeDefaultValue);
                }
            }
            else
            {
                // rename group action
                QAction* renameGroupAction = menu.addAction("Rename group");
                renameGroupAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Edit.png"));
                connect(renameGroupAction, &QAction::triggered, this, &ParameterWindow::OnRenameGroup);
            }

            // edit action
            QAction* editAction = menu.addAction("Edit");
            editAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Edit.png"));
            connect(editAction, &QAction::triggered, this, &ParameterWindow::OnEditButton);
        }
        if (!mSelectedParameterNames.empty())
        {
            menu.addSeparator();

            // select group parameter action
            QMenu* groupMenu = new QMenu("Assign to group", &menu);
            QAction* noneGroupAction = groupMenu->addAction("Default");
            noneGroupAction->setCheckable(true);

            if (!parameter)
            {
                noneGroupAction->setChecked(true);
            }
            else
            {
                noneGroupAction->setChecked(false);
            }

            connect(noneGroupAction, &QAction::triggered, this, &ParameterWindow::OnGroupParameterSelected);

            // get the parent of the parameter
            const EMotionFX::GroupParameter* parentGroup = nullptr;
            if (parameter)
            {
                parentGroup = animGraph->FindParentGroupParameter(parameter);
            }

            // get the number of group parameters and iterate through them
            EMotionFX::GroupParameterVector groupParametersInCurrentParameter;
            if (azrtti_typeid(parameter) == azrtti_typeid<EMotionFX::GroupParameter>())
            {
                const EMotionFX::GroupParameter* groupParameter = static_cast<const EMotionFX::GroupParameter*>(parameter);
                groupParametersInCurrentParameter = groupParameter->RecursivelyGetChildGroupParameters();
            }
            const EMotionFX::GroupParameterVector allGroupParameters = animGraph->RecursivelyGetGroupParameters();
            for (const EMotionFX::GroupParameter* groupParameter : allGroupParameters)
            {
                if (groupParameter != parameter
                    && AZStd::find(groupParametersInCurrentParameter.begin(),
                        groupParametersInCurrentParameter.end(),
                        groupParameter) == groupParametersInCurrentParameter.end())
                {
                    QAction* groupAction = groupMenu->addAction(groupParameter->GetName().c_str());
                    groupAction->setCheckable(true);
                    groupAction->setChecked(groupParameter == parentGroup);
                    connect(groupAction, &QAction::triggered, this, &ParameterWindow::OnGroupParameterSelected);
                }
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
            QAction* moveUpAction = menu.addAction("Move up");
            moveUpAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/UpArrow.png"));
            connect(moveUpAction, &QAction::triggered, this, &ParameterWindow::OnMoveParameterUp);
        }

        // move down action
        if (moveDownPossible)
        {
            QAction* moveDownAction = menu.addAction("Move down");
            moveDownAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/DownArrow.png"));
            connect(moveDownAction, &QAction::triggered, this, &ParameterWindow::OnMoveParameterDown);
        }

        menu.addSeparator();

        // add parameter action
        QAction* addParameter = menu.addAction("Add parameter");
        addParameter->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
        connect(addParameter, &QAction::triggered, this, &ParameterWindow::OnAddParameter);

        // add group action
        QAction* addGroupAction = menu.addAction("Add group");
        addGroupAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
        connect(addGroupAction, &QAction::triggered, this, &ParameterWindow::OnAddGroupButton);

        menu.addSeparator();

        // remove action
        if (!mSelectedParameterNames.empty())
        {
            QAction* removeAction = menu.addAction("Remove");
            removeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Remove.png"));
            connect(removeAction, &QAction::triggered, this, &ParameterWindow::OnRemoveButton);
        }

        // clear action
        if (animGraph->GetNumParameters() > 0)
        {
            QAction* clearAction = menu.addAction("Clear");
            clearAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Clear.png"));
            connect(clearAction, &QAction::triggered, this, &ParameterWindow::OnClearButton);
        }

        // show the menu at the given position
        if (menu.isEmpty() == false)
        {
            menu.exec(point);
        }
    }

    void ParameterWindow::PropertySelectionChanged(AzToolsFramework::InstanceDataNode*, bool)
    {
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

        const EMotionFX::Parameter* parameter = mAnimGraph->FindParameterByName(attributeInfoName);
        if (parameter)
        {
            // update the game controller settings
            SetGamepadState(mAnimGraph, parameter, button->isChecked());

            // update the interface
            ParameterWidgetByParameter::const_iterator paramWidgetIt = m_parameterWidgets.find(parameter);
            if (paramWidgetIt != m_parameterWidgets.end())
            {
                paramWidgetIt->second.m_valueParameterEditor->setIsReadOnly(button->isChecked());
                paramWidgetIt->second.m_propertyEditor->InvalidateAll();
            }
        }
    }


    // enable/disable recording/playback mode
    void ParameterWindow::OnRecorderStateChanged()
    {
        const bool readOnly = (EMotionFX::GetRecorder().GetIsInPlayMode()); // disable when in playback mode, enable otherwise
        if (mAnimGraph)
        {
            const EMotionFX::ValueParameterVector& valueParameters = mAnimGraph->RecursivelyGetValueParameters();
            for (const EMotionFX::ValueParameter* valueParameter : valueParameters)
            {
                // update the interface
                ParameterWidgetByParameter::const_iterator paramWidgetIt = m_parameterWidgets.find(valueParameter);
                if (paramWidgetIt != m_parameterWidgets.end())
                {
                    paramWidgetIt->second.m_valueParameterEditor->setIsReadOnly(readOnly);
                    paramWidgetIt->second.m_propertyEditor->InvalidateAll();
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
        for (ParameterWidgetByParameter::value_type& param : m_parameterWidgets)
        {
            param.second.m_valueParameterEditor->UpdateValue();
            param.second.m_propertyEditor->InvalidateValues();
        }
    }


    // create the list of parameters
    void ParameterWindow::Init()
    {
        mLockSelection = true;

        // clear the parameter tree and the widget table
        mTreeWidget->clear();
        m_parameterWidgets.clear();

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        mAnimGraph = animGraph;
        if (animGraph == nullptr
            || GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances() > 1) // only allow one actor or none instance to be selected
        {
            // update the interface and return
            UpdateInterface();
            mLockSelection = false;
            return;
        }

        // add all parameters, this will recursively add parameters to groups
        const EMotionFX::ParameterVector& childParameters = animGraph->GetChildParameters();
        for (const EMotionFX::Parameter* parameter : childParameters)
        {
            AddParameterToInterface(animGraph, parameter, mTreeWidget->invisibleRootItem());
        }

        mLockSelection = false;

        UpdateInterface();
    }


    void ParameterWindow::SingleSelectGroupParameter(const char* groupName, bool ensureVisibility, bool updateInterface)
    {
        mSelectedParameterNames.clear();

        mSelectedParameterNames.push_back(groupName);

        mEnsureVisibility = ensureVisibility;

        if (updateInterface)
        {
            UpdateInterface();
        }
    }


    void ParameterWindow::OnTextFilterChanged(const QString& text)
    {
        mFilterString = text.toUtf8().data();
        Init();
    }


    void ParameterWindow::OnSelectionChanged()
    {
        // update the local arrays which store the selected group parameters and parameter names
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

        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        if (!parameter)
        {
            return;
        }

        // To detect if we can move up or down, we are going to get a flat list of all the parameters belonging
        // to the same group (non-recursive) then find the parameter.
        // If the parameter is the first one, we can only move up if we are in a group (this will move the parameter
        // to the parent group, making it a sibling of the group)
        // If the parameter is the last of the list, then we can move down if are in a group
        const EMotionFX::GroupParameter* parentGroup = animGraph->FindParentGroupParameter(parameter);

        // If we have a parent group, we dont need to inspect the siblings, we can always move up/down
        if (parentGroup)
        {
            *outMoveUpPossible = true;
            *outMoveDownPossible = true;
            return;
        }

        const EMotionFX::ParameterVector& siblingParameters = parentGroup ? parentGroup->GetChildParameters() : animGraph->GetChildParameters();
        AZ_Assert(!siblingParameters.empty(), "Expected at least one parameter (the one we are analyzing)");

        if (*siblingParameters.begin() != parameter)
        {
            *outMoveUpPossible = true;
        }
        if (*siblingParameters.rbegin() != parameter)
        {
            *outMoveDownPossible = true;
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

        // always allow to add a parameter when there is a anim graph selected
        mAddButton->setEnabled(true);

        // enable the clear button in case we have more than zero parameters
        mClearButton->setEnabled(animGraph->GetNumParameters() > 0);

        // disable the remove and edit buttton if we dont have any parameter selected
        mRemoveButton->setEnabled(true);
        mEditButton->setEnabled(true);
        if (mSelectedParameterNames.empty())
        {
            mRemoveButton->setEnabled(false);
            mEditButton->setEnabled(false);
        }

        // check if we can move up/down the currently single selected item
        bool moveUpPossible, moveDownPossible;
        CanMove(&moveUpPossible, &moveDownPossible);

        mMoveUpButton->setEnabled(moveUpPossible);
        mMoveDownButton->setEnabled(moveDownPossible);
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

        // show the create parameter dialog
        ParameterCreateEditDialog dialog(mPlugin, this);
        dialog.Init();
        if (dialog.exec() == QDialog::Rejected)
        {
            return;
        }

        //------------------------
        AZStd::string       commandResult;
        AZStd::string       commandString;
        MCore::CommandGroup commandGroup("Add parameter");

        // Construct the create parameter command and add it to the command group.
        const AZStd::unique_ptr<EMotionFX::Parameter>& parameter = dialog.GetParameter();

        CommandSystem::ConstructCreateParameterCommand(commandString,
            animGraph,
            parameter.get(),
            MCORE_INVALIDINDEX32);
        commandGroup.AddCommandString(commandString);

        const EMotionFX::GroupParameter* parentGroup = nullptr;
        const EMotionFX::Parameter* selectedParameter = GetSingleSelectedParameter();
        // if we have a group selected add the new parameter to this group
        if (selectedParameter)
        {
            if (azrtti_typeid(selectedParameter) == azrtti_typeid<EMotionFX::GroupParameter>())
            {
                parentGroup = static_cast<const EMotionFX::GroupParameter*>(selectedParameter);
            }
            else
            {
                // add it as sibling of the current selected parameter
                parentGroup = mAnimGraph->FindParentGroupParameter(selectedParameter);
            }
        }
        if (parentGroup)
        {
            commandString = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %d -name \"%s\" -parameterNames \"%s\" -action \"add\"",
                    animGraph->GetID(),
                    parentGroup->GetName().c_str(),
                    parameter->GetName().c_str());
            commandGroup.AddCommandString(commandString);
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
        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        if (!parameter)
        {
            return;
        }

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // in case it gets renamed
        const AZStd::string oldName = parameter->GetName();

        // create and init the dialog
        ParameterCreateEditDialog dialog(mPlugin, this, parameter);
        dialog.Init();
        if (dialog.exec() == QDialog::Rejected)
        {
            return;
        }

        //------------------------
        AZStd::string commandString;
        AZStd::string resultString;

        // convert the interface type into a string
        const AZStd::unique_ptr<EMotionFX::Parameter>& editedParameter = dialog.GetParameter();
        const AZStd::string contents = MCore::ReflectionSerializer::Serialize(editedParameter.get()).GetValue();

        // Build the command string and execute it.
        commandString = AZStd::string::format("AnimGraphAdjustParameter -animGraphID %i -name \"%s\" -newName \"%s\" -type \"%s\" -contents {%s}",
                animGraph->GetID(),
                oldName.c_str(),
                editedParameter->GetName().c_str(),
                azrtti_typeid(editedParameter.get()).ToString<AZStd::string>().c_str(),
                contents.c_str());

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
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
            mSelectedParameterNames.emplace_back(selectedItem->data(0, Qt::UserRole).toString().toUtf8().data());
        }
    }


    // get the index of the selected parameter
    const EMotionFX::Parameter* ParameterWindow::GetSingleSelectedParameter() const
    {
        if (mSelectedParameterNames.size() != 1)
        {
            return nullptr;
        }

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return nullptr;
        }

        // find and return the index of the parameter in the anim graph
        return animGraph->FindParameterByName(mSelectedParameterNames[0]);
    }


    // remove the selected parameters and groups
    void ParameterWindow::OnRemoveButton()
    {
        if (MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_INFO)
        {
            // log the parameters and the group parameters
            EMotionFX::AnimGraph* logAnimGraph = mPlugin->GetActiveAnimGraph();
            const EMotionFX::ValueParameterVector& valueParameters = logAnimGraph->RecursivelyGetValueParameters();
            const size_t logNumParams = valueParameters.size();
            MCore::LogInfo("=================================================");
            MCore::LogInfo("Parameters: (%i)", logNumParams);
            for (size_t p = 0; p < logNumParams; ++p)
            {
                MCore::LogInfo("Parameter #%i: Name='%s'", p, valueParameters[p]->GetName().c_str());
            }
            const EMotionFX::GroupParameterVector groupParameters = logAnimGraph->RecursivelyGetGroupParameters();
            const size_t logNumGroups = groupParameters.size();
            MCore::LogInfo("Group parameters: (%i)", logNumGroups);
            for (uint32 g = 0; g < logNumGroups; ++g)
            {
                const EMotionFX::GroupParameter* groupParam = groupParameters[g];
                MCore::LogInfo("Group parameter #%i: Name='%s'", g, groupParam->GetName().c_str());
                const EMotionFX::ValueParameterVector childValueParams = groupParam->GetChildValueParameters();
                for (const EMotionFX::ValueParameter* childValueParam : childValueParams)
                {
                    MCore::LogInfo("   + Parameter: Name='%s'", childValueParam->GetName().c_str());
                }
            }
        }

        // check if the anim graph is valid
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Remove parameters/groups");

        AZStd::vector<AZStd::string> paramsOfSelectedGroup;
        AZStd::vector<AZStd::string> selectedValueParameters;

        // get the number of selected parameters and iterate through them
        for (const AZStd::string& selectedParameter : mSelectedParameterNames)
        {
            const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(selectedParameter);
            if (!parameter)
            {
                continue;
            }
            if (azrtti_typeid(parameter) == azrtti_typeid<EMotionFX::GroupParameter>())
            {
                // remove the group parameter
                const EMotionFX::GroupParameter* groupParameter = static_cast<const EMotionFX::GroupParameter*>(parameter);
                CommandSystem::RemoveGroupParameter(animGraph, groupParameter, false, &commandGroup);

                // check if we have selected all parameters inside the group
                // if not we should ask if we want to remove them along with the group
                const EMotionFX::ParameterVector parametersInGroup = groupParameter->RecursivelyGetChildParameters();
                for (const EMotionFX::Parameter* parameter : parametersInGroup)
                {
                    const AZStd::string& parameterName = parameter->GetName();
                    if (AZStd::find(mSelectedParameterNames.begin(), mSelectedParameterNames.end(), parameterName) == mSelectedParameterNames.end())
                    {
                        paramsOfSelectedGroup.push_back(parameterName);
                    }
                }
            }
            else
            {
                selectedValueParameters.push_back(selectedParameter);
            }
        }

        CommandSystem::BuildRemoveParametersCommandGroup(animGraph, selectedValueParameters, &commandGroup);

        if (!paramsOfSelectedGroup.empty())
        {
            const QMessageBox::StandardButton result = QMessageBox::question(this, "Remove parameters along with the groups?", "Would you also like to remove the parameters inside the group? Clicking no will move them into the root.",
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (result == QMessageBox::Yes)
            {
                // Remove the contained parameters, since they can be groups or regular parameters, we
                // iterate over them moving the groups to a different vector to be deleted after
                AZStd::vector<const EMotionFX::GroupParameter*> groupParameters;

                AZStd::vector<AZStd::string>::const_iterator it = paramsOfSelectedGroup.begin();
                while (it != paramsOfSelectedGroup.end())
                {
                    const EMotionFX::GroupParameter* groupParameter = animGraph->FindGroupParameterByName(*it);
                    if (groupParameter)
                    {
                        groupParameters.push_back(groupParameter);
                        it = paramsOfSelectedGroup.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                CommandSystem::BuildRemoveParametersCommandGroup(animGraph, paramsOfSelectedGroup, &commandGroup);
                for (const EMotionFX::GroupParameter* groupParameter : groupParameters)
                {
                    CommandSystem::RemoveGroupParameter(animGraph, groupParameter, false, &commandGroup);
                }
            }
        }

        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
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
        if (QMessageBox::question(this, "Remove all groups and parameters?", "Are you sure you want to remove all parameters and all group parameters from the anim graph?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Clear parameters/groups");

        // add the commands to remove all groups and parameters
        CommandSystem::ClearParametersCommand(animGraph, &commandGroup);
        CommandSystem::ClearGroupParameters(animGraph, &commandGroup);

        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
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
        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        if (parameter)
        {
            const EMotionFX::GroupParameter* parentGroup = animGraph->FindParentGroupParameter(parameter);
            const AZ::Outcome<size_t> relativeIndex = parentGroup ? parentGroup->FindRelativeParameterIndex(parameter) : animGraph->FindRelativeParameterIndex(parameter);
            AZ_Assert(relativeIndex.IsSuccess(), "Expected a valid index");

            AZStd::string commandString;
            if (relativeIndex.GetValue() != 0)
            {
                commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %d -name \"%s\" -index %d ",
                    animGraph->GetID(),
                    parameter->GetName().c_str(),
                    relativeIndex.GetValue() - 1);
                if (parentGroup)
                {
                    commandString += AZStd::string::format("-parent \"%s\"", parentGroup->GetName().c_str());
                }
            }
            else
            {
                // We need to move the parameter to the parent group and put it at the index where the parent group is
                AZ_Assert(parentGroup, "CanMove should have restricted this option");
                const EMotionFX::GroupParameter* grandparentGroup = animGraph->FindParentGroupParameter(parentGroup);
                const AZ::Outcome<size_t> parentRelativeIndex = grandparentGroup ? grandparentGroup->FindRelativeParameterIndex(parentGroup) : animGraph->FindRelativeParameterIndex(parentGroup);
                AZ_Assert(parentRelativeIndex.IsSuccess(), "Expected a valid index");

                commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %d -name \"%s\" -index %d ",
                        animGraph->GetID(),
                        parameter->GetName().c_str(),
                        parentRelativeIndex.GetValue()); // In this case we position the parameter in the position where the parent is
                if (grandparentGroup)
                {
                    commandString += AZStd::string::format("-parent \"%s\"", grandparentGroup->GetName().c_str());
                }
            }

            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommand(commandString, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    // move parameter down in the list
    void ParameterWindow::OnMoveParameterDown()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();

        // get the selected parameter index and make sure it is valid
        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        if (parameter)
        {
            const EMotionFX::GroupParameter* parentGroup = animGraph->FindParentGroupParameter(parameter);
            const AZ::Outcome<size_t> relativeIndex = parentGroup ? parentGroup->FindRelativeParameterIndex(parameter) : animGraph->FindRelativeParameterIndex(parameter);
            const size_t parentParameterCount = parentGroup ? parentGroup->GetChildParameters().size() : animGraph->GetChildParameters().size();
            AZ_Assert(relativeIndex.IsSuccess(), "Expected a valid index");

            AZStd::string commandString;
            if (relativeIndex.GetValue() != (parentParameterCount - 1))
            {
                commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %d -name \"%s\" -index %d ",
                        animGraph->GetID(),
                        parameter->GetName().c_str(),
                        relativeIndex.GetValue() + 1);
                if (parentGroup)
                {
                    commandString += AZStd::string::format("-parent \"%s\"", parentGroup->GetName().c_str());
                }
            }
            else
            {
                // We need to move the parameter to the parent group and put it after the index where the parent group is
                AZ_Assert(parentGroup, "CanMove should have restricted this option");
                const EMotionFX::GroupParameter* grandparentGroup = animGraph->FindParentGroupParameter(parentGroup);
                const AZ::Outcome<size_t> parentRelativeIndex = grandparentGroup ? grandparentGroup->FindRelativeParameterIndex(parentGroup) : animGraph->FindRelativeParameterIndex(parentGroup);
                AZ_Assert(parentRelativeIndex.IsSuccess(), "Expected a valid index");

                commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %d -name \"%s\" -index %d ",
                        animGraph->GetID(),
                        parameter->GetName().c_str(),
                        parentRelativeIndex.GetValue() + 1); // In this case we position the parameter after the position where the parent is
                if (grandparentGroup)
                {
                    commandString += AZStd::string::format("-parent \"%s\"", grandparentGroup->GetName().c_str());
                }
            }

            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommand(commandString, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void ParameterWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        RequestPropertyContextMenu(nullptr, event->globalPos());
    }


    void ParameterWindow::OnGroupParameterSelected()
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

        // construct the name of the group parameter
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

        // target group parameter
        const AZStd::string groupParameterName = action->text().toUtf8().data();
        const EMotionFX::GroupParameter* groupParameter = animGraph->FindGroupParameterByName(groupParameterName);

        if (groupParameter)
        {
            for (const AZStd::string& selectedParameterName : mSelectedParameterNames)
            {
                // get the selected parameter
                const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(selectedParameterName);
                if (parameter)
                {
                    commandString = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %d -name \"%s\" -parameterNames \"%s\" -action \"add\"",
                            animGraph->GetID(),
                            groupParameter->GetName().c_str(),
                            parameter->GetName().c_str());
                    commandGroup.AddCommandString(commandString);
                }
            }
        }
        else
        {
            for (const AZStd::string& selectedParameterName : mSelectedParameterNames)
            {
                // get the selected parameter
                const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(selectedParameterName);
                if (parameter)
                {
                    commandString = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %d -parameterNames \"%s\" -action \"clear\"",
                            animGraph->GetID(),
                            parameter->GetName().c_str());
                    commandGroup.AddCommandString(commandString);
                }
            }
        }

        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // make it the default value
    void ParameterWindow::OnMakeDefaultValue()
    {
        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        if (!parameter)
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
        const AZ::Outcome<size_t> parameterIndex = animGraph->FindParameterIndex(parameter);
        if (parameterIndex.IsSuccess() == false)
        {
            return;
        }

        // get the anim graph instance if there is any
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (!animGraphInstance)
        {
            return;
        }

        // init the parameter default value from the current value
        const MCore::Attribute* instanceValue = animGraphInstance->GetParameterValue(static_cast<uint32>(parameterIndex.GetValue()));
        AZStd::string defaultValue;
        instanceValue->ConvertToString(defaultValue);
        MCore::ReflectionSerializer::DeserializeIntoMember(const_cast<EMotionFX::Parameter*>(parameter), "defaultValue", defaultValue);
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
        const EMotionFX::GroupParameterVector groupParameters = animGraph->RecursivelyGetGroupParameters();
        for (const EMotionFX::GroupParameter* groupParameter : groupParameters)
        {
            invalidNames.push_back(groupParameter->GetName());
        }

        // generate a unique group name
        const AZStd::string uniqueGroupName = MCore::GenerateUniqueString("GroupParameter", [&invalidNames](const AZStd::string& value)
                {
                    return AZStd::find(invalidNames.begin(), invalidNames.end(), value) == invalidNames.end();
                });

        // show the create window
        ParameterCreateRenameWindow createWindow("Create Group", "Please enter the group name:", uniqueGroupName.c_str(), "", invalidNames, this);
        if (createWindow.exec() != QDialog::Accepted)
        {
            return;
        }

        AZStd::string command = AZStd::string::format("AnimGraphAddGroupParameter -animGraphID %i -name \"%s\"", animGraph->GetID(), createWindow.GetName().c_str());

        const EMotionFX::GroupParameter* parentGroup = nullptr;
        const EMotionFX::Parameter* selectedParameter = GetSingleSelectedParameter();
        // if we have a group selected add the new parameter to this group
        if (selectedParameter)
        {
            if (azrtti_typeid(selectedParameter) == azrtti_typeid<EMotionFX::GroupParameter>())
            {
                parentGroup = static_cast<const EMotionFX::GroupParameter*>(selectedParameter);
            }
            else
            {
                // add it as sibling of the current selected parameter
                parentGroup = mAnimGraph->FindParentGroupParameter(selectedParameter);
            }
        }
        if (parentGroup)
        {
            // create the group as a child of the current selected group parameter
            command += AZStd::string::format(" -parent \"%s\"", parentGroup->GetName().c_str());
        }

        // select our new group directly (this needs UpdateInterface() to be called, but the command does that internally)
        SingleSelectGroupParameter(createWindow.GetName().c_str(), true);

        // Execute command.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommand(command, result))
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

        // get the currently selected group parameter, return in case none is selected
        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        ;
        if (!parameter || azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>())
        {
            return;
        }
        const EMotionFX::GroupParameter* groupParameter = static_cast<const EMotionFX::GroupParameter*>(parameter);

        // fill in the invalid names array
        AZStd::vector<AZStd::string> invalidNames;
        const EMotionFX::GroupParameterVector groupParameters = animGraph->RecursivelyGetGroupParameters();
        for (const EMotionFX::GroupParameter* otherGroupParameter : groupParameters)
        {
            if (otherGroupParameter != groupParameter)
            {
                invalidNames.push_back(groupParameter->GetName());
            }
        }

        // show the rename window
        ParameterCreateRenameWindow renameWindow("Rename Group", "Please enter the new group name:", groupParameter->GetName().c_str(), groupParameter->GetName().c_str(), invalidNames, this);
        if (renameWindow.exec() != QDialog::Accepted)
        {
            return;
        }

        // Build and execute command.
        const AZStd::string command = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %i -name \"%s\" -newName \"%s\"", animGraph->GetID(), groupParameter->GetName().c_str(), renameWindow.GetName().c_str());

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void ParameterWindow::OnGroupExpanded(QTreeWidgetItem* /*item*/)
    {
        // Collapse/expanded state was being saved in the animgraph file. This can cause multiple users that are using the animgraph
        // to see the file dirty because of the collapsed state. This setting likely should be by user and stored in a separate file.
        // The RPE supports serializing this state.
        //// get the anim graph
        //EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        //if (animGraph == nullptr)
        //{
        //    return;
        //}

        //// find the group parameter
        //const EMotionFX::GroupParameter* groupParameter = animGraph->FindGroupParameterByName(FromQtString(item->text(0)));
        //if (groupParameter)
        //{
        //    groupParameter->SetIsCollapsed(false);
        //}
    }


    void ParameterWindow::OnGroupCollapsed(QTreeWidgetItem* /*item*/)
    {
        // Collapse/expanded state was being saved in the animgraph file. This can cause multiple users that are using the animgraph
        // to see the file dirty because of the collapsed state. This setting likely should be by user and stored in a separate file.
        // The RPE supports serializing this state.
        //// get the anim graph
        //EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        //if (animGraph == nullptr)
        //{
        //    return;
        //}

        //// find the group parameter
        //EMotionFX::GroupParameter* groupParameter = animGraph->FindGroupParameterByName(FromQtString(item->text(0)).c_str());
        //if (groupParameter)
        //{
        //    groupParameter->SetIsCollapsed(true);
        //}
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


    // Find the parameter widget for a given parameter.
    void ParameterWindow::UpdateParameterValue(const EMotionFX::Parameter* parameter)
    {
        ParameterWidgetByParameter::const_iterator itWidget = m_parameterWidgets.find(parameter);
        if (itWidget != m_parameterWidgets.end())
        {
            itWidget->second.m_valueParameterEditor->UpdateValue();
            itWidget->second.m_propertyEditor->InvalidateValues();
        }
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // Command callbacks
    //----------------------------------------------------------------------------------------------------------------------------------

    bool ParameterWindowCommandCallback()
    {
        // get the plugin object
        EMStudioPlugin* plugin = GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
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
                if (azrtti_typeid(blendTreeVisualNode->GetEMFXNode()) == azrtti_typeid<EMotionFX::BlendTreeParameterNode>())
                {
                    blendTreeVisualNode->Sync();
                    //syncHappened = true;
                }
            }
        }

        // update the parameter window
        animGraphPlugin->GetParameterWindow()->Init();
        return true;
    }

    bool ParameterWindow::CommandCreateBlendParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandCreateBlendParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the remove parameter command which calls the callback already

    bool ParameterWindow::CommandRemoveBlendParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandRemoveBlendParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the create parameter command which calls the callback already

    bool ParameterWindow::CommandAdjustBlendParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandAdjustBlendParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the adjust parameter command which calls the callback already

    bool ParameterWindow::CommandAnimGraphAddGroupParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandAnimGraphAddGroupParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)          { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the remove group parameter command which calls the callback already

    bool ParameterWindow::CommandAnimGraphRemoveGroupParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandAnimGraphRemoveGroupParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the create group parameter command which calls the callback already

    bool ParameterWindow::CommandAnimGraphAdjustGroupParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ParameterWindowCommandCallback(); }
    bool ParameterWindow::CommandAnimGraphAdjustGroupParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return true; }// internally calls the adjust group parameter command which calls the callback already
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.moc>