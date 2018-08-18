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

#ifndef CRYINCLUDE_CRYAISYSTEM_MANNEQUIN_MANNEQUINGOALOP_H
#define CRYINCLUDE_CRYAISYSTEM_MANNEQUIN_MANNEQUINGOALOP_H
#pragma once

#include "EnterLeaveUpdateGoalOp.h"


//////////////////////////////////////////////////////////////////////////
class CMannequinTagGoalOp
    : public EnterLeaveUpdateGoalOp
{
protected:
    CMannequinTagGoalOp();
    CMannequinTagGoalOp(const char* tagName);
    CMannequinTagGoalOp(const uint32 tagCrc);
    CMannequinTagGoalOp(const XmlNodeRef& node);

    uint32 GetTagCrc() const { return m_tagCrc; }

private:
    uint32 m_tagCrc;
};


//////////////////////////////////////////////////////////////////////////
class CSetAnimationTagGoalOp
    : public CMannequinTagGoalOp
{
public:
    CSetAnimationTagGoalOp();
    CSetAnimationTagGoalOp(const char* tagName);
    CSetAnimationTagGoalOp(const uint32 tagCrc);
    CSetAnimationTagGoalOp(const XmlNodeRef& node);

    virtual void Update(CPipeUser& pipeUser);
};


//////////////////////////////////////////////////////////////////////////
class CClearAnimationTagGoalOp
    : public CMannequinTagGoalOp
{
public:
    CClearAnimationTagGoalOp();
    CClearAnimationTagGoalOp(const char* tagName);
    CClearAnimationTagGoalOp(const uint32 tagCrc);
    CClearAnimationTagGoalOp(const XmlNodeRef& node);

    virtual void Update(CPipeUser& pipeUser);
};


#endif // CRYINCLUDE_CRYAISYSTEM_MANNEQUIN_MANNEQUINGOALOP_H
