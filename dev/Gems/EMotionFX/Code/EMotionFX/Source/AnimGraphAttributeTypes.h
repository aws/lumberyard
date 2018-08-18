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

// include the standard headers
#include "EMotionFXConfig.h"
#include "ActorInstance.h"
#include "Actor.h"
#include "Pose.h"
#include "AnimGraphPose.h"

#include <MCore/Source/StringIdPool.h>
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
#include <MCore/Source/StringConversions.h>


namespace EMotionFX
{
    enum : uint32
    {
        ATTRIBUTE_INTERFACETYPE_ROTATION                    = 18 // AttributeRotation
    };

    // forward declaration
    class AnimGraph;
    class AnimGraphNode;

    /**
     * A rotation attribute, which is internally represented by a quaternion.
     * To remember the rotation angles entered by the user we also store 3 floats.
     * If we would convert the quaternion into euler angles, instead of storing those explictly, then the values shown in the interface
     * could suddenly change, even while they end up in the same visual rotation. That's why we keep track of the angles separately.
     * The angles however are not used in any calculations by any blend nodes. They use the quaternion directly.
     */
    class EMFX_API AttributeRotation
        : public MCore::Attribute
    {
        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00001000
        };

        enum ERotationOrder : uint8
        {
            ROTATIONORDER_ZYX   = 0,
            ROTATIONORDER_ZXY   = 1,
            ROTATIONORDER_YZX   = 2,
            ROTATIONORDER_YXZ   = 3,
            ROTATIONORDER_XYZ   = 4,
            ROTATIONORDER_XZY   = 5
        };

        static AttributeRotation* Create();
        static AttributeRotation* Create(const AZ::Vector3& angles, const MCore::Quaternion& quat, ERotationOrder order);
        static AttributeRotation* Create(float xDeg, float yDeg, float zDeg);

        void UpdateRotationQuaternion();

        const MCore::Quaternion& GetRotationQuaternion() const              { return mRotation; }
        const MCore::Matrix GetRotationMatrix() const                       { return mRotation.ToMatrix(); }
        const AZ::Vector3& GetRotationAngles() const                        { return mDegrees; }
        void SetRotationAngles(const AZ::Vector3& degrees, bool updateQuat = true)
        {
            mDegrees = degrees;
            if (updateQuat)
            {
                UpdateRotationQuaternion();
            }
        }
        void SetDirect(const AZ::Vector3& angles, const MCore::Quaternion& quat, ERotationOrder order)       { mDegrees = angles; mRotation = quat; mOrder = order; }
        void SetDirect(const MCore::Quaternion& quat, bool updateAngles = false)
        {
            if (updateAngles)
            {
                mOrder = ROTATIONORDER_ZYX;
                AZ::Vector3 euler = quat.ToEuler();
                mDegrees.Set(MCore::Math::RadiansToDegrees(euler.GetX()), MCore::Math::RadiansToDegrees(euler.GetY()), MCore::Math::RadiansToDegrees(euler.GetZ()));
            }
            mRotation = quat;
        }

        void SetRotationOrder(ERotationOrder order)                         { mOrder = order; UpdateRotationQuaternion(); }
        ERotationOrder GetRotationOrder() const                             { return mOrder; }

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                            { return Create(mDegrees, mRotation, mOrder); }
        MCore::Attribute* CreateInstance(void* destMemory) override         { return new(destMemory) AttributeRotation(); }
        const char* GetTypeString() const override                          { return "Rotation"; }
        uint32 GetDefaultInterfaceType() const override                     { return ATTRIBUTE_INTERFACETYPE_ROTATION; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributeRotation* rot = static_cast<const AttributeRotation*>(other);
            mDegrees = rot->mDegrees;
            mRotation = rot->mRotation;
            mOrder = rot->mOrder;
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override
        {
            // split the array into parts
            AZStd::vector<AZStd::string> values;
            AzFramework::StringFunc::Tokenize(valueString.c_str(), values, MCore::CharacterConstants::comma, true /* keep empty strings */, true /* keep space strings */);

            if (values.size() != 8)
            {
                return false;
            }

            // verify if they are valid floats
            float floatValues[7];
            for (uint32 i = 0; i < 7; ++i)
            {
                if (!AzFramework::StringFunc::LooksLikeFloat(values[i].c_str(), &floatValues[i]))
                {
                    return false;
                }
            }

            // make sure the last value is an integer
            int order;
            if (!AzFramework::StringFunc::LooksLikeInt(values[7].c_str(), &order))
            {
                return false;
            }
            // check the validity of the order
            if (order < 0 || order > 5)
            {
                return false;
            }

            // convert the string parts into floats
            mDegrees.Set(floatValues[0], floatValues[1], floatValues[2]);
            mRotation.x = floatValues[3];
            mRotation.y = floatValues[4];
            mRotation.z = floatValues[5];
            mRotation.w = floatValues[6];

            mOrder = static_cast<ERotationOrder>(order);

            // renormalize the quaternion for safety
            mRotation.Normalize();
            return true;
        }

        bool ConvertToString(AZStd::string& outString) const override
        {
            outString = AZStd::string::format("%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%d",
                    static_cast<float>(mDegrees.GetX()),
                    static_cast<float>(mDegrees.GetY()),
                    static_cast<float>(mDegrees.GetZ()),
                    mRotation.x,
                    mRotation.y,
                    mRotation.z,
                    mRotation.w,
                    mOrder);
            return true;
        }
        uint32 GetClassSize() const override                                { return sizeof(AttributeRotation); }

    private:
        MCore::Quaternion   mRotation;      /**< The unit quaternion rotation. */
        AZ::Vector3         mDegrees;       /**< The rotation angles. As programmer you don't need to setup these values. They are only to display in the GUI. */
        ERotationOrder      mOrder;         /**< The rotation order, which defaults to ZYX. */

        AttributeRotation()
            : MCore::Attribute(TYPE_ID)                         { mDegrees = AZ::Vector3::CreateZero(); mOrder = ROTATIONORDER_ZYX; }
        AttributeRotation(const AZ::Vector3& angles, const MCore::Quaternion& quat)
            : MCore::Attribute(TYPE_ID)      { mDegrees = angles; mRotation = quat; mOrder = ROTATIONORDER_ZYX; }
        AttributeRotation(float xDeg, float yDeg, float zDeg)
            : MCore::Attribute(TYPE_ID)                         { mOrder = ROTATIONORDER_ZYX; mDegrees.Set(xDeg, yDeg, zDeg); UpdateRotationQuaternion(); }
        ~AttributeRotation()    {}

        uint32 GetDataSize() const override;
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override;
    };


    class EMFX_API AttributePose
        : public MCore::Attribute
    {
        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00001001
        };

        static AttributePose* Create();
        static AttributePose* Create(AnimGraphPose* pose);

        void SetValue(AnimGraphPose* value)                                { mValue = value; }
        AnimGraphPose* GetValue() const                                    { return mValue; }
        AnimGraphPose* GetValue()                                          { return mValue; }

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                            { return Create(mValue); }
        MCore::Attribute* CreateInstance(void* destMemory) override         { return new(destMemory) AttributePose(); }
        const char* GetTypeString() const override                          { return "Pose"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributePose* pose = static_cast<const AttributePose*>(other);
            mValue = pose->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override      { MCORE_UNUSED(valueString); return false; }    // unsupported
        bool ConvertToString(AZStd::string& outString) const override       { MCORE_UNUSED(outString); return false; }  // unsupported
        uint32 GetClassSize() const override                                { return sizeof(AttributePose); }
        uint32 GetDefaultInterfaceType() const override                     { return MCore::ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        AnimGraphPose* mValue;

        AttributePose()
            : MCore::Attribute(TYPE_ID)     { mValue = nullptr; }
        AttributePose(AnimGraphPose* pose)
            : MCore::Attribute(TYPE_ID)     { mValue = pose; }
        ~AttributePose() {}

        uint32 GetDataSize() const override                                 { return 0; }
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override   { MCORE_UNUSED(stream); MCORE_UNUSED(streamEndianType); MCORE_UNUSED(version); return false; }  // unsupported
    };


    class EMFX_API AttributeMotionInstance
        : public MCore::Attribute
    {
        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00001002
        };

        static AttributeMotionInstance* Create();
        static AttributeMotionInstance* Create(MotionInstance* motionInstance);

        void SetValue(MotionInstance* value)                                { mValue = value; }
        MotionInstance* GetValue() const                                    { return mValue; }
        MotionInstance* GetValue()                                          { return mValue; }

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                            { return Create(mValue); }
        MCore::Attribute* CreateInstance(void* destMemory) override         { return new(destMemory) AttributeMotionInstance(); }
        const char* GetTypeString() const override                          { return "MotionInstance"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributeMotionInstance* pose = static_cast<const AttributeMotionInstance*>(other);
            mValue = pose->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override      { MCORE_UNUSED(valueString); return false; }    // unsupported
        bool ConvertToString(AZStd::string& outString) const override       { MCORE_UNUSED(outString); return false; }  // unsupported
        uint32 GetClassSize() const override                                { return sizeof(AttributeMotionInstance); }
        uint32 GetDefaultInterfaceType() const override                     { return MCore::ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        MotionInstance* mValue;

        AttributeMotionInstance()
            : MCore::Attribute(TYPE_ID)     { mValue = nullptr; }
        AttributeMotionInstance(MotionInstance* motionInstance)
            : MCore::Attribute(TYPE_ID)     { mValue = motionInstance; }
        ~AttributeMotionInstance() {}

        uint32 GetDataSize() const override                                 { return 0; }
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override   { MCORE_UNUSED(stream); MCORE_UNUSED(streamEndianType); MCORE_UNUSED(version); return false; }  // unsupported
    };

    
} // namespace EMotionFX