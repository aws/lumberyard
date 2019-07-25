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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphPlayTimeCondition.h>
#include <EMotionFX/Source/AnimGraphStateCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphTriggerAction.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MetricsEventSender.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphEditor.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNodeFactory.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <Source/Editor/ObjectEditor.h>


namespace EMStudio
{
    AttributesWindow::AttributesWindow(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        mPlugin                 = plugin;
        mPasteConditionsWindow  = nullptr;
        mScrollArea             = new QScrollArea();

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setSpacing(1);
        setLayout(mainLayout);

        mainLayout->addWidget(mScrollArea);
        mScrollArea->setWidgetResizable(true);

        // The main reflected widget will contain the non-custom attribute version of the
        // attribute widget. The intention is to reuse the Reflected Property Editor and
        // Cards
        {
            m_mainReflectedWidget = new QWidget();
            m_mainReflectedWidget->setVisible(false);

            QVBoxLayout* verticalLayout = new QVBoxLayout();
            m_mainReflectedWidget->setLayout(verticalLayout);
            verticalLayout->setAlignment(Qt::AlignTop);
            verticalLayout->setMargin(0);
            verticalLayout->setSpacing(0);
            verticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            }
            else
            {
                // 1. Create anim graph card.
                m_animGraphEditor = new EMotionFX::AnimGraphEditor(nullptr, serializeContext, m_mainReflectedWidget);
                verticalLayout->addWidget(m_animGraphEditor);
                m_animGraphEditor->setVisible(false);

                // Align the layout spacing with the entity inspector.
                verticalLayout->addSpacerItem(new QSpacerItem(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));

                // 2. Create object card
                m_objectEditor = new EMotionFX::ObjectEditor(serializeContext, m_mainReflectedWidget);

                m_objectCard = new AzQtComponents::Card(m_mainReflectedWidget);
                m_objectCard->setTitle("");
                m_objectCard->setContentWidget(m_objectEditor);
                m_objectCard->setExpanded(true);

                AzQtComponents::CardHeader* cardHeader = m_objectCard->header();
                cardHeader->setHasContextMenu(false);
                cardHeader->setHelpURL("");

                verticalLayout->addWidget(m_objectCard);
                m_objectCard->setVisible(false);
            }
            {
                m_conditionsWidget = new QWidget();
                QVBoxLayout* conditionsVerticalLayout = new QVBoxLayout();
                m_conditionsWidget->setLayout(conditionsVerticalLayout);
                conditionsVerticalLayout->setAlignment(Qt::AlignTop);
                conditionsVerticalLayout->setMargin(0);
                conditionsVerticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
                conditionsVerticalLayout->addSpacerItem(new QSpacerItem(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));

                m_conditionsLayout = new QVBoxLayout();
                m_conditionsLayout->setAlignment(Qt::AlignTop);
                m_conditionsLayout->setMargin(0);
                m_conditionsLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
                conditionsVerticalLayout->addLayout(m_conditionsLayout);

                AddConditionButton* addConditionButton = new AddConditionButton(mPlugin, m_conditionsWidget);
                connect(addConditionButton, &AddConditionButton::ObjectTypeChosen, this, [=](AZ::TypeId conditionType)
                    {
                        AddCondition(conditionType);
                    });

                conditionsVerticalLayout->addWidget(addConditionButton);

                verticalLayout->addWidget(m_conditionsWidget);
                m_conditionsWidget->setVisible(false);
            }
            {
                m_actionsWidget = new QWidget();
                QVBoxLayout* actionVerticalLayout = new QVBoxLayout();
                m_actionsWidget->setLayout(actionVerticalLayout);
                actionVerticalLayout->setAlignment(Qt::AlignTop);
                actionVerticalLayout->setMargin(0);
                actionVerticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
                actionVerticalLayout->addSpacerItem(new QSpacerItem(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));

                m_actionsLayout = new QVBoxLayout();
                m_actionsLayout->setAlignment(Qt::AlignTop);
                m_actionsLayout->setMargin(0);
                m_actionsLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
                actionVerticalLayout->addLayout(m_actionsLayout);

                AddActionButton* addActionButton = new AddActionButton(mPlugin, m_actionsWidget);
                connect(addActionButton, &AddActionButton::ObjectTypeChosen, this, [=](AZ::TypeId actionType)
                    {
                        const AnimGraphModel::ModelItemType itemType = m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
                        if (itemType == AnimGraphModel::ModelItemType::TRANSITION)
                        {
                            AddTransitionAction(actionType);
                        }
                        else
                        {
                            AddStateAction(actionType);
                        }
                    });
                actionVerticalLayout->addWidget(addActionButton);

                verticalLayout->addWidget(m_actionsWidget);
                m_actionsWidget->setVisible(false);
            }
        }

        connect(&plugin->GetAnimGraphModel().GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &AttributesWindow::OnSelectionChanged);
        connect(&plugin->GetAnimGraphModel(), &AnimGraphModel::dataChanged, this, &AttributesWindow::OnDataChanged);

        Init(QModelIndex(), true);

        AttributesWindowRequestBus::Handler::BusConnect();
    }

    AttributesWindow::~AttributesWindow()
    {
        AttributesWindowRequestBus::Handler::BusDisconnect();

        if (m_mainReflectedWidget)
        {
            if (mScrollArea->widget() == m_mainReflectedWidget)
            {
                mScrollArea->takeWidget();
            }
            delete m_mainReflectedWidget;
        }
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


    void AttributesWindow::Init(const QModelIndex& modelIndex, bool forceUpdate)
    {
        if (m_isLocked)
        {
            return;
        }

        if (!modelIndex.isValid())
        {
            m_objectEditor->ClearInstances(false);
        }

        // This only works on TRANSITIONS and NODES
        AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
        if (itemType != AnimGraphModel::ModelItemType::NODE && itemType != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return;
        }

        EMotionFX::AnimGraphObject* object = modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_OBJECT_PTR).value<EMotionFX::AnimGraphObject*>();

        QWidget* attributeWidget = mPlugin->GetGraphNodeFactory()->CreateAttributeWidget(azrtti_typeid(object));
        if (attributeWidget)
        {
            // In the case we have a custom attribute widget, we cannot reuse the widget, so we just replace it
            if (mScrollArea->widget() == m_mainReflectedWidget)
            {
                mScrollArea->takeWidget();
            }
            mScrollArea->setWidget(attributeWidget);
        }
        else
        {
            EMotionFX::AnimGraph* animGraph = nullptr;
            if (object)
            {
                animGraph = object->GetAnimGraph();
            }
            else
            {
                animGraph = mPlugin->GetActiveAnimGraph();
            }

            m_animGraphEditor->SetAnimGraph(animGraph);
            m_animGraphEditor->setVisible(animGraph);

            if (object)
            {
                m_objectCard->setTitle(object->GetPaletteName());

                AzQtComponents::CardHeader* cardHeader = m_objectCard->header();
                cardHeader->setHelpURL(object->GetHelpUrl());

                if (!forceUpdate && object == m_objectEditor->GetObject())
                {
                    m_objectEditor->InvalidateValues();
                }
                else
                {
                    m_objectEditor->ClearInstances(false);
                    m_objectEditor->AddInstance(object, azrtti_typeid(object));
                }

                UpdateConditions(object, serializeContext, forceUpdate);
                UpdateActions(object, serializeContext, forceUpdate);
            }
            else
            {
                // In case the previous selected object was showing any of these
                m_conditionsWidget->setVisible(false);
                m_actionsWidget->setVisible(false);
            }

            m_objectCard->setVisible(object);

            if (mScrollArea->widget() != m_mainReflectedWidget)
            {
                mScrollArea->setWidget(m_mainReflectedWidget);
            }
        }

        m_displayingModelIndex = modelIndex;
    }

    void AttributesWindow::UpdateConditions(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, bool forceUpdate)
    {
        if (azrtti_typeid(object) == azrtti_typeid<EMotionFX::AnimGraphStateTransition>())
        {
            const EMotionFX::AnimGraphStateTransition* stateTransition = static_cast<EMotionFX::AnimGraphStateTransition*>(object);

            const size_t numConditions = stateTransition->GetNumConditions();
            const size_t numConditionsWidgets = m_conditionsCachedWidgets.size();
            const size_t numConditionsAlreadyWithWidgets = AZStd::min(numConditions, numConditionsWidgets);
            for (size_t c = 0; c < numConditionsAlreadyWithWidgets; ++c)
            {
                EMotionFX::AnimGraphTransitionCondition* condition = stateTransition->GetCondition(c);
                CachedWidgets& conditionWidgets = m_conditionsCachedWidgets[c];

                conditionWidgets.m_card->setTitle(condition->GetPaletteName());
                AzQtComponents::CardHeader* cardHeader = conditionWidgets.m_card->header();
                cardHeader->setHelpURL(condition->GetHelpUrl());

                if (!forceUpdate && conditionWidgets.m_objectEditor->GetObject() == condition)
                {
                    conditionWidgets.m_objectEditor->InvalidateValues();
                }
                else
                {
                    conditionWidgets.m_objectEditor->ClearInstances(false);
                    conditionWidgets.m_objectEditor->AddInstance(condition, azrtti_typeid(condition));
                }
            }

            if (numConditions > numConditionsWidgets)
            {
                for (size_t c = numConditionsWidgets; c < numConditions; ++c)
                {
                    EMotionFX::AnimGraphTransitionCondition* condition = stateTransition->GetCondition(c);

                    EMotionFX::ObjectEditor* conditionEditor = new EMotionFX::ObjectEditor(serializeContext, this);
                    conditionEditor->AddInstance(condition, azrtti_typeid(condition));

                    // Create the card and put the editor widget in it.
                    AzQtComponents::Card* card = new AzQtComponents::Card(m_conditionsWidget);
                    connect(card, &AzQtComponents::Card::contextMenuRequested, this, &AttributesWindow::OnConditionContextMenu);

                    card->setTitle(condition->GetPaletteName());
                    card->setContentWidget(conditionEditor);
                    card->setProperty("conditionIndex", static_cast<unsigned int>(c));
                    card->setExpanded(true);

                    AzQtComponents::CardHeader* cardHeader = card->header();
                    cardHeader->setHelpURL(condition->GetHelpUrl());

                    m_conditionsLayout->addWidget(card);

                    m_conditionsCachedWidgets.emplace_back(card, conditionEditor);
                } // for all conditions
            }
            else if (numConditionsWidgets > numConditions)
            {
                // remove all the widgets that are no longer valid
                for (size_t w = numConditions; w < numConditionsWidgets; ++w)
                {
                    CachedWidgets& conditionWidgets = m_conditionsCachedWidgets[w];

                    // Just the card needs to be removed.
                    conditionWidgets.m_card->setVisible(false);
                    m_conditionsLayout->removeWidget(conditionWidgets.m_card);
                }
                m_conditionsCachedWidgets.erase(m_conditionsCachedWidgets.begin() + numConditions, m_conditionsCachedWidgets.end());
            }

            m_conditionsWidget->setVisible(true);
        }
        else
        {
            m_conditionsWidget->setVisible(false);
        }
    }

    void AttributesWindow::UpdateActions(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, bool forceUpdate)
    {
        const EMotionFX::TriggerActionSetup* actionSetup = nullptr;

        if (azrtti_istypeof<EMotionFX::AnimGraphNode>(object))
        {
            const EMotionFX::AnimGraphNode* node = static_cast<EMotionFX::AnimGraphNode*>(object);
            const EMotionFX::AnimGraphNode* parent = node->GetParentNode();
            if (node->GetCanActAsState() && parent && azrtti_istypeof<EMotionFX::AnimGraphStateMachine>(parent))
            {
                actionSetup = &node->GetTriggerActionSetup();
            }
        }
        else if (azrtti_typeid(object) == azrtti_typeid<EMotionFX::AnimGraphStateTransition>())
        {
            const EMotionFX::AnimGraphStateTransition* stateTransition = static_cast<EMotionFX::AnimGraphStateTransition*>(object);
            actionSetup = &stateTransition->GetTriggerActionSetup();
        }

        if (actionSetup)
        {
            const size_t numActions = actionSetup->GetNumActions();
            const size_t numActionWidgets = m_actionsCachedWidgets.size();
            const size_t numActionsAlreadyWithWidgets = AZStd::min(numActions, numActionWidgets);
            for (size_t a = 0; a < numActionsAlreadyWithWidgets; ++a)
            {
                EMotionFX::AnimGraphTriggerAction* action = actionSetup->GetAction(a);
                CachedWidgets& actionWidgets = m_actionsCachedWidgets[a];

                actionWidgets.m_card->setTitle(action->GetPaletteName());
                AzQtComponents::CardHeader* cardHeader = actionWidgets.m_card->header();
                cardHeader->setHelpURL(action->GetHelpUrl());

                if (!forceUpdate && actionWidgets.m_objectEditor->GetObject() == action)
                {
                    actionWidgets.m_objectEditor->InvalidateValues();
                }
                else
                {
                    actionWidgets.m_objectEditor->ClearInstances(false);
                    actionWidgets.m_objectEditor->AddInstance(action, azrtti_typeid(action));
                }
            }

            if (numActions > numActionWidgets)
            {
                for (size_t a = numActionWidgets; a < numActions; ++a)
                {
                    EMotionFX::AnimGraphTriggerAction* action = actionSetup->GetAction(a);

                    EMotionFX::ObjectEditor* actionEditor = new EMotionFX::ObjectEditor(serializeContext, this);
                    actionEditor->AddInstance(action, azrtti_typeid(action));

                    // Create the card and put the editor widget in it.
                    AzQtComponents::Card* card = new AzQtComponents::Card(m_actionsWidget);
                    connect(card, &AzQtComponents::Card::contextMenuRequested, this, &AttributesWindow::OnActionContextMenu);

                    card->setTitle(action->GetPaletteName());
                    card->setContentWidget(actionEditor);
                    card->setProperty("actionIndex", static_cast<unsigned int>(a));
                    card->setExpanded(true);

                    AzQtComponents::CardHeader* cardHeader = card->header();
                    cardHeader->setHelpURL(action->GetHelpUrl());

                    m_actionsLayout->addWidget(card);

                    m_actionsCachedWidgets.emplace_back(card, actionEditor);
                } // for all actions
            }
            else if (numActionWidgets > numActions)
            {
                // remove all the widgets that are no longer valid
                for (size_t w = numActions; w < numActionWidgets; ++w)
                {
                    CachedWidgets& actionWidgets = m_actionsCachedWidgets[w];

                    // Just the card needs to be removed
                    actionWidgets.m_card->setVisible(false);
                    m_actionsLayout->removeWidget(actionWidgets.m_card);
                }
                m_actionsCachedWidgets.erase(m_actionsCachedWidgets.begin() + numActions, m_actionsCachedWidgets.end());
            }

            m_actionsWidget->setVisible(true);
        }
        else
        {
            m_actionsWidget->setVisible(false);
        }
    }


    void AttributesWindow::OnConditionContextMenu(const QPoint& position)
    {
        const AzQtComponents::Card* card = static_cast<AzQtComponents::Card*>(sender());
        const int conditionIndex = card->property("conditionIndex").toInt();

        QMenu contextMenu(this);

        QAction* deleteAction = contextMenu.addAction("Delete condition");
        deleteAction->setProperty("conditionIndex", conditionIndex);
        connect(deleteAction, &QAction::triggered, this, &AttributesWindow::OnRemoveCondition);

        QAction* copyAction = contextMenu.addAction("Copy conditions");
        copyAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
        connect(copyAction, &QAction::triggered, this, &AttributesWindow::OnCopyConditions);

        if (!m_copyPasteClipboard.empty())
        {
            QAction* pasteAction = contextMenu.addAction("Paste conditions");
            pasteAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
            connect(pasteAction, &QAction::triggered, this, &AttributesWindow::OnPasteConditions);

            QAction* pasteSelectiveAction = contextMenu.addAction("Paste conditions selective");
            pasteSelectiveAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
            connect(pasteSelectiveAction, &QAction::triggered, this, &AttributesWindow::OnPasteConditionsSelective);
        }

        if (!contextMenu.isEmpty())
        {
            contextMenu.exec(position);
        }
    }

    void AttributesWindow::OnActionContextMenu(const QPoint& position)
    {
        const AnimGraphModel::ModelItemType itemType = m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();

        const AzQtComponents::Card* card = static_cast<AzQtComponents::Card*>(sender());
        const int actionIndex = card->property("actionIndex").toInt();

        QMenu contextMenu(this);


        QAction* deleteAction = contextMenu.addAction("Delete action");
        deleteAction->setProperty("actionIndex", actionIndex);
        if (itemType == AnimGraphModel::ModelItemType::TRANSITION)
        {
            connect(deleteAction, &QAction::triggered, this, &AttributesWindow::OnRemoveTransitionAction);
        }
        else
        {
            connect(deleteAction, &QAction::triggered, this, &AttributesWindow::OnRemoveStateAction);
        }


        if (!contextMenu.isEmpty())
        {
            contextMenu.exec(position);
        }
    }

    void AttributesWindow::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        AZ_UNUSED(selected);
        AZ_UNUSED(deselected);

        const QModelIndexList modelIndexes = mPlugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (!modelIndexes.empty())
        {
            Init(modelIndexes.front());
        }
        else
        {
            Init();
        }
    }

    void AttributesWindow::OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
    {
        QItemSelection changes(topLeft, bottomRight);
        if (changes.contains(m_displayingModelIndex))
        {
            if (roles.empty())
            {
                Init(m_displayingModelIndex);
            }
            else
            {
                if (roles.contains(AnimGraphModel::ROLE_TRANSITION_CONDITIONS))
                {
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    if (!serializeContext)
                    {
                        AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                        return;
                    }

                    EMotionFX::AnimGraphObject* object = m_displayingModelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_OBJECT_PTR).value<EMotionFX::AnimGraphObject*>();
                    UpdateConditions(object, serializeContext);
                }
                else if (roles.contains(AnimGraphModel::ROLE_TRIGGER_ACTIONS))
                {
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    if (!serializeContext)
                    {
                        AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                        return;
                    }

                    EMotionFX::AnimGraphObject* object = m_displayingModelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_OBJECT_PTR).value<EMotionFX::AnimGraphObject*>();
                    UpdateActions(object, serializeContext);
                }
            }
        }
    }


    void AttributesWindow::AddCondition(const AZ::TypeId& conditionType)
    {
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION,
            "Expected a transition");

        const EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        const EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        const EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        const EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();

        if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            const EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<const EMotionFX::AnimGraphStateMachine*>(parentNode);

            AZStd::optional<AZStd::string> contents;
            if (conditionType == azrtti_typeid<EMotionFX::AnimGraphMotionCondition>() &&
                sourceNode && azrtti_typeid(sourceNode) == azrtti_typeid<EMotionFX::AnimGraphMotionNode>())
            {
                EMotionFX::AnimGraphMotionCondition motionCondition;
                motionCondition.SetMotionNodeId(sourceNode->GetId());

                AZ::Outcome<AZStd::string> serializeOutcome = MCore::ReflectionSerializer::Serialize(&motionCondition);
                if (serializeOutcome.IsSuccess())
                {
                    contents = serializeOutcome.GetValue();
                }
            }
            else if (conditionType == azrtti_typeid<EMotionFX::AnimGraphStateCondition>() &&
                     sourceNode && azrtti_typeid(sourceNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                EMotionFX::AnimGraphStateCondition stateCondition;
                stateCondition.SetStateId(sourceNode->GetId());

                AZ::Outcome<AZStd::string> serializeOutcome = MCore::ReflectionSerializer::Serialize(&stateCondition);
                if (serializeOutcome.IsSuccess())
                {
                    contents = serializeOutcome.GetValue();
                }
            }
            else if (conditionType == azrtti_typeid<EMotionFX::AnimGraphPlayTimeCondition>() &&
                     sourceNode)
            {
                EMotionFX::AnimGraphPlayTimeCondition playTimeCondition;
                playTimeCondition.SetNodeId(sourceNode->GetId());

                AZ::Outcome<AZStd::string> serializeOutcome = MCore::ReflectionSerializer::Serialize(&playTimeCondition);
                if (serializeOutcome.IsSuccess())
                {
                    contents = serializeOutcome.GetValue();
                }
            }

            CommandSystem::AddCondition(transition, conditionType, contents);
        }
    }


    // when we press the remove condition button
    void AttributesWindow::OnRemoveCondition()
    {
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION,
            "Expected a transition");

        QAction* action = static_cast<QAction*>(sender());
        const int conditionIndex = action->property("conditionIndex").toInt();

        // convert the object into a state transition
        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        CommandSystem::RemoveCondition(transition, conditionIndex);
    }


    void AttributesWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        if (!m_displayingModelIndex.isValid()
            || m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }

        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        QMenu menu(this);

        if (transition->GetNumConditions() > 0)
        {
            QAction* copyAction = menu.addAction("Copy conditions");
            copyAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
            connect(copyAction, &QAction::triggered, this, &AttributesWindow::OnCopyConditions);
        }

        if (!m_copyPasteClipboard.empty())
        {
            QAction* pasteAction = menu.addAction("Paste conditions");
            pasteAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
            connect(pasteAction, &QAction::triggered, this, &AttributesWindow::OnPasteConditions);

            QAction* pasteSelectiveAction = menu.addAction("Paste conditions selective");
            pasteSelectiveAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
            connect(pasteSelectiveAction, &QAction::triggered, this, &AttributesWindow::OnPasteConditionsSelective);
        }

        // show the menu at the given position
        if (menu.isEmpty() == false)
        {
            menu.exec(event->globalPos());
        }
    }


    void AttributesWindow::OnCopyConditions()
    {
        if (!m_displayingModelIndex.isValid()
            || m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }

        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        m_copyPasteClipboard.clear();

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
                m_copyPasteClipboard.push_back(copyPasteObject);
            }
        }
    }


    void AttributesWindow::OnPasteConditions()
    {
        if (!m_displayingModelIndex.isValid()
            || m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }
        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        MCore::CommandGroup commandGroup;
        for (const CopyPasteConditionObject& copyPasteObject : m_copyPasteClipboard)
        {
            CommandSystem::AddCondition(transition, copyPasteObject.mConditionType, copyPasteObject.mContents, /*insertAt*/ AZStd::nullopt, &commandGroup);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
        else
        {
            m_copyPasteClipboard.clear();
        }

        // Send LyMetrics event.
        MetricsEventSender::SendPasteConditionsEvent(static_cast<AZ::u32>(m_copyPasteClipboard.size()));
    }


    void AttributesWindow::OnPasteConditionsSelective()
    {
        if (!m_displayingModelIndex.isValid()
            || m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }

        delete mPasteConditionsWindow;
        mPasteConditionsWindow = nullptr;

        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        // Open the select conditions window and return if the user canceled it.
        mPasteConditionsWindow = new PasteConditionsWindow(this);
        if (mPasteConditionsWindow->exec() == QDialog::Rejected)
        {
            return;
        }

        AZStd::string commandString;
        MCore::CommandGroup commandGroup;

        AZ::u32 numPastedConditions = 0;
        const size_t numConditions = m_copyPasteClipboard.size();
        for (size_t i = 0; i < numConditions; ++i)
        {
            // check if the condition was selected in the window, if not skip it
            if (!mPasteConditionsWindow->GetIsConditionSelected(i))
            {
                continue;
            }

            CommandSystem::AddCondition(transition, m_copyPasteClipboard[i].mConditionType, m_copyPasteClipboard[i].mContents, /*insertAt*/ AZStd::nullopt, &commandGroup);

            numPastedConditions++;
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
        else
        {
            m_copyPasteClipboard.clear();
        }

        // Send LyMetrics event.
        MetricsEventSender::SendPasteConditionsEvent(numPastedConditions);
    }

    void AttributesWindow::AddTransitionAction(const AZ::TypeId& actionType)
    {
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION,
            "Expected a transition");

        const EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        CommandSystem::AddTransitionAction(transition, actionType);
    }

    void AttributesWindow::AddStateAction(const AZ::TypeId& actionType)
    {
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE,
            "StateAction must added on an anim graph node");

        const EMotionFX::AnimGraphNode* node = m_displayingModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        CommandSystem::AddStateAction(node, actionType);
    }

    // when we press the remove condition button
    void AttributesWindow::OnRemoveTransitionAction()
    {
        AZ_Assert(m_displayingModelIndex.isValid(), "Object shouldn't be null.");

        QAction* action = static_cast<QAction*>(sender());
        const int actionIndex = action->property("actionIndex").toInt();
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION,
            "Expected a transition");

        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        CommandSystem::RemoveTransitionAction(transition, actionIndex);
    }

    // when we press the remove condition button
    void AttributesWindow::OnRemoveStateAction()
    {
        AZ_Assert(m_displayingModelIndex.isValid(), "Object shouldn't be null.");

        QAction* action = static_cast<QAction*>(sender());
        const int actionIndex = action->property("actionIndex").toInt();

        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE,
            "StateAction must added on an anim graph node");

        EMotionFX::AnimGraphNode* node = m_displayingModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        CommandSystem::RemoveStateAction(node, actionIndex);
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

        mCheckboxes.clear();
        const AZStd::vector<AttributesWindow::CopyPasteConditionObject>& copyPasteClipboard = attributeWindow->GetCopyPasteConditionClipboard();
        for (const AttributesWindow::CopyPasteConditionObject& copyPasteObject : copyPasteClipboard)
        {
            QCheckBox* checkbox = new QCheckBox(copyPasteObject.mSummary.c_str());
            mCheckboxes.push_back(checkbox);
            checkbox->setCheckState(Qt::Checked);
            layout->addWidget(checkbox);
        }

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton = new QPushButton("OK");
        mCancelButton = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(mOKButton, &QPushButton::clicked, this, &PasteConditionsWindow::accept);
        connect(mCancelButton, &QPushButton::clicked, this, &PasteConditionsWindow::reject);
    }


    // destructor
    PasteConditionsWindow::~PasteConditionsWindow()
    {
    }


    // check if the condition is selected
    bool PasteConditionsWindow::GetIsConditionSelected(size_t index) const
    {
        return mCheckboxes[index]->checkState() == Qt::Checked;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AddConditionButton::AddConditionButton(AnimGraphPlugin* plugin, QWidget* parent)
        : EMotionFX::TypeChoiceButton("Add condition", "", parent)
    {
        const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = plugin->GetAnimGraphObjectFactory()->GetUiObjectPrototypes();
        AZStd::unordered_map<AZ::TypeId, AZStd::string> types;
        m_types.reserve(objectPrototypes.size());

        for (const EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
        {
            if (azrtti_istypeof<EMotionFX::AnimGraphTransitionCondition>(objectPrototype))
            {
                m_types.emplace(azrtti_typeid(objectPrototype), objectPrototype->GetPaletteName());
            }
        }
    }

    AddActionButton::AddActionButton(AnimGraphPlugin* plugin, QWidget* parent)
        : EMotionFX::TypeChoiceButton("Add action", "", parent)
    {
        const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = plugin->GetAnimGraphObjectFactory()->GetUiObjectPrototypes();
        AZStd::unordered_map<AZ::TypeId, AZStd::string> types;
        types.reserve(objectPrototypes.size());

        for (const EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
        {
            if (azrtti_istypeof<EMotionFX::AnimGraphTriggerAction>(objectPrototype))
            {
                types.emplace(azrtti_typeid(objectPrototype), objectPrototype->GetPaletteName());
            }
        }

        SetTypes(types);
    }
} // namespace EMStudio
