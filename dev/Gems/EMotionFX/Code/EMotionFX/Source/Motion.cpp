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

// include the required headers
#include "EMotionFXConfig.h"
#include "Motion.h"
#include <MCore/Source/IDGenerator.h>
#include "Pose.h"
#include "MotionInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "MotionManager.h"
#include "EventManager.h"
#include "EventHandler.h"
#include "MotionEventTable.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Motion, MotionAllocator, 0)


    // constructor
    Motion::Motion(const char* name)
        : BaseObject()
    {
        mCustomData             = nullptr;
        mDefaultPlayBackInfo    = nullptr;
        mNameID                 = MCORE_INVALIDINDEX32;
        mID                     = MCore::GetIDGenerator().GenerateID();
        mEventTable             = MotionEventTable::Create();
        mUnitType               = GetEMotionFX().GetUnitType();
        mFileUnitType           = mUnitType;
        mExtractionFlags        = static_cast<EMotionExtractionFlags>(0);

        if (name)
        {
            SetName(name);
        }

        mMaxTime                = 0.0f;
        mMotionFPS              = 30.0f;
        mDirtyFlag              = false;
        mAutoUnregister         = true;

#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime       = false;
#endif // EMFX_DEVELOPMENT_BUILD

        // automatically register the motion
        GetMotionManager().AddMotion(this);
    }


    // destructor
    Motion::~Motion()
    {
        // trigger the OnDeleteMotion event
        GetEventManager().OnDeleteMotion(this);

        // automatically unregister the motion
        if (mAutoUnregister)
        {
            GetMotionManager().RemoveMotion(this, false);
        }

        // get rid of the default playback info
        delete mDefaultPlayBackInfo;

        if (mEventTable)
        {
            mEventTable->Destroy();
        }
    }


    // set the name of the motion
    void Motion::SetName(const char* name)
    {
        // calculate the ID
        mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // set the filename of the motion
    void Motion::SetFileName(const char* filename)
    {
        mFileName = filename;
    }


    // set the maximum time of the motion
    void Motion::SetMaxTime(float maxTime)
    {
        mMaxTime = maxTime;
    }


    // adjust the dirty flag
    void Motion::SetDirtyFlag(bool dirty)
    {
        mDirtyFlag = dirty;
    }


    // adjust the auto unregistering from the motion manager on delete
    void Motion::SetAutoUnregister(bool enabled)
    {
        mAutoUnregister = enabled;
    }


    // do we auto unregister from the motion manager on delete?
    bool Motion::GetAutoUnregister() const
    {
        return mAutoUnregister;
    }

    void Motion::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime = isOwnedByRuntime;
#endif
    }


    bool Motion::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return mIsOwnedByRuntime;
#else
        return true;
#endif
    }


    const char* Motion::GetName() const
    {
        return MCore::GetStringIdPool().GetName(mNameID).c_str();
    }


    const AZStd::string& Motion::GetNameString() const
    {
        return MCore::GetStringIdPool().GetName(mNameID);
    }


    float Motion::GetMaxTime() const
    {
        return mMaxTime;
    }


    void Motion::SetMotionFPS(float motionFPS)
    {
        mMotionFPS = motionFPS;
    }


    float Motion::GetMotionFPS() const
    {
        return mMotionFPS;
    }


    // allocate memory for the default playback info
    void Motion::CreateDefaultPlayBackInfo()
    {
        if (mDefaultPlayBackInfo == nullptr)
        {
            mDefaultPlayBackInfo = new PlayBackInfo();
        }
    }


    // set the default playback info to the given playback info
    void Motion::SetDefaultPlayBackInfo(const PlayBackInfo& playBackInfo)
    {
        CreateDefaultPlayBackInfo();
        *mDefaultPlayBackInfo = playBackInfo;
    }


    // get a pointer to the default playback info
    PlayBackInfo* Motion::GetDefaultPlayBackInfo() const
    {
        return mDefaultPlayBackInfo;
    }


    bool Motion::GetDirtyFlag() const
    {
        return mDirtyFlag;
    }


    void Motion::SetMotionExtractionFlags(EMotionExtractionFlags flags)
    {
        mExtractionFlags = flags;
    }


    EMotionExtractionFlags Motion::GetMotionExtractionFlags() const
    {
        return mExtractionFlags;
    }


    void Motion::SetCustomData(void* dataPointer)
    {
        mCustomData = dataPointer;
    }


    void* Motion::GetCustomData() const
    {
        return mCustomData;
    }


    MotionEventTable* Motion::GetEventTable() const
    {
        return mEventTable;
    }


    void Motion::SetID(uint32 id)
    {
        mID = id;
    }

    const char* Motion::GetFileName() const
    {
        return mFileName.c_str();
    }


    const AZStd::string& Motion::GetFileNameString() const
    {
        return mFileName;
    }


    uint32 Motion::GetID() const
    {
        return mID;
    }


    void Motion::SetUnitType(MCore::Distance::EUnitType unitType)
    {
        mUnitType = unitType;
    }


    MCore::Distance::EUnitType Motion::GetUnitType() const
    {
        return mUnitType;
    }


    void Motion::SetFileUnitType(MCore::Distance::EUnitType unitType)
    {
        mFileUnitType = unitType;
    }


    MCore::Distance::EUnitType Motion::GetFileUnitType() const
    {
        return mFileUnitType;
    }



    // scale everything to the given unit type
    void Motion::ScaleToUnitType(MCore::Distance::EUnitType targetUnitType)
    {
        if (mUnitType == targetUnitType)
        {
            return;
        }

        // calculate the scale factor and scale
        const float scaleFactor = static_cast<float>(MCore::Distance::GetConversionFactor(mUnitType, targetUnitType));
        Scale(scaleFactor);

        // update the unit type
        mUnitType = targetUnitType;
    }
} // namespace EMotionFX
