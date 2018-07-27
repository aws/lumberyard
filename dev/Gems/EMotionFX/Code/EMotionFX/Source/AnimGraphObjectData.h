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

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include <AzCore/Memory/Memory.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AttributeInt32.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeColor.h>
#include <MCore/Source/AttributeQuaternion.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AttributeVector3.h>
#include <MCore/Source/AttributeVector4.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphObject;
    class AnimGraphInstance;
    class AnimGraphNode;
    class AnimGraphPose;

    // implement standard load and save
#define EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE                      \
public:                                                                  \
    uint32 Save(uint8 * outputBuffer) const override             \
    {                                                                    \
        if (outputBuffer) {                                              \
            MCore::MemCopy(outputBuffer, (uint8*)this, sizeof(*this)); } \
        return sizeof(*this);                                            \
    }                                                                    \
                                                                         \
    uint32 Load(const uint8 * dataBuffer) override               \
    {                                                                    \
        if (dataBuffer) {                                                \
            MCore::MemCopy((uint8*)this, dataBuffer, sizeof(*this)); }   \
        return sizeof(*this);                                            \
    }


    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphObjectData
        : public BaseObject
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            FLAGS_HAS_ERROR = 1 << 0
        };

        AnimGraphObjectData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
        virtual ~AnimGraphObjectData();

        static AnimGraphObjectData* Create(AnimGraphObject* object, AnimGraphInstance* animGraphInstance);

        MCORE_INLINE AnimGraphObject* GetObject() const                { return mObject; }
        void SetObject(AnimGraphObject* object)                        { mObject = object; }

        // save and return number of bytes written, when outputBuffer is nullptr only return num bytes it would write
        virtual uint32 Save(uint8* outputBuffer) const;
        virtual uint32 Load(const uint8* dataBuffer);

        virtual void Reset() {}

        MCORE_INLINE uint8 GetObjectFlags() const                       { return mObjectFlags; }
        MCORE_INLINE void SetObjectFlags(uint8 flags)                   { mObjectFlags = flags; }
        MCORE_INLINE void EnableObjectFlags(uint8 flagsToEnable)        { mObjectFlags |= flagsToEnable; }
        MCORE_INLINE void DisableObjectFlags(uint8 flagsToDisable)      { mObjectFlags &= ~flagsToDisable; }
        MCORE_INLINE void SetObjectFlags(uint8 flags, bool enabled)
        {
            if (enabled)
            {
                mObjectFlags |= flags;
            }
            else
            {
                mObjectFlags &= ~flags;
            }
        }
        MCORE_INLINE bool GetIsObjectFlagEnabled(uint8 flag) const      { return (mObjectFlags & flag) != 0; }

        MCORE_INLINE bool GetHasError() const                           { return (mObjectFlags & FLAGS_HAS_ERROR); }
        MCORE_INLINE void SetHasError(bool hasError)                    { SetObjectFlags(FLAGS_HAS_ERROR, hasError); }

    protected:
        AnimGraphObject*    mObject;               /**< Pointer to the object where this data belongs to. */
        AnimGraphInstance*  mAnimGraphInstance;    /**< The animgraph instance where this unique data belongs to. */
        uint8               mObjectFlags;
    };
}   // namespace EMotionFX
