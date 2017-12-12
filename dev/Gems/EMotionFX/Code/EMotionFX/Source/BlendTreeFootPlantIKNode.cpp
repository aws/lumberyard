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
#include "BlendTreeFootPlantIKNode.h"
#include "BlendTreeParameterNode.h"
#include "Attachment.h"
#include "EventManager.h"
#include <MCore/Source/Random.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/MatrixDecomposer.h>
#include "AnimGraphManager.h"


namespace EMotionFX
{
    /*
    // constructor
    BlendTreeFootPlantIKNode::BlendTreeFootPlantIKNode(AnimGraph* animGraph) : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
    }


    // destructor
    BlendTreeFootPlantIKNode::~BlendTreeFootPlantIKNode()
    {
    }


    // create
    BlendTreeFootPlantIKNode* BlendTreeFootPlantIKNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeFootPlantIKNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeFootPlantIKNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void BlendTreeFootPlantIKNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts( 3 );
        SetupInputPort( "Pose",                 INPUTPORT_POSE,     AttributePose::TYPE_ID,             PORTID_INPUT_POSE );
        SetupInputPortAsNumber( "Foot Offset",  INPUTPORT_FOOTHEIGHTOFFSET, PORTID_INPUT_FOOTHEIGHTOFFSET );
        SetupInputPortAsNumber( "Weight",       INPUTPORT_WEIGHT,   PORTID_INPUT_WEIGHT );

        // setup the output ports
        InitOutputPorts( 1 );
        SetupOutputPortAsPose( "Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE );
    }


    // register the parameters
    void BlendTreeFootPlantIKNode::RegisterAttributes()
    {
        // LEFT LEG:
        // the foot node
        MCore::AttributeSettings* attrib = RegisterAttribute("Left Foot", "leftFootNode", "The left foot node.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetDefaultValue( MCore::AttributeString::Create() );
        attrib->SetReinitObjectOnValueChange(true);

        // the end effector node
        attrib = RegisterAttribute("Left End Effector", "leftEndEffectorNode", "The left end effector node (optional).", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetDefaultValue( MCore::AttributeString::Create() );
        attrib->SetReinitObjectOnValueChange(true);

        // the bend direction node
        attrib = RegisterAttribute("Left Bend Dir Node", "leftBendDirNode", "The left bend direciton node (optional). The vector from the start node to the bend dir node will be used as bend direction.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetDefaultValue( AttributeString::Create() );
        attrib->SetReinitObjectOnValueChange(true);

        // RIGHT LEG:
        // the foot node
        attrib = RegisterAttribute("Right Foot", "rightFootNode", "The right foot node.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetDefaultValue( MCore::AttributeString::Create() );
        attrib->SetReinitObjectOnValueChange(true);

        // the end effector node
        attrib = RegisterAttribute("Right End Effector", "rightEndEffectorNode", "The right end effector node (optional).", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetDefaultValue( MCore::AttributeString::Create() );
        attrib->SetReinitObjectOnValueChange(true);

        // the bend direction node
        attrib = RegisterAttribute("Right Bend Dir Node", "rightBendDirNode", "The right bend direciton node (optional). The vector from the start node to the bend dir node will be used as bend direction.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetDefaultValue( AttributeString::Create() );
        attrib->SetReinitObjectOnValueChange(true);

        // the hip node
        attrib = RegisterAttribute("Pelvis Node", "pelvisNode", "The pelvis node, which can be moved vertically by the footplant system (optional).", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetDefaultValue( AttributeString::Create() );
        attrib->SetReinitObjectOnValueChange(true);

        // extract the bend direction from the input pose?
        attrib = RegisterAttribute("Displace Pelvis", "displacePelvis", "Allow pelvis correction for a more natural pose?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue( MCore::AttributeFloat::Create(1) );
    }


    // get the palette name
    const char* BlendTreeFootPlantIKNode::GetPaletteName() const
    {
        return "FootPlant IK";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFootPlantIKNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeFootPlantIKNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeFootPlantIKNode* clone = new BlendTreeFootPlantIKNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo( clone );

        // return a pointer to the clone
        return clone;
    }


    // build the update path from the end effector towards the foot
    bool BlendTreeFootPlantIKNode::BuildEndEffectorUpdatePath(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, uint32 legIndex)
    {
        // get the actor
        const Actor* actor = animGraphInstance->GetActorInstance()->GetActor();

        // get the leg info
        UniqueData::LegInfo& legInfo = uniqueData->mLegInfos[legIndex];

        // clear the array
        legInfo.mUpdatePath.Clear(false);

        // if no end node or end effector has been set
        if (legInfo.mEndEffectorNodeIndex == MCORE_INVALIDINDEX32)
        {
            if (legInfo.mNodeIndexC == MCORE_INVALIDINDEX32)
                return false;
            else
                return true;
        }

        // start at the end effector
        Node* currentNode   = actor->GetNode( legInfo.mEndEffectorNodeIndex );
        Node* endNode       = actor->GetNode( legInfo.mNodeIndexC );
        while (currentNode != endNode)
        {
            // add the current node to the update list
            legInfo.mUpdatePath.Add( currentNode->GetNodeIndex() );

            // move up the hierarchy, towards the root and end node
            currentNode = currentNode->GetParentNode();
            if (currentNode == nullptr) // we didnt find the end node, so this is not a valid end effector
                return false;
        }

        return true;
    }


    // update the unique data
    void BlendTreeFootPlantIKNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // update the unique data if needed
        if (uniqueData->mMustUpdate == false)
            return;

        // update the pelvis node
        const char* pelvisNodeName = GetAttributeString(ATTRIB_PELVISNODE)->AsChar();
        if (pelvisNodeName)
        {
            const Node* pelvisNode = animGraphInstance->GetActorInstance()->GetActor()->FindNodeByName(pelvisNodeName);
            uniqueData->mPelvisIndex = (pelvisNode) ? pelvisNode->GetNodeIndex() : MCORE_INVALIDINDEX32;
        }
        else
            uniqueData->mPelvisIndex = MCORE_INVALIDINDEX32;

        // don't update the next time again
        uniqueData->mMustUpdate = false;

        // get the leg info
        for (uint32 legIndex=0; legIndex<NUMLEGS; ++legIndex)
        {
            UniqueData::LegInfo& legInfo = uniqueData->mLegInfos[legIndex];

            const uint32 attributeOffset = legIndex * (ATTRIB_LEFTBENDDIRNODE+1);

            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            Actor* actor = actorInstance->GetActor();

            // get the first node
            legInfo.mNodeIndexA         = MCORE_INVALIDINDEX32;
            legInfo.mNodeIndexB         = MCORE_INVALIDINDEX32;
            legInfo.mNodeIndexC         = MCORE_INVALIDINDEX32;
            legInfo.mBendDirNodeIndex   = MCORE_INVALIDINDEX32;
            legInfo.mEndEffectorNodeIndex = MCORE_INVALIDINDEX32;
            legInfo.mIsValid            = false;
            legInfo.mRootUpdatePath.Clear(false);
            legInfo.mBendDirUpdatePath.Clear(false);
            legInfo.mUpdatePath.Clear(false);

            // find the end node
            const char* nodeName = GetAttributeString(ATTRIB_LEFTFOOTNODE + attributeOffset)->AsChar();
            if (nodeName == nullptr)
            {
                BuildEndEffectorUpdatePath(animGraphInstance, uniqueData, legIndex);
                continue;
            }

            const Node* nodeC = actor->FindNodeByName( nodeName );
            if (nodeC == nullptr)
            {
                BuildEndEffectorUpdatePath(animGraphInstance, uniqueData, legIndex);
                continue;
            }

            legInfo.mNodeIndexC = nodeC->GetNodeIndex();

            // get the second node
            legInfo.mNodeIndexB = nodeC->GetParentIndex();
            if (legInfo.mNodeIndexB == MCORE_INVALIDINDEX32)
            {
                BuildEndEffectorUpdatePath(animGraphInstance, uniqueData, legIndex);
                continue;
            }

            // get the third node
            legInfo.mNodeIndexA = actor->GetNode( legInfo.mNodeIndexB )->GetParentIndex();
            if (legInfo.mNodeIndexA == MCORE_INVALIDINDEX32)
            {
                BuildEndEffectorUpdatePath(animGraphInstance, uniqueData, legIndex);
                continue;
            }

            const Node* endEffectorNode = actor->FindNodeByName( GetAttributeString(ATTRIB_LEFTENDEFFECTORNODE + attributeOffset)->AsChar() );
            if (endEffectorNode)
            {
                legInfo.mEndEffectorNodeIndex = endEffectorNode->GetNodeIndex();
                actor->GenerateUpdatePathToRoot( legInfo.mEndEffectorNodeIndex, legInfo.mRootUpdatePath );
            }
            else
                actor->GenerateUpdatePathToRoot( legInfo.mNodeIndexC, legInfo.mRootUpdatePath );

            //----------------------

            // find the bend direction node
            const char* bendDirNodeName = GetAttributeString(ATTRIB_LEFTBENDDIRNODE + attributeOffset)->AsChar();
            const Node* bendDirNode = actor->FindNodeByName( bendDirNodeName );
            if (bendDirNode)
            {
                legInfo.mBendDirNodeIndex = bendDirNode->GetNodeIndex();
                actor->GenerateUpdatePathToRoot( legInfo.mBendDirNodeIndex, legInfo.mBendDirUpdatePath );
            }

            //----------------------

            // update path from end effector to end node
            if (BuildEndEffectorUpdatePath(animGraphInstance, uniqueData, legIndex) == false)
                legInfo.mIsValid = false;
            else
                legInfo.mIsValid = true;
        }
    }
    */
    /*
    MCore::Quaternion RotateTowards(const MCore::Vector3& lookAt, const MCore::Vector3& upDirection)
    {
        MCore::Vector3 forward  = lookAt;
        MCore::Vector3 up       = upDirection;
        forward = up.Cross( up.Cross(forward) );
        //MCore::Vector::OrthoNormalize(&forward, &up);
        MCore::Vector3 right = up.Cross(forward);

        float recip;
        Quaternion ret;
        ret.w = MCore::Math::SafeSqrt(1.0f + right.x + up.y + forward.z) * 0.5f;
        if (MCore::Math::Abs(ret.w) > MCore::Math::epsilon)
            recip = 1.0f / (4.0f * ret.w);
        else
            recip = 1.0f;

        ret.x = (up.z - forward.y) * recip;
        ret.y = (forward.x - right.z) * recip;
        ret.z = (right.y - up.x) * recip;

        return ret;
    }
    */
    /*
    // the main process method of the final node
    void BlendTreeFootPlantIKNode::Output(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(threadIndex);

        AnimGraphPose* outputPose;

        // make sure we have at least an input pose, otherwise output the bind pose
        if (GetInputPort(INPUTPORT_POSE).mConnection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitToBindPose( actorInstance );
            return;
        }

        // get the weight
        float weight = 1.0f;
        if (GetInputPort(INPUTPORT_WEIGHT).mConnection)
        {
            OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_WEIGHT) );
            weight = GetInputNumberAsFloat(INPUTPORT_WEIGHT);
            weight = MCore::Clamp<float>( weight, 0.0f, 1.0f );
        }

        // if the IK weight is near zero, we can skip all calculations and act like a pass-trough node
        if (weight < MCore::Math::epsilon || mDisabled==true)
        {
            OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_POSE) );
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(OUTPUTPORT_POSE)->GetValue();
            const AnimGraphPose* inputPose = GetInputPose(INPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
            return;
        }

        // get the input pose and copy it over to the output pose
        OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_POSE) );
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(OUTPUTPORT_POSE)->GetValue();
        const AnimGraphPose* inputPose = GetInputPose(INPUTPORT_POSE)->GetValue();
        *outputPose = *inputPose;

        //------------------------------------
        // get the node indices to work on
        //------------------------------------
        UniqueData* uniqueData = static_cast<UniqueData*>( FindUniqueNodeData(animGraphInstance) );
        UpdateUniqueData( animGraphInstance, uniqueData );  // update the unique data (lookup node indices when something changed)
        for (uint32 legIndex=0; legIndex<NUMLEGS; ++legIndex)
        {
            if (uniqueData->mLegInfos[legIndex].mIsValid == false)
            {
                #ifdef EMFX_EMSTUDIOBUILD
                    SetHasError(animGraphInstance, true);
                #endif
                return;
            }
        }

        // there is no error, as we have all we need to solve this
        #ifdef EMFX_EMSTUDIOBUILD
            SetHasError( animGraphInstance, false );
        #endif

        // get the foot height offset
        OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_FOOTHEIGHTOFFSET) );
        const float footHeightOffset = GetInputNumberAsFloat(INPUTPORT_FOOTHEIGHTOFFSET);

        // calculate the global space matrices
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        Matrix* globalMatrices = actorInstance->GetTransformData()->GetGlobalMatrices();
        Matrix* localMatrices = actorInstance->GetTransformData()->GetLocalMatrices();

        // adjust the pelvis
        const uint32 pelvisIndex = uniqueData->mPelvisIndex;
        const uint32 upIndex = MCore::GetCoordinateSystem().GetUpIndex();
        if (pelvisIndex != MCORE_INVALIDINDEX32)
        {
            actorInstance->CalcGlobalMatricesFromRoot(&outputPose->GetPose(), localMatrices, globalMatrices, uniqueData->mLegInfos[0].mRootUpdatePath );

            IntersectionInfo pelvisIntersectInfo;
            pelvisIntersectInfo.mIgnoreActorInstance = actorInstance;
            Vector3 rayStart = globalMatrices[pelvisIndex].GetTranslation();
            Vector3 rayEnd = rayStart;
            rayEnd[upIndex] -= 100000.0f;
            if (GetEventManager().OnRayIntersectionTest(rayStart, rayEnd, &pelvisIntersectInfo))
            {
                // visualize the pelvis ray result
                #ifdef EMFX_EMSTUDIOBUILD
                    if (GetCanVisualize(animGraphInstance))
                        GetEventManager().OnDrawLine( rayStart, pelvisIntersectInfo.mPosition, MCore::RGBA(0, 255, 255));
                #endif

                // calculate the height from the ground until the pelvis
                const float currentHeight = (rayStart - pelvisIntersectInfo.mPosition).SafeLength();
                const float inputPosePelvisHeight = animGraphInstance->GetActorInstance()->GetActor()->GetInverseBindPoseGlobalMatrix(pelvisIndex).Inversed().GetTranslation()[upIndex];

                // if the height is smaller than the height in the input pose, compensate for it by moving the pelvis upwards
                // this keeps the animation integrity in tact
                if (currentHeight < inputPosePelvisHeight)
                {
                    MCore::Vector3 correctionVector;
                    correctionVector.Zero();
                    correctionVector[upIndex] = inputPosePelvisHeight - currentHeight;

                    Transform transform = outputPose->GetPose().GetLocalTransform(pelvisIndex);
                    transform.mPosition += correctionVector;
                    outputPose->GetPose().SetLocalTransform(pelvisIndex, transform);
                    //outputPose->GetPose().mTransforms[ pelvisIndex ].mPosition += correctionVector;
                }
            }
        }


        if (GetAttributeFloatAsBool(ATTRIB_DISPLACEPELVIS))
        {
            // trace rays to find out where to plant the feet, if needed
            float pelvisDisplacement = 0.0f;
            for (uint32 legIndex=0; legIndex<NUMLEGS; ++legIndex)
            {
                const UniqueData::LegInfo& legInfo = uniqueData->mLegInfos[legIndex];
                //const uint32 attributeOffset = legIndex * (ATTRIB_LEFTBENDDIRNODE+1);

                const uint32 nodeIndexA     = legInfo.mNodeIndexA;
                //const uint32 nodeIndexB       = legInfo.mNodeIndexB;
                const uint32 nodeIndexC     = legInfo.mNodeIndexC;
                uint32 endEffectorNodeIndex = legInfo.mEndEffectorNodeIndex;

                // use the end node as end effector node if no goal node has been specified
                if (endEffectorNodeIndex == MCORE_INVALIDINDEX32)
                    endEffectorNodeIndex = nodeIndexC;

                //------------------------------
                actorInstance->CalcGlobalMatricesFromRoot(&outputPose->GetPose(), localMatrices, globalMatrices, legInfo.mRootUpdatePath );
                //--------------------------------

                //------------------------------------
                // perform the calculation part
                //------------------------------------
                // get the positions of the nodes we perform calculations on
                Vector3 posA = globalMatrices[nodeIndexA].GetTranslation();
                //Vector3 posB = globalMatrices[nodeIndexB].GetTranslation();
                Vector3 posC = globalMatrices[nodeIndexC].GetTranslation();

                // get the goal
                //Vector3 goal(0.0f, 0.0f, 0.0f);
                IntersectionInfo intersectInfo;
                intersectInfo.mIgnoreActorInstance = actorInstance;
                Vector3 rayStart = posC;
                rayStart[upIndex] = posA[upIndex];
                Vector3 rayEnd = posC;
                rayEnd[upIndex] = -100000.0f;
                //bool doIK = false;
                if (GetEventManager().OnRayIntersectionTest(rayStart, rayEnd, &intersectInfo))
                {
                    float heightDiff = MCore::Math::Abs( intersectInfo.mPosition[upIndex] - (posC[upIndex] - footHeightOffset) );
                    if (heightDiff < 50.0f && intersectInfo.mPosition[upIndex] < posC[upIndex]) // TODO: make configurable
                        pelvisDisplacement = MCore::Max(pelvisDisplacement, heightDiff);
                }
            }


            // adjust pelvis again for a more natural look
            if (pelvisDisplacement > MCore::Math::epsilon)
            {
                MCore::Vector3 correctionVector;
                correctionVector.Zero();
                correctionVector[upIndex] = pelvisDisplacement;

                Transform transform = outputPose->GetPose().GetLocalTransform(pelvisIndex);
                transform.mPosition -= correctionVector;
                outputPose->GetPose().SetLocalTransform(pelvisIndex, transform);
                //outputPose->GetPose().mTransforms[ pelvisIndex ].mPosition -= correctionVector;

                #ifdef EMFX_EMSTUDIOBUILD
                    if (GetCanVisualize(animGraphInstance))
                    {
                        actorInstance->CalcGlobalMatricesFromRoot(&outputPose->GetPose(), localMatrices, globalMatrices, uniqueData->mLegInfos[0].mRootUpdatePath );
                        Vector3 rayStart = globalMatrices[pelvisIndex].GetTranslation() + Vector3(1.0f, 0.0f, 0.0f);
                        GetEventManager().OnDrawLine( rayStart, rayStart + correctionVector, MCore::RGBA(128, 255, 0));
                    }
                #endif
            }
        }


        for (uint32 legIndex=0; legIndex<NUMLEGS; ++legIndex)
        {
            const UniqueData::LegInfo& legInfo = uniqueData->mLegInfos[legIndex];
            //const uint32 attributeOffset = legIndex * (ATTRIB_LEFTBENDDIRNODE+1);

            const uint32 nodeIndexA     = legInfo.mNodeIndexA;
            const uint32 nodeIndexB     = legInfo.mNodeIndexB;
            const uint32 nodeIndexC     = legInfo.mNodeIndexC;
            const uint32 bendDirIndex   = legInfo.mBendDirNodeIndex;
            uint32 endEffectorNodeIndex = legInfo.mEndEffectorNodeIndex;

            // use the end node as end effector node if no goal node has been specified
            if (endEffectorNodeIndex == MCORE_INVALIDINDEX32)
                endEffectorNodeIndex = nodeIndexC;

            //------------------------------
            actorInstance->CalcGlobalMatricesFromRoot(&outputPose->GetPose(), localMatrices, globalMatrices, legInfo.mRootUpdatePath );
            //--------------------------------

            //------------------------------------
            // perform the calculation part
            //------------------------------------
            // get the positions of the nodes we perform calculations on
            Vector3 posA = globalMatrices[nodeIndexA].GetTranslation();
            Vector3 posB = globalMatrices[nodeIndexB].GetTranslation();
            Vector3 posC = globalMatrices[nodeIndexC].GetTranslation();

            // get the goal
            Vector3 goal(0.0f, 0.0f, 0.0f);
            IntersectionInfo intersectInfo;
            intersectInfo.mIgnoreActorInstance = actorInstance;
            Vector3 rayStart = posC;
            rayStart[upIndex] = posA[upIndex];
            Vector3 rayEnd = posC;
            rayEnd[upIndex] = -100000.0f;
            bool doIK = false;
            if (GetEventManager().OnRayIntersectionTest(rayStart, rayEnd, &intersectInfo))
            {
                if (intersectInfo.mPosition[upIndex] > posC[upIndex] - footHeightOffset)
                {
                    goal = intersectInfo.mPosition;
                    goal[upIndex] += footHeightOffset;
                    doIK = true;
                }
            }

            // extract the bend direction from the input pose?
            Vector3 bendDir;
            //const bool extractBendDir = GetAttributeFloat(ATTRIB_EXTRACTBENDDIR)->GetValue();
            //if (extractBendDir)
            {
                if (bendDirIndex != MCORE_INVALIDINDEX32)
                {
                    actorInstance->CalcGlobalMatricesFromRoot(&outputPose->GetPose(), localMatrices, globalMatrices, legInfo.mBendDirUpdatePath );
                    bendDir = globalMatrices[bendDirIndex].GetTranslation() - posA;
                }
                else
                    bendDir = posB - posA;
            }


            bendDir.Set( 0.0f, 0.0f, -1.0f );
            bendDir = actorInstance->GetGlobalTransform().mRotation * bendDir;

                bendDir.SafeNormalize();

            // get some write shortcuts to the output transforms of the nodes
            Pose& outLocalPose = outputPose->GetPose();

            Transform transformA = outLocalPose.GetLocalTransform( nodeIndexA );
            Transform transformB = outLocalPose.GetLocalTransform( nodeIndexB );
            Transform transformC = outLocalPose.GetLocalTransform( nodeIndexC );

            //Transform& transformA = outLocalPose.mTransforms[ nodeIndexA ];
            //Transform& transformB = outLocalPose.mTransforms[ nodeIndexB ];
            //Transform& transformC = outLocalPose.mTransforms[ nodeIndexC ];
            // update all the nodes from the end node (nodeC) towards the end effector
            // we need to do this in order to calculate the global space transform of the end effector, as this now changed by nodeA's rotation change
            uint32 curNodeIndex;
            Node* curNode;
            const Actor* actor = actorInstance->GetActor();
            for (int32 i=legInfo.mUpdatePath.GetLength()-1; i>=0; --i)
            {
                curNode         = actor->GetNode( legInfo.mUpdatePath[i] );
                curNodeIndex    = curNode->GetNodeIndex();
                globalMatrices[curNodeIndex].MultMatrix4x3(localMatrices[curNodeIndex], globalMatrices[curNode->GetParentIndex()]);
            }

            // adjust the goal and get the end effector position
            Vector3 endEffectorNodePos = globalMatrices[endEffectorNodeIndex].GetTranslation();
            //const Vector3 posCToEndEffector = endEffectorNodePos - posC;
            //if (enableRotation)
                //goal -= posCToEndEffector;

            // draw debug lines
            #ifdef EMFX_EMSTUDIOBUILD
                if (GetCanVisualize(animGraphInstance))
                {
                    const float s = animGraphInstance->GetVisualizeScale() * actorInstance->GetVisualizeScale();

                    Vector3 realGoal;
                    //if (enableRotation)
                        //realGoal = goal + posCToEndEffector;
                    //else
                        realGoal = goal;

                    uint32 color = mVisualizeColor;
                    GetEventManager().OnDrawLine( realGoal-Vector3(s,0,0), realGoal+Vector3(s,0,0), color);
                    GetEventManager().OnDrawLine( realGoal-Vector3(0,s,0), realGoal+Vector3(0,s,0), color);
                    GetEventManager().OnDrawLine( realGoal-Vector3(0,0,s), realGoal+Vector3(0,0,s), color);

                    color = MCore::RGBA(0, 255, 255);
                    GetEventManager().OnDrawLine( posA, posA+bendDir*s*2.5f, color);
                    GetEventManager().OnDrawLine( posA-Vector3(s,0,0), posA+Vector3(s,0,0), color);
                    GetEventManager().OnDrawLine( posA-Vector3(0,s,0), posA+Vector3(0,s,0), color);
                    GetEventManager().OnDrawLine( posA-Vector3(0,0,s), posA+Vector3(0,0,s), color);

                    //color = MCore::RGBA(0, 255, 0);
                    //GetEventManager().OnDrawLine( endEffectorNodePos-Vector3(s,0,0), endEffectorNodePos+Vector3(s,0,0), color);
                    //GetEventManager().OnDrawLine( endEffectorNodePos-Vector3(0,s,0), endEffectorNodePos+Vector3(0,s,0), color);
                    //GetEventManager().OnDrawLine( endEffectorNodePos-Vector3(0,0,s), endEffectorNodePos+Vector3(0,0,s), color);

                    //color = MCore::RGBA(255, 255, 0);
                    //GetEventManager().OnDrawLine( posA, posB, color);
                    //GetEventManager().OnDrawLine( posB, posC, color);
                    //GetEventManager().OnDrawLine( posC, endEffectorNodePos, MCore::RGBA(255, 255, 128));
                }
            #endif

            // perform IK, try to find a solution by calculating the new middle node position
            Vector3 midPos;
            //if (enableRotation)
                //TwoLinkIKSolver::Solve2LinkIK( posA, posB, posC, goal, bendDir, &midPos);
            //else

            if (doIK)
            {
                TwoLinkIKSolver::Solve2LinkIK( posA, posB, endEffectorNodePos, goal, bendDir, &midPos);

                // --------------------------------------
                // calculate the new node transforms
                // --------------------------------------
                // the vector pointing down the bone in the old orientataion
                Vector3 forward = posB - posA;
                forward.SafeNormalize();

                // where the new vector should be pointing
                Vector3 newForward = midPos - posA;
                newForward.SafeNormalize();

                // build a rotation, which rotates the old into the new vector
                Matrix rotMat;
                rotMat.SetRotationMatrixTwoVectors( newForward, forward );

                // rotate the first node into the new orientation
                globalMatrices[nodeIndexA].MultMatrix3x3( rotMat );
                globalMatrices[nodeIndexA].SetTranslation( posA );
                globalMatrices[nodeIndexB].MultMatrix4x3( localMatrices[nodeIndexB], globalMatrices[nodeIndexA] );  // update the global space matrices of the other two nodes (elbow/knee and foot/hand)
                globalMatrices[nodeIndexC].MultMatrix4x3( localMatrices[nodeIndexC], globalMatrices[nodeIndexB] );  // nodeC woudl be the foot or hand

                // update all the nodes from the end node (nodeC) towards the end effector
                // we need to do this in order to calculate the global space transform of the end effector, as this now changed by nodeA's rotation change
                for (int32 i=legInfo.mUpdatePath.GetLength()-1; i>=0; --i)
                {
                    curNode         = actor->GetNode( legInfo.mUpdatePath[i] );
                    curNodeIndex    = curNode->GetNodeIndex();
                    globalMatrices[curNodeIndex].MultMatrix4x3(localMatrices[curNodeIndex], globalMatrices[curNode->GetParentIndex()]);
                }

                // get the new current node positions
                midPos = globalMatrices[nodeIndexB].GetTranslation();
                posB = globalMatrices[nodeIndexB].GetTranslation();
                posC = globalMatrices[nodeIndexC].GetTranslation();
                endEffectorNodePos = globalMatrices[endEffectorNodeIndex].GetTranslation();

                // second node
                //if (enableRotation)
                    //forward = posC - posB;
                //else
                    forward = endEffectorNodePos - posB;

                forward.SafeNormalize();

                newForward = goal - posB;
                newForward.SafeNormalize();

                // calculate the rotation for the second node, and apply it
                rotMat.SetRotationMatrixTwoVectors( newForward, forward );
                globalMatrices[nodeIndexB].MultMatrix3x3( rotMat );
                globalMatrices[nodeIndexB].SetTranslation( midPos );
                globalMatrices[nodeIndexC].MultMatrix4x3( localMatrices[nodeIndexC], globalMatrices[nodeIndexB] );  // update the global space matrix of the last node (foot/hand)

                // convert back into local matrices
                const uint32 parentIndex = actor->GetNode(nodeIndexA)->GetParentIndex();
                if (parentIndex != MCORE_INVALIDINDEX32)
                    localMatrices[nodeIndexA].MultMatrix4x3( globalMatrices[nodeIndexA], globalMatrices[parentIndex].Inversed());
                localMatrices[nodeIndexB].MultMatrix4x3( globalMatrices[nodeIndexB], globalMatrices[nodeIndexA].Inversed());
                localMatrices[nodeIndexC].MultMatrix4x3( globalMatrices[nodeIndexC], globalMatrices[nodeIndexB].Inversed());

                // decompose the first node's local space matrix (the shoulder/hip/upperarm/upperleg)
                #ifndef EMFX_SCALE_DISABLED
                    MatrixDecomposer decomposer;
                    decomposer.InitFromMatrix( localMatrices[nodeIndexA] );
                    transformA.mRotation        = decomposer.GetRotation();
                    transformA.mPosition        = decomposer.GetTranslation();
                    EMFX_SCALECODE
                    (
                        transformA.mScale           = decomposer.GetScale();
                        transformA.mScaleRotation   = decomposer.GetScaleRotation();
                    )

                    // decompose the second node (the elbow/knee)
                    decomposer.InitFromMatrix( localMatrices[nodeIndexB] );
                    transformB.mRotation        = decomposer.GetRotation();
                    transformB.mPosition        = decomposer.GetTranslation();
                    EMFX_SCALECODE
                    (
                        transformB.mScale           = decomposer.GetScale();
                        transformB.mScaleRotation   = decomposer.GetScaleRotation();
                    )
                #else
                    localMatrices[nodeIndexA].Decompose(&transformA.mPosition, &transformA.mRotation);
                    localMatrices[nodeIndexB].Decompose(&transformB.mPosition, &transformB.mRotation);
                #endif

                outLocalPose.SetLocalTransform(nodeIndexA, transformA);
                outLocalPose.SetLocalTransform(nodeIndexB, transformB);
                outLocalPose.SetLocalTransform(nodeIndexC, transformC); // TODO: can we skip it?
            }

            // render some debug lines
            #ifdef EMFX_EMSTUDIOBUILD
                if (GetCanVisualize(animGraphInstance) && doIK)
                {
                    // update all the nodes from the end node (nodeC) towards the end effector
                    for (int32 i=legInfo.mUpdatePath.GetLength()-1; i>=0; --i)
                    {
                        curNode         = actor->GetNode( legInfo.mUpdatePath[i] );
                        curNodeIndex    = curNode->GetNodeIndex();
                        globalMatrices[curNodeIndex].MultMatrix4x3( localMatrices[curNodeIndex] * globalMatrices[curNode->GetParentIndex()] );
                    }

                    const uint32 color = mVisualizeColor;
                    GetEventManager().OnDrawLine( posA, midPos, color);
                    GetEventManager().OnDrawLine( midPos, globalMatrices[nodeIndexC].GetTranslation(), color);
                    GetEventManager().OnDrawLine( globalMatrices[nodeIndexC].GetTranslation(), globalMatrices[endEffectorNodeIndex].GetTranslation(), color);

                    GetEventManager().OnDrawLine( intersectInfo.mPosition, intersectInfo.mPosition + intersectInfo.mNormal * 3.0f, MCore::RGBA(255, 255, 0));
                }
            #endif

            // only blend when needed
            if (weight < 0.999f)
            {
                // get the original input pose
                const Pose& inputLocalPose = inputPose->GetPose();

                // get the original input transforms
                Transform finalTransformA = inputLocalPose.GetLocalTransform(nodeIndexA);
                Transform finalTransformB = inputLocalPose.GetLocalTransform(nodeIndexB);
                Transform finalTransformC = inputLocalPose.GetLocalTransform(nodeIndexC);

                // blend them into the new transforms after IK
                finalTransformA.Blend( transformA, weight );
                finalTransformB.Blend( transformB, weight );
                finalTransformC.Blend( transformC, weight );

                // copy them to the output transforms
                outLocalPose.SetLocalTransform(nodeIndexA, finalTransformA);
                outLocalPose.SetLocalTransform(nodeIndexB, finalTransformB);
                outLocalPose.SetLocalTransform(nodeIndexC, finalTransformC);
                //transformA = finalTransformA;
                //transformB = finalTransformB;
                //transformC = finalTransformC;
            }
        }   // for all legs
    }


    // get the type string
    const char* BlendTreeFootPlantIKNode::GetTypeString() const
    {
        return "BlendTreeFootPlantIKNode";
    }


    // perform motion extraction
    void BlendTreeFootPlantIKNode::ExtractMotion(AnimGraphInstance* animGraphInstance)
    {
        // get the source node of the pose connection
        BlendTreeConnection* connection = mInputPorts[INPUTPORT_POSE].mConnection;
        if (connection == nullptr)
        {
            //UniqueData* uniqueData = static_cast<UniqueData*>( FindUniqueNodeData(animGraphInstance) );
            ZeroTrajectoryDelta();
            return;
        }

        // and just use the motion extraction info from that node
        connection->GetSourceNode()->PerformExtractMotion( animGraphInstance );

        //UniqueData* uniqueData = static_cast<UniqueData*>( FindUniqueNodeData(animGraphInstance) );
        SetTrajectoryDelta( connection->GetSourceNode()->GetTrajectoryDelta() );
        SetTrajectoryDeltaMirrored( connection->GetSourceNode()->GetTrajectoryDeltaMirrored() );
    }


    // update the parameter contents, such as combobox values
    void BlendTreeFootPlantIKNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find our unique data
        UniqueData* uniqueData = static_cast<UniqueData*>( animGraphInstance->FindUniqueNodeData(this) );
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew( TYPE_ID, this, animGraphInstance );
            animGraphInstance->RegisterUniqueObjectData( uniqueData );
        }

        uniqueData->mMustUpdate = true;
        UpdateUniqueData( animGraphInstance, uniqueData );
    }


    // pre-allocate data
    void BlendTreeFootPlantIKNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
    }
    */
} // namespace EMotionFX


