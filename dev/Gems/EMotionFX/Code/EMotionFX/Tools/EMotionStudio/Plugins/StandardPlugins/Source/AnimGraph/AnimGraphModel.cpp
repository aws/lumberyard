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

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <QtGui/QFont>
#include <QtGui/QColor>
#include <QtGui/QPixmap>


namespace EMStudio
{
    AnimGraphModel::AnimGraphModel()
        : m_selectionModel(this)
    {
        m_selectionModel.setModel(this);

        EMotionFX::AnimGraphNotificationBus::Handler::BusConnect();

        m_commandCallbacks.emplace_back(new CommandDidLoadAnimGraphCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("LoadAnimGraph", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidCreateAnimGraphCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("CreateAnimGraph", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandWillRemoveAnimGraphCallback(*this, true, true));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("RemoveAnimGraph", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidRemoveAnimGraphCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("RemoveAnimGraph", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidActivateAnimGraphCallback(*this, true));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("ActivateAnimGraph", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidActivateAnimGraphPostUndoCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("ActivateAnimGraph", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidCreateNodeCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphCreateNode", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandWillRemoveNodeCallback(*this, true, true));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNode", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidRemoveNodeCallback(*this, false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNode", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidAdjustNodeCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustNode", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidCreateConnectionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphCreateConnection", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandWillRemoveConnectionCallback(*this, true, true));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveConnection", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidRemoveConnectionCallback(*this, false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveConnection", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidAdjustConnectionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandSystem::CommandAnimGraphAdjustTransition::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidAddConditionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandSystem::CommandAddTransitionCondition::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidRemoveConditionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandSystem::CommandRemoveTransitionCondition::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidEditActionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandSystem::CommandAnimGraphAddTransitionAction::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidEditActionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandSystem::CommandAnimGraphRemoveTransitionAction::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidEditActionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandSystem::CommandAnimGraphAddStateAction::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidEditActionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandSystem::CommandAnimGraphRemoveStateAction::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidSetEntryStateCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphSetEntryState", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidCreateParameterCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphCreateParameter", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidAdjustParameterCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameter", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidRemoveParameterCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameter", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidMoveParameterCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphMoveParameter", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidAddGroupParameterCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphAddGroupParameter", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidRemoveGroupParameterCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveGroupParameter", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidAdjustGroupParameterCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustGroupParameter", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidCreateMotionSetCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("CreateMotionSet", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidRemoveMotionSetCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("RemoveMotionSet", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidAdjustMotionSetCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AdjustMotionSet", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidMotionSetAddMotionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("MotionSetAddMotion", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidMotionSetRemoveMotionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("MotionSetRemoveMotion", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidMotionSetAdjustMotionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("MotionSetAdjustMotion", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidLoadMotionSetCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("LoadMotionSet", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidSaveMotionSetCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("SaveMotionSet", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandDidPlayMotionCallback(*this));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("PlayMotion", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandRemoveActorInstanceCallback(*this, false, true));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", m_commandCallbacks.back());

        // Since the UI could be loaded after anim graphs are added to the manager, we need to pull all the current ones
        // and add them to the model
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
            if (!animGraph->GetIsOwnedByRuntime() && !animGraph->GetIsOwnedByAsset())
            {
                Add(animGraph);
            }
        }

        AZ::Data::AssetBus::Router::BusRouterConnect();
    }

    AnimGraphModel::~AnimGraphModel()
    {
        AZ::Data::AssetBus::Router::BusRouterDisconnect();

        for (MCore::Command::Callback* callback : m_commandCallbacks)
        {
            CommandSystem::GetCommandManager()->RemoveCommandCallback(callback, true);
        }
        m_commandCallbacks.clear();

        EMotionFX::AnimGraphNotificationBus::Handler::BusDisconnect();

        Reset();
    }

    QModelIndex AnimGraphModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (hasIndex(row, column, parent) && (!parent.isValid() || parent.column() == 0))
        {
            if (parent.isValid())
            {
                const ModelItemData* parentModelItemData = static_cast<const ModelItemData*>(parent.internalPointer());
                if (row < parentModelItemData->m_children.size())
                {
                    return createIndex(row, column, parentModelItemData->m_children[row]);
                }
            }
            else if (!m_rootModelItemData.empty())
            {
                return createIndex(row, column, m_rootModelItemData[row]);
            }
        }

        return QModelIndex();
    }

    QModelIndex AnimGraphModel::parent(const QModelIndex& child) const
    {
        AZ_Assert(child.isValid(), "Expected valid child model index");
        if (!child.isValid())
        {
            return QModelIndex();
        }
        ModelItemData* childModelItemData = static_cast<ModelItemData*>(child.internalPointer());
        ModelItemData* parentModelItemData = childModelItemData->m_parent;
        if (parentModelItemData)
        {
            return createIndex(parentModelItemData->m_row, 0, parentModelItemData);
        }
        else
        {
            return QModelIndex();
        }
    }

    int AnimGraphModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
        {
            if (parent.column() != 0)
            {
                return 0;
            }
            const ModelItemData* modelItemData = static_cast<const ModelItemData*>(parent.internalPointer());
            return static_cast<int>(modelItemData->m_children.size());
        }
        else
        {
            return static_cast<int>(m_rootModelItemData.size());
        }
    }

    int AnimGraphModel::columnCount(const QModelIndex& parent) const
    {
        return 2;
    }

    QVariant AnimGraphModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section)
            {
            case 0:
                return "Name";
            case 1:
                return "Type";
            default:
                break;
            }
        }

        return QVariant();
    }

    QVariant AnimGraphModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        const ModelItemData* modelItemData = static_cast<const ModelItemData*>(index.internalPointer());
        AZ_Assert(modelItemData, "Expected valid ModelItemData pointer");

        switch (role)
        {
        case Qt::DisplayRole:
        {
            AZ_Assert(modelItemData->m_type == ModelItemType::NODE, "Expected a node");
            EMotionFX::AnimGraphNode* node = modelItemData->m_object.m_node;
            switch (index.column())
            {
            case COLUMN_NAME:
                if (modelItemData->m_parent)
                {
                    return node->GetName();
                }
                else
                {
                    // For the case of root nodes, return the name of the anim graph
                    AZStd::string filename;
                    AzFramework::StringFunc::Path::GetFileName(node->GetAnimGraph()->GetFileNameString().c_str(), filename);
                    if (!filename.empty())
                    {
                        return QString(filename.c_str());
                    }
                    else
                    {
                        return "<Unsaved Animgraph>";
                    }
                }
            case COLUMN_PALETTE_NAME:
                return node->GetPaletteName();
            default:
                break;
            }
            break;
        }
        case Qt::FontRole:
            if (m_parentFocus.isValid() && index == m_parentFocus)
            {
                QFont font;
                font.setBold(true);
                return font;
            }
            break;
        case Qt::DecorationRole:
            if (index.column() == 0)
            {
                if (modelItemData->m_type == ModelItemType::NODE)
                {
                    EMotionFX::AnimGraphNode* node = modelItemData->m_object.m_node;
                    QPixmap pixmap(QSize(12, 8));
                    QColor nodeColor = node->GetVisualColor();
                    nodeColor.darker(30);
                    pixmap.fill(nodeColor);
                    return pixmap;
                }
            }
        case ROLE_MODEL_ITEM_TYPE:
            return QVariant::fromValue(modelItemData->m_type);
        case ROLE_ID:
            switch (modelItemData->m_type)
            {
            case ModelItemType::NODE:
                return QVariant::fromValue(modelItemData->m_object.m_node->GetId());
            case ModelItemType::TRANSITION:
                return QVariant::fromValue(modelItemData->m_object.m_transition->GetId());
            case ModelItemType::CONNECTION:
                return QVariant::fromValue(modelItemData->m_object.m_connection->GetId());
            default:
                AZ_Assert(false, "Unknown model item type");
            }
            break;
        case ROLE_POINTER:
            return QVariant::fromValue(modelItemData->m_object.m_ptr);
        case ROLE_ANIM_GRAPH_INSTANCE:
            return QVariant::fromValue(modelItemData->m_animGraphInstance);

        case ROLE_RTTI_TYPE_ID:
            AZ_Assert(modelItemData->m_type == ModelItemType::NODE || modelItemData->m_type == ModelItemType::TRANSITION, "Expected a node or transition");
            return QVariant::fromValue(azrtti_typeid(static_cast<EMotionFX::AnimGraphObject*>(modelItemData->m_object.m_ptr)));

        case ROLE_ANIM_GRAPH_OBJECT_PTR:
            AZ_Assert(modelItemData->m_type == ModelItemType::NODE || modelItemData->m_type == ModelItemType::TRANSITION, "Expected a node or transition");
            return QVariant::fromValue(static_cast<EMotionFX::AnimGraphObject*>(modelItemData->m_object.m_ptr));

        case ROLE_NODE_POINTER:
            AZ_Assert(modelItemData->m_type == ModelItemType::NODE, "Expected a node");
            return QVariant::fromValue(modelItemData->m_object.m_node);

        case ROLE_NODE_CAN_ACT_AS_STATE:
            AZ_Assert(modelItemData->m_type == ModelItemType::NODE, "Expected a node");
            return modelItemData->m_object.m_node->GetCanActAsState();

        case ROLE_TRANSITION_POINTER:
            AZ_Assert(modelItemData->m_type == ModelItemType::TRANSITION, "Expected a transition");
            return QVariant::fromValue(modelItemData->m_object.m_transition);

        case ROLE_CONNECTION_POINTER:
            AZ_Assert(modelItemData->m_type == ModelItemType::CONNECTION, "Expected a connection");
            return QVariant::fromValue(modelItemData->m_object.m_connection);

        default:
            break;
        }

        return QVariant();
    }

    void AnimGraphModel::Focus(const QModelIndex& focusIndex)
    {
        QModelIndex newFocusIndex = focusIndex;

        // Always focus column 0
        if (newFocusIndex.column() != 0)
        {
            newFocusIndex = newFocusIndex.sibling(newFocusIndex.row(), 0);
        }
        if (newFocusIndex != m_focus)
        {
            QModelIndex parentFocus = newFocusIndex;
            if (parentFocus.isValid())
            {
                // We store a "parent focus". For a node that has a visual graph or can have children (blend tree, state machine, reference, etc) the parent
                // focus is itself. For a node that doesn't have a visual graph and can't have children (motion, blend, etc) the parent is the first immediate
                // parent that has a visual graph or can have children.

                ModelItemData* parentModelItemData = static_cast<ModelItemData*>(parentFocus.internalPointer());
                if (parentModelItemData->m_type == ModelItemType::NODE)
                {
                    EMotionFX::AnimGraphNode* parentNode = parentModelItemData->m_object.m_node;
                    if (!parentNode->GetHasVisualGraph()
                        && !parentNode->GetCanHaveChildren())
                    {
                        parentFocus = newFocusIndex.parent();
                    }
                }
            }

            QModelIndex currentFocus = m_focus;
            QModelIndex currentParentFocus = m_parentFocus;
            m_focus = newFocusIndex;
            m_parentFocus = parentFocus;
            FocusChanged(m_focus, m_parentFocus, currentFocus, currentParentFocus);
        }
    }

    QModelIndexList AnimGraphModel::FindModelIndexes(EMotionFX::AnimGraphObject* animGraphObject)
    {
        QModelIndexList modelIndexList;

        EMotionFX::AnimGraph* animGraph = animGraphObject->GetAnimGraph();
        const size_t animGraphInstanceCount = animGraph->GetNumAnimGraphInstances();

        // Find all the entries for all anim graph instances
        {
            for (size_t i = 0; i < animGraphInstanceCount; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);

                // Find the model index
                ModelItemData modelItemData(animGraphInstance, animGraphObject);
                AZStd::pair<ModelItemDataSet::const_iterator, ModelItemDataSet::const_iterator> itModelItemData = m_modelItemDataSet.equal_range(&modelItemData);
                for (ModelItemDataSet::const_iterator it = itModelItemData.first; it != itModelItemData.second; ++it)
                {
                    ModelItemData* modelItemData = *it;
                    modelIndexList.push_back(createIndex(modelItemData->m_row, 0, modelItemData));
                }
            }
        }

        // Find for all non-anim graph instances entries
        {
            // Find the model index
            ModelItemData modelItemData(nullptr, animGraphObject);
            AZStd::pair<ModelItemDataSet::const_iterator, ModelItemDataSet::const_iterator> itModelItemData = m_modelItemDataSet.equal_range(&modelItemData);
            for (ModelItemDataSet::const_iterator it = itModelItemData.first; it != itModelItemData.second; ++it)
            {
                ModelItemData* modelItemData = *it;
                modelIndexList.push_back(createIndex(modelItemData->m_row, 0, modelItemData));
            }
        }

        return modelIndexList;
    }

    QModelIndexList AnimGraphModel::FindModelIndexes(EMotionFX::BlendTreeConnection* blendTreeConnection)
    {
        QModelIndexList modelIndexList;

        EMotionFX::AnimGraph* animGraph = blendTreeConnection->GetSourceNode()->GetAnimGraph();
        const size_t animGraphInstanceCount = animGraph->GetNumAnimGraphInstances();

        // Find all the entries for all anim graph instances
        {
            for (size_t i = 0; i < animGraphInstanceCount; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);

                // Find the model index
                ModelItemData modelItemData(animGraphInstance, blendTreeConnection);
                AZStd::pair<ModelItemDataSet::const_iterator, ModelItemDataSet::const_iterator> itModelItemData = m_modelItemDataSet.equal_range(&modelItemData);
                for (ModelItemDataSet::const_iterator it = itModelItemData.first; it != itModelItemData.second; ++it)
                {
                    ModelItemData* modelItemData = *it;
                    modelIndexList.push_back(createIndex(modelItemData->m_row, 0, modelItemData));
                }
            }
        }

        // Find for all non-anim graph instances entries
        {
            // Find the model index
            ModelItemData modelItemData(nullptr, blendTreeConnection);
            AZStd::pair<ModelItemDataSet::const_iterator, ModelItemDataSet::const_iterator> itModelItemData = m_modelItemDataSet.equal_range(&modelItemData);
            for (ModelItemDataSet::const_iterator it = itModelItemData.first; it != itModelItemData.second; ++it)
            {
                ModelItemData* modelItemData = *it;
                modelIndexList.push_back(createIndex(modelItemData->m_row, 0, modelItemData));
            }
        }

        return modelIndexList;
    }


    QModelIndex AnimGraphModel::FindModelIndex(EMotionFX::AnimGraphObject* animGraphObject, EMotionFX::AnimGraphInstance* graphInstance)
    {   
        AZ_Assert(graphInstance, "Anim graph instance cannot be a nullptr.");

        ModelItemData modelItemData(graphInstance, animGraphObject);
        AZStd::pair<ModelItemDataSet::const_iterator, ModelItemDataSet::const_iterator> itModelItemData = m_modelItemDataSet.equal_range(&modelItemData);
        for (ModelItemDataSet::const_iterator it = itModelItemData.first; it != itModelItemData.second; ++it)
        {
            ModelItemData* modelItemData = *it;
            return createIndex(modelItemData->m_row, 0, modelItemData);
        }
        return QModelIndex();
    }


    QModelIndex AnimGraphModel::FindFirstModelIndex(EMotionFX::AnimGraphObject* animGraphObject)
    {
        EMotionFX::AnimGraph* animGraph = animGraphObject->GetAnimGraph();
        const size_t animGraphInstanceCount = animGraph->GetNumAnimGraphInstances();

        // Find all the entries for all anim graph instances
        {
            for (size_t i = 0; i < animGraphInstanceCount; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);

                // Find the model index
                ModelItemData modelItemData(animGraphInstance, animGraphObject);
                AZStd::pair<ModelItemDataSet::const_iterator, ModelItemDataSet::const_iterator> itModelItemData = m_modelItemDataSet.equal_range(&modelItemData);
                for (ModelItemDataSet::const_iterator it = itModelItemData.first; it != itModelItemData.second; ++it)
                {
                    ModelItemData* modelItemData = *it;
                    return createIndex(modelItemData->m_row, 0, modelItemData);
                }
            }
        }

        // Find for all non-anim graph instances entries
        {
            // Find the model index
            ModelItemData modelItemData(nullptr, animGraphObject);
            AZStd::pair<ModelItemDataSet::const_iterator, ModelItemDataSet::const_iterator> itModelItemData = m_modelItemDataSet.equal_range(&modelItemData);
            for (ModelItemDataSet::const_iterator it = itModelItemData.first; it != itModelItemData.second; ++it)
            {
                ModelItemData* modelItemData = *it;
                return createIndex(modelItemData->m_row, 0, modelItemData);
            }
        }

        return QModelIndex();
    }

    QItemSelectionModel& AnimGraphModel::GetSelectionModel()
    {
        return m_selectionModel;
    }

    template<class AnimGraphObjectType>
    AZStd::unordered_map<EMotionFX::AnimGraph*, AZStd::vector<AnimGraphObjectType*>> AnimGraphModel::GetSelectedObjectsOfType() const
    {
        AZStd::unordered_map<EMotionFX::AnimGraph*, AZStd::vector<AnimGraphObjectType*>> objectsByAnimGraph;

        QModelIndexList selectedIndexes = m_selectionModel.selectedRows();
        for (const QModelIndex& modelIndex : selectedIndexes)
        {
            if (modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<ModelItemType>() == ItemTypeForClass<AnimGraphObjectType>::itemType)
            {
                AnimGraphObjectType* animGraphObject = modelIndex.data(RoleForClass<AnimGraphObjectType>::role).template value<AnimGraphObjectType*>();
                objectsByAnimGraph[animGraphObject->GetAnimGraph()].emplace_back(animGraphObject);
            }
        }

        return objectsByAnimGraph;
    }

    // Explicit instanciation.
    template AZStd::unordered_map<EMotionFX::AnimGraph*, AZStd::vector<EMotionFX::AnimGraphNode*>> AnimGraphModel::GetSelectedObjectsOfType<EMotionFX::AnimGraphNode>() const;
    template AZStd::unordered_map<EMotionFX::AnimGraph*, AZStd::vector<EMotionFX::AnimGraphStateTransition*>> AnimGraphModel::GetSelectedObjectsOfType<EMotionFX::AnimGraphStateTransition>() const;
    template AZStd::unordered_map<EMotionFX::AnimGraph*, AZStd::vector<EMotionFX::BlendTreeConnection*>> AnimGraphModel::GetSelectedObjectsOfType<EMotionFX::BlendTreeConnection>() const;

    EMotionFX::AnimGraph* AnimGraphModel::GetFocusedAnimGraph() const
    {
        const QModelIndex focusModelIndex = GetFocus();
        const AnimGraphModel::ModelItemType itemType = focusModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
        const EMotionFX::AnimGraphNode* node = focusModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

        if (itemType != AnimGraphModel::ModelItemType::NODE || !node)
        {
            return nullptr;
        }

        return node->GetAnimGraph();
    }

    bool AnimGraphModel::CheckAnySelectedNodeBelongsToReferenceGraph() const
    {
        QModelIndexList selectedIndexes = m_selectionModel.selectedRows();
        for (QModelIndex& modelIndex : selectedIndexes)
        {
            if (modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<ModelItemType>() == ItemTypeForClass<EMotionFX::AnimGraphNode>::itemType)
            {
                EMotionFX::AnimGraphNode* animGraphNode = modelIndex.data(RoleForClass<EMotionFX::AnimGraphNode>::role).value<EMotionFX::AnimGraphNode*>();

                // find the root item, check if the root node and the selected node points to the same anim graph.
                QModelIndex& parentIndex = modelIndex;
                while (parentIndex.parent().isValid())
                {
                    parentIndex = parentIndex.parent();
                }
                EMotionFX::AnimGraphNode* rootNode = parentIndex.data(RoleForClass<EMotionFX::AnimGraphNode>::role).value<EMotionFX::AnimGraphNode*>();
                
                if (animGraphNode->GetAnimGraph() != rootNode->GetAnimGraph())
                {
                    return true;
                }
            }
        }
        return false;
    }

    void AnimGraphModel::Reset()
    {
        beginResetModel();
        for (ModelItemData* modelItemData : m_modelItemDataSet)
        {
            delete modelItemData;
        }
        m_modelItemDataSet.clear();
        m_rootModelItemData.clear();
        endResetModel();
    }

    void AnimGraphModel::SetAnimGraphInstance(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphInstance* currentAnimGraphInstance, EMotionFX::AnimGraphInstance* newAnimGraphInstance)
    {
        AZ_Assert(currentAnimGraphInstance != newAnimGraphInstance, "newAnimGraphInstance should be different than currentAnimGraphInstance");

        AZStd::vector<ModelItemData*>::const_iterator itRootModelItemData = m_rootModelItemData.begin();
        for (; itRootModelItemData != m_rootModelItemData.end(); ++itRootModelItemData)
        {
            ModelItemData* modelItemData = *itRootModelItemData;
            if (modelItemData->m_animGraphInstance == currentAnimGraphInstance && modelItemData->m_object.m_node->GetAnimGraph() == animGraph)
            {
                // Since the anim graph instance changes how elements get hashed, we need to take them out of m_modelItemDataSet
                // patch them and add them again. Since the index won't get affected, there is no need to notify the UI.
                RecursiveSetAnimGraphInstance(modelItemData, currentAnimGraphInstance, newAnimGraphInstance);

                break; // we only have one element (and the iterators get invalidated in RecursiveSetAnimGraphInstance)
            }
        }
    }

    void AnimGraphModel::RecursiveSetAnimGraphInstance(ModelItemData* modelItemData, EMotionFX::AnimGraphInstance* currentAnimGraphInstance, EMotionFX::AnimGraphInstance* newAnimGraphInstance)
    {
        if (modelItemData->m_animGraphInstance == currentAnimGraphInstance) // otherwise we reached a reference node and we can stop the patching
        {
            // remove it and add it after patching since the hashing of the element will change
            m_modelItemDataSet.erase(modelItemData);
            modelItemData->m_animGraphInstance = newAnimGraphInstance;
            m_modelItemDataSet.insert(modelItemData);

            for (ModelItemData* child : modelItemData->m_children)
            {
                if (child->m_type == ModelItemType::NODE && azrtti_typeid(child->m_object.m_node) == azrtti_typeid<EMotionFX::AnimGraphReferenceNode>())
                {
                    RecursiveSetReferenceNodeAnimGraphInstance(child, currentAnimGraphInstance, newAnimGraphInstance);
                }
                else
                {
                    RecursiveSetAnimGraphInstance(child, currentAnimGraphInstance, newAnimGraphInstance);
                }
            }
        }
    }

    void AnimGraphModel::RecursiveSetReferenceNodeAnimGraphInstance(ModelItemData* modelItemData, EMotionFX::AnimGraphInstance* currentAnimGraphInstance, EMotionFX::AnimGraphInstance* newAnimGraphInstance)
    {
        AZ_Assert(modelItemData->m_type == ModelItemType::NODE && azrtti_typeid(modelItemData->m_object.m_node) == azrtti_typeid<EMotionFX::AnimGraphReferenceNode>(), "Expected to have a reference node in modelItemData");

        if (modelItemData->m_animGraphInstance == currentAnimGraphInstance) // otherwise we reached a reference node and we can stop the patching
        {
            // Remove it and add it after patching since the hashing of the element will change
            m_modelItemDataSet.erase(modelItemData);
            // The reference node still has the parent anim graph instance, the children will have a different one
            modelItemData->m_animGraphInstance = newAnimGraphInstance;
            m_modelItemDataSet.insert(modelItemData);

            EMotionFX::AnimGraphReferenceNode* referenceNode = static_cast<EMotionFX::AnimGraphReferenceNode*>(modelItemData->m_object.m_node);
            EMotionFX::AnimGraph* referencedAnimGraph = referenceNode->GetReferencedAnimGraph();
            EMotionFX::AnimGraphInstance* currentReferencedAnimGraphInstance = nullptr;
            EMotionFX::AnimGraphInstance* newReferencedAnimGraphInstance = referencedAnimGraph ? referenceNode->GetReferencedAnimGraphInstance(newAnimGraphInstance) : nullptr;

            // Patch the alias
            AliasMap::const_iterator itAlias = m_aliasMap.find(modelItemData);
            if (itAlias != m_aliasMap.end())
            {
                // Found an alias (a root state machine). We need to patch the anim graph instance
                ModelItemData* rootStateMachineItemData = itAlias->second;
                m_modelItemDataSet.erase(rootStateMachineItemData);
                currentReferencedAnimGraphInstance = rootStateMachineItemData->m_animGraphInstance;
                rootStateMachineItemData->m_animGraphInstance = newReferencedAnimGraphInstance;
                m_modelItemDataSet.insert(rootStateMachineItemData);
            }

            for (ModelItemData* child : modelItemData->m_children)
            {
                if (child->m_type == ModelItemType::NODE && azrtti_typeid(child->m_object.m_node) == azrtti_typeid<EMotionFX::AnimGraphReferenceNode>())
                {
                    RecursiveSetReferenceNodeAnimGraphInstance(child, currentReferencedAnimGraphInstance, newReferencedAnimGraphInstance);
                }
                else
                {
                    RecursiveSetAnimGraphInstance(child, currentReferencedAnimGraphInstance, newReferencedAnimGraphInstance);
                }
            }
        }
    }

    QModelIndex AnimGraphModel::Add(EMotionFX::AnimGraph* animGraph)
    {
        // Check if the AnimGraph was already inserted as a root, if it was, remove it with all
        // its children before adding it again.
        Remove(animGraph);

        EMotionFX::AnimGraphStateMachine* rootStateMachine = animGraph->GetRootStateMachine();
        AZ_Assert(rootStateMachine, "Anim graph with null root state machine");

        const int row = static_cast<int>(m_rootModelItemData.size());

        beginInsertRows(QModelIndex(), row, row);
        ModelItemData* graphModelItemData = RecursivelyAddNode(nullptr, rootStateMachine, nullptr, row);
        endInsertRows();

        return createIndex(row, 0, graphModelItemData);
    }

    void AnimGraphModel::Remove(EMotionFX::AnimGraph* animGraph)
    {
        AZStd::vector<ModelItemData*>::const_iterator itRootModelItemData = m_rootModelItemData.begin();
        for (; itRootModelItemData != m_rootModelItemData.end(); ++itRootModelItemData)
        {
            ModelItemData* modelItemData = *itRootModelItemData;
            // Root elements are only nodes (not transitions nor connections). Therefore, we can
            // find the root node in the anim graph to start removing
            if (modelItemData->m_type == ModelItemType::NODE && modelItemData->m_object.m_node->GetAnimGraph() == animGraph)
            {
                const QModelIndex modelIndex = createIndex(modelItemData->m_row, 0, modelItemData);
                AboutToBeRemoved(modelIndex);
                RemovePending();
                break;
            }
        }
    }

    void AnimGraphModel::OnSyncVisualObject(EMotionFX::AnimGraphObject* object)
    {
        Edited(object);
    }

    void AnimGraphModel::OnReferenceAnimGraphChanged(EMotionFX::AnimGraphReferenceNode* referenceNode)
    {
        // Locate the model indexes for the referenceNode in question, remove the children so we don't loose the connections
        const QModelIndexList modelIndexList = FindModelIndexes(referenceNode);
        QModelIndexList childModelIndexList;
        for (const QModelIndex& modelIndex : modelIndexList)
        {
            RemoveAlias(modelIndex);
            const int rows = rowCount(modelIndex);
            for (int i = rows - 1; i >= 0; --i)
            {
                const QModelIndex childItem = modelIndex.child(i, 0);
                childModelIndexList.push_back(childItem);
            }
        }
        if (!childModelIndexList.empty())
        {
            AboutToBeRemoved(childModelIndexList);
            RemovePending();
        }

        EMotionFX::AnimGraph* referencedAnimGraph = referenceNode->GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            for (const QModelIndex& modelIndex : modelIndexList)
            {
                ModelItemData* modelItemData = static_cast<ModelItemData*>(modelIndex.internalPointer());
                EMotionFX::AnimGraphInstance* animGraphInstance = modelItemData->m_animGraphInstance;
                EMotionFX::AnimGraphStateMachine* rootStateMachine = referencedAnimGraph->GetRootStateMachine();

                const uint32 rowCount = rootStateMachine->GetNumConnections() + rootStateMachine->GetNumChildNodes() + static_cast<uint32>(rootStateMachine->GetNumTransitions());
                if (rowCount > 0)
                {
                    const QModelIndex referenceNodeModelIndex = createIndex(modelItemData->m_row, 0, modelItemData);
                    beginInsertRows(referenceNodeModelIndex, 0, rowCount - 1);
                    RecursivelyAddReferenceNodeContents(animGraphInstance, referenceNode, modelItemData, 0);
                    endInsertRows();
                }
                else
                {
                    // If it is empty we dont need to notify the UI
                    RecursivelyAddReferenceNodeContents(animGraphInstance, referenceNode, modelItemData, 0);
                }
            }
        }
    }

    AnimGraphModel::ModelItemData::ModelItemData(EMotionFX::AnimGraphNode* animGraphNode, EMotionFX::AnimGraphInstance* animGraphInstance, ModelItemData* parent, int row)
        : m_type(ModelItemType::NODE)
        , m_object(animGraphNode)
        , m_animGraphInstance(animGraphInstance)
        , m_parent(parent)
        , m_row(row)
    {
        if (m_parent)
        {
            m_parent->m_children.emplace_back(this);
        }
    }

    AnimGraphModel::ModelItemData::ModelItemData(EMotionFX::AnimGraphStateTransition* animGrapStateTransition, EMotionFX::AnimGraphInstance* animGraphInstance, ModelItemData* parent, int row)
        : m_type(ModelItemType::TRANSITION)
        , m_object(animGrapStateTransition)
        , m_animGraphInstance(animGraphInstance)
        , m_parent(parent)
        , m_row(row)
    {
        if (m_parent)
        {
            m_parent->m_children.emplace_back(this);
        }
    }

    AnimGraphModel::ModelItemData::ModelItemData(EMotionFX::BlendTreeConnection* animGraphConnection, EMotionFX::AnimGraphInstance* animGraphInstance, ModelItemData* parent, int row)
        : m_type(ModelItemType::CONNECTION)
        , m_object(animGraphConnection)
        , m_animGraphInstance(animGraphInstance)
        , m_parent(parent)
        , m_row(row)
    {
        if (m_parent)
        {
            m_parent->m_children.emplace_back(this);
        }
    }

    AnimGraphModel::ModelItemData* AnimGraphModel::RecursivelyAddNode(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* node, ModelItemData* parent, int row)
    {
        AZ_Assert(node, "Expected a node");

        ModelItemData* currentModelItemData = new ModelItemData(node, animGraphInstance, parent, row);
        m_modelItemDataSet.emplace(currentModelItemData);

        if (!parent)
        {
            row = static_cast<int>(m_rootModelItemData.size());
            currentModelItemData->m_row = row;
            m_rootModelItemData.emplace_back(currentModelItemData);
        }

        int childRow = 0;
        const uint32 connectionCount = node->GetNumConnections();
        for (uint32 i = 0; i < connectionCount; ++i)
        {
            m_modelItemDataSet.emplace(new ModelItemData(node->GetConnection(i), animGraphInstance, currentModelItemData, childRow + i));
        }
        childRow += connectionCount;

        const uint32 childNodeCount = node->GetNumChildNodes();
        for (uint32 i = 0; i < childNodeCount; ++i)
        {
            RecursivelyAddNode(animGraphInstance, node->GetChildNode(i), currentModelItemData, childRow + i);
        }
        childRow += childNodeCount;

        const AZ::TypeId nodeTypeId = azrtti_typeid(node);
        if (nodeTypeId == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(node);
            const size_t childTransitionCount = stateMachine->GetNumTransitions();
            for (size_t i = 0; i < childTransitionCount; ++i)
            {
                AddTransition(animGraphInstance, stateMachine->GetTransition(i), currentModelItemData, childRow + static_cast<int>(i));
            }
            childRow += static_cast<int>(childTransitionCount);
        }
        else if (nodeTypeId == azrtti_typeid<EMotionFX::AnimGraphReferenceNode>())
        {
            EMotionFX::AnimGraphReferenceNode* referenceNode = static_cast<EMotionFX::AnimGraphReferenceNode*>(node);
            RecursivelyAddReferenceNodeContents(animGraphInstance, referenceNode, currentModelItemData, childRow);
        }
        return currentModelItemData;
    }

    void AnimGraphModel::RecursivelyAddReferenceNodeContents(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphReferenceNode* referenceNode, ModelItemData* referenceNodeModelItemData, int row)
    {
        EMotionFX::AnimGraph* referencedAnimGraph = referenceNode->GetReferencedAnimGraph();

        if (referencedAnimGraph)
        {
            // We dont want to add the root state machine as an common entry into the model because this will create extra levels in the views
            // we dont want. Instead, we are going to add the children of the root state machine as they were the children of the reference node.
            // Then we will add an "alias" item with the root state machine so we can find the right entry when the referenced root state machine is
            // looked up.
            EMotionFX::AnimGraphStateMachine* rootStateMachine = referencedAnimGraph->GetRootStateMachine();
            EMotionFX::AnimGraphInstance* referencedAnimGraphInstance = referenceNode->GetReferencedAnimGraphInstance(animGraphInstance);

            const uint32 rootConnectionCount = rootStateMachine->GetNumConnections();
            for (uint32 i = 0; i < rootConnectionCount; ++i)
            {
                m_modelItemDataSet.emplace(new ModelItemData(rootStateMachine->GetConnection(i), referencedAnimGraphInstance, referenceNodeModelItemData, row + i));
            }
            row += rootConnectionCount;

            const uint32 rootChildNodeCount = rootStateMachine->GetNumChildNodes();
            for (uint32 i = 0; i < rootChildNodeCount; ++i)
            {
                RecursivelyAddNode(referencedAnimGraphInstance, rootStateMachine->GetChildNode(i), referenceNodeModelItemData, row + i);
            }
            row += rootChildNodeCount;

            const size_t rootChildTransitionCount = rootStateMachine->GetNumTransitions();
            for (size_t i = 0; i < rootChildTransitionCount; ++i)
            {
                AddTransition(referencedAnimGraphInstance, rootStateMachine->GetTransition(i), referenceNodeModelItemData, row + static_cast<int>(i));
            }
            row += static_cast<int>(rootChildTransitionCount);

            // Now we add the "alias" item
            ModelItemData* rootStateMachineItem = new ModelItemData(rootStateMachine, referencedAnimGraphInstance, nullptr, referenceNodeModelItemData->m_row);
            rootStateMachineItem->m_parent = referenceNodeModelItemData;
            m_modelItemDataSet.emplace(rootStateMachineItem);
            m_aliasMap.emplace(referenceNodeModelItemData, rootStateMachineItem);
        }
    }

    void AnimGraphModel::AddConnection(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::BlendTreeConnection* connection, ModelItemData* parent, int row)
    {
        AZ_Assert(connection, "Expected a connection");
        AZ_Assert(parent, "Expected a parent node data for the transition");

        ModelItemDataSet::const_iterator itInserted = m_modelItemDataSet.emplace(new ModelItemData(connection, animGraphInstance, parent, row));
        AZ_Assert(itInserted != m_modelItemDataSet.end(), "Expected to insert element");
    }

    void AnimGraphModel::AddTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition, ModelItemData* parent, int row)
    {
        AZ_Assert(transition, "Expected a transition");
        AZ_Assert(parent, "Expected a parent node data for the transition");

        ModelItemDataSet::const_iterator itInserted = m_modelItemDataSet.emplace(new ModelItemData(transition, animGraphInstance, parent, row));
        AZ_Assert(itInserted != m_modelItemDataSet.end(), "Expected to insert element");
    }

    void AnimGraphModel::Remove(ModelItemData* modelItemData)
    {
        AZ_Assert(modelItemData, "Expected a valid parent");
        ModelItemData* parentModelItemData = modelItemData->m_parent;

        if (parentModelItemData)
        {
            // Remove myself from the parent and update siblings
            AZStd::vector<ModelItemData*>::iterator itSibling = parentModelItemData->m_children.erase(parentModelItemData->m_children.begin() + modelItemData->m_row);
            for (; itSibling != parentModelItemData->m_children.end(); ++itSibling)
            {
                (*itSibling)->m_row -= 1;
            }
        }
        else if (modelItemData->m_row >= 0)
        {
            AZ_Assert(m_rootModelItemData[modelItemData->m_row] == modelItemData, "Invalid root element");
            AZStd::vector<ModelItemData*>::iterator itSibling = m_rootModelItemData.erase(m_rootModelItemData.begin() + modelItemData->m_row);
            for (; itSibling != m_rootModelItemData.end(); ++itSibling)
            {
                (*itSibling)->m_row -= 1;
            }
        }

        AZStd::pair<ModelItemDataSet::const_iterator, ModelItemDataSet::const_iterator> itModelItemData = m_modelItemDataSet.equal_range(modelItemData);
        for (ModelItemDataSet::const_iterator it = itModelItemData.first; it != itModelItemData.second; ++it)
        {
            if (*it == modelItemData)
            {
                m_modelItemDataSet.erase(it);
                break;
            }
        }

        delete modelItemData;
    }

    void AnimGraphModel::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetType() == azrtti_typeid<EMotionFX::Integration::AnimGraphAsset>())
        {
            EMotionFX::AnimGraph* animGraph = asset.GetAs<EMotionFX::Integration::AnimGraphAsset>()->GetAnimGraph();
            if (!animGraph->GetIsOwnedByRuntime())
            {
                const QModelIndex addedIndex = Add(animGraph);
                m_modelIndexByAssetId.emplace(asset.GetId(), addedIndex);
            }
        }
    }

    void AnimGraphModel::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetType() == azrtti_typeid<EMotionFX::Integration::AnimGraphAsset>())
        {
            // Remove the old AnimGraph that was used for this asset before adding the new one (if it exists)
            auto itModelIndex = m_modelIndexByAssetId.find(asset.GetId());
            if (itModelIndex != m_modelIndexByAssetId.end())
            {
                EMotionFX::AnimGraph* removeGraph = data(itModelIndex->second, AnimGraphModel::ROLE_ANIM_GRAPH_OBJECT_PTR).value<EMotionFX::AnimGraphObject*>()->GetAnimGraph();
                Remove(removeGraph);
            }

            EMotionFX::AnimGraph* animGraph = asset.GetAs<EMotionFX::Integration::AnimGraphAsset>()->GetAnimGraph();
            if (!animGraph->GetIsOwnedByRuntime())
            {
                const QModelIndex addedIndex = Add(animGraph);
                m_modelIndexByAssetId[asset.GetId()] = addedIndex;
            }
        }
    }

    void AnimGraphModel::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType)
    {
        if (assetType == azrtti_typeid<EMotionFX::Integration::AnimGraphAsset>())
        {
            auto itModelIndex = m_modelIndexByAssetId.find(assetId);
            if (itModelIndex != m_modelIndexByAssetId.end())
            {
                if (itModelIndex->second.isValid())
                {
                    AboutToBeRemoved(itModelIndex->second);
                    RemovePending();
                    m_modelIndexByAssetId.erase(itModelIndex);
                }
            }
        }
    }

    bool AnimGraphModel::NodeAdded(EMotionFX::AnimGraphNode* animGraphNode)
    {
        EMotionFX::AnimGraphNode* parentNode = animGraphNode->GetParentNode();
        if (parentNode)
        {
            QModelIndexList parentNodeIndexes = FindModelIndexes(parentNode);
            for (const QModelIndex& parentNodeIndex : parentNodeIndexes)
            {
                QModelIndex aliasedParentNodeIndex = parentNodeIndex; // for the reference node case
                GetParentIfReferencedRootStateMachine(aliasedParentNodeIndex);

                const int fromRow = rowCount(aliasedParentNodeIndex);
                beginInsertRows(aliasedParentNodeIndex, fromRow, fromRow);

                ModelItemData* parentModelItemData = static_cast<ModelItemData*>(parentNodeIndex.internalPointer());
                ModelItemData* aliasedParentNodeIndexData = static_cast<ModelItemData*>(aliasedParentNodeIndex.internalPointer());

                RecursivelyAddNode(parentModelItemData->m_animGraphInstance, animGraphNode, aliasedParentNodeIndexData, fromRow);
                endInsertRows();
            }
        }
        else
        {
            EMotionFX::AnimGraph* animGraph = animGraphNode->GetAnimGraph();
            const size_t animGraphInstanceCount = animGraph->GetNumAnimGraphInstances();
            if (animGraphInstanceCount != 0)
            {
                for (size_t i = 0; i < animGraphInstanceCount; ++i)
                {
                    const int fromRow = static_cast<int>(m_rootModelItemData.size());
                    beginInsertRows(QModelIndex(), fromRow, fromRow);
                    RecursivelyAddNode(animGraph->GetAnimGraphInstance(i), animGraphNode, nullptr, fromRow);
                    endInsertRows();
                }
            }
            else
            {
                const int fromRow = static_cast<int>(m_rootModelItemData.size());
                beginInsertRows(QModelIndex(), fromRow, fromRow);
                RecursivelyAddNode(nullptr, animGraphNode, nullptr, fromRow);
                endInsertRows();
            }
        }

        return true;
    }

    bool AnimGraphModel::ConnectionAdded(EMotionFX::AnimGraphNode* animGraphTargetNode, EMotionFX::BlendTreeConnection* animGraphConnection)
    {
        QModelIndexList targetNodeIndexes = FindModelIndexes(animGraphTargetNode);
        for (const QModelIndex& targetNodeIndex : targetNodeIndexes)
        {
            ModelItemData* targetNodeModelItemData = static_cast<ModelItemData*>(targetNodeIndex.internalPointer());
            const int fromRow = static_cast<int>(targetNodeModelItemData->m_children.size());

            beginInsertRows(targetNodeIndex, fromRow, fromRow);
            AddConnection(targetNodeModelItemData->m_animGraphInstance, animGraphConnection, targetNodeModelItemData, fromRow);
            endInsertRows();
        }
        return true;
    }


    bool AnimGraphModel::TransitionAdded(EMotionFX::AnimGraphStateTransition* animGraphTransition)
    {
        EMotionFX::AnimGraphNode* targetParent = animGraphTransition->GetTargetNode()->GetParentNode();
        QModelIndexList targetParentIndexes = FindModelIndexes(targetParent);
        for (const QModelIndex& targetParentIndex : targetParentIndexes)
        {
            QModelIndex aliasedTargetParentIndex = targetParentIndex; // for the reference node case
            GetParentIfReferencedRootStateMachine(aliasedTargetParentIndex);

            const int fromRow = rowCount(aliasedTargetParentIndex);
            beginInsertRows(aliasedTargetParentIndex, fromRow, fromRow);

            ModelItemData* targetParentModelItemData = static_cast<ModelItemData*>(targetParentIndex.internalPointer());
            ModelItemData* aliasedTargetModelItemData = static_cast<ModelItemData*>(aliasedTargetParentIndex.internalPointer());

            AddTransition(targetParentModelItemData->m_animGraphInstance, animGraphTransition, aliasedTargetModelItemData, fromRow);
            endInsertRows();
        }
        return true;
    }

    void AnimGraphModel::AboutToBeRemoved(const QModelIndexList& modelIndexList)
    {
        for (const QModelIndex& modelIndex : modelIndexList)
        {
            AboutToBeRemoved(modelIndex);
        }
    }

    void AnimGraphModel::AboutToBeRemoved(const QModelIndex& modelIndex)
    {
        m_pendingToDeleteIndices.emplace_back(modelIndex);

        // Find the focus element that needs to be set before deleting
        if (m_focus.isValid())
        {
            // Collect recursively the parents of m_focus, then remove those contained in m_pendingToDeleteIndices.
            // Note that a parent could be in m_pendingToDeleteIndices, so we need to remove all the children focus as well.
            AZStd::vector<QModelIndex> focusParents;

            // 1) collect recursively the parents
            QModelIndex currentFocus = m_focus;
            while (currentFocus.isValid())
            {
                focusParents.emplace_back(currentFocus);
                currentFocus = currentFocus.parent();
            }

            // 2) Starting from the parent-most, check if they are in m_pendingToDeleteIndices, if they are, we can stop there
            // since all children will be removed as well. If not, we continue finding the child-most focus element
            const int focusCount = static_cast<int>(focusParents.size());
            for (int i = focusCount - 1; i >= 0; --i)
            {
                if (AZStd::find(m_pendingToDeleteIndices.begin(), m_pendingToDeleteIndices.end(), focusParents[i]) != m_pendingToDeleteIndices.end())
                {
                    // 2.a.) we found the element, remove everything up to this point since all children will be removed
                    focusParents.erase(focusParents.begin(), focusParents.begin() + i + 1);
                    break; // nothing else to do
                }
            }

            // 3) Move the focus to thew new element, if the focus is maintained, the Focus method won't do anything
            if (!focusParents.empty())
            {
                // If we still have an element, this is our new focus
                m_pendingFocus = *focusParents.begin();
            }
        }

        // Care must be taken when removing items from the model. In general,
        // this model does not behave the way that Qt expects. The preferred
        // way is to call beginRemoveRows() _before_ the underlying objects are
        // destroyed. The model does not currently guarantee that. By the time
        // that RemovePending() is called, the AnimGraphObjects that the model
        // uses internally have already been destroyed. So anything that may
        // change the current selection (which will re-query the model for
        // data) has to be done before RemovePending() is called. So, we clear
        // the focus item whenever we're about to remove items.
        Focus();
    }

    void AnimGraphModel::RemovePending()
    {
        if (m_pendingToDeleteIndices.empty())
        {
            return; // early out, nothing to do
        }

        for (const QPersistentModelIndex& modelIndex : m_pendingToDeleteIndices)
        {
            if (modelIndex.isValid())
            {
                const QModelIndex parentModelIndex = modelIndex.parent();
                beginRemoveRows(parentModelIndex, modelIndex.row(), modelIndex.row());
                Remove(modelIndex);
                endRemoveRows();
            }
        }
        m_pendingToDeleteIndices.clear();

        Focus(m_pendingFocus);
        m_pendingFocus = QPersistentModelIndex();
    }

    void AnimGraphModel::Remove(const QModelIndexList& modelIndexList)
    {
        for (const QModelIndex& modelIndex : modelIndexList)
        {
            Remove(modelIndex);
        }
    }

    void AnimGraphModel::Remove(const QModelIndex& modelIndex)
    {
        RemoveChildren(modelIndex);

        ModelItemData* modelItemData = static_cast<ModelItemData*>(modelIndex.internalPointer());
        Remove(modelItemData);
    }

    void AnimGraphModel::RemoveAlias(const QModelIndex& modelIndex)
    {
        // Check if we have an alias for this item
        AliasMap::const_iterator itAlias = m_aliasMap.find(static_cast<ModelItemData*>(modelIndex.internalPointer()));
        if (itAlias != m_aliasMap.end())
        {
            // Remove the alias
            ModelItemData* aliasItem = itAlias->second;

            AZStd::pair<ModelItemDataSet::const_iterator, ModelItemDataSet::const_iterator> itModelItemData = m_modelItemDataSet.equal_range(aliasItem);
            for (ModelItemDataSet::const_iterator it = itModelItemData.first; it != itModelItemData.second; ++it)
            {
                if (*it == aliasItem)
                {
                    m_modelItemDataSet.erase(it);
                    break;
                }
            }

            delete aliasItem;

            m_aliasMap.erase(itAlias);
        }
    }

    void AnimGraphModel::RemoveChildren(const QModelIndexList& modelIndexList)
    {
        for (const QModelIndex& modelIndex : modelIndexList)
        {
            RemoveChildren(modelIndex);
        }
    }

    void AnimGraphModel::RemoveChildren(const QModelIndex& modelIndex)
    {
        RemoveAlias(modelIndex);

        // remove all the children
        const int rows = modelIndex.model()->rowCount(modelIndex);
        for (int i = rows - 1; i >= 0; --i)
        {
            Remove(modelIndex.child(i, 0));
        }
    }

    bool AnimGraphModel::Edited(EMotionFX::AnimGraphObject* animGraphObject, const QVector<int>& roles)
    {
        QModelIndexList modelIndexes = FindModelIndexes(animGraphObject);

        for (QModelIndex& modelIndex : modelIndexes)
        {
            GetParentIfReferencedRootStateMachine(modelIndex);
            dataChanged(modelIndex, modelIndex, roles);
        }

        return true;
    }

    bool AnimGraphModel::ParameterEdited(EMotionFX::AnimGraph* animGraph)
    {
        MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
        animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);

        const uint32 parameterNodeCount = parameterNodes.GetLength();
        for (uint32 i = 0; i < parameterNodeCount; ++i)
        {
            QModelIndexList modelIndexes = FindModelIndexes(parameterNodes[i]);
            for (QModelIndex& modelIndex : modelIndexes)
            {
                GetParentIfReferencedRootStateMachine(modelIndex);
                dataChanged(modelIndex, modelIndex);
            }
        }

        emit ParametersChanged(animGraph);

        return true;
    }

    bool AnimGraphModel::MotionEdited()
    {
        MCore::Array<EMotionFX::AnimGraphNode*> motionNodes;

        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            motionNodes.Clear();
            animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), &motionNodes);

            const uint32 motionNodesCount = motionNodes.GetLength();
            for (uint32 j = 0; j < motionNodesCount; ++j)
            {
                QModelIndexList modelIndexes = FindModelIndexes(motionNodes[j]);
                for (QModelIndex& modelIndex : modelIndexes)
                {
                    GetParentIfReferencedRootStateMachine(modelIndex);
                    dataChanged(modelIndex, modelIndex);
                }
            }
        }

        return true;
    }

    void AnimGraphModel::GetParentIfReferencedRootStateMachine(QModelIndex& modelIndex) const
    {
        ModelItemData* modelItemData = static_cast<ModelItemData*>(modelIndex.internalPointer());
        if (modelItemData->m_type == ModelItemType::NODE && azrtti_typeid(modelItemData->m_object.m_node) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(modelItemData->m_object.m_node);
            if (!stateMachine->GetParentNode() && modelItemData->m_parent && modelItemData->m_parent->m_type == ModelItemType::NODE && azrtti_typeid(modelItemData->m_parent->m_object.m_node) == azrtti_typeid<EMotionFX::AnimGraphReferenceNode>())
            {
                modelIndex = createIndex(modelItemData->m_parent->m_row, 0, modelItemData->m_parent);
            }
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.moc>
