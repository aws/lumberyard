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
#include "AiGoal.h"

//////////////////////////////////////////////////////////////////////////
// CAIgoal implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CAIGoal::CAIGoal()
{
    m_atomic = false;
    m_modified = false;
}

//////////////////////////////////////////////////////////////////////////
CAIGoal::~CAIGoal()
{
    m_atomic = false;
}

//////////////////////////////////////////////////////////////////////////
void CAIGoal::Serialize(XmlNodeRef& node, bool bLoading)
{
    if (bLoading)
    {
        m_stages.clear();

        // Loading.
        node->getAttr("Name", m_name);

        m_stages.resize(node->getChildCount());
        for (int i = 0; i < node->getChildCount(); i++)
        {
            // Write goals stages to xml.
            CAIGoalStage& stage = m_stages[i];
            XmlNodeRef stageNode = node->getChild(i);
            stageNode->getAttr("Blocking", stage.blocking);
            stage.params->copyAttributes(stageNode);
            stage.params->delAttr("Blocking");
        }
    }
    else
    {
        // Saving.
        node->setAttr("Name", m_name.toUtf8().data());

        for (int i = 0; i < m_stages.size(); i++)
        {
            // Write goals stages to xml.
            CAIGoalStage& stage = m_stages[i];
            XmlNodeRef stageNode = node->newChild(stage.name.toUtf8().data());
            stageNode->copyAttributes(stage.params);
            stageNode->setAttr("Blocking", stage.blocking);
        }
    }
}