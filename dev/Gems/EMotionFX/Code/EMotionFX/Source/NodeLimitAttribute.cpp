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

// include required headers
#include "NodeLimitAttribute.h"


namespace EMotionFX
{
    // constructor
    NodeLimitAttribute::NodeLimitAttribute()
        : NodeAttribute()
    {
        mLimitFlags = 0;
        mTranslationMax.Zero();
        mRotationMax.Zero();
        mScaleMax.Set(1.0f, 1.0f, 1.0f);
    }


    // destructor
    NodeLimitAttribute::~NodeLimitAttribute()
    {
    }


    // create
    NodeLimitAttribute* NodeLimitAttribute::Create()
    {
        return new NodeLimitAttribute();
    }


    // get the type
    uint32 NodeLimitAttribute::GetType() const
    {
        return TYPE_ID;
    }


    // get the type string
    const char* NodeLimitAttribute::GetTypeString() const
    {
        return "NodeLimitAttribute";
    }


    // clone
    NodeAttribute* NodeLimitAttribute::Clone()
    {
        NodeLimitAttribute* clone = new NodeLimitAttribute();

        // copy limits
        clone->mTranslationMin  = mTranslationMin;
        clone->mTranslationMax  = mTranslationMax;
        clone->mRotationMin     = mRotationMin;
        clone->mRotationMax     = mRotationMax;
        clone->mScaleMin        = mScaleMin;
        clone->mScaleMax        = mScaleMax;
        clone->mLimitFlags      = mLimitFlags;

        // return the cloned limit
        return clone;
    }


    // set the minimum translation values, and automatically enable the limit to be true
    void NodeLimitAttribute::SetTranslationMin(const MCore::Vector3& translateMin)
    {
        mTranslationMin = translateMin;
    }


    // set the maximum translation values, and automatically enable the limit to be true
    void NodeLimitAttribute::SetTranslationMax(const MCore::Vector3& translateMax)
    {
        mTranslationMax = translateMax;
    }


    // set the minimum rotation values, and automatically enable the limit to be true
    void NodeLimitAttribute::SetRotationMin(const MCore::Vector3& rotationMin)
    {
        mRotationMin = rotationMin;
    }


    // set the maximum rotation values, and automatically enable the limit to be true
    void NodeLimitAttribute::SetRotationMax(const MCore::Vector3& rotationMax)
    {
        mRotationMax = rotationMax;
    }


    // set the minimum scale values, and automatically enable the limit to be true
    void NodeLimitAttribute::SetScaleMin(const MCore::Vector3& scaleMin)
    {
        mScaleMin = scaleMin;
    }

    // set the maximum scale values, and automatically enable the limit to be true
    void NodeLimitAttribute::SetScaleMax(const MCore::Vector3& scaleMax)
    {
        mScaleMax = scaleMax;
    }


    // get the minimum translation values
    const MCore::Vector3& NodeLimitAttribute::GetTranslationMin() const
    {
        return mTranslationMin;
    }


    // get the maximum translation values
    const MCore::Vector3& NodeLimitAttribute::GetTranslationMax() const
    {
        return mTranslationMax;
    }


    // get the minimum rotation values
    const MCore::Vector3& NodeLimitAttribute::GetRotationMin() const
    {
        return mRotationMin;
    }


    // get the maximum rotation values
    const MCore::Vector3& NodeLimitAttribute::GetRotationMax() const
    {
        return mRotationMax;
    }


    // get the minimum scale values
    const MCore::Vector3& NodeLimitAttribute::GetScaleMin() const
    {
        return mScaleMin;
    }


    // get the maximum scale values
    const MCore::Vector3& NodeLimitAttribute::GetScaleMax() const
    {
        return mScaleMax;
    }


    // enable or disable the limit for the specified limit type
    void NodeLimitAttribute::EnableLimit(ELimitType limitType, bool enable)
    {
        if (enable)
        {
            mLimitFlags |= limitType;
        }
        else
        {
            mLimitFlags &= ~limitType;
        }
    }


    // toggle limit state
    void NodeLimitAttribute::ToggleLimit(ELimitType limitType)
    {
        const bool enable = !GetIsLimited(limitType);
        EnableLimit(limitType, enable);
    }


    // determine if the specified limit is enabled or disabled
    bool NodeLimitAttribute::GetIsLimited(ELimitType limitType) const
    {
        return (mLimitFlags & limitType) != 0;
    }
}   // namespace EMotionFX


