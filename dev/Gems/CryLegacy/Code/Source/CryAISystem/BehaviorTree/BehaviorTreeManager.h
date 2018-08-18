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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYAISYSTEM_BEHAVIORTREE_BEHAVIORTREEMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_BEHAVIORTREE_BEHAVIORTREEMANAGER_H
#pragma once

#include "BehaviorTree/IBehaviorTree.h"
#include "BehaviorTree/NodeFactory.h"
#include "BehaviorTree/BehaviorTreeManagerRequestBus.h"

namespace BehaviorTree
{
#ifdef USING_BEHAVIOR_TREE_EXECUTION_STACKS_FILE_LOG
    class ExecutionStackFileLogger;
#endif

    class BehaviorTreeManager
        : public IBehaviorTreeManager
        , private BehaviorTreeManagerRequestBus::Handler
    {
    public:
        BehaviorTreeManager();
        ~BehaviorTreeManager() override = default;

        // IBehaviorTreeManager
        virtual INodeFactory& GetNodeFactory() override;
#ifdef USING_BEHAVIOR_TREE_SERIALIZATION
        virtual Serialization::ClassFactory<INode>& GetNodeSerializationFactory() override;
#endif
        virtual void LoadFromDiskIntoCache(const char* behaviorTreeName) override;
        virtual bool StartModularBehaviorTree(const AZ::EntityId entityId, const char* treeName) override;
        virtual bool StartModularBehaviorTreeFromXml(const AZ::EntityId entityId, const char* treeName, XmlNodeRef treeXml) override;
        virtual void StopModularBehaviorTree(const AZ::EntityId entityId) override;


        virtual void HandleEvent(const AZ::EntityId entityId, Event& event) override;

        virtual Variables::Collection* GetBehaviorVariableCollection_Deprecated(const AZ::EntityId entityId) const override;
        virtual const Variables::Declarations* GetBehaviorVariableDeclarations_Deprecated(const AZ::EntityId entityId) const override;
        // ~IBehaviorTreeManager

        void Reset();
        void Update();

        size_t GetTreeInstanceCount() const;
        EntityId GetTreeInstanceEntityIdByIndex(const size_t index) const;

        BehaviorTreeInstancePtr CreateBehaviorTreeInstanceFromXml(const char* behaviorTreeName, XmlNodeRef behaviorTreeXmlNode);

#ifdef CRYAISYSTEM_DEBUG
        void DrawMemoryInformation();
#endif

    private:
        //////////////////////////////////////////////////////////////////////////
        // BehaviortreeRequestBus::Handler
        bool StartBehaviorTree(const AZ::EntityId& entityId, const char* treeName) override;
        void StopBehaviorTree(const AZ::EntityId& entityId) override;
        AZStd::vector<AZ::Crc32> GetVariableNameCrcs(const AZ::EntityId& entityId) override;
        bool GetVariableValue(const AZ::EntityId& entityId, AZ::Crc32 variableNameCrc) override;
        void SetVariableValue(const AZ::EntityId& entityId, AZ::Crc32 variableNameCrc, bool newValue) override;
        //////////////////////////////////////////////////////////////////////////

        BehaviorTreeInstancePtr CreateBehaviorTreeInstanceFromDiskCache(const char* behaviorTreeName);
        BehaviorTreeInstancePtr LoadFromCache(const char* behaviorTreeName);
        bool StartBehaviorInstance(const AZ::EntityId entityId, BehaviorTreeInstancePtr instance, const char* treeName);
        void StopAllBehaviorTreeInstances();

        bool LoadBehaviorTreeTemplate(const char* behaviorTreeName, XmlNodeRef behaviorTreeXmlNode, BehaviorTreeTemplate& behaviorTreeTemplate);

        BehaviorTreeInstance* GetBehaviorTree(const AZ::EntityId entityId) const;

#if defined(DEBUG_MODULAR_BEHAVIOR_TREE)
        void UpdateDebugVisualization(UpdateContext updateContext, const AZ::EntityId entityId, DebugTree debugTree, BehaviorTreeInstance& instance, IEntity* agentEntity);
#endif // DEBUG_MODULAR_BEHAVIOR_TREE

#ifdef USING_BEHAVIOR_TREE_EXECUTION_STACKS_FILE_LOG
        void UpdateExecutionStackLogging(UpdateContext updateContext, const AZ::EntityId entityId, DebugTree debugTree, BehaviorTreeInstance& instance);
#endif

        typedef string BehaviorTreeName;
        typedef AZStd::unordered_map<BehaviorTreeName, BehaviorTreeTemplatePtr, stl::hash_string_caseless<string>, stl::equality_string_caseless<string> > BehaviorTreeCache;
        BehaviorTreeCache m_behaviorTreeCache;

        AZStd::unique_ptr<NodeFactory> m_nodeFactory;

        typedef VectorMap<AZ::EntityId, BehaviorTreeInstancePtr> Instances;
        Instances m_instances;

        typedef std::vector<AZ::EntityId> EntityIdVector;
#ifdef DEBUG_MODULAR_BEHAVIOR_TREE
        EntityIdVector m_errorStatusTrees;
#endif // DEBUG_MODULAR_BEHAVIOR_TREE

#ifdef USING_BEHAVIOR_TREE_EXECUTION_STACKS_FILE_LOG
        typedef AZStd::shared_ptr<ExecutionStackFileLogger> ExecutionStackFileLoggerPtr;
        typedef VectorMap<AZ::EntityId, ExecutionStackFileLoggerPtr> ExecutionStackFileLoggerInstances;
        ExecutionStackFileLoggerInstances m_executionStackFileLoggerInstances;
#endif
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_BEHAVIORTREE_BEHAVIORTREEMANAGER_H
