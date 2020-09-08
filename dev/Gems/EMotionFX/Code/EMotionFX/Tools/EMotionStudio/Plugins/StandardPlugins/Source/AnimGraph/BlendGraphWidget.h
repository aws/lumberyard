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

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraphWidget.h>
#include <MCore/Source/CommandGroup.h>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QItemSelection);

namespace EMotionFX
{
    class AnimGraphNode;
    class AnimGraphStateTransition;
    class BlendTreeConnection;
}

namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;


    class BlendGraphWidget
        : public NodeGraphWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendGraphWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        BlendGraphWidget(AnimGraphPlugin* plugin, QWidget* parent);
        ~BlendGraphWidget();

        // overloaded
        bool CheckIfIsCreateConnectionValid(uint32 portNr, GraphNode* portNode, NodePort* port, bool isInputPort) override;
        bool CheckIfIsValidTransition(GraphNode* sourceState, GraphNode* targetState) override;
        bool CheckIfIsValidTransitionSource(GraphNode* sourceState) override;
        bool CreateConnectionMustBeCurved() override;
        bool CreateConnectionShowsHelpers() override;

        // callbacks
        void OnMoveStart() override;
        void OnMoveNode(GraphNode* node, int32 x, int32 y) override;
        void OnMoveEnd() override;
        void OnNodeCollapsed(GraphNode* node, bool isCollapsed) override;
        void OnShiftClickedNode(GraphNode* node) override;
        void OnVisualizeToggle(GraphNode* node, bool visualizeEnabled) override;
        void OnEnabledToggle(GraphNode* node, bool enabled) override;
        void OnSetupVisualizeOptions(GraphNode* node) override;
        void ReplaceTransition(NodeConnection* connection, QPoint oldStartOffset, QPoint oldEndOffset, GraphNode* oldSourceNode, GraphNode* oldTargetNode, GraphNode* newSourceNode, GraphNode* newTargetNode) override;

        void OnCreateConnection(uint32 sourcePortNr, GraphNode* sourceNode, bool sourceIsInputPort, uint32 targetPortNr, GraphNode* targetNode, bool targetIsInputPort, const QPoint& startOffset, const QPoint& endOffset) override;

        void DeleteSelectedItems(NodeGraph* nodeGraph);

        static bool OnEnterDropEvent(QDragEnterEvent* event, EMotionFX::AnimGraphNode* currentNode, NodeGraph* activeGraph);

        // checks if the currently shown graph is a state machine
        bool CheckIfIsStateMachine();

        // context menu shared function (definitions in ContextMenu.cpp)
        void AddNodeGroupSubmenu(QMenu* menu, EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNode*>& selectedNodes);
        void AddPreviewMotionSubmenu(QMenu* menu, AnimGraphActionManager* actionManager, const EMotionFX::AnimGraphNode* selectedNode);
        void AddAnimGraphObjectCategoryMenu(AnimGraphPlugin* plugin, QMenu* parentMenu,
            EMotionFX::AnimGraphObject::ECategory category, EMotionFX::AnimGraphObject* focusedGraphObject);

        void OnContextMenuEvent(QWidget* parentWidget, QPoint localMousePos, QPoint globalMousePos, AnimGraphPlugin* plugin,
            const AZStd::vector<EMotionFX::AnimGraphNode*>& selectedNodes, bool graphWidgetOnlyMenusEnabled, bool selectingAnyReferenceNodeFromNavigation,
            const AnimGraphActionFilter& actionFilter);

        void SetSelectedTransitionsEnabled(bool isEnabled);

        bool PreparePainting() override;

        void ProcessFrame(bool redraw);

        void SetVirtualFinalNode(const QModelIndex& nodeModelIndex);

    protected:
        void dropEvent(QDropEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragLeaveEvent(QDragLeaveEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;

        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        void OnContextMenuEvent(QPoint mousePos, QPoint globalMousePos, const AnimGraphActionFilter& actionFilter);

        bool event(QEvent* event) override;

    public slots:
        void DeleteSelectedItems();
        void OnContextMenuCreateNode();
        void OnNodeGroupSelected();
        void EnableSelectedTransitions()                    { SetSelectedTransitionsEnabled(true); }
        void DisableSelectedTransitions()                   { SetSelectedTransitionsEnabled(false); }

        void OnRowsInserted(const QModelIndex& parent, int first, int last);
        void OnRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
        void OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);
        void OnSelectionModelChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:
        void keyReleaseEvent(QKeyEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

        EMotionFX::AnimGraphStateTransition* FindTransitionForConnection(NodeConnection* connection) const;
        EMotionFX::BlendTreeConnection* FindBlendTreeConnection(NodeConnection* connection) const;

        // We are going to cache the NodeGraph that we have been focusing on
        // so we can swap them quickly.
        // TODO: investigate if we can avoid the caching
        // TODO: defer updates from graphs we are not showing
        using NodeGraphByModelIndex = AZStd::unordered_map<QPersistentModelIndex, AZStd::unique_ptr<NodeGraph>, QPersistentModelIndexHash>;
        NodeGraphByModelIndex m_nodeGraphByModelIndex;

        QPoint                      mContextMenuEventMousePos;
        bool                        mDoubleClickHappened;
        MCore::CommandGroup         mMoveGroup;
    };
}   // namespace EMStudio
