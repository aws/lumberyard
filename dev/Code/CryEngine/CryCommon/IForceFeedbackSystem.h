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

// Description : Interface for force feedback system


#ifndef CRYINCLUDE_CRYACTION_IFORCEFEEDBACKSYSTEM_H
#define CRYINCLUDE_CRYACTION_IFORCEFEEDBACKSYSTEM_H
#pragma once

#include <IInput.h>

typedef uint16 ForceFeedbackFxId;
static const ForceFeedbackFxId InvalidForceFeedbackFxId = 0xFFFF;
struct SFFTriggerOutputData;

struct IFFSPopulateCallBack
{
    virtual ~IFFSPopulateCallBack(){}
    // Description:
    //          Callback function to retrieve all effects available
    //          Use it in conjunction with IForceFeedbackSystem::EnumerateEffects
    // See Also:
    //          IForceFeedbackSystem::EnumerateEffects
    //          IForceFeedbackSystem::GetEffectNamesCount
    // Arguments:
    //     pName - Name of one of the effects retrieved
    virtual void AddFFSEffectName(const char* const pName) = 0;
};


struct SForceFeedbackRuntimeParams
{
    SForceFeedbackRuntimeParams()
        : intensity (1.0f)
        , delay(0.0f)
    {
    }

    SForceFeedbackRuntimeParams(float _intensity, float _delay)
        : intensity(_intensity)
        , delay(_delay)
    {
    }

    float intensity;        //Scales overall intensity of the effect (0.0f - 1.0f)
    float delay;                //Start playback delay
};

struct IForceFeedbackSystem
{
    virtual ~IForceFeedbackSystem(){}
    // Description:
    //          Execute a force feedback effect by id
    // See Also:
    //          IForceFeedbackSystem::GetEffectIdByName
    // Arguments:
    //          id - Effect id
    //          runtimeParams - Runtime params for effect, including intensity, delay...
    virtual void PlayForceFeedbackEffect(ForceFeedbackFxId id, const SForceFeedbackRuntimeParams& runtimeParams, EInputDeviceType inputDeviceType) = 0;

    // Description:
    //          Stops an specific effect by id (all running instances, if more than one)
    // See Also:
    //          IForceFeedbackSystem::GetEffectIdByName
    //          IForceFeedbackSystem::StopAllEffects
    // Arguments:
    //     id - Effect id
    virtual void StopForceFeedbackEffect(ForceFeedbackFxId id) = 0;

    // Description:
    //          Returns the internal id of the effect for a given name, if defined
    //          If not found, it will return InvalidForceFeedbackFxId
    // Arguments:
    //     effectName - Name of the effect
    // Return Value:
    //     Effect id for the given name. InvalidForceFeedbackFxId if the effect was not found
    virtual ForceFeedbackFxId GetEffectIdByName(const char* effectName) const = 0;

    // Description:
    //          Stops all running effects
    // See Also:
    //          IForceFeedbackSystem::StopForceFeedbackEffect
    virtual void StopAllEffects() = 0;

    // Description:
    //          This function can be used to request custom frame vibration values for the frame
    //          It can be useful, if a very specific vibration pattern/rules are used
    //          This custom values will be added to any other pre-defined effect running
    // Arguments:
    //          amplifierA - Vibration amount from 0.0 to 1.0 for high frequency motor
    //          amplifierB - Vibration amount from 0.0 to 1.0 for low frequency motor
    virtual void AddFrameCustomForceFeedback(const float amplifierA, const float amplifierB, EInputDeviceType inputDeviceType) = 0;

    // Description:
    //          This function can be used to request custom vibration values for the triggers
    //          It can be useful, if a very specific vibration pattern/rules are used
    //          This custom values will be added to any other pre-defined effect running
    // Arguments:
    //          leftGain - Vibration amount from 0.0 to 1.0 for left trigger motor
    //          rightGain - Vibration amount from 0.0 to 1.0 for right trigger motor
    //          leftEnvelope - Envelope value (uint16) from 0 to 2000 for left trigger motor
    //          rightEnvelope - Envelope value (uint16) from 0 to 2000 for left trigger motor
    virtual void AddCustomTriggerForceFeedback(const SFFTriggerOutputData& triggersData, EInputDeviceType inputDeviceType) = 0;

    // Description:
    //          Use this function to retrieve all effects names available.
    //          pCallback will be used and invoked once for every effect available, passing its name
    // See Also:
    //          IFFSPopulateCallBack::AddFFSEffectName
    // Arguments:
    //     pCallBack - Pointer to object which implements IFFSPopulateCallBack interace
    virtual void EnumerateEffects(IFFSPopulateCallBack* pCallBack) = 0;    // intended to be used only from the editor

    // Description:
    //          Returns the number of effects available
    // See Also:
    //          IFFSPopulateCallBack::AddFFSEffectName
    //          IForceFeedbackSystem::EnumerateEffects
    // Return Value:
    //          Number of effects
    virtual int GetEffectNamesCount() const = 0;

    // Description:
    //          Prevents force feedback effects from starting. Each with bSuppressEffects = true
    //          will increment the lock count, with false will decrement
    virtual void SuppressEffects(bool bSuppressEffects) = 0;
};

#endif // CRYINCLUDE_CRYACTION_IFORCEFEEDBACKSYSTEM_H
