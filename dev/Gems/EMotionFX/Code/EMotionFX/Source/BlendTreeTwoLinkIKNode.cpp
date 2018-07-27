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
#include <EMotionFX/Source/AnimGraph.h>
#include "BlendTreeTwoLinkIKNode.h"
#include "EventManager.h"
#include "AnimGraphManager.h"
#include "Recorder.h"
#include "Node.h"
#include "TransformData.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeTwoLinkIKNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeTwoLinkIKNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)


    BlendTreeTwoLinkIKNode::BlendTreeTwoLinkIKNode()
        : AnimGraphNode()
        , m_rotationEnabled(false)
        , m_relativeBendDir(true)
        , m_extractBendDir(false)
    {
        // setup the input ports
        InitInputPorts(5);
        SetupInputPort("Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPort("Goal Pos", INPUTPORT_GOALPOS, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_GOALPOS);
        SetupInputPort("Bend Dir", INPUTPORT_BENDDIR, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_BENDDIR);
        SetupInputPort("Goal Rot", INPUTPORT_GOALROT, AttributeRotation::TYPE_ID, PORTID_INPUT_GOALROT);
        SetupInputPortAsNumber("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    BlendTreeTwoLinkIKNode::~BlendTreeTwoLinkIKNode()
    {
    }


    void BlendTreeTwoLinkIKNode::Reinit()
    {
        if (!mAnimGraph)
        {
            return;
        }

        AnimGraphNode::Reinit();

        const size_t numInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);

            UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));
            if (!uniqueData)
            {
                continue;
            }

            uniqueData->mMustUpdate = true;
            animGraphInstance->UpdateUniqueData();
        }
    }


    bool BlendTreeTwoLinkIKNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeTwoLinkIKNode::GetPaletteName() const
    {
        return "TwoLink IK";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeTwoLinkIKNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
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
            if (m_endNodeName.empty())
            {
                return;
            }
            const Node* nodeC = skeleton->FindNodeByName(m_endNodeName.c_str());
            if (!nodeC)
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
            const Node* endEffectorNode = skeleton->FindNodeByName(m_endEffectorNodeName.c_str());
            if (endEffectorNode)
            {
                uniqueData->mEndEffectorNodeIndex = endEffectorNode->GetNodeIndex();
            }

            // find the bend direction node
            const Node* bendDirNode = skeleton->FindNodeByName(m_bendDirNodeName.c_str());
            if (bendDirNode)
            {
                uniqueData->mBendDirNodeIndex = bendDirNode->GetNodeIndex();
            }

            // lookup the actor instance to get the alignment node from
            const ActorInstance* alignInstance = animGraphInstance->FindActorInstanceFromParentDepth(m_alignToNode.second);
            if (alignInstance)
            {
                if (!m_alignToNode.first.empty())
                {
                    const Node* alignNode = alignInstance->GetActor()->GetSkeleton()->FindNodeByName(m_alignToNode.first.c_str());
                    if (alignNode)
                    {
                        uniqueData->mAlignNodeIndex = alignNode->GetNodeIndex();
                    }
                }
            }

            uniqueData->mIsValid = true;
        }
    }


    // solve the IK problem by calculating the 'knee/elbow' position
    bool BlendTreeTwoLinkIKNode::Solve2LinkIK(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& goal, const AZ::Vector3& bendDir, AZ::Vector3* outMidPos)
    {
        const AZ::Vector3 localGoal = goal - posA;
        const float rLen = MCore::SafeLength(localGoal);

        // get the lengths of the bones A and B
        const float lengthA = MCore::SafeLength(posB - posA);
        const float lengthB = MCore::SafeLength(posC - posB);

        // calculate the d and e values from the equations by Ken Perlin
        const float d = (rLen > MCore::Math::epsilon) ? MCore::Max<float>(0.0f, MCore::Min<float>(lengthA, (rLen + (lengthA * lengthA - lengthB * lengthB) / rLen) * 0.5f)) : MCore::Max<float>(0.0f, MCore::Min<float>(lengthA, rLen));
        const float square = lengthA * lengthA - d * d;
        const float e = MCore::Math::SafeSqrt(square);

        // the solution on the YZ plane
        const AZ::Vector3 solution(d, e, 0);

        // calculate the matrix that rotates from IK solve space into global space
        MCore::Matrix matForward;
        matForward.Identity();
        CalculateMatrix(localGoal, bendDir, &matForward);

        // rotate the solution (the mid "knee/elbow" position) into global space
        *outMidPos = posA + matForward.Mul3x3(solution);

        // check if we found a solution or not
        return (d > MCore::Math::epsilon && d < lengthA + MCore::Math::epsilon);
    }


    // calculate the direction matrix
    void BlendTreeTwoLinkIKNode::CalculateMatrix(const AZ::Vector3& goal, const AZ::Vector3& bendDir, MCore::Matrix* outForward)
    {
        // the inverse matrix defines a coordinate system whose x axis contains P, so X = unit(P).
        const AZ::Vector3 x = MCore::SafeNormalize(goal);

        // the y axis of the inverse is perpendicular to P, so Y = unit( D - X(D�X) ).
        const float dot = bendDir.Dot(x);
        const AZ::Vector3 y = MCore::SafeNormalize(bendDir - (dot * x));

        // the z axis of the inverse is perpendicular to both X and Y, so Z = X�Y.
        const AZ::Vector3 z = x.Cross(y);

        // set the rotation vectors of the output matrix
        outForward->SetRow(0, x);
        outForward->SetRow(1, y);
        outForward->SetRow(2, z);
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
        AZ::Vector3 goal = (inputGoalPos) ? AZ::Vector3(inputGoalPos->GetValue()) : AZ::Vector3::CreateZero();

        // there is no error, as we have all we need to solve this
    #ifdef EMFX_EMSTUDIOBUILD
        SetHasError(animGraphInstance, false);
    #endif

        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        ActorInstance* alignInstance = actorInstance;

        // adjust the gizmo offset value
        if (alignNodeIndex != MCORE_INVALIDINDEX32)
        {
            // update the alignment actor instance
            alignInstance = animGraphInstance->FindActorInstanceFromParentDepth(m_alignToNode.second);
            if (alignInstance)
            {
                const AZ::Vector3& offset = alignInstance->GetTransformData()->GetCurrentPose()->GetGlobalTransformInclusive(alignNodeIndex).mPosition;
                goal += offset;

            #ifdef EMFX_EMSTUDIOBUILD
                // check if the offset goal pos values comes from a param node
                const BlendTreeConnection* posConnection = GetInputPort(INPUTPORT_GOALPOS).mConnection;
                if (posConnection)
                {
                    if (azrtti_typeid(posConnection->GetSourceNode()) == azrtti_typeid<BlendTreeParameterNode>())
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
                if (azrtti_typeid(posConnection->GetSourceNode()) == azrtti_typeid<BlendTreeParameterNode>())
                {
                    BlendTreeParameterNode* parameterNode = static_cast<BlendTreeParameterNode*>(posConnection->GetSourceNode());
                    GetEventManager().OnSetVisualManipulatorOffset(animGraphInstance, parameterNode->GetParameterIndex(posConnection->GetSourcePort()), AZ::Vector3::CreateZero());
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
        AZ::Vector3 bendDir;
        if (m_extractBendDir)
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
            bendDir = (inputBendDir) ? AZ::Vector3(inputBendDir->GetValue()) : AZ::Vector3(0.0f, 0.0f, -1.0f);
        }

        // if we want a relative bend dir, rotate it with the actor (only do this if we don't extract the bend dir)
        if (m_relativeBendDir && !m_extractBendDir)
        {
            bendDir = actorInstance->GetGlobalTransform().mRotation * bendDir;
            bendDir = MCore::SafeNormalize(bendDir);
        }
        else
        {
            bendDir = MCore::SafeNormalize(bendDir);
        }

        // if end node rotation is enabled
        if (m_rotationEnabled)
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
        AZ::Vector3 endEffectorNodePos = outTransformPose.GetGlobalTransformInclusive(endEffectorNodeIndex).mPosition;
        const AZ::Vector3 posCToEndEffector = endEffectorNodePos - globalTransformC.mPosition;
        if (m_rotationEnabled)
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

            AZ::Vector3 realGoal;
            if (m_rotationEnabled)
            {
                realGoal = goal + posCToEndEffector;
            }
            else
            {
                realGoal = goal;
            }

            uint32 color = mVisualizeColor;
            GetEventManager().OnDrawLine(realGoal - AZ::Vector3(s, 0, 0), realGoal + AZ::Vector3(s, 0, 0), color);
            GetEventManager().OnDrawLine(realGoal - AZ::Vector3(0, s, 0), realGoal + AZ::Vector3(0, s, 0), color);
            GetEventManager().OnDrawLine(realGoal - AZ::Vector3(0, 0, s), realGoal + AZ::Vector3(0, 0, s), color);

            color = MCore::RGBA(0, 255, 255);
            GetEventManager().OnDrawLine(globalTransformA.mPosition, globalTransformA.mPosition + bendDir * s * 2.5f, color);
            GetEventManager().OnDrawLine(globalTransformA.mPosition - AZ::Vector3(s, 0, 0), globalTransformA.mPosition + AZ::Vector3(s, 0, 0), color);
            GetEventManager().OnDrawLine(globalTransformA.mPosition - AZ::Vector3(0, s, 0), globalTransformA.mPosition + AZ::Vector3(0, s, 0), color);
            GetEventManager().OnDrawLine(globalTransformA.mPosition - AZ::Vector3(0, 0, s), globalTransformA.mPosition + AZ::Vector3(0, 0, s), color);

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
        AZ::Vector3 midPos;
        if (m_rotationEnabled)
        {
            Solve2LinkIK(globalTransformA.mPosition, globalTransformB.mPosition, globalTransformC.mPosition, goal, bendDir, &midPos);
        }
        else
        {
            Solve2LinkIK(globalTransformA.mPosition, globalTransformB.mPosition, endEffectorNodePos, goal, bendDir, &midPos);
        }

        // --------------------------------------
        // calculate the new node transforms
        // --------------------------------------
        // calculate the differences between the current forward vector and the new one after IK
        AZ::Vector3 oldForward = globalTransformB.mPosition - globalTransformA.mPosition;
        AZ::Vector3 newForward = midPos - globalTransformA.mPosition;
        oldForward = MCore::SafeNormalize(oldForward);
        newForward = MCore::SafeNormalize(newForward);

        // perform a delta rotation to rotate into the new direction after IK
        float deltaAngle = MCore::Math::ACos(oldForward.Dot(newForward));
        AZ::Vector3 axis = oldForward.Cross(newForward);
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
        if (m_rotationEnabled)
        {
            oldForward = globalTransformC.mPosition - globalTransformB.mPosition;
        }
        else
        {
            oldForward = endEffectorNodePos - globalTransformB.mPosition;
        }

        oldForward = MCore::SafeNormalize(oldForward);
        newForward = goal - globalTransformB.mPosition;
        newForward = MCore::SafeNormalize(newForward);

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
        if (m_rotationEnabled)
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


    // update the parameter contents, such as combobox values
    void BlendTreeTwoLinkIKNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find our unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->mMustUpdate = true;
        UpdateUniqueData(animGraphInstance, uniqueData);
    }


    AZ::Crc32 BlendTreeTwoLinkIKNode::GetRelativeBendDirVisibility() const
    {
        return m_extractBendDir ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    void BlendTreeTwoLinkIKNode::SetRelativeBendDir(bool relativeBendDir)
    {
        m_relativeBendDir = relativeBendDir;
    }
    
    void BlendTreeTwoLinkIKNode::SetExtractBendDir(bool extractBendDir)
    {
        m_extractBendDir = extractBendDir;
    }
    
    void BlendTreeTwoLinkIKNode::SetRotationEnabled(bool rotationEnabled)
    {
        m_rotationEnabled = rotationEnabled;
    }

    void BlendTreeTwoLinkIKNode::SetBendDirNodeName(const AZStd::string& bendDirNodeName)
    {
        m_bendDirNodeName = bendDirNodeName;
    }

    void BlendTreeTwoLinkIKNode::SetAlignToNode(const NodeAlignmentData& alignToNode)
    {
        m_alignToNode = alignToNode;
    }

    void BlendTreeTwoLinkIKNode::SetEndEffectorNodeName(const AZStd::string& endEffectorNodeName)
    {
        m_endEffectorNodeName = endEffectorNodeName;
    }

    void BlendTreeTwoLinkIKNode::SetEndNodeName(const AZStd::string& endNodeName)
    {
        m_endNodeName = endNodeName;
    }

    void BlendTreeTwoLinkIKNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeTwoLinkIKNode, AnimGraphNode>()
            ->Field("endNodeName", &BlendTreeTwoLinkIKNode::m_endNodeName)
            ->Field("endEffectorNodeName", &BlendTreeTwoLinkIKNode::m_endEffectorNodeName)
            ->Field("alignToNode", &BlendTreeTwoLinkIKNode::m_alignToNode)
            ->Field("bendDirNodeName", &BlendTreeTwoLinkIKNode::m_bendDirNodeName)
            ->Field("rotationEnabled", &BlendTreeTwoLinkIKNode::m_rotationEnabled)
            ->Field("relativeBendDir", &BlendTreeTwoLinkIKNode::m_relativeBendDir)
            ->Field("extractBendDir", &BlendTreeTwoLinkIKNode::m_extractBendDir)
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeTwoLinkIKNode>("Two Link IK", "Two Link IK attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeTwoLinkIKNode::m_endNodeName, "End Node", "The end node name of the chain, for example the foot, or hand.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeTwoLinkIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeTwoLinkIKNode::m_endEffectorNodeName, "End Effector", "The end effector node, which represents the node that actually tries to reach the goal. This is probably also the hand, or a child node of it for example. If not set, the end node is used.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeTwoLinkIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("ActorGoalNode", 0xaf1e8a3a), &BlendTreeTwoLinkIKNode::m_alignToNode, "Align To", "The node to align the end node to. This basically sets the goal to this node.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeTwoLinkIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeTwoLinkIKNode::m_bendDirNodeName, "Bend Dir Node", "The optional node to control the bend direction. The vector from the start node to the bend dir node will be used as bend direction.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeTwoLinkIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTwoLinkIKNode::m_rotationEnabled, "Enable Rotation Goal", "Enable the goal orientation?")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTwoLinkIKNode::m_relativeBendDir, "Relative Bend Dir", "Use a relative (to the actor instance) bend direction, instead of global space?")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeTwoLinkIKNode::GetRelativeBendDirVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTwoLinkIKNode::m_extractBendDir, "Extract Bend Dir", "Extract the bend direction from the input pose instead of using the bend dir input value?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
        ;
    }
} // namespace EMotionFX