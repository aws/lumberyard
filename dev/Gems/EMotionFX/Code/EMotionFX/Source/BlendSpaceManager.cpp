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

#include <EMotionFX/Source/BlendSpaceManager.h>


namespace EMotionFX
{
    BlendSpaceManager* BlendSpaceManager::Create()
    {
        return new BlendSpaceManager();
    }


    void BlendSpaceManager::RegisterParameterEvaluator(BlendSpaceParamEvaluator* evaluator)
    {
        m_parameterEvaluators.push_back(evaluator);
    }


    void BlendSpaceManager::ClearParameterEvaluators()
    {
        for (BlendSpaceParamEvaluator* evaluator : m_parameterEvaluators)
        {
            evaluator->Destroy();
        }

        m_parameterEvaluators.clear();
    }


    size_t BlendSpaceManager::GetParameterEvaluatorCount() const
    {
        return m_parameterEvaluators.size();
    }


    BlendSpaceParamEvaluator* BlendSpaceManager::GetParameterEvaluator(size_t index) const
    {
        return m_parameterEvaluators[index];
    }


    BlendSpaceParamEvaluator* BlendSpaceManager::FindParameterEvaluatorByName(const AZStd::string& evaluatorName) const
    {
        for (BlendSpaceParamEvaluator* evaluator : m_parameterEvaluators)
        {
            if (evaluatorName == evaluator->GetName())
            {
                return evaluator;
            }
        }

        return nullptr;
    }


    BlendSpaceManager::BlendSpaceManager()
        : BaseObject()
    {
        m_parameterEvaluators.reserve(9);

        // Register standard parameter evaluators.
        RegisterParameterEvaluator(new BlendSpaceParamEvaluatorNone());
        RegisterParameterEvaluator(new BlendSpaceFrontBackVelocityParamEvaluator());
        RegisterParameterEvaluator(new BlendSpaceLeftRightVelocityParamEvaluator());
        RegisterParameterEvaluator(new BlendSpaceMoveSpeedParamEvaluator());
        RegisterParameterEvaluator(new BlendSpaceTravelDirectionParamEvaluator());
        RegisterParameterEvaluator(new BlendSpaceTravelDistanceParamEvaluator());
        RegisterParameterEvaluator(new BlendSpaceTravelSlopeParamEvaluator());
        RegisterParameterEvaluator(new BlendSpaceTurnAngleParamEvaluator());
        RegisterParameterEvaluator(new BlendSpaceTurnSpeedParamEvaluator());
    }


    BlendSpaceManager::~BlendSpaceManager()
    {
        ClearParameterEvaluators();
    }
} // namespace EMotionFX