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

#ifndef CRYINCLUDE_CRYCOMMON_IAIACTION_H
#define CRYINCLUDE_CRYCOMMON_IAIACTION_H
#pragma once


struct IGoalPipe;

///////////////////////////////////////////////////
// IAIAction references an Action Flow Graph -
// ordered sequence of elementary actions which
// will be executed to "use" a smart object
///////////////////////////////////////////////////
struct IAIAction
{
    /// Event names used to inform listeners of an action when the action's state changes
    enum EActionEvent
    {
        ActionEnd = 0,
        ActionStart,
        ActionCancel,
        ActionAbort,
    };

    struct IAIActionListener
    {
        // <interfuscator:shuffle>
        virtual ~IAIActionListener(){}
        virtual void OnActionEvent(EActionEvent event) = 0;
        // </interfuscator:shuffle>
    };

    // <interfuscator:shuffle>
    virtual ~IAIAction(){}
    // returns the unique name of this AI Action
    virtual const char* GetName() const = 0;

    // returns the goal pipe which executes this AI Action
    virtual IGoalPipe* GetGoalPipe() const = 0;

    // returns the User entity associated to this AI Action
    virtual IEntity* GetUserEntity() const = 0;

    // returns the Object entity associated to this AI Action
    virtual IEntity* GetObjectEntity() const = 0;

    // returns true if action is active and marked as high priority
    virtual bool IsHighPriority() const { return false; }

    // ends execution of this AI Action
    virtual void EndAction() = 0;

    // cancels execution of this AI Action
    virtual void CancelAction() = 0;

    // aborts execution of this AI Action - will start clean up procedure
    virtual bool AbortAction() = 0;

    // marks this AI Action as modified
    virtual void Invalidate() = 0;

    virtual bool IsSignaledAnimation() const = 0;
    virtual bool IsExactPositioning() const = 0;
    virtual const char* GetAnimationName() const = 0;

    virtual const Vec3& GetAnimationPos() const = 0;
    virtual const Vec3& GetAnimationDir() const = 0;
    virtual bool IsUsingAutoAssetAlignment() const = 0;

    virtual const Vec3& GetApproachPos() const = 0;

    virtual float GetStartWidth() const = 0;
    virtual float GetStartArcAngle() const = 0;
    virtual float GetDirectionTolerance() const = 0;

    virtual void RegisterListener(IAIActionListener* eventListener, const char* name) = 0;
    virtual void UnregisterListener(IAIActionListener* eventListener) = 0;
    // </interfuscator:shuffle>
};


#endif // CRYINCLUDE_CRYCOMMON_IAIACTION_H
