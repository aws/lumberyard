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

#include "CryLegacy_precompiled.h"

#include "FlowInitManager.h"

#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/StringFunc/StringFunc.h>


#define FLOW_INIT_DIR "Libs/FlowNodes/FlowInitData/"

FlowInitNodeData::FlowInitNodeData(const rapidjson::Value& initData)
    : m_nodeRegistered(false)                                                                  // False until the client registers it (The file could contain stale data)
{
    auto valueIter = initData.FindMember("ClassTag");

    if (valueIter != initData.MemberEnd() && valueIter->value.IsString())
    {
        m_classTag = valueIter->value.GetString();
    }

    valueIter = initData.FindMember("UIName");

    if (valueIter != initData.MemberEnd() && valueIter->value.IsString())
    {
        m_uiName = valueIter->value.GetString();
    }
}

FlowInitNodeData::FlowInitNodeData(const char* classTag)
    : m_classTag(classTag)
    , m_nodeRegistered(false)
{
}

void FlowInitManager::LoadInitData()
{
    m_initNodeData.clear();

    if (gEnv->pCryPak)
    {
        AZStd::string searchPath(FLOW_INIT_DIR);

        _finddata_t fd;
        intptr_t handle = gEnv->pCryPak->FindFirst((searchPath + "*.json").c_str(), &fd);

        if (handle < 0)
        {
            return;
        }

        do
        {
            // Go through each file in the directory and add the data to our initializations
            // Duplicate entries will overwrite previous data
            AZ::IO::HandleType readHandle = gEnv->pCryPak->FOpen((searchPath + fd.name).c_str(), "rt");

            if (readHandle != AZ::IO::InvalidHandle)
            {
                size_t fileSize = gEnv->pCryPak->FGetSize(readHandle);
                if (fileSize > 0)
                {
                    AZStd::string fileBuf;
                    fileBuf.resize(fileSize);

                    size_t read = gEnv->pCryPak->FRead(fileBuf.data(), fileSize, readHandle);
                    gEnv->pCryPak->FClose(readHandle);

                    rapidjson::Document parseDoc;
                    parseDoc.Parse<rapidjson::kParseStopWhenDoneFlag>(fileBuf.data());

                    if (parseDoc.HasParseError())
                    {
                        continue;
                    }

                    const rapidjson::Value& nodesList = parseDoc["Nodes"];
                    if (!nodesList.IsArray())
                    {
                        continue;
                    }

                    for (rapidjson::SizeType nodeCount = 0; nodeCount < nodesList.Size(); ++nodeCount)
                    {
                        const rapidjson::Value& thisNode = nodesList[nodeCount];

                        const rapidjson::Value& classTag = thisNode["ClassTag"];
                        if (classTag.IsString())
                        {
                            m_initNodeData[classTag.GetString()] = AZStd::make_shared<FlowInitNodeData>(thisNode);
                        }
                    }
                }
            }
        } while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);
        gEnv->pCryPak->FindClose(handle);
    }
    RegisterAllUINames();
}

InitNodePtr FlowInitManager::GetNode(const AZStd::string& classTag) const
{
    InitNodeMap::const_iterator nodeIter = m_initNodeData.find(classTag);

    if (nodeIter != m_initNodeData.end())
    {
        return nodeIter->second;
    }
    return InitNodePtr {};
}

void FlowInitManager::RegisterAllUINames()
{
    m_uiNameToClassTag.clear();

    for (const auto& thisElement : m_initNodeData)
    {
        if (thisElement.second)
        {
            SetClassTagForUIName(thisElement.second->GetUIName(), thisElement.first);
        }
    }
}

void FlowInitManager::RegisterNodeInfo(const char* type)
{
    InitNodePtr thisNode = GetNode(AZStd::string(type));

    if (thisNode)
    {
        thisNode->SetRegistered(true);
    }
}

// This maintains our 1 to 1 relationship of UI name to class name to handle requests from our various interfaces
// which currently talk to us in terms of UI names.  Plan for the future is to convert existing systems to make requests
// in terms of class names.
void FlowInitManager::SetClassTagForUIName(const AZStd::string& uiName, const AZStd::string& className)
{
    if (!uiName.length())
    {
        return;
    }

    m_uiNameToClassTag[uiName] = className;

    // These should never be out of sync currently
    // An implementation where many UI names pointed to one class name is possible
    // but would probably not be maintainable long term
    InitNodePtr thisNode = GetNode(className);
    if (!thisNode)
    {
        return;
    }
    thisNode->SetUIName(uiName.c_str());
}

//////////////////////////////////////////////////////////////////////////
const AZStd::string& FlowInitManager::GetClassTagFromUIName(const AZStd::string& uiName) const
{
    ClassTagToUINameMap::const_iterator nameIter = m_uiNameToClassTag.find(uiName);
    if (nameIter != m_uiNameToClassTag.end())
    {
        return nameIter->second;
    }
    return uiName;
}

const AZStd::string& FlowInitManager::GetUINameFromClassTag(const AZStd::string& classTag) const
{
    InitNodePtr thisNode = GetNode(classTag);

    if (thisNode)
    {
        if (thisNode->GetUIName().length())
        {
            return thisNode->GetUIName();
        }
    }
    return classTag;
}
