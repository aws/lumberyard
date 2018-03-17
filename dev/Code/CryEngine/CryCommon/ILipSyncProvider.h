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

// Description : Interface for the lip-sync provider that gets injected into the IEntitySoundProxy.


#pragma once

#include <IAudioInterfacesCommonData.h>
#include "BoostHelpers.h"
typedef uint32 tSoundID;

enum ELipSyncMethod
{
    eLSM_None,
    eLSM_MatchAnimationToSoundName,
};

using IComponentAudioPtr = std::shared_ptr<struct IComponentAudio>;

struct ILipSyncProvider
{
    virtual ~ILipSyncProvider() {}

    virtual void RequestLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;  // Use this to start loading as soon as the sound gets requested (this can be safely ignored)
    virtual void StartLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
    virtual void PauseLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
    virtual void UnpauseLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
    virtual void StopLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
    virtual void UpdateLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
};

DECLARE_SMART_POINTERS(ILipSyncProvider);

