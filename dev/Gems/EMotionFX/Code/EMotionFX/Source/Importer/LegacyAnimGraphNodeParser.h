/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector2.h>
#include <MCore/Source/Endian.h>
#include "Importer.h"
#include "AnimGraphFileFormat.h"
#include "../AnimGraphObject.h"
#include <MCore/Source/File.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include "../AnimGraphNode.h"


namespace EMotionFX
{
    class AnimGraphNode;
    class AnimGraphStateMachine;
    class AnimGraphStateTransition;
    class AnimGraphTransitionCondition;

    const AZ::TypeId GetNewTypeIdByOldNodeTypeId(uint32 oldNodeTypeId);

    enum LegacyAttributeTypeId
    {
        ATTRIBUTE_FLOAT_TYPE_ID = 0x00000001,
        ATTRIBUTE_INT32_TYPE_ID = 0x00000002,
        ATTRIBUTE_BOOL_TYPE_ID = 0x00000004
    };

    enum LegacyERotationOrder
    {
        ROTATIONORDER_ZYX = 0,
        ROTATIONORDER_ZXY = 1,
        ROTATIONORDER_YZX = 2,
        ROTATIONORDER_YXZ = 3,
        ROTATIONORDER_XYZ = 4,
        ROTATIONORDER_XZY = 5
    };


    class LegacyAttributeHeader
    {
    public:
        LegacyAttributeHeader() { }

        AZ::u32 GetAttributeType() const
        {
            return m_attribType;
        }

        AZ::u32 GetAttributeSize() const
        {
            return m_attributeSize;
        }

        static bool Parse(MCore::File* stream, MCore::Endian::EEndianType endianType, LegacyAttributeHeader& attributeHeader);
    private:
        LegacyAttributeHeader(const LegacyAttributeHeader& src) :
            m_attribType(src.m_attribType)
            , m_attributeSize(src.m_attributeSize)
            , m_name(src.m_name)
        { }

        AZ::u32 m_attribType;
        AZ::u32 m_attributeSize;
        AZStd::string m_name;
    };

    template <class T>
    class LegacyAttribute
    {
    public:
        bool Parse(MCore::File* stream, MCore::Endian::EEndianType endianType);
        const T& GetValue() const;
    private:
        T m_value;
    };

    class LegacyAnimGraphNodeParser
    {
    public:
        static bool ParseAnimGraphNodeChunk(MCore::File* file,
            Importer::ImportParameters& importParams,
            const char* nodeName,
            FileFormat::AnimGraph_NodeHeader& nodeHeader,
            AnimGraphNode*& node);

        static bool ParseTransitionConditionChunk(MCore::File* file,
            Importer::ImportParameters& importParams,
            const FileFormat::AnimGraph_NodeHeader& nodeHeader,
            AnimGraphTransitionCondition*& transitionCondition);

        template <class T>
        static bool ParseLegacyAttributes(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);

        static bool Forward(MCore::File* stream, size_t numBytes);

    private:
        static bool InitializeNodeGeneralData(const char* nodeName, Importer::ImportParameters& importParams, FileFormat::AnimGraph_NodeHeader& nodeHeader, AnimGraphNode* node);
        static bool GetBlendSpaceNodeEvaluatorTypeId(uint32 legacyIndex, AZ::TypeId& value);
        static bool ConvertFloatAttributeValueToBool(float value);
        static bool TryGetFloatFromAttribute(MCore::File* stream, MCore::Endian::EEndianType endianType, const LegacyAttributeHeader& attributeHeader, float& outputValue);

        template <class T>
        static bool ParseAnimGraphNode(MCore::File* file,
            Importer::ImportParameters& importParams,
            const char* nodeName,
            FileFormat::AnimGraph_NodeHeader& nodeHeader,
            AnimGraphNode*& node)
        {
            node = aznew T();
            node->SetAnimGraph(importParams.mAnimGraph);
            if (!InitializeNodeGeneralData(nodeName, importParams, nodeHeader, node))
            {
                MCore::LogError("Error on initializing node general data");
                return false;
            }
            if (!ParseLegacyAttributes<T>(file, nodeHeader.mNumAttributes, importParams.mEndianType, importParams, *node))
            {
                MCore::LogError("Unable to parse node legacy attributes");
                return false;
            }
            return true;
        }

        template <class T>
        static bool ParseAnimGraphTransitionCondition(MCore::File* file,
            Importer::ImportParameters& importParams,
            const FileFormat::AnimGraph_NodeHeader& header,
            AnimGraphTransitionCondition*& transitionCondition)
        {
            transitionCondition = aznew T();
            return ParseLegacyAttributes<T>(file, header.mNumAttributes, importParams.mEndianType, importParams, *transitionCondition);
        }
    };

    template<class T, template<class> class LegacyAttribute>
    class LegacyAttributeArray
    {
    public:
        bool Parse(MCore::File* stream, MCore::Endian::EEndianType endianType);
        const AZStd::vector< LegacyAttribute<T> >& GetValue() const;
    private:
        bool PopulateAttributeDynamicArray(MCore::File* stream, MCore::Endian::EEndianType endianType);

        AZStd::vector< LegacyAttribute<T> > m_attributes;
        // Used when reading version 2 attribute array
        uint32 m_elementTypeId;
    };

    class LegacyAttributeSettingsParser
    {
    public:
        static bool Parse(MCore::File* stream, MCore::Endian::EEndianType endianType);
    };

} // Namespace EMotionFX
