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

#ifndef CRYINCLUDE_EDITOR_AI_AIGOAL_H
#define CRYINCLUDE_EDITOR_AI_AIGOAL_H
#pragma once


/*
 *  CAIGoalStage is single stage of AI goal.
 */
class CAIGoalStage
{
public:
    //! Name of goal used by this stage.
    QString name;
    //! True if this stage will block goal pipeline execution.
    bool blocking;
    //! Goal parameters.
    XmlNodeRef params;
};

/*!
 *  CAIGoal contain definition of AI goal pipe.
 */
class CAIGoal
    : public CRefCountBase
{
public:
    CAIGoal();
    ~CAIGoal();

    const QString& GetName() { return m_name; }
    void SetName(const QString& name) { m_name = name; }

    //! Get human readable description of this goal.
    const QString& GetDescription() { return m_description; }
    //! Set human readable description of this goal.
    void SetDescription(const QString& desc) { m_description = desc; }

    //////////////////////////////////////////////////////////////////////////
    //! Return true if this goal is Atomic goal, (atomic goals defined by system)
    bool IsAtomic() const { return m_atomic; };
    //! Set this goal as atomic.
    void SetAtomic(bool atomic) { m_atomic = atomic; };

    //! Return true if goal was modified by user and should be stored in goal database.
    bool IsModified() const { return m_modified; };
    // Mark this goal as modified.
    void SetModified(bool modified) { m_modified = modified; }

    //! Get number of stages in goal.
    int GetStageCount() const { return m_stages.size(); };
    //! Get goal stage at specified index.
    CAIGoalStage&   GetStage(int index) { return m_stages[index]; }
    const CAIGoalStage& GetStage(int index) const { return m_stages[index]; }
    void AddStage(const CAIGoalStage& stage) { m_stages.push_back(stage); }

    //! Template for parameters used in goal.
    XmlNodeRef& GetParamsTemplate() { return m_paramsTemplate; };

    //! Serialize Goal to/from xml.
    void Serialize(XmlNodeRef& node, bool bLoading);

private:
    QString m_name;
    QString m_description;

    std::vector<CAIGoalStage> m_stages;
    XmlNodeRef m_attributes;
    //! True if its atomic goal.
    bool m_atomic;
    bool m_modified;

    //! Parameters template for this goal.
    XmlNodeRef m_paramsTemplate;
};

// Define smart pointer to AIGoal
typedef TSmartPtr<CAIGoal> CAIGoalPtr;

#endif // CRYINCLUDE_EDITOR_AI_AIGOAL_H
