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

#ifndef CRYINCLUDE_EDITOR_SCRIPT_SCRIPTENVIRONMENT_H
#define CRYINCLUDE_EDITOR_SCRIPT_SCRIPTENVIRONMENT_H
#pragma once


#include <ScriptHelpers.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Script/ScriptSystemComponent.h>


class EditorScriptEnvironment
    : public IEditorNotifyListener
    , public CScriptableBase
{
public:
    EditorScriptEnvironment();
    ~EditorScriptEnvironment();
    // IEditorNotifyListener
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    // ~IEditorNotifyListener

private:
    void RegisterWithScriptSystem();
    void SelectionChanged();

    SmartScriptTable m_selection;
private:
    int Command(IFunctionHandler* pH, const char* commandName);
};


#endif // CRYINCLUDE_EDITOR_SCRIPT_SCRIPTENVIRONMENT_H
