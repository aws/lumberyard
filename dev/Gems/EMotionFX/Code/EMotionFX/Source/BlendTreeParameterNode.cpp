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
#include "EMotionFXConfig.h"
#include "BlendTreeParameterNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraph.h"
#include "EventManager.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // only motion and playback info are known
    BlendTreeParameterNode::BlendTreeParameterNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        mParameterIndices.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_NODES);

        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        mUpdateParameterMaskLocked = false;
    }


    // destructor
    BlendTreeParameterNode::~BlendTreeParameterNode()
    {
    }


    // create
    BlendTreeParameterNode* BlendTreeParameterNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeParameterNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeParameterNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeParameterNode::RegisterPorts()
    {
        // since this node dynamically sets the ports we don't really pre-create anything
        // The init function handles that
    }


    void BlendTreeParameterNode::RegisterAttributes()
    {
        MCore::AttributeSettings* attrib = RegisterAttribute("Mask", "mask", "The visible and available of the node.", ATTRIBUTE_INTERFACETYPE_PARAMETERNAMES);
        attrib->SetDefaultValue(AttributeParameterMask::Create());
        attrib->SetReinitGuiOnValueChange(true);
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


    // create a clone of this node
    AnimGraphObject* BlendTreeParameterNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeParameterNode* clone = new BlendTreeParameterNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // after the node has been created, initialize it
    void BlendTreeParameterNode::InitForAnimGraph(AnimGraph* animGraph)
    {
        RemoveInternalAttributesForAllInstances();

        // check if the parameter mask is empty
        if (mParameterIndices.GetIsEmpty())
        {
            // create an output port for each parameter
            const uint32 numParameters = animGraph->GetNumParameters();
            InitOutputPorts(numParameters);

            // name the output ports
            for (uint32 i = 0; i < numParameters; ++i)
            {
                MCore::AttributeSettings* parameter = animGraph->GetParameter(i);
                SetOutputPortName(i, parameter->GetName());

                // delete the old output port attribute and create a clone of the actual one from the anim graph parameter
                /////if (mOutputPorts[i].mValue)
                //////mOutputPorts[i].mValue->Destroy();
                //////mOutputPorts[i].mValue = MCore::GetAttributePool().RequestNew( parameter->GetDefaultValue()->GetType() );

                mOutputPorts[i].mPortID = i;
                mOutputPorts[i].mCompatibleTypes[0] = parameter->GetDefaultValue()->GetType();
            }
        }
        else
        {
            // create an output port for the parameters inside the parameter mask
            const uint32 numParameters = mParameterIndices.GetLength();
            InitOutputPorts(numParameters);

            // name the output ports
            for (uint32 i = 0; i < numParameters; ++i)
            {
                MCore::AttributeSettings* parameter = animGraph->GetParameter(mParameterIndices[i]);
                SetOutputPortName(i, parameter->GetName());

                // delete the old output port attribute and create a clone of the actual one from the anim graph parameter
                //////if (mOutputPorts[i].mValue)
                //////mOutputPorts[i].mValue->Destroy();
                //////mOutputPorts[i].mValue = MCore::GetAttributePool().RequestNew( parameter->GetDefaultValue()->GetType() );

                mOutputPorts[i].mPortID = i;
                mOutputPorts[i].mCompatibleTypes[0] = parameter->GetDefaultValue()->GetType();
            }
        }

        // reinit
        InitInternalAttributesForAllInstances();
    }



    // the main process method of the final node
    void BlendTreeParameterNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);

        //AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        // check if the parameter mask is empty
        if (mParameterIndices.GetIsEmpty())
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
            const uint32 numParameters = mParameterIndices.GetLength();
            for (uint32 i = 0; i < numParameters; ++i)
            {
                GetOutputValue(animGraphInstance, i)->InitFrom(animGraphInstance->GetParameterValue(mParameterIndices[i]));
            }
        }
    }


    // get the parameter index based on the port number
    uint32 BlendTreeParameterNode::GetParameterIndex(uint32 portNr) const
    {
        // check if the parameter mask is empty
        if (mParameterIndices.GetIsEmpty())
        {
            return portNr;
        }

        // get the mapped parameter index in case the given port is valid
        if (portNr < mParameterIndices.GetLength())
        {
            return mParameterIndices[portNr];
        }

        // return failure, the input port is not in range
        return MCORE_INVALIDINDEX32;
    }


    // find the port number based on the parameter index
    uint32 BlendTreeParameterNode::GetPortForParameterIndex(uint32 parameterIndex) const
    {
        // check if the parameter mask is empty
        if (mParameterIndices.GetIsEmpty())
        {
            return parameterIndex;
        }

        // else find the parameter index in the mapped parameter index array
        return mParameterIndices.Find(parameterIndex);
    }


    uint32 BlendTreeParameterNode::CalcNewPortForParameterIndex(uint32 parameterIndex, const AZStd::vector<AZStd::string>& parametersToBeRemoved) const
    {
        uint32 newIndex = 0;
        AttributeParameterMask* parameterMaskAttribute = static_cast<AttributeParameterMask*>(mAttributeValues[ATTRIB_MASK]);

        // Get the name of the given parameter.
        MCore::String parameterName = mAnimGraph->GetParameter(parameterIndex)->GetName();
        AZ_Assert(AZStd::find(parametersToBeRemoved.begin(), parametersToBeRemoved.end(), parameterName.AsChar()) == parametersToBeRemoved.end(),
            "Can't calculate the new parameter index for a parameter that is going to be removed.");

        const uint32 numParameters = mAnimGraph->GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            const char* currentParameterName = mAnimGraph->GetParameter(i)->GetName();

            // Skip all parameters that are going to be removed.
            if (AZStd::find(parametersToBeRemoved.begin(), parametersToBeRemoved.end(), currentParameterName) != parametersToBeRemoved.end())
            {
                continue;
            }

            // Do we have a parameter mask set?
            if (parameterMaskAttribute->GetNumParameterNames() > 0)
            {
                // Skip the parameter in case it is not part of the parameter mask.
                if (parameterMaskAttribute->FindParameterNameIndex(currentParameterName) == MCORE_INVALIDINDEX32)
                {
                    continue;
                }
            }

            // Did we reach the parameter we're interested in?
            if (parameterName.CheckIfIsEqual(currentParameterName))
            {
                return newIndex;
            }

            newIndex++;
        }

        // failure, the given parameter name hasn't been found
        return MCORE_INVALIDINDEX32;
    }


    // update the parameter mask and init the node again
    void BlendTreeParameterNode::Reinit()
    {
        // get the parameter mask attribute
        AttributeParameterMask* parameterMaskAttribute = static_cast<AttributeParameterMask*>(mAttributeValues[ATTRIB_MASK]);

        // get the number of parameters inside the parameter mask and resize the parameter index array correspondingly
        const uint32 numParameters = parameterMaskAttribute->GetNumParameterNames();
        mParameterIndices.Resize(numParameters);

        // iterate through all parameters and fill the parameter index array
        for (uint32 i = 0; i < numParameters; ++i)
        {
            const char* parameterName = parameterMaskAttribute->GetParameterName(i);
            const uint32 parameterIndex = mAnimGraph->FindParameterIndex(parameterName);
            MCORE_ASSERT(parameterIndex != MCORE_INVALIDINDEX32);
            mParameterIndices[i]        = parameterIndex;
        }

        // reinit the parameter node
        InitForAnimGraph(mAnimGraph);
    }


    bool BlendTreeParameterNode::CheckIfParameterIndexUpdateNeeded() const
    {
        // get the parameter mask attribute and the number of parameters inside it
        AttributeParameterMask* parameterMaskAttribute = static_cast<AttributeParameterMask*>(mAttributeValues[ATTRIB_MASK]);
        const uint32 numParameters = parameterMaskAttribute->GetNumParameterNames();

        // check if the two arrays have the same length, if not return directly
        if (mParameterIndices.GetLength() != numParameters)
        {
            return true;
        }

        // iterate through all parameters and check the parameter index array
        for (uint32 i = 0; i < numParameters; ++i)
        {
            const uint32 parameterIndex = mAnimGraph->FindParameterIndex(parameterMaskAttribute->GetParameterName(i));
            MCORE_ASSERT(parameterIndex != MCORE_INVALIDINDEX32);
            if (mParameterIndices[i] != parameterIndex)
            {
                return true;
            }
        }

        // everything is okay, no update needed
        return false;
    }


    // when the attributes change
    void BlendTreeParameterNode::OnUpdateAttributes()
    {
        if (mUpdateParameterMaskLocked == false && CheckIfParameterIndexUpdateNeeded())
        {
            mUpdateParameterMaskLocked = true;
            GetEventManager().OnParameterNodeMaskChanged(this);
            mUpdateParameterMaskLocked = false;

        #ifndef EMFX_EMSTUDIOBUILD
            MCore::LogWarning("Please do not adjust the parameter mask at runtime! Only adjust the parameter mask inside EMotion Studio.");
        #endif
        }
    }


    // get the type string
    const char* BlendTreeParameterNode::GetTypeString() const
    {
        return "BlendTreeParameterNode";
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
                        outParameterNames.push_back(mAnimGraph->GetParameter(parameterIndex)->GetName());
                    }
                }
            }
        }
    }


    const MCore::Array<uint32>& BlendTreeParameterNode::GetParameterIndices() const
    {
        return mParameterIndices;
    }


    uint32 BlendTreeParameterNode::GetVisualColor() const
    {
        return MCore::RGBA(150, 150, 150);
    }
}   // namespace EMotionFX

