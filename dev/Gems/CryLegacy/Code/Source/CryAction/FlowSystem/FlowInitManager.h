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

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWINITMANAGER_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWINITMANAGER_H
#pragma once

#include <IFlowInitManager.h>

#include <AzCore/JSON/document.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>

// Flow node init data created from JSON
class FlowInitNodeData
{
public:
    FlowInitNodeData(const rapidjson::Value& initData);
    FlowInitNodeData(const char* classTag);

    const AZStd::string& GetClassTag() const { return m_classTag; }

    const AZStd::string& GetUIName() const { return m_uiName; }
    void SetUIName(const char* uiName) { m_uiName = uiName; }

    bool GetRegistered() const { return m_nodeRegistered; }
    void SetRegistered(bool isRegistered) { m_nodeRegistered = isRegistered; }

private:
    AZStd::string m_classTag;
    AZStd::string m_uiName;
    bool m_nodeRegistered;
};

using InitNodePtr = AZStd::shared_ptr<FlowInitNodeData>;
using InitNodeMap = AZStd::unordered_map<AZStd::string, InitNodePtr>;
using ClassTagToUINameMap = AZStd::unordered_map<AZStd::string, AZStd::string>;

/*
Internal interface for a class intended to load and maintain a registry of
flow node types and user defined customization data.  Loads a JSON file
containing configuration and initialization data such as UI name overrides
*/
class FlowInitManager
    : public IFlowInitManager
{
public:
    virtual ~FlowInitManager() {}

    InitNodePtr GetNode(const AZStd::string& classTag) const;

    // Load all initialization files
    virtual void LoadInitData() override final;

    // Accept notification of an engine defined node type
    virtual void RegisterNodeInfo(const char* type) override final;

    // UI Name/ClassTag management
    virtual const AZStd::string& GetClassTagFromUIName(const AZStd::string& uiName) const override final;
    virtual const AZStd::string& GetUINameFromClassTag(const AZStd::string& uiName) const override final;
private:

    // Go through our init node data map and register each pair to our uiNameToClassTag map
    void RegisterAllUINames();
    // Add an individual uiName/classTag pair
    virtual void SetClassTagForUIName(const AZStd::string& uiName, const AZStd::string& classTag);

    // All of our internal node config data
    InitNodeMap m_initNodeData;

    // Ui
    ClassTagToUINameMap m_uiNameToClassTag;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWINITMANAGER_H