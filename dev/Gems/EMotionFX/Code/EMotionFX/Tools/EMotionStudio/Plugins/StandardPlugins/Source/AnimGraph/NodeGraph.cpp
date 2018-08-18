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

// include the required headers
#include "NodeGraph.h"
#include "GraphNode.h"
#include "NodeConnection.h"
#include "BlendGraphWidget.h"
#include "NodeGraphWidget.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/Vector.h>


namespace EMStudio
{
    // constructor
    NodeGraph::NodeGraph(NodeGraphWidget* graphWidget)
        : QObject()
    {
        mNodes.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        mErrorBlinkOffset = 0.0f;
        mUseAnimation   = true;
        mDashOffset     = 0.0f;
        mScale          = 1.0f;
        mScrollOffset   = QPoint(0.0f, 0.0f);
        mScalePivot     = QPoint(0.0f, 0.0f);
        mGraphWidget    = graphWidget;
        mLowestScale    = 0.15f;
        mMinStepSize    = 1;
        mMaxStepSize    = 75;
        mSlowRenderTime = 0.0f;
        mFastRenderTime = 0.0f;
        mEntryNode      = nullptr;
        mRenderNodeGroupsCallback = nullptr;
        mStartFitHappened = false;

        // init connection creation
        mConPortNr          = MCORE_INVALIDINDEX32;
        mConIsInputPort     = true;
        mConNode            = nullptr;  // nullptr when no connection is being created
        mConPort            = nullptr;
        mConIsValid         = false;
        mTargetPort         = nullptr;
        mRelinkConnection   = nullptr;
        mReplaceTransitionHead = nullptr;
        mReplaceTransitionTail = nullptr;
        mReplaceTransitionSourceNode = nullptr;
        mReplaceTransitionTargetNode = nullptr;

        // setup scroll interpolator
        mStartScrollOffset          = QPointF(0.0f, 0.0f);
        mTargetScrollOffset         = QPointF(0.0f, 0.0f);
        mScrollTimer.setSingleShot(false);
        connect(&mScrollTimer, SIGNAL(timeout()), this, SLOT(UpdateAnimatedScrollOffset()));

        // setup scale interpolator
        mStartScale                 = 1.0f;
        mTargetScale                = 1.0f;
        mScaleTimer.setSingleShot(false);
        connect(&mScaleTimer, SIGNAL(timeout()), this, SLOT(UpdateAnimatedScale()));
    }


    // destructor
    NodeGraph::~NodeGraph()
    {
        RemoveAllNodes();
    }


    // add a node to the graph
    void NodeGraph::AddNode(GraphNode* node)
    {
    #ifdef MCORE_DEBUG
        // check if the node with the given node id already is part of the graph
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (mNodes[i]->GetId() == node->GetId())
            {
                // this should never happen!
                MCORE_ASSERT(false);
                delete node;
                //MCore::LogInfo( "Node '%s' has already been added the graph", node->GetName() );
                return;
            }
        }
    #endif

        //MCore::LogInfo( "#### Adding node '%s' to graph '%s'", node->GetName(), "???" );
        mNodes.Add(node);
        node->SetParentGraph(this);
    }


    // remove all nodes
    void NodeGraph::RemoveAllNodes()
    {
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            delete mNodes[i];
        }

        mNodes.Clear();
    }


    // check if a rect intersects this smoothed line
    bool NodeGraph::RectIntersectsSmoothedLine(const QRect& rect, int32 x1, int32 y1, int32 x2, int32 y2)
    {
        if (x2 >= x1)
        {
            // find the min and max points
            int32 minX, maxX, startY, endY;
            if (x1 <= x2)
            {
                minX    = x1;
                maxX    = x2;
                startY  = y1;
                endY    = y2;
            }
            else
            {
                minX    = x2;
                maxX    = x1;
                startY  = y2;
                endY    = y1;
            }

            // draw the lines
            int32 lastX = minX;
            int32 lastY = startY;

            if (minX != maxX)
            {
                for (int32 x = minX; x <= maxX; x++)
                {
                    const float t = MCore::CalcCosineInterpolationWeight((x - minX) / (float)(maxX - minX)); // calculate the smooth interpolated value
                    const int32 y = startY + (endY - startY) * t; // calculate the y coordinate
                    if (LineIntersectsRect(rect, lastX, lastY, x, y))
                    {
                        return true;
                    }
                    lastX = x;
                    lastY = y;
                }
            }
            else // special case where there is just one line up
            {
                return LineIntersectsRect(rect, x1, y1, x2, y2);
            }
        }
        else
        {
            // find the min and max points
            int32 minY, maxY, startX, endX;
            if (y1 <= y2)
            {
                minY    = y1;
                maxY    = y2;
                startX  = x1;
                endX    = x2;
            }
            else
            {
                minY    = y2;
                maxY    = y1;
                startX  = x2;
                endX    = x1;
            }

            // draw the lines
            int32 lastY = minY;
            int32 lastX = startX;

            if (minY != maxY)
            {
                for (int32 y = minY; y <= maxY; y++)
                {
                    const float t = MCore::CalcCosineInterpolationWeight((y - minY) / (float)(maxY - minY)); // calculate the smooth interpolated value
                    const int32 x = startX + (endX - startX) * t; // calculate the y coordinate
                    if (LineIntersectsRect(rect, lastX, lastY, x, y))
                    {
                        return true;
                    }
                    lastX = x;
                    lastY = y;
                }
            }
            else // special case where there is just one line up
            {
                return LineIntersectsRect(rect, x1, y1, x2, y2);
            }
        }

        return false;
    }


    // check if a point is close to a given smoothed line
    bool NodeGraph::PointCloseToSmoothedLine(int32 x1, int32 y1, int32 x2, int32 y2, int32 px, int32 py)
    {
        if (x2 >= x1)
        {
            // find the min and max points
            int32 minX, maxX, startY, endY;
            if (x1 <= x2)
            {
                minX    = x1;
                maxX    = x2;
                startY  = y1;
                endY    = y2;
            }
            else
            {
                minX    = x2;
                maxX    = x1;
                startY  = y2;
                endY    = y1;
            }

            // draw the lines
            int32 lastX = minX;
            int32 lastY = startY;

            if (minX != maxX)
            {
                for (int32 x = minX; x <= maxX; x++)
                {
                    const float t = MCore::CalcCosineInterpolationWeight((x - minX) / (float)(maxX - minX)); // calculate the smooth interpolated value
                    const int32 y = startY + (endY - startY) * t; // calculate the y coordinate
                    if (DistanceToLine(lastX, lastY, x, y, px, py) <= 5.0f)
                    {
                        return true;
                    }
                    lastX = x;
                    lastY = y;
                }
            }
            else // special case where there is just one line up
            {
                return (DistanceToLine(x1, y1, x2, y2, px, py) <= 5.0f);
            }
        }
        else
        {
            // find the min and max points
            int32 minY, maxY, startX, endX;
            if (y1 <= y2)
            {
                minY    = y1;
                maxY    = y2;
                startX  = x1;
                endX    = x2;
            }
            else
            {
                minY    = y2;
                maxY    = y1;
                startX  = x2;
                endX    = x1;
            }

            // draw the lines
            int32 lastY = minY;
            int32 lastX = startX;

            if (minY != maxY)
            {
                for (int32 y = minY; y <= maxY; y++)
                {
                    const float t = MCore::CalcCosineInterpolationWeight((y - minY) / (float)(maxY - minY)); // calculate the smooth interpolated value
                    const int32 x = startX + (endX - startX) * t; // calculate the y coordinate
                    if (DistanceToLine(lastX, lastY, x, y, px, py) <= 5.0f)
                    {
                        return true;
                    }
                    lastX = x;
                    lastY = y;
                }
            }
            else // special case where there is just one line up
            {
                return (DistanceToLine(x1, y1, x2, y2, px, py) <= 5.0f);
            }
        }

        return false;
    }


    void NodeGraph::RenderEntryPoint(QPainter& painter, GraphNode* node)
    {
        if (node == nullptr)
        {
            return;
        }

        QPen oldPen = painter.pen();
        QColor color(150, 150, 150);
        QPen newPen(color);
        newPen.setWidth(3);
        painter.setBrush(color);
        painter.setPen(color);

        int32   arrowLength     = 30;
        int32   circleSize      = 4;
        QRect   rect            = node->GetRect();
        QPoint  start           = rect.topLeft() + QPoint(-arrowLength, 0) + QPoint(0, rect.height() / 2);
        QPoint  end             = rect.topLeft() + QPoint(0, rect.height() / 2);


        // calculate the line direction
        AZ::Vector2 lineDir = AZ::Vector2(end.x(), end.y()) - AZ::Vector2(start.x(), start.y());
        float length = lineDir.GetLength();
        lineDir.Normalize();

        // draw the arrow
        QPointF direction;
        direction.setX(lineDir.GetX() * 10.0f);
        direction.setY(lineDir.GetY() * 10.0f);

        QPointF normalOffset((end.y() - start.y()) / length, (start.x() - end.x()) / length);

        QPointF points[3];
        points[0] = end;
        points[1] = end - direction + (normalOffset * 6.7f);
        points[2] = end - direction - (normalOffset * 6.7f);

        painter.drawPolygon(points, 3);

        // draw the end circle
        painter.drawEllipse(start, circleSize, circleSize);

        // draw the arror line
        painter.setPen(newPen);
        painter.drawLine(start, end + QPoint(-5, 0));

        painter.setPen(oldPen);
    }


    // draw a smoothed line
    void NodeGraph::DrawSmoothedLine(QPainter& painter, int32 x1, int32 y1, int32 x2, int32 y2, int32 stepSize, const QRect& visibleRect)
    {
        if (x2 >= x1)
        {
            // find the min and max points
            int32 minX, maxX, startY, endY;
            if (x1 <= x2)
            {
                minX    = x1;
                maxX    = x2;
                startY  = y1;
                endY    = y2;
            }
            else
            {
                minX    = x2;
                maxX    = x1;
                startY  = y2;
                endY    = y1;
            }

            // draw the lines
            int32 lastX = minX;
            int32 lastY = startY;

            int32 realStepSize = stepSize;

            if (minX != maxX)
            {
                bool lastInside = true;
                int32 x = minX;
                while (x < maxX)
                {
                    const float t = MCore::CalcCosineInterpolationWeight((x - minX) / (float)(maxX - minX)); // calculate the smooth interpolated value
                    const int32 y = startY + (endY - startY) * t; // calculate the y coordinate

                    if (NodeGraph::LineIntersectsRect(visibleRect, lastX, lastY, x, y))
                    {
                        painter.drawLine(lastX, lastY, x, y);       // draw the line
                        lastInside = true;
                    }
                    else
                    {
                        lastInside = false;
                    }

                    if (lastInside == false && realStepSize < 40)
                    {
                        realStepSize = 40;
                    }
                    else
                    {
                        realStepSize = stepSize;
                    }

                    lastX = x;
                    lastY = y;
                    x += realStepSize;
                }

                const float t = MCore::CalcCosineInterpolationWeight(1.0f); // calculate the smooth interpolated value
                const int32 y = startY + (endY - startY) * t; // calculate the y coordinate
                painter.drawLine(lastX, lastY, maxX, y);        // draw the line
            }
            else // special case where there is just one line up
            {
                painter.drawLine(x1, y1, x2, y2);
            }
        }
        else
        {
            // find the min and max points
            int32 minY, maxY, startX, endX;
            if (y1 <= y2)
            {
                minY    = y1;
                maxY    = y2;
                startX  = x1;
                endX    = x2;
            }
            else
            {
                minY    = y2;
                maxY    = y1;
                startX  = x2;
                endX    = x1;
            }

            // draw the lines
            int32 lastY = minY;
            int32 lastX = startX;
            int32 realStepSize = stepSize;

            if (minY != maxY)
            {
                int32 y = minY;
                bool lastInside = true;
                while (y < maxY)
                {
                    const float t = MCore::CalcCosineInterpolationWeight((y - minY) / (float)(maxY - minY)); // calculate the smooth interpolated value
                    const int32 x = startX + (endX - startX) * t; // calculate the y coordinate

                    if (NodeGraph::LineIntersectsRect(visibleRect, lastX, lastY, x, y))
                    {
                        painter.drawLine(lastX, lastY, x, y);       // draw the line
                        lastInside = true;
                    }
                    else
                    {
                        lastInside = false;
                    }

                    lastX = x;
                    lastY = y;

                    if (lastInside == false && realStepSize < 40)
                    {
                        realStepSize = 40;
                    }
                    else
                    {
                        realStepSize = stepSize;
                    }

                    y += realStepSize;
                }

                const float t = MCore::CalcCosineInterpolationWeight(1.0f); // calculate the smooth interpolated value
                const int32 x = startX + (endX - startX) * t; // calculate the y coordinate
                painter.drawLine(lastX, lastY, x, maxY);        // draw the line
            }
            else // special case where there is just one line up
            {
                painter.drawLine(x1, y1, x2, y2);
            }
        }
    }



    // draw a smoothed line
    void NodeGraph::DrawSmoothedLineFast(QPainter& painter, int32 x1, int32 y1, int32 x2, int32 y2, int32 stepSize)
    {
        if (x2 >= x1)
        {
            // find the min and max points
            int32 minX, maxX, startY, endY;
            if (x1 <= x2)
            {
                minX    = x1;
                maxX    = x2;
                startY  = y1;
                endY    = y2;
            }
            else
            {
                minX    = x2;
                maxX    = x1;
                startY  = y2;
                endY    = y1;
            }

            // draw the lines
            int32 lastX = minX;
            int32 lastY = startY;

            if (minX != maxX)
            {
                int32 x = minX;
                while (x < maxX)
                {
                    const float t = MCore::CalcCosineInterpolationWeight((x - minX) / (float)(maxX - minX)); // calculate the smooth interpolated value
                    const int32 y = startY + (endY - startY) * t; // calculate the y coordinate
                    painter.drawLine(lastX, lastY, x, y);       // draw the line
                    lastX = x;
                    lastY = y;
                    x += stepSize;
                }

                const float t = MCore::CalcCosineInterpolationWeight(1.0f); // calculate the smooth interpolated value
                const int32 y = startY + (endY - startY) * t; // calculate the y coordinate
                painter.drawLine(lastX, lastY, maxX, y);        // draw the line
            }
            else // special case where there is just one line up
            {
                painter.drawLine(x1, y1, x2, y2);
            }
        }
        else
        {
            // find the min and max points
            int32 minY, maxY, startX, endX;
            if (y1 <= y2)
            {
                minY    = y1;
                maxY    = y2;
                startX  = x1;
                endX    = x2;
            }
            else
            {
                minY    = y2;
                maxY    = y1;
                startX  = x2;
                endX    = x1;
            }

            // draw the lines
            int32 lastY = minY;
            int32 lastX = startX;

            if (minY != maxY)
            {
                int32 y = minY;
                while (y < maxY)
                {
                    const float t = MCore::CalcCosineInterpolationWeight((y - minY) / (float)(maxY - minY)); // calculate the smooth interpolated value
                    const int32 x = startX + (endX - startX) * t; // calculate the y coordinate
                    painter.drawLine(lastX, lastY, x, y);       // draw the line
                    lastX = x;
                    lastY = y;
                    y += stepSize;
                }

                const float t = MCore::CalcCosineInterpolationWeight(1.0f); // calculate the smooth interpolated value
                const int32 x = startX + (endX - startX) * t; // calculate the y coordinate
                painter.drawLine(lastX, lastY, x, maxY);        // draw the line
            }
            else // special case where there is just one line up
            {
                painter.drawLine(x1, y1, x2, y2);
            }
        }
    }


    void NodeGraph::UpdateNodesAndConnections(int32 width, int32 height, const QPoint& mousePos)
    {
        uint32 i;

        // calculate the visible rect
        QRect visibleRect;
        visibleRect = QRect(0, 0, width, height);

        // update the nodes
        const uint32 numNodes = mNodes.GetLength();
        for (i = 0; i < numNodes; ++i)
        {
            mNodes[i]->Update(visibleRect, mousePos);
        }

        // update the connections
        for (i = 0; i < numNodes; ++i)
        {
            GraphNode* node = mNodes[i];
            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                node->GetConnection(c)->Update(visibleRect, mousePos);
            }
        }
    }


    // find the connection at the given mouse position
    NodeConnection* NodeGraph::FindConnection(const QPoint& mousePos)
    {
        // for all nodes in the graph
        const uint32 numNodes = GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* graphNode = GetNode(i);

            // iterate over all connections
            const uint32 numConnections = graphNode->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection  = graphNode->GetConnection(c);
                if (connection->CheckIfIsCloseTo(mousePos))
                {
                    return connection;
                }
            }
        }

        // failure, there is no connection at the given mouse position
        return nullptr;
    }


    NodeConnection* NodeGraph::FindConnectionByID(uint32 connectionID)
    {
        // for all nodes in the graph
        const uint32 numNodes = GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* graphNode = GetNode(i);

            // iterate over all connections
            const uint32 numConnections = graphNode->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection  = graphNode->GetConnection(c);
                if (connection->GetID() == connectionID)
                {
                    return connection;
                }
            }
        }

        // failure, there is no connection at the given mouse position
        return nullptr;
    }


    void NodeGraph::UpdateHighlightConnectionFlags(const QPoint& mousePos)
    {
        // get the node graph
        EMStudio::NodeGraph* graph = mGraphWidget->GetActiveGraph();
        if (graph == nullptr)
        {
            return;
        }

        bool highlightedConnectionFound = false;

        // for all nodes in the graph
        const uint32 numNodes = graph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* graphNode = graph->GetNode(i);

            // iterate over all connections
            const uint32 numConnections = graphNode->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection  = graphNode->GetConnection(c);
                GraphNode*      sourceNode  = connection->GetSourceNode();
                GraphNode*      targetNode  = connection->GetTargetNode();

                // set the highlight flag
                // Note: connections get reset in the Connection::Update() method already
                if (highlightedConnectionFound == false && connection->CheckIfIsCloseTo(mousePos))
                {
                    highlightedConnectionFound = true;
                    connection->SetIsHighlighted(true);

                    connection->SetIsHeadHighlighted(connection->CheckIfIsCloseToHead(mousePos));
                    connection->SetIsTailHighlighted(connection->CheckIfIsCloseToTail(mousePos));
                }
                else
                {
                    connection->SetIsHeadHighlighted(false);
                    connection->SetIsTailHighlighted(false);
                }

                if (mReplaceTransitionHead == connection)
                {
                    connection->SetIsHeadHighlighted(true);
                }

                if (mReplaceTransitionTail == connection)
                {
                    connection->SetIsTailHighlighted(true);
                }

                // enable higlighting if either the source or the target node is selected
                if (sourceNode && sourceNode->GetIsSelected())
                {
                    connection->SetIsConnectedHighlighted(true);
                }

                if (targetNode->GetIsSelected())
                {
                    connection->SetIsConnectedHighlighted(true);
                }

                // or in case the source or target node are highlighted
                if (targetNode->GetIsHighlighted() || (sourceNode && sourceNode->GetIsHighlighted()))
                {
                    connection->SetIsHighlighted(true);
                }
            }
        }
    }


    //#define GRAPH_PERFORMANCE_INFO
    //#define GRAPH_PERFORMANCE_FRAMEDURATION

    // render the graph
    void NodeGraph::Render(QPainter& painter, int32 width, int32 height, const QPoint& mousePos, float timePassedInSeconds)
    {
        uint32 i;

        // control the scroll speed of the dashed blend tree connections etc
        mDashOffset -= 7.5f * timePassedInSeconds;
        mErrorBlinkOffset += 5.0f * timePassedInSeconds;

#ifdef GRAPH_PERFORMANCE_FRAMEDURATION
        MCore::Timer timer;
#endif

        //  painter.setRenderHint( QPainter::HighQualityAntialiasing );
        //  painter.setRenderHint( QPainter::TextAntialiasing );

        // calculate the visible rect
        QRect visibleRect;
        visibleRect = QRect(0, 0, width, height);
        //visibleRect.adjust(50, 50, -50, -50);

        // setup the transform
        mTransform.reset();
        mTransform.translate(mScalePivot.x(), mScalePivot.y());
        mTransform.scale(mScale, mScale);
        mTransform.translate(-mScalePivot.x() + mScrollOffset.x(), -mScalePivot.y() + mScrollOffset.y());
        painter.setTransform(mTransform);

        // render the background
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer backgroundTimer;
        backgroundTimer.GetTimeDelta();
#endif
        RenderBackground(painter, width, height);
#ifdef GRAPH_PERFORMANCE_INFO
        const float backgroundTime = backgroundTimer.GetTimeDelta();
        MCore::LogInfo("   Background: %.2f ms", backgroundTime * 1000);
#endif

        // update the nodes
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer updateTimer;
#endif
        UpdateNodesAndConnections(width, height, mousePos);
        UpdateHighlightConnectionFlags(mousePos); // has to come after nodes and connections are updated
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::LogInfo("   Update: %.2f ms", updateTimer.GetTime() * 1000);
#endif

        // render the node groups
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer groupsTimer;
#endif
        if (mRenderNodeGroupsCallback)
        {
            mRenderNodeGroupsCallback->RenderNodeGroups(painter, this);
        }
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::LogInfo("   Groups: %.2f ms", groupsTimer.GetTime() * 1000);
#endif

        // calculate the connection stepsize
        // the higher the value, the less lines it renders (so faster)
        int32 stepSize = ((1.0f / (mScale * (mScale * 1.75f))) * 10) - 7;
        stepSize = MCore::Clamp<int32>(stepSize, mMinStepSize, mMaxStepSize);

        QRect scaledVisibleRect = mTransform.inverted().mapRect(visibleRect);

        bool renderShadow = false;
        if (GetScale() >= 0.3f)
        {
            renderShadow = true;
        }

        // render connections
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer connectionsTimer;
#endif
        QPen connectionsPen;
        QBrush connectionsBrush;
        const uint32 numNodes = mNodes.GetLength();
        for (i = 0; i < numNodes; ++i)
        {
            GraphNode::RenderConnections(mNodes[i], painter, &connectionsPen, &connectionsBrush, scaledVisibleRect, stepSize);
        }
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::LogInfo("   Connections: %.2f ms", connectionsTimer.GetTime() * 1000);
#endif

        // render all nodes
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer nodesTimer;
#endif
        QPen nodesPen;
        for (i = 0; i < numNodes; ++i)
        {
            mNodes[i]->Render(painter, &nodesPen, renderShadow);
        }
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::LogInfo("   Nodes: %.2f ms", nodesTimer.GetTime() * 1000);
#endif

        // render the connection we are creating, if any
        RenderCreateConnection(painter);

        RenderReplaceTransition(painter);

        // render the entry state arrow
        RenderEntryPoint(painter, mEntryNode);

        //----------
        /*
            // adapt the connection subdivision detail based on how fast or slow things are running
            float renderTime = timer.GetTimeDelta();
            if (renderTime > 1.0f / 60.0f)
            {
                mSlowRenderTime += renderTime;
                mFastRenderTime = 0.0f;
            }
            else
            {
                mFastRenderTime += renderTime;
                mSlowRenderTime = 0.0f;
            }

            // if we're rendering slowly already for over a given amount of seconds
            if (mSlowRenderTime > 1.0f)
            {
                mMinStepSize += 2;  // increase the min step size
                if (mMinStepSize > 100)
                    mMinStepSize = 100;

                mMaxStepSize += 2;
                if (mMaxStepSize > 100)
                    mMaxStepSize = 100;
            }
            else
            {
                if (mFastRenderTime > 1.0f)
                {
                    mMinStepSize -= 2;
                    if (mMinStepSize < 1)
                        mMinStepSize = 1;

                    mMaxStepSize -= 2;
                    if (mMaxStepSize < mMinStepSize)
                        mMaxStepSize = mMinStepSize;
                }
            }
        */
#ifdef GRAPH_PERFORMANCE_FRAMEDURATION
        MCore::LogInfo("GraphRenderingTime: %.2f ms.", timer.GetTime() * 1000);
#endif

        // render FPS counter
        //#ifdef GRAPH_PERFORMANCE_FRAMEDURATION
        /*  static MCore::AnsiString tempFPSString;
            static MCore::Timer fpsTimer;
            static double fpsTimeElapsed = 0.0;
            static uint32 fpsNumFrames = 0;
            static uint32 lastFPS = 0;
            fpsTimeElapsed += fpsTimer.GetTimeDelta();
            fpsNumFrames++;
            if (fpsTimeElapsed > 1.0f)
            {
                lastFPS         = fpsNumFrames;
                fpsTimeElapsed  = 0.0;
                fpsNumFrames    = 0;
            }
            tempFPSString.Format( "%i FPS", lastFPS );
            painter.setPen( QColor(255, 255, 255) );
            painter.resetTransform();
            painter.drawText( 5, 20, tempFPSString.c_str() );
            */
        //#endif
    }


    // select all nodes within a given rect
    void NodeGraph::SelectNodesInRect(const QRect& rect, bool overwriteCurSelection, bool select, bool toggleMode)
    {
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* node = mNodes[i];
            if (node->GetRect().intersects(rect))
            {
                if (toggleMode)
                {
                    node->SetIsSelected(!node->GetIsSelected());
                }
                else
                {
                    node->SetIsSelected(select);
                }
            }
            else
            {
                if (overwriteCurSelection)
                {
                    node->SetIsSelected(false);
                }
            }

            // check the connections of this node
            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection = node->GetConnection(c);
                if (connection->Intersects(rect))
                {
                    if (toggleMode)
                    {
                        connection->SetIsSelected(!connection->GetIsSelected());
                    }
                    else
                    {
                        connection->SetIsSelected(select);
                    }
                }
                else
                {
                    if (overwriteCurSelection)
                    {
                        connection->SetIsSelected(false);
                    }
                }
            }
        }

        // trigger the selection change callback
        mGraphWidget->OnSelectionChanged();
    }


    // select all nodes
    void NodeGraph::SelectAllNodes(bool selectConnectionsToo)
    {
        // compute the actual selected nodes
        const uint32 numSelectedBefore = CalcNumSelectedNodes();

        // select all nodes and connections
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the node
            GraphNode* node = mNodes[i];

            // set the node selected
            node->SetIsSelected(true);

            // set each connection selected if needed
            if (selectConnectionsToo)
            {
                const uint32 numConnections = node->GetNumConnections();
                for (uint32 c = 0; c < numConnections; ++c)
                {
                    NodeConnection* connection = node->GetConnection(c);
                    connection->SetIsSelected(true);
                }
            }
        }

        // trigger the selection change callback
        if (numSelectedBefore != CalcNumSelectedNodes())
        {
            mGraphWidget->OnSelectionChanged();
        }
    }


    // find the node
    GraphNode* NodeGraph::FindNode(const QPoint& globalPoint)
    {
        // for all nodes
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // check if the point is inside the node rect
            if (mNodes[i]->GetIsInside(globalPoint))
            {
                return mNodes[i];
            }
        }

        // not found
        return nullptr;
    }


    // unselect all nodes
    void NodeGraph::UnselectAllNodes(bool unselectConnectionsToo)
    {
        // compute the actual selected nodes
        const uint32 numBefore = CalcNumSelectedNodes();

        // unselect all nodes and connection if needed
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mNodes[i]->SetIsSelected(false);

            if (unselectConnectionsToo)
            {
                const uint32 numConnections = mNodes[i]->GetNumConnections();
                for (uint32 c = 0; c < numConnections; ++c)
                {
                    mNodes[i]->GetConnection(c)->SetIsSelected(false);
                }
            }
        }

        // trigger the selection change callback
        if (numBefore != CalcNumSelectedNodes())
        {
            mGraphWidget->OnSelectionChanged();
        }
    }


    // add a connection
    NodeConnection* NodeGraph::AddConnection(GraphNode* sourceNode, uint32 outputPortNr, GraphNode* targetNode, uint32 inputPortNr)
    {
        NodeConnection* connection = new NodeConnection(targetNode, inputPortNr, sourceNode, outputPortNr);
        targetNode->AddConnection(connection);
        return connection;
    }


    // select connections close to a given point
    NodeConnection* NodeGraph::SelectConnectionCloseTo(const QPoint& point, bool overwriteCurSelection, bool toggle)
    {
        // for all nodes
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* node = mNodes[i];

            // check the connections of this node
            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                // if the point is close to this connection
                NodeConnection* connection = node->GetConnection(c);
                if (connection->CheckIfIsCloseTo(point))
                {
                    // unselect all connections in all nodes
                    if (overwriteCurSelection)
                    {
                        for (uint32 n = 0; n < numNodes; ++n)
                        {
                            for (uint32 a = 0; a < mNodes[n]->GetNumConnections(); ++a)
                            {
                                mNodes[n]->GetConnection(a)->SetIsSelected(false);
                            }
                        }
                    }

                    // select the current node
                    if (toggle)
                    {
                        mNodes[i]->GetConnection(c)->ToggleSelected();
                    }
                    else
                    {
                        mNodes[i]->GetConnection(c)->SetIsSelected(true);
                    }

                    // trigger the selection change callback
                    mGraphWidget->OnSelectionChanged();

                    return connection;
                }
                else
                {
                    // disable the selection of this connection if we are replacing the selection
                    if (overwriteCurSelection)
                    {
                        connection->SetIsSelected(false);
                    }
                }
            }
        }

        // trigger the selection change callback
        mGraphWidget->OnSelectionChanged();

        // no connection found
        return nullptr;
    }


    // render a small grid
    void NodeGraph::RenderSubGrid(QPainter& painter, int32 startX, int32 startY)
    {
        // setup spacing and size of the grid
        const int32 endX = startX + 300; // 300 units
        const int32 endY = startY + 300;
        const int32 spacing = 50;       // grid cell size of 50
        /*
            // draw vertical lines
            for (int32 x=startX; x<=endX; x+=spacing)
                painter.drawLine(x, startY, x, endY);

            // draw horizontal lines
            for (int32 y=startY; y<=endY; y+=spacing)
                painter.drawLine(startX, y, endX, y);
        */

        // calculate the alpha
        float scale = mScale * mScale * 1.5f;
        scale = MCore::Clamp<float>(scale, 0.0f, 1.0f);
        const int32 alpha = MCore::CalcCosineInterpolationWeight(scale) * 255;

        // draw the checkerboard
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(35, 35, 35, alpha));
        bool even = true;
        for (int32 x = startX; x < endX; x += spacing)
        {
            int32 nr = even ? 0 : 1;
            for (int32 y = startY; y < endY; y += spacing)
            {
                if (nr % 2 == 0 && alpha > 5)
                {
                    painter.drawRect(x, y, spacing, spacing);
                }

                nr++;
            }
            even ^= true;
        }
    }


    // render the background
    void NodeGraph::RenderBackground(QPainter& painter, int32 width, int32 height)
    {
        // grid line color
        painter.setPen(QColor(40, 40, 40));

        // calculate the coordinates in 'zoomed out and scrolled' coordinates, of the window rect
        QPoint upperLeft    = mTransform.inverted().map(QPoint(0, 0));
        QPoint lowerRight   = mTransform.inverted().map(QPoint(width, height));

        // calculate the start and end ranges in 'scrolled and zoomed out' coordinates
        // we need to render sub-grids covering that area
        const int32 startX  = upperLeft.x() - (upperLeft.x() % 100) - 100;
        const int32 startY  = upperLeft.y() - (upperLeft.y() % 100) - 100;
        const int32 endX    = lowerRight.x();
        const int32 endY    = lowerRight.y();
        /*
            // render the subgrid patches
            for (int32 x=startX; x<endX; x+=300)
                for (int32 y=startY; y<endY; y+=300)
                    RenderSubGrid(painter, x, y);
        */

        // calculate the alpha
        float scale = mScale * mScale * 1.5f;
        scale = MCore::Clamp<float>(scale, 0.0f, 1.0f);
        const int32 alpha = MCore::CalcCosineInterpolationWeight(scale) * 255;

        if (alpha < 10)
        {
            return;
        }

        //  mGridPen.setColor( QColor(58, 58, 58, alpha) );
        //  mGridPen.setColor( QColor(46, 46, 46, alpha) );
        mGridPen.setColor(QColor(61, 61, 61, alpha));
        mSubgridPen.setColor(QColor(55, 55, 55, alpha));

        // setup spacing and size of the grid
        const int32 spacing = 10;       // grid cell size of 20

        // draw subgridlines first
        painter.setPen(mSubgridPen);

        // draw vertical lines
        for (int32 x = startX; x < endX; x += spacing)
        {
            if ((x - startX) % 100 != 0)
            {
                painter.drawLine(x, startY, x, endY);
            }
        }

        // draw horizontal lines
        for (int32 y = startY; y < endY; y += spacing)
        {
            if ((y - startY) % 100 != 0)
            {
                painter.drawLine(startX, y, endX, y);
            }
        }

        // draw render grid lines
        painter.setPen(mGridPen);

        // draw vertical lines
        for (int32 x = startX; x < endX; x += spacing)
        {
            if ((x - startX) % 100 == 0)
            {
                painter.drawLine(x, startY, x, endY);
            }
        }

        // draw horizontal lines
        for (int32 y = startY; y < endY; y += spacing)
        {
            if ((y - startY) % 100 == 0)
            {
                painter.drawLine(startX, y, endX, y);
            }
        }
    }



    // render a small grid
    void NodeGraph::RenderLinesSubGrid(QPainter& painter, int32 startX, int32 startY)
    {
        // calculate the alpha
        float scale = mScale * mScale * 1.5f;
        scale = MCore::Clamp<float>(scale, 0.0f, 1.0f);
        const int32 alpha = MCore::CalcCosineInterpolationWeight(scale) * 255;

        if (alpha < 10)
        {
            return;
        }

        painter.setPen(QColor(58, 58, 58, alpha));

        // setup spacing and size of the grid
        const int32 endX = startX + 100; // 300 units
        const int32 endY = startY + 100;
        const int32 spacing = 20;       // grid cell size of 20

        // draw vertical lines
        for (int32 x = startX; x < endX; x += spacing)
        {
            if (x != startX)
            {
                painter.setPen(QColor(55, 55, 55, alpha));
            }
            else
            {
                painter.setPen(QColor(58, 58, 58, alpha));
            }
            //          painter.setPen( QColor(43, 43, 43, alpha) );

            painter.drawLine(x, startY, x, endY);
        }

        // draw horizontal lines
        for (int32 y = startY; y < endY; y += spacing)
        {
            if (y != startY)
            {
                painter.setPen(QColor(55, 55, 55, alpha));
            }
            else
            {
                painter.setPen(QColor(58, 58, 58, alpha));
            }
            //painter.setPen( QColor(43, 43, 43, alpha) );

            painter.drawLine(startX, y, endX, y);
        }
    }



    // NOTE: Based on code from: http://alienryderflex.com/intersect/
    //
    //  Determines the intersection point of the line segment defined by points A and B
    //  with the line segment defined by points C and D.
    //
    //  Returns true if the intersection point was found, and stores that point in X,Y.
    //  Returns false if there is no determinable intersection point, in which case X,Y will
    //  be unmodified.
    bool NodeGraph::LinesIntersect(double Ax, double Ay, double Bx, double By,
        double Cx, double Cy, double Dx, double Dy,
        double* X, double* Y)
    {
        double distAB, theCos, theSin, newX, ABpos;

        //  Fail if either line segment is zero-length.
        if ((Ax == Bx && Ay == By) || (Cx == Dx && Cy == Dy))
        {
            return false;
        }

        //  Fail if the segments share an end-point.
        if ((Ax == Cx && Ay == Cy) || (Bx == Cx && By == Cy) || (Ax == Dx && Ay == Dy) || (Bx == Dx && By == Dy))
        {
            return false;
        }

        //  (1) Translate the system so that point A is on the origin.
        Bx -= Ax;
        By -= Ay;
        Cx -= Ax;
        Cy -= Ay;
        Dx -= Ax;
        Dy -= Ay;

        //  Discover the length of segment A-B.
        distAB = sqrt(Bx * Bx + By * By);

        //  (2) Rotate the system so that point B is on the positive X axis.
        theCos  = Bx / distAB;
        theSin  = By / distAB;
        newX    = Cx * theCos + Cy * theSin;
        Cy      = Cy * theCos - Cx * theSin;
        Cx      = newX;
        newX    = Dx * theCos + Dy * theSin;
        Dy      = Dy * theCos - Dx * theSin;
        Dx      = newX;

        //  Fail if segment C-D doesn't cross line A-B.
        if ((Cy < 0.0 && Dy < 0.0) || (Cy >= 0.0 && Dy >= 0.0))
        {
            return false;
        }

        //  (3) Discover the position of the intersection point along line A-B.
        ABpos = Dx + (Cx - Dx) * Dy / (Dy - Cy);

        //  Fail if segment C-D crosses line A-B outside of segment A-B.
        if (ABpos < 0.0 || ABpos > distAB)
        {
            return false;
        }

        //  (4) Apply the discovered position to line A-B in the original coordinate system.
        if (X)
        {
            *X = Ax + ABpos * theCos;
        }
        if (Y)
        {
            *Y = Ay + ABpos * theSin;
        }

        // intersection found
        return true;
    }


    // check intersection between line and rect
    bool NodeGraph::LineIntersectsRect(const QRect& b, float x1, float y1, float x2, float y2, double* outX, double* outY)
    {
        // check first if any of the points are inside the rect
        if (outX == nullptr && outY == nullptr)
        {
            if (b.contains(QPoint(x1, y1)) || b.contains(QPoint(x2, y2)))
            {
                return true;
            }
        }

        // if not test for intersection with the line segments
        // check the top
        if (LinesIntersect(x1, y1, x2, y2, b.topLeft().x(), b.topLeft().y(), b.topRight().x(), b.topRight().y(), outX, outY))
        {
            return true;
        }

        // check the bottom
        if (LinesIntersect(x1, y1, x2, y2, b.bottomLeft().x(), b.bottomLeft().y(), b.bottomRight().x(), b.bottomRight().y(), outX, outY))
        {
            return true;
        }

        // check the left
        if (LinesIntersect(x1, y1, x2, y2, b.topLeft().x(), b.topLeft().y(), b.bottomLeft().x(), b.bottomLeft().y(), outX, outY))
        {
            return true;
        }

        // check the right
        if (LinesIntersect(x1, y1, x2, y2, b.topRight().x(), b.topRight().y(), b.bottomRight().x(), b.bottomRight().y(), outX, outY))
        {
            return true;
        }

        return false;
    }


    // distance to a line
    float NodeGraph::DistanceToLine(float x1, float y1, float x2, float y2, float px, float py)
    {
        const AZ::Vector2 pos(px, py);
        const AZ::Vector2 lineStart(x1, y1);
        const AZ::Vector2 lineEnd(x2, y2);

        // a vector from start to end of the line
        const AZ::Vector2 startToEnd = lineEnd - lineStart;

        // the distance of pos projected on the line
        float t = (pos - lineStart).Dot(startToEnd) / startToEnd.GetLengthSq();

        // make sure that we clip this distance to be sure its on the line segment
        if (t < 0.0f)
        {
            t = 0.0f;
        }
        if (t > 1.0f)
        {
            t = 1.0f;
        }

        // calculate the position projected on the line
        const AZ::Vector2 projected = lineStart + t * startToEnd;

        // the vector from the projected position to the point we are testing with
        return (pos - projected).GetLength();
    }


    // calc the number of selected nodes
    uint32 NodeGraph::CalcNumSelectedNodes() const
    {
        uint32 result = 0;

        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (mNodes[i]->GetIsSelected())
            {
                result++;
            }
        }

        return result;
    }


    // calc the number of selected connections
    uint32 NodeGraph::CalcNumSelectedConnections() const
    {
        uint32 result = 0;

        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* node = mNodes[i];

            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                if (node->GetConnection(c)->GetIsSelected())
                {
                    result++;
                }
            }
        }

        return result;
    }



    GraphNode* NodeGraph::FindFirstSelectedGraphNode() const
    {
        // get the number of nodes and iterate through them
        const uint32 numNodes = GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // check if the node is selected, if so, delete it
            GraphNode* node = mNodes[i];
            if (node->GetIsSelected())
            {
                // take the first selected node
                return node;
            }
        }

        return nullptr;
    }


    MCore::Array<GraphNode*> NodeGraph::GetSelectedNodes() const
    {
        MCore::Array<GraphNode*> result;

        // get the number of nodes and iterate through them
        const uint32 numNodes = GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // check if the node is selected, if so, add it to the array
            GraphNode* node = mNodes[i];

            if (node->GetIsSelected())
            {
                result.Add(node);
            }
        }

        return result;
    }


    NodeConnection* NodeGraph::FindFirstSelectedConnection() const
    {
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* node = mNodes[i];

            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                if (node->GetConnection(c)->GetIsSelected())
                {
                    return node->GetConnection(c);
                }
            }
        }

        return nullptr;
    }


    // calc the number of selected nodes and connections together
    uint32 NodeGraph::CalcNumSelectedItems() const
    {
        return CalcNumSelectedConnections() + CalcNumSelectedNodes();
    }


    // calc the selection rect
    QRect NodeGraph::CalcRectFromSelection(bool includeConnections) const
    {
        QRect result;

        // for all nodes
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* node = mNodes[i];

            // add the rect
            if (node->GetIsSelected())
            {
                result = result.united(node->GetRect());
            }

            // if we want to include connections in the rect
            if (includeConnections)
            {
                // for all connections
                const uint32 numConnections = node->GetNumConnections();
                for (uint32 c = 0; c < numConnections; ++c)
                {
                    if (node->GetConnection(c)->GetIsSelected())
                    {
                        result = result.united(node->GetConnection(c)->CalcRect());
                    }
                }
            }
        }

        return result;
    }


    // calculate the rect from the entire graph
    QRect NodeGraph::CalcRectFromGraph() const
    {
        QRect result;

        // for all nodes
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* node = mNodes[i];

            // add the rect
            result = result.united(node->GetRect());

            // for all connections
            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                result = result.united(node->GetConnection(c)->CalcRect());
            }
        }

        return result;
    }


    // make the given rect visible
    void NodeGraph::ZoomOnRect(const QRect& rect, int32 width, int32 height, bool animate)
    {
        QRect localRect = rect;

        // calculate the space left after we move this the rect to the upperleft of the screen
        const int32 widthLeft   = width - localRect.width();
        const int32 heightLeft  = height - localRect.height();

        //MCore::LOG("Screen: width=%d, height=%d, Rect: width=%d, height=%d", width, height, localRect.width(), localRect.height());

        if (widthLeft > 0 && heightLeft > 0)
        {
            // center the rect in the middle of the screen
            QPoint offset;
            const int32 left = localRect.left();
            const int32 top = localRect.top();
            offset.setX(-left + widthLeft / 2);
            offset.setY(-top  + heightLeft / 2);

            if (animate)
            {
                ZoomTo(1.0f);
                ScrollTo(offset);
            }
            else
            {
                mScrollOffset = offset;
                mScale = 1.0f;
            }
        }
        else
        {
            // grow the rect a bit to keep some empty space around the borders
            localRect.adjust(-5, -5, 5, 5);

            // put the center of the selection in the middle of the screen
            QPoint offset = -localRect.center() + QPoint(width / 2, height / 2);
            if (animate)
            {
                ScrollTo(offset);
            }
            else
            {
                mScrollOffset = offset;
            }

            // set the zoom factor so it exactly fits
            // find out how many extra pixels we need to fit on screen
            const int32 widthDif  = localRect.width()  - width;
            const int32 heightDif = localRect.height() - height;

            // calculate how much zoom out we need for width and height
            float widthZoom  = 1.0f;
            float heightZoom = 1.0f;

            if (widthDif > 0)
            {
                widthZoom  = 1.0f / ((widthDif / (float)width) + 1.0f);
            }

            if (heightDif > 0)
            {
                heightZoom = 1.0f / ((heightDif / (float)height) + 1.0f);
            }

            if (animate == false)
            {
                mScale = MCore::Min<float>(widthZoom, heightZoom);
            }
            else
            {
                ZoomTo(MCore::Min<float>(widthZoom, heightZoom));
            }
        }
    }


    // start an animated scroll to the given scroll offset
    void NodeGraph::ScrollTo(const QPointF& point)
    {
        mStartScrollOffset  = mScrollOffset;
        mTargetScrollOffset = point;
        mScrollTimer.start(1000 / 60);
        mScrollPreciseTimer.Stamp();
    }


    // update the animated scroll offset
    void NodeGraph::UpdateAnimatedScrollOffset()
    {
        const float duration = 0.75f; // duration in seconds

        float timePassed = mScrollPreciseTimer.GetDeltaTimeInSeconds();
        if (timePassed > duration)
        {
            timePassed = duration;
            mScrollTimer.stop();
        }

        const float t = timePassed / duration;
        mScrollOffset = MCore::CosineInterpolate<QPointF>(mStartScrollOffset, mTargetScrollOffset, t).toPoint();
        //mGraphWidget->update();
    }


    // update the animated scale
    void NodeGraph::UpdateAnimatedScale()
    {
        const float duration = 0.75f; // duration in seconds

        float timePassed = mScalePreciseTimer.GetDeltaTimeInSeconds();
        if (timePassed > duration)
        {
            timePassed = duration;
            mScaleTimer.stop();
        }

        const float t = timePassed / duration;
        mScale = MCore::CosineInterpolate<float>(mStartScale, mTargetScale, t);
        //mGraphWidget->update();
    }

    //static float scaleExp = 1.0f;

    // zoom in
    void NodeGraph::ZoomIn()
    {
        //scaleExp += 0.2f;
        //if (scaleExp > 1.0f)
        //scaleExp = 1.0f;

        //float t = -6 + (6 * scaleExp);
        //float newScale = 1/(1+exp(-t)) * 2;

        float newScale = mScale + 0.35f;
        newScale = MCore::Clamp<float>(newScale, mLowestScale, 1.0f);
        ZoomTo(newScale);
    }



    // zoom out
    void NodeGraph::ZoomOut()
    {
        float newScale = mScale - 0.35f;
        //scaleExp -= 0.2f;
        //if (scaleExp < 0.01f)
        //scaleExp = 0.01f;

        //  float t = -6 + (6 * scaleExp);
        //float newScale = 1/(1+exp(-t)) * 2;
        //MCore::LogDebug("%f", newScale);
        newScale = MCore::Clamp<float>(newScale, mLowestScale, 1.0f);
        ZoomTo(newScale);
    }


    // zoom to a given amount
    void NodeGraph::ZoomTo(float scale)
    {
        mStartScale     = mScale;
        mTargetScale    = scale;
        mScaleTimer.start(1000 / 60);
        mScalePreciseTimer.Stamp();
        if (scale < mLowestScale)
        {
            mLowestScale = scale;
        }
    }


    // stop an animated zoom
    void NodeGraph::StopAnimatedZoom()
    {
        mScaleTimer.stop();
    }


    // stop an animated scroll
    void NodeGraph::StopAnimatedScroll()
    {
        mScrollTimer.stop();
    }


    // fit the graph on the screen
    void NodeGraph::FitGraphOnScreen(int32 width, int32 height, const QPoint& mousePos, bool autoUpdate, bool animate)
    {
        MCORE_UNUSED(autoUpdate);

        // fit the entire graph in the view
        UpdateNodesAndConnections(width, height, mousePos);
        QRect sceneRect = CalcRectFromGraph();

        //  MCore::LOG("sceneRect: (%d, %d, %d, %d)", sceneRect.left(), sceneRect.top(), sceneRect.width(), sceneRect.height());

        if (sceneRect.isEmpty() == false)
        {
            const float border = 10.0f * (1.0f / mScale);
            sceneRect.adjust(-border, -border, border, border);
            ZoomOnRect(sceneRect, width, height, animate);
        }

        mStartFitHappened = true;

        //if (autoUpdate)
        //  mGraphWidget->update();
    }


    // remove a given node
    void NodeGraph::RemoveNode(GraphNode* node, bool delFromMem)
    {
        if (mEntryNode == node)
        {
            mEntryNode = nullptr;
        }

        mNodes.RemoveByValue(node);
        if (delFromMem)
        {
            delete node;
        }
    }


    // find the port at a given location
    NodePort* NodeGraph::FindPort(int32 x, int32 y, GraphNode** outNode, uint32* outPortNr, bool* outIsInputPort, bool includeInputPorts)
    {
        // get the number of nodes in the graph and iterate through them
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 n = 0; n < numNodes; ++n)
        {
            // get a pointer to the graph node
            GraphNode* graphNode = mNodes[n];

            // skip the node in case it is collapsed
            if (graphNode->GetIsCollapsed())
            {
                continue;
            }

            // check if we're in a port of the given node
            NodePort* result = graphNode->FindPort(x, y, outPortNr, outIsInputPort, includeInputPorts);
            if (result)
            {
                *outNode = graphNode;
                return result;
            }
        }

        // failure, no port at the given coordinates
        return nullptr;
    }


    // start creating a connection
    void NodeGraph::StartCreateConnection(uint32 portNr, bool isInputPort, GraphNode* portNode, NodePort* port, const QPoint& startOffset)
    {
        mConPortNr          = portNr;
        mConIsInputPort     = isInputPort;
        mConNode            = portNode;
        mConPort            = port;
        mConStartOffset     = startOffset;
    }


    // start relinking a connection
    void NodeGraph::StartRelinkConnection(NodeConnection* connection, uint32 portNr, GraphNode* node)
    {
        mConPortNr              = portNr;
        mConNode                = node;
        mRelinkConnection       = connection;

        //MCore::LogInfo( "StartRelinkConnection: Connection=(%s->%s) portNr=%i, graphNode=%s", connection->GetSourceNode()->GetName(), connection->GetTargetNode()->GetName(), portNr, node->GetName()  );
    }


    void NodeGraph::StartReplaceTransitionHead(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode)
    {
        mReplaceTransitionHead = connection;

        mReplaceTransitionStartOffset   = startOffset;
        mReplaceTransitionEndOffset     = endOffset;
        mReplaceTransitionSourceNode    = sourceNode;
        mReplaceTransitionTargetNode    = targetNode;
    }


    void NodeGraph::StartReplaceTransitionTail(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode)
    {
        mReplaceTransitionTail = connection;

        mReplaceTransitionStartOffset   = startOffset;
        mReplaceTransitionEndOffset     = endOffset;
        mReplaceTransitionSourceNode    = sourceNode;
        mReplaceTransitionTargetNode    = targetNode;
    }


    void NodeGraph::GetReplaceTransitionInfo(NodeConnection** outConnection, QPoint* outStartOffset, QPoint* outEndOffset, GraphNode** outSourceNode, GraphNode** outTargetNode)
    {
        if (mReplaceTransitionHead)
        {
            *outConnection = mReplaceTransitionHead;
        }
        if (mReplaceTransitionTail)
        {
            *outConnection = mReplaceTransitionTail;
        }

        *outStartOffset = mReplaceTransitionStartOffset;
        *outEndOffset   = mReplaceTransitionEndOffset;
        *outSourceNode  = mReplaceTransitionSourceNode;
        *outTargetNode  = mReplaceTransitionTargetNode;
    }


    void NodeGraph::StopReplaceTransitionHead()
    {
        mReplaceTransitionHead = nullptr;
    }


    void NodeGraph::StopReplaceTransitionTail()
    {
        mReplaceTransitionTail = nullptr;
    }



    // reset members
    void NodeGraph::StopRelinkConnection()
    {
        mConPortNr              = MCORE_INVALIDINDEX32;
        mConNode                = nullptr;
        mRelinkConnection       = nullptr;
        mConIsValid             = false;
        mTargetPort             = nullptr;
    }



    // reset members
    void NodeGraph::StopCreateConnection()
    {
        mConPortNr          = MCORE_INVALIDINDEX32;
        mConIsInputPort     = true;
        mConNode            = nullptr;  // nullptr when no connection is being created
        mConPort            = nullptr;
        mTargetPort         = nullptr;
        mConIsValid         = false;
    }


    // render the connection we're creating, if any
    void NodeGraph::RenderReplaceTransition(QPainter& painter)
    {
        // prepare the Qt painter
        QColor headTailColor(0, 255, 0);
        painter.setPen(headTailColor);
        painter.setBrush(headTailColor);
        const uint32 circleRadius = 4;

        // get the number of nodes and iterate through them
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* graphNode = mNodes[i];

            // get the number of connections and iterate through them
            const uint32 numConnections = graphNode->GetNumConnections();
            for (uint32 j = 0; j < numConnections; ++j)
            {
                NodeConnection* connection = graphNode->GetConnection(j);

                // in case the mouse is over the transition
                if (connection->GetIsTailHighlighted() && connection->GetIsWildcardTransition() == false)
                {
                    // calculate its start and end points
                    QPoint start, end;
                    connection->CalcStartAndEndPoints(start, end);

                    // calculate the normalized direction vector of the transition from tail to head
                    AZ::Vector2 dir = AZ::Vector2(end.x() - start.x(), end.y() - start.y());
                    dir.Normalize();

                    AZ::Vector2 newStart = AZ::Vector2(start.x(), start.y()) + dir * (float)circleRadius;
                    painter.drawEllipse(QPoint(newStart.GetX(), newStart.GetY()), circleRadius, circleRadius);
                    return;
                }
            }
        }
    }


    // render the connection we're creating, if any
    void NodeGraph::RenderCreateConnection(QPainter& painter)
    {
        if (GetIsRelinkingConnection())
        {
            // gather some information from the connection
            NodeConnection* connection      = GetRelinkConnection();
            //GraphNode*        sourceNode      = connection->GetSourceNode();
            //uint32            sourcePortNr    = connection->GetOutputPortNr();
            //NodePort*     port            = sourceNode->GetOutputPort( connection->GetOutputPortNr() );
            QPoint          start           = connection->GetSourceRect().center();
            QPoint          end             = mGraphWidget->GetMousePos();

            QPen pen;
            pen.setColor(QColor(100, 100, 100));
            pen.setStyle(Qt::DotLine);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);

            QRect areaRect(end.x() - 150, end.y() - 150, 300, 300);
            const uint32 numNodes = mNodes.GetLength();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                GraphNode* node = mNodes[n];

                if (node->GetIsCollapsed())
                {
                    continue;
                }

                // if the node isn't intersecting the area rect it is not close enough
                if (areaRect.intersects(node->GetRect()) == false)
                {
                    continue;
                }

                // now check all ports to see if they would be valid
                const uint32 numInputPorts = node->GetNumInputPorts();
                for (uint32 i = 0; i < numInputPorts; ++i)
                {
                    if (BlendGraphWidget::CheckIfIsRelinkConnectionValid(mRelinkConnection, node, i, true))
                    {
                        QPoint tempStart = end;
                        QPoint tempEnd = node->GetInputPort(i)->GetRect().center();

                        if ((tempStart - tempEnd).manhattanLength() < 150)
                        {
                            painter.drawLine(tempStart, tempEnd);
                        }
                    }
                }
            }

            // figure out the color of the connection line
            if (mTargetPort)
            {
                if (mConIsValid)
                {
                    painter.setPen(QColor(0, 255, 0));
                }
                else
                {
                    painter.setPen(QColor(255, 0, 0));
                }
            }
            else
            {
                painter.setPen(QColor(255, 255, 0));
            }

            // render the smooth line towards the mouse cursor
            painter.setBrush(Qt::NoBrush);

            //if (mGraphWidget->CreateConnectionMustBeCurved())
            DrawSmoothedLineFast(painter, start.x(), start.y(), end.x(), end.y(), 1);
            /*else
            {
                QRect sourceRect = GetCreateConnectionNode()->GetRect();
                sourceRect.adjust(-2,-2,2,2);

                if (sourceRect.contains(end))
                    return;

                // calc the real start point
                double realX, realY;
                if (NodeGraph::LineIntersectsRect(sourceRect, start.x(), start.y(), end.x(), end.y(), &realX, &realY))
                {
                    start.setX( realX );
                    start.setY( realY );
                }

                painter.drawLine( start, end );
            }*/
        }


        // if we're not creating a connection there is nothing to render
        if (GetIsCreatingConnection() == false)
        {
            return;
        }

        //------------------------------------------
        // draw the suggested valid connections
        //------------------------------------------
        QPoint start = mGraphWidget->GetMousePos();
        QPoint end;

        QPen pen;
        pen.setColor(QColor(100, 100, 100));
        pen.setStyle(Qt::DotLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        if (mGraphWidget->CreateConnectionShowsHelpers())
        {
            QRect areaRect(start.x() - 150, start.y() - 150, 300, 300);
            const uint32 numNodes = mNodes.GetLength();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                GraphNode* node = mNodes[n];

                if (node->GetIsCollapsed())
                {
                    continue;
                }

                // if the node isn't intersecting the area rect it is not close enough
                if (areaRect.intersects(node->GetRect()) == false)
                {
                    continue;
                }

                // now check all ports to see if they would be valid
                const uint32 numInputPorts = node->GetNumInputPorts();
                for (uint32 i = 0; i < numInputPorts; ++i)
                {
                    if (mGraphWidget->CheckIfIsCreateConnectionValid(i, node, node->GetInputPort(i), true))
                    {
                        end = node->GetInputPort(i)->GetRect().center();

                        if ((start - end).manhattanLength() < 150)
                        {
                            painter.drawLine(start, end);
                        }
                    }
                }

                // now check all ports to see if they would be valid
                const uint32 numOutputPorts = node->GetNumOutputPorts();
                for (uint32 a = 0; a < numOutputPorts; ++a)
                {
                    if (mGraphWidget->CheckIfIsCreateConnectionValid(a, node, node->GetOutputPort(a), false))
                    {
                        end = node->GetOutputPort(a)->GetRect().center();

                        if ((start - end).manhattanLength() < 150)
                        {
                            painter.drawLine(start, end);
                        }
                    }
                }
            }
        }

        //------------------------------

        // update the end point
        //start = mConPort->GetRect().center();
        start = GetCreateConnectionNode()->GetRect().topLeft() + GetCreateConnectionStartOffset();
        end   = mGraphWidget->GetMousePos();

        // figure out the color of the connection line
        if (mTargetPort)
        {
            if (mConIsValid)
            {
                painter.setPen(QColor(0, 255, 0));
            }
            else
            {
                painter.setPen(QColor(255, 0, 0));
            }
        }
        else
        {
            painter.setPen(QColor(255, 255, 0));
        }

        // render the smooth line towards the mouse cursor
        painter.setBrush(Qt::NoBrush);

        if (mGraphWidget->CreateConnectionMustBeCurved())
        {
            DrawSmoothedLineFast(painter, start.x(), start.y(), end.x(), end.y(), 1);
        }
        else
        {
            QRect sourceRect = GetCreateConnectionNode()->GetRect();
            sourceRect.adjust(-2, -2, 2, 2);

            if (sourceRect.contains(end))
            {
                return;
            }

            // calc the real start point
            double realX, realY;
            if (NodeGraph::LineIntersectsRect(sourceRect, start.x(), start.y(), end.x(), end.y(), &realX, &realY))
            {
                start.setX(realX);
                start.setY(realY);
            }

            painter.drawLine(start, end);
        }
    }


    // check if this connection already exists
    bool NodeGraph::CheckIfHasConnection(GraphNode* sourceNode, uint32 outputPortNr, GraphNode* targetNode, uint32 inputPortNr) const
    {
        const uint32 numConnections = targetNode->GetNumConnections();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            NodeConnection* connection = targetNode->GetConnection(i);

            // check if the connection properties are equal
            if (connection->GetInputPortNr() == inputPortNr)
            {
                if (connection->GetSourceNode() == sourceNode)
                {
                    if (connection->GetOutputPortNr() == outputPortNr)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }


    NodeConnection* NodeGraph::FindInputConnection(GraphNode* targetNode, uint32 targetPortNr) const
    {
        if (targetNode == nullptr || targetPortNr == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        const uint32 numConnections = targetNode->GetNumConnections();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            NodeConnection* connection = targetNode->GetConnection(i);

            // check if the connection ports are equal
            if (connection->GetInputPortNr() == targetPortNr)
            {
                return connection;
            }
        }

        return nullptr;
    }


    GraphNode* NodeGraph::FindNodeById(EMotionFX::AnimGraphNodeId id)
    {
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (mNodes[i]->GetId() == id)
            {
                return mNodes[i];
            }
        }

        return nullptr;
    }
}   // namespace EMotionFX

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.moc>
