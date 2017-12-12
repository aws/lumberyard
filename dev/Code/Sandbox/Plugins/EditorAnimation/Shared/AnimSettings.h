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

#include <string>
#include <vector>
#include <map>

#include <Serialization/BlackBox.h>
#include "Strings.h"

struct ICharacterInstance;
class ICryXML;
struct IPakSystem;

namespace Serialization
{
    class IArchive;
}

struct SkeletonAlias
{
    string& alias;

    SkeletonAlias(string& alias)
        : alias(alias) {}
};

using std::vector;

enum EControllerEnabledState
{
    eCES_ForceDelete,
    eCES_UseEpsilons,
};

enum
{
    SERIALIZE_COMPRESSION_SETTINGS_AS_TREE = 1 << 31
};

struct SControllerCompressionSettings
{
    EControllerEnabledState state;
    float multiply;

    SControllerCompressionSettings()
        : state(eCES_UseEpsilons)
        , multiply(1.f)
    {
    }

    bool operator== (const SControllerCompressionSettings& rhs) const
    {
        return state == rhs.state && fabsf(multiply - rhs.multiply) < 0.00001f;
    }

    bool operator!= (const SControllerCompressionSettings& rhs) const
    {
        return !(*this == rhs);
    }

    void Serialize(Serialization::IArchive& ar);
};


struct SCompressionSettings
{
    float m_positionEpsilon;
    float m_rotationEpsilon;
    int m_compressionValue;

    // This flag is needed to keep old RC behavior with missing
    // "CompressionSettings" xml node. In this case it uses default settings in
    // new format that has different representation.
    bool m_useNewFormatWithDefaultSettings;
    bool m_usesNameContainsInPerBoneSettings;

    typedef std::vector<std::pair<string, SControllerCompressionSettings> > TControllerSettings;
    TControllerSettings m_controllerCompressionSettings;

    SCompressionSettings();

    void SetZeroCompression();
    void InitializeForCharacter(ICharacterInstance* pCharacter);
    void SetControllerCompressionSettings(const char* controllerName, const SControllerCompressionSettings& settings);
    const SControllerCompressionSettings& GetControllerCompressionSettings(const char* controllerName) const;

    void GetControllerNames(std::vector< string >& controllerNamesOut) const;

    void Serialize(Serialization::IArchive& ar);
};

struct SAnimationBuildSettings
{
    string skeletonAlias;
    bool additive;
    SCompressionSettings compression;
    std::vector<string> tags;

    SAnimationBuildSettings()
        : additive(false)
    {
    }

    void Serialize(Serialization::IArchive& ar);
};

struct SAnimSettings
{
    SAnimationBuildSettings build;
    Serialization::SBlackBox production;
    Serialization::SBlackBox runtime;

    void Serialize(Serialization::IArchive& ar);
    bool Load(const char* filename, const std::vector<string>& jointNames, ICryXML* cryXml, IPakSystem* pakSystem);
    bool LoadXMLFromMemory(const char* data, size_t length, const std::vector<string>& jointNames, ICryXML* cryXml);
    bool SaveUsingAssetPath(const char* assetPath) const;
    bool SaveUsingFullPath(const char* fullPath) const;

    static string GetAnimSettingsFilename(const char* animationPath);
    static string GetIntermediateFilename(const char* animationPath);
    static void SplitTagString(std::vector<string>* tags, const char* str);
};
