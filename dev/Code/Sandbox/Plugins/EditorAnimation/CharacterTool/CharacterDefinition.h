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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <Cry_Math.h>
#include <ICryAnimation.h>
#include <IAttachment.h>
#include <vector>

namespace Serialization {
    class IArchive;
}

struct ICharacterInstance;

namespace CharacterTool
{
    using std::vector;

    struct CharacterDefinition;

    struct CharacterAttachment
    {
        enum TransformSpace
        {
            SPACE_CHARACTER,
            SPACE_JOINT
        };

        enum ProxyPurpose
        {
            AUXILIARY,
            CLOTH,
            RAGDOLL
        };

        AttachmentTypes m_attachmentType;
        string m_strSocketName;
        TransformSpace m_positionSpace;
        TransformSpace m_rotationSpace;
        QuatT m_characterSpacePosition;
        QuatT m_jointSpacePosition;
        string m_strJointName;
        string m_strGeometryFilepath;
        string m_strMaterial;
        Vec4   m_ProxyParams;
        ProxyPurpose m_ProxyPurpose;
        SimulationParams m_simulationParams;

        string m_strRowJointName;
        RowSimulationParams m_rowSimulationParams;

        SVClothParams m_vclothParams;
        float m_viewDistanceMultiplier;
        int m_nFlags;
        CharacterDefinition* m_definition;

        CharacterAttachment()
            : m_attachmentType(CA_BONE)
            , m_positionSpace(SPACE_JOINT)
            , m_rotationSpace(SPACE_JOINT)
            , m_characterSpacePosition(IDENTITY)
            , m_jointSpacePosition(IDENTITY)
            , m_ProxyParams(0, 0, 0, 0)
            , m_ProxyPurpose(AUXILIARY)
            , m_viewDistanceMultiplier(1.0f)
            , m_nFlags(0)
            , m_definition()
        {
        }

        void Serialize(Serialization::IArchive& ar);
    };

    struct ICryAnimation;

    struct CharacterDefinition
    {
        bool m_initialized;
        string skeleton;
        string materialPath;
        string physics;
        string rig;
        vector<CharacterAttachment> attachments;
        DynArray<string> m_arrAllProcFunctions; //all procedural functions

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
        IAnimationSerializablePtr modifiers;
#endif

        CharacterDefinition()
        {
            m_initialized = false;
        }

        bool LoadFromXml(const XmlNodeRef& root);
        bool LoadFromXmlFile(const char* filename);
        void ApplyToCharacter(bool* skinSetChanged, ICharacterInstance* character, ICharacterManager* cryAnimation, bool showDebug);
        void ApplyBoneAttachment(IAttachment* pIAttachment, ICharacterManager* characterManager, const CharacterAttachment& desc, ICharacterInstance* pICharacterInstance, bool showDebug) const;
        void ApplyFaceAttachment(IAttachment* pIAttachment, ICharacterManager* characterManager, const CharacterAttachment& desc, bool showDebug) const;
        void ApplySkinAttachment(IAttachment* pIAttachment, ICharacterManager* characterManager, const CharacterAttachment& desc, ICharacterInstance* pICharacterInstance, bool* skinChanged) const;

        void SynchModifiers(ICharacterInstance& character);

        string GenerateUniqueAttachmentName(const char* referenceName) const;

        void Serialize(Serialization::IArchive& ar);
        bool Save(const char* filename);
        XmlNodeRef SaveToXml();
        static void ExportBoneAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments);
        static void ExportFaceAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments);
        static void ExportSkinAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments);
        static void ExportProxyAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments);
        static void ExportPClothAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments);
        static void ExportVClothAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments);
        static void ExportSimulation(const CharacterAttachment& attach, XmlNodeRef nodeAttach);

        bool SaveToMemory(vector<char>* buffer);
    };
}
