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
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/AttributeSettings.h>
#include <MCore/Source/AttributePool.h>
#include <MCore/Source/MCoreSystem.h>
#include "EMotionFXConfig.h"
#include "AnimGraphObject.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"
#include "AnimGraph.h"


namespace EMotionFX
{
    // declare static variable
    //Array<AnimGraphObject::AttributeInfoSet*>    AnimGraphObject::mAttributeInfoSets;


    // constructor
    AnimGraphObject::AnimGraphObject(AnimGraph* animGraph, uint32 typeID)
        : BaseObject()
    {
        mAttributeValues.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS);

        // reserve some memory
        mAnimGraph         = animGraph;
        mObjectIndex        = MCORE_INVALIDINDEX32;
        mTypeID             = typeID;
    }


    // destructor
    AnimGraphObject::~AnimGraphObject()
    {
        RemoveAllAttributeValues(true);
    }


    // remove all attribute values
    void AnimGraphObject::RemoveAllAttributeValues(bool delFromMem)
    {
        if (delFromMem)
        {
            const uint32 numAttribs = mAttributeValues.GetLength();
            for (uint32 i = 0; i < numAttribs; ++i)
            {
                if (mAttributeValues[i])
                {
                    mAttributeValues[i]->Destroy();
                }
            }
        }

        mAttributeValues.Clear();
    }


    // unregister itself
    void AnimGraphObject::Unregister()
    {
        // delete the attribute info set
        AnimGraphManager::AttributeInfoSet* attributeInfoSet = GetAnimGraphManager().FindAttributeInfoSet(this);
        GetAnimGraphManager().RemoveAttributeInfoSet(GetType(), false);
        delete attributeInfoSet;

        // delete the object data pool type
        GetAnimGraphManager().GetObjectDataPool().UnregisterObjectType(GetType());
    }



    // copy over base class settings
    void AnimGraphObject::CopyBaseObjectTo(AnimGraphObject* object)
    {
        // clone the attribute flags
    #ifdef EMFX_EMSTUDIOBUILD
        object->mAttributeFlags = mAttributeFlags;
        object->mAttributeFlags.Resize(GetNumAttributes());
    #else
        MCORE_UNUSED(object);
    #endif
        /*
            // clone the attribute values
            const uint32 numAttribs = mAttributeValues.GetLength();
            MCORE_ASSERT( object->mAttributeValues.GetLength() == 0 );
            object->mAttributeValues.Resize( numAttribs );
            for (uint32 i=0; i<numAttribs; ++i)
                object->mAttributeValues[i] = mAttributeValues[i]->Clone();*/
    }


    uint32 AnimGraphObject::GetNumAttributes() const
    {
        return GetAnimGraphManager().GetNumAttributes(this);
    }


    MCore::AttributeSettings* AnimGraphObject::RegisterAttribute(const char* name, const char* internalName, const char* description, uint32 interfaceType)
    {
        return GetAnimGraphManager().RegisterAttribute(this, name, internalName, description, interfaceType);
    }



    // create attribute values
    void AnimGraphObject::CreateAttributeValues()
    {
        // get rid of existing values
        RemoveAllAttributeValues(true);

        // create the attribute values
        const uint32 numAttributes = GetNumAttributes();
        mAttributeValues.Resize(numAttributes);
    #ifdef EMFX_EMSTUDIOBUILD
        mAttributeFlags.Resize(numAttributes);
    #endif

        // initialize the values to their default values
        AnimGraphManager& animGraphManager = GetAnimGraphManager();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            mAttributeValues[i] = animGraphManager.GetAttributeInfo(this, i)->GetDefaultValue()->Clone();

        #ifdef EMFX_EMSTUDIOBUILD
            mAttributeFlags[i]  = 0;    // reset the flag
        #endif
        }
    }


    // generate a command line string from the current parameter values
    // this builds a string like: "-numIterations 10 -maxValue 3.54665474 -enabled true"
    MCore::String AnimGraphObject::CreateAttributesString() const
    {
        MCore::String result;
        MCore::String valueString;
        result.Reserve(1024);

        // for all attributes
        const AnimGraphManager& animGraphManager = GetAnimGraphManager();
        const uint32 numAttribs = GetNumAttributes();
        for (uint32 a = 0; a < numAttribs; ++a)
        {
            const MCore::AttributeSettings* attribInfo = animGraphManager.GetAttributeInfo(const_cast<AnimGraphObject*>(this), a);

            // get the parameter value
            if (GetAttribute(a)->ConvertToString(valueString) == false)
            {
                MCore::LogWarning("EMotionFX::AnimGraphObject::CreateParameterString() - failed to convert the value for attribute '%s' into a string (probably dont know how to handle the data type).", attribInfo->GetInternalName());
            }
            else
            {
                result += (a == 0) ? "" : " ";
                result += "-";
                result += attribInfo->GetInternalName();
                result += " {";
                result += valueString.AsChar();
                result += "}";
            }
        }

        return result;
    }


    // init current attribute values from an attribute string
    void AnimGraphObject::InitAttributesFromString(const char* attribString)
    {
        InitAttributesFromCommandLine(MCore::CommandLine(attribString));
    }


    // find the parameter index for a parameter with a given internal name
    uint32 AnimGraphObject::FindAttributeByInternalName(const char* internalName) const
    {
        const AnimGraphManager& animGraphManager = GetAnimGraphManager();
        const uint32 numAttribs = animGraphManager.GetNumAttributes(this);
        for (uint32 a = 0; a < numAttribs; ++a)
        {
            MCore::AttributeSettings* attributeInfo = animGraphManager.GetAttributeInfo(this, a);
            if (attributeInfo->GetInternalNameString().CheckIfIsEqualNoCase(internalName))
            {
                return a;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // init parameter values by a command line
    void AnimGraphObject::InitAttributesFromCommandLine(const MCore::CommandLine& commandLine)
    {
        MCore::String attribName;
        MCore::String attribValue;

        // for all attributes in the command line
        const uint32 numAttributes = commandLine.GetNumParameters();
        for (uint32 a = 0; a < numAttributes; ++a)
        {
            // get the name and value
            attribName  = commandLine.GetParameterName(a);
            attribValue = commandLine.GetParameterValue(a);

            // find the attribute number
            const uint32 attribIndex = FindAttributeByInternalName(attribName.AsChar());
            if (attribIndex == MCORE_INVALIDINDEX32)
            {
                MCore::LogWarning("EMotionFX::AnimGraphObject::InitAttributesFromCommandLine() - Failed to find internal attribute with name '%s' for object type '%s'", attribName.AsChar(), GetTypeString());
                continue;
            }

            // init the attribute value by converting the string into the actual value type
            // so for example converting a "5.3546,10.234" into Vector2 data
            if (GetAttribute(attribIndex)->InitFromString(attribValue) == false)
            {
                MCore::LogWarning("EMotionFX::AnimGraphObject::InitAttributesFromCommandLine() - Failed to convert the value string '%s' for attribute '%s' into real data for object type '%s'", attribValue.AsChar(), attribName.AsChar(), GetTypeString());
            }
        }
    }


    // get the data type for a given interface type
    uint32 AnimGraphObject::InterfaceTypeToDataType(uint32 interfaceType)
    {
        switch (interfaceType)
        {
        case MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER:
            return MCore::AttributeFloat::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_FLOATSLIDER:
            return MCore::AttributeFloat::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER:
            return MCore::AttributeFloat::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_INTSLIDER:
            return MCore::AttributeFloat::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX:
            return MCore::AttributeFloat::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX:
            return MCore::AttributeFloat::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR2:
            return MCore::AttributeVector2::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO:
            return MCore::AttributeVector3::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3:
            return MCore::AttributeVector3::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR4:
            return MCore::AttributeVector4::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_ROTATION:
            return AttributeRotation::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_COLOR:
            return MCore::AttributeColor::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_FILEBROWSE:
            return MCore::AttributeString::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_STRING:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_MOTIONPICKER:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_PARAMETERPICKER:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_NODENAMES:
            return AttributeNodeMask::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_PARAMETERNAMES:
            return AttributeParameterMask::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_BLENDTREEMOTIONNODEPICKER:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_MOTIONEVENTTRACKPICKER:
            return MCore::AttributeInt32::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_NODESELECTION:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_GOALNODESELECTION:
            return AttributeGoalNode::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_STATEFILTERLOCAL:
            return AttributeStateFilterLocal::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_TAG:
            return MCore::AttributeFloat::TYPE_ID;

        default:
            MCore::LogWarning("Unknown data type for interface type %d.", interfaceType);
            return 0;
        }
        ;
    }


    // get the data type for a given interface type
    uint32 AnimGraphObject::InterfaceTypeToDataTypeLegacy(uint32 interfaceType)
    {
        switch (interfaceType)
        {
        case MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER:
            return MCore::AttributeFloat::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_FLOATSLIDER:
            return MCore::AttributeFloat::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER:
            return MCore::AttributeInt32::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_INTSLIDER:
            return MCore::AttributeInt32::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX:
            return MCore::AttributeInt32::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX:
            return MCore::AttributeBool::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR2:
            return MCore::AttributeVector2::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO:
            return MCore::AttributeVector3::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3:
            return MCore::AttributeVector3::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR4:
            return MCore::AttributeVector4::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_ROTATION:
            return AttributeRotation::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_COLOR:
            return MCore::AttributeColor::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_FILEBROWSE:
            return MCore::AttributeString::TYPE_ID;
        case MCore::ATTRIBUTE_INTERFACETYPE_STRING:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_MOTIONPICKER:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_PARAMETERPICKER:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_NODENAMES:
            return AttributeNodeMask::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_PARAMETERNAMES:
            return AttributeParameterMask::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_BLENDTREEMOTIONNODEPICKER:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_MOTIONEVENTTRACKPICKER:
            return MCore::AttributeInt32::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_NODESELECTION:
            return MCore::AttributeString::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_GOALNODESELECTION:
            return AttributeGoalNode::TYPE_ID;
        case ATTRIBUTE_INTERFACETYPE_STATEFILTERLOCAL:
            return AttributeStateFilterLocal::TYPE_ID;

        default:
            MCore::LogWarning("Unknown data type for interface type %d.", interfaceType);
            return 0;
        }
        ;
    }


    // write the attributes
    bool AnimGraphObject::WriteAttributes(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const
    {
        const AnimGraphManager& animGraphManager = GetAnimGraphManager();

        // get the number of attributes and iterate through them
        const uint32 numAttributes = GetNumAttributes();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            MCore::Attribute*           attribute       = GetAttribute(i);
            MCore::AttributeSettings*   attributeInfo   = animGraphManager.GetAttributeInfo(const_cast<AnimGraphObject*>(this), i);
            uint32                      numCharacters   = attributeInfo->GetInternalNameString().GetLength();
            uint32                      attributeType   = attribute->GetType();
            uint32                      attributeSize   = attribute->GetStreamSize();

            if (attributeInfo->GetInternalNameString().GetLength() == 0)
            {
                numCharacters = 0;
            }

            // convert endian
            MCore::Endian::ConvertUnsignedInt32To(&attributeType, targetEndianType);
            MCore::Endian::ConvertUnsignedInt32To(&attributeSize, targetEndianType);

            // write the attribute type
            if (stream->Write(&attributeType, sizeof(uint32)) == 0)
            {
                return false;
            }

            // write the attribute size
            if (stream->Write(&attributeSize, sizeof(uint32)) == 0)
            {
                return false;
            }

            // write the attribute name string
            const uint32 numChars = numCharacters;
            MCore::Endian::ConvertUnsignedInt32To(&numCharacters, targetEndianType);
            stream->Write(&numCharacters, sizeof(uint32));
            if (numChars > 0)
            {
                if (stream->Write(attributeInfo->GetInternalName(), attributeInfo->GetInternalNameString().GetLength()) == 0)
                {
                    return false;
                }
            }

            // save the actual attribute data
            if (attribute->Write(stream, targetEndianType) == false)
            {
                return false;
            }
        }

        return true;
    }


    // read the attributes
    bool AnimGraphObject::ReadAttributes(MCore::Stream* stream, uint32 numAttributes, uint32 version, MCore::Endian::EEndianType endianType)
    {
        MCORE_UNUSED(version);

        // our string that we use to read string data
        MCore::String name;
        name.Reserve(32);

        // for all attributes
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            // read the attribute size
            uint32 attribType;
            if (stream->Read(&attribType, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&attribType, endianType);

            // read the attribute size
            uint32 attributeSize;
            if (stream->Read(&attributeSize, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&attributeSize, endianType);

            // first read the number of characters
            uint32 numCharacters;
            if (stream->Read(&numCharacters, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numCharacters, endianType);

            // read the string
            if (numCharacters > 0)
            {
                name.Resize(numCharacters);
                if (stream->Read(name.GetPtr(), numCharacters) == 0)
                {
                    return false;
                }
            }

            // read the data
            const uint32 index = FindAttributeByInternalName(name.AsChar());
            if (index == MCORE_INVALIDINDEX32)
            {
                // try to convert the attribute
                MCore::Attribute* tempAttribute = MCore::GetAttributeFactory().CreateAttributeByType(attribType);
                if (tempAttribute)
                {
                    // try to read it
                    if (tempAttribute->Read(stream, endianType) == false)
                    {
                        return false;
                    }

                    // try to convert the attribute
                    if (ConvertAttribute(index, tempAttribute, name) == false)
                    {
                        MCore::LogWarning("AnimGraphObject::ReadAttributes() - Skipping attribute '%s' as it doesn't exist in the object and cannot be converted (fileAttribType=%s, objectType=%s).", name.AsChar(), tempAttribute->GetTypeString(), GetTypeString());
                    }
                    else
                    {
                        MCore::LogWarning("AnimGraphObject::ReadAttributes() - Successfully converted attribute '%s' from which the key name changed (fileAttribType=%s, objectType=%s).", name.AsChar(), tempAttribute->GetTypeString(), GetTypeString());
                    }

                    // get rid of the temp attribute again
                    MCore::GetAttributePool().Free(tempAttribute);
                }
                else
                {
                    MCore::LogWarning("AnimGraphObject::ReadAttributes() - Skipping attribute '%s' as the attribute is not found in the node (objectType='%s').", name.AsChar(), GetTypeString());

                    // skip the attribute data
                    uint8 tempByte;
                    for (uint32 b = 0; b < attributeSize; ++b)
                    {
                        stream->Read(&tempByte, 1);
                    }
                }
            }
            else
            {
                // check if the attribute type inside the file is different from the one of the anim graph object
                if (GetAttribute(index)->GetType() != attribType)
                {
                    // try to convert the attribute
                    MCore::Attribute* tempAttribute = MCore::GetAttributeFactory().CreateAttributeByType(attribType);
                    if (tempAttribute)
                    {
                        // try to read it
                        if (tempAttribute->Read(stream, endianType) == false)
                        {
                            return false;
                        }

                        // try to convert the attribute
                        if (ConvertAttribute(index, tempAttribute, name) == false)
                        {
                            MCore::LogWarning("AnimGraphObject::ReadAttributes() - Skipping attribute '%s' as the attribute type changed (fileAttribType=%s, nodeAttribType=%s, objectType=%s). The conversion failed.", name.AsChar(), tempAttribute->GetTypeString(), GetAttribute(index)->GetTypeString(), GetTypeString());
                        }
                        else
                        {
                            MCore::LogWarning("AnimGraphObject::ReadAttributes() - Converted attribute attribute '%s' from type (fileAttribType=%s, nodeAttribType=%s, objectType=%s).", name.AsChar(), tempAttribute->GetTypeString(), GetAttribute(index)->GetTypeString(), GetTypeString());
                        }

                        // get rid of the temp attribute again
                        MCore::GetAttributePool().Free(tempAttribute);
                    }
                    else
                    {
                        MCore::LogWarning("AnimGraphObject::ReadAttributes() - Skipping attribute '%s' as the attribute type changed (fileAttribType=%d, nodeAttribType=%d (%s), objectType=%s). We also cannot convert the attribute.", name.AsChar(), attribType, GetAttribute(index)->GetType(), GetAttribute(index)->GetTypeString(), GetTypeString());

                        // skip the attribute data
                        uint8 tempByte;
                        for (uint32 b = 0; b < attributeSize; ++b)
                        {
                            stream->Read(&tempByte, 1);
                        }
                    }
                }
                else
                if (GetAttribute(index)->Read(stream, endianType) == false)
                {
                    return false;
                }
            }
        }

        return true;
    }


    // skip reading the attributes
    bool AnimGraphObject::SkipReadAttributes(MCore::Stream* stream, uint32 numAttributes, uint32 version, MCore::Endian::EEndianType endianType)
    {
        MCORE_UNUSED(version);

        // for all attributes
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            // read the attribute size
            uint32 attribType;
            if (stream->Read(&attribType, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&attribType, endianType);

            // read the attribute size
            uint32 attributeSize;
            if (stream->Read(&attributeSize, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&attributeSize, endianType);

            // first read the number of characters
            uint32 numCharacters;
            if (stream->Read(&numCharacters, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numCharacters, endianType);

            uint8 tempByte;

            // skip reading the string
            if (numCharacters > 0)
            {
                // skip the attribute data
                for (uint32 c = 0; c < numCharacters; ++c)
                {
                    stream->Read(&tempByte, 1);
                }
            }

            // skip reading the attribute data
            for (uint32 c = 0; c < attributeSize; ++c)
            {
                stream->Read(&tempByte, 1);
            }
        }

        return true;
    }

    /*
    // get an attribute as float value
    float AnimGraphObject::GetAttributeAsFloat(const Attribute* attribute) const
    {
        const uint32 typeID = attribute->GetType();
        switch (typeID)
        {
            case AttributeFloat::TYPE_ID:   return static_cast<const AttributeFloat*>(attribute)->GetValue();
            case AttributeInt32::TYPE_ID:   return static_cast<const AttributeInt32*>(attribute)->GetValue();
            case AttributeBool::TYPE_ID:    return static_cast<const AttributeBool*>(attribute)->GetValue();
            default:
                return 0.0f;
        };
    }
    */

    // construct and output the information summary string for this object
    void AnimGraphObject::GetSummary(MCore::String* outResult) const
    {
        MCore::String attributeString = CreateAttributesString();
        outResult->Format("%s: %s", GetTypeString(), attributeString.AsChar());
    }


    // construct and output the tooltip for this object
    void AnimGraphObject::GetTooltip(MCore::String* outResult) const
    {
        MCore::String attributeString = CreateAttributesString();
        outResult->Format("%s: %s", GetTypeString(), attributeString.AsChar());
    }


    // save and return number of bytes written, when outputBuffer is nullptr only return num bytes it would write
    uint32 AnimGraphObject::SaveUniqueData(const AnimGraphInstance* animGraphInstance, uint8* outputBuffer) const
    {
        AnimGraphObjectData* data = animGraphInstance->FindUniqueObjectData(this);
        if (data)
        {
            return data->Save(outputBuffer);
        }

        return 0;
    }



    // load and return number of bytes read, when dataBuffer is nullptr, 0 should be returned
    uint32 AnimGraphObject::LoadUniqueData(const AnimGraphInstance* animGraphInstance, const uint8* dataBuffer)
    {
        AnimGraphObjectData* data = animGraphInstance->FindUniqueObjectData(this);
        if (data)
        {
            return data->Load(dataBuffer);
        }

        return 0;
    }


    // collect internal objects
    void AnimGraphObject::RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const
    {
        outObjects.Add(const_cast<AnimGraphObject*>(this));
    }


#ifdef EMFX_EMSTUDIOBUILD
    // enable or disable all the anim graph attributes
    void AnimGraphObject::EnableAllAttributes(bool enabled)
    {
        const uint32 numAttributes = GetNumAttributes();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (enabled)
            {
                mAttributeFlags[i] &= ~FLAG_DISABLED;
            }
            else
            {
                mAttributeFlags[i] |= FLAG_DISABLED;
            }
        }
    }
#endif


    // on default create a base class object
    void AnimGraphObject::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // try to find existing data
        AnimGraphObjectData* data = animGraphInstance->FindUniqueObjectData(this);
        if (data == nullptr) // doesn't exist
        {
            //AnimGraphObjectData* newData = new AnimGraphObjectData(this, animGraphInstance);
            AnimGraphObjectData* newData = GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(GetType(), this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(newData);
        }
    }


    // reset the unique data
    void AnimGraphObject::ResetUniqueData(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphObjectData* uniqueData = animGraphInstance->FindUniqueObjectData(this);
        if (uniqueData)
        {
            uniqueData->Reset();
        }
    }


    // register a sync attribute
    MCore::AttributeSettings* AnimGraphObject::RegisterSyncAttribute()
    {
        MCore::AttributeSettings* attributeInfo = RegisterAttribute("Sync Mode", "sync", "The synchronization method to use. Event track based will use event tracks, full clip based will ignore the events and sync as a full clip. If set to Event Track Based while no sync events exist inside the track a full clip based sync will be performed instead.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attributeInfo->ResizeComboValues(3);
        attributeInfo->SetComboValue(SYNCMODE_DISABLED,     "Disabled");
        attributeInfo->SetComboValue(SYNCMODE_TRACKBASED,   "Event Track Based");
        attributeInfo->SetComboValue(SYNCMODE_CLIPBASED,    "Full Clip Based");
        //  attributeInfo->SetDefaultValue( MCore::AttributeFloat::Create(SYNCMODE_TRACKBASED) );
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(SYNCMODE_DISABLED));
        attributeInfo->SetFlag(MCore::AttributeSettings::FLAGINDEX_REINITGUI_ONVALUECHANGE, true);
        return attributeInfo;
    }


    // register the event filter mode
    MCore::AttributeSettings* AnimGraphObject::RegisterEventFilterAttribute()
    {
        MCore::AttributeSettings* attributeInfo = RegisterAttribute("Event Filter Mode", "eventMode", "The event filter mode, which controls which events are passed further up the hierarchy.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attributeInfo->ResizeComboValues(4);
        attributeInfo->SetComboValue(EVENTMODE_MASTERONLY,  "Master Node Only");
        attributeInfo->SetComboValue(EVENTMODE_SLAVEONLY,   "Slave Node Only");
        attributeInfo->SetComboValue(EVENTMODE_BOTHNODES,   "Both Nodes");
        attributeInfo->SetComboValue(EVENTMODE_MOSTACTIVE,  "Most Active");
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(EVENTMODE_MOSTACTIVE));
        attributeInfo->SetFlag(MCore::AttributeSettings::FLAGINDEX_REINITGUI_ONVALUECHANGE, true);
        return attributeInfo;
    }


    // get a goal node attribute
    AttributeGoalNode* AnimGraphObject::GetAttributeGoalNode(uint32 index)
    {
        MCORE_ASSERT(mAttributeValues[index]->GetType() == AttributeGoalNode::TYPE_ID);
        return static_cast<AttributeGoalNode*>(mAttributeValues[index]);
    }


    // default update implementation
    void AnimGraphObject::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);
        animGraphInstance->EnableObjectFlags(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_UPDATE_READY);
    }


    // check for an error
    bool AnimGraphObject::GetHasErrorFlag(AnimGraphInstance* animGraphInstance) const
    {
        AnimGraphObjectData* uniqueData = animGraphInstance->FindUniqueObjectData(this);
        if (uniqueData)
        {
            return uniqueData->GetHasError();
        }
        else
        {
            return false;
        }
    }


    // set the error flag
    void AnimGraphObject::SetHasErrorFlag(AnimGraphInstance* animGraphInstance, bool hasError)
    {
        AnimGraphObjectData* uniqueData = animGraphInstance->FindUniqueObjectData(this);
        if (uniqueData == nullptr)
        {
            return;
        }

        uniqueData->SetHasError(hasError);
    }


    // convert attributes
    bool AnimGraphObject::ConvertAttribute(uint32 attributeIndex, const MCore::Attribute* attributeToConvert, const MCore::String& attributeName)
    {
        MCORE_UNUSED(attributeName);
        if (attributeIndex == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // if we have a float, but the attributes come in as bool or int, conver them into a float
        if (GetAnimGraphManager().GetAttributeInfo(this, attributeIndex)->GetDefaultValue()->GetType() == MCore::AttributeFloat::TYPE_ID)
        {
            if (attributeToConvert->GetType() == MCore::AttributeInt32::TYPE_ID)
            {
                GetAttributeFloat(attributeIndex)->SetValue((float)static_cast<const MCore::AttributeInt32*>(attributeToConvert)->GetValue());
                return true;
            }
            else
            if (attributeToConvert->GetType() == MCore::AttributeBool::TYPE_ID)
            {
                const bool value = static_cast<const MCore::AttributeBool*>(attributeToConvert)->GetValue();
                GetAttributeFloat(attributeIndex)->SetValue((value) ? 1.0f : 0.0f);
                return true;
            }
        }

        return false;
    }


    // recursively clone
    AnimGraphObject* AnimGraphObject::RecursiveClone(AnimGraph* animGraph, AnimGraphObject* parentObject)
    {
        MCORE_UNUSED(parentObject);

        // clone the node
        AnimGraphObject* result = Clone(animGraph);

        // add it to the animgraph
        animGraph->AddObject(result);

        // copy values
        const uint32 numAttributes = mAttributeValues.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            result->mAttributeValues[i]->InitFrom(mAttributeValues[i]);
        }

        return result;
    }


    // init the internal attributes
    void AnimGraphObject::InitInternalAttributes(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
        // currently base objects do not do this, but will come later
    }


    // remove the internal attributes
    void AnimGraphObject::RemoveInternalAttributesForAllInstances()
    {
        // currently base objects themselves do not have internal attributes yet, but this will come at some stage
    }


    // decrease internal attribute indices for index values higher than the specified parameter
    void AnimGraphObject::DecreaseInternalAttributeIndices(uint32 decreaseEverythingHigherThan)
    {
        MCORE_UNUSED(decreaseEverythingHigherThan);
        // currently no implementation for the base object type, but this will come later
    }


    // does the init for all anim graph instances in the parent animgraph
    void AnimGraphObject::InitInternalAttributesForAllInstances()
    {
        if (mAnimGraph == nullptr)
        {
            return;
        }

        const uint32 numInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            InitInternalAttributes(mAnimGraph->GetAnimGraphInstance(i));
        }
    }


    // scale
    void AnimGraphObject::Scale(float scaleFactor)
    {
        AnimGraphManager::AttributeInfoSet* attributeInfoSet = GetAnimGraphManager().FindAttributeInfoSet(this);

        const uint32 numAttribs = attributeInfoSet->mAttributes->GetNumAttributes();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            MCore::AttributeSettings* attribute = attributeInfoSet->mAttributes->GetAttribute(i);
            if (attribute->GetCanScale())
            {
                attribute->Scale(scaleFactor);
            }
        }
    }
}   // namespace EMotionFX
