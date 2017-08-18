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

#ifndef CRYINCLUDE_EDITOR_AI_AIGOALLIBRARY_H
#define CRYINCLUDE_EDITOR_AI_AIGOALLIBRARY_H
#pragma once


#include "AiGoal.h"
class CAIBehaviorLibrary;

/*!
 * CAIGoalLibrary is collection of global AI goals.
 */
class CAIGoalLibrary
{
public:
    CAIGoalLibrary();
    ~CAIGoalLibrary() {};

    //! Add new goal to the library.
    void AddGoal(CAIGoal* goal);
    //! Remove goal from the library.
    void RemoveGoal(CAIGoal* goal);

    CAIGoal* FindGoal(const QString& name) const;

    //! Clear all goals from library.
    void ClearGoals();

    //! Get all stored goals as a vector.
    void GetGoals(std::vector<CAIGoalPtr>& goals) const;

    //! Load all goals from given path and add them to library.
    void LoadGoals(const QString& path);

    //! Initialize atomic goals from AI system.
    void InitAtomicGoals();

private:
    StdMap<QString, CAIGoalPtr> m_goals;
};

#endif // CRYINCLUDE_EDITOR_AI_AIGOALLIBRARY_H
