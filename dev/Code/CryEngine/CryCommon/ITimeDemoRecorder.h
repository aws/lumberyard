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

// Description : Provides interface for TimeDemo  CryAction
//               Can attach listener to perform game specific stuff on Record Play

#pragma once

#include <AzCore/EBus/EBus.h>

struct STimeDemoFrameRecord
{
    Vec3 playerPosition;
    Quat playerRotation;
    Quat playerViewRotation;
    Vec3 hmdPositionOffset;
    Quat hmdViewRotation;
};

struct ITimeDemoListener
{
    virtual ~ITimeDemoListener(){}
    virtual void OnRecord(bool bEnable) = 0;
    virtual void OnPlayback(bool bEnable) = 0;
    virtual void OnPlayFrame() = 0;
};



/**
 * ITimeDemoRecorder has been updated to be an ebus to allow it to be deprecated in 1.11 and then removed later.
 * This system has not been heavily tested, and is presumed to not be functional with modern Lumberyard
 * features like the component system. If you use this system, please let Lumberyard support know.
 */
struct ITimeDemoRecorder
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    virtual bool IsRecording() const = 0;
    virtual bool IsPlaying() const = 0;

    virtual void RegisterListener(ITimeDemoListener* pListener) = 0;
    virtual void UnregisterListener(ITimeDemoListener* pListener) = 0;
    virtual void GetCurrentFrameRecord(STimeDemoFrameRecord& record) const = 0;

    virtual void PreUpdate() = 0;
    virtual void PostUpdate() = 0;
    virtual void Reset() = 0;

    virtual bool IsChainLoading() const = 0;
    virtual bool IsTimeDemoActive() const = 0;
    virtual void GetMemoryStatistics(class ICrySizer* pSizer) const = 0;
};

typedef AZ::EBus<ITimeDemoRecorder> TimeDemoRecorderBus;
