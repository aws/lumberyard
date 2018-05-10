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
#include "BlendTreeMorphTargetNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphManager.h"
#include "AnimGraphRefCountedData.h"
#include "EMotionFXManager.h"
#include "MorphSetup.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    BlendTreeMorphTargetNode::BlendTreeMorphTargetNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
        SetNodeInfo("<none>");
    }


    BlendTreeMorphTargetNode* BlendTreeMorphTargetNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeMorphTargetNode(animGraph);
    }


    AnimGraphObjectData* BlendTreeMorphTargetNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    const char* BlendTreeMorphTargetNode::GetTypeString() const
    {
        return "BlendTreeMorphTargetNode";
    }


    void BlendTreeMorphTargetNode::RegisterPorts()
    {
        // Setup input ports.
        InitInputPorts(2);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsNumber("Morph Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // Setup output ports.
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    void BlendTreeMorphTargetNode::RegisterAttributes()
    {
        MCore::AttributeSettings* attrib = RegisterAttribute("Morph Target", "morphTarget", "The morph target to apply the weight changes to.", ATTRIBUTE_INTERFACETYPE_MORPHTARGETPICKER);
        attrib->SetDefaultValue(MCore::AttributeArray::Create(MCore::AttributeString::TYPE_ID));
        attrib->SetReinitObjectOnValueChange(true);
        attrib->SetReinitGuiOnValueChange(true);
    }


    const char* BlendTreeMorphTargetNode::GetPaletteName() const
    {
        return "Morph Target";
    }


    AnimGraphObject::ECategory BlendTreeMorphTargetNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    AnimGraphObject* BlendTreeMorphTargetNode::Clone(AnimGraph* animGraph)
    {
        BlendTreeMorphTargetNode* clone = new BlendTreeMorphTargetNode(animGraph);
        CopyBaseObjectTo(clone);
        return clone;
    }


    void BlendTreeMorphTargetNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
    }


    // Update the morph indices, which will convert the morph target name into an index into the current LOD.
    void BlendTreeMorphTargetNode::UpdateMorphIndices(ActorInstance* actorInstance, UniqueData* uniqueData, bool forceUpdate)
    {
        // Check if our LOD level changed, if not, we don't need to refresh it.
        const uint32 lodLevel = actorInstance->GetLODLevel();
        if (!forceUpdate && uniqueData->m_lastLodLevel == lodLevel)
            return;

        // Convert the morph target name into an index for fast lookup.
        MCore::AttributeArray* morphArray = GetAttributeArray(ATTRIB_MORPHTARGET);
        if (morphArray->GetNumAttributes() > 0)
        {
            const EMotionFX::MorphSetup* morphSetup = actorInstance->GetActor()->GetMorphSetup(lodLevel);
            const char* morphTargetName = static_cast<MCore::AttributeString*>(morphArray->GetAttribute(0))->AsChar();
            uniqueData->m_morphTargetIndex = morphSetup->FindMorphTargetIndexByName(morphTargetName);
        }
        else
        {
            uniqueData->m_morphTargetIndex = MCORE_INVALIDINDEX32;
        }

        uniqueData->m_lastLodLevel = lodLevel;
    }


    // Calculate the output pose.
    void BlendTreeMorphTargetNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // Mark this node as having an error when the morph target cannot be found.
        // If there is none setup, we see that as a non-error state, otherwise you would see the node marked as error directly after creation.
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        MCore::AttributeArray* morphArray = GetAttributeArray(ATTRIB_MORPHTARGET);
#ifdef EMFX_EMSTUDIOBUILD
        if (morphArray->GetNumAttributes() == 0)
        {
            SetHasError(animGraphInstance, false);
        }
        else
        {
            SetHasError(animGraphInstance, uniqueData->m_morphTargetIndex == MCORE_INVALIDINDEX32);
        }
#endif

        // Refresh the morph target indices when needed.
        // This has to happen when we changed LOD levels, as the new LOD might have another number of morph targets.
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        UpdateMorphIndices(actorInstance, uniqueData, false);
        
        // If there is no input pose init the uutput pose to the bind pose.
        AnimGraphPose* outputPose;
        if (!mInputPorts[INPUTPORT_POSE].mConnection)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        }
        else // Init the output pose to the input pose.
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
        }

        // Try to modify the morph target weight with the value we specified as input.
        if (!mDisabled && uniqueData->m_morphTargetIndex != MCORE_INVALIDINDEX32)
        {
            // If we have an input to the weight port, read that value use that value to overwrite the pose value with.
            if (mInputPorts[INPUTPORT_WEIGHT].mConnection)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_WEIGHT));
                const float morphWeight = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT);

                // Overwrite the morph target weight.
                outputPose->GetPose().SetMorphWeight(uniqueData->m_morphTargetIndex, morphWeight);
            }
        }

        // Debug visualize the output pose.
        #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
        #endif
    }


    void BlendTreeMorphTargetNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // Create the unique data object if it doesn't yet exist.
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (!uniqueData)
        {
            uniqueData = static_cast<UniqueData*>(GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance));
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        // Force update the morph target indices.
        UpdateMorphIndices(animGraphInstance->GetActorInstance(), uniqueData, true);

        // Update the node info string.
        MCore::AttributeArray* morphArray = GetAttributeArray(ATTRIB_MORPHTARGET);
        if (morphArray->GetNumAttributes() > 0)
        {
            const EMotionFX::MorphSetup* morphSetup = animGraphInstance->GetActorInstance()->GetActor()->GetMorphSetup(0);
            const char* morphTargetName = static_cast<MCore::AttributeString*>(morphArray->GetAttribute(0))->AsChar();
            SetNodeInfo(morphTargetName);
        }
        else
        {
            SetNodeInfo("<none>");
        }
    }

}   // namespace EMotionFX

