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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/BlendSpaceParamEvaluator.h>


namespace EMotionFX
{
    class EMFX_API BlendSpaceManager
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceManager, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);

    public:
        static BlendSpaceManager* Create();

        void RegisterParameterEvaluator(BlendSpaceParamEvaluator* evaluator);
        void ClearParameterEvaluators();

        size_t GetParameterEvaluatorCount() const;
        BlendSpaceParamEvaluator* GetParameterEvaluator(size_t index) const;
        BlendSpaceParamEvaluator* FindParameterEvaluatorByName(const AZStd::string& evaluatorName) const;

    private:
        BlendSpaceManager();
        ~BlendSpaceManager();

    private:
        AZStd::vector<BlendSpaceParamEvaluator*> m_parameterEvaluators;
    };
} // namespace EMotionFX