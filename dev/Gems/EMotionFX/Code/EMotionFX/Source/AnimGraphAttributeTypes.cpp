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

#include <AzCore/Casting/numeric_cast.h>
#include "EMotionFXConfig.h"
#include "AnimGraphNodeGroup.h"
#include "AnimGraphNode.h"
#include <MCore/Source/Attribute.h>
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
    AttributeRotation* AttributeRotation::Create(const AZ::Vector3& angles, const MCore::Quaternion& quat, ERotationOrder order)
    {
        AttributeRotation* result = static_cast<AttributeRotation*>(MCore::GetAttributePool().RequestNew(AttributeRotation::TYPE_ID));
        result->SetDirect(angles, quat, order);
        return result;
    }


    // static create
    AttributeRotation* AttributeRotation::Create(float xDeg, float yDeg, float zDeg)
    {
        AttributeRotation* result = static_cast<AttributeRotation*>(MCore::GetAttributePool().RequestNew(AttributeRotation::TYPE_ID));
        result->SetRotationAngles(AZ::Vector3(xDeg, yDeg, zDeg), true);
        return result;
    }



    void AttributeRotation::UpdateRotationQuaternion()
    {
        switch (mOrder)
        {
        case ROTATIONORDER_ZYX:
            mRotation = MCore::Quaternion(AZ::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.GetZ())) *
                MCore::Quaternion(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetY())) *
                MCore::Quaternion(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetX()));
            break;

        case ROTATIONORDER_ZXY:
            mRotation = MCore::Quaternion(AZ::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.GetZ())) *
                MCore::Quaternion(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetX())) *
                MCore::Quaternion(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetY()));
            break;

        case ROTATIONORDER_YZX:
            mRotation = MCore::Quaternion(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetY())) *
                MCore::Quaternion(AZ::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.GetZ())) *
                MCore::Quaternion(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetX()));
            break;

        case ROTATIONORDER_YXZ:
            mRotation = MCore::Quaternion(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetY())) *
                MCore::Quaternion(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetX())) *
                MCore::Quaternion(AZ::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.GetZ()));
            break;

        case ROTATIONORDER_XYZ:
            mRotation = MCore::Quaternion(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetX())) *
                MCore::Quaternion(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetY())) *
                MCore::Quaternion(AZ::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.GetZ()));
            break;

        case ROTATIONORDER_XZY:
            mRotation = MCore::Quaternion(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetX())) *
                MCore::Quaternion(AZ::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.GetZ())) *
                MCore::Quaternion(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetY()));
            break;

        default:
            mRotation = MCore::Quaternion(AZ::Vector3(0.0f, 0.0f, 1.0f), MCore::Math::DegreesToRadians(mDegrees.GetZ())) *
                MCore::Quaternion(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetY())) *
                MCore::Quaternion(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::DegreesToRadians(mDegrees.GetX()));
            MCORE_ASSERT(false);
        }
        ;
    }



    uint32 AttributeRotation::GetDataSize() const
    {
        //     rotation angles          rotation quat               rotation order
        return sizeof(AZ::Vector3) + sizeof(MCore::Quaternion) + sizeof(uint8);
    }


    // read from a stream
    bool AttributeRotation::ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version)
    {
        if (version == 1)
        {
            // read the value
            AZ::PackedVector3f streamValue;
            if (stream->Read(&streamValue, sizeof(AZ::PackedVector3f)) == 0)
            {
                return false;
            }

            // convert endian
            MCore::Endian::ConvertVector3(&streamValue, streamEndianType);

            // read only the degrees, automatically calculate the quaternion
            mDegrees = AZ::Vector3(streamValue);
            mRotation.SetEuler(MCore::Math::DegreesToRadians(mDegrees.GetX()), MCore::Math::DegreesToRadians(mDegrees.GetY()), MCore::Math::DegreesToRadians(mDegrees.GetZ()));
        }
        else
        if (version == 2)
        {
            // read the value
            AZ::PackedVector3f streamValue;
            if (stream->Read(&streamValue, sizeof(AZ::PackedVector3f)) == 0)
            {
                return false;
            }

            // convert endian
            MCore::Endian::ConvertVector3(&streamValue, streamEndianType);
            mDegrees = AZ::Vector3(streamValue);

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
            AZ::PackedVector3f streamValue;
            if (stream->Read(&streamValue, sizeof(AZ::PackedVector3f)) == 0)
            {
                return false;
            }

            // convert endian
            MCore::Endian::ConvertVector3(&streamValue, streamEndianType);
            mDegrees = AZ::Vector3(streamValue);

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


} // namespace EMotionFX