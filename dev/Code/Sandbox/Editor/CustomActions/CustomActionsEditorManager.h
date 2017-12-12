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

#ifndef CRYINCLUDE_EDITOR_CUSTOMACTIONS_CUSTOMACTIONSEDITORMANAGER_H
#define CRYINCLUDE_EDITOR_CUSTOMACTIONS_CUSTOMACTIONSEDITORMANAGER_H
#pragma once


// Forward declarations.

//////////////////////////////////////////////////////////////////////////
// Custom Actions Editor Manager
//////////////////////////////////////////////////////////////////////////
class CCustomActionsEditorManager
{
public:
    CCustomActionsEditorManager();
    ~CCustomActionsEditorManager();
    void Init(ISystem* system);

    void ReloadCustomActionGraphs();
    void SaveCustomActionGraphs();
    void SaveAndReloadCustomActionGraphs();
    bool NewCustomAction(QString& filename);
    void GetCustomActions(QStringList& values) const;

private:
    void LoadCustomActionGraphs();
    void FreeCustomActionGraphs();
};

#endif // CRYINCLUDE_EDITOR_CUSTOMACTIONS_CUSTOMACTIONSEDITORMANAGER_H
