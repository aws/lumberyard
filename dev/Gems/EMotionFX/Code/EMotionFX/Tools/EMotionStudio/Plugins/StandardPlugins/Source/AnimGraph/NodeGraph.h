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

#ifndef __EMSTUDIO_NODEGRAPH_H
#define __EMSTUDIO_NODEGRAPH_H

#include <AzCore/Debug/Timer.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include <EMotionFX/Source/AnimGraphNodeId.h>
#include "../StandardPluginsConfig.h"
#include <QPainter>
#include <QTimer>


namespace EMStudio
{
    // forward declarations
    class GraphNode;
    class NodeConnection;
    class NodeGraphWidget;
    class NodePort;


    class NodeGraph
        : public QObject
    {
        MCORE_MEMORYOBJECTCATEGORY(NodeGraph, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        NodeGraph(NodeGraphWidget* graphWidget = nullptr);
        virtual ~NodeGraph();

        class NodeGroupsRenderCallback
        {
        public:
            virtual void RenderNodeGroups(QPainter& painter, NodeGraph* nodeGraph) = 0;
        };

        MCORE_INLINE MCore::Array<GraphNode*>& GetNodes()           { return mNodes; }
        MCORE_INLINE uint32 GetNumNodes() const                     { return mNodes.GetLength(); }
        MCORE_INLINE GraphNode* GetNode(uint32 index)               { return mNodes[index]; }
        MCORE_INLINE const QTransform& GetTransform() const         { return mTransform; }
        MCORE_INLINE float GetScale() const                         { return mScale; }
        MCORE_INLINE void SetScale(float scale)                     { mScale = scale; }
        MCORE_INLINE const QPoint& GetScrollOffset() const          { return mScrollOffset; }
        MCORE_INLINE const QPoint& GetScalePivot() const            { return mScalePivot; }
        MCORE_INLINE void SetScrollOffset(const QPoint& offset)     { mScrollOffset = offset; }
        MCORE_INLINE void SetScalePivot(const QPoint& pivot)        { mScalePivot = pivot; }
        MCORE_INLINE void SetGraphWidget(NodeGraphWidget* widget)   { mGraphWidget = widget; }
        MCORE_INLINE NodeGraphWidget* GetGraphWidget()              { return mGraphWidget; }
        MCORE_INLINE float GetLowestScale() const                   { return mLowestScale; }
        MCORE_INLINE bool GetIsCreatingConnection() const           { return (mConNode && mRelinkConnection == nullptr); }
        MCORE_INLINE bool GetIsRelinkingConnection() const          { return (mConNode && mRelinkConnection); }
        MCORE_INLINE void SetCreateConnectionIsValid(bool isValid)  { mConIsValid = isValid; }
        MCORE_INLINE bool GetIsCreateConnectionValid() const        { return mConIsValid; }
        MCORE_INLINE void SetTargetPort(NodePort* port)             { mTargetPort = port; }
        MCORE_INLINE NodePort* GetTargetPort()                      { return mTargetPort; }
        MCORE_INLINE float GetDashOffset() const                    { return mDashOffset; }
        MCORE_INLINE float GetErrorBlinkOffset() const              { return mErrorBlinkOffset; }
        MCORE_INLINE QColor GetErrorBlinkColor() const              { int32 red = 160 + ((0.5f + 0.5f * MCore::Math::Cos(mErrorBlinkOffset)) * 96); red = MCore::Clamp<int32>(red, 0, 255); return QColor(red, 0, 0); }

        bool GetIsRepositioningTransitionHead() const               { return (mReplaceTransitionHead); }
        bool GetIsRepositioningTransitionTail() const               { return (mReplaceTransitionTail); }
        NodeConnection* GetRepositionedTransitionHead() const       { return mReplaceTransitionHead; }
        NodeConnection* GetRepositionedTransitionTail() const       { return mReplaceTransitionTail; }

        void StartReplaceTransitionHead(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode);
        void StartReplaceTransitionTail(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode);
        void GetReplaceTransitionInfo(NodeConnection** outConnection, QPoint* outStartOffset, QPoint* outEndOffset, GraphNode** outSourceNode, GraphNode** outTargetNode);
        void StopReplaceTransitionHead();
        void StopReplaceTransitionTail();
        void SetReplaceTransitionValid(bool isValid)                        { mReplaceTransitionValid = isValid; }
        bool GetReplaceTransitionValid() const                              { return mReplaceTransitionValid; }
        void RenderReplaceTransition(QPainter& painter);

        MCORE_INLINE GraphNode* GetCreateConnectionNode()                   { return mConNode; }
        MCORE_INLINE NodeConnection* GetRelinkConnection()                  { return mRelinkConnection; }
        MCORE_INLINE NodePort* GetCreateConnectionPort()                    { return mConPort; }
        MCORE_INLINE uint32 GetCreateConnectionPortNr() const               { return mConPortNr; }
        MCORE_INLINE bool GetCreateConnectionIsInputPort() const            { return mConIsInputPort; }
        MCORE_INLINE const QPoint& GetCreateConnectionStartOffset() const   { return mConStartOffset; }
        MCORE_INLINE const QPoint& GetCreateConnectionEndOffset() const     { return mConEndOffset; }
        MCORE_INLINE void SetCreateConnectionEndOffset(const QPoint& offset){ mConEndOffset = offset; }

        NodeConnection* AddConnection(GraphNode* sourceNode, uint32 outputPortNr, GraphNode* targetNode, uint32 inputPortNr);
        bool CheckIfHasConnection(GraphNode* sourceNode, uint32 outputPortNr, GraphNode* targetNode, uint32 inputPortNr) const;
        NodeConnection* FindInputConnection(GraphNode* targetNode, uint32 targetPortNr) const;
        NodeConnection* FindFirstSelectedConnection() const;
        NodeConnection* FindConnection(const QPoint& mousePos);
        NodeConnection* FindConnectionByID(uint32 connectionID);
        GraphNode* FindFirstSelectedGraphNode() const;
        MCore::Array<GraphNode*> GetSelectedNodes() const;

        void AddNode(GraphNode* node);
        void RemoveNode(GraphNode* node, bool delFromMem = true);
        void RemoveAllNodes();
        void SelectAllNodes(bool selectConnectionsToo);
        void UnselectAllNodes(bool unselectConnectionsToo);

        uint32 CalcNumSelectedNodes() const;
        uint32 CalcNumSelectedConnections() const;
        uint32 CalcNumSelectedItems() const;    // nodes + connections

        GraphNode* FindNode(const QPoint& globalPoint);
        GraphNode* FindNodeById(EMotionFX::AnimGraphNodeId id);
        void StartCreateConnection(uint32 portNr, bool isInputPort, GraphNode* portNode, NodePort* port, const QPoint& startOffset);
        void StartRelinkConnection(NodeConnection* connection, uint32 portNr, GraphNode* node);
        void StopCreateConnection();
        void StopRelinkConnection();

        /*
         * Update highlight flags for all connections in the currently visible graph.
         * This is called when the selection or the graph changes and makes sure to highlight the connections that are connected to of from
         * the currently selected nodes, to easily spot them in spagetthi graphs.
         */
        void UpdateHighlightConnectionFlags(const QPoint& mousePos);

        virtual void RenderBackground(QPainter& painter, int32 width, int32 height);
        virtual void Render(QPainter& painter, int32 width, int32 height, const QPoint& mousePos, float timePassedInSeconds);
        virtual void RenderCreateConnection(QPainter& painter);
        void UpdateNodesAndConnections(int32 width, int32 height, const QPoint& mousePos);

        void RenderSubGrid(QPainter& painter, int32 startX, int32 startY);
        void RenderLinesSubGrid(QPainter& painter, int32 startX, int32 startY);
        void SelectNodesInRect(const QRect& rect, bool overwriteCurSelection = true, bool select = true, bool toggleMode = false);
        NodeConnection* SelectConnectionCloseTo(const QPoint& point, bool overwriteCurSelection = true, bool toggle = false);
        QRect CalcRectFromSelection(bool includeConnections = true) const;
        QRect CalcRectFromGraph() const;
        NodePort* FindPort(int32 x, int32 y, GraphNode** outNode, uint32* outPortNr, bool* outIsInputPort, bool includeInputPorts = true);

        void SetRenderNodeGroupsCallback(NodeGroupsRenderCallback* callback)    { mRenderNodeGroupsCallback = callback; }
        NodeGroupsRenderCallback* GetRenderNodeGroupsCallback()                 { return mRenderNodeGroupsCallback; }

        // entry state helper functions
        MCORE_INLINE GraphNode* GetEntryNode() const                                                { return mEntryNode; }
        MCORE_INLINE void SetEntryNode(GraphNode* entryNode)                                        { mEntryNode = entryNode; }
        static void RenderEntryPoint(QPainter& painter, GraphNode* node);

        void FitGraphOnScreen(int32 width, int32 height, const QPoint& mousePos, bool autoUpdate = true, bool animate = true);
        void ScrollTo(const QPointF& point);
        void ZoomIn();
        void ZoomOut();
        void ZoomTo(float scale);
        void ZoomOnRect(const QRect& rect, int32 width, int32 height, bool animate = true);
        void StopAnimatedZoom();
        void StopAnimatedScroll();

        // helpers
        static void DrawSmoothedLine(QPainter& painter, int32 x1, int32 y1, int32 x2, int32 y2, int32 stepSize, const QRect& visibleRect);
        static void DrawSmoothedLineFast(QPainter& painter, int32 x1, int32 y1, int32 x2, int32 y2, int32 stepSize);
        static float DistanceToLine(float x1, float y1, float x2, float y2, float px, float py);
        static bool LinesIntersect(double Ax, double Ay, double Bx, double By,  double Cx, double Cy, double Dx, double Dy, double* X, double* Y);
        static bool LineIntersectsRect(const QRect& b, float x1, float y1, float x2, float y2, double* outX = nullptr, double* outY = nullptr);
        static bool PointCloseToSmoothedLine(int32 x1, int32 y1, int32 x2, int32 y2, int32 px, int32 py);
        static bool RectIntersectsSmoothedLine(const QRect& rect, int32 x1, int32 y1, int32 x2, int32 y2);

        bool GetStartFitHappened() const                                { return mStartFitHappened; }
        MCORE_INLINE bool GetUseAnimation() const                       { return mUseAnimation; }
        MCORE_INLINE void SetUseAnimation(bool useAnim)                 { mUseAnimation = useAnim; }

    protected:
        NodeGroupsRenderCallback*   mRenderNodeGroupsCallback;
        NodeGraphWidget*            mGraphWidget;
        MCore::Array<GraphNode*>    mNodes;
        GraphNode*                  mEntryNode;
        QTransform                  mTransform;
        float                       mScale;
        float                       mLowestScale;
        float                       mSlowRenderTime;
        float                       mFastRenderTime;
        int32                       mMinStepSize;
        int32                       mMaxStepSize;
        QPoint                      mScrollOffset;
        QPoint                      mScalePivot;

        QPointF                     mTargetScrollOffset;
        QPointF                     mStartScrollOffset;
        QTimer                      mScrollTimer;
        AZ::Debug::Timer            mScrollPreciseTimer;

        float                       mTargetScale;
        float                       mStartScale;
        QTimer                      mScaleTimer;
        AZ::Debug::Timer            mScalePreciseTimer;

        // connection info
        QPoint                      mConStartOffset;
        QPoint                      mConEndOffset;
        uint32                      mConPortNr;
        bool                        mConIsInputPort;
        GraphNode*                  mConNode;       // nullptr when no connection is being created
        NodeConnection*             mRelinkConnection; // nullptr when not relinking a connection
        NodePort*                   mConPort;
        NodePort*                   mTargetPort;
        bool                        mConIsValid;
        bool                        mStartFitHappened;
        float                       mDashOffset;
        float                       mErrorBlinkOffset;
        bool                        mUseAnimation;

        NodeConnection*             mReplaceTransitionHead; // nullptr when not replacing a transition head
        NodeConnection*             mReplaceTransitionTail; // nullptr when not replacing a transition tail
        QPoint                      mReplaceTransitionStartOffset;
        QPoint                      mReplaceTransitionEndOffset;
        GraphNode*                  mReplaceTransitionSourceNode;
        GraphNode*                  mReplaceTransitionTargetNode;
        bool                        mReplaceTransitionValid;

        QPen                        mSubgridPen;
        QPen                        mGridPen;

    protected slots:
        void UpdateAnimatedScrollOffset();
        void UpdateAnimatedScale();
    };
}   // namespace EMStudio

#endif
