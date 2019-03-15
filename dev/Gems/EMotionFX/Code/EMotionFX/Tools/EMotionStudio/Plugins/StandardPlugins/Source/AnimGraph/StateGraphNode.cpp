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

#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/StateGraphNode.h>
#include <MCore/Source/Compare.h>


namespace EMStudio
{
    StateConnection::StateConnection(const QModelIndex& modelIndex, GraphNode* sourceNode, GraphNode* targetNode, bool isWildcardConnection)
        : NodeConnection(modelIndex, targetNode, 0, sourceNode, 0)
    {
        mColor                  = QColor(125, 125, 125);
        mIsWildcardConnection   = isWildcardConnection;
    }


    StateConnection::~StateConnection()
    {
    }


    void StateConnection::Render(QPainter& painter, QPen* pen, QBrush* brush, int32 stepSize, const QRect& visibleRect, float opacity, bool alwaysColor)
    {
        MCORE_UNUSED(stepSize);
        MCORE_UNUSED(visibleRect);
        MCORE_UNUSED(opacity);
        MCORE_UNUSED(alwaysColor);

        QPoint start, end;
        CalcStartAndEndPoints(start, end);

        // check if we are dealing with a wildcard transition
        if (mIsWildcardConnection)
        {
            start = end - QPoint(WILDCARDTRANSITION_SIZE, WILDCARDTRANSITION_SIZE);
            end += QPoint(3, 3);
        }

        QColor color;

        // check if we're transitioning
        EMotionFX::AnimGraphStateTransition* transition = m_modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        EMotionFX::AnimGraphInstance* transitionAnimGraphInstance = m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(m_modelIndex.parent().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>());
        EMotionFX::AnimGraphInstance* stateMachineAnimGraphInstance = m_modelIndex.parent().data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();

        bool isTransitioning = false;
        if (transitionAnimGraphInstance)
        {
            isTransitioning = stateMachine->GetActiveTransition(stateMachineAnimGraphInstance) == transition;
        }

        // draw some small horizontal lines that go outside of the connection port
        if (mIsSelected)
        {
            pen->setWidth(2);
            color.setRgb(255, 128, 0);
        }
        else
        {
            pen->setWidthF(1.5f);
            color = mColor;

            if (isTransitioning)
            {
                color.setRgb(255, 0, 255);
            }
            else
            if (mIsSynced)
            {
                color.setRgb(115, 125, 200);
            }
        }

        // darken the color in case the transition is disabled
        if (mIsDisabled)
        {
            color = color.darker(165);
        }

        // lighten the color in case the transition is highlighted
        if (mIsHighlighted)
        {
            color = color.lighter(150);
            painter.setOpacity(1.0);
        }

        // lighten the color in case the transition is connected to the currently selected node
        if (mIsConnectedHighlighted)
        {
            pen->setWidth(2);
            color = color.lighter(150);
            painter.setOpacity(1.0);
        }

        // set the pen
        pen->setColor(color);
        pen->setStyle(Qt::SolidLine);
        painter.setPen(*pen);

        // set the brush
        brush->setColor(color);
        brush->setStyle(Qt::SolidPattern);
        painter.setBrush(*brush);

        // calculate the line direction
        AZ::Vector2 lineDir = AZ::Vector2(end.x(), end.y()) - AZ::Vector2(start.x(), start.y());

        // make sure the transition isn't starting and ending at the same position, if so, return directly
        if (MCore::Compare<float>::CheckIfIsClose(lineDir.GetX(), 0.0f, MCore::Math::epsilon) &&
            MCore::Compare<float>::CheckIfIsClose(lineDir.GetY(), 0.0f, MCore::Math::epsilon))
        {
            return;
        }

        // if it is safe, get the length and normalize the direction vector
        float length = lineDir.GetLength();
        lineDir.Normalize();

        QPointF direction;
        direction.setX(lineDir.GetX() * 8.0f);
        direction.setY(lineDir.GetY() * 8.0f);

        QPointF normalOffset((end.y() - start.y()) / length, (start.x() - end.x()) / length);

        QPointF points[3];
        points[0] = end;
        points[1] = end - direction + (normalOffset * 5.0f);
        points[2] = end - direction - (normalOffset * 5.0f);

        // draw line
        if (mIsDisabled)
        {
            pen->setStyle(Qt::DashLine);
            painter.setPen(*pen);
        }
        else
        {
            pen->setStyle(Qt::SolidLine);
            painter.setPen(*pen);
        }


        // render the transition line
        // Make sure the anim graph that is currently simulated on the selected actor instance is the same anim graph as the currently
        // selected and rendered ones in the anim graph window.
        if (transitionAnimGraphInstance && transitionAnimGraphInstance->GetAnimGraph() == transition->GetAnimGraph())
        {
            const float blendWeight = transition->GetBlendWeight(transitionAnimGraphInstance);

            if (isTransitioning)
            {
                // linear gradient for the background
                QLinearGradient gradient(start, end);

                gradient.setColorAt(0.0, color);
                gradient.setColorAt(MCore::Clamp(blendWeight - 0.35f, 0.0f, 1.0f), color);
                gradient.setColorAt(MCore::Clamp(blendWeight, 0.0f, 1.0f), QColor(255, 255, 255));
                gradient.setColorAt(MCore::Clamp(blendWeight + 0.01f, 0.0f, 1.0f), color);
                gradient.setColorAt(1.0, color);

                painter.setBrush(gradient);

                pen->setBrush(gradient);
                painter.setPen(*pen);
            }
        }

        painter.drawLine(start, end);

        // render arrow head triangle
        if (mIsHeadHighlighted && mIsWildcardConnection == false)
        {
            QColor headTailColor(0, 255, 0);
            brush->setColor(headTailColor);
            painter.setBrush(*brush);
            pen->setColor(headTailColor);
            painter.setPen(*pen);
        }

        brush->setStyle(Qt::SolidPattern);
        painter.drawPolygon(points, 3);

        if (mIsHeadHighlighted)
        {
            brush->setColor(color);
            painter.setBrush(*brush);
            pen->setColor(color);
            painter.setPen(*pen);
        }

        // visualize the conditions
        RenderConditionsAndActions(transitionAnimGraphInstance, &painter, pen, brush, start, end);
    }

    void StateConnection::RenderConditionsAndActions(EMotionFX::AnimGraphInstance* animGraphInstance, QPainter* painter, QPen* pen, QBrush* brush, QPoint& start, QPoint& end)
    {
        // disable the dash pattern in case the transition is disabled
        pen->setStyle(Qt::SolidLine);
        painter->setPen(*pen);

        const AZ::Vector2   transitionStart(start.rx(), start.ry());
        const AZ::Vector2   transitionEnd(end.rx(), end.ry());

        EMotionFX::AnimGraphStateTransition* transition = m_modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        AZ_Assert(transition, "Expected non-null transition");

        const size_t numConditions = transition->GetNumConditions();
        const size_t numActions = transition->GetTriggerActionSetup().GetNumActions();
        const size_t sumSize = numConditions + numActions;
        EMotionFX::TriggerActionSetup& actionSetup = transition->GetTriggerActionSetup();

        // precalculate some values we need for the condition rendering
        const float             shapeDiameter = 3.0f;
        const float             shapeStride = 4.0f;
        const float             elementSize = shapeDiameter + shapeStride;
        const AZ::Vector2       localEnd = transitionEnd - transitionStart;

        // only draw the transition conditions in case the arrow has enough space for it, avoid zero rect sized crashes as well
        if (localEnd.GetLength() > sumSize * elementSize)
        {
            const AZ::Vector2   transitionMid = transitionStart + localEnd * 0.5;
            const AZ::Vector2   transitionDir = localEnd.GetNormalized();
            const AZ::Vector2   conditionStart = transitionMid - transitionDir * (elementSize * 0.5f * (float)(sumSize));
            const AZ::Vector2   actionStart = transitionMid - transitionDir * (elementSize * 0.5f * (float)sumSize) + transitionDir * elementSize * numConditions;

            for (size_t i = 0; i < numConditions; ++i)
            {
                EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);

                // set the condition color either green if the test went okay or red if the test returned false
                QColor conditionColor;
                if (animGraphInstance)
                {
                    conditionColor = condition->TestCondition(animGraphInstance) ? Qt::green : Qt::red;
                }
                else
                {
                    conditionColor = Qt::gray;
                }

                // darken the color in case the transition is disabled
                if (mIsDisabled)
                {
                    conditionColor = conditionColor.darker(185);
                }

                brush->setColor(conditionColor);

                // calculate the circle middle point
                const AZ::Vector2 circleMid = conditionStart + transitionDir * (elementSize * (float)i);

                // render the circle per condition
                painter->setBrush(*brush);
                painter->drawEllipse(QPointF(circleMid.GetX(), circleMid.GetY()), shapeDiameter, shapeDiameter);
            }

            QColor actionColor = Qt::yellow;
            // darken the color in case the transition is disabled
            if (mIsDisabled)
            {
                actionColor = actionColor.darker(185);
            }

            for (size_t i = 0; i < numActions; ++i)
            {
                EMotionFX::AnimGraphTriggerAction* action = actionSetup.GetAction(i);
                brush->setColor(actionColor);

                // calculate the rect left top
                const AZ::Vector2 recLeft = actionStart + transitionDir * (elementSize * (float)i) - AZ::Vector2(shapeDiameter, shapeDiameter);

                // render the rect per action
                painter->setBrush(*brush);
                painter->drawRect(recLeft.GetX(), recLeft.GetY(), shapeDiameter * 2.0f, shapeDiameter * 2.0f);
            }
        }
    }

    EMotionFX::AnimGraphTransitionCondition* StateConnection::FindCondition(const QPoint& mousePos)
    {
        // if the transition is invalid, return directly
        if (!m_modelIndex.isValid())
        {
            return nullptr;
        }

        QPoint start, end;
        CalcStartAndEndPoints(start, end);

        // check if we are dealing with a wildcard transition
        if (mIsWildcardConnection)
        {
            start = end - QPoint(WILDCARDTRANSITION_SIZE, WILDCARDTRANSITION_SIZE);
            end += QPoint(3, 3);
        }

        const AZ::Vector2   transitionStart(start.rx(), start.ry());
        const AZ::Vector2   transitionEnd(end.rx(), end.ry());

        EMotionFX::AnimGraphStateTransition* transition = m_modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        AZ_Assert(transition, "Expected non-null transition");

        const size_t numConditions = transition->GetNumConditions();

        // precalculate some values we need for the condition rendering
        const float             circleDiameter  = 3.0f;
        const float             circleStride    = 4.0f;
        const float             elementSize     = circleDiameter + circleStride;
        const AZ::Vector2   localEnd        = transitionEnd - transitionStart;

        // only draw the transition conditions in case the arrow has enough space for it, avoid zero rect sized crashes as well
        if (localEnd.GetLength() > numConditions * elementSize)
        {
            const AZ::Vector2   transitionMid   = transitionStart + localEnd * 0.5f;
            const AZ::Vector2   transitionDir   = localEnd.GetNormalized();
            const AZ::Vector2   conditionStart  = transitionMid - transitionDir * (elementSize * 0.5f * (float)numConditions);

            // iterate through the conditions and render them
            for (size_t i = 0; i < numConditions; ++i)
            {
                // get access to the condition
                EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);

                // calculate the circle middle point
                const AZ::Vector2 circleMid = conditionStart  + transitionDir * (elementSize * (float)i);

                const float distance = AZ::Vector2(AZ::Vector2(mousePos.x(), mousePos.y()) - circleMid).GetLength();
                if (distance <= circleDiameter)
                {
                    return condition;
                }
            }
        }

        return nullptr;
    }

    bool StateConnection::Intersects(const QRect& rect)
    {
        QPoint start, end;
        CalcStartAndEndPoints(start, end);
        return NodeGraph::LineIntersectsRect(rect, start.x(), start.y(), end.x(), end.y());
    }

    bool StateConnection::CheckIfIsCloseTo(const QPoint& point)
    {
        QPoint start, end;
        CalcStartAndEndPoints(start, end);
        return (NodeGraph::DistanceToLine(start.x(), start.y(), end.x(), end.y(), point.x(), point.y()) <= 5.0f);

        //QRect testRect(point.x() - 1, point.y() - 1, 2, 2);
        //return Intersects(testRect);
    }

    bool StateConnection::CheckIfIsCloseToHead(const QPoint& point) const
    {
        QPoint start, end;
        CalcStartAndEndPoints(start, end);

        AZ::Vector2 dir = AZ::Vector2(end.x() - start.x(), end.y() - start.y());
        dir.Normalize();
        AZ::Vector2 newStart = AZ::Vector2(end.x(), end.y()) - dir * 5.0f;

        return (NodeGraph::DistanceToLine(newStart.GetX(), newStart.GetY(), end.x(), end.y(), point.x(), point.y()) <= 7.0f);

    }

    bool StateConnection::CheckIfIsCloseToTail(const QPoint& point) const
    {
        QPoint start, end;
        CalcStartAndEndPoints(start, end);

        AZ::Vector2 dir = AZ::Vector2(end.x() - start.x(), end.y() - start.y());
        dir.Normalize();
        AZ::Vector2 newStart = AZ::Vector2(start.x(), start.y()) + dir * 6.0f;

        return (AZ::Vector2(newStart - AZ::Vector2(point.x(), point.y())).GetLength() <= 6.0f);
    }

    void StateConnection::CalcStartAndEndPoints(QPoint& outStart, QPoint& outEnd) const
    {
        EMotionFX::AnimGraphStateTransition* transition = m_modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        const QPoint startOffset = QPoint(transition->GetVisualStartOffsetX(), transition->GetVisualStartOffsetY());
        const QPoint endOffset = QPoint(transition->GetVisualEndOffsetX(), transition->GetVisualEndOffsetY());

        QPoint start = startOffset;
        QPoint end = mTargetNode->GetRect().topLeft() + endOffset;
        if (mSourceNode)
        {
            start += mSourceNode->GetRect().topLeft();
        }
        else
        {
            start = end - QPoint(WILDCARDTRANSITION_SIZE, WILDCARDTRANSITION_SIZE);
        }

        QRect sourceRect;
        if (mSourceNode)
        {
            sourceRect = mSourceNode->GetRect();
        }

        QRect targetRect = mTargetNode->GetRect();
        targetRect.adjust(-2, -2, 2, 2);

        // calc the real start point
        double realX, realY;
        if (NodeGraph::LineIntersectsRect(sourceRect, start.x(), start.y(), end.x(), end.y(), &realX, &realY))
        {
            start.setX(realX);
            start.setY(realY);
        }

        // calc the real end point
        if (NodeGraph::LineIntersectsRect(targetRect, start.x(), start.y(), end.x(), end.y(), &realX, &realY))
        {
            end.setX(realX);
            end.setY(realY);
        }

        outStart    = start;
        outEnd      = end;
    }


    //--------------------------------------------------------------------------------
    // class StateGraphNode
    //--------------------------------------------------------------------------------
    StateGraphNode::StateGraphNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node)
        : AnimGraphVisualNode(modelIndex, plugin, node)
    {
        ResetBorderColor();
        SetCreateConFromOutputOnly(true);

        //  mTextOptions.setAlignment( Qt::AlignCenter );

        mInputPorts.Resize(1);
        mOutputPorts.Resize(4);
    }

    StateGraphNode::~StateGraphNode()
    {
    }

    void StateGraphNode::Sync()
    {
        AnimGraphVisualNode::Sync();

        EMotionFX::AnimGraphStateMachine* parentStateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(mEMFXNode->GetParentNode());
        if (parentStateMachine->GetEntryState() == mEMFXNode)
        {
            mParentGraph->SetEntryNode(this);
        }
    }
    
    void StateGraphNode::Render(QPainter& painter, QPen* pen, bool renderShadow)
    {
        // only render if the given node is visible
        if (mIsVisible == false)
        {
            return;
        }

        // render node shadow
        if (renderShadow)
        {
            RenderShadow(painter);
        }

        EMotionFX::AnimGraphInstance* animGraphInstance = m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();

        // figure out the border color
        mBorderColor.setRgb(0, 0, 0);
        if (animGraphInstance && mEMFXNode && animGraphInstance->GetAnimGraph() == mEMFXNode->GetAnimGraph())
        {
            MCORE_ASSERT(azrtti_typeid(mEMFXNode->GetParentNode()) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>());
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(mEMFXNode->GetParentNode());
            EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetActiveTransition(animGraphInstance);
            EMotionFX::AnimGraphNode* nodeA = nullptr;
            EMotionFX::AnimGraphNode* nodeB = nullptr;
            stateMachine->GetActiveStates(animGraphInstance, &nodeA, &nodeB);
            if (transition)
            {
                if (transition->GetTargetNode() == mEMFXNode)
                {
                    mBorderColor.setRgb(0, 255, 0);
                }
                else
                if (transition->GetSourceNode() == mEMFXNode)
                {
                    mBorderColor.setRgb(255, 0, 255);
                }
            }
            else
            {
                if (nodeA == mEMFXNode)
                {
                    mBorderColor.setRgb(0, 255, 0);
                }
            }
        }

        QColor borderColor;
        pen->setWidth(2);
        if (mIsSelected)
        {
            borderColor.setRgb(255, 128, 0);
        }
        else
        {
            borderColor = mBorderColor;
        }

        // background color
        QColor bgColor;
        if (mIsSelected)
        {
            bgColor.setRgbF(0.93f, 0.547f, 0.0f, 1.0f); //  rgb(72, 63, 238)
        }
        else
        {
            bgColor = mBaseColor;
        }

        // blinking red error color
        const bool hasError = GetHasError();
        if (hasError && mIsSelected == false)
        {
            if (mParentGraph->GetUseAnimation())
            {
                borderColor = mParentGraph->GetErrorBlinkColor();
            }
            else
            {
                borderColor = Qt::red;
            }

            //bgColor = borderColor;
        }

        QColor bgColor2;
        bgColor2 = bgColor.lighter(30); // make darker actually, 30% of the old color, same as bgColor * 0.3f;

        QColor textColor = mIsSelected ? Qt::black : Qt::white;

        // is highlighted/hovered (on-mouse-over effect)
        if (mIsHighlighted)
        {
            bgColor = bgColor.lighter(120);
            bgColor2 = bgColor2.lighter(120);
        }

        // draw the main rect
        // check if we need to color all nodes or not
        //const bool colorAllNodes = GetAlwaysColor();
        //if (mIsProcessed || colorAllNodes || mIsSelected==true)
        {
            QLinearGradient bgGradient(0, mRect.top(), 0, mRect.bottom());
            bgGradient.setColorAt(0.0f, bgColor);
            bgGradient.setColorAt(1.0f, bgColor2);
            painter.setBrush(bgGradient);
            painter.setPen(borderColor);
        }
        /*  else
            {
                if (mIsHighlighted == false)
                    painter.setBrush( QColor(40, 40, 40) );
                else
                    painter.setBrush( QColor(50, 50, 50) );
            }*/

        // add 4px to have empty space for the visualize button
        painter.drawRoundedRect(mRect, BORDER_RADIUS, BORDER_RADIUS);

        // if the scale is so small that we can still see the small things
        if (mParentGraph->GetScale() > 0.3f)
        {
            // draw the visualize area
            if (mCanVisualize)
            {
                RenderVisualizeRect(painter, bgColor, bgColor2);
            }

            // render the tracks etc
            if (mEMFXNode->GetHasOutputPose() && mIsProcessed)
            {
                RenderTracks(painter, bgColor, bgColor2, 3);
            }

            // render the marker which indicates that you can go inside this node
            RenderHasChildsIndicator(painter, pen, borderColor, bgColor2);
        }

        painter.setClipping(false);

        // render the text overlay with the pre-baked node name and port names etc.
        const float textOpacity = MCore::Clamp<float>(mParentGraph->GetScale() * mParentGraph->GetScale() * 1.5f, 0.0f, 1.0f);
        painter.setOpacity(textOpacity);
        painter.setFont(mHeaderFont);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(textColor);
        //painter.drawStaticText(mRect.left(), mRect.center().y()-6, mTitleText);
        painter.drawStaticText(mRect.left(), mRect.center().y() - mTitleText.size().height() / 2, mTitleText);
        //  painter.drawPixmap( mRect, mTextPixmap );
        painter.setOpacity(1.0f);

        RenderDebugInfo(painter);
    }

    int32 StateGraphNode::CalcRequiredHeight() const
    {
        return 40;
    }

    int32 StateGraphNode::CalcRequiredWidth()
    {
        const uint32 headerWidth = mHeaderFontMetrics->width(mElidedName) + 40;

        // make sure the node is at least 100 units in width
        return MCore::Max<uint32>(headerWidth, 100);
    }

    QRect StateGraphNode::CalcInputPortRect(uint32 portNr)
    {
        MCORE_UNUSED(portNr);
        return mRect.adjusted(10, 10, -10, -10);
    }

    QRect StateGraphNode::CalcOutputPortRect(uint32 portNr)
    {
        switch (portNr)
        {
        case 0:
            return QRect(mRect.left(),     mRect.top(),            mRect.width(),  8);
            break;                                                                                                  // top
        case 1:
            return QRect(mRect.left(),     mRect.bottom() - 8,       mRect.width(),  9);
            break;                                                                                                  // bottom
        case 2:
            return QRect(mRect.left(),     mRect.top(),            8,              mRect.height());
            break;                                                                                                  // left
        case 3:
            return QRect(mRect.right() - 8,  mRect.top(),            9,              mRect.height());
            break;                                                                                                  // right
        default:
            MCORE_ASSERT(false);
            return QRect();
        }
        ;
        //MCore::LOG("CalcOutputPortRect: (%i, %i, %i, %i)", rect.top(), rect.left(), rect.bottom(), rect.right());
    }

    void StateGraphNode::RenderVisualizeRect(QPainter& painter, const QColor& bgColor, const QColor& bgColor2)
    {
        MCORE_UNUSED(bgColor2);

        QColor vizBorder;
        if (mVisualize)
        {
            vizBorder = Qt::black;
        }
        else
        {
            vizBorder = bgColor.darker(225);
        }

        painter.setPen(mVisualizeHighlighted ? QColor(255, 128, 0) : vizBorder);
        if (mIsSelected == false)
        {
            painter.setBrush(mVisualize ? mVisualizeColor : bgColor);
        }
        else
        {
            painter.setBrush(mVisualize ? QColor(255, 128, 0) : bgColor);
        }

        painter.drawRect(mVisualizeRect);
    }

    void StateGraphNode::UpdateTextPixmap()
    {
        mTitleText.setTextOption(mTextOptionsCenter);
        mTitleText.setTextFormat(Qt::PlainText);
        mTitleText.setPerformanceHint(QStaticText::AggressiveCaching);
        mTitleText.setTextWidth(mRect.width());
        mTitleText.setText(mElidedName);
        mTitleText.prepare(QTransform(), mHeaderFont);
    }
} // namespace EMStudio