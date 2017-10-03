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

#include "BaseObject.h"


namespace EMotionFX
{
    class MotionInstance;

    class EMFX_API BlendSpaceParamEvaluator
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceParamEvaluator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        virtual float ComputeParamValue(const MotionInstance& motionInstance) = 0;
        virtual const char* GetName() const = 0;
        virtual bool IsNullEvaluator() const { return false; }
    };

    class EMFX_API BlendSpaceParamEvaluatorNone
        : public BlendSpaceParamEvaluator
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceParamEvaluatorNone, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
        bool IsNullEvaluator() const override { return true; }
    };

    class EMFX_API BlendSpaceMoveSpeedParamEvaluator
        : public BlendSpaceParamEvaluator
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceMoveSpeedParamEvaluator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceTurnSpeedParamEvaluator
        : public BlendSpaceParamEvaluator
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceTurnSpeedParamEvaluator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };


    class EMFX_API BlendSpaceTravelDirectionParamEvaluator
        : public BlendSpaceParamEvaluator
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceTravelDirectionParamEvaluator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };


    class EMFX_API BlendSpaceTravelSlopeParamEvaluator
        : public BlendSpaceParamEvaluator
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceTravelSlopeParamEvaluator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceTurnAngleParamEvaluator
        : public BlendSpaceParamEvaluator
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceTurnAngleParamEvaluator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceTravelDistanceParamEvaluator
        : public BlendSpaceParamEvaluator
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceTravelDistanceParamEvaluator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceLeftRightVelocityParamEvaluator
        : public BlendSpaceParamEvaluator
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceLeftRightVelocityParamEvaluator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceFrontBackVelocityParamEvaluator
        : public BlendSpaceParamEvaluator
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceFrontBackVelocityParamEvaluator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };
} // namespace EMotionFX
