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

#ifndef CRYINCLUDE_CRYCOMMON_ISELECTIONTREEMANAGER_H
#define CRYINCLUDE_CRYCOMMON_ISELECTIONTREEMANAGER_H
#pragma once


#pragma  once

struct ISelectionTreeObserver
{
    virtual ~ISelectionTreeObserver() {}

    typedef std::map<string, bool> TVariableStateMap;
    struct STreeNodeInfo
    {
        uint16 Id;
        typedef std::vector<STreeNodeInfo> TChildList;
        TChildList Childs;
    };

    virtual void SetSelectionTree(const char* name, const STreeNodeInfo& rootNode) = 0;
    virtual void DumpVars(const TVariableStateMap& vars) = 0;
    virtual void StartEval() = 0;
    virtual void EvalNode(uint16 nodeId) = 0;
    virtual void EvalNodeCondition(uint16 nodeId, bool condition) = 0;
    virtual void EvalStateCondition(uint16 nodeId, bool condition) = 0;
    virtual void StopEval(uint16 nodeId) = 0;
};

struct ISelectionTreeDebugger
{
    virtual ~ISelectionTreeDebugger(){}

    virtual void SetObserver(ISelectionTreeObserver* pListener, const char* nameAI) = 0;
};

struct ISelectionTreeManager
{
    // <interfuscator:shuffle>
    virtual ~ISelectionTreeManager(){}
    virtual uint32 GetSelectionTreeCount() const = 0;
    virtual uint32 GetSelectionTreeCountOfType(const char* typeName) const = 0;

    virtual const char* GetSelectionTreeName(uint32 index) const = 0;
    virtual const char* GetSelectionTreeNameOfType(const char* typeName, uint32 index) const = 0;

    virtual void Reload() = 0;

    virtual ISelectionTreeDebugger* GetDebugger() const = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ISELECTIONTREEMANAGER_H
