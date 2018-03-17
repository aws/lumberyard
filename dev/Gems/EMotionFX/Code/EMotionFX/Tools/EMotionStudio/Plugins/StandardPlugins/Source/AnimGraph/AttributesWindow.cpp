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
#include "AttributesWindow.h"
#include "AnimGraphPlugin.h"
#include "GraphNodeFactory.h"
#include "AttributeWidget.h"
#include "BlendGraphWidget.h"
#include "ConditionSelectDialog.h"

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

#include <QVBoxLayout>
#include <QApplication>
#include <QGridLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QContextMenuEvent>
#include <QTreeWidgetItem>


namespace EMStudio
{
    // constructor
    AttributesWindow::AttributesWindow(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        mPlugin                 = plugin;
        mMainWidget             = nullptr;
        //  mGridLayout             = nullptr;
        mObject                 = nullptr;
        mPasteConditionsWindow  = nullptr;
        mScrollArea             = new QScrollArea();

        mRemoveButtonTable.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        mMainLayout = new QVBoxLayout();
        mMainLayout->setMargin(0);
        //mainLayout->setContentsMargins(0, 0, 0, 10);
        mMainLayout->setSpacing(1);
        setLayout(mMainLayout);
        //mainLayout->setSizeConstraint( QLayout::SetNoConstraint );
        mMainLayout->addWidget(mScrollArea);
        mScrollArea->setWidgetResizable(true);

        // init
        InitForAnimGraphObject(nullptr);
    }


    // destructor
    AttributesWindow::~AttributesWindow()
    {
    }


    // re-init the window
    void AttributesWindow::Reinit()
    {
        InitForAnimGraphObject(mObject);
    }


    // init for a given node
    void AttributesWindow::InitForAnimGraphObject(EMotionFX::AnimGraphObject* object)
    {
        //mPlugin->SetDisableRendering( true );

        // process all events to make sure all deleteLater()s in the message queue are called before reiniting the window
        //      GetApp()->processEvents();

        //mPlugin->SetDisableRendering( false );

        // delete the existing property browser
        mMainWidget->deleteLater();
        mMainWidget = nullptr;
        mScrollArea->setVisible(false);
        mScrollArea->setWidget(nullptr);
        mObject = object;

        // if there is no node, leave
        if (object == nullptr)
        {
            mAttributes = new MysticQt::PropertyWidget();

            mMainWidget = new QWidget();
            mMainWidget->setVisible(false);
            QVBoxLayout* verticalLayout = new QVBoxLayout();
            mMainWidget->setLayout(verticalLayout);
            verticalLayout->addWidget(mAttributes);
            verticalLayout->setAlignment(Qt::AlignTop);
            verticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
            verticalLayout->setSpacing(2);
            verticalLayout->setMargin(0);

            layout()->addWidget(mMainWidget);
            mScrollArea->setWidget(mMainWidget);

            mMainWidget->setVisible(true);
            mScrollArea->show();
            /*
                    // set the initial text that appears inside the widget
                    QFont font;
                    font.setPointSize( 8 );
                    QLabel* initialText = new QLabel("<c>Select a node or transition inside the <b>Anim Graph</b> window first.<br>This window will show the <b>attributes</b> of<br>the selected node or transition.</c>");
                    initialText->setAlignment( Qt::AlignCenter );
                    initialText->setTextFormat( Qt::RichText );
                    initialText->setFont( font );
                    initialText->setMaximumSize( 100000, 100000 );
                    initialText->setMargin( 0 );
                    initialText->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
                    mScrollArea->setWidget( initialText );
            */
            return;
        }

        // update the object's attributes
        //object->OnUpdateAttributes();

        // create the new property parent window
        mMainWidget = mPlugin->GetGraphNodeFactory()->CreateAttributeWidget(object->GetType());
        if (mMainWidget == nullptr) // if there is no custom attribute widget, generate one
        {
            mAttributes = new MysticQt::PropertyWidget();

            mMainWidget = new QWidget();
            mMainWidget->setVisible(false);
            //mGridLayout       = new QGridLayout();
            QVBoxLayout* verticalLayout = new QVBoxLayout();
            /*
                    // insert the add transition condition button
                    if (object->GetBaseType() == EMotionFX::AnimGraphStateTransition::BASETYPE_ID)
                    {
                        QPushButton* addConditionButton = new QPushButton("Add Condition");
                        addConditionButton->setIcon( MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Plus.png") );
                        connect(addConditionButton, SIGNAL(clicked()), this, SLOT(OnAddCondition()));
                        verticalLayout->addWidget( addConditionButton );
                    }*/

            mMainWidget->setLayout(verticalLayout);
            verticalLayout->addWidget(mAttributes);
            verticalLayout->setAlignment(Qt::AlignTop);
            verticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
            verticalLayout->setSpacing(2);
            verticalLayout->setMargin(0);

            bool readOnly = false; // TODO: can it be true too? :)

            // add the name as property first in case we are dealing with a node
            if (object->GetBaseType() == EMotionFX::AnimGraphNode::BASETYPE_ID)
            {
                // detect attribute changes
                using AZStd::placeholders::_1;
                using AZStd::placeholders::_2;
                MysticQt::AttributeChangedFunction func = AZStd::bind(&EMStudio::AttributesWindow::OnAttributeChanged, this, _1, _2);
                mAttributes->AddProperty("Attributes", "Node Name", new NodeNameAttributeWidget((EMotionFX::AnimGraphNode*)object, nullptr, readOnly, false, func), nullptr, nullptr);
            }
            mAttributes->SetIsExpanded("Attributes", true);

            // add all attributes
            mAttributeLinks.Clear(false);
            MCore::String labelString;
            MCore::Array<MCore::Attribute*> attributes;
            const uint32 attribInfoSetIndex = EMotionFX::GetAnimGraphManager().FindAttributeInfoSetIndex(object);
            const uint32 numAttributes = object->GetNumAttributes();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                // get the attribute and the corresponding attribute info
                MCore::Attribute*                   attribute           = object->GetAttribute(i);
                MCore::AttributeSettings*           attributeSettings   = EMotionFX::GetAnimGraphManager().GetAttributeInfo(attribInfoSetIndex, i);

                attributes.Clear(false); // keep memory
                attributes.Add(attribute);

                // create the attribute and add it to the layout
                using AZStd::placeholders::_1;
                using AZStd::placeholders::_2;
                MysticQt::AttributeChangedFunction func = AZStd::bind(&EMStudio::AttributesWindow::OnAttributeChanged, this, _1, _2);
                MysticQt::AttributeWidget* attributeWidget = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->CreateAttributeWidget(attributes, attributeSettings, object, readOnly, false, true, MysticQt::AttributeWidgetFactory::ATTRIBUTE_NORMAL, false, func);
                connect(attributeWidget, SIGNAL(RequestParentReInit()), this, SLOT(ReInitCurrentAnimGraphObject()), Qt::QueuedConnection);
                mAttributes->AddProperty("Attributes", attributeSettings->GetName(), attributeWidget, attribute, attributeSettings, false);
                mAttributeLinks.AddEmpty();
                mAttributeLinks.GetLast().mAttributeIndex = i;
                mAttributeLinks.GetLast().mObject = object;
                mAttributeLinks.GetLast().mWidget = attributeWidget;
            }

            // check if we are dealing with a state transition
            if (object->GetBaseType() == EMotionFX::AnimGraphStateTransition::BASETYPE_ID)
            {
                AddConditions(object, verticalLayout, readOnly);
            }
        }

        object->OnUpdateAttributes();
        UpdateAttributeWidgetStates();

        layout()->addWidget(mMainWidget);
        mScrollArea->setWidget(mMainWidget);

        mMainWidget->setVisible(true);
        mScrollArea->show();

        // resize column to contents
        const uint32 numAttributesWidgetColumns = mAttributes->columnCount();
        for (uint32 i = 0; i < numAttributesWidgetColumns; ++i)
        {
            mAttributes->resizeColumnToContents(i);
        }
    }


    // add the conditions management for to a given layout
    void AttributesWindow::AddConditions(EMotionFX::AnimGraphObject* object, QVBoxLayout* mainLayout, bool readOnly)
    {
        MCORE_ASSERT(object->GetBaseType() == EMotionFX::AnimGraphStateTransition::BASETYPE_ID);

        mRemoveButtonTable.Clear();

        const char* groupName = "Conditions";

        // insert the add transition condition button
        QPushButton* addConditionButton = new QPushButton("Add Condition");
        addConditionButton->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Plus.png"));
        connect(addConditionButton, SIGNAL(clicked()), this, SLOT(OnAddCondition()));
        mainLayout->addWidget(addConditionButton);

        // convert the object into a state transition
        EMotionFX::AnimGraphStateTransition* stateTransition = static_cast<EMotionFX::AnimGraphStateTransition*>(object);

        // for all transition conditions
        MCore::String conditionName;
        MCore::String conditionGroupName;
        const uint32 numConditions = stateTransition->GetNumConditions();
        for (uint32 c = 0; c < numConditions; ++c)
        {
            EMotionFX::AnimGraphTransitionCondition* condition = stateTransition->GetCondition(c);

            // create the condition group name
            conditionName.Format("#%i: %s", c, condition->GetPaletteName());
            conditionGroupName.Format("%s.%s", groupName, conditionName.AsChar());

            // create the remove condition attribute widget and add it as property to the property widget
            using AZStd::placeholders::_1;
            using AZStd::placeholders::_2;
            MysticQt::AttributeChangedFunction func = AZStd::bind(&EMStudio::AttributesWindow::OnAttributeChanged, this, _1, _2);
            ButtonAttributeWidget* removeConditionAttributeWidget = new ButtonAttributeWidget(MCore::Array<MCore::Attribute*>(), nullptr, nullptr, readOnly, false, func);
            mAttributes->AddProperty(groupName, conditionName.AsChar(), removeConditionAttributeWidget, nullptr, nullptr);

            // get the button from the property widget and adjust it
            QPushButton* removeConditionButton = removeConditionAttributeWidget->GetButton();
            EMStudioManager::MakeTransparentButton(removeConditionButton, "/Images/Icons/Remove.png", "Remove condition from the transition.");
            connect(removeConditionButton, SIGNAL(clicked()), this, SLOT(OnRemoveCondition()));

            // add the remove button to the table, so we know this button would remove what condition
            mRemoveButtonTable.AddEmpty();
            mRemoveButtonTable.GetLast().mButton    = removeConditionButton;
            mRemoveButtonTable.GetLast().mIndex     = c;

            // add all transition condition
            const uint32 attribInfoSetIndex = EMotionFX::GetAnimGraphManager().FindAttributeInfoSetIndex(condition);
            const uint32 numAttributes = condition->GetNumAttributes();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                // get the attribute and the corresponding attribute info
                MCore::Attribute*                   attribute           = condition->GetAttribute(i);
                MCore::AttributeSettings*           attributeSettings   = EMotionFX::GetAnimGraphManager().GetAttributeInfo(attribInfoSetIndex, i);

                MCore::Array<MCore::Attribute*> attributes;
                attributes.Add(attribute);

                using AZStd::placeholders::_1;
                using AZStd::placeholders::_2;
                MysticQt::AttributeChangedFunction func2 = AZStd::bind(&EMStudio::AttributesWindow::OnAttributeChanged, this, _1, _2);

                // create the attribute and add it to the layout
                MysticQt::AttributeWidget* attributeWidget = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->CreateAttributeWidget(attributes, attributeSettings, object, readOnly, false, true, MysticQt::AttributeWidgetFactory::ATTRIBUTE_NORMAL, false, func2);

                mAttributes->AddProperty(conditionGroupName.AsChar(), attributeSettings->GetName(), attributeWidget, attribute, attributeSettings, false);

                mAttributeLinks.AddEmpty();
                mAttributeLinks.GetLast().mAttributeIndex = i;
                mAttributeLinks.GetLast().mObject = condition;
                mAttributeLinks.GetLast().mWidget = attributeWidget;
            }

            // expand the condition group
            mAttributes->SetIsExpanded(conditionGroupName.AsChar(), true);
        } // for all conditions

        mAttributes->SetIsExpanded(groupName, true);
    }


    // when we press the add conditions button
    void AttributesWindow::OnAddCondition()
    {
        // get the active anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        MCORE_ASSERT(animGraph);

        // create the dialog
        ConditionSelectDialog dialog(this);
        if (dialog.exec() == QDialog::Rejected)
        {
            return;
        }

        // get the selected transition condition type
        const uint32 selectedConditionType = dialog.GetSelectedConditionType();
        if (selectedConditionType == MCORE_INVALIDINDEX32)
        {
            return;
        }

        MCORE_ASSERT(mObject->GetBaseType() == EMotionFX::AnimGraphStateTransition::BASETYPE_ID);

        // add it to the transition
        EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(mObject);

        // get the source node of the transition and the parent of that node, which is the state machine in which the transition is stored at
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();

        // check if the parent node really is a state machine
        if (parentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            // convert the node to a state machine
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            // execute the command
            MCore::String commandString, outResult;
            commandString.Format("AnimGraphAddCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionType %i", animGraph->GetID(), stateMachine->GetName(), transition->GetID(), selectedConditionType);
            if (GetCommandManager()->ExecuteCommand(commandString.AsChar(), outResult) == false)
            {
                if (outResult.GetIsEmpty() == false)
                {
                    MCore::LogError(outResult.AsChar());
                }
            }

            // get the newly created condition and check if we're dealing with a motion condition
            if (transition->GetNumConditions() > 0)
            {
                // get the last condition and check if it is a motion condition
                EMotionFX::AnimGraphTransitionCondition* transitionCondition = transition->GetCondition(transition->GetNumConditions() - 1);
                if (transitionCondition->GetType() == EMotionFX::AnimGraphMotionCondition::TYPE_ID)
                {
                    // type cast it to a motion condition
                    EMotionFX::AnimGraphMotionCondition* motionCondition = static_cast<EMotionFX::AnimGraphMotionCondition*>(transitionCondition);

                    // get the transition source node name
                    MCore::String transitionSourceNodeName;
                    if (transition->GetSourceNode() && transition->GetSourceNode()->GetType() == EMotionFX::AnimGraphMotionNode::TYPE_ID)
                    {
                        transitionSourceNodeName = transition->GetSourceNode()->GetName();
                    }

                    // set the new source node to the condition and update its data
                    motionCondition->GetAttributeString(EMotionFX::AnimGraphMotionCondition::ATTRIB_MOTIONNODE)->SetValue(transitionSourceNodeName.AsChar());
                    motionCondition->OnUpdateAttributes();
                }

                // do the same game for the state condition
                if (transitionCondition->GetType() == EMotionFX::AnimGraphStateCondition::TYPE_ID)
                {
                    // type cast it to a motion condition
                    EMotionFX::AnimGraphStateCondition* stateCondition = static_cast<EMotionFX::AnimGraphStateCondition*>(transitionCondition);

                    // get the transition source node name
                    MCore::String transitionSourceNodeName;
                    if (transition->GetSourceNode() && transition->GetSourceNode()->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
                    {
                        transitionSourceNodeName = transition->GetSourceNode()->GetName();
                    }

                    // set the new source node to the condition and update its data
                    stateCondition->GetAttributeString(EMotionFX::AnimGraphStateCondition::ATTRIB_STATE)->SetValue(transitionSourceNodeName.AsChar());
                    stateCondition->OnUpdateAttributes();
                }

                // do the same game for the playtime condition
                if (transitionCondition->GetType() == EMotionFX::AnimGraphPlayTimeCondition::TYPE_ID)
                {
                    // type cast it to a playtime condition
                    EMotionFX::AnimGraphPlayTimeCondition* playtimeCondition = static_cast<EMotionFX::AnimGraphPlayTimeCondition*>(transitionCondition);

                    // get the transition source node name
                    MCore::String transitionSourceNodeName;
                    if (transition->GetSourceNode())
                    {
                        transitionSourceNodeName = transition->GetSourceNode()->GetName();
                    }

                    // set the new source node to the condition and update its data
                    playtimeCondition->GetAttributeString(EMotionFX::AnimGraphPlayTimeCondition::ATTRIB_NODE)->SetValue(transitionSourceNodeName.AsChar());
                    playtimeCondition->OnUpdateAttributes();
                }
            }
        }

        mPlugin->OnUpdateUniqueData();
        Reinit();
    }


    // find the index for the given button
    uint32 AttributesWindow::FindRemoveButtonIndex(QObject* button) const
    {
        // for all table entries
        const uint32 numButtons = mRemoveButtonTable.GetLength();
        for (uint32 i = 0; i < numButtons; ++i)
        {
            if (mRemoveButtonTable[i].mButton == button) // this is button we search for
            {
                return mRemoveButtonTable[i].mIndex;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // reinits the interface based on the current anim graph object
    void AttributesWindow::ReInitCurrentAnimGraphObject()
    {
        InitForAnimGraphObject(mObject);
    }


    // update the states
    void AttributesWindow::UpdateAttributeWidgetStates()
    {
        if (mObject == nullptr)
        {
            return;
        }

        const uint32 numLinks = mAttributeLinks.GetLength();
        for (uint32 i = 0; i < numLinks; ++i)
        {
            const AttributeLink& curLink = mAttributeLinks[i];
            const bool enabled = curLink.mObject->GetIsAttributeEnabled(curLink.mAttributeIndex);
            curLink.mWidget->EnableWidgets(enabled);
        }
    }



    // when we press the remove condition button
    void AttributesWindow::OnRemoveCondition()
    {
        MCORE_ASSERT(mObject && mObject->GetBaseType() == EMotionFX::AnimGraphStateTransition::BASETYPE_ID);

        // find the condition to remove
        const uint32 index = FindRemoveButtonIndex(sender());
        assert(index != MCORE_INVALIDINDEX32);

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
        if (parentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            // convert the node to a state machine
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            // execute the command
            MCore::String commandString, outResult;
            commandString.Format("AnimGraphRemoveCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionIndex %i", animGraph->GetID(), stateMachine->GetName(), transition->GetID(), index);
            if (GetCommandManager()->ExecuteCommand(commandString.AsChar(), outResult) == false)
            {
                if (outResult.GetIsEmpty() == false)
                {
                    MCore::LogError(outResult.AsChar());
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
        if (mObject == nullptr || mObject->GetBaseType() != EMotionFX::AnimGraphStateTransition::BASETYPE_ID)
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
        if (mObject == nullptr || mObject->GetBaseType() != EMotionFX::AnimGraphStateTransition::BASETYPE_ID)
        {
            return;
        }

        // get the transition and reset the clipboard data
        EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(mObject);
        mCopyPasteClipboard.Clear(false);

        // iterate through the conditions and put them into the clipboard
        MCore::String commandString;
        MCore::String attributesString;
        const uint32 numConditions = transition->GetNumConditions();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            // get the condition
            EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);

            // get the index of the condition type in the object factory
            const uint32 conditionTypeIndex = EMotionFX::GetAnimGraphManager().GetObjectFactory()->FindRegisteredObjectByTypeID(condition->GetType());

            // construct the copy & paste object and put it into the clipboard
            CopyPasteConditionObject copyPasteObject;
            copyPasteObject.mAttributes     = condition->CreateAttributesString();
            copyPasteObject.mConditionType  = conditionTypeIndex;
            condition->GetSummary(&copyPasteObject.mSummary);
            mCopyPasteClipboard.Add(copyPasteObject);
        }
    }


    void AttributesWindow::OnPasteConditions()
    {
        if (!mObject || mObject->GetBaseType() != EMotionFX::AnimGraphStateTransition::BASETYPE_ID)
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
        if (!parentNode || parentNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            return;
        }

        AZStd::string command;
        MCore::CommandGroup commandGroup;

        const uint32 numConditions = mCopyPasteClipboard.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            command = AZStd::string::format("AnimGraphAddCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionType %i -attributesString \"%s\"", animGraph->GetID(), parentNode->GetName(), transition->GetID(), mCopyPasteClipboard[i].mConditionType, mCopyPasteClipboard[i].mAttributes.AsChar());
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
        if (!mObject || mObject->GetBaseType() != EMotionFX::AnimGraphStateTransition::BASETYPE_ID)
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
        if (!parentNode || parentNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
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

            command = AZStd::string::format("AnimGraphAddCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionType %i -attributesString \"%s\"", animGraph->GetID(), parentNode->GetName(), transition->GetID(), mCopyPasteClipboard[i].mConditionType, mCopyPasteClipboard[i].mAttributes.AsChar());
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


    // some attribute has changed
    void AttributesWindow::OnAttributeChanged(MCore::Attribute* attribute, MCore::AttributeSettings* settings)
    {
        if (mObject == nullptr)
        {
            return;
        }

        // inform the object about the change
        mObject->OnUpdateAttribute(attribute, settings);
        mObject->OnUpdateAttributes();

        // update the widget states (disable widgets when needed)
        UpdateAttributeWidgetStates();

        // reinit the visual graph part
        if (settings->GetReinitGuiOnValueChange())
        {
            if (mObject->GetBaseType() == EMotionFX::AnimGraphNode::BASETYPE_ID)
            {
                mPlugin->SyncVisualNode(static_cast<EMotionFX::AnimGraphNode*>(mObject));
            }
            else
            if (mObject->GetBaseType() == EMotionFX::AnimGraphStateTransition::BASETYPE_ID)
            {
                mPlugin->SyncTransition(static_cast<EMotionFX::AnimGraphStateTransition*>(mObject));
            }
        }

        // trigger a unique data update
        if (settings->GetReinitObjectOnValueChange())
        {
            EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
            if (animGraph)
            {
                for (uint32 i = 0; i < animGraph->GetNumAnimGraphInstances(); ++i)
                {
                    mObject->OnUpdateUniqueData(animGraph->GetAnimGraphInstance(i));
                }
            }
        }
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
            QCheckBox* checkbox = new QCheckBox(copyPasteClipboard[i].mSummary.AsChar());
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
