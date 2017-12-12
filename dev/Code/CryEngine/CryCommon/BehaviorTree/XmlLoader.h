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

#ifndef CRYINCLUDE_CRYCOMMON_BEHAVIORTREE_XMLLOADER_H
#define CRYINCLUDE_CRYCOMMON_BEHAVIORTREE_XMLLOADER_H
#pragma once

#include "BehaviorTree/IBehaviorTree.h"

namespace BehaviorTree
{
    class XmlLoader
    {
    public:
        XmlNodeRef LoadBehaviorTreeXmlFile(const char* path, const char* name)
        {
            stack_string file;
            file.Format("%s%s.xml", path, name);
            XmlNodeRef behaviorTreeNode = GetISystem()->LoadXmlFromFile(file);
            return behaviorTreeNode;
        }

        INodePtr CreateBehaviorTreeRootNodeFromBehaviorTreeXml(const XmlNodeRef& behaviorTreeXmlNode, const LoadContext& context) const
        {
            XmlNodeRef rootXmlNode = behaviorTreeXmlNode->findChild("Root");
            IF_UNLIKELY (!rootXmlNode)
            {
                gEnv->pLog->LogError("Failed to load behavior tree '%s'. The 'Root' node is missing.", context.treeName);
                return INodePtr();
            }

            return CreateBehaviorTreeNodeFromXml(rootXmlNode, context);
        }

        INodePtr CreateBehaviorTreeNodeFromXml(const XmlNodeRef& xmlNode, const LoadContext& context) const
        {
            if (xmlNode->getChildCount() != 1)
            {
                gEnv->pLog->LogWarning("Failed to load behavior tree '%s'. The node '%s' must contain exactly one child node.", context.treeName, xmlNode->getTag());
                return INodePtr();
            }

            return context.nodeFactory.CreateNodeFromXml(xmlNode->getChild(0), context);
        }
    };
}

#endif // CRYINCLUDE_CRYCOMMON_BEHAVIORTREE_XMLLOADER_H
