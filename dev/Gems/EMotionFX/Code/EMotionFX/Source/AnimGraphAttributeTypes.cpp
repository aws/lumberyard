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
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphNodeGroup.h"
#include "AnimGraphNode.h"
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeSettings.h>
#include <MCore/Source/AttributeSettingsSet.h>
#include <MCore/Source/AttributePool.h>
#include <MCore/Source/MCoreSystem.h>
#include "Importer/SharedFileFormatStructs.h"
#include "AnimGraph.h"


namespace EMotionFX
{
    // static create
    AttributeRotation* AttributeRotation::Create()
    {
        return static_cast<AttributeRotation*>(MCore::GetAttributePool().RequestNew(AttributeRotation::TYPE_ID));
    }


    // static create
    AttributeRotation* AttributeRotation::Create(const MCore::Vector3& angles, const MCore::Quaternion& quat, ERotationOrder order)
    {
        AttributeRotation* result = static_cast<AttributeRotation*>(MCore::GetAttributePool().RequestNew(AttributeRotation::TYPE_ID));
        result->SetDirect(angles, quat, order);
        return result;
    }


    // static create
    AttributeRotation* AttributeRotation::Create(float xDeg, float yDeg, float zDeg)
    {
        AttributeRotation* result = static_cast<AttributeRotation*>(MCore::GetAttributePool().RequestNew(AttributeRotation::TYPE_ID));
        result->SetRotationAngles(MCore::Vector3(xDeg, yDeg, zDeg), true);
        return result;
    }



    void AttributeRotation::UpdateRotationQuaternion()
    {
        switch (mOrder)
        {
        case ROTATIONORDER_ZYX:
            mRotation = MCore::Quaternion(MCore::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.z)) *
                MCore::Quaternion(MCore::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.y)) *
                MCore::Quaternion(MCore::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.x));
            break;

        case ROTATIONORDER_ZXY:
            mRotation = MCore::Quaternion(MCore::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.z)) *
                MCore::Quaternion(MCore::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.x)) *
                MCore::Quaternion(MCore::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.y));
            break;

        case ROTATIONORDER_YZX:
            mRotation = MCore::Quaternion(MCore::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.y)) *
                MCore::Quaternion(MCore::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.z)) *
                MCore::Quaternion(MCore::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.x));
            break;

        case ROTATIONORDER_YXZ:
            mRotation = MCore::Quaternion(MCore::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.y)) *
                MCore::Quaternion(MCore::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.x)) *
                MCore::Quaternion(MCore::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.z));
            break;

        case ROTATIONORDER_XYZ:
            mRotation = MCore::Quaternion(MCore::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.x)) *
                MCore::Quaternion(MCore::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.y)) *
                MCore::Quaternion(MCore::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.z));
            break;

        case ROTATIONORDER_XZY:
            mRotation = MCore::Quaternion(MCore::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.x)) *
                MCore::Quaternion(MCore::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.z)) *
                MCore::Quaternion(MCore::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.y));
            break;

        default:
            mRotation = MCore::Quaternion(MCore::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.z)) *
                MCore::Quaternion(MCore::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.y)) *
                MCore::Quaternion(MCore::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.x));
            MCORE_ASSERT(false);
        }
        ;
    }



    uint32 AttributeRotation::GetDataSize() const
    {
        //     rotation angles          rotation quat               rotation order
        return sizeof(MCore::Vector3) + sizeof(MCore::Quaternion) + sizeof(uint8);
    }


    // read from a stream
    bool AttributeRotation::ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version)
    {
        if (version == 1)
        {
            // read the value
            MCore::Vector3 streamValue;
            if (stream->Read(&streamValue, sizeof(MCore::Vector3)) == 0)
            {
                return false;
            }

            // convert endian
            MCore::Endian::ConvertVector3(&streamValue, streamEndianType);

            // read only the degrees, automatically calculate the quaternion
            mDegrees = streamValue;
            mRotation.SetEuler(MCore::Math::DegreesToRadians(mDegrees.x), MCore::Math::DegreesToRadians(mDegrees.y), MCore::Math::DegreesToRadians(mDegrees.z));
        }
        else
        if (version == 2)
        {
            // read the value
            MCore::Vector3 streamValue;
            if (stream->Read(&streamValue, sizeof(MCore::Vector3)) == 0)
            {
                return false;
            }

            // convert endian
            MCore::Endian::ConvertVector3(&streamValue, streamEndianType);
            mDegrees = streamValue;

            // read the quaternion
            MCore::Quaternion streamValueQ;
            if (stream->Read(&streamValueQ, sizeof(MCore::Quaternion)) == 0)
            {
                return false;
            }

            // convert endian
            MCore::Endian::ConvertQuaternion(&streamValueQ, streamEndianType);
            mRotation = streamValueQ;
        }
        else
        if (version == 3)
        {
            // read the value
            MCore::Vector3 streamValue;
            if (stream->Read(&streamValue, sizeof(MCore::Vector3)) == 0)
            {
                return false;
            }

            // convert endian
            MCore::Endian::ConvertVector3(&streamValue, streamEndianType);
            mDegrees = streamValue;

            // read the quaternion
            MCore::Quaternion streamValueQ;
            if (stream->Read(&streamValueQ, sizeof(MCore::Quaternion)) == 0)
            {
                return false;
            }

            // convert endian
            MCore::Endian::ConvertQuaternion(&streamValueQ, streamEndianType);
            mRotation = streamValueQ;

            // read the rotation order
            uint8 order = 0;
            if (stream->Read(&order, sizeof(uint8)) == 0)
            {
                return false;
            }

            mOrder = (ERotationOrder)order;
        }

        return true;
    }


    // write to a stream
    bool AttributeRotation::WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const
    {
        // write the degrees
        MCore::Vector3 streamValue = mDegrees;
        MCore::Endian::ConvertVector3To(&streamValue, targetEndianType);
        if (stream->Write(&streamValue, sizeof(MCore::Vector3)) == 0)
        {
            return false;
        }

        // write the quaternion
        MCore::Quaternion streamValueQ = mRotation;
        MCore::Endian::ConvertQuaternionTo(&streamValueQ, targetEndianType);
        if (stream->Write(&streamValueQ, sizeof(MCore::Quaternion)) == 0)
        {
            return false;
        }

        // write the rotation order
        if (stream->Write(&mOrder, sizeof(uint8)) == 0)
        {
            return false;
        }

        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------

    // static create
    AttributePose* AttributePose::Create()
    {
        return static_cast<AttributePose*>(MCore::GetAttributePool().RequestNew(AttributePose::TYPE_ID));
    }


    // static create
    AttributePose* AttributePose::Create(AnimGraphPose* pose)
    {
        AttributePose* result = static_cast<AttributePose*>(MCore::GetAttributePool().RequestNew(AttributePose::TYPE_ID));
        result->SetValue(pose);
        return result;
    }


    //---------------------------------------------------------------------------------------------------------------------

    // static create
    AttributeMotionInstance* AttributeMotionInstance::Create()
    {
        return static_cast<AttributeMotionInstance*>(MCore::GetAttributePool().RequestNew(AttributeMotionInstance::TYPE_ID));
    }


    // static create
    AttributeMotionInstance* AttributeMotionInstance::Create(MotionInstance* motionInstance)
    {
        AttributeMotionInstance* result = static_cast<AttributeMotionInstance*>(MCore::GetAttributePool().RequestNew(AttributeMotionInstance::TYPE_ID));
        result->SetValue(motionInstance);
        return result;
    }

    //---------------------------------------------------------------------------------------------------------------------


    AttributeNodeMask* AttributeNodeMask::Create()
    {
        return static_cast<AttributeNodeMask*>(MCore::GetAttributePool().RequestNew(AttributeNodeMask::TYPE_ID));
    }


    AttributeNodeMask* AttributeNodeMask::Create(const MCore::Array<MaskEntry>& nodes)
    {
        AttributeNodeMask* result = static_cast<AttributeNodeMask*>(MCore::GetAttributePool().RequestNew(AttributeNodeMask::TYPE_ID));
        result->SetNodes(nodes);
        return result;
    }


    // get the data size
    uint32 AttributeNodeMask::GetDataSize() const
    {
        uint32 totalSizeInBytes = 0;

        // the number of entries int
        totalSizeInBytes += sizeof(uint32);

        const uint32 numEntries = mNodes.GetLength();
        for (uint32 i = 0; i < numEntries; ++i) // add the size of all mask entries
        {
            totalSizeInBytes += sizeof(float); // the weight value
            totalSizeInBytes += sizeof(uint32); // the number of characters/bytes for the string
            totalSizeInBytes += mNodes[i].mName.GetLength(); // the name string
        }

        // return the total size in bytes
        return totalSizeInBytes;
    }


    // read from a stream
    bool AttributeNodeMask::ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version)
    {
        MCORE_UNUSED(version);

        // read the number of entries
        uint32 numEntries;
        if (stream->Read(&numEntries, sizeof(uint32)) == 0)
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numEntries, streamEndianType);

        // read the entries
        mNodes.Resize(numEntries);
        for (uint32 i = 0; i < numEntries; ++i)
        {
            // read the weight
            float weight;
            if (stream->Read(&weight, sizeof(float)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertFloat(&weight, streamEndianType);

            // read the number of string bytes to follow
            uint32 numStringBytes;
            if (stream->Read(&numStringBytes, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numStringBytes, streamEndianType);

            if (numStringBytes > 0)
            {
                mNodes[i].mName.Resize(numStringBytes);

                // read the string data
                if (stream->Read(mNodes[i].mName.GetPtr(), numStringBytes) == 0)
                {
                    return false;
                }
            }
            else
            {
                mNodes[i].mName.Clear();
            }

            // add the entry to the mask
            mNodes[i].mWeight   = weight;
        }

        return true;
    }


    // write to the stream
    bool AttributeNodeMask::WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const
    {
        // save out the number of entries
        uint32 numEntries = mNodes.GetLength();
        MCore::Endian::ConvertUnsignedInt32To(&numEntries, targetEndianType);
        if (stream->Write(&numEntries, sizeof(uint32)) == 0)
        {
            return false;
        }

        // save all entries
        numEntries = mNodes.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            // write the weight
            float weight = mNodes[i].mWeight;
            MCore::Endian::ConvertFloatTo(&weight, targetEndianType);
            if (stream->Write(&weight, sizeof(float)) == 0)
            {
                return false;
            }

            // write the name string
            const MCore::String& name = mNodes[i].mName;
            uint32 numNameBytes = name.GetLength();
            MCore::Endian::ConvertUnsignedInt32To(&numNameBytes, targetEndianType);
            if (stream->Write(&numNameBytes, sizeof(uint32)) == 0)
            {
                return false;
            }

            // write the character data
            if (name.GetLength() > 0)
            {
                if (stream->Write(name.AsChar(), name.GetLength()) == 0)
                {
                    return false;
                }
            }
        }

        return true;
    }


    bool AttributeNodeMask::InitFromString(const MCore::String& valueString)
    {
        // decompose the string, which is structured as:
        // <nodename>;<weight>;<nodename>;<weight>;<nodename>;<weight>... etc
        MCore::Array<MCore::String> parts = valueString.Split(MCore::UnicodeCharacter::semiColon);
        const uint32 numParts = parts.GetLength();
        bool evenNumParts = (numParts % 2 == 0);
        if (evenNumParts == false) // make sure it is an even amount of data
        {
            return false;
        }

        // now rebuild the items
        const uint32 numItems = numParts / 2;
        mNodes.Resize(numItems);
        uint32 offset = 0;
        for (uint32 i = 0; i < numItems; ++i)
        {
            mNodes[i].mName     = parts[offset++];
            mNodes[i].mWeight   = parts[offset++].ToFloat();// convert the string into a float
        }

        return true;
    }


    bool AttributeNodeMask::ConvertToString(MCore::String& outString) const
    {
        if (mNodes.GetLength() == 0)
        {
            outString.Clear();
            return true;
        }

        // convert all entries into one big string formatted like:
        // <nodename>;<weight>;<nodename>;<weight>;<nodename>;<weight>... etc
        uint32 i;
        const uint32 numEntries = mNodes.GetLength();

        // reserve space for the string
        outString.Clear();
        uint32 reserveDataSize = 13 * numEntries;
        for (i = 0; i < numEntries; ++i) // add the size of all mask entries
        {
            reserveDataSize += mNodes[i].mName.GetLength() + 11; // some extra characters for the value and semicolon
        }
        outString.Reserve(reserveDataSize);

        for (i = 0; i < numEntries; ++i) // add the size of all mask entries
        {
            if (i < numEntries - 1)
            {
                outString.FormatAdd("%s;%.8f;", mNodes[i].mName.AsChar(), mNodes[i].mWeight);
            }
            else
            {
                outString.FormatAdd("%s;%.8f", mNodes[i].mName.AsChar(), mNodes[i].mWeight);
            }
        }

        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------


    // static create
    AttributeParameterMask* AttributeParameterMask::Create()
    {
        return static_cast<AttributeParameterMask*>(MCore::GetAttributePool().RequestNew(AttributeParameterMask::TYPE_ID));
    }


    // static create
    AttributeParameterMask* AttributeParameterMask::Create(const MCore::Array<uint32>& parameterIDs)
    {
        AttributeParameterMask* result = static_cast<AttributeParameterMask*>(MCore::GetAttributePool().RequestNew(AttributeParameterMask::TYPE_ID));
        result->SetParameters(parameterIDs);
        return result;
    }


    bool AttributeParameterMask::InitFromString(const MCore::String& valueString)
    {
        // decompose the string, which is structured as:
        // <parametername>;<parametername>;<parametername>;... etc
        MCore::Array<MCore::String> parts = valueString.Split(MCore::UnicodeCharacter::semiColon);
        const uint32 numParts = parts.GetLength();

        // now rebuild the items
        mParameterNameIDs.Resize(numParts);
        for (uint32 i = 0; i < numParts; ++i)
        {
            SetParameterName(i, parts[i].AsChar());
        }

        return true;
    }


    bool AttributeParameterMask::ConvertToString(MCore::String& outString) const
    {
        if (mParameterNameIDs.GetIsEmpty())
        {
            outString = "";
            return true;
        }

        // convert all entries into one big string formatted like:
        // <parametername>;<parametername>;<parametername>;... etc
        const uint32 numEntries = mParameterNameIDs.GetLength();
        for (uint32 i = 0; i < numEntries; ++i) // add the size of all mask entries
        {
            if (i < numEntries - 1)
            {
                outString.FormatAdd("%s;", GetParameterName(i));
            }
            else
            {
                outString.FormatAdd("%s", GetParameterName(i));
            }
        }

        return true;
    }


    // get the data size
    uint32 AttributeParameterMask::GetDataSize() const
    {
        uint32 totalSizeInBytes = 0;

        // the number of entries int
        totalSizeInBytes += sizeof(uint32);

        const uint32 numEntries = mParameterNameIDs.GetLength();
        for (uint32 i = 0; i < numEntries; ++i) // add the size of all mask entries
        {
            totalSizeInBytes += sizeof(uint32); // the number of characters/bytes for the string
            totalSizeInBytes += GetParameterNameString(i).GetLength(); // name string
        }

        // return the total size in bytes
        return totalSizeInBytes;
    }


    // read from a stream
    bool AttributeParameterMask::ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version)
    {
        MCORE_UNUSED(version);

        // read the number of entries
        uint32 numEntries;
        if (stream->Read(&numEntries, sizeof(uint32)) == 0)
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numEntries, streamEndianType);

        // read the entries
        MCore::String name;
        name.Reserve(64);
        mParameterNameIDs.Resize(numEntries);
        for (uint32 i = 0; i < numEntries; ++i)
        {
            // read the number of string bytes to follow
            uint32 numStringBytes;
            if (stream->Read(&numStringBytes, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numStringBytes, streamEndianType);

            if (numStringBytes > 0)
            {
                name.Resize(numStringBytes);

                // read the string data
                if (stream->Read(name.GetPtr(), numStringBytes) == 0)
                {
                    return false;
                }
            }
            else
            {
                name.Clear();
            }

            // add the entry to the mask
            SetParameterName(i, name.AsChar());
        }

        return true;
    }


    // write to the stream
    bool AttributeParameterMask::WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const
    {
        // save out the number of entries
        uint32 numEntries = mParameterNameIDs.GetLength();
        MCore::Endian::ConvertUnsignedInt32To(&numEntries, targetEndianType);
        if (stream->Write(&numEntries, sizeof(uint32)) == 0)
        {
            return false;
        }

        // save all entries
        numEntries = mParameterNameIDs.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            // write the name string
            uint32 numNameBytes = GetParameterNameString(i).GetLength();
            MCore::Endian::ConvertUnsignedInt32To(&numNameBytes, targetEndianType);
            if (stream->Write(&numNameBytes, sizeof(uint32)) == 0)
            {
                return false;
            }

            // write the character data
            if (GetParameterNameString(i).GetLength() > 0)
            {
                if (stream->Write(GetParameterNameString(i).AsChar(), GetParameterNameString(i).GetLength()) == 0)
                {
                    return false;
                }
            }
        }

        return true;
    }


    void AttributeParameterMask::Log()
    {
        const uint32 numParameters = GetNumParameterNames();
        MCore::LogInfo("AttributeParameterMask: (NumParameters=%d)", numParameters);
        for (uint32 i = 0; i < numParameters; ++i)
        {
            MCore::LogInfo("   - #%d: Name='%s' NameID=%d", i, GetParameterName(i), mParameterNameIDs[i]);
        }
    }


    void AttributeParameterMask::SetParameters(MCore::Array<MCore::String>& parameterNames)
    {
        // get the number of parameters and resize the parameter name id array
        const uint32 numParameters = parameterNames.GetLength();
        mParameterNameIDs.Resize(numParameters);

        // iterate through all parameters, generate and set the name ids for them
        for (uint32 i = 0; i < numParameters; ++i)
        {
            SetParameterName(i, parameterNames[i].AsChar());
        }
    }


    uint32 AttributeParameterMask::FindParameterNameIndex(const char* parameterName) const
    {
        // get the number of parameters and iterate through them
        const uint32 numParameters = mParameterNameIDs.GetLength();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            // compare the names and return in case they are equal
            if (GetParameterNameString(i).CheckIfIsEqual(parameterName))
            {
                return i;
            }
        }

        // failure, the given parameter name hasn't been found
        return MCORE_INVALIDINDEX32;
    }


    // find the local parameter index
    uint32 AttributeParameterMask::FindLocalParameterIndex(AnimGraph* animGraph, const char* parameterName) const
    {
        // in case there is no parameter mask set we can just use the anim graph find parameter index method
        if (GetNumParameterNames() == 0)
        {
            return animGraph->FindParameterIndex(parameterName);
        }

        // get the number of parameters and iterate through them
        uint32 parameterIndex = 0;
        const uint32 numParameters = animGraph->GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            // get the name of the currently iterated parameter
            const char*     currentParameterName = animGraph->GetParameter(i)->GetName();
            const uint32    currentParameterIndex = FindParameterNameIndex(currentParameterName);

            // compare the names and return the current parameter index
            if (animGraph->GetParameter(i)->GetNameString().CheckIfIsEqual(parameterName) && currentParameterIndex != MCORE_INVALIDINDEX32)
            {
                return parameterIndex;
            }

            // increase the parameter index in case the currently iterated parameter is part of the parameter mask
            if (currentParameterIndex != MCORE_INVALIDINDEX32)
            {
                parameterIndex++;
            }
        }

        // return failure, the parameter has not been found
        return MCORE_INVALIDINDEX32;
    }


    // sort the parameter mask
    void AttributeParameterMask::Sort(AnimGraph* animGraph)
    {
        MCore::Array<uint32> parameterNameIDs;

        // get the number of parameters inside the anim graph and iterate through them
        const uint32 numParameters = animGraph->GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            // get the name of the currently iterated parameter
            const char* currentParameterName = animGraph->GetParameter(i)->GetName();

            // check if the currently iterated parameter is part of the parameter mask and add it to the array
            if (FindParameterNameIndex(currentParameterName) != MCORE_INVALIDINDEX32)
            {
                parameterNameIDs.Add(animGraph->GetParameter(i)->GetInternalNameID());
            }
        }

        // finally copy over the sorted parameter name id array
        mParameterNameIDs = parameterNameIDs;
    }

    //---------------------------------------------------------------------------------------------------------------------

    // static create
    AttributeGoalNode* AttributeGoalNode::Create()
    {
        return static_cast<AttributeGoalNode*>(MCore::GetAttributePool().RequestNew(AttributeGoalNode::TYPE_ID));
    }


    // static create
    AttributeGoalNode* AttributeGoalNode::Create(const char* nodeName, uint32 parentDepth)
    {
        AttributeGoalNode* result = static_cast<AttributeGoalNode*>(MCore::GetAttributePool().RequestNew(AttributeGoalNode::TYPE_ID));
        result->SetNodeName(nodeName);
        result->SetParentDepth(parentDepth);
        return result;
    }


    bool AttributeGoalNode::InitFromString(const MCore::String& valueString)
    {
        MCore::Array<MCore::String> parts = valueString.Split(MCore::UnicodeCharacter::semiColon);
        if (parts.GetLength() != 2)
        {
            return false;
        }

        if (parts[1].CheckIfIsValidInt() == false)
        {
            return false;
        }

        int32 depth = parts[1].ToInt();
        if (depth < 0)
        {
            return false;
        }

        mNodeName       = parts[0];
        mParentDepth    = depth;
        return true;
    }


    bool AttributeGoalNode::ConvertToString(MCore::String& outString) const
    {
        outString.Format("%s;%d", mNodeName.AsChar(), mParentDepth);
        return true;
    }


    uint32 AttributeGoalNode::GetDataSize() const
    {
        return sizeof(uint32) + (mNodeName.GetLength() + sizeof(uint32));
    }


    // read from a stream
    bool AttributeGoalNode::ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version)
    {
        MCORE_UNUSED(version);

        // read the number of characters
        uint32 numCharacters;
        if (stream->Read(&numCharacters, sizeof(uint32)) == 0)
        {
            return false;
        }

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&numCharacters, streamEndianType);

        // read the character data
        if (numCharacters > 0)
        {
            mNodeName.Resize(numCharacters);
            if (stream->Read(mNodeName.GetPtr(), sizeof(char) * numCharacters) == 0)
            {
                return false;
            }
        }
        else
        {
            mNodeName.Clear();
        }

        // read the parent depth
        uint32 parentDepth;
        if (stream->Read(&parentDepth, sizeof(uint32)) == 0)
        {
            return false;
        }

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&parentDepth, streamEndianType);
        mParentDepth = parentDepth;
        return true;
    }


    // write to a stream
    bool AttributeGoalNode::WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const
    {
        // write the number of characters
        uint32 numCharacters = mNodeName.GetLength();
        MCore::Endian::ConvertUnsignedInt32To(&numCharacters, targetEndianType);
        if (stream->Write(&numCharacters, sizeof(uint32)) == 0)
        {
            return false;
        }

        // write the character data
        if (mNodeName.GetLength() > 0)
        {
            if (stream->Write(mNodeName.AsChar(), mNodeName.GetLength()) == 0)
            {
                return false;
            }
        }

        uint32 streamDepth = mParentDepth;
        MCore::Endian::ConvertUnsignedInt32To(&streamDepth, targetEndianType);
        if (stream->Write(&streamDepth, sizeof(uint32)) == 0)
        {
            return false;
        }

        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------

    // static create
    AttributeStateFilterLocal* AttributeStateFilterLocal::Create()
    {
        return static_cast<AttributeStateFilterLocal*>(MCore::GetAttributePool().RequestNew(AttributeStateFilterLocal::TYPE_ID));
    }

    // static create
    AttributeStateFilterLocal* AttributeStateFilterLocal::Create(const MCore::Array<uint32>& groupNameIDs, const MCore::Array<uint32>& nodeNameIDs)
    {
        AttributeStateFilterLocal* result = static_cast<AttributeStateFilterLocal*>(MCore::GetAttributePool().RequestNew(AttributeStateFilterLocal::TYPE_ID));
        result->Set(groupNameIDs, nodeNameIDs);
        return result;
    }


    // get the data size
    uint32 AttributeStateFilterLocal::GetDataSize() const
    {
        uint32 totalSizeInBytes = 0;

        // the number of node and group name entries int
        totalSizeInBytes += 2 * sizeof(uint32);

        // node groups
        const uint32 numGroupEntries = mGroupNameIDs.GetLength();
        for (uint32 i = 0; i < numGroupEntries; ++i)
        {
            totalSizeInBytes += sizeof(uint32);                     // the number of characters/bytes for the string
            totalSizeInBytes += GetNodeGroupNameString(i).GetLength(); // the name string
        }

        // nodes
        const uint32 numNodeEntries = mNodeNameIDs.GetLength();
        for (uint32 i = 0; i < numNodeEntries; ++i)
        {
            totalSizeInBytes += sizeof(uint32);                     // the number of characters/bytes for the string
            totalSizeInBytes += GetNodeNameString(i).GetLength();   // the name string
        }

        // return the total size in bytes
        return totalSizeInBytes;
    }


    // read from a stream
    bool AttributeStateFilterLocal::ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version)
    {
        MCORE_UNUSED(version);

        // read the number of group entries
        uint32 numGroupEntries;
        if (stream->Read(&numGroupEntries, sizeof(uint32)) == 0)
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numGroupEntries, streamEndianType);

        // read the number of node entries
        uint32 numNodeEntries;
        if (stream->Read(&numNodeEntries, sizeof(uint32)) == 0)
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numNodeEntries, streamEndianType);

        // read the group entries
        MCore::String convTemp;
        mGroupNameIDs.Resize(numGroupEntries);
        for (uint32 i = 0; i < numGroupEntries; ++i)
        {
            // read the number of string bytes to follow
            uint32 numStringBytes;
            if (stream->Read(&numStringBytes, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numStringBytes, streamEndianType);

            // read the string data
            if (numStringBytes > 0)
            {
                convTemp.Resize(numStringBytes);
                if (stream->Read(convTemp.GetPtr(), numStringBytes) == 0)
                {
                    return false;
                }
            }
            else
            {
                convTemp.Clear();
            }

            // add the entry to the mask
            SetNodeGroupName(i, convTemp.AsChar());
        }

        // read the node entries
        mNodeNameIDs.Resize(numNodeEntries);
        for (uint32 i = 0; i < numNodeEntries; ++i)
        {
            // read the number of string bytes to follow
            uint32 numStringBytes;
            if (stream->Read(&numStringBytes, sizeof(uint32)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numStringBytes, streamEndianType);

            // read the string data
            if (numStringBytes > 0)
            {
                convTemp.Resize(numStringBytes);
                if (stream->Read(convTemp.GetPtr(), numStringBytes) == 0)
                {
                    return false;
                }
            }
            else
            {
                convTemp.Clear();
            }

            // add the entry to the mask
            SetNodeName(i, convTemp.AsChar());
        }

        return true;
    }


    // write to the stream
    bool AttributeStateFilterLocal::WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const
    {
        // save out the number of group entries
        uint32 numGroupEntries = mGroupNameIDs.GetLength();
        MCore::Endian::ConvertUnsignedInt32To(&numGroupEntries, targetEndianType);
        if (stream->Write(&numGroupEntries, sizeof(uint32)) == 0)
        {
            return false;
        }

        // save out the number of node entries
        uint32 numNodeEntries = mNodeNameIDs.GetLength();
        MCore::Endian::ConvertUnsignedInt32To(&numNodeEntries, targetEndianType);
        if (stream->Write(&numNodeEntries, sizeof(uint32)) == 0)
        {
            return false;
        }

        // save all group entries
        numGroupEntries = mGroupNameIDs.GetLength();
        for (uint32 i = 0; i < numGroupEntries; ++i)
        {
            // write the name string
            const MCore::String& name = GetNodeGroupNameString(i);
            uint32 numNameBytes = name.GetLength();
            MCore::Endian::ConvertUnsignedInt32To(&numNameBytes, targetEndianType);
            if (stream->Write(&numNameBytes, sizeof(uint32)) == 0)
            {
                return false;
            }

            // write the character data
            if (name.GetLength() > 0)
            {
                if (stream->Write(name.AsChar(), name.GetLength()) == 0)
                {
                    return false;
                }
            }
        }

        // save all node entries
        numNodeEntries = mNodeNameIDs.GetLength();
        for (uint32 i = 0; i < numNodeEntries; ++i)
        {
            // write the name string
            const MCore::String& name = GetNodeNameString(i);
            uint32 numNameBytes = name.GetLength();
            MCore::Endian::ConvertUnsignedInt32To(&numNameBytes, targetEndianType);
            if (stream->Write(&numNameBytes, sizeof(uint32)) == 0)
            {
                return false;
            }

            // write the character data
            if (name.GetLength() > 0)
            {
                if (stream->Write(name.AsChar(), name.GetLength()) == 0)
                {
                    return false;
                }
            }
        }

        return true;
    }


    bool AttributeStateFilterLocal::InitFromString(const MCore::String& valueString)
    {
        // clear the arrays
        mGroupNameIDs.Clear();
        mNodeNameIDs.Clear();

        // decompose the string, which is structured as:
        // <#numGroups>;<#numNodes>;<group1>;<group2>;...;<node1>;<node2>;... etc
        MCore::Array<MCore::String> parts = valueString.Split(MCore::UnicodeCharacter::semiColon);
        const uint32 numParts = parts.GetLength();
        if (numParts < 2)
        {
            return false;
        }

        // get the number of groups and the number of nodes
        const uint32 numGroups  = parts[0].ToInt();
        const uint32 numNodes   = parts[1].ToInt();
        const uint32 numEntries = numGroups + numNodes;
        if (numParts != numEntries + 2)
        {
            MCore::LogError("AttributeStateFilterLocal::InitFromString(): Something is wrong with the attribute. It says to store %i groups and %i nodes, but has % string entries in total.", numGroups, numNodes, numParts);
            return false;
        }

        // resize the node group array and set the names
        uint32 partIndex = 2;
        mGroupNameIDs.Resize(numGroups);
        for (uint32 i = 0; i < numGroups; ++i)
        {
            SetNodeGroupName(i, parts[partIndex].AsChar());
            partIndex++;
        }

        // resize the node array and set the names
        mNodeNameIDs.Resize(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            SetNodeName(i, parts[partIndex].AsChar());
            partIndex++;
        }

        return true;
    }


    bool AttributeStateFilterLocal::ConvertToString(MCore::String& outString) const
    {
        // <#numGroups>;<#numNodes>;<group1>;<group2>;...;<node1>;<node2>;... etc

        const uint32 numGroups  = mGroupNameIDs.GetLength();
        const uint32 numNodes   = mNodeNameIDs.GetLength();
        outString.Format("%i;%i;", numGroups, numNodes);

        // add all groups to the string
        for (uint32 i = 0; i < numGroups; ++i)
        {
            outString += GetNodeGroupName(i);
            outString += ";";
        }

        // add all nodes to the string
        for (uint32 i = 0; i < numNodes; ++i)
        {
            outString += GetNodeName(i);
            outString += ";";
        }

        outString.TrimRight(MCore::UnicodeCharacter::semiColon);

        return true;
    }


    // set the node groups by their names
    void AttributeStateFilterLocal::SetNodeGroupNames(const MCore::Array<MCore::String>& groupNames)
    {
        // get the new number of node groups and resize the array accordingly
        const uint32 newNumGroups = groupNames.GetLength();
        mGroupNameIDs.Resize(newNumGroups);

        // set all node groups
        for (uint32 i = 0; i < newNumGroups; ++i)
        {
            SetNodeGroupName(i, groupNames[i].AsChar());
        }
    }


    // set the nodes by their names
    void AttributeStateFilterLocal::SetNodeNames(const MCore::Array<MCore::String>& nodeNames)
    {
        // get the new number of nodes and resize the array accordingly
        const uint32 newNumNodes = nodeNames.GetLength();
        mNodeNameIDs.Resize(newNumNodes);

        // set all node groups
        for (uint32 i = 0; i < newNumNodes; ++i)
        {
            SetNodeName(i, nodeNames[i].AsChar());
        }
    }


    // find the local index of the node group with the given name
    uint32 AttributeStateFilterLocal::FindNodeGroupNameIndex(const char* groupName) const
    {
        // get the number of node groups and iterate through them
        const uint32 numNodeGroups = mGroupNameIDs.GetLength();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // compare the names and return in case they are equal
            if (GetNodeGroupNameString(i).CheckIfIsEqual(groupName))
            {
                return i;
            }
        }

        // failure, the given node group name hasn't been found
        return MCORE_INVALIDINDEX32;
    }


    // find the local index of the node with the given name
    uint32 AttributeStateFilterLocal::FindNodeNameIndex(const char* nodeName) const
    {
        // get the new number of nodes and iterate through them
        const uint32 numNodes = mNodeNameIDs.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // compare the names and return in case they are equal
            if (GetNodeNameString(i).CheckIfIsEqual(nodeName))
            {
                return i;
            }
        }

        // failure, the given node name hasn't been found
        return MCORE_INVALIDINDEX32;
    }


    // calculate the number of total states inside the state filter attribute
    uint32 AttributeStateFilterLocal::CalcNumTotalStates(AnimGraph* animGraph, AnimGraphNode* parentNode) const
    {
        // initialize our new array using the node ids
        MCore::Array<uint32> stateIDs = mNodeNameIDs;

        // now add the node groups
        const uint32 numNodeGroups = mGroupNameIDs.GetLength();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // get the node group
            AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupByNameID(mGroupNameIDs[i]);
            if (nodeGroup == nullptr)
            {
                continue;
            }

            // get the number of nodes inside the node group and iterate through them
            const uint32 numNodes = nodeGroup->GetNumNodes();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                const uint32    nodeID  = nodeGroup->GetNode(n);
                AnimGraphNode* node    = animGraph->RecursiveFindNodeByID(nodeID);

                // make sure the node is part of the state machine to where the state filter belongs to
                if (parentNode->FindChildNodeIndex(node) != MCORE_INVALIDINDEX32)
                {
                    // add the node in case it is not in yet
                    if (stateIDs.Find(nodeID) == MCORE_INVALIDINDEX32)
                    {
                        stateIDs.Add(nodeID);
                    }
                }
            }
        }

        return stateIDs.GetLength();
    }

    //---------------------------------------------------------------------------------------------------------------------

    AttributeBlendSpaceMotion* AttributeBlendSpaceMotion::Create()
    {
        return static_cast<AttributeBlendSpaceMotion*>(MCore::GetAttributePool().RequestNew(AttributeBlendSpaceMotion::TYPE_ID));
    }

    AttributeBlendSpaceMotion* AttributeBlendSpaceMotion::Create(const AZStd::string& motionName)
    {
        AttributeBlendSpaceMotion* result = static_cast<AttributeBlendSpaceMotion*>(MCore::GetAttributePool().RequestNew(AttributeBlendSpaceMotion::TYPE_ID));
        result->Set(motionName, AZ::Vector2(0.0, 0.0), 0);
        return result;
    }

    AttributeBlendSpaceMotion* AttributeBlendSpaceMotion::Create(const AZStd::string& motionName, const AZ::Vector2& coordinates, AZ::u8 typeFlags)
    {
        AttributeBlendSpaceMotion* result = static_cast<AttributeBlendSpaceMotion*>(MCore::GetAttributePool().RequestNew(AttributeBlendSpaceMotion::TYPE_ID));
        result->Set(motionName, coordinates, typeFlags);
        return result;
    }

    void AttributeBlendSpaceMotion::Set(const AZStd::string& motionId, const AZ::Vector2& coordinates, AZ::u8 typeFlags)
    {
        m_motionId      = motionId;
        m_coordinates   = coordinates;
        mTypeFlags      = typeFlags;
    }

    void AttributeBlendSpaceMotion::MarkXCoordinateSetByUser(bool setByUser)
    {
        if (setByUser)
        {
            mTypeFlags |= UserSetCoordinateX;
        }
        else
        {
            mTypeFlags &= ~UserSetCoordinateX;
        }
    }

    void AttributeBlendSpaceMotion::MarkYCoordinateSetByUser(bool setByUser)
    {
        if (setByUser)
        {
            mTypeFlags |= UserSetCoordinateY;
        }
        else
        {
            mTypeFlags &= ~UserSetCoordinateY;
        }
    }

    bool AttributeBlendSpaceMotion::IsXCoordinateSetByUser() const
    {
        return (mTypeFlags & UserSetCoordinateX) != 0;
    }

    bool AttributeBlendSpaceMotion::IsYCoordinateSetByUser() const
    {
        return (mTypeFlags & UserSetCoordinateY) != 0;
    }

    int AttributeBlendSpaceMotion::GetDimension() const
    {
        if (mTypeFlags & BlendSpace1D)
            return 1;

        if (mTypeFlags & BlendSpace2D)
            return 2;

        return 0;
    }

    void AttributeBlendSpaceMotion::SetDimension(int dimension)
    {
        switch (dimension)
        {
            case 1:
                mTypeFlags |= BlendSpace1D;
                break;
            case 2:
                mTypeFlags |= BlendSpace2D;
                break;
            default:
                AZ_Assert(false, "Unexpected value for dimension");
        }
    }

    bool AttributeBlendSpaceMotion::InitFrom(const MCore::Attribute* other)
    {
        if (other->GetType() != TYPE_ID)
        {
            return false;
        }

        const AttributeBlendSpaceMotion* blendMotion = static_cast<const AttributeBlendSpaceMotion*>(other);

        m_motionId  = blendMotion->m_motionId;
        m_coordinates  = blendMotion->m_coordinates;
        mTypeFlags  = blendMotion->mTypeFlags;

        return true;
    }

    bool AttributeBlendSpaceMotion::InitFromString(const MCore::String& valueString)
    {
        MCore::Array<MCore::String> values = valueString.Split(MCore::UnicodeCharacter::comma);
        if (values.GetLength() != 4)
        {
            return false;
        }

        if (!values[1].CheckIfIsValidFloat() ||
            !values[2].CheckIfIsValidFloat() ||
            !values[3].CheckIfIsValidInt())
        {
            return false;
        }

        m_coordinates.SetElement(0, values[1].ToFloat());
        m_coordinates.SetElement(1, values[2].ToFloat());

        mTypeFlags = (AZ::u8)values[3].ToInt();

        return true;
    }

    bool AttributeBlendSpaceMotion::ConvertToString(MCore::String& outString) const
    { 
        outString.Format("%s,%.8f,%.8f,%d", m_motionId.c_str(), m_coordinates.GetX(), m_coordinates.GetY(), mTypeFlags);
        return true; 
    }

    uint32 AttributeBlendSpaceMotion::GetDataSize() const
    {
        return sizeof(AZ::u32) + static_cast<AZ::u32>(m_motionId.size()) // motionId
            + sizeof(FileFormat::FileVector2) // coordinates
            + sizeof(uint8); // flags
    }

    bool AttributeBlendSpaceMotion::ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version)
    {
        MCORE_UNUSED(version);

        // Read motion id.
        AZ::u32 numCharacters;
        if (stream->Read(&numCharacters, sizeof(AZ::u32)) == 0)
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numCharacters, streamEndianType);
        if (numCharacters == 0)
        {
            m_motionId.clear();
        }
        else
        {
            m_motionId.resize(numCharacters);
            if (stream->Read((void*)m_motionId.c_str(), numCharacters) != numCharacters)
            {
                return false;
            }
        }

        // Read the motion coordinates.
        FileFormat::FileVector2 filePos;
        if (stream->Read(&filePos, sizeof(FileFormat::FileVector2)) != sizeof(FileFormat::FileVector2))
        {
            return false;
        }
        MCore::Endian::ConvertFloat(&filePos.mX, streamEndianType);
        MCore::Endian::ConvertFloat(&filePos.mY, streamEndianType);
        m_coordinates.Set(filePos.mX, filePos.mY);


        AZ::u8 streamTypeFlags;
        if (stream->Read(&streamTypeFlags, sizeof(AZ::u8)) == 0)
        {
            return false;
        }
        mTypeFlags = streamTypeFlags;

        return true;
    }

    bool AttributeBlendSpaceMotion::WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const
    {
        // Write the motion id.
        AZ::u32 numChars = static_cast<AZ::u32>(m_motionId.size());
        MCore::Endian::ConvertUnsignedInt32To(&numChars, targetEndianType);
        if (stream->Write(&numChars, sizeof(AZ::u32)) == 0)
        {
            return false;
        }
        if (stream->Write(m_motionId.c_str(), numChars) == 0)
        {
            return false;
        }

        // Write the motion coordinates.
        FileFormat::FileVector2 fileParam;
        fileParam.mX = m_coordinates.GetX();
        fileParam.mY = m_coordinates.GetY();
        MCore::Endian::ConvertFloat(&fileParam.mX, targetEndianType);
        MCore::Endian::ConvertFloat(&fileParam.mY, targetEndianType);
        if (stream->Write(&fileParam, sizeof(FileFormat::FileVector2)) == 0)
        {
            return false;
        }

        if (stream->Write(&mTypeFlags, sizeof(AZ::u8)) == 0)
        {
            return false;
        }

        return true;
    }

    AttributeBlendSpaceMotion::AttributeBlendSpaceMotion()
        : MCore::Attribute(TYPE_ID)
        , m_coordinates(0.0f, 0.0f)
        , mTypeFlags(0)
    {
    }

    AttributeBlendSpaceMotion::AttributeBlendSpaceMotion(const AZStd::string& motionId, const AZ::Vector2& coordinates, AZ::u8 typeFlags)
        : MCore::Attribute(TYPE_ID)
        , m_motionId(motionId)
        , m_coordinates(coordinates)
        , mTypeFlags(typeFlags)
    {
    }
} // namespace EMotionFX