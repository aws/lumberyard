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

#ifndef __EMSTUDIO_STATEGRAPHNODE_H
#define __EMSTUDIO_STATEGRAPHNODE_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include "AnimGraphVisualNode.h"
#include "NodeConnection.h"
#include <QStaticText>

class AnimGraphPlugin;


namespace EMStudio
{
    class StateConnection
        : public NodeConnection
    {
        MCORE_MEMORYOBJECTCATEGORY(StateConnection, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
    public:
        enum
        {
            TYPE_ID = 0x00000002
        };
        StateConnection(EMotionFX::AnimGraphStateMachine* stateMachine, GraphNode* sourceNode, GraphNode* targetNode, const QPoint& startOffset, const QPoint& endOffset, bool isWildcardConnection, uint32 transitionID);
        ~StateConnection();

        void Render(QPainter& painter, QPen* pen, QBrush* brush, int32 stepSize, const QRect& visibleRect, float opacity, bool alwaysColor) override;
        bool Intersects(const QRect& rect) override;
        bool CheckIfIsCloseTo(const QPoint& point) override;
        void CalcStartAndEndPoints(QPoint& start, QPoint& end) const override;

        bool CheckIfIsCloseToHead(const QPoint& point) const override;
        bool CheckIfIsCloseToTail(const QPoint& point) const override;

        uint32 GetType() override                           { return TYPE_ID; }

        EMotionFX::AnimGraphTransitionCondition* FindCondition(const QPoint& mousePos);

        void SetEndOffset(const QPoint& point)      { mEndOffset = point; }
        void SetStartOffset(const QPoint& point)    { mStartOffset = point; }

        QPoint GetStartOffset() const               { return mStartOffset; }
        QPoint GetEndOffset() const                 { return mEndOffset; }

        bool GetIsWildcardTransition() const override       { return mIsWildcardConnection; }

    private:
        void RenderConditions(QPainter* painter, QPen* pen, QBrush* brush, QPoint& start, QPoint& end);

        QPoint                                  mStartOffset;
        QPoint                                  mEndOffset;
        bool                                    mIsWildcardConnection;
        EMotionFX::AnimGraphStateMachine*      mStateMachine;
        EMotionFX::AnimGraphStateTransition*   mTransition;
    };


    // the blend graph node
    class StateGraphNode
        : public AnimGraphVisualNode
    {
        MCORE_MEMORYOBJECTCATEGORY(StateGraphNode, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        enum
        {
            TYPE_ID = 0x00000003
        };

        StateGraphNode(AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node);
        ~StateGraphNode();

        void Render(QPainter& painter, QPen* pen, bool renderShadow) override;
        void RenderVisualizeRect(QPainter& painter, const QColor& bgColor, const QColor& bgColor2) override;
        //void Update(const QRect& visibleRect, const QPoint& mousePos) override;

        int32 CalcRequiredHeight() const override;
        int32 CalcRequiredWidth() override;

        QRect CalcInputPortRect(uint32 portNr) override;
        QRect CalcOutputPortRect(uint32 portNr) override;

        bool RemoveConnection(uint32 targetPortNr, GraphNode* sourceNode, uint32 sourcePortNr, uint32 connectionID) override;

        void SyncWithEMFX() override;

        void UpdateTextPixmap() override;

        uint32 GetType() const override                     { return StateGraphNode::TYPE_ID; }
    };
}   // namespace EMStudio


#endif
