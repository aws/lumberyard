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

#ifndef CRYINCLUDE_CRYCOMMON_IFLOWINITMANAGER_H
#define CRYINCLUDE_CRYCOMMON_IFLOWINITMANAGER_H
#pragma once

/*
    User facing interface for a class intended to load and maintain a registry of
    flow node types and user defined customization data
*/
class IFlowInitManager
{
public:
    virtual ~IFlowInitManager() {}

    // Load all data files
    virtual void LoadInitData() = 0;

    // Accept notification of an engine defined node type
    virtual void RegisterNodeInfo(const char* type) = 0;

    // UI Name/Class Name management
    virtual const AZStd::string& GetClassTagFromUIName(const AZStd::string& uiName) const = 0;
    virtual const AZStd::string& GetUINameFromClassTag(const AZStd::string& classTag) const = 0;
};

#endif