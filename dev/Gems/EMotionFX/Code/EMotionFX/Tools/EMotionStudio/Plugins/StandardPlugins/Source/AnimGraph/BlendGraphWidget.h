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

#ifndef __EMSTUDIO_BLENDGRAPHWIDGET_H
#define __EMSTUDIO_BLENDGRAPHWIDGET_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include "NodeGraphWidget.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MysticQt/Source/LinkWidget.h>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;

    //
    class BlendGraphWidget
        : public NodeGraphWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendGraphWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        BlendGraphWidget(AnimGraphPlugin* plugin, QWidget* parent);
        ~BlendGraphWidget();

        void ShowNothing();

        void SetCurrentNode(EMotionFX::AnimGraphNode* node);
        MCORE_INLINE EMotionFX::AnimGraphNode* GetCurrentNode()                    { return mCurrentNode; }
        MCORE_INLINE AnimGraphPlugin* GetPlugin()                                  { return mPlugin; }

        EMotionFX::AnimGraphStateTransition* FindTransitionForConnection(NodeConnection* connection);
        EMotionFX::BlendTreeConnection* FindBlendTreeConnection(NodeConnection* connection) const;
        EMotionFX::AnimGraphNode* FindFirstSelectedAnimGraphNode();

        // overloaded
        bool CheckIfIsCreateConnectionValid(uint32 portNr, GraphNode* portNode, NodePort* port, bool isInputPort) override;
        static bool CheckIfIsRelinkConnectionValid(NodeConnection* connection, GraphNode* newTargetNode, uint32 newTargetPortNr, bool isTargetInput);
        bool CheckIfIsValidTransition(GraphNode* sourceState, GraphNode* targetState) override;
        bool CheckIfIsValidTransitionSource(GraphNode* sourceState) override;
        bool CreateConnectionMustBeCurved() override;
        bool CreateConnectionShowsHelpers() override;

        // callbacks
        void OnDrawOverlay(QPainter& painter) override;
        void OnMoveStart() override;
        void OnMoveNode(GraphNode* node, int32 x, int32 y) override;
        void OnMoveEnd() override;
        void OnNodeCollapsed(GraphNode* node, bool isCollapsed) override;
        void OnShiftClickedNode(GraphNode* node) override;
        void OnVisualizeToggle(GraphNode* node, bool visualizeEnabled) override;
        void OnEnabledToggle(GraphNode* node, bool enabled) override;
        void OnSetupVisualizeOptions(GraphNode* node) override;
        void ReplaceTransition(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode) override;

        void OnSelectionChanged() override;
        void OnCreateConnection(uint32 sourcePortNr, GraphNode* sourceNode, bool sourceIsInputPort, uint32 targetPortNr, GraphNode* targetNode, bool targetIsInputPort, const QPoint& startOffset, const QPoint& endOffset) override;

        void DeleteSelectedItems(NodeGraph* nodeGraph);

        static bool OnEnterDropEvent(QDragEnterEvent* event, EMotionFX::AnimGraphNode* currentNode, NodeGraph* activeGraph);

        // checks if the currently shown graph is a state machine
        bool CheckIfIsStateMachine();

        QPoint GetContextMenuEventMousePos()                            { return mContextMenuEventMousePos; }

        // context menu shared function
        void AddNodeGroupSubmenu(QMenu* menu, EMotionFX::AnimGraph* animGraph, const MCore::Array<EMotionFX::AnimGraphNode*>& selectedNodes);
        void RegisterItems(AnimGraphPlugin* plugin, QMenu* menu, EMotionFX::AnimGraphObject* object, EMotionFX::AnimGraphObject::ECategory category);
        void OnContextMenuEvent(QWidget* parentWidget, QPoint localMousePos, QPoint globalMousePos, AnimGraphPlugin* plugin, const MCore::Array<EMotionFX::AnimGraphNode*> selectedNodes, bool graphWidgetOnlyMenusEnabled);
        void SetSelectedTransitionsEnabled(bool isEnabled);

        // gather all selected connections/transitions of the currently shown graph and put them into the given array, the array will be cleared upfront
        void CollectSelectedConnections(MCore::Array<NodeConnection*>* outConnections);

        bool PreparePainting() override;

    protected:
        void dropEvent(QDropEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragLeaveEvent(QDragLeaveEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;

        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        //void contextMenuEvent(QContextMenuEvent* event);

        void OnContextMenuEvent(QPoint mousePos, QPoint globalMousePos);

        //      void paintEvent(QPaintEvent* event);
        bool event(QEvent* event) override;

    public slots:
        void OnMouseClickTimeout();
        void DeleteSelectedItems();
        void OnContextMenuCreateNode();
        void OnNodeGroupSelected();
        void EnableSelectedTransitions()                    { SetSelectedTransitionsEnabled(true); }
        void DisableSelectedTransitions()                   { SetSelectedTransitionsEnabled(false); }

    private:
        void keyReleaseEvent(QKeyEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        QPoint                      mContextMenuEventMousePos;
        //QTimer*                   mMouseClickTimer;
        bool                        mLastRightClick;
        bool                        mDoubleClickHappened;
        QPoint                      mLastGlobalMousePos;
        EMotionFX::AnimGraphNode*   mCurrentNode;
        MCore::CommandGroup         mMoveGroup;
        AZStd::string               mMoveString;
        AZStd::string               mTextString;
        QFont                       mOverlayFont;
    };
}   // namespace EMStudio


#endif

