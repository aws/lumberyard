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

// Description : Interface of the DialogSystem [mainly for Editor->CryAction]


#ifndef CRYINCLUDE_CRYCOMMON_IDIALOGSYSTEM_H
#define CRYINCLUDE_CRYCOMMON_IDIALOGSYSTEM_H
#pragma once

struct IDialogScriptIterator
{
    struct SDialogScript
    {
        const char* id;
        const char* desc;
        int numRequiredActors;
        int numLines;
        bool bIsLegacyExcel;
    };
    // <interfuscator:shuffle>
    virtual ~IDialogScriptIterator(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual bool Next(SDialogScript&) = 0;
    // </interfuscator:shuffle>
};
typedef _smart_ptr<IDialogScriptIterator> IDialogScriptIteratorPtr;


struct IDialogSystem
{
    // <interfuscator:shuffle>
    virtual ~IDialogSystem(){}
    virtual bool Init() = 0;
    virtual void Update(const float dt) = 0;
    virtual void Reset(bool bUnload) = 0;
    virtual bool ReloadScripts(const char* levelName) = 0;
    virtual IDialogScriptIteratorPtr CreateScriptIterator() = 0;

    // Returns if entity is currently playing a dialog
    virtual bool IsEntityInDialog(EntityId entityId) const = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IDIALOGSYSTEM_H
