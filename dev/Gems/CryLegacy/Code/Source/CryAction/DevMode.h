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

// Description : Helper class for CCryAction implementing developer mode-only
//               functionality


#ifndef CRYINCLUDE_CRYACTION_DEVMODE_H
#define CRYINCLUDE_CRYACTION_DEVMODE_H
#pragma once

#include "IInput.h"

struct STagFileEntry
{
    Vec3 pos;
    Ang3 ang;
};

class CDevMode
    : public IInputEventListener
    , public IRemoteConsoleListener
{
public:
    CDevMode();
    ~CDevMode();

    void GotoTagPoint(int i);
    void SaveTagPoint(int i);

    // IInputEventListener
    bool OnInputEvent(const SInputEvent&);
    // ~IInputEventListener

    // IRemoteConsoleListener
    virtual void OnGameplayCommand(const char* cmd);
    // ~IRemoteConsoleListener


    void GetMemoryStatistics(ICrySizer* s) { s->Add(*this); }

private:
    bool m_bSlowDownGameSpeed;
    bool m_bHUD;
    std::vector<STagFileEntry> LoadTagFile();
    void SaveTagFile(const std::vector<STagFileEntry>&);
    string TagFileName();
    void SwitchSlowDownGameSpeed();
    void SwitchHUD();
    void GotoSpecialSpawnPoint(int i);
};

#endif // CRYINCLUDE_CRYACTION_DEVMODE_H
