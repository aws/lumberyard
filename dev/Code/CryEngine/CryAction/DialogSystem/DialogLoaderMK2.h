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

// Description : Dialog Loader


#ifndef CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGLOADERMK2_H
#define CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGLOADERMK2_H
#pragma once

#include "DialogScript.h"

class CDialogSystem;

class CDialogLoaderMK2
{
public:
    CDialogLoaderMK2(CDialogSystem* pDS);
    virtual ~CDialogLoaderMK2();

    // Loads all DialogScripts below a given path
    bool LoadScriptsFromPath(const string& path, TDialogScriptMap& outScripts, const char* levelName);

    // Loads a single DialogScript
    bool LoadScript(const string& stripPath, const string& fileName, TDialogScriptMap& outScripts);

protected:
    void InternalLoadFromPath(const string& stripPath, const string& path, TDialogScriptMap& outScriptMap, int& numLoaded, const char* levelName);

    // get actor from string [1-based]
    bool GetActor(const char* actor, int& outID);
    bool GetLookAtActor(const char* actor, int& outID, bool& outSticky);
    bool ProcessScript(CDialogScript* pScript, const XmlNodeRef& rootNode);
    bool ReadLine(const XmlNodeRef& lineNode, CDialogScript::SScriptLine& outLine, const char* scriptID, int lineNumber);
    void ResetLine(CDialogScript::SScriptLine& scriptLine);

protected:

    CDialogSystem* m_pDS;
};

#endif // CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGLOADERMK2_H
