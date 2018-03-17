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

#include <MCore/Source/StringIDGenerator.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeInt32.h>
#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AttributeArray.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AttributeVector3.h>
#include <MCore/Source/AttributeVector4.h>
#include <MCore/Source/AttributeQuaternion.h>
#include <MCore/Source/AttributeColor.h>


namespace EMotionFX
{
    enum : uint32
    {
        ATTRIBUTE_INTERFACETYPE_NODENAMES                   = 12,// AttributeNodeMask
        ATTRIBUTE_INTERFACETYPE_MOTIONPICKER                = 13,// String
        ATTRIBUTE_INTERFACETYPE_PARAMETERPICKER             = 14,// String
        ATTRIBUTE_INTERFACETYPE_BLENDTREEMOTIONNODEPICKER   = 15,// String
        ATTRIBUTE_INTERFACETYPE_MOTIONEVENTTRACKPICKER      = 16,// int
        ATTRIBUTE_INTERFACETYPE_NODESELECTION               = 17,// String
        ATTRIBUTE_INTERFACETYPE_ROTATION                    = 18,// AttributeRotation
        ATTRIBUTE_INTERFACETYPE_GOALNODESELECTION           = 19,// AttributeGoalNode
        ATTRIBUTE_INTERFACETYPE_MOTIONEXTRACTIONCOMPONENTS  = 20,// int
        ATTRIBUTE_INTERFACETYPE_PARAMETERNAMES              = 21,// AttributeParameterMask
        ATTRIBUTE_INTERFACETYPE_STATESELECTION              = 22,// String
        ATTRIBUTE_INTERFACETYPE_STATEFILTERLOCAL            = 23,// AttributeStateFilterLocal
        ATTRIBUTE_INTERFACETYPE_ANIMGRAPHNODEPICKER         = 24,// String
        ATTRIBUTE_INTERFACETYPE_MULTIPLEMOTIONPICKER        = 25,// AttributeArray (of AttributeString attributes)
        ATTRIBUTE_INTERFACETYPE_TAG                         = 26,// bool
        ATTRIBUTE_INTERFACETYPE_TAGPICKER                   = 27,// AttributeArray (of AttributeString attributes)
        ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONS           = 28,// AttributeArray (of AttributeBlendSpaceMotion attributes)
        ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONPICKER      = 29,// String
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
        uint8 GetStreamWriteVersion() const override                        { return 3; }   // the version we write to the streams
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
        bool InitFromString(const MCore::String& valueString) override
        {
            // split the array into parts
            MCore::Array<MCore::String> values = valueString.Split(MCore::UnicodeCharacter::comma);
            if (values.GetLength() != 8)
            {
                return false;
            }

            // verify if they are valid floats
            for (uint32 i = 0; i < 7; ++i)
            {
                if (values[i].CheckIfIsValidFloat() == false)
                {
                    return false;
                }
            }

            // make sure the last value is an integer
            if (values[7].CheckIfIsValidInt() == false)
            {
                return false;
            }

            // convert the string parts into floats
            mDegrees.SetX(values[0].ToFloat());
            mDegrees.SetY(values[1].ToFloat());
            mDegrees.SetZ(values[2].ToFloat());
            mRotation.x = values[3].ToFloat();
            mRotation.y = values[4].ToFloat();
            mRotation.z = values[5].ToFloat();
            mRotation.w = values[6].ToFloat();
            const int32 order = values[7].ToInt();
            ;
            mOrder      = (ERotationOrder)order;

            // check the validity of the order
            if (order < 0 || order > 5)
            {
                return false;
            }

            // renormalize the quaternion for safety
            mRotation.Normalize();
            return true;
        }

        bool ConvertToString(MCore::String& outString) const override       { outString.Format("%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%d", static_cast<float>(mDegrees.GetX()), static_cast<float>(mDegrees.GetY()), static_cast<float>(mDegrees.GetZ()), mRotation.x, mRotation.y, mRotation.z, mRotation.w, mOrder); return true; }
        uint32 GetClassSize() const override                                { return sizeof(AttributeRotation); }

    private:
        MCore::Quaternion   mRotation;      /**< The unit quaternion rotation. */
        AZ::Vector3         mDegrees;       /**< The rotation angles. As programmer you don't need to setup these values. They are only to display in the GUI. */
        ERotationOrder      mOrder;         /**< The rotation order, which defaults to ZYX. */

        AttributeRotation()
            : MCore::Attribute(TYPE_ID)     { mDegrees = AZ::Vector3::CreateZero(); mOrder = ROTATIONORDER_ZYX; }
        AttributeRotation(const AZ::Vector3& angles, const MCore::Quaternion& quat)
            : MCore::Attribute(TYPE_ID)     { mDegrees = angles; mRotation = quat; mOrder = ROTATIONORDER_ZYX; }
        AttributeRotation(float xDeg, float yDeg, float zDeg)
            : MCore::Attribute(TYPE_ID)     { mOrder = ROTATIONORDER_ZYX; mDegrees.Set(xDeg, yDeg, zDeg); UpdateRotationQuaternion(); }
        ~AttributeRotation()    {}

        uint32 GetDataSize() const override;
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override;
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override;
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
        bool InitFromString(const MCore::String& valueString) override      { MCORE_UNUSED(valueString); return false; }    // unsupported
        bool ConvertToString(MCore::String& outString) const override       { MCORE_UNUSED(outString); return false; }  // unsupported
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
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override           { MCORE_UNUSED(stream); MCORE_UNUSED(targetEndianType); return false; } // unsupported
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
        bool InitFromString(const MCore::String& valueString) override      { MCORE_UNUSED(valueString); return false; }    // unsupported
        bool ConvertToString(MCore::String& outString) const override       { MCORE_UNUSED(outString); return false; }  // unsupported
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
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override           { MCORE_UNUSED(stream); MCORE_UNUSED(targetEndianType); return false; } // unsupported
    };


    class EMFX_API AttributeNodeMask
        : public MCore::Attribute
    {
        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00001003
        };

        struct MaskEntry
        {
            MCore::String   mName;
            float           mWeight;
        };

        static AttributeNodeMask* Create();
        static AttributeNodeMask* Create(const MCore::Array<MaskEntry>& nodes);

        void SetNodes(const MCore::Array<MaskEntry>& nodes)                 { mNodes = nodes; }
        const MCore::Array<MaskEntry>& GetNodes() const                     { return mNodes; }
        MCore::Array<MaskEntry>& GetNodes()                                 { return mNodes; }
        const MaskEntry& GetNode(uint32 index) const                        { return mNodes[index]; }
        MaskEntry& GetNode(uint32 index)                                    { return mNodes[index]; }
        uint32 GetNumNodes() const                                          { return mNodes.GetLength(); }

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                            { return Create(mNodes); }
        MCore::Attribute* CreateInstance(void* destMemory) override         { return new(destMemory) AttributeNodeMask(); }
        const char* GetTypeString() const override                          { return "NodeMask"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributeNodeMask* pose = static_cast<const AttributeNodeMask*>(other);
            mNodes = pose->mNodes;
            return true;
        }

        bool InitFromString(const MCore::String& valueString) override;
        bool ConvertToString(MCore::String& outString) const override;
        uint32 GetClassSize() const override                                { return sizeof(AttributeNodeMask); }
        uint32 GetDefaultInterfaceType() const override                     { return ATTRIBUTE_INTERFACETYPE_NODENAMES; }

    private:
        MCore::Array<MaskEntry>     mNodes;

        AttributeNodeMask()
            : MCore::Attribute(TYPE_ID)     { mNodes.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_ATTRIBUTES); }
        AttributeNodeMask(const MCore::Array<MaskEntry>& nodes)
            : MCore::Attribute(TYPE_ID)     { mNodes.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_ATTRIBUTES); mNodes = nodes; }
        ~AttributeNodeMask() {}

        uint32 GetDataSize() const override;
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override;
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override;
    };



    class EMFX_API AttributeParameterMask
        : public MCore::Attribute
    {
        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00201005
        };

        static AttributeParameterMask* Create();
        static AttributeParameterMask* Create(const MCore::Array<uint32>& parameterIDs);

        void SetParameters(MCore::Array<MCore::String>& parameterNames);
        const char* GetParameterName(uint32 localIndex) const
        {
            if (localIndex != MCORE_INVALIDINDEX32)
            {
                return MCore::GetStringIDGenerator().GetName(mParameterNameIDs[localIndex]).AsChar();
            }
            else
            {
                return "";
            }
        }
        const MCore::String& GetParameterNameString(uint32 localIndex) const        { return MCore::GetStringIDGenerator().GetName(mParameterNameIDs[localIndex]); }
        void SetParameterName(uint32 localIndex, const char* parameterName)         { mParameterNameIDs[localIndex] = MCore::GetStringIDGenerator().GenerateIDForString(parameterName); }
        uint32 GetNumParameterNames() const                                         { return mParameterNameIDs.GetLength(); }
        void AddParameterName(const char* parameterName)                            { mParameterNameIDs.Add(MCore::GetStringIDGenerator().GenerateIDForString(parameterName)); }
        void RemoveParameterName(uint32 localIndex)                                 { mParameterNameIDs.Remove(localIndex); }
        void SetParameters(const MCore::Array<uint32>& parameterIDs)                { mParameterNameIDs = parameterIDs; }
        void Sort(AnimGraph* animGraph);

        void Log();
        uint32 FindParameterNameIndex(const char* parameterName) const;

        /**
         * Find the local parameter index (to be used on a parameter node with a parameter mask set) based on the parameter name.
         */
        uint32 FindLocalParameterIndex(AnimGraph* animGraph, const char* parameterName) const;

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                                    { return Create(mParameterNameIDs); }
        MCore::Attribute* CreateInstance(void* destMemory) override                 { return new(destMemory) AttributeParameterMask(); }
        const char* GetTypeString() const override                                  { return "ParameterMask"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributeParameterMask* mask = static_cast<const AttributeParameterMask*>(other);
            mParameterNameIDs = mask->mParameterNameIDs;
            return true;
        }

        bool InitFromString(const MCore::String& valueString) override;
        bool ConvertToString(MCore::String& outString) const override;
        uint32 GetClassSize() const override                                        { return sizeof(AttributeParameterMask); }
        uint32 GetDefaultInterfaceType() const override                             { return ATTRIBUTE_INTERFACETYPE_PARAMETERNAMES; }

    private:
        MCore::Array<uint32>    mParameterNameIDs;

        AttributeParameterMask()
            : MCore::Attribute(TYPE_ID)     { mParameterNameIDs.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_ATTRIBUTES); }
        AttributeParameterMask(const MCore::Array<uint32>& parameterIDs)
            : MCore::Attribute(TYPE_ID)     { mParameterNameIDs.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_ATTRIBUTES); mParameterNameIDs = parameterIDs; }
        ~AttributeParameterMask() {}

        uint32 GetDataSize() const override;
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override;
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override;
    };



    class EMFX_API AttributeGoalNode
        : public MCore::Attribute
    {
        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00001004
        };

        static AttributeGoalNode* Create();
        static AttributeGoalNode* Create(const char* nodeName, uint32 parentDepth);

        void SetNodeName(const char* nodeName)                                  { mNodeName = nodeName; }
        void SetParentDepth(uint32 depth)                                       { mParentDepth = depth; }
        const char* GetNodeName() const                                         { return mNodeName.AsChar(); }
        const MCore::String& GetNodeNameString() const                          { return mNodeName; }
        uint32 GetParentDepth() const                                           { return mParentDepth; }

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                                { return Create(mNodeName.AsChar(), mParentDepth); }
        MCore::Attribute* CreateInstance(void* destMemory) override             { return new(destMemory) AttributeGoalNode(); }
        const char* GetTypeString() const override                              { return "GoalNode"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributeGoalNode* cast = static_cast<const AttributeGoalNode*>(other);
            mNodeName = cast->mNodeName;
            mParentDepth = cast->mParentDepth;
            return true;
        }
        bool InitFromString(const MCore::String& valueString) override;
        bool ConvertToString(MCore::String& outString) const override;
        uint32 GetClassSize() const override                                    { return sizeof(AttributeGoalNode); }
        uint32 GetDefaultInterfaceType() const override                         { return ATTRIBUTE_INTERFACETYPE_GOALNODESELECTION; }

    private:
        MCore::String       mNodeName;
        uint32              mParentDepth;   // 0=current, 1=parent, 2=parent of parent, 3=parent of parent of parent, etc

        AttributeGoalNode()
            : MCore::Attribute(TYPE_ID)     { mParentDepth = 0; }
        AttributeGoalNode(const char* nodeName, uint32 parentDepth)
            : MCore::Attribute(TYPE_ID)     { mNodeName = nodeName; mParentDepth = parentDepth; }
        ~AttributeGoalNode()    {}

        uint32 GetDataSize() const override;
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override;
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override;
    };


    class EMFX_API AttributeStateFilterLocal
        : public MCore::Attribute
    {
        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00001007
        };

        static AttributeStateFilterLocal* Create();
        static AttributeStateFilterLocal* Create(const MCore::Array<uint32>& groupNameIDs, const MCore::Array<uint32>& nodeNameIDs);

        // set the node groups and nodes by name
        void SetNodeGroupNames(const MCore::Array<MCore::String>& groupNames);
        void SetNodeNames(const MCore::Array<MCore::String>& nodeNames);
        void Set(const MCore::Array<uint32>& groupNameIDs, const MCore::Array<uint32>& nodeNameIDs)     { mGroupNameIDs = groupNameIDs; mNodeNameIDs = nodeNameIDs; }

        // get the number of node groups and nodes
        uint32 GetNumNodes() const                                                  { return mNodeNameIDs.GetLength(); }
        uint32 GetNumNodeGroups() const                                             { return mGroupNameIDs.GetLength(); }
        bool GetIsEmpty() const                                                     { return (mNodeNameIDs.GetIsEmpty() && mGroupNameIDs.GetIsEmpty()); }
        uint32 CalcNumTotalStates(AnimGraph* animGraph, AnimGraphNode* parentNode) const;

        // node groups
        const char* GetNodeGroupName(uint32 localIndex) const
        {
            if (localIndex != MCORE_INVALIDINDEX32)
            {
                return MCore::GetStringIDGenerator().GetName(mGroupNameIDs[localIndex]).AsChar();
            }
            else
            {
                return "";
            }
        }
        uint32 GetNodeGroupNameID(uint32 localIndex) const
        {
            if (localIndex != MCORE_INVALIDINDEX32)
            {
                return mGroupNameIDs[localIndex];
            }
            else
            {
                return MCORE_INVALIDINDEX32;
            }
        }
        const MCore::String& GetNodeGroupNameString(uint32 localIndex) const        { return MCore::GetStringIDGenerator().GetName(mGroupNameIDs[localIndex]); }
        void SetNodeGroupName(uint32 localIndex, const char* groupName)             { mGroupNameIDs[localIndex] = MCore::GetStringIDGenerator().GenerateIDForString(groupName); }
        void AddNodeGroupName(const char* groupName)                                { mGroupNameIDs.Add(MCore::GetStringIDGenerator().GenerateIDForString(groupName)); }
        void RemoveNodeGroupName(uint32 localIndex)                                 { mGroupNameIDs.Remove(localIndex); }
        uint32 FindNodeGroupNameIndex(const char* groupName) const;

        // nodes
        const char* GetNodeName(uint32 localIndex) const
        {
            if (localIndex != MCORE_INVALIDINDEX32)
            {
                return MCore::GetStringIDGenerator().GetName(mNodeNameIDs[localIndex]).AsChar();
            }
            else
            {
                return "";
            }
        }
        uint32 GetNodeNameID(uint32 localIndex) const
        {
            if (localIndex != MCORE_INVALIDINDEX32)
            {
                return mNodeNameIDs[localIndex];
            }
            else
            {
                return MCORE_INVALIDINDEX32;
            }
        }
        const MCore::String& GetNodeNameString(uint32 localIndex) const             { return MCore::GetStringIDGenerator().GetName(mNodeNameIDs[localIndex]); }
        void SetNodeName(uint32 localIndex, const char* nodeName)                   { mNodeNameIDs[localIndex] = MCore::GetStringIDGenerator().GenerateIDForString(nodeName); }
        void AddNodeName(const char* nodeName)                                      { mNodeNameIDs.Add(MCore::GetStringIDGenerator().GenerateIDForString(nodeName)); }
        void RemoveNodeName(uint32 localIndex)                                      { mNodeNameIDs.Remove(localIndex); }
        uint32 FindNodeNameIndex(const char* nodeName) const;

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                                    { return Create(mGroupNameIDs, mNodeNameIDs); }
        MCore::Attribute* CreateInstance(void* destMemory) override                 { return new(destMemory) AttributeStateFilterLocal(); }
        const char* GetTypeString() const override                                  { return "StateFilterLocal"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributeStateFilterLocal* cast = static_cast<const AttributeStateFilterLocal*>(other);
            mGroupNameIDs = cast->mGroupNameIDs;
            mNodeNameIDs = cast->mNodeNameIDs;
            return true;
        }

        bool InitFromString(const MCore::String& valueString) override;
        bool ConvertToString(MCore::String& outString) const override;
        uint32 GetClassSize() const override                                        { return sizeof(AttributeStateFilterLocal); }
        uint32 GetDefaultInterfaceType() const override                             { return ATTRIBUTE_INTERFACETYPE_STATEFILTERLOCAL; }

    private:
        MCore::Array<uint32> mGroupNameIDs;
        MCore::Array<uint32> mNodeNameIDs;

        AttributeStateFilterLocal()
            : MCore::Attribute(TYPE_ID)     { mGroupNameIDs.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_ATTRIBUTES); mNodeNameIDs.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_ATTRIBUTES); }
        AttributeStateFilterLocal(const MCore::Array<uint32>& groupNameIDs, const MCore::Array<uint32>& nodeNameIDs)
            : MCore::Attribute(TYPE_ID)     { mGroupNameIDs.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_ATTRIBUTES); mNodeNameIDs.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_ATTRIBUTES); mGroupNameIDs = groupNameIDs; mNodeNameIDs = nodeNameIDs; }
        ~AttributeStateFilterLocal() {}

        uint32 GetDataSize() const override;
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override;
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override;
    };

    /**
    * Attribute used to represent a motion included in a blend space.
    */
    class EMFX_API AttributeBlendSpaceMotion
        : public MCore::Attribute
    {
        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00003001
        };
        enum class TypeFlags : AZ::u8
        {
            None = 0,
            UserSetCoordinateX = 1 << 0,   // Flag set if the x coordinate is set by the user instead of being auto computed.
            BlendSpace1D = 1 << 1,
            BlendSpace2D = 1 << 2,
            UserSetCoordinateY = 1 << 3,   // Flag set if the y coordinate is set by the user instead of being auto computed.
            InvalidMotion = 1 << 4    // Flag set when the motion is invalid
        };

        static AttributeBlendSpaceMotion* Create();
        static AttributeBlendSpaceMotion* Create(const AZStd::string& motionId);
        static AttributeBlendSpaceMotion* Create(const AZStd::string& motionId, const AZ::Vector2& coordinates, TypeFlags typeFlags = TypeFlags::None);

        void Set(const AZStd::string& motionId, const AZ::Vector2& coordinates, TypeFlags typeFlags = TypeFlags::None);
        AZ_INLINE const AZStd::string& GetMotionId() const                  { return m_motionId; }

        AZ_INLINE const AZ::Vector2& GetCoordinates() const                 { return m_coordinates; }

        AZ_INLINE float GetXCoordinate() const                              { return m_coordinates.GetX(); }
        AZ_INLINE float GetYCoordinate() const                              { return m_coordinates.GetY(); }

        AZ_INLINE void SetXCoordinate(float x)                              { m_coordinates.SetX(x); }
        AZ_INLINE void SetYCoordinate(float y)                              { m_coordinates.SetY(y); }

        bool IsXCoordinateSetByUser() const;
        bool IsYCoordinateSetByUser() const;

        void MarkXCoordinateSetByUser(bool setByUser);
        void MarkYCoordinateSetByUser(bool setByUser);

        int GetDimension() const;
        void SetDimension(int dimension);

        // MCore::Attribute overrides
        MCore::Attribute*   Clone() const override { return Create(m_motionId, m_coordinates, m_TypeFlags); }
        MCore::Attribute*   CreateInstance(void* destMemory) override { return new(destMemory) AttributeBlendSpaceMotion(); }
        const char*         GetTypeString() const override { return "BlendSpaceMotion"; }
        uint8               GetStreamWriteVersion() const override { return 1; }
        uint32              GetDefaultInterfaceType() const override { return ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONS; }
        bool                InitFrom(const MCore::Attribute* other) override;

        bool                InitFromString(const MCore::String& valueString) override;
        bool                ConvertToString(MCore::String& outString) const override;
        uint32              GetClassSize() const override { return sizeof(AttributeBlendSpaceMotion); }
        uint32              GetDataSize() const override;

        AZ_INLINE void SetFlag(TypeFlags flag);
        AZ_INLINE void UnsetFlag(TypeFlags flag);
        AZ_INLINE bool TestFlag(TypeFlags flag) const;

    protected:
        // MCore::Attribute overrides
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override;
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override;


    private:
        AttributeBlendSpaceMotion();
        AttributeBlendSpaceMotion(const AZStd::string& motionName, const AZ::Vector2& coordinates, TypeFlags typeFlags);
        ~AttributeBlendSpaceMotion() {}

    private:
        AZStd::string   m_motionId;
        AZ::Vector2     m_coordinates; // Coordinates of the motion in blend space
        TypeFlags       m_TypeFlags;
    };

    AZ_INLINE AttributeBlendSpaceMotion::TypeFlags operator~(AttributeBlendSpaceMotion::TypeFlags a) { return static_cast<AttributeBlendSpaceMotion::TypeFlags>(~static_cast<AZ::u8>(a)); }
    AZ_INLINE AttributeBlendSpaceMotion::TypeFlags operator| (AttributeBlendSpaceMotion::TypeFlags a, AttributeBlendSpaceMotion::TypeFlags b) { return static_cast<AttributeBlendSpaceMotion::TypeFlags>(static_cast<AZ::u8>(a) | static_cast<AZ::u8>(b)); }
    AZ_INLINE AttributeBlendSpaceMotion::TypeFlags operator& (AttributeBlendSpaceMotion::TypeFlags a, AttributeBlendSpaceMotion::TypeFlags b) { return static_cast<AttributeBlendSpaceMotion::TypeFlags>(static_cast<AZ::u8>(a) & static_cast<AZ::u8>(b)); }
    AZ_INLINE AttributeBlendSpaceMotion::TypeFlags operator^ (AttributeBlendSpaceMotion::TypeFlags a, AttributeBlendSpaceMotion::TypeFlags b) { return static_cast<AttributeBlendSpaceMotion::TypeFlags>(static_cast<AZ::u8>(a) ^ static_cast<AZ::u8>(b)); }
    AZ_INLINE AttributeBlendSpaceMotion::TypeFlags& operator|= (AttributeBlendSpaceMotion::TypeFlags& a, AttributeBlendSpaceMotion::TypeFlags b) { (AZ::u8&)(a) |= static_cast<AZ::u8>(b); return a; }
    AZ_INLINE AttributeBlendSpaceMotion::TypeFlags& operator&= (AttributeBlendSpaceMotion::TypeFlags& a, AttributeBlendSpaceMotion::TypeFlags b) { (AZ::u8&)(a) &= static_cast<AZ::u8>(b); return a; }
    AZ_INLINE AttributeBlendSpaceMotion::TypeFlags& operator^= (AttributeBlendSpaceMotion::TypeFlags& a, AttributeBlendSpaceMotion::TypeFlags b) { (AZ::u8&)(a) ^= static_cast<AZ::u8>(b); return a; }

    void AttributeBlendSpaceMotion::SetFlag(TypeFlags flag) { m_TypeFlags |= flag; }
    void AttributeBlendSpaceMotion::UnsetFlag(TypeFlags flag) { m_TypeFlags &= ~flag; }
    bool AttributeBlendSpaceMotion::TestFlag(TypeFlags flag) const { return (m_TypeFlags & flag) == flag; }

} // namespace EMotionFX