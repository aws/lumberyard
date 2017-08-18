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
#include "BehaviorTreeDocument.h"

#include <BehaviorTree/XmlLoader.h>

BehaviorTreeDocument::BehaviorTreeDocument()
    : m_changed(false)
{
}

void BehaviorTreeDocument::Reset()
{
    m_changed = false;
    m_behaviorTreeName.clear();
    m_absoluteFilePath.clear();
    m_behaviorTreeTemplate.reset();
}

bool BehaviorTreeDocument::Loaded()
{
    return m_behaviorTreeTemplate != NULL;
}

bool BehaviorTreeDocument::Changed()
{
    return m_changed;
}

void BehaviorTreeDocument::SetChanged()
{
    m_changed = true;
}

void BehaviorTreeDocument::Serialize(Serialization::IArchive& archive)
{
    if (!m_behaviorTreeTemplate)
    {
        return;
    }

    archive(m_behaviorTreeName, "name", "!Tree Name");

#ifdef USING_BEHAVIOR_TREE_SERIALIZATION

#ifdef USING_VARIABLE_COLLECTION_SERIALIZATION
    archive(m_behaviorTreeTemplate->variableDeclarations, "variableDeclarations", "Variables");
    Serialization::SContext<Variables::Declarations> variableDeclarations(archive, &m_behaviorTreeTemplate->variableDeclarations);
    archive(m_behaviorTreeTemplate->signalHandler, "signalHandler", "Event handles");
#endif
    archive(m_behaviorTreeTemplate->defaultTimestampCollection, "TimestampCollection", "Timestamps");
    archive(m_behaviorTreeTemplate->rootNode, "root", "=<>+Tree root");
    if (!m_behaviorTreeTemplate->rootNode.get())
    {
        archive.Error(m_behaviorTreeTemplate->rootNode, "Node must be specified");
    }
#endif
}

void BehaviorTreeDocument::NewFile(const char* behaviorTreeName, const char* absoluteFilePath)
{
    Reset();
    m_changed = true;
    m_behaviorTreeName = behaviorTreeName;
    m_absoluteFilePath = absoluteFilePath;
    m_behaviorTreeTemplate = BehaviorTree::BehaviorTreeTemplatePtr(new BehaviorTree::BehaviorTreeTemplate());
}

bool BehaviorTreeDocument::OpenFile(const char* behaviorTreeName, const char* absoluteFilePath)
{
    XmlNodeRef behaviorTreeXmlFile = GetISystem()->LoadXmlFromFile(absoluteFilePath);
    if (!behaviorTreeXmlFile)
    {
        return false;
    }

    Reset();

    BehaviorTree::BehaviorTreeTemplatePtr newBehaviorTreeTemplate = BehaviorTree::BehaviorTreeTemplatePtr(new BehaviorTree::BehaviorTreeTemplate());

    if (XmlNodeRef variablesXml = behaviorTreeXmlFile->findChild("Variables"))
    {
        if (!newBehaviorTreeTemplate->variableDeclarations.LoadFromXML(variablesXml, behaviorTreeName))
        {
            return false;
        }
    }

    if (XmlNodeRef signalVariablesXml = behaviorTreeXmlFile->findChild("SignalVariables"))
    {
        if (!newBehaviorTreeTemplate->signalHandler.LoadFromXML(newBehaviorTreeTemplate->variableDeclarations, signalVariablesXml, behaviorTreeName))
        {
            return false;
        }
    }

    if (XmlNodeRef timestampsXml = behaviorTreeXmlFile->findChild("Timestamps"))
    {
        newBehaviorTreeTemplate->defaultTimestampCollection.LoadFromXml(timestampsXml);
    }

    BehaviorTree::INodeFactory& factory = gEnv->pAISystem->GetIBehaviorTreeManager()->GetNodeFactory();
    BehaviorTree::LoadContext context(factory, behaviorTreeName, newBehaviorTreeTemplate->variableDeclarations);
    newBehaviorTreeTemplate->rootNode = BehaviorTree::XmlLoader().CreateBehaviorTreeRootNodeFromBehaviorTreeXml(behaviorTreeXmlFile, context);
    if (!newBehaviorTreeTemplate->rootNode.get())
    {
        return false;
    }

    m_changed = false;
    m_behaviorTreeName = behaviorTreeName;
    m_absoluteFilePath = absoluteFilePath;
    m_behaviorTreeTemplate = newBehaviorTreeTemplate;

    return true;
}

bool BehaviorTreeDocument::Save()
{
    return SaveToFile(m_behaviorTreeName, m_absoluteFilePath);
}

bool BehaviorTreeDocument::SaveToFile(const char* behaviorTreeName, const char* absoluteFilePath)
{
    XmlNodeRef behaviorTreeXml = GenerateBehaviorTreeXml();
    if (!behaviorTreeXml)
    {
        return false;
    }

    if (!behaviorTreeXml->saveToFile(absoluteFilePath))
    {
        return false;
    }

    m_changed = false;
    m_behaviorTreeName = behaviorTreeName;
    m_absoluteFilePath = absoluteFilePath;

    return true;
}

XmlNodeRef BehaviorTreeDocument::GenerateBehaviorTreeXml()
{
    if (!m_behaviorTreeTemplate && !m_behaviorTreeTemplate->rootNode)
    {
        return XmlNodeRef();
    }

    XmlNodeRef behaviorTreeXml = GetISystem()->CreateXmlNode("BehaviorTree");

#ifdef USING_BEHAVIOR_TREE_XML_DESCRIPTION_CREATION

#ifdef USING_VARIABLE_COLLECTION_XML_DESCRIPTION_CREATION
    if (XmlNodeRef variablesXml = m_behaviorTreeTemplate->variableDeclarations.CreateXmlDescription())
    {
        behaviorTreeXml->addChild(variablesXml);
    }

    if (XmlNodeRef signalHandlerXml = m_behaviorTreeTemplate->signalHandler.CreateXmlDescription())
    {
        behaviorTreeXml->addChild(signalHandlerXml);
    }
#endif

    if (XmlNodeRef timestampsXml = m_behaviorTreeTemplate->defaultTimestampCollection.CreateXmlDescription())
    {
        behaviorTreeXml->addChild(timestampsXml);
    }

    XmlNodeRef rootXml = behaviorTreeXml->newChild("Root");
    if (m_behaviorTreeTemplate->rootNode)
    {
        XmlNodeRef rootFirstNodeXml = m_behaviorTreeTemplate->rootNode->CreateXmlDescription();
        if (!rootFirstNodeXml)
        {
            return XmlNodeRef();
        }

        rootXml->addChild(rootFirstNodeXml);
    }
    else
    {
        return XmlNodeRef();
    }
#endif

    return behaviorTreeXml;
}