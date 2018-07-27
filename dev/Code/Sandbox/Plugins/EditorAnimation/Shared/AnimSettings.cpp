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

#include "stdafx.h"
#include "AnimSettings.h"
#include "Serialization.h"
#include "Serialization/JSONIArchive.h"
#include "Serialization/JSONOArchive.h"
#include <CryAssert.h>
#include <IXml.h>
#include <ISystem.h>
#include <IConsole.h>
#include <ICryAnimation.h>
#include <CryPath.h>

#ifdef RESOURCE_COMPILER
# include "../../../Tools/CryXML/ICryXML.h"
# include "../../../Tools/CryXML/IXMLSerializer.h"
# include "../../../Tools/CryCommonTools/PakXmlFileBufferSource.h"
#endif

#if defined(AZ_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

using std::vector;
using std::pair;

// ---------------------------------------------------------------------------

namespace
{
    template< typename T >
    bool ReadValueFromXmlChildNode(XmlNodeRef parentNode, const char* childNodeName, T& valueOut)
    {
        assert(parentNode);
        assert(childNodeName);

        XmlNodeRef childNode = parentNode->findChild(childNodeName);
        if (!childNode)
        {
            return false;
        }

        return childNode->getAttr("value", valueOut);
    }


    bool ReadValueFromXmlChildNode(XmlNodeRef parentNode, const char* childNodeName, string& valueOut)
    {
        assert(parentNode);
        assert(childNodeName);

        XmlNodeRef childNode = parentNode->findChild(childNodeName);
        if (!childNode)
        {
            return false;
        }

        const char* str = "";
        if (!childNode->getAttr("value", &str))
        {
            return false;
        }

        valueOut = str;
        return true;
    }
}

string SAnimSettings::GetAnimSettingsFilename(const char* animationPath)
{
    return PathUtil::ReplaceExtension(animationPath, "animsettings");
}

string SAnimSettings::GetIntermediateFilename(const char* animationPath)
{
    const string gameFolderPath = PathUtil::AddSlash(gEnv->pFileIO->GetAlias("@devassets@"));
    const string animationFilePath = gameFolderPath + animationPath;

    const string animSettingsFilename = PathUtil::ReplaceExtension(animationFilePath.c_str(), "i_caf");
    return animSettingsFilename;
}

bool SAnimSettings::SaveUsingAssetPath(const char* assetPath) const
{
    string outputFilename = PathUtil::AddSlash(gEnv->pFileIO->GetAlias("@devassets@")) + assetPath;
    return SaveUsingFullPath(outputFilename.c_str());
}

bool SAnimSettings::SaveUsingFullPath(const char* fullPath) const
{
    Serialization::JSONOArchive oa;
    oa(*this);
    return oa.save(fullPath);
}

static string TrimString(const char* start, const char* end)
{
    while (start != end && (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n'))
    {
        ++start;
    }
    while (end != start && (*(end - 1) == ' ' || *(end - 1) == '\t' || *(end - 1) == '\r' || *(end - 1) == '\n'))
    {
        --end;
    }
    return string(start, end);
}

void SAnimSettings::SplitTagString(vector<string>* tags, const char* str)
{
    if (!tags)
    {
        return;
    }
    if (!str)
    {
        return;
    }

    tags->clear();
    if (str[0] == '\0')
    {
        return;
    }

    const char* tag_start = str;
    for (const char* p = str; *p != '\0'; ++p)
    {
        if (*p == ',')
        {
            tags->push_back(TrimString(tag_start, p));
            tag_start = p + 1;
        }
    }
    tags->push_back(TrimString(tag_start, str + strlen(str)));
}

static bool DetectXML(const char* text, size_t size)
{
    const char* end = text + size;
    while (text != end && (*text == ' ' || *text == '\r' || *text == '\n' || *text == '\t'))
    {
        ++text;
    }
    return text != end && *text == '<';
}

static bool LoadFile(vector<char>* buffer, const char* filename, IPakSystem* pakSystem)
{
#ifdef RESOURCE_COMPILER
    PakSystemFile* f = pakSystem->Open(filename, "rb");
    if (!f)
    {
        return false;
    }

    size_t size = pakSystem->GetLength(f);
    buffer->resize(size);

    //returning true allows the calling code to handle the empty buffer correctly and 
    //accurately tracks that the file is truly there and accessible.
    if (size == 0)
    {
        return true;
    }
    bool result = pakSystem->Read(f, &(*buffer)[0], size) == size;
    pakSystem->Close(f);
#else
    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(filename, "rb");
    if (!fileHandle)
    {
        return false;
    }

    gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_END);
    size_t size = gEnv->pCryPak->FTell(fileHandle);
    gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_SET);

    buffer->resize(size);
    bool result = gEnv->pCryPak->FRead(&(*buffer)[0], size, fileHandle) == size;
    gEnv->pCryPak->FClose(fileHandle);
#endif
    return result;
}

bool SAnimSettings::Load(const char* filename, const vector<string>& jointNames, ICryXML* cryXml, IPakSystem* pakSystem)
{
    vector<char> content;
    if (!LoadFile(&content, filename, pakSystem))
    {
        return false;
    }

    if (content.empty())
    {
        return true;
    }

    if (DetectXML(&content[0], content.size()))
    {
        return LoadXMLFromMemory(&content[0], content.size(), jointNames, cryXml);
    }
    else
    {
        Serialization::JSONIArchive ia;
        if (!ia.open(&content[0], content.size()))
        {
            return false;
        }

        ia(*this);
        return true;
    }
}


bool SAnimSettings::LoadXMLFromMemory(const char* data, size_t length, const vector<string>& jointNames, ICryXML* cryXml)
{
#ifdef RESOURCE_COMPILER
    char error[1024];
    XmlNodeRef xmlRoot = cryXml->GetXMLSerializer()->Read(PakXmlBufferSource(data, length), true, sizeof(error), error);
#else
    XmlNodeRef xmlRoot = GetISystem()->LoadXmlFromBuffer(data, length);
#endif

    if (!xmlRoot)
    {
        return false;
    }

    build = SAnimationBuildSettings();
    SCompressionSettings& compression = build.compression;

    ReadValueFromXmlChildNode(xmlRoot, "AdditiveAnimation", build.additive);
    ReadValueFromXmlChildNode(xmlRoot, "Skeleton", build.skeletonAlias);

    XmlNodeRef xmlCompressionSettings = xmlRoot->findChild("CompressionSettings");
    if (xmlCompressionSettings)
    {
        int version = 0;
        if (ReadValueFromXmlChildNode(xmlCompressionSettings, "Version", version))
        {
            if (version == 0)
            {
                ReadValueFromXmlChildNode(xmlCompressionSettings, "Compression", compression.m_compressionValue);
                ReadValueFromXmlChildNode(xmlCompressionSettings, "RotEpsilon", compression.m_rotationEpsilon);
                ReadValueFromXmlChildNode(xmlCompressionSettings, "PosEpsilon", compression.m_positionEpsilon);
            }
        }

        const SControllerCompressionSettings defaultBoneSettings;

        XmlNodeRef xmlPerBoneCompression = xmlCompressionSettings->findChild("PerBoneCompression");
        if (xmlPerBoneCompression)
        {
            vector<pair<string, SControllerCompressionSettings> > obsoleteEntries;

            const int boneSettingsCount = xmlPerBoneCompression->getChildCount();
            for (int i = 0; i < boneSettingsCount; ++i)
            {
                XmlNodeRef xmlBone = xmlPerBoneCompression->getChild(i);

                SControllerCompressionSettings boneSettings;


                int forceDelete = 0;
                xmlBone->getAttr("Delete", forceDelete);
                if (forceDelete == 1)
                {
                    boneSettings.state = eCES_ForceDelete;
                }

                xmlBone->getAttr("Multiply", boneSettings.multiply);
                const char* boneName = xmlBone->getAttr("Name");
                if (boneName[0] == '\0')
                {
                    const char* nameContains = xmlBone->getAttr("NameContains");
                    if (nameContains[0] != '\0')
                    {
                        obsoleteEntries.push_back(std::make_pair(string(nameContains), boneSettings));
                    }
                    continue;
                }

                if (boneSettings != defaultBoneSettings)
                {
                    compression.SetControllerCompressionSettings(boneName, boneSettings);
                }
            }

            // As NameContains uses substring matching we replace it with Name (full
            // name) specification, but we still to convert old entries with 'NameContains'
            // we do this by expanding NameContains to full controller names.
            if (!obsoleteEntries.empty())
            {
                if (jointNames.empty())
                {
                    compression.m_usesNameContainsInPerBoneSettings = true;
                }
                else
                {
                    for (size_t i = 0; i < jointNames.size(); ++i)
                    {
                        for (size_t j = 0; j < obsoleteEntries.size(); ++j)
                        {
                            string name = jointNames[i].c_str();
                            name.MakeLower();
                            string nameContains = obsoleteEntries[j].first.c_str();
                            nameContains.MakeLower();
                            const SControllerCompressionSettings& boneSettings = obsoleteEntries[j].second;
                            if (strstr(name, nameContains) != 0)
                            {
                                if (boneSettings != defaultBoneSettings)
                                {
                                    compression.SetControllerCompressionSettings(jointNames[i].c_str(), boneSettings);
                                }
                                break;
                            }
                        }
                    }
                }
                // *pConvertedOut = true;
            }
        }
    }
    else
    {
        compression.m_useNewFormatWithDefaultSettings = true;
    }


    build.tags.clear();
    XmlNodeRef xmlTags = xmlRoot->findChild("Tags");
    if (xmlTags)
    {
        string tagString = xmlTags->getContent();
        SplitTagString(&build.tags, tagString);
    }

    return true;
}

// ---------------------------------------------------------------------------

void SAnimationBuildSettings::Serialize(Serialization::IArchive& ar)
{
    ar(additive, "additive", "Additive");
    ar(SkeletonAlias(skeletonAlias), "skeletonAlias", "Skeleton Alias");
    ar(compression, "compression", "Compression");
    ar(tags, "tags", "Tags");
}

void SAnimSettings::Serialize(Serialization::IArchive& ar)
{
    ar(build, "build", "Build");
    ar(production, "production", "Production");
    ar(runtime, "runtime", "Runtime");
}

// ---------------------------------------------------------------------------
SCompressionSettings::SCompressionSettings()
    : m_useNewFormatWithDefaultSettings(false)
    , m_usesNameContainsInPerBoneSettings(false)
{
    m_positionEpsilon = 0.001f;
    m_rotationEpsilon = 0.00001f;
    m_compressionValue = 10;

    m_controllerCompressionSettings.clear();
}

void SCompressionSettings::InitializeForCharacter(ICharacterInstance* pCharacter)
{
    *this = SCompressionSettings();

    if (pCharacter == NULL)
    {
        return;
    }

    ISkeletonPose& skeletonPose = *pCharacter->GetISkeletonPose();
    IDefaultSkeleton& rIDefaultSkeleton = pCharacter->GetIDefaultSkeleton();

    const uint32 jointCount = rIDefaultSkeleton.GetJointCount();
    for (uint32 jointId = 0; jointId < jointCount; ++jointId)
    {
        const char* jointName = rIDefaultSkeleton.GetJointNameByID(jointId);
        m_controllerCompressionSettings.push_back(std::make_pair(jointName, SControllerCompressionSettings()));
    }
}

void SCompressionSettings::SetZeroCompression()
{
    m_positionEpsilon = 0.0f;
    m_rotationEpsilon = 0.0f;
    m_compressionValue = 0;
    m_controllerCompressionSettings.clear();
}


void SCompressionSettings::SetControllerCompressionSettings(const char* controllerName, const SControllerCompressionSettings& settings)
{
    assert(controllerName);
    if (!controllerName)
    {
        return;
    }

    for (size_t i = 0; i < m_controllerCompressionSettings.size(); ++i)
    {
        if (azstricmp(m_controllerCompressionSettings[i].first.c_str(), controllerName) == 0)
        {
            m_controllerCompressionSettings[i].second = settings;
            return;
        }
    }

    m_controllerCompressionSettings.push_back(std::make_pair(controllerName, settings));
}

static SControllerCompressionSettings* FindSettings(SCompressionSettings::TControllerSettings& settings, const char* controllerName)
{
    for (size_t i = 0; i < settings.size(); ++i)
    {
        if (azstricmp(settings[i].first.c_str(), controllerName) == 0)
        {
            return &settings[i].second;
        }
    }
    return 0;
}

static const SControllerCompressionSettings* FindSettings(const SCompressionSettings::TControllerSettings& settings, const char* controllerName)
{
    for (size_t i = 0; i < settings.size(); ++i)
    {
        if (azstricmp(settings[i].first.c_str(), controllerName) == 0)
        {
            return &settings[i].second;
        }
    }
    return 0;
}

const SControllerCompressionSettings& SCompressionSettings::GetControllerCompressionSettings(const char* controllerName) const
{
    assert(controllerName);

    if (controllerName)
    {
        if (const SControllerCompressionSettings* settings = FindSettings(m_controllerCompressionSettings, controllerName))
        {
            return *settings;
        }
    }

    static SControllerCompressionSettings s_defaultSettings;
    return s_defaultSettings;
}

void SCompressionSettings::GetControllerNames(std::vector< string >& controllerNamesOut) const
{
    controllerNamesOut.clear();

    for (size_t i = 0; i < m_controllerCompressionSettings.size(); ++i)
    {
        controllerNamesOut.push_back(m_controllerCompressionSettings[i].first);
    }
}


void SControllerCompressionSettings::Serialize(Serialization::IArchive& ar)
{
    bool deleteController = state == eCES_ForceDelete;
    ar(multiply, "multiply", deleteController ? 0 : ">60>^Multiply");
    ar(deleteController, "delete", "^Delete");
    if (ar.IsInput())
    {
        state = deleteController ? eCES_ForceDelete : eCES_UseEpsilons;
    }
}


struct SPerBoneSettingsTree
{
    struct SNode
    {
        const char* name;
        bool isRoot;
        vector<std::unique_ptr<SNode> > children;
        SControllerCompressionSettings settings;

        SNode()
            : name("")
            , isRoot(false) {}

        void Serialize(Serialization::IArchive& ar)
        {
            bool deleteController = settings.state == eCES_ForceDelete;
            ar(deleteController, "delete", deleteController ? "^^Delete" : "^^");
            if (ar.IsInput())
            {
                settings.state = deleteController ? eCES_ForceDelete : eCES_UseEpsilons;
            }
            ar(Serialization::Range(settings.multiply, 0.0f, 10.0f, 0.0f, 1000.f), "multiply", deleteController ? 0 : "^>40> *");

            for (size_t i = 0; i < children.size(); ++i)
            {
                ar(*children[i], children[i]->name, children[i]->name);
            }
        }
    };

    SCompressionSettings::TControllerSettings* controllerMap;
    IDefaultSkeleton* skeleton;

    SPerBoneSettingsTree(SCompressionSettings::TControllerSettings* controllerMap, IDefaultSkeleton* skeleton)
        : controllerMap(controllerMap)
        , skeleton(skeleton)
    {
    }

    void Serialize(Serialization::IArchive& ar)
    {
        SNode root;
        root.name = skeleton->GetJointNameByID(0);
        SControllerCompressionSettings* rootSettings = FindSettings(*controllerMap, root.name);
        if (rootSettings)
        {
            root.settings = *rootSettings;
        }
        root.isRoot = true;

        vector<SNode*> allJoints;
        int numJoints = skeleton->GetJointCount();
        allJoints.resize(numJoints);
        allJoints[0] = &root;
        for (int i = 1; i < numJoints; ++i)
        {
            allJoints[i] = new SNode();
            allJoints[i]->name = skeleton->GetJointNameByID(i);
            SControllerCompressionSettings* controller = FindSettings(*controllerMap, allJoints[i]->name);
            if (controller)
            {
                allJoints[i]->settings = *controller;
            }
        }

        for (int i = 0; i < numJoints; ++i)
        {
            int parentId = skeleton->GetJointParentIDByID(i);
            if (parentId >= 0)
            {
                allJoints[parentId]->children.push_back(std::unique_ptr<SNode>(allJoints[i]));
            }
        }

        ar(root, "root", root.name);

        if (ar.IsInput())
        {
            controllerMap->clear();
            for (int i = 0; i < numJoints; ++i)
            {
                if (allJoints[i]->settings != SControllerCompressionSettings())
                {
                    SControllerCompressionSettings* controller = FindSettings(*controllerMap, allJoints[i]->name);
                    if (controller)
                    {
                        * controller = allJoints[i]->settings;
                    }
                    else
                    {
                        controllerMap->push_back(std::make_pair(allJoints[i]->name, allJoints[i]->settings));
                    }
                }
            }
        }
    }
};


struct SEditableJointSettings
    : std::pair<string, SControllerCompressionSettings>
{
    void Serialize(Serialization::IArchive& ar)
    {
        ar(JointName(first), "joint", "^");
        ar(second, "settings", "^");
    }
};

void SCompressionSettings::Serialize(Serialization::IArchive& ar)
{
    using Serialization::Range;
    ar(Range(m_compressionValue, 0, 1000000), "compressionValue", "Compression Value");
    if (ar.OpenBlock("removalThreshold", "+Controller Removal Threshold"))
    {
        ar(Range(m_positionEpsilon, 0.0f, 0.2f), "positionEpsilon", "Position Epsilon");
        ar(Range(m_rotationEpsilon, 0.0f, 3.1415926f * 0.25f), "rotationEpsilon", "Rotation Epsilon");
        ar.CloseBlock();
    }


    if (ar.IsEdit())
    {
        IDefaultSkeleton* skeleton = ar.FindContext<IDefaultSkeleton>();
        if (skeleton && ar.GetFilter() && ar.Filter(SERIALIZE_COMPRESSION_SETTINGS_AS_TREE))
        {
            SPerBoneSettingsTree tree(&m_controllerCompressionSettings, skeleton);
            ar(tree, "perJointSettings", "Per-Joint Settings");
        }
        else
        {
            ar((vector<SEditableJointSettings>&)m_controllerCompressionSettings, "perJointSettings", "Per-Joint Settings");
        }
    }
    else
    {
        ar(m_controllerCompressionSettings, "perJointSettings");
    }
}
