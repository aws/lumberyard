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

#ifndef __MCOMMON_MANIPULATORCALLBACKS_H
#define __MCOMMON_MANIPULATORCALLBACKS_H

// include the Core system
#include "../EMStudioConfig.h"
#include <EMotionFX/Rendering/Common/TranslateManipulator.h>
#include "../EMStudioManager.h"


namespace EMStudio
{
    /**
     * callback base class used to update actorinstances.
     */
    class TranslateManipulatorCallback
        : public MCommon::ManipulatorCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(TranslateManipulatorCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        /**
         * the constructor.
         */
        TranslateManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const MCore::Vector3& oldValue)
            : ManipulatorCallback(actorInstance, oldValue)
        {
        }

        /**
         * the destructor.
         */
        virtual ~TranslateManipulatorCallback() {}

        /**
         * update the actor instance.
         */
        void Update(const MCore::Vector3& value) override
        {
            ManipulatorCallback::Update(value);

            // update the position, if actorinstance is still valid
            uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
            if (actorInstanceID != MCORE_INVALIDINDEX32)
            {
                mActorInstance->SetLocalPosition(value);
            }
        }

        /**
         * update old transformation values of the callback
         */
        void UpdateOldValues() override
        {
            // update the rotation, if actorinstance is still valid
            uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
            if (actorInstanceID != MCORE_INVALIDINDEX32)
            {
                mOldValueVec = mActorInstance->GetLocalPosition();
            }
        }

        /**
         * apply the transformation.
         */
        void ApplyTransformation() override
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance)
            {
                MCore::Vector3 newPos = actorInstance->GetLocalPosition();
                actorInstance->SetLocalPosition(mOldValueVec);

                if (MCore::Vector3(mOldValueVec - newPos).Length() >= MCore::Math::epsilon)
                {
                    MCore::String outResult;
                    if (GetCommandManager()->ExecuteCommand(MCore::String().Format("AdjustActorInstance -actorInstanceID %i -pos %s", actorInstance->GetID(), MCore::String(newPos).AsChar()).AsChar(), outResult) == false)
                    {
                        MCore::LogError(outResult.AsChar());
                    }
                    else
                    {
                        UpdateOldValues();
                    }
                }
            }
        }

        bool GetResetFollowMode() const override                { return true; }
    protected:
    };


    class RotateManipulatorCallback
        : public MCommon::ManipulatorCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(RotateManipulatorCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        /**
         * the constructor.
         */
        RotateManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const MCore::Vector3& oldValue)
            : ManipulatorCallback(actorInstance, oldValue)
        {
        }

        RotateManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const MCore::Quaternion& oldValue)
            : ManipulatorCallback(actorInstance, oldValue)
        {
        }

        /**
         * the destructor.
         */
        virtual ~RotateManipulatorCallback() {}

        /**
         * update the actor instance.
         */
        void Update(const MCore::Quaternion& value) override
        {
            // update the rotation, if actorinstance is still valid
            uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
            if (actorInstanceID != MCORE_INVALIDINDEX32)
            {
                // temporarily update the actor instance
                mActorInstance->SetLocalRotation((value * mActorInstance->GetLocalRotation()).Normalize());

                // update the callback parent
                ManipulatorCallback::Update(mActorInstance->GetLocalRotation());
            }
        }

        /**
         * update old transformation values of the callback
         */
        void UpdateOldValues() override
        {
            // update the rotation, if actorinstance is still valid
            uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
            if (actorInstanceID != MCORE_INVALIDINDEX32)
            {
                mOldValueQuat = mActorInstance->GetLocalRotation();
            }
        }

        /**
         * apply the transformation.
         */
        void ApplyTransformation() override
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance)
            {
                MCore::Quaternion newRot = actorInstance->GetLocalRotation();
                actorInstance->SetLocalRotation(mOldValueQuat);

                const float dot = newRot.Dot(mOldValueQuat);
                if (dot < 1.0f - MCore::Math::epsilon && dot > -1.0f + MCore::Math::epsilon)
                {
                    MCore::String outResult;
                    if (GetCommandManager()->ExecuteCommand(MCore::String().Format("AdjustActorInstance -actorInstanceID %i -rot \"%s\"", actorInstance->GetID(), MCore::String(AZ::Vector4(newRot.x, newRot.y, newRot.z, newRot.w)).AsChar()).AsChar(), outResult) == false)
                    {
                        MCore::LogError(outResult.AsChar());
                    }
                    else
                    {
                        UpdateOldValues();
                    }
                }
            }
        }
    protected:
    };

    class ScaleManipulatorCallback
        : public MCommon::ManipulatorCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(ScaleManipulatorCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        /**
         * the constructor.
         */
        ScaleManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const MCore::Vector3& oldValue)
            : ManipulatorCallback(actorInstance, oldValue)
        {
        }

        /**
         * the destructor.
         */
        virtual ~ScaleManipulatorCallback() {}

        /**
         * function to get the current value.
         */
        MCore::Vector3 GetCurrValueVec() override
        {
            uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
            if (actorInstanceID != MCORE_INVALIDINDEX32)
            {
                return mActorInstance->GetLocalScale();
            }
            else
            {
                return MCore::Vector3(1.0f, 1.0f, 1.0f);
            }
        }

        /**
         * update the actor instance.
         */
        void Update(const MCore::Vector3& value) override
        {
            // update the position, if actorinstance is still valid
            uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
            if (actorInstanceID != MCORE_INVALIDINDEX32)
            {
                float minScale = 0.001f;
                MCore::Vector3 scale = MCore::Vector3(MCore::Max(mOldValueVec.x * value.x, minScale), MCore::Max(mOldValueVec.y * value.y, minScale), MCore::Max(mOldValueVec.z * value.z, minScale));
                mActorInstance->SetLocalScale(scale);

                // update the callback
                ManipulatorCallback::Update(scale);
            }
        }

        /**
         * update old transformation values of the callback
         */
        void UpdateOldValues() override
        {
            // update the rotation, if actorinstance is still valid
            uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
            if (actorInstanceID != MCORE_INVALIDINDEX32)
            {
                mOldValueVec = mActorInstance->GetLocalScale();
            }
        }

        /**
         * apply the transformation.
         */
        void ApplyTransformation() override
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance)
            {
                MCore::Vector3 newScale = actorInstance->GetLocalScale();
                actorInstance->SetLocalScale(mOldValueVec);

                if (MCore::Vector3(mOldValueVec - newScale).Length() >= MCore::Math::epsilon)
                {
                    MCore::String outResult;
                    if (GetCommandManager()->ExecuteCommand(MCore::String().Format("AdjustActorInstance -actorInstanceID %i -scale %s", actorInstance->GetID(), MCore::String(newScale).AsChar()).AsChar(), outResult) == false)
                    {
                        MCore::LogError(outResult.AsChar());
                    }
                    else
                    {
                        UpdateOldValues();
                    }
                }
            }
        }

        bool GetResetFollowMode() const override                { return true; }
    protected:
    };
} // namespace MCommon


#endif
