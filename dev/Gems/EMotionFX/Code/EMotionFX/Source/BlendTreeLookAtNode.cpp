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
#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/Compare.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/BlendTreeLookAtNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ConstraintTransformRotationAngles.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeLookAtNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeLookAtNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    BlendTreeLookAtNode::BlendTreeLookAtNode()
        : AnimGraphNode()
        , m_constraintRotation(AZ::Quaternion::CreateIdentity())
        , m_postRotation(AZ::Quaternion::CreateIdentity())
        , m_limitMin(-90.0f, -50.0f)
        , m_limitMax(90.0f, 30.0f)
        , m_followSpeed(0.75f)
        , m_twistAxis(ConstraintTransformRotationAngles::AXIS_Y)
        , m_limitsEnabled(false)
    {
        // setup the input ports
        InitInputPorts(3);
        SetupInputPort("Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPort("Goal Pos", INPUTPORT_GOALPOS, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_GOALPOS);
        SetupInputPortAsNumber("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    BlendTreeLookAtNode::~BlendTreeLookAtNode()
    {
    }


    void BlendTreeLookAtNode::Reinit()
    {
        const size_t numAnimGraphInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);

            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            if (uniqueData)
            {
                uniqueData->mMustUpdate = true;
            }

            OnUpdateUniqueData(animGraphInstance);
        }

        AnimGraphNode::Reinit();
    }


    bool BlendTreeLookAtNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeLookAtNode::GetPaletteName() const
    {
        return "LookAt";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeLookAtNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    // pre-create unique data object
    void BlendTreeLookAtNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->mMustUpdate = true;
        UpdateUniqueData(animGraphInstance, uniqueData);
    }


    // the main process method of the final node
    void BlendTreeLookAtNode::Output(AnimGraphInstance* animGraphInstance)
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

        // if the weight is near zero, we can skip all calculations and act like a pass-trough node
        if (weight < MCore::Math::epsilon || mDisabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            UpdateUniqueData(animGraphInstance, uniqueData); // update the unique data (lookup node indices when something changed)
            uniqueData->mFirstUpdate = true;
            return;
        }

        // get the input pose and copy it over to the output pose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        *outputPose = *inputPose;

        //------------------------------------
        // get the node indices to work on
        //------------------------------------
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        UpdateUniqueData(animGraphInstance, uniqueData); // update the unique data (lookup node indices when something changed)
        if (uniqueData->mIsValid == false)
        {
        #ifdef EMFX_EMSTUDIOBUILD
            uniqueData->mMustUpdate = true;
            UpdateUniqueData(animGraphInstance, uniqueData);

            if (uniqueData->mIsValid == false)
            {
                SetHasError(animGraphInstance, true);
            }
        #endif
            return;
        }

        // there is no error
    #ifdef EMFX_EMSTUDIOBUILD
        SetHasError(animGraphInstance, false);
    #endif

        // get the goal
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALPOS));
        const MCore::AttributeVector3* inputGoalPos = GetInputVector3(animGraphInstance, INPUTPORT_GOALPOS);
        AZ::Vector3 goal = (inputGoalPos) ? AZ::Vector3(inputGoalPos->GetValue()) : AZ::Vector3::CreateZero();

        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // get a shortcut to the local transform object
        const uint32 nodeIndex = uniqueData->mNodeIndex;

        Pose& outTransformPose = outputPose->GetPose();
        Transform globalTransform = outTransformPose.GetGlobalTransformInclusive(nodeIndex);

        Skeleton* skeleton = actorInstance->GetActor()->GetSkeleton();

        // Prevent invalid float values inside the LookAt matrix construction when both position and goal are the same
        const AZ::Vector3 diff = globalTransform.mPosition - goal;
        if (diff.GetLengthSq() < AZ::g_fltEps)
        {
            goal += AZ::Vector3(0.0f, 0.000001f, 0.0f);
        }

        // calculate the lookat transform
        // TODO: a quaternion lookat function would be nicer, so that there are no matrix operations involved
        MCore::Matrix lookAt;
        lookAt.LookAt(globalTransform.mPosition, goal, AZ::Vector3(0.0f, 0.0f, 1.0f));
        MCore::Quaternion destRotation = lookAt.Transposed();   // implicit matrix to quat conversion

        // apply the post rotation
        MCore::Quaternion postRotation = MCore::AzQuatToEmfxQuat(m_postRotation);
        destRotation = destRotation * postRotation;

        // get the constraint rotation
        const MCore::Quaternion constraintRotation = MCore::AzQuatToEmfxQuat(m_constraintRotation);

        if (m_limitsEnabled)
        {
            // calculate the delta between the bind pose rotation and current target rotation and constraint that to our limits
            const uint32 parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
            MCore::Quaternion parentRotationGlobal;
            MCore::Quaternion bindRotationLocal;
            if (parentIndex != MCORE_INVALIDINDEX32)
            {               
                parentRotationGlobal = inputPose->GetPose().GetGlobalTransformInclusive(parentIndex).mRotation;
                bindRotationLocal    = actorInstance->GetTransformData()->GetBindPose()->GetLocalTransform(parentIndex).mRotation;
            }
            else
            {
                parentRotationGlobal.Identity();
                bindRotationLocal.Identity();
            }

            const MCore::Quaternion destRotationLocal = destRotation * parentRotationGlobal.Conjugated();
            const MCore::Quaternion deltaRotLocal     = destRotationLocal * bindRotationLocal.Conjugated();

            // setup the constraint and execute it
            // the limits are relative to the bind pose in local space
            ConstraintTransformRotationAngles constraint;   // TODO: place this inside the unique data? would be faster, but takes more memory, or modify the constraint to support internal modification of a transform directly
            constraint.SetMinRotationAngles(m_limitMin);
            constraint.SetMaxRotationAngles(m_limitMax);
            constraint.SetMinTwistAngle(0.0f);
            constraint.SetMaxTwistAngle(0.0f);
            constraint.SetTwistAxis(m_twistAxis);
            constraint.GetTransform().mRotation = (deltaRotLocal * constraintRotation.Conjugated());
            constraint.Execute();

            #ifdef EMFX_EMSTUDIOBUILD
                if (GetCanVisualize(animGraphInstance))
                {
                    MCore::Matrix offset = (postRotation.Inversed() * bindRotationLocal * constraintRotation * parentRotationGlobal).ToMatrix();
                    offset.SetTranslation(globalTransform.mPosition);
                    constraint.DebugDraw(offset, GetVisualizeColor(), 0.5f);
                }
            #endif

            // convert back into global space
            destRotation = (bindRotationLocal * (constraint.GetTransform().mRotation * constraintRotation)) * parentRotationGlobal;
        }

        // init the rotation quaternion to the initial rotation
        if (uniqueData->mFirstUpdate)
        {
            uniqueData->mRotationQuat = destRotation;
            uniqueData->mFirstUpdate = false;
        }

        // interpolate between the current rotation and the destination rotation
        const float speed = m_followSpeed * uniqueData->mTimeDelta * 10.0f;
        if (speed < 1.0f)
        {
            uniqueData->mRotationQuat = uniqueData->mRotationQuat.Slerp(destRotation, speed);
        }
        else
        {
            uniqueData->mRotationQuat = destRotation;
        }

        uniqueData->mRotationQuat.Normalize();
        globalTransform.mRotation = uniqueData->mRotationQuat;

        // only blend when needed
        if (weight < 0.999f)
        {
            outTransformPose.SetGlobalTransformInclusive(nodeIndex, globalTransform);
            outTransformPose.UpdateLocalTransform(nodeIndex);

            const Pose& inputTransformPose = inputPose->GetPose();
            Transform finalTransform = inputTransformPose.GetLocalTransform(nodeIndex);
            finalTransform.Blend(outTransformPose.GetLocalTransform(nodeIndex), weight);

            outTransformPose.SetLocalTransform(nodeIndex, finalTransform);
        }
        else
        {
            outTransformPose.SetGlobalTransformInclusive(nodeIndex, globalTransform);
            outTransformPose.UpdateLocalTransform(nodeIndex);
        }

        // perform some debug rendering
        #ifdef EMFX_EMSTUDIOBUILD
            if (GetCanVisualize(animGraphInstance))
            {
                const float s = animGraphInstance->GetVisualizeScale() * actorInstance->GetVisualizeScale();

                const uint32 color = mVisualizeColor;
                GetEventManager().OnDrawLine(goal - AZ::Vector3(s, 0, 0), goal + AZ::Vector3(s, 0, 0), color);
                GetEventManager().OnDrawLine(goal - AZ::Vector3(0, s, 0), goal + AZ::Vector3(0, s, 0), color);
                GetEventManager().OnDrawLine(goal - AZ::Vector3(0, 0, s), goal + AZ::Vector3(0, 0, s), color);

                const AZ::Vector3& pos = globalTransform.mPosition;
                GetEventManager().OnDrawLine(pos, goal, mVisualizeColor);

                GetEventManager().OnDrawLine(globalTransform.mPosition, globalTransform.mPosition + globalTransform.mRotation.CalcUpAxis() * s * 50.0f, MCore::RGBA(0, 0, 255));
            }
        #endif
    }


    // update the unique data
    void BlendTreeLookAtNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // update the unique data if needed
        if (uniqueData->mMustUpdate)
        {
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            Actor* actor = actorInstance->GetActor();

            // don't update the next time again
            uniqueData->mMustUpdate = false;

            // get the node
            uniqueData->mNodeIndex  = MCORE_INVALIDINDEX32;
            uniqueData->mIsValid    = false;

            if (m_targetNodeName.empty())
            {
                return;
            }

            const Node* targetNode = actor->GetSkeleton()->FindNodeByName(m_targetNodeName.c_str());
            if (!targetNode)
            {
                return;
            }

            uniqueData->mNodeIndex  = targetNode->GetNodeIndex();
            uniqueData->mIsValid = true;
        }
    }


    // update
    void BlendTreeLookAtNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update the weight node
        AnimGraphNode* weightNode = GetInputNode(INPUTPORT_WEIGHT);
        if (weightNode)
        {
            UpdateIncomingNode(animGraphInstance, weightNode, timePassedInSeconds);
        }

        // update the goal node
        AnimGraphNode* goalNode = GetInputNode(INPUTPORT_GOALPOS);
        if (goalNode)
        {
            UpdateIncomingNode(animGraphInstance, goalNode, timePassedInSeconds);
        }

        // update the pose node
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE);
        if (inputNode)
        {
            UpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);

            // update the sync track
            uniqueData->Init(animGraphInstance, inputNode);
        }
        else
        {
            uniqueData->Clear();
        }

        uniqueData->mTimeDelta = timePassedInSeconds;
    }


    AZ::Crc32 BlendTreeLookAtNode::GetLimitWidgetsVisibility() const
    {
        return m_limitsEnabled ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }


    void BlendTreeLookAtNode::SetTargetNodeName(const AZStd::string& targetNodeName)
    {
        m_targetNodeName = targetNodeName;
    }

    void BlendTreeLookAtNode::SetConstraintRotation(const AZ::Quaternion& constraintRotation)
    {
        m_constraintRotation = constraintRotation;
    }

    void BlendTreeLookAtNode::SetPostRotation(const AZ::Quaternion& postRotation)
    {
        m_postRotation = postRotation;
    }

    void BlendTreeLookAtNode::SetLimitMin(const AZ::Vector2& limitMin)
    {
        m_limitMin = limitMin;
    }

    void BlendTreeLookAtNode::SetLimitMax(const AZ::Vector2& limitMax)
    {
        m_limitMax = limitMax;
    }

    void BlendTreeLookAtNode::SetFollowSpeed(float followSpeed)
    {
        m_followSpeed = followSpeed;
    }

    void BlendTreeLookAtNode::SetTwistAxis(ConstraintTransformRotationAngles::EAxis twistAxis)
    {
        m_twistAxis = twistAxis;
    }

    void BlendTreeLookAtNode::SetLimitsEnabled(bool limitsEnabled)
    {
        m_limitsEnabled = limitsEnabled;
    }

    void BlendTreeLookAtNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeLookAtNode, AnimGraphNode>()
            ->Version(1)
            ->Field("targetNodeName", &BlendTreeLookAtNode::m_targetNodeName)
            ->Field("limitMin", &BlendTreeLookAtNode::m_limitMin)
            ->Field("limitMax", &BlendTreeLookAtNode::m_limitMax)
            ->Field("constraintRotation", &BlendTreeLookAtNode::m_constraintRotation)
            ->Field("postRotation", &BlendTreeLookAtNode::m_postRotation)
            ->Field("followSpeed", &BlendTreeLookAtNode::m_followSpeed)
            ->Field("twistAxis", &BlendTreeLookAtNode::m_twistAxis)
            ->Field("limitsEnabled", &BlendTreeLookAtNode::m_limitsEnabled)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeLookAtNode>("Look At", "Look At attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeLookAtNode::m_targetNodeName, "Node", "The node to apply the lookat to. For example the head.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeLookAtNode::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_limitMin, "Yaw/Pitch Min", "The minimum rotational yaw and pitch angle limits, in degrees.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeLookAtNode::GetLimitWidgetsVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, AZ::Vector2(-90.0f, -90.0f))
                ->Attribute(AZ::Edit::Attributes::Max, AZ::Vector2(90.0f, 90.0f))
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_limitMax, "Yaw/Pitch Max", "The maximum rotational yaw and pitch angle limits, in degrees.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeLookAtNode::GetLimitWidgetsVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, AZ::Vector2(-90.0f, -90.0f))
                ->Attribute(AZ::Edit::Attributes::Max, AZ::Vector2(90.0f, 90.0f))
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_constraintRotation, "Constraint Rotation", "A rotation that rotates the constraint space.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeLookAtNode::GetLimitWidgetsVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_postRotation, "Post Rotation", "The relative rotation applied after solving.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_followSpeed, "Follow Speed", "The speed factor at which to follow the goal. A value near zero meaning super slow and a value of 1 meaning instant following.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeLookAtNode::m_twistAxis, "Roll Axis", "The axis used for twist/roll.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeLookAtNode::GetLimitWidgetsVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_limitsEnabled, "Enable Limits", "Enable rotational limits?")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ;
    }
} // namespace EMotionFX