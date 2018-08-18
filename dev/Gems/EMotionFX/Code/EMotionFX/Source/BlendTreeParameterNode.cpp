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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>
#include "EMotionFXConfig.h"
#include "BlendTreeParameterNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraph.h"
#include "EventManager.h"

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeParameterNode, AnimGraphAllocator, 0)

    BlendTreeParameterNode::BlendTreeParameterNode()
        : AnimGraphNode()
    {
        // since this node dynamically sets the ports we don't really pre-create anything
        // The reinit function handles that
    }


    BlendTreeParameterNode::~BlendTreeParameterNode()
    {
    }


    void BlendTreeParameterNode::Reinit()
    {
        // Sort the parameter name mask in the way the parameters are stored in the anim graph.
        SortParameterNames(mAnimGraph, m_parameterNames);

        const size_t parameterCount = m_parameterNames.size();
        m_parameterIndices.resize(parameterCount);

        // Iterate through the parameter name mask and find the corresponding cached value parameter indices.
        // This expects the parameter names to be sorted in the way the parameters are stored in the anim graph.
        for (size_t i = 0; i < parameterCount; ++i)
        {
            const AZ::Outcome<size_t> parameterIndex = mAnimGraph->FindValueParameterIndexByName(m_parameterNames[i].c_str());
            MCORE_ASSERT(parameterIndex.IsSuccess());
            m_parameterIndices[i] = static_cast<uint32>(parameterIndex.GetValue());
        }

        RemoveInternalAttributesForAllInstances();

        if (parameterCount == 0)
        {
            // Parameter mask is empty, add ports for all parameters.
            const ValueParameterVector valueParameters = mAnimGraph->RecursivelyGetValueParameters();
            const uint32 valueParameterCount = static_cast<uint32>(valueParameters.size());
            InitOutputPorts(valueParameterCount);

            for (uint32 i = 0; i < valueParameterCount; ++i)
            {
                const ValueParameter* parameter = valueParameters[i];
                SetOutputPortName(static_cast<uint32>(i), parameter->GetName().c_str());

                mOutputPorts[i].mPortID = i;
                mOutputPorts[i].mCompatibleTypes[0] = parameter->GetType();
                if (GetTypeSupportsFloat(parameter->GetType()))
                {
                    mOutputPorts[i].mCompatibleTypes[1] = MCore::AttributeFloat::TYPE_ID;
                }
            }
        }
        else
        {
            // Parameter mask not empty, only add ports for the parameters in the mask.
            InitOutputPorts(static_cast<uint32>(parameterCount));

            for (uint32 i = 0; i < static_cast<uint32>(parameterCount); ++i)
            {
                const ValueParameter* parameter = mAnimGraph->FindValueParameter(m_parameterIndices[i]);
                SetOutputPortName(i, parameter->GetName().c_str());

                mOutputPorts[i].mPortID = i;
                mOutputPorts[i].mCompatibleTypes[0] = parameter->GetType();
                if (GetTypeSupportsFloat(parameter->GetType()))
                {
                    mOutputPorts[i].mCompatibleTypes[1] = MCore::AttributeFloat::TYPE_ID;
                }
            }
        }

        InitInternalAttributesForAllInstances();

        AnimGraphNode::Reinit();
        SyncVisualObject();
    }


    bool BlendTreeParameterNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* BlendTreeParameterNode::GetPaletteName() const
    {
        return "Parameters";
    }


    // get the palette category
    AnimGraphObject::ECategory BlendTreeParameterNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // the main process method of the final node
    void BlendTreeParameterNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);

        if (m_parameterIndices.empty())
        {
            // output all anim graph instance parameter values into the output ports
            const uint32 numParameters = mOutputPorts.GetLength();
            for (uint32 i = 0; i < numParameters; ++i)
            {
                GetOutputValue(animGraphInstance, i)->InitFrom(animGraphInstance->GetParameterValue(i));
            }
        }
        else
        {
            // output only the parameter values that have been selected in the parameter mask
            const size_t parameterCount = m_parameterIndices.size();
            for (size_t i = 0; i < parameterCount; ++i)
            {
                GetOutputValue(animGraphInstance, static_cast<AZ::u32>(i))->InitFrom(animGraphInstance->GetParameterValue(m_parameterIndices[i]));
            }
        }
    }


    // get the parameter index based on the port number
    uint32 BlendTreeParameterNode::GetParameterIndex(uint32 portNr) const
    {
        // check if the parameter mask is empty
        if (m_parameterIndices.empty())
        {
            return portNr;
        }

        // get the mapped parameter index in case the given port is valid
        if (portNr < m_parameterIndices.size())
        {
            return m_parameterIndices[portNr];
        }

        // return failure, the input port is not in range
        return MCORE_INVALIDINDEX32;
    }


    size_t BlendTreeParameterNode::FindPortForParameterIndex(size_t valueParameterIndex) const
    {
        // In case the parameter mask is empty, the parameter node holds ports for all value parameters, which means
        // that we can just return the given value parameter index.
        if (m_parameterIndices.empty())
        {
            return valueParameterIndex;
        }

        // The parameter mask is used. Find the port index for the given value parameter index in the cached parameter index array
        // which holds a value parameter index for each parameter in the parameter mask.
        const auto iterator = AZStd::find(m_parameterIndices.begin(), m_parameterIndices.end(), valueParameterIndex);
        if (iterator == m_parameterIndices.end())
        {
            // The given value parameter is not part of the mask, thus there is no port for it. Return failure.
            return MCORE_INVALIDINDEX32;
        }

        return iterator - m_parameterIndices.begin();
    }


    size_t BlendTreeParameterNode::CalcNewPortIndexForParameter(const AZStd::string& parameterName, const AZStd::vector<AZStd::string>& parametersToBeRemoved) const
    {
        size_t newIndex = 0;

        AZ_Assert(AZStd::find(parametersToBeRemoved.begin(), parametersToBeRemoved.end(), parameterName) == parametersToBeRemoved.end(),
            "Can't calculate the new parameter index for a parameter that is going to be removed.");

        const ValueParameterVector valueParameters = mAnimGraph->RecursivelyGetValueParameters();

        // Do we have a parameter mask set?
        if (m_parameterNames.empty())
        {
            for (const ValueParameter* parameter : valueParameters)
            {
                const AZStd::string& currentParameterName = parameter->GetName();

                // Skip all parameters that are going to be removed.
                if (AZStd::find(parametersToBeRemoved.begin(), parametersToBeRemoved.end(), currentParameterName) != parametersToBeRemoved.end())
                {
                    continue;
                }

                // Did we reach the parameter we're interested in?
                if (parameterName == currentParameterName)
                {
                    return newIndex;
                }

                newIndex++;
            }
        }
        else
        {
            // Parameter mask is used.
            for (const AZStd::string& currentParameterName : m_parameterNames)
            {
                // Skip all parameters that are going to be removed.
                if (AZStd::find(parametersToBeRemoved.begin(), parametersToBeRemoved.end(), currentParameterName) != parametersToBeRemoved.end())
                {
                    continue;
                }

                // Did we reach the parameter we're interested in?
                if (parameterName == currentParameterName)
                {
                    return newIndex;
                }

                newIndex++;
            }
        }

        // failure, the given parameter name hasn't been found
        return MCORE_INVALIDINDEX32;
    }


    bool BlendTreeParameterNode::GetTypeSupportsFloat(uint32 parameterType)
    {
        switch (parameterType)
        {
        case MCore::AttributeBool::TYPE_ID:
        case MCore::AttributeInt32::TYPE_ID:
            return true;
        default:
            // MCore::AttributeFloat is not considered because float->float conversion is not required
            return false;
        }
    }


    bool BlendTreeParameterNode::CheckIfParameterIndexUpdateNeeded() const
    {
        const size_t parameterCount = m_parameterNames.size();

        // check if the two arrays have the same length, if not return directly
        if (m_parameterIndices.size() != parameterCount)
        {
            return true;
        }

        // iterate through all parameters and check the parameter index array
        for (uint32 i = 0; i < parameterCount; ++i)
        {
            const AZ::Outcome<size_t> parameterIndex = mAnimGraph->FindValueParameterIndexByName(m_parameterNames[i].c_str());
            MCORE_ASSERT(parameterIndex.IsSuccess());
            if (m_parameterIndices[i] != parameterIndex.GetValue())
            {
                return true;
            }
        }

        // everything is okay, no update needed
        return false;
    }


    void BlendTreeParameterNode::CalcConnectedParameterNames(AZStd::vector<AZStd::string>& outParameterNames)
    {
        outParameterNames.clear();

        AnimGraphNode* parentNode = GetParentNode();
        if (!parentNode)
        {
            return;
        }

        // Check if any of the nodes in the graph is connected to the parameter node.
        const uint32 numNodes = parentNode->GetNumChildNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::AnimGraphNode* childNode = parentNode->GetChildNode(i);
            if (childNode == this)
            {
                continue;
            }

            // Iterate over the incoming connections for all nodes (each node only knows about the incoming nodes).
            const uint32 numConnections = childNode->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                EMotionFX::BlendTreeConnection* connection = childNode->GetConnection(c);

                // Is the node connected to the parameter node?
                if (connection->GetSourceNode() == this)
                {
                    const uint16 sourcePort     = connection->GetSourcePort();
                    const uint32 parameterIndex = GetParameterIndex(sourcePort);

                    // Add the parameter name to the resulting array.
                    if (parameterIndex != MCORE_INVALIDINDEX32)
                    {
                        outParameterNames.push_back(mAnimGraph->FindValueParameter(parameterIndex)->GetName());
                    }
                }
            }
        }
    }


    void BlendTreeParameterNode::SortParameterNames(AnimGraph* animGraph, AZStd::vector<AZStd::string>& outParameterNames)
    {
        // Iterate over all value parameters in the anim graph in the order they are stored.
        size_t currentIndex = 0;
        const size_t parameterCount = animGraph->GetNumValueParameters();
        for (size_t i = 0; i < parameterCount; ++i)
        {
            const ValueParameter* parameter = animGraph->FindValueParameter(i);

            // Check if the parameter is part of the parameter mask.
            auto parameterIterator = AZStd::find(outParameterNames.begin(), outParameterNames.end(), parameter->GetName());
            if (parameterIterator != outParameterNames.end())
            {
                // We found the parameter in the parameter mask. Swap the found element position with the current parameter index.
                // Increase the current parameter index as we found another parameter that got inserted.
                AZStd::iter_swap(outParameterNames.begin() + currentIndex, parameterIterator);
                currentIndex++;
            }
        }
    }


    const AZStd::vector<AZ::u32>& BlendTreeParameterNode::GetParameterIndices() const
    {
        return m_parameterIndices;
    }


    uint32 BlendTreeParameterNode::GetVisualColor() const
    {
        return MCore::RGBA(150, 150, 150);
    }


    void BlendTreeParameterNode::AddParameter(const AZStd::string& parameterName)
    {
        m_parameterNames.emplace_back(parameterName);
        Reinit();
    }


    void BlendTreeParameterNode::SetParameters(const AZStd::vector<AZStd::string>& parameterNames)
    {
        m_parameterNames = parameterNames;
        if (mAnimGraph)
        {
            Reinit();
        }
    }


    void BlendTreeParameterNode::SetParameters(const AZStd::string& parameterNamesWithSemicolons)
    {
        AZStd::vector<AZStd::string> parameterNames;
        AzFramework::StringFunc::Tokenize(parameterNamesWithSemicolons.c_str(), parameterNames, ";", false, true);

        SetParameters(parameterNames);
    }


    const AZStd::vector<AZStd::string>& BlendTreeParameterNode::GetParameters() const
    {
        return m_parameterNames;
    }


    AZStd::string BlendTreeParameterNode::ConstructParameterNamesString() const
    {
        return ConstructParameterNamesString(m_parameterNames);
    }


    AZStd::string BlendTreeParameterNode::ConstructParameterNamesString(const AZStd::vector<AZStd::string>& parameterNames)
    {
        AZStd::string result;

        for (const AZStd::string& parameterName : parameterNames)
        {
            if (!result.empty())
            {
                result += ';';
            }

            result += parameterName;
        }

        return result;
    }


    AZStd::string BlendTreeParameterNode::ConstructParameterNamesString(const AZStd::vector<AZStd::string>& parameterNames, const AZStd::vector<AZStd::string>& excludedParameterNames)
    {
        AZStd::string result;

        for (const AZStd::string& parameterName : parameterNames)
        {
            if (AZStd::find(excludedParameterNames.begin(), excludedParameterNames.end(), parameterName) == excludedParameterNames.end())
            {
                if (!result.empty())
                {
                    result += ';';
                }

                result += parameterName;
            }
        }

        return result;
    }


    void BlendTreeParameterNode::RemoveParameterByName(const AZStd::string& parameterName)
    {
        m_parameterNames.erase(AZStd::remove(m_parameterNames.begin(), m_parameterNames.end(), parameterName), m_parameterNames.end());
        if (mAnimGraph)
        {
            Reinit();
        }
    }

    void BlendTreeParameterNode::RenameParameterName(const AZStd::string& currentName, const AZStd::string& newName)
    {
        bool somethingChanged = false;
        for (AZStd::string& parameterName : m_parameterNames)
        {
            if (parameterName == currentName)
            {
                somethingChanged = true;
                parameterName = newName;
                break; // we should have only one parameter with this name
            }
        }
        if (somethingChanged && mAnimGraph)
        {
            Reinit();
        }
    }


    void BlendTreeParameterNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeParameterNode, AnimGraphNode>()
            ->Version(1)
            ->Field("parameterNames", &BlendTreeParameterNode::m_parameterNames)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeParameterNode>("Parameters", "Parameter node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("AnimGraphParameterMask", 0x67dd0993), &BlendTreeParameterNode::m_parameterNames, "Mask", "The visible and available of the node.")
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeParameterNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
        ;
    }
} // namespace EMotionFX

