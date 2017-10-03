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

#pragma once

#include "EMotionFXConfig.h"
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphObject;
    class AnimGraph;

    /**
     *
     *
     */
    class EMFX_API AnimGraphObjectFactory
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphObjectFactory, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS);

    public:
        AnimGraphObjectFactory();
        ~AnimGraphObjectFactory();

        void Init();

        AnimGraphObject* CreateObjectByTypeID(AnimGraph* animGraph, uint32 typeID);
        AnimGraphObject* CreateObjectByTypeString(AnimGraph* animGraph, const char* typeNameString);
        AnimGraphObject* CreateObject(AnimGraph* animGraph, AnimGraphObject* registeredNode);

        bool RegisterObjectType(AnimGraphObject* object);
        void UnregisterAllObjects();
        bool UnregisterObjectByTypeID(uint32 typeID);
        bool UnregisterObjectByTypeString(const char* typeNameString);
        bool CheckIfHasRegisteredObjectByTypeID(uint32 typeID) const;
        bool CheckIfHasRegisteredObjectByTypeString(const char* typeNameString) const;
        bool CheckIfHasRegisteredObject(AnimGraphObject* node) const;
        uint32 FindRegisteredObjectByTypeID(uint32 typeID) const;
        uint32 FindRegisteredObjectByTypeString(const char* typeString) const;
        uint32 FindRegisteredObject(AnimGraphObject* registeredObject) const;
        AZ::Uuid FindObjectTypeByTypeString(const char* typeNameString);

        MCORE_INLINE size_t GetNumRegisteredObjects() const                        { return mRegisteredObjects.size(); }
        MCORE_INLINE AnimGraphObject* GetRegisteredObject(size_t index)            { return mRegisteredObjects[index]; }

        void Log();

    private:
        AZStd::vector<AnimGraphObject*> mRegisteredObjects;
    };
}   // namespace EMotionFX
