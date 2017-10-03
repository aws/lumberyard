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

#include "EMotionFXConfig.h"
#include <AzCore/std/sort.h>
#include "AnimGraphParameterGroup.h"
#include "AnimGraphManager.h"
#include "AnimGraphInstance.h"


namespace EMotionFX
{
    AnimGraphParameterGroup::AnimGraphParameterGroup(const char* name)
        : BaseObject()
        , m_name(name)
        , m_isCollapsed(false)
    {
    }


    AnimGraphParameterGroup::~AnimGraphParameterGroup()
    {
        RemoveAllParameters();
    }


    AnimGraphParameterGroup* AnimGraphParameterGroup::Create(const char* name)
    {
        return new AnimGraphParameterGroup(name);
    }


    const char* AnimGraphParameterGroup::GetName() const
    {
        return m_name.c_str();
    }


    const AZStd::string& AnimGraphParameterGroup::GetNameString() const
    {
        return m_name;
    }


    void AnimGraphParameterGroup::AddParameter(uint32 parameterIndex)
    {
        if (AZStd::find(m_parameterIndices.begin(), m_parameterIndices.end(), parameterIndex) != m_parameterIndices.end())
        {
            AZ_Assert(false, "Parameter %d already part of parameter group '%s'.", m_name.c_str());
            return;
        }

        m_parameterIndices.push_back(parameterIndex);
        AZStd::sort(m_parameterIndices.begin(), m_parameterIndices.end());
    }


    uint32 AnimGraphParameterGroup::GetNumParameters() const
    {
        return static_cast<uint32>(m_parameterIndices.size());
    }


    size_t AnimGraphParameterGroup::GetParameterCount() const
    {
        return m_parameterIndices.size();
    }


    uint32 AnimGraphParameterGroup::GetParameter(uint32 index) const
    {
        return m_parameterIndices[index];
    }


    void AnimGraphParameterGroup::RemoveParameter(uint32 parameterIndex)
    {
        // Find the iterator for the given parameter and return in case the parameter is not part of the group.
        const auto& iterator = AZStd::find(m_parameterIndices.begin(), m_parameterIndices.end(), parameterIndex);
        if (iterator == m_parameterIndices.end())
        {
            return;
        }

        // Remove the paramter from the group and resort afterwards.
        m_parameterIndices.erase(iterator);
        AZStd::sort(m_parameterIndices.begin(), m_parameterIndices.end());
    }


    void AnimGraphParameterGroup::RemoveAllParameters()
    {
        m_parameterIndices.clear();
    }


    const AZStd::vector<uint32>& AnimGraphParameterGroup::GetParameterArray() const
    {
        return m_parameterIndices;
    }


    bool AnimGraphParameterGroup::Contains(uint32 parameterIndex) const
    {
        if (AZStd::find(m_parameterIndices.begin(), m_parameterIndices.end(), parameterIndex) == m_parameterIndices.end())
        {
            return false;
        }

        return true;
    }


    AZ::u32 AnimGraphParameterGroup::FindLocalParameterIndex(uint32 parameterIndex) const
    {
        const auto& iterator = AZStd::find(m_parameterIndices.begin(), m_parameterIndices.end(), parameterIndex);
        if (iterator == m_parameterIndices.end())
        {
            return MCORE_INVALIDINDEX32;
        }

        const size_t localIndex = iterator - m_parameterIndices.begin();
        return static_cast<AZ::u32>(localIndex);
    }


    void AnimGraphParameterGroup::SetName(const char* name)
    {
        m_name = name;
    }


    void AnimGraphParameterGroup::SetNumParameters(uint32 numParameters)
    {
        m_parameterIndices.resize(numParameters);
    }


    void AnimGraphParameterGroup::SetParameter(uint32 index, uint32 parameterIndex)
    {
        m_parameterIndices[index] = parameterIndex;
    }


    void AnimGraphParameterGroup::InitFrom(const AnimGraphParameterGroup& other)
    {
        m_parameterIndices  = other.m_parameterIndices;
        m_name              = other.m_name;
        m_isCollapsed       = other.m_isCollapsed;
    }
} // namespace EMotionFX