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


#ifndef CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGLOADER_H
#define CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGLOADER_H
#pragma once

#include "DialogScript.h"

class CDialogSystem;

class CDialogLoader
{
public:
    CDialogLoader(CDialogSystem* pDS);
    virtual ~CDialogLoader();

    // Loads all DialogScripts below a given path
    bool LoadScriptsFromPath(const string& path, TDialogScriptMap& outScripts);

    // Loads a single DialogScript
    bool LoadScript(const string& fileName, TDialogScriptMap& outScripts);

protected:

    // some intermediate data structure
    struct IMScriptLine
    {
        IMScriptLine() { Reset(); }
        void Reset()
        {
            audioID = INVALID_AUDIO_CONTROL_ID;
            actor = anim = facial = lookat = "";
            delay = facialFadeTime = 0.0f;
            facialWeight = 0.5f;
        }

        bool IsValid() const { return (actor != 0 && *actor != 0); }

        const char* actor;
        //const char* sound;
        Audio::TAudioControlID audioID;
        const char* anim;
        const char* lookat;
        float       delay;
        string      facial;
        float       facialWeight;
        float       facialFadeTime;
    };

    struct IMScript
    {
        IMScript() { Reset(); }
        void Reset() { name = 0; lines.resize(0); }
        bool IsValid() const { return (name != 0 && *name != 0 && lines.size() > 0); }
        const char* name;
        std::vector<IMScriptLine> lines;
    };

    // returns the numer of successfully loaded scripts
    int  LoadFromTable(XmlNodeRef tableNode, const string& fileName, TDialogScriptMap& outScriptMap);

    // preprocessing stuff
    bool ProcessScript(IMScript& theScript, const string& groupName, TDialogScriptMap& outScriptMap);

    // get actor from string [1-based]
    bool GetActor(const char* actor, int& outID);
    bool GetLookAtActor(const char* actor, int& outID, bool& outSticky);

protected:

    CDialogSystem* m_pDS;
};

#endif // CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGLOADER_H
