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

#include "AttributesWindow.h"
#include "AnimGraphPlugin.h"
#include "GraphNodeFactory.h"
#include "BlendGraphWidget.h"
#include "ConditionSelectDialog.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphStateCondition.h>
#include <EMotionFX/Source/AnimGraphPlayTimeCondition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MetricsEventSender.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphEditor.h>
#include <MCore/Source/ReflectionSerializer.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QGridLayout>
#include <QIcon>
#include <QScrollArea>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QContextMenuEvent>
#include <QTreeWidgetItem>
#include <QPixmap>
#include <QLabel>


namespace EMStudio
{
    AttributesWindow::AttributesWindow(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
        , m_objectEditor(nullptr)
    {
        mPlugin                 = plugin;
        mMainWidget             = nullptr;
        mObject                 = nullptr;
        mPasteConditionsWindow  = nullptr;
        mScrollArea             = new QScrollArea();

        mMainLayout = new QVBoxLayout();
        mMainLayout->setMargin(0);
        mMainLayout->setSpacing(1);
        setLayout(mMainLayout);

        mMainLayout->addWidget(mScrollArea);
        mScrollArea->setWidgetResizable(true);

        InitForAnimGraphObject(nullptr);
    }


    AttributesWindow::~AttributesWindow()
    {
    }


    // re-init the window
    void AttributesWindow::Reinit()
    {
        InitForAnimGraphObject(mObject);
    }


    QIcon AttributesWindow::GetIconForObject(EMotionFX::AnimGraphObject* object)
    {
        AZStd::string filename = AZStd::string::format("%s/Images/AnimGraphPlugin/%s.png", MysticQt::GetDataDir().c_str(), object->RTTI_GetTypeName());

        if (!QFile::exists(filename.c_str()))
        {
            filename = AZStd::string::format("%sImages/AnimGraphPlugin/AnimGraphStateMachine.png", MysticQt::GetDataDir().c_str());
        }

        return QIcon(filename.c_str());
    }


    void AttributesWindow::InitForAnimGraphObject(EMotionFX::AnimGraphObject* object)
    {
        // delete the existing property browser
        mMainWidget->deleteLater();
        mMainWidget = nullptr;
        mScrollArea->setVisible(false);
        mScrollArea->setWidget(nullptr);
        mObject = object;

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return;
        }

        // create the new property parent window
        mMainWidget = mPlugin->GetGraphNodeFactory()->CreateAttributeWidget(azrtti_typeid(object));
        if (mMainWidget == nullptr) // if there is no custom attribute widget, generate one
        {
            mMainWidget = new QWidget();
            mMainWidget->setVisible(false);

            QVBoxLayout* verticalLayout = new QVBoxLayout();
            mMainWidget->setLayout(verticalLayout);
            verticalLayout->setAlignment(Qt::AlignTop);
            verticalLayout->setMargin(0);
            verticalLayout->setSpacing(0);

            QVBoxLayout* verticalCardsLayout = new QVBoxLayout();
            verticalCardsLayout->setAlignment(Qt::AlignTop);
            verticalCardsLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
            verticalCardsLayout->setMargin(0);

            // Align the layout spacing with the entity inspector.
            verticalCardsLayout->setSpacing(13);


            if (object)
            {
                // 1. Create anim graph card.
                EMotionFX::AnimGraph* animGraph = object->GetAnimGraph();
                EMotionFX::AnimGraphEditor* animGraphEditor = new EMotionFX::AnimGraphEditor(animGraph, serializeContext, mMainWidget);
                verticalLayout->addWidget(animGraphEditor);

                // 2. Create object card
                m_objectEditor = new EMotionFX::ObjectEditor(serializeContext, this);

                m_objectCard = new AzQtComponents::Card(this);
                m_objectCard->setTitle(object->GetPaletteName());
                m_objectCard->setContentWidget(m_objectEditor);
                m_objectCard->setExpanded(true);

                AzQtComponents::CardHeader* cardHeader = m_objectCard->header();
                cardHeader->setHasContextMenu(false);
                cardHeader->setHelpURL(object->GetHelpUrl());

                m_objectEditor->AddInstance(object, azrtti_typeid(object));
                verticalCardsLayout->addWidget(m_objectCard);

                // 3. Add the condition cards and the add condition button in case we're dealing with a transition.
                CreateConditionsGUI(object, serializeContext, verticalCardsLayout);
            }
            else
            {
                // In case we're not inspecting a anim graph object, only create the anim graph card.
                EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
                if (animGraph)
                {
                    EMotionFX::AnimGraphEditor* animGraphEditor = new EMotionFX::AnimGraphEditor(animGraph, serializeContext, mMainWidget);
                    verticalLayout->addWidget(animGraphEditor);
                }
            }

            verticalLayout->addLayout(verticalCardsLayout);
        }

        mScrollArea->setWidget(mMainWidget);

        mMainWidget->setVisible(true);
        mScrollArea->show();
    }


    AzQtComponents::Card* AttributesWindow::CreateAnimGraphCard(AZ::SerializeContext* serializeContext, EMotionFX::AnimGraph* animGraph)
    {
        EMotionFX::AnimGraphEditor* animGraphEditor = new EMotionFX::AnimGraphEditor(animGraph, serializeContext, mMainWidget);

        AzQtComponents::Card* animGraphCard = new AzQtComponents::Card(this);
        animGraphCard->setTitle("Anim Graph");
        animGraphCard->setContentWidget(animGraphEditor);
        animGraphCard->setExpanded(true);

        AzQtComponents::CardHeader* cardHeader = animGraphCard->header();
        cardHeader->setHasContextMenu(false);

        return animGraphCard;
    }


    void AttributesWindow::CreateConditionsGUI(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, QVBoxLayout* mainLayout)
    {
        if (azrtti_typeid(object) != azrtti_typeid<EMotionFX::AnimGraphStateTransition>())
        {
            // Skip adding the condition UI elements in case the given object is not a transition.
            return;
        }

        const EMotionFX::AnimGraphStateTransition* stateTransition = static_cast<EMotionFX::AnimGraphStateTransition*>(object);

        const size_t numConditions = stateTransition->GetNumConditions();
        for (size_t c = 0; c < numConditions; ++c)
        {
            EMotionFX::AnimGraphTransitionCondition* condition = stateTransition->GetCondition(c);

            EMotionFX::ObjectEditor* conditionEditor = new EMotionFX::ObjectEditor(serializeContext, this);
            conditionEditor->AddInstance(condition, azrtti_typeid(condition));

            // Create the card and put the editor widget in it.
            AzQtComponents::Card* card = new AzQtComponents::Card(this);
            connect(card, &AzQtComponents::Card::contextMenuRequested, this, &AttributesWindow::OnConditionContextMenu);

            card->setTitle(condition->GetPaletteName());
            card->setContentWidget(conditionEditor);
            card->setProperty("conditionIndex", static_cast<unsigned int>(c));
            card->setExpanded(true);

            AzQtComponents::CardHeader* cardHeader = card->header();
            cardHeader->setHelpURL(condition->GetHelpUrl());

            mainLayout->addWidget(card);
        } // for all conditions

        QPushButton* addConditionButton = new QPushButton("Add condition");
        connect(addConditionButton, &QPushButton::clicked, this, &AttributesWindow::OnAddCondition);
        mainLayout->addWidget(addConditionButton);
    }


    void AttributesWindow::OnConditionContextMenu(const QPoint& position)
    {
        const AzQtComponents::Card* card = static_cast<AzQtComponents::Card*>(sender());
        const int conditionIndex = card->property("conditionIndex").toInt();

        QMenu contextMenu(this);

        QAction* addAction = contextMenu.addAction("Add condition");
        connect(addAction, &QAction::triggered, this, &AttributesWindow::OnAddCondition);

        QAction* deleteAction = contextMenu.addAction("Delete condition");
        deleteAction->setProperty("conditionIndex", conditionIndex);
        connect(deleteAction, &QAction::triggered, this, &AttributesWindow::OnRemoveCondition);

        if (!contextMenu.isEmpty())
        {
            contextMenu.exec(position);
        }
    }


    void AttributesWindow::OnAddCondition()
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            AZ_Error("EMotionFX", false, "Cannot add condition to transition. Anim graph is not valid.");
            return;
        }

        ConditionSelectDialog dialog(this);
        if (dialog.exec() == QDialog::Rejected)
        {
            return;
        }

        // get the selected transition condition type
        const AZ::TypeId selectedConditionType = dialog.GetSelectedConditionType();
        if (selectedConditionType.IsNull())
        {
            return;
        }

        MCORE_ASSERT(azrtti_istypeof<EMotionFX::AnimGraphStateTransition>(mObject));

        // add it to the transition
        EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(mObject);

        // get the source node of the transition and the parent of that node, which is the state machine in which the transition is stored at
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();

        // check if the parent node really is a state machine
        if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            // convert the node to a state machine
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            // execute the command
            AZStd::string commandString, outResult;
            commandString = AZStd::string::format("AnimGraphAddCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionType \"%s\"", 
                animGraph->GetID(), 
                stateMachine->GetName(), 
                transition->GetID(), 
                selectedConditionType.ToString<AZStd::string>().c_str());
            if (GetCommandManager()->ExecuteCommand(commandString.c_str(), outResult) == false)
            {
                if (outResult.empty() == false)
                {
                    MCore::LogError(outResult.c_str());
                }
            }

            if (transition->GetNumConditions() > 0)
            {
                EMotionFX::AnimGraphTransitionCondition* transitionCondition = transition->GetCondition(transition->GetNumConditions() - 1);

                if (azrtti_typeid(transitionCondition) == azrtti_typeid<EMotionFX::AnimGraphMotionCondition>())
                {
                    EMotionFX::AnimGraphMotionCondition* motionCondition = static_cast<EMotionFX::AnimGraphMotionCondition*>(transitionCondition);
                    
                    EMotionFX::AnimGraphNodeId newMotionNodeId;
                    if (transition->GetSourceNode() && azrtti_typeid(transition->GetSourceNode()) == azrtti_typeid<EMotionFX::AnimGraphMotionNode>())
                    {
                        newMotionNodeId = transition->GetSourceNode()->GetId();
                    }

                    motionCondition->SetMotionNodeId(newMotionNodeId);
                }

                if (azrtti_typeid(transitionCondition) == azrtti_typeid<EMotionFX::AnimGraphStateCondition>())
                {
                    EMotionFX::AnimGraphStateCondition* stateCondition = static_cast<EMotionFX::AnimGraphStateCondition*>(transitionCondition);

                    EMotionFX::AnimGraphNodeId newStateId;
                    if (transition->GetSourceNode() && azrtti_typeid(transition->GetSourceNode()) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                    {
                        newStateId = transition->GetSourceNode()->GetId();
                    }

                    stateCondition->SetStateId(newStateId);
                }

                if (azrtti_typeid(transitionCondition) == azrtti_typeid<EMotionFX::AnimGraphPlayTimeCondition>())
                {
                    EMotionFX::AnimGraphPlayTimeCondition* playTimeCondition = static_cast<EMotionFX::AnimGraphPlayTimeCondition*>(transitionCondition);

                    EMotionFX::AnimGraphNodeId newNodeId;
                    if (transition->GetSourceNode())
                    {
                        newNodeId = transition->GetSourceNode()->GetId();
                    }

                    playTimeCondition->SetNodeId(newNodeId);
                }
            }
        }

        mPlugin->OnUpdateUniqueData();
        Reinit();
    }


    // reinits the interface based on the current anim graph object
    void AttributesWindow::ReInitCurrentAnimGraphObject()
    {
        InitForAnimGraphObject(mObject);
    }

    // when we press the remove condition button
    void AttributesWindow::OnRemoveCondition()
    {
        MCORE_ASSERT(mObject && azrtti_istypeof<EMotionFX::AnimGraphStateTransition>(mObject));

        QAction* action = static_cast<QAction*>(sender());
        const int conditionIndex = action->property("conditionIndex").toInt();

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // convert the object into a state transition
        EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(mObject);

        // get the source node of the transition and the parent of that node, which is the state machine in which the transition is stored at
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        if (targetNode == nullptr)
        {
            MCore::LogError("Cannot remove condition from transition with id %i. Target node is nullptr.", transition->GetID());
            return;
        }

        // get the parent node
        EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();

        // check if the parent node really is a state machine
        if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            // convert the node to a state machine
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            // execute the command
            AZStd::string commandString, outResult;
            commandString = AZStd::string::format("AnimGraphRemoveCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionIndex %i", animGraph->GetID(), stateMachine->GetName(), transition->GetID(), conditionIndex);
            if (GetCommandManager()->ExecuteCommand(commandString.c_str(), outResult) == false)
            {
                if (outResult.empty() == false)
                {
                    MCore::LogError(outResult.c_str());
                }
            }

            // re-init the attributes window
            InitForAnimGraphObject(mObject);
        }
        else
        {
            MCore::LogError("AttributesWindow::OnRemoveCondition(): Parent node is no state machine.");
        }
    }


    void AttributesWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        if (mObject == nullptr || !azrtti_istypeof<EMotionFX::AnimGraphStateTransition>(mObject))
        {
            return;
        }

        EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(mObject);

        // create the context menu
        QMenu menu(this);

        // allow to put the conditions into the clipboard
        if (transition->GetNumConditions() > 0)
        {
            QAction* copyAction = menu.addAction("Copy Conditions");
            copyAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
            connect(copyAction, SIGNAL(triggered()), this, SLOT(OnCopyConditions()));
        }

        // if we already copied some conditions, allow pasting
        if (mCopyPasteClipboard.GetIsEmpty() == false)
        {
            QAction* pasteAction = menu.addAction("Paste Conditions");
            pasteAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
            connect(pasteAction, SIGNAL(triggered()), this, SLOT(OnPasteConditions()));

            QAction* pasteSelectiveAction = menu.addAction("Paste Conditions Selective");
            pasteSelectiveAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
            connect(pasteSelectiveAction, SIGNAL(triggered()), this, SLOT(OnPasteConditionsSelective()));
        }

        // show the menu at the given position
        if (menu.isEmpty() == false)
        {
            menu.exec(event->globalPos());
        }
    }


    void AttributesWindow::OnCopyConditions()
    {
        if (mObject == nullptr || !azrtti_istypeof<EMotionFX::AnimGraphStateTransition>(mObject))
        {
            return;
        }

        // get the transition and reset the clipboard data
        EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(mObject);
        mCopyPasteClipboard.Clear(false);

        // iterate through the conditions and put them into the clipboard
        const size_t numConditions = transition->GetNumConditions();
        for (size_t i = 0; i < numConditions; ++i)
        {
            EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);

            // construct the copy & paste object and put it into the clipboard
            AZ::Outcome<AZStd::string> contents = MCore::ReflectionSerializer::Serialize(condition);
            if (contents.IsSuccess())
            {
                CopyPasteConditionObject copyPasteObject;
                copyPasteObject.mContents = contents.GetValue();
                copyPasteObject.mConditionType = azrtti_typeid(condition);
                condition->GetSummary(&copyPasteObject.mSummary);
                mCopyPasteClipboard.Add(copyPasteObject);
            }
        }
    }


    void AttributesWindow::OnPasteConditions()
    {
        if (!mObject || !azrtti_istypeof<EMotionFX::AnimGraphStateTransition>(mObject))
        {
            return;
        }

        EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(mObject);

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            return;
        }

        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        if (!targetNode)
        {
            return;
        }

        // Only allow when the parent is a valid state machine.
        EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();
        if (!parentNode || azrtti_typeid(parentNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            return;
        }

        AZStd::string command;
        MCore::CommandGroup commandGroup;

        const uint32 numConditions = mCopyPasteClipboard.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            command = AZStd::string::format("AnimGraphAddCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionType \"%s\" -contents {%s}", 
                animGraph->GetID(), 
                parentNode->GetName(), 
                transition->GetID(), 
                mCopyPasteClipboard[i].mConditionType.ToString<AZStd::string>().c_str(), 
                mCopyPasteClipboard[i].mContents.c_str());
            commandGroup.AddCommandString(command);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Send LyMetrics event.
        MetricsEventSender::SendPasteConditionsEvent(numConditions);
    }


    void AttributesWindow::OnPasteConditionsSelective()
    {
        if (!mObject || !azrtti_istypeof<EMotionFX::AnimGraphStateTransition>(mObject))
        {
            return;
        }

        delete mPasteConditionsWindow;
        mPasteConditionsWindow = nullptr;

        EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(mObject);

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            return;
        }

        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        if (!targetNode)
        {
            return;
        }

        EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();
        if (!parentNode || azrtti_typeid(parentNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            return;
        }

        // Open the select conditions window and return if the user canceled it.
        mPasteConditionsWindow = new PasteConditionsWindow(this);
        if (mPasteConditionsWindow->exec() == QDialog::Rejected)
        {
            return;
        }

        AZStd::string command;
        MCore::CommandGroup commandGroup;

        AZ::u32 numPastedConditions = 0;
        const AZ::u32 numConditions = mCopyPasteClipboard.GetLength();
        for (AZ::u32 i = 0; i < numConditions; ++i)
        {
            // check if the condition was selected in the window, if not skip it
            if (mPasteConditionsWindow->GetIsConditionSelected(i) == false)
            {
                continue;
            }

            command = AZStd::string::format("AnimGraphAddCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionType \"%s\" -contents {%s}",
                animGraph->GetID(),
                parentNode->GetName(),
                transition->GetID(), \
                mCopyPasteClipboard[i].mConditionType.ToString<AZStd::string>().c_str(), 
                mCopyPasteClipboard[i].mContents.c_str());
            commandGroup.AddCommandString(command);
            numPastedConditions++;
        }

        mCopyPasteClipboard.Clear(false);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Send LyMetrics event.
        MetricsEventSender::SendPasteConditionsEvent(numPastedConditions);

    }


    // constructor
    PasteConditionsWindow::PasteConditionsWindow(AttributesWindow* attributeWindow)
        : QDialog(attributeWindow)
    {
        // update title of the about dialog
        setWindowTitle("Paste Transition Conditions");

        // create the about dialog's layout
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetFixedSize);

        layout->addWidget(new QLabel("Please select the conditions you want to paste:"));

        mCheckboxes.Clear();
        const MCore::Array<AttributesWindow::CopyPasteConditionObject>& copyPasteClipboard = attributeWindow->GetCopyPasteConditionClipboard();
        const uint32 numConditions = copyPasteClipboard.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            QCheckBox* checkbox = new QCheckBox(copyPasteClipboard[i].mSummary.c_str());
            mCheckboxes.Add(checkbox);
            checkbox->setCheckState(Qt::Checked);
            layout->addWidget(checkbox);
        }

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mCancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(mOKButton, SIGNAL(clicked()), this, SLOT(accept()));
        connect(mCancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    }


    // destructor
    PasteConditionsWindow::~PasteConditionsWindow()
    {
    }


    // check if the condition is selected
    bool PasteConditionsWindow::GetIsConditionSelected(uint32 index) const
    {
        return mCheckboxes[index]->checkState() == Qt::Checked;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.moc>
