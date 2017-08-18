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

#include "StdAfx.h"
#include "AiGoalLibrary.h"
#include "AiGoal.h"
#include "AiBehaviorLibrary.h"

#include "IAISystem.h"
#include "AIManager.h"

//////////////////////////////////////////////////////////////////////////
// CAIGoalLibrary implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CAIGoalLibrary::CAIGoalLibrary()
{
}

//////////////////////////////////////////////////////////////////////////
void CAIGoalLibrary::InitAtomicGoals()
{
    // Create Atomic goals.
    IAISystem* ai = GetIEditor()->GetAI()->GetAISystem();
    char xmlBuf[32768];
    xmlBuf[0] = 0;
    int num = 0;//ai->EnumAtomicGoals( xmlBuf,sizeof(xmlBuf) );

    XmlNodeRef node = XmlHelpers::LoadXmlFromBuffer(xmlBuf, strlen(xmlBuf));

    //FILE *file = fopen( "c:\\test.xml","wt" );
    //fprintf( file,"%s",xmlBuf );
    //fclose(file);

    if (!node)
    {
        return;
    }

    for (int i = 0; i < node->getChildCount(); i++)
    {
        // Create new atomic goal.
        XmlNodeRef goalNode = node->getChild(i);
        QString goalName = goalNode->getTag();
        QString goalDesc;
        goalNode->getAttr("Description", goalDesc);

        CAIGoalPtr goal = new CAIGoal;
        goal->SetName(goalName);
        goal->SetDescription(goalDesc);
        goal->SetAtomic(true);
        XmlNodeRef params = XmlHelpers::CreateXmlNode("Params");
        for (int j = 0; j < goalNode->getChildCount(); j++)
        {
            params->addChild(goalNode->getChild(j));
        }
        goal->GetParamsTemplate() = params;
        AddGoal(goal);
    }

    CAIGoalPtr goal = new CAIGoal;
    goal->SetName("Attack");
    goal->SetDescription("Attacks enemy");

    CAIGoalStage stage;
    stage.name = "locate";
    stage.blocking = true;
    goal->AddStage(stage);
    stage.name = "approach";
    stage.blocking = true;
    goal->AddStage(stage);
    stage.name = "pathfind";
    stage.blocking = true;
    goal->AddStage(stage);


    //goal->AddStage(
    AddGoal(goal);
}

//////////////////////////////////////////////////////////////////////////
void CAIGoalLibrary::AddGoal(CAIGoal* goal)
{
    CAIGoalPtr g;
    if (m_goals.Find(goal->GetName(), g))
    {
        // Goal with this name already exist in the library.
    }
    m_goals[goal->GetName()] = goal;
}

//////////////////////////////////////////////////////////////////////////
void CAIGoalLibrary::RemoveGoal(CAIGoal* goal)
{
    m_goals.Erase(goal->GetName());
}

//////////////////////////////////////////////////////////////////////////
CAIGoal* CAIGoalLibrary::FindGoal(const QString& name) const
{
    CAIGoalPtr goal = 0;
    m_goals.Find(name, goal);
    return goal;
}

//////////////////////////////////////////////////////////////////////////
void CAIGoalLibrary::ClearGoals()
{
    m_goals.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CAIGoalLibrary::LoadGoals(const QString& path)
{
    // Scan all goal files.
}

//////////////////////////////////////////////////////////////////////////
void CAIGoalLibrary::GetGoals(std::vector<CAIGoalPtr>& goals) const
{
    m_goals.GetAsVector(goals);
}