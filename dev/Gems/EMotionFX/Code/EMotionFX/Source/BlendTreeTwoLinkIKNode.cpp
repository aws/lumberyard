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
#include "BlendTreeTwoLinkIKNode.h"
#include "EventManager.h"
#include "AnimGraphManager.h"
#include "Recorder.h"
#include "Node.h"
#include "TransformData.h"
#include "TwoLinkIKSolver.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeTwoLinkIKNode::BlendTreeTwoLinkIKNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeTwoLinkIKNode::~BlendTreeTwoLinkIKNode()
    {
    }


    // create
    BlendTreeTwoLinkIKNode* BlendTreeTwoLinkIKNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeTwoLinkIKNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeTwoLinkIKNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void BlendTreeTwoLinkIKNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(5);
        SetupInputPort("Pose",     INPUTPORT_POSE,     AttributePose::TYPE_ID,             PORTID_INPUT_POSE);
        SetupInputPort("Goal Pos", INPUTPORT_GOALPOS,  MCore::AttributeVector3::TYPE_ID,   PORTID_INPUT_GOALPOS);
        SetupInputPort("Bend Dir", INPUTPORT_BENDDIR,  MCore::AttributeVector3::TYPE_ID,   PORTID_INPUT_BENDDIR);
        SetupInputPort("Goal Rot", INPUTPORT_GOALROT,  AttributeRotation::TYPE_ID,         PORTID_INPUT_GOALROT);
        SetupInputPortAsNumber("Weight",   INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTreeTwoLinkIKNode::RegisterAttributes()
    {
        // the end node
        MCore::AttributeSettings* attrib = RegisterAttribute("End Node", "endNode", "The end node name of the chain, for example the foot, or hand.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetReinitObjectOnValueChange(true);
        attrib->SetDefaultValue(MCore::AttributeString::Create());

        // the end effector node
        attrib = RegisterAttribute("End Effector", "endEffectorNode", "The end effector node, which represents the node that actually tries to reach the goal. This is probably also the hand, or a child node of it for example. If not set, the end node is used.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetReinitObjectOnValueChange(true);
        attrib->SetDefaultValue(MCore::AttributeString::Create());

        // the node to align the end effector to
        attrib = RegisterAttribute("Align To", "alignNode", "The node to align the end node to. This basically sets the goal to this node.", ATTRIBUTE_INTERFACETYPE_GOALNODESELECTION);
        attrib->SetReinitObjectOnValueChange(true);
        attrib->SetDefaultValue(AttributeGoalNode::Create());

        // the node to align the end effector to
        attrib = RegisterAttribute("Bend Dir Node", "bendDirNode", "The optional node to control the bend direction. The vector from the start node to the bend dir node will be used as bend direction.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetReinitObjectOnValueChange(true);
        attrib->SetDefaultValue(MCore::AttributeString::Create());

        // adjust the rotation of the end effector?
        attrib = RegisterAttribute("Enable Rotation Goal", "rotationEnabled", "Enable the goal orientation?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // relative bend direction (rotate with the actor instance?)
        attrib = RegisterAttribute("Relative Bend Dir", "relativeBendDir", "Use a relative (to the actor instance) bend direction, instead of global space?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1));

        // extract the bend direction from the input pose?
        attrib = RegisterAttribute("Extract Bend Dir", "extractBendDir", "Extract the bend direction from the input pose instead of using the bend dir input value?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetReinitGuiOnValueChange(true);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));
    }


    // get the palette name
    const char* BlendTreeTwoLinkIKNode::GetPaletteName() const
    {
        return "TwoLink IK";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeTwoLinkIKNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeTwoLinkIKNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeTwoLinkIKNode* clone = new BlendTreeTwoLinkIKNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // update the unique data
    void BlendTreeTwoLinkIKNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // update the unique data if needed
        if (uniqueData->mMustUpdate)
        {
            ActorInstance*  actorInstance   = animGraphInstance->GetActorInstance();
            Actor*          actor           = actorInstance->GetActor();
            Skeleton*       skeleton        = actor->GetSkeleton();

            // don't update the next time again
            uniqueData->mMustUpdate             = false;
            uniqueData->mNodeIndexA             = MCORE_INVALIDINDEX32;
            uniqueData->mNodeIndexB             = MCORE_INVALIDINDEX32;
            uniqueData->mNodeIndexC             = MCORE_INVALIDINDEX32;
            uniqueData->mAlignNodeIndex         = MCORE_INVALIDINDEX32;
            uniqueData->mBendDirNodeIndex       = MCORE_INVALIDINDEX32;
            uniqueData->mEndEffectorNodeIndex   = MCORE_INVALIDINDEX32;
            uniqueData->mIsValid                = false;

            // find the end node
            const char* nodeName = GetAttributeString(ATTRIB_ENDNODE)->AsChar();
            if (nodeName == nullptr)
            {
                return;
            }
            const Node* nodeC = skeleton->FindNodeByName(nodeName);
            if (nodeC == nullptr)
            {
                return;
            }
            uniqueData->mNodeIndexC = nodeC->GetNodeIndex();

            // get the second node
            uniqueData->mNodeIndexB = nodeC->GetParentIndex();
            if (uniqueData->mNodeIndexB == MCORE_INVALIDINDEX32)
            {
                return;
            }

            // get the third node
            uniqueData->mNodeIndexA = skeleton->GetNode(uniqueData->mNodeIndexB)->GetParentIndex();
            if (uniqueData->mNodeIndexA == MCORE_INVALIDINDEX32)
            {
                return;
            }

            // get the end effector node
            const Node* endEffectorNode = skeleton->FindNodeByName(GetAttributeString(ATTRIB_ENDEFFECTORNODE)->AsChar());
            if (endEffectorNode)
            {
                uniqueData->mEndEffectorNodeIndex = endEffectorNode->GetNodeIndex();
            }

            // find the bend direction node
            const char* bendDirNodeName = GetAttributeString(ATTRIB_BENDDIRNODE)->AsChar();
            const Node* bendDirNode = skeleton->FindNodeByName(bendDirNodeName);
            if (bendDirNode)
            {
                uniqueData->mBendDirNodeIndex = bendDirNode->GetNodeIndex();
            }

            // lookup the actor instance to get the alignment node from
            const AttributeGoalNode* goalNodeAttrib = GetAttributeGoalNode(ATTRIB_ALIGNNODE);
            const ActorInstance* alignInstance = animGraphInstance->FindActorInstanceFromParentDepth(goalNodeAttrib->GetParentDepth());
            if (alignInstance)
            {
                nodeName = GetAttributeGoalNode(ATTRIB_ALIGNNODE)->GetNodeName();
                if (nodeName)
                {
                    const Node* alignNode = alignInstance->GetActor()->GetSkeleton()->FindNodeByName(nodeName);
                    if (alignNode)
                    {
                        uniqueData->mAlignNodeIndex = alignNode->GetNodeIndex();
                    }
                }
            }

            uniqueData->mIsValid = true;
        }
    }


    // the main process method of the final node
    void BlendTreeTwoLinkIKNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // make sure we have at least an input pose, otherwise output the bind pose
        if (GetInputPort(INPUTPORT_POSE).mConnection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        // get the weight
        float weight = 1.0f;
        if (GetInputPort(INPUTPORT_WEIGHT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_WEIGHT));
            weight = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT);
            weight = MCore::Clamp<float>(weight, 0.0f, 1.0f);
        }

        // if the IK weight is near zero, we can skip all calculations and act like a pass-trough node
        if (weight < MCore::Math::epsilon || mDisabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
            return;
        }

        // get the input pose and copy it over to the output pose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
        const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *inputPose;

        //------------------------------------
        // get the node indices to work on
        //------------------------------------
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        UpdateUniqueData(animGraphInstance, uniqueData); // update the unique data (lookup node indices when something changed)
        if (uniqueData->mIsValid == false)
        {
        #ifdef EMFX_EMSTUDIOBUILD
            SetHasError(animGraphInstance, true);
        #endif
            return;
        }

        // get the node indices
        const uint32 nodeIndexA     = uniqueData->mNodeIndexA;
        const uint32 nodeIndexB     = uniqueData->mNodeIndexB;
        const uint32 nodeIndexC     = uniqueData->mNodeIndexC;
        const uint32 bendDirIndex   = uniqueData->mBendDirNodeIndex;
        uint32 alignNodeIndex       = uniqueData->mAlignNodeIndex;
        uint32 endEffectorNodeIndex = uniqueData->mEndEffectorNodeIndex;

        // use the end node as end effector node if no goal node has been specified
        if (endEffectorNodeIndex == MCORE_INVALIDINDEX32)
        {
            endEffectorNodeIndex = nodeIndexC;
        }

        // get the goal
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALPOS));
        const MCore::AttributeVector3* inputGoalPos = GetInputVector3(animGraphInstance, INPUTPORT_GOALPOS);
        MCore::Vector3 goal = (inputGoalPos) ? inputGoalPos->GetValue() : MCore::Vector3(0.0f, 0.0f, 0.0f);

        // there is no error, as we have all we need to solve this
    #ifdef EMFX_EMSTUDIOBUILD
        SetHasError(animGraphInstance, false);
    #endif

        // calculate the global space matrices
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // lookup the actor instance to get the alignment node from
        const AttributeGoalNode* goalNodeAttrib = GetAttributeGoalNode(ATTRIB_ALIGNNODE);
        ActorInstance* alignInstance = actorInstance;

        // adjust the gizmo offset value
        if (alignNodeIndex != MCORE_INVALIDINDEX32)
        {
            // update the alignment actor instance
            alignInstance = animGraphInstance->FindActorInstanceFromParentDepth(goalNodeAttrib->GetParentDepth());
            if (alignInstance)
            {
                const MCore::Vector3& offset = alignInstance->GetTransformData()->GetCurrentPose()->GetGlobalTransformInclusive(alignNodeIndex).mPosition;
                goal += offset;

            #ifdef EMFX_EMSTUDIOBUILD
                // check if the offset goal pos values comes from a param node
                const BlendTreeConnection* posConnection = GetInputPort(INPUTPORT_GOALPOS).mConnection;
                if (posConnection)
                {
                    if (posConnection->GetSourceNode()->GetType() == BlendTreeParameterNode::TYPE_ID)
                    {
                        BlendTreeParameterNode* parameterNode = static_cast<BlendTreeParameterNode*>(posConnection->GetSourceNode());
                        GetEventManager().OnSetVisualManipulatorOffset(animGraphInstance, parameterNode->GetParameterIndex(posConnection->GetSourcePort()), offset);
                    }
                }
            #endif
            }
            else
            {
                alignNodeIndex = MCORE_INVALIDINDEX32; // we were not able to get the align instance, so set the align node index to the invalid index
            }
        }
    #ifdef EMFX_EMSTUDIOBUILD
        else
        {
            const BlendTreeConnection* posConnection = GetInputPort(INPUTPORT_GOALPOS).mConnection;
            if (posConnection)
            {
                if (posConnection->GetSourceNode()->GetType() == BlendTreeParameterNode::TYPE_ID)
                {
                    BlendTreeParameterNode* parameterNode = static_cast<BlendTreeParameterNode*>(posConnection->GetSourceNode());
                    GetEventManager().OnSetVisualManipulatorOffset(animGraphInstance, parameterNode->GetParameterIndex(posConnection->GetSourcePort()), MCore::Vector3(0.0f, 0.0f, 0.0f));
                }
            }
        }
    #endif

        //------------------------------------
        // perform the main calculation part
        //------------------------------------
        Pose& outTransformPose = outputPose->GetPose();
        Transform globalTransformA = outTransformPose.GetGlobalTransformInclusive(nodeIndexA);
        Transform globalTransformB = outTransformPose.GetGlobalTransformInclusive(nodeIndexB);
        Transform globalTransformC = outTransformPose.GetGlobalTransformInclusive(nodeIndexC);

        // extract the bend direction from the input pose?
        MCore::Vector3 bendDir;
        const bool extractBendDir = GetAttributeFloatAsBool(ATTRIB_EXTRACTBENDDIR);
        if (extractBendDir)
        {
            if (bendDirIndex != MCORE_INVALIDINDEX32)
            {
                bendDir = outTransformPose.GetGlobalTransformInclusive(bendDirIndex).mPosition - globalTransformA.mPosition;
            }
            else
            {
                bendDir = globalTransformB.mPosition - globalTransformA.mPosition;
            }
        }
        else
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_BENDDIR));
            const MCore::AttributeVector3* inputBendDir = GetInputVector3(animGraphInstance, INPUTPORT_BENDDIR);
            bendDir = (inputBendDir) ? inputBendDir->GetValue() : MCore::Vector3(0.0f, 0.0f, -1.0f);
        }

        // if we want a relative bend dir, rotate it with the actor (only do this if we don't extract the bend dir)
        if (GetAttributeFloatAsBool(ATTRIB_RELBENDDIR) && extractBendDir == false)
        {
            bendDir = actorInstance->GetGlobalTransform().mRotation * bendDir;
            bendDir.SafeNormalize();
        }
        else
        {
            bendDir.SafeNormalize();
        }

        // if end node rotation is enabled
        const bool enableRotation = GetAttributeFloatAsBool(ATTRIB_ENABLEROTATION);
        if (enableRotation)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALROT));
            const AttributeRotation* inputGoalRot = GetInputRotation(animGraphInstance, INPUTPORT_GOALROT);

            // if we don't want to align the rotation and position to another given node
            if (alignNodeIndex == MCORE_INVALIDINDEX32)
            {
                MCore::Quaternion newRotation; // identity quat
                if (inputGoalRot)
                {
                    newRotation = inputGoalRot->GetRotationQuaternion(); // use our new rotation directly
                }
                globalTransformC.mRotation = newRotation;
                outTransformPose.SetGlobalTransformInclusive(nodeIndexC, globalTransformC);
                outTransformPose.UpdateLocalTransform(nodeIndexC);
            }
            else // align to another node
            {
                if (inputGoalRot)
                {
                    OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALROT));
                    globalTransformC.mRotation = GetInputRotation(animGraphInstance, INPUTPORT_GOALROT)->GetRotationQuaternion() * alignInstance->GetTransformData()->GetCurrentPose()->GetGlobalTransformInclusive(alignNodeIndex).mRotation;
                }
                else
                {
                    globalTransformC.mRotation = alignInstance->GetTransformData()->GetCurrentPose()->GetGlobalTransformInclusive(alignNodeIndex).mRotation;
                }

                outTransformPose.SetGlobalTransformInclusive(nodeIndexC, globalTransformC);
                outTransformPose.UpdateLocalTransform(nodeIndexC);
            }
        }

        // adjust the goal and get the end effector position
        MCore::Vector3 endEffectorNodePos = outTransformPose.GetGlobalTransformInclusive(endEffectorNodeIndex).mPosition;
        const MCore::Vector3 posCToEndEffector = endEffectorNodePos - globalTransformC.mPosition;
        if (enableRotation)
        {
            goal -= posCToEndEffector;
        }

        // store the desired rotation
        MCore::Quaternion newNodeRotationC = globalTransformC.mRotation;

        // draw debug lines
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            const float s = animGraphInstance->GetVisualizeScale() * actorInstance->GetVisualizeScale();

            MCore::Vector3 realGoal;
            if (enableRotation)
            {
                realGoal = goal + posCToEndEffector;
            }
            else
            {
                realGoal = goal;
            }

            uint32 color = mVisualizeColor;
            GetEventManager().OnDrawLine(realGoal - MCore::Vector3(s, 0, 0), realGoal + MCore::Vector3(s, 0, 0), color);
            GetEventManager().OnDrawLine(realGoal - MCore::Vector3(0, s, 0), realGoal + MCore::Vector3(0, s, 0), color);
            GetEventManager().OnDrawLine(realGoal - MCore::Vector3(0, 0, s), realGoal + MCore::Vector3(0, 0, s), color);

            color = MCore::RGBA(0, 255, 255);
            GetEventManager().OnDrawLine(globalTransformA.mPosition, globalTransformA.mPosition + bendDir * s * 2.5f, color);
            GetEventManager().OnDrawLine(globalTransformA.mPosition - MCore::Vector3(s, 0, 0), globalTransformA.mPosition + MCore::Vector3(s, 0, 0), color);
            GetEventManager().OnDrawLine(globalTransformA.mPosition - MCore::Vector3(0, s, 0), globalTransformA.mPosition + MCore::Vector3(0, s, 0), color);
            GetEventManager().OnDrawLine(globalTransformA.mPosition - MCore::Vector3(0, 0, s), globalTransformA.mPosition + MCore::Vector3(0, 0, s), color);

            //color = MCore::RGBA(0, 255, 0);
            //GetEventManager().OnDrawLine( endEffectorNodePos-Vector3(s,0,0), endEffectorNodePos+Vector3(s,0,0), color);
            //GetEventManager().OnDrawLine( endEffectorNodePos-Vector3(0,s,0), endEffectorNodePos+Vector3(0,s,0), color);
            //GetEventManager().OnDrawLine( endEffectorNodePos-Vector3(0,0,s), endEffectorNodePos+Vector3(0,0,s), color);

            //color = MCore::RGBA(255, 255, 0);
            //GetEventManager().OnDrawLine( posA, posB, color);
            //GetEventManager().OnDrawLine( posB, posC, color);
            //GetEventManager().OnDrawLine( posC, endEffectorNodePos, color);
        }
    #endif

        // perform IK, try to find a solution by calculating the new middle node position
        MCore::Vector3 midPos;
        if (enableRotation)
        {
            TwoLinkIKSolver::Solve2LinkIK(globalTransformA.mPosition, globalTransformB.mPosition, globalTransformC.mPosition, goal, bendDir, &midPos);
        }
        else
        {
            TwoLinkIKSolver::Solve2LinkIK(globalTransformA.mPosition, globalTransformB.mPosition, endEffectorNodePos, goal, bendDir, &midPos);
        }

        // --------------------------------------
        // calculate the new node transforms
        // --------------------------------------
        // calculate the differences between the current forward vector and the new one after IK
        MCore::Vector3 oldForward = globalTransformB.mPosition - globalTransformA.mPosition;
        MCore::Vector3 newForward = midPos - globalTransformA.mPosition;
        oldForward.SafeNormalize();
        newForward.SafeNormalize();

        // perform a delta rotation to rotate into the new direction after IK
        float deltaAngle = MCore::Math::ACos(oldForward.Dot(newForward));
        MCore::Vector3 axis = oldForward.Cross(newForward);
        MCore::Quaternion deltaRot(axis, deltaAngle);
        globalTransformA.mRotation = deltaRot * globalTransformA.mRotation;
        outTransformPose.SetGlobalTransformInclusive(nodeIndexA, globalTransformA);

        //globalTransformA = outTransformPose.GetGlobalTransformIncludingActorInstanceTransform(nodeIndexA);
        globalTransformB = outTransformPose.GetGlobalTransformInclusive(nodeIndexB);
        globalTransformC = outTransformPose.GetGlobalTransformInclusive(nodeIndexC);

        // get the new current node positions
        midPos = globalTransformB.mPosition;
        endEffectorNodePos = outTransformPose.GetGlobalTransformInclusive(endEffectorNodeIndex).mPosition;

        // second node
        if (enableRotation)
        {
            oldForward = globalTransformC.mPosition - globalTransformB.mPosition;
        }
        else
        {
            oldForward = endEffectorNodePos - globalTransformB.mPosition;
        }

        oldForward.SafeNormalize();
        newForward = goal - globalTransformB.mPosition;
        newForward.SafeNormalize();

        // calculate the delta rotation
        const float dotProduct = oldForward.Dot(newForward);
        if (dotProduct < 1.0f - MCore::Math::epsilon)
        {
            deltaAngle = MCore::Math::ACos(dotProduct);
            axis = oldForward.Cross(newForward);
            deltaRot = MCore::Quaternion(axis, deltaAngle);
        }
        else
        {
            deltaRot.Identity();
        }

        globalTransformB.mRotation = deltaRot * globalTransformB.mRotation;
        globalTransformB.mPosition = midPos;
        outTransformPose.SetGlobalTransformInclusive(nodeIndexB, globalTransformB);
        //outTransformPose.UpdateLocalTransform( nodeIndexB );

        // update the rotation of node C
        if (enableRotation)
        {
            globalTransformC = outTransformPose.GetGlobalTransformInclusive(nodeIndexC);
            globalTransformC.mRotation = newNodeRotationC;
            outTransformPose.SetGlobalTransformInclusive(nodeIndexC, globalTransformC);
            outTransformPose.UpdateLocalTransform(nodeIndexC);
        }

        // only blend when needed
        if (weight < 0.999f)
        {
            // get the original input pose
            const Pose& inputTransformPose = inputPose->GetPose();

            // get the original input transforms
            Transform finalTransformA = inputTransformPose.GetLocalTransform(nodeIndexA);
            Transform finalTransformB = inputTransformPose.GetLocalTransform(nodeIndexB);
            Transform finalTransformC = inputTransformPose.GetLocalTransform(nodeIndexC);

            // blend them into the new transforms after IK
            finalTransformA.Blend(outTransformPose.GetLocalTransform(nodeIndexA), weight);
            finalTransformB.Blend(outTransformPose.GetLocalTransform(nodeIndexB), weight);
            finalTransformC.Blend(outTransformPose.GetLocalTransform(nodeIndexC), weight);

            // copy them to the output transforms
            outTransformPose.SetLocalTransform(nodeIndexA, finalTransformA);
            outTransformPose.SetLocalTransform(nodeIndexB, finalTransformB);
            outTransformPose.SetLocalTransform(nodeIndexC, finalTransformC);
        }

        // render some debug lines
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            const uint32 color = mVisualizeColor;
            GetEventManager().OnDrawLine(outTransformPose.GetGlobalTransformInclusive(nodeIndexA).mPosition, outTransformPose.GetGlobalTransformInclusive(nodeIndexB).mPosition, color);
            GetEventManager().OnDrawLine(outTransformPose.GetGlobalTransformInclusive(nodeIndexB).mPosition, outTransformPose.GetGlobalTransformInclusive(nodeIndexC).mPosition, color);
        }
    #endif
    }


    // get the type string
    const char* BlendTreeTwoLinkIKNode::GetTypeString() const
    {
        return "BlendTreeTwoLinkIKNode";
    }


    // init
    void BlendTreeTwoLinkIKNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
        //mOutputPose.InitToBindPose( animGraphInstance->GetActorInstance() );
    }


    // update the parameter contents, such as combobox values
    void BlendTreeTwoLinkIKNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find our unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->mMustUpdate = true;
        UpdateUniqueData(animGraphInstance, uniqueData);
    }


    // update attributes
    void BlendTreeTwoLinkIKNode::OnUpdateAttributes()
    {
        // disable GUI items that have no influence
    #ifdef EMFX_EMSTUDIOBUILD
        EnableAllAttributes(true);

        // disable relative bend dir when extract bend dir is enabled
        if (GetAttributeFloatAsBool(ATTRIB_EXTRACTBENDDIR))
        {
            SetAttributeDisabled(ATTRIB_RELBENDDIR);
        }
    #endif
    }
} // namespace EMotionFX

