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

// Description : (Interface) Checks the player and other game specific objects' states and communicate them to the TestManager


#ifndef CRYINCLUDE_CRYACTION_TESTSYSTEM_IGAMESTATERECORDER_H
#define CRYINCLUDE_CRYACTION_TESTSYSTEM_IGAMESTATERECORDER_H
#pragma once

typedef enum
{
    GPM_Disabled,
    GPM_SingleActor,
    GPM_AllActors
} EGameProfileMode;


struct IGameplayListener;
struct IEntity;
struct GameplayEvent;

struct IGameStateRecorder
{
    virtual ~IGameStateRecorder(){}
    virtual void Enable(bool bEnable, bool bRecording) = 0;

    virtual void    OnRecordedGameplayEvent(IEntity* pEntity, const GameplayEvent& event, int currentFrame = 0, bool bRecording = false) = 0;
    virtual void RegisterListener(IGameplayListener* pL) = 0;
    virtual void UnRegisterListener(IGameplayListener* pL) = 0;
    virtual float RenderInfo(float y, bool bRecording) = 0;
    virtual void GetMemoryStatistics(ICrySizer* s) = 0;
    virtual void Release() = 0;
    virtual bool IsEnabled() = 0;
    virtual void Update() = 0;
};

#endif // CRYINCLUDE_CRYACTION_TESTSYSTEM_IGAMESTATERECORDER_H
