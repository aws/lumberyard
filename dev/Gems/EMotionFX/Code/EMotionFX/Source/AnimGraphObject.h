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

// include the required headers
#include "EMotionFXConfig.h"
#include <AzCore/RTTI/RTTI.h>
#include <MCore/Source/Stream.h>
#include <MCore/Source/CommandLine.h>
#include <MCore/Source/Color.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeInt32.h>
#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AttributeVector3.h>
#include <MCore/Source/AttributeVector4.h>
#include <MCore/Source/AttributeQuaternion.h>
#include <MCore/Source/AttributeColor.h>
#include <MCore/Source/AttributeArray.h>
#include <MCore/Source/AttributeSettingsSet.h>
#include "AnimGraphObjectData.h"
#include "BaseObject.h"


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class AnimGraph;
    class MotionSet;
    class AnimGraphNode;
    class AttributeGoalNode;
    class AnimGraphInstance;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphObject
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphObject, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS);

    public:
        AZ_RTTI(AnimGraphObject, "{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}");

        enum
        {
            FLAG_DISABLED       = 1 << 0    // if the attribute is disabled
        };

        enum ECategory
        {
            CATEGORY_SOURCES                = 0,
            CATEGORY_BLENDING               = 1,
            CATEGORY_CONTROLLERS            = 2,
            CATEGORY_PHYSICS                = 3,
            CATEGORY_LOGIC                  = 4,
            CATEGORY_MATH                   = 5,
            CATEGORY_MISC                   = 6,
            CATEGORY_TRANSITIONS            = 10,
            CATEGORY_TRANSITIONCONDITIONS   = 11
        };

        enum ESyncMode
        {
            SYNCMODE_DISABLED               = 0,
            SYNCMODE_TRACKBASED             = 1,
            SYNCMODE_CLIPBASED              = 2
        };

        enum EEventMode
        {
            EVENTMODE_MASTERONLY            = 0,
            EVENTMODE_SLAVEONLY             = 1,
            EVENTMODE_BOTHNODES             = 2,
            EVENTMODE_MOSTACTIVE            = 3
        };

        virtual void RegisterAttributes() {}
        virtual void Unregister();
        MCORE_INLINE uint32 GetType() const                                             { return mTypeID; }
        virtual uint32 GetBaseType() const = 0;
        virtual const char* GetTypeString() const = 0;
        virtual const char* GetPaletteName() const = 0;
        virtual void GetSummary(MCore::String* outResult) const;
        virtual void GetTooltip(MCore::String* outResult) const;
        virtual ECategory GetPaletteCategory() const = 0;
        virtual AnimGraphObject* Clone(AnimGraph* animGraph) = 0;
        virtual void PostClone(AnimGraphObject* sourceObject, AnimGraph* sourceAnimGraph) { MCORE_UNUSED(sourceObject); MCORE_UNUSED(sourceAnimGraph); }
        virtual AnimGraphObjectData* CreateObjectData() = 0;
        virtual AnimGraphObject* RecursiveClone(AnimGraph* animGraph, AnimGraphObject* parentObject);

        void InitInternalAttributesForAllInstances();   // does the init for all anim graph instances in the parent animgraph
        virtual void InitInternalAttributes(AnimGraphInstance* animGraphInstance);
        virtual void RemoveInternalAttributesForAllInstances();
        virtual void DecreaseInternalAttributeIndices(uint32 decreaseEverythingHigherThan);

        virtual void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        virtual void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance);
        virtual void OnUpdateAttributes() {}
        virtual void OnUpdateAttribute(MCore::Attribute* attribute, MCore::AttributeSettings* settings)          { MCORE_UNUSED(attribute);         MCORE_UNUSED(settings); }
        virtual void OnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)            { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(newMotionSet); }
        virtual void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName)      { MCORE_UNUSED(animGraph);         MCORE_UNUSED(node);         MCORE_UNUSED(oldName); }
        virtual void OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node)                                    { MCORE_UNUSED(animGraph);         MCORE_UNUSED(node); }
        virtual void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)                             { MCORE_UNUSED(animGraph);         MCORE_UNUSED(nodeToRemove); }
        virtual void RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)   { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(newMotionSet); }
        virtual void OnActorMotionExtractionNodeChanged()                                                        {}

        void CopyBaseObjectTo(AnimGraphObject* object);

        uint32 GetNumAttributes() const;
        uint32 FindAttributeByInternalName(const char* internalName) const;
        MCore::AttributeSettings* RegisterAttribute(const char* name, const char* internalName, const char* description, uint32 interfaceType);
        void CreateAttributeValues();

        void EnableAllAttributes(bool enabled);

        #ifdef EMFX_EMSTUDIOBUILD
        bool GetIsAttributeEnabled(uint32 index) const          { return !(mAttributeFlags[index] & FLAG_DISABLED); }
        bool GetIsAttributeDisabled(uint32 index) const         { return (mAttributeFlags[index] & FLAG_DISABLED); }
        void SetAttributeEnabled(uint32 index)                  { mAttributeFlags[index] &= ~FLAG_DISABLED; }
        void SetAttributeDisabled(uint32 index)                 { mAttributeFlags[index] |= FLAG_DISABLED; }
        #endif

        MCore::String CreateAttributesString() const;
        void InitAttributesFromString(const char* attribString);
        void InitAttributesFromCommandLine(const MCore::CommandLine& commandLine);

        void RemoveAllAttributeValues(bool delFromMem);

        MCORE_INLINE MCore::Attribute*          GetAttribute(uint32 index) const                { return mAttributeValues[index]; }
        MCORE_INLINE MCore::AttributeFloat*     GetAttributeFloat(uint32 index) const           { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeFloat::TYPE_ID);       return static_cast<MCore::AttributeFloat*>(mAttributeValues[index]); }
        MCORE_INLINE MCore::AttributeInt32*     GetAttributeInt32(uint32 index) const           { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeInt32::TYPE_ID);       return static_cast<MCore::AttributeInt32*>(mAttributeValues[index]); }
        MCORE_INLINE MCore::AttributeString*    GetAttributeString(uint32 index) const          { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeString::TYPE_ID);      return static_cast<MCore::AttributeString*>(mAttributeValues[index]); }
        MCORE_INLINE MCore::AttributeBool*      GetAttributeBool(uint32 index) const            { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeBool::TYPE_ID);        return static_cast<MCore::AttributeBool*>(mAttributeValues[index]); }
        MCORE_INLINE MCore::AttributeVector2*   GetAttributeVector2(uint32 index) const         { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeVector2::TYPE_ID);     return static_cast<MCore::AttributeVector2*>(mAttributeValues[index]); }
        MCORE_INLINE MCore::AttributeVector3*   GetAttributeVector3(uint32 index) const         { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeVector3::TYPE_ID);     return static_cast<MCore::AttributeVector3*>(mAttributeValues[index]); }
        MCORE_INLINE MCore::AttributeVector4*   GetAttributeVector4(uint32 index) const         { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeVector4::TYPE_ID);     return static_cast<MCore::AttributeVector4*>(mAttributeValues[index]); }
        MCORE_INLINE MCore::AttributeQuaternion* GetAttributeQuaternion(uint32 index) const     { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeQuaternion::TYPE_ID);  return static_cast<MCore::AttributeQuaternion*>(mAttributeValues[index]); }
        MCORE_INLINE MCore::AttributeColor*     GetAttributeColor(uint32 index) const           { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeColor::TYPE_ID);       return static_cast<MCore::AttributeColor*>(mAttributeValues[index]); }
        MCORE_INLINE MCore::AttributeArray*     GetAttributeArray(uint32 index) const           { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeArray::TYPE_ID);       return static_cast<MCore::AttributeArray*>(mAttributeValues[index]); }
        MCORE_INLINE bool                       GetAttributeFloatAsBool(uint32 index) const     { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeFloat::TYPE_ID);       return (static_cast<MCore::AttributeFloat*>(mAttributeValues[index])->GetValue() > MCore::Math::epsilon); }
        MCORE_INLINE int32                      GetAttributeFloatAsInt32(uint32 index) const    { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeFloat::TYPE_ID);       return static_cast<int32>(static_cast<MCore::AttributeFloat*>(mAttributeValues[index])->GetValue()); }
        MCORE_INLINE uint32                     GetAttributeFloatAsUint32(uint32 index) const   { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeFloat::TYPE_ID);       return static_cast<uint32>(static_cast<MCore::AttributeFloat*>(mAttributeValues[index])->GetValue()); }
        MCORE_INLINE float                      GetAttributeFloatAsFloat(uint32 index) const    { MCORE_ASSERT(mAttributeValues[index]->GetType() == MCore::AttributeFloat::TYPE_ID);       return static_cast<MCore::AttributeFloat*>(mAttributeValues[index])->GetValue(); }
        AttributeGoalNode* GetAttributeGoalNode(uint32 index);

        MCORE_INLINE uint32 GetObjectIndex() const                                      { return mObjectIndex; }
        MCORE_INLINE void SetObjectIndex(uint32 index)                                  { mObjectIndex = index; }

        MCORE_INLINE AnimGraph* GetAnimGraph() const                                  { return mAnimGraph; }
        MCORE_INLINE void SetAnimGraph(AnimGraph* animGraph)                         { mAnimGraph = animGraph; }

        static uint32 InterfaceTypeToDataType(uint32 interfaceType);
        
        virtual uint32 GetAnimGraphSaveVersion() const        { return 1; }

        /**
         * Get the size in bytes of the custom data which will be saved with the object.
         * @return The size in bytes of the custom data.
         */
        virtual uint32 GetCustomDataSize() const        { return 0; }
        virtual bool WriteAttributes(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const;
        virtual bool ReadAttributes(MCore::Stream* stream, uint32 numAttributes, uint32 version, MCore::Endian::EEndianType endianType);
        static bool SkipReadAttributes(MCore::Stream* stream, uint32 numAttributes, uint32 version, MCore::Endian::EEndianType endianType);

        /**
         * Write the custom data which will be saved with the object.
         * @param[in] stream A pointer to a stream to which the custom data will be saved to.
         * @param[in] targetEndianType The endian type in which the custom data should be saved in.
         */
        virtual bool WriteCustomData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType)            { MCORE_UNUSED(stream); MCORE_UNUSED(targetEndianType); return true; }

        /**
         * Read the custom data which got saved with the object.
         * @param[in] stream A pointer to a stream from which the custom data will be read from.
         * @param[in] version The version of the custom data.
         * @param[in] endianType The endian type in which the custom data should be saved in.
         */
        virtual bool ReadCustomData(MCore::Stream* stream, uint32 version, MCore::Endian::EEndianType endianType)   { MCORE_UNUSED(stream); MCORE_UNUSED(version); MCORE_UNUSED(endianType); return true; }

        uint32 SaveUniqueData(const AnimGraphInstance* animGraphInstance, uint8* outputBuffer) const;  // save and return number of bytes written, when outputBuffer is nullptr only return num bytes it would write
        uint32 LoadUniqueData(const AnimGraphInstance* animGraphInstance, const uint8* dataBuffer);    // load and return number of bytes read, when dataBuffer is nullptr, 0 should be returned

        virtual void RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const;

        virtual void RecursiveInit(AnimGraphInstance* animGraphInstance)       { Init(animGraphInstance); }
        virtual void Init(AnimGraphInstance* animGraphInstance)                { MCORE_UNUSED(animGraphInstance); }

        void ResetUniqueData(AnimGraphInstance* animGraphInstance);
        MCore::AttributeSettings* RegisterSyncAttribute();
        MCore::AttributeSettings* RegisterEventFilterAttribute();

        virtual bool ConvertAttribute(uint32 attributeIndex, const MCore::Attribute* attributeToConvert, const MCore::String& attributeName);

        bool GetHasErrorFlag(AnimGraphInstance* animGraphInstance) const;
        void SetHasErrorFlag(AnimGraphInstance* animGraphInstance, bool hasError);

        void Scale(float scaleFactor);

    protected:
        MCore::Array<MCore::Attribute*>         mAttributeValues;
        AnimGraph*                             mAnimGraph;
        uint32                                  mObjectIndex;
        uint32                                  mTypeID;

        #ifdef EMFX_EMSTUDIOBUILD
        MCore::Array<uint8>                 mAttributeFlags;
        #endif

        AnimGraphObject(AnimGraph* animGraph, uint32 typeID);
        virtual ~AnimGraphObject();
    };
}   // namespace EMotionFX
