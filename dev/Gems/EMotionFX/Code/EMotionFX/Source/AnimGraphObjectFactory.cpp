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
#include "AnimGraphObjectFactory.h"
#include "AnimGraphObject.h"
#include "AnimGraphObjectDataPool.h"
#include "AnimGraphManager.h"

#include <MCore/Source/LogManager.h>

// default object types
#include "BlendTreeParameterNode.h"
#include "AnimGraphBindPoseNode.h"
#include "AnimGraphMotionNode.h"
#include "BlendSpace2DNode.h"
#include "BlendSpace1DNode.h"
#include "BlendTreeFinalNode.h"
#include "BlendTreeBlend2Node.h"
#include "BlendTreeBlendNNode.h"
#include "BlendTreePoseSwitchNode.h"
#include "BlendTreeFloatMath1Node.h"
#include "BlendTreeFloatMath2Node.h"
#include "BlendTreeFloatSwitchNode.h"
#include "BlendTreeFloatConditionNode.h"
#include "BlendTreeBoolLogicNode.h"
#include "BlendTreeSmoothingNode.h"
#include "BlendTreeMaskNode.h"
#include "BlendTreeMotionFrameNode.h"
#include "BlendTreeVector3Math1Node.h"
#include "BlendTreeVector3Math2Node.h"
#include "BlendTreeVector2DecomposeNode.h"
#include "BlendTreeVector3DecomposeNode.h"
#include "BlendTreeVector4DecomposeNode.h"
#include "BlendTreeVector2ComposeNode.h"
#include "BlendTreeVector3ComposeNode.h"
#include "BlendTreeVector4ComposeNode.h"
#include "BlendTreeTwoLinkIKNode.h"
#include "BlendTreeLookAtNode.h"
#include "BlendTreeTransformNode.h"
#include "BlendTreeAccumTransformNode.h"
#include "BlendTreeRangeRemapperNode.h"
#include "BlendTreeDirectionToWeightNode.h"
#include "BlendTree.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphExitNode.h"
#include "AnimGraphEntryNode.h"
#include "AnimGraphTransitionCondition.h"
#include "AnimGraphParameterCondition.h"
#include "AnimGraphMotionCondition.h"
#include "AnimGraphStateCondition.h"
#include "AnimGraphTimeCondition.h"
#include "AnimGraphTagCondition.h"
#include "AnimGraphVector2Condition.h"
#include "AnimGraphPlayTimeCondition.h"
#include "BlendTreeMirrorPoseNode.h"

// in production
//#include "AnimGraphMotionFieldNode.h"
//#include "BlendTreeRetargetPoseNode.h"
//#include "BlendTreeFootPlantIKNode.h"
//#include "AnimGraphReferenceNode.h"


namespace EMotionFX
{
    // constructor
    AnimGraphObjectFactory::AnimGraphObjectFactory()
    {
    }


    // destructor
    AnimGraphObjectFactory::~AnimGraphObjectFactory()
    {
        // unregister all objects
        UnregisterAllObjects();
    }


    // init
    void AnimGraphObjectFactory::Init()
    {
        // reserve space for the registered object types
        mRegisteredObjects.reserve(64);

        // register default object types
        RegisterObjectType(AnimGraphStateMachine::Create(nullptr));
        RegisterObjectType(BlendTree::Create(nullptr));
        RegisterObjectType(BlendTreeFinalNode::Create(nullptr));
        RegisterObjectType(AnimGraphMotionNode::Create(nullptr));
        RegisterObjectType(BlendSpace1DNode::Create(nullptr));
        RegisterObjectType(BlendSpace2DNode::Create(nullptr));
        RegisterObjectType(BlendTreeBlend2Node::Create(nullptr));
        RegisterObjectType(BlendTreeBlendNNode::Create(nullptr));
        RegisterObjectType(BlendTreeParameterNode::Create(nullptr));
        RegisterObjectType(BlendTreeFloatMath1Node::Create(nullptr));
        RegisterObjectType(BlendTreeFloatMath2Node::Create(nullptr));
        RegisterObjectType(BlendTreeFloatConditionNode::Create(nullptr));
        RegisterObjectType(BlendTreeFloatSwitchNode::Create(nullptr));
        RegisterObjectType(BlendTreeBoolLogicNode::Create(nullptr));
        RegisterObjectType(BlendTreePoseSwitchNode::Create(nullptr));
        RegisterObjectType(BlendTreeMaskNode::Create(nullptr));
        RegisterObjectType(AnimGraphBindPoseNode::Create(nullptr));
        RegisterObjectType(BlendTreeMotionFrameNode::Create(nullptr));
        RegisterObjectType(BlendTreeVector3Math1Node::Create(nullptr));
        RegisterObjectType(BlendTreeVector3Math2Node::Create(nullptr));
        RegisterObjectType(BlendTreeVector2DecomposeNode::Create(nullptr));
        RegisterObjectType(BlendTreeVector3DecomposeNode::Create(nullptr));
        RegisterObjectType(BlendTreeVector4DecomposeNode::Create(nullptr));
        RegisterObjectType(BlendTreeVector2ComposeNode::Create(nullptr));
        RegisterObjectType(BlendTreeVector3ComposeNode::Create(nullptr));
        RegisterObjectType(BlendTreeVector4ComposeNode::Create(nullptr));
        RegisterObjectType(BlendTreeSmoothingNode::Create(nullptr));
        RegisterObjectType(BlendTreeRangeRemapperNode::Create(nullptr));
        RegisterObjectType(AnimGraphExitNode::Create(nullptr));
        RegisterObjectType(AnimGraphEntryNode::Create(nullptr));
        RegisterObjectType(BlendTreeDirectionToWeightNode::Create(nullptr));
        RegisterObjectType(BlendTreeMirrorPoseNode::Create(nullptr));
        //RegisterObjectType( BlendTreeRetargetPoseNode::Create(nullptr) );
        //RegisterObjectType( AnimGraphReferenceNode::Create(nullptr) );
        //RegisterObjectType( AnimGraphMotionFieldNode::Create(nullptr) );

        // controller nodes
        RegisterObjectType(BlendTreeTwoLinkIKNode::Create(nullptr));
        RegisterObjectType(BlendTreeLookAtNode::Create(nullptr));
        RegisterObjectType(BlendTreeTransformNode::Create(nullptr));
        RegisterObjectType(BlendTreeAccumTransformNode::Create(nullptr));
        //RegisterObjectType( BlendTreeFootPlantIKNode::Create(nullptr) );

        // register transition nodes
        RegisterObjectType(AnimGraphStateTransition::Create(nullptr));

        // register default transition condition types
        RegisterObjectType(AnimGraphParameterCondition::Create(nullptr));
        RegisterObjectType(AnimGraphVector2Condition::Create(nullptr));
        RegisterObjectType(AnimGraphMotionCondition::Create(nullptr));
        RegisterObjectType(AnimGraphStateCondition::Create(nullptr));
        RegisterObjectType(AnimGraphTimeCondition::Create(nullptr));
        RegisterObjectType(AnimGraphPlayTimeCondition::Create(nullptr));
        RegisterObjectType(AnimGraphTagCondition::Create(nullptr));
    }

    // create a given object by a given type ID
    AnimGraphObject* AnimGraphObjectFactory::CreateObjectByTypeID(AnimGraph* animGraph, uint32 typeID)
    {
        // try to find the registered object index
        const uint32 index = FindRegisteredObjectByTypeID(typeID);
        if (index == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("EMotionFX::AnimGraphObjectFactory::CreateObjectByTypeID() - failed to create object with type ID %d, as no such object type has been registered", typeID);
            return nullptr;
        }

        // create a clone of the registered object
        AnimGraphObject* object =  mRegisteredObjects[index]->Clone(animGraph);
        return object;
    }


    // create a given object by its type string
    AnimGraphObject* AnimGraphObjectFactory::CreateObjectByTypeString(AnimGraph* animGraph, const char* typeNameString)
    {
        // try to find the registered object index
        const uint32 index = FindRegisteredObjectByTypeString(typeNameString);
        if (index == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("EMotionFX::AnimGraphObjectFactory::CreateObjectByTypeString() - failed to create object with type string '%s', as no such object type has been registered", typeNameString);
            return nullptr;
        }

        // create a clone of the registered object
        AnimGraphObject* object = mRegisteredObjects[index]->Clone(animGraph);
        return object;
    }


    // create a given object from a given registered object
    AnimGraphObject* AnimGraphObjectFactory::CreateObject(AnimGraph* animGraph, AnimGraphObject* registeredObject)
    {
        MCORE_ASSERT(registeredObject);

        // try to find the registered object index
        const uint32 index = FindRegisteredObject(registeredObject);
        if (index == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("EMotionFX::AnimGraphObjectFactory::CreateObject() - failed to create object with from pointer 0x%x (typeString='%s'), as no such object type has been registered", registeredObject, registeredObject->GetTypeString());
            return nullptr;
        }

        // create a clone of the registered object
        AnimGraphObject* object = mRegisteredObjects[index]->Clone(animGraph);
        return object;
    }


    // unregister all nodes
    void AnimGraphObjectFactory::UnregisterAllObjects()
    {
        const size_t numObjects = mRegisteredObjects.size();
        for (size_t i = 0; i < numObjects; ++i)
        {
            AnimGraphObject* object = mRegisteredObjects[i];
            object->Unregister();
            object->Destroy();
        }

        mRegisteredObjects.clear();
    }


    // unregister a object with a given type ID
    bool AnimGraphObjectFactory::UnregisterObjectByTypeID(uint32 typeID)
    {
        // try to find the object index
        const uint32 index = FindRegisteredObjectByTypeID(typeID);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // remove the object
        mRegisteredObjects[index]->Unregister();
        mRegisteredObjects[index]->Destroy();
        mRegisteredObjects.erase(mRegisteredObjects.begin() + index);

        // successfully unregistered
        return true;
    }


    // unregister a given object by its type name
    bool AnimGraphObjectFactory::UnregisterObjectByTypeString(const char* typeNameString)
    {
        // try to find the object index
        const uint32 index = FindRegisteredObjectByTypeString(typeNameString);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // remove the object
        mRegisteredObjects[index]->Unregister();
        mRegisteredObjects[index]->Destroy();
        mRegisteredObjects.erase(mRegisteredObjects.begin() + index);

        // successfully unregistered
        return true;
    }


    // register a given object
    bool AnimGraphObjectFactory::RegisterObjectType(AnimGraphObject* object)
    {
        // make sure we didn't already register this object
        if (CheckIfHasRegisteredObject(object))
        {
            MCore::LogWarning("EMotionFX::BlendTreeObjectFactor::RegisterObjectType() - Already registered the given object (type=%s)", object->GetTypeString());
            MCORE_ASSERT(false); // this is quite important, it shouldn't happen
            return false;
        }

        // register it
        mRegisteredObjects.push_back(object);

        // create the shared data object and register the ports and parameters for this object
        GetAnimGraphManager().CreateSharedData(object);
        object->RegisterAttributes();

        // register the unique data to the pool
        GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().Register(object->GetType(), 32, AnimGraphObjectDataPool::POOLTYPE_DYNAMIC, 128);

        // scale the attributes (min, max, default) while assuming they are inside the nodes initialized in meters
        const float scaleFactor = (float)MCore::Distance::GetConversionFactor(MCore::Distance::UNITTYPE_METERS, GetEMotionFX().GetUnitType());
        object->Scale(scaleFactor);

        return true;
    }


    // check if we have a registered object of the given type
    bool AnimGraphObjectFactory::CheckIfHasRegisteredObjectByTypeID(uint32 typeID) const
    {
        return (FindRegisteredObjectByTypeID(typeID) != MCORE_INVALIDINDEX32);
    }


    // check if we have a registered object with the given type string
    bool AnimGraphObjectFactory::CheckIfHasRegisteredObjectByTypeString(const char* typeNameString) const
    {
        return (FindRegisteredObjectByTypeString(typeNameString) != MCORE_INVALIDINDEX32);
    }


    // check if we have a given object registered already
    bool AnimGraphObjectFactory::CheckIfHasRegisteredObject(AnimGraphObject* object) const
    {
        return (FindRegisteredObject(object) != MCORE_INVALIDINDEX32);
    }


    // find out if we already registered the given object pointer
    uint32 AnimGraphObjectFactory::FindRegisteredObject(AnimGraphObject* object) const
    {
        const size_t numObjects = mRegisteredObjects.size();
        for (size_t i = 0; i < numObjects; ++i)
        {
            if (mRegisteredObjects[i] == object)
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find registered object by its type ID
    uint32 AnimGraphObjectFactory::FindRegisteredObjectByTypeID(uint32 typeID) const
    {
        const size_t numObjects = mRegisteredObjects.size();
        for (size_t i = 0; i < numObjects; ++i)
        {
            if (mRegisteredObjects[i]->GetType() == typeID)
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find a registered object by its type string
    uint32 AnimGraphObjectFactory::FindRegisteredObjectByTypeString(const char* typeString) const
    {
        const size_t numObjects = mRegisteredObjects.size();
        for (size_t i = 0; i < numObjects; ++i)
        {
            if (strcmp(typeString, mRegisteredObjects[i]->GetTypeString()) == 0) // case sensitive string compare
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    AZ::Uuid AnimGraphObjectFactory::FindObjectTypeByTypeString(const char* typeNameString)
    {
        const uint32 index = FindRegisteredObjectByTypeString(typeNameString);
        if (index == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("EMotionFX::AnimGraphObjectFactory::FindObjectUuidByTypeString() - failed to find object with type string '%s', as no such object type has been registered", typeNameString);
            return AZ::Uuid::CreateNull();
        }

        AnimGraphObject* object = mRegisteredObjects[index];
        return azrtti_typeid(object);
    }


    void AnimGraphObjectFactory::Log()
    {
        const size_t numRegisteredObjects = mRegisteredObjects.size();
        AZ_Printf("EMotionFX", " - AnimGraphObjectFactory: NumRegisteredObjects=%d", numRegisteredObjects);

        AZ::Uuid uuid;
        AZStd::string uuidString;
        for (size_t i = 0; i < numRegisteredObjects; ++i)
        {
            const AnimGraphObject* object = mRegisteredObjects[i];

            uuid = azrtti_typeid(object);
            uuidString = uuid.ToString<AZStd::string>();

            AZ_Printf("EMotionFX", "    + Object #%d: Name='%s' UUID='%s'", i+1, object->GetPaletteName(), uuidString.c_str());
        }
    }
} // namespace EMotionFX