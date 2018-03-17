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

#include "AnimationInfoLoader.h"
#include "../../../CryXML/ICryXML.h"
#include "../../../CryXML/IXMLSerializer.h"

#include "PakXmlFileBufferSource.h"
#include "FileUtil.h"
#include "PathHelpers.h"
#include "StringHelpers.h"
#include "Util.h"
#include "Plugins/EditorAnimation/Shared/AnimSettings.h"

#include <cstring>     // strchr()


string SAnimationDefinition::s_OverrideAnimationSettingsFilename;


CAnimationInfoLoader::CAnimationInfoLoader(ICryXML* pXML)
    : m_pXML(pXML)
{
}

CAnimationInfoLoader::~CAnimationInfoLoader(void)
{
}


static bool GetAsBool(const string& val, bool defaultValue)
{
    if (!val.c_str())
    {
        return defaultValue;
    }

    if ((strcmp(val.c_str(), "1") == 0) || (_stricmp(val.c_str(), "yes") == 0))
    {
        return true;
    }

    return false;
}


static int GetNodeIDFromName(const char* name, const XmlNodeRef& node)
{
    uint32 numChildren = node->getChildCount();

    for (uint32 child = 0; child < numChildren; ++child)
    {
        if (_stricmp(name, node->getChild(child)->getTag()) == 0)
        {
            return child;
        }
    }

    return -1;
}


bool CAnimationInfoLoader::LoadDescription(const string& name, IPakSystem* pPakSystem)
{
    // fill structures
    char error[4096];

    //Move to use pak file system or file: XmlNodeRef root =  m_pXML->GetXMLSerializer()->Read(FileXmlBufferSource(name.c_str()), sizeof(error), error);
    const bool bRemoveNonessentialSpacesFromContent = true;
    XmlNodeRef root = m_pXML->GetXMLSerializer()->Read(PakXmlFileBufferSource(pPakSystem, name.c_str()), bRemoveNonessentialSpacesFromContent, sizeof(error), error);
    if (!root)
    {
        RCLogError("AnimationInfoLoader: Cannot read file \"%s\": %s\n", name.c_str(), error);
        return false;
    }

    string UnifiedPath(const string&path);
    string RelativePath(const string&path, const string&relativeToPath);

    const string cbaFilePath = name;
    const string cbaFolderPath = UnifiedPath(PathHelpers::GetDirectory(PathHelpers::GetAbsoluteAsciiPath(cbaFilePath)));
    const string animationsRootFolderPath = UnifiedPath(PathHelpers::GetDirectory(cbaFolderPath));

    const uint32 numDefinitions = root->getChildCount();
    for (uint32 nDefinitionNode = 0; nDefinitionNode < numDefinitions; ++nDefinitionNode)
    {
        XmlNodeRef pRoot = root->getChild(nDefinitionNode);
        if (!pRoot)
        {
            RCLogError("AnimationInfoLoader: Node not found");
            continue;
        }

        if (_stricmp(pRoot->getTag(), "preset") == 0)
        {
            continue;
        }

        if (_stricmp(pRoot->getTag(), "Platform") == 0)
        {
            SPlatformAnimationSetup platform;

            const char* pName = pRoot->getAttr("Name");
            if (pName && pName[0])
            {
                platform.m_platform = pName;
            }

            const char* pMultiplier = pRoot->getAttr("CompressionMultiplier");
            if (pMultiplier && pMultiplier[0])
            {
                platform.m_compressionMultiplier = float(atof(pMultiplier));
            }

            if (!platform.m_platform.empty())
            {
                m_platformSetups.push_back(platform);
            }
            continue;
        }

        SAnimationDefinition def;
        def.m_MainDesc.m_bNewFormat = false;

        {
            uint32 model = GetNodeIDFromName("model", pRoot);
            if (model == -1)
            {
                RCLog("Model name not found in \"%s\" node", pRoot->getTag());
            }
            else
            {
                def.m_Model = pRoot->getChild(model)->getAttr("File");
                if (def.m_Model.empty())
                {
                    RCLogError("AnimationInfoLoader:: Model filename not found in \"%s\" node", pRoot->getTag());
                }
                else
                {
                    bool bAutofixed = false;

                    while (!PathHelpers::IsRelative(def.m_Model))
                    {
                        RCLogWarning("AnimationInfoLoader: Model filename \"%s\" in \"%s\" node is not relative", def.m_Model.c_str(), pRoot->getTag());

                        const size_t pos = def.m_Model.find_first_of("/\\:");
                        if (pos == string::npos)
                        {
                            assert(0);
                            RCLogError("AnimationInfoLoader: Unexpected failure: a bug in PathHelpers::IsRelative()");
                            def.m_Model.clear();
                            break;
                        }

                        def.m_Model = def.m_Model.substr(pos + 1, string::npos);
                        bAutofixed = true;

                        if (def.m_Model.empty())
                        {
                            RCLogError("AnimationInfoLoader: Failed to fix the model filename");
                            def.m_Model.clear();
                            break;
                        }
                    }

                    if (!def.m_Model.empty() && bAutofixed)
                    {
                        RCLogWarning("AnimationInfoLoader: Changed to \"%s\"", def.m_Model.c_str());
                    }
                }
            }
        }

        {
            uint32 path = GetNodeIDFromName("animation", pRoot);
            if (path == -1)
            {
                RCLog("AnimationInfoLoader: Path not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
            }
            else
            {
                const char* pChar = pRoot->getChild(path)->getAttr("Path");
                if (!pChar)
                {
                    RCLogError("AnimationInfoLoader: Path not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
                }

                const string animationFullPath = UnifiedPath(PathHelpers::Join(cbaFolderPath, PathHelpers::Join(pChar, "a.a")));
                const string animationPath = UnifiedPath(RelativePath(animationFullPath, animationsRootFolderPath));
                if (animationPath.empty())
                {
                    RCLogError("AnimationInfoLoader: Animation path \"%s\" is not under root animation folder \"%s\"", PathHelpers::GetDirectory(animationFullPath).c_str(), animationsRootFolderPath.c_str());
                }

                def.SetAnimationPath(string(pChar), PathHelpers::GetDirectory(animationPath));
            }
        }

        {
            uint32 path = GetNodeIDFromName("database", pRoot);
            if (path == -1)
            {
                // ReportInfo("Database not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
            }
            else
            {
                const char* pChar = pRoot->getChild(path)->getAttr("Path");
                if (!pChar)
                {
                    RCLogError("AnimationInfoLoader: Path not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
                }

                def.m_DBName = string(pChar);
            }
        }


        //int m_RootToHeels;
        {
            uint32 skipSaveToDB = GetNodeIDFromName("SkipSaveToDatabase", pRoot);
            if (skipSaveToDB == -1)
            {
                //ReportInfo("SkipSaveToDatabase value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
                const bool defaultSkipToDatabase = def.m_DBName.empty();
                def.m_MainDesc.m_bSkipSaveToDatabase = defaultSkipToDatabase;
            }
            else
            {
                string val = pRoot->getChild(skipSaveToDB)->getAttr("value");
                if (val.empty())
                {
                    RCLog("AnimationInfoLoader: SaveToDatabase value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
                }
                def.m_MainDesc.m_bSkipSaveToDatabase = GetAsBool(val, false);
            }
        }

        {
            int32 compression = GetNodeIDFromName("compression", pRoot);
            if (compression == -1 || !pRoot->getChild(compression)->getAttr("value", def.m_MainDesc.format.oldFmt.m_CompressionQuality))
            {
                RCLog("AnimationInfoLoader: Compression value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
            }
        }

        {
            uint32 ROTEpsilon = GetNodeIDFromName("RotEpsilon", pRoot);
            if (ROTEpsilon == -1)
            {
            }
            else
            {
                uint32 status = pRoot->getChild(ROTEpsilon)->getAttr("value", def.m_MainDesc.format.oldFmt.m_fROT_EPSILON);
                if (status == 0)
                {
                    RCLog("AnimationInfoLoader: ROT_EPSILON value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
                }
                ;
            }
        }
        {
            uint32 POSEpsilon = GetNodeIDFromName("PosEpsilon", pRoot);
            if (POSEpsilon == -1)
            {
            }
            else
            {
                uint32 status = pRoot->getChild(POSEpsilon)->getAttr("value", def.m_MainDesc.format.oldFmt.m_fPOS_EPSILON);
                if (status == 0)
                {
                    RCLog("AnimationInfoLoader: ROT_EPSILON value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
                }
                ;
            }
        }

        def.m_MainDesc.LoadBoneSettingsFromXML(pRoot);
        //-----------------------------------------------------------------------


        XmlNodeRef pAttrib;

        uint32 details = GetNodeIDFromName("SpecialAnimsList", pRoot);
        if (details != -1)
        {
            pAttrib = pRoot->getChild(details);
            uint32 numAnims = pAttrib->getChildCount();
            for (uint32 nAnims = 0; nAnims < numAnims; ++nAnims)
            {
                XmlNodeRef pAnim = pAttrib->getChild(nAnims);
                if (!pAnim)
                {
                    RCLog("AnimationInfoLoader: Node not found");
                    continue;
                }

                SAnimationDesc desc(def.m_MainDesc);
                desc.m_bNewFormat = false;

                desc.LoadBoneSettingsFromXML(pAnim);

                const char* pPath = pAnim->getAttr("APath");

                if (!pPath)
                {
                    RCLog("AnimationInfoLoader: Animation name not found in \"%s\" node \"subnode\" \"%s\"", pRoot->getTag(), pAnim->getTag(), def.m_Model.c_str());
                    continue;
                }
                desc.SetAnimationName(string(pPath));

                string val;
                val = pAnim->getAttr("SkipSaveToDatabase");
                if (!val.empty())
                {
                    desc.m_bSkipSaveToDatabase = GetAsBool(val, false);
                }

                val = pAnim->getAttr("PosEpsilon");
                if (!val.empty())
                {
                    pAnim->getAttr("PosEpsilon", desc.format.oldFmt.m_fPOS_EPSILON);
                }

                val = pAnim->getAttr("RotEpsilon");
                if (!val.empty())
                {
                    pAnim->getAttr("RotEpsilon", desc.format.oldFmt.m_fROT_EPSILON);
                }

                val = pAnim->getAttr("compression");
                if (!val.empty())
                {
                    pAnim->getAttr("compression", desc.format.oldFmt.m_CompressionQuality);
                }

                val = pAnim->getAttr("Additive_Animation");
                if (!val.empty())
                {
                    desc.m_bAdditiveAnimation = GetAsBool(val, false);
                }

                if (def.FindIdentical(string(pPath), true))
                {
                    RCLog("AnimationInfoLoader: Duplicate  \"%s\" \"%s\"", pPath,  def.m_Model.c_str());
                }
                else
                {
                    desc.m_perBoneCompressionDesc.insert(desc.m_perBoneCompressionDesc.end(), def.m_MainDesc.m_perBoneCompressionDesc.begin(), def.m_MainDesc.m_perBoneCompressionDesc.end());
                    def.m_OverrideAnimations.push_back(desc);
                }
            }
        }

        size_t num = m_ADefinitions.size();
        m_ADefinitions.push_back(def);
    }

    return true;
}


static bool ParseDeleteValue(SBoneCompressionValues::EDelete& result, const char* pValue)
{
    if (!pValue || !pValue[0] || StringHelpers::EqualsIgnoreCase(pValue, "AutoDelete"))
    {
        result = SBoneCompressionValues::eDelete_Auto;
    }
    else if (StringHelpers::EqualsIgnoreCase(pValue, "DontDelete"))
    {
        result = SBoneCompressionValues::eDelete_No;
    }
    else if (StringHelpers::EqualsIgnoreCase(pValue, "Delete"))
    {
        result = SBoneCompressionValues::eDelete_Yes;
    }
    else
    {
        result = SBoneCompressionValues::eDelete_Auto;
        return false;
    }

    return true;
}


void SAnimationDesc::LoadBoneSettingsFromXML(const XmlNodeRef& pRoot)
{
    m_perBoneCompressionDesc.clear();

    const uint32 id = GetNodeIDFromName("PerBoneCompression", pRoot);
    if (id == -1)
    {
        return;
    }

    const XmlNodeRef node = pRoot->getChild(id);

    const uint32 count = node ? node->getChildCount() : 0;
    for (uint32 i = 0; i < count; ++i)
    {
        XmlNodeRef boneNode = node->getChild(i);
        if (!boneNode)
        {
            continue;
        }

        const char* name = boneNode->getAttr("Name");
        const char* nameContains = boneNode->getAttr("NameContains");
        if (nameContains[0] == '\0' && name[0] == '\0')
        {
            RCLogError("AnimationInfoLoader: PerBoneCompression element must contain either non-empty \"Name\" or non-empty \"NameContains\"");
        }

        SBoneCompressionDesc boneDesc;

        if (name[0] != '\0')
        {
            boneDesc.m_namePattern = name;
        }
        else if (nameContains[0] == '\0')
        {
            boneDesc.m_namePattern = "*";
        }
        else if (strchr(nameContains, '*') || strchr(nameContains, '?'))
        {
            boneDesc.m_namePattern = nameContains;
        }
        else
        {
            boneDesc.m_namePattern = string("*") + string(nameContains) + string("*");
        }

        if (m_bNewFormat)
        {
            const char* autodeletePos = boneNode->getAttr("autodeletePos");
            const char* autodeleteRot = boneNode->getAttr("autodeleteRot");

            const char* compressPosEps = boneNode->getAttr("compressPosEps");
            const char* compressRotEps = boneNode->getAttr("compressRotEps");

            if (!autodeletePos || !autodeleteRot || !compressPosEps || !compressRotEps)
            {
                RCLogError("PerBoneCompression must contain \"autodeletePos\", \"autodeleteRot\", \"compressPosEps\", \"compressRotEps\"");
            }

            boneDesc.format.newFmt.m_compressPosEps = float(atof(compressPosEps));
            boneDesc.format.newFmt.m_compressRotEps = float(atof(compressRotEps));

            if (!ParseDeleteValue(boneDesc.m_eDeletePos, autodeletePos) ||
                !ParseDeleteValue(boneDesc.m_eDeleteRot, autodeleteRot))
            {
                continue;
            }
        }
        else
        {
            const char* mult = boneNode->getAttr("Multiply");
            const char* remove = boneNode->getAttr("Delete");

            if (!mult && !remove)
            {
                RCLogError("AnimationInfoLoader: PerBoneCompression element must contain \"Multiply\" and/or \"Delete\"");
            }

            if (mult && mult[0])
            {
                boneDesc.format.oldFmt.m_mult = float(atof(mult));
            }
            else
            {
                boneDesc.format.oldFmt.m_mult = 1.0f;
            }

            boneDesc.m_eDeletePos = (atoi(remove) != 0) ? SBoneCompressionValues::eDelete_Yes : SBoneCompressionValues::eDelete_Auto;
            boneDesc.m_eDeleteRot = boneDesc.m_eDeletePos;
        }

        m_perBoneCompressionDesc.push_back(boneDesc);
    }
}


SBoneCompressionValues SAnimationDesc::GetBoneCompressionValues(const char* boneName, float platformMultiplier) const
{
    SBoneCompressionValues bc;

    bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_Auto;
    bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_Auto;

    float posErr;
    float rotErr;

    if (m_bNewFormat)
    {
        bc.m_autodeletePosEps = format.newFmt.m_autodeletePosEps;
        bc.m_autodeleteRotEps = format.newFmt.m_autodeleteRotEps;

        posErr = format.newFmt.m_compressPosEps;
        rotErr = format.newFmt.m_compressRotEps;
    }
    else
    {
        bc.m_autodeletePosEps = format.oldFmt.m_fPOS_EPSILON;
        bc.m_autodeleteRotEps = format.oldFmt.m_fROT_EPSILON;

        posErr = 0.00000001f * (float)format.oldFmt.m_CompressionQuality;
        rotErr = 0.00000001f * (float)format.oldFmt.m_CompressionQuality;
    }

    if (boneName)
    {
        const size_t count = m_perBoneCompressionDesc.size();
        for (size_t i = 0; i < count; ++i)
        {
            const SBoneCompressionDesc& desc = m_perBoneCompressionDesc[i];

            if (StringHelpers::MatchesWildcardsIgnoreCase(boneName, desc.m_namePattern))
            {
                bc.m_eAutodeletePos = desc.m_eDeletePos;
                bc.m_eAutodeleteRot = desc.m_eDeleteRot;

                if (m_bNewFormat)
                {
                    posErr = desc.format.newFmt.m_compressPosEps;
                    rotErr = desc.format.newFmt.m_compressRotEps;
                }
                else
                {
                    posErr *= desc.format.oldFmt.m_mult;
                    rotErr *= desc.format.oldFmt.m_mult;
                }

                break;
            }
        }
    }

    posErr *= platformMultiplier;
    rotErr *= platformMultiplier;

    if (!m_bNewFormat)
    {
        // squared distance -> distance
        posErr = sqrtf(Util::getMax(posErr, 0.0f));

        // old error tolerance value -> tolerance in degrees
        const float angleInRadians = 2 * sqrtf(Util::getMax(rotErr, 0.0f));
        const float angleInDegrees = Util::getClamped(RAD2DEG(angleInRadians), 0.0f, 180.0f);
        rotErr = angleInDegrees;
        //RCLog("%.10f -> %f deg", rotErr, angleInDegrees);
    }

    bc.m_compressPosTolerance = posErr;
    bc.m_compressRotToleranceInDegrees = rotErr;

    return bc;
}


const SAnimationDesc& SAnimationDefinition::GetAnimationDesc(const string& name) const
{
    string searchname(name);
    UnifyPath(searchname);

    uint32 numOverAnims = m_OverrideAnimations.size();
    for (uint32 i = 0; i < numOverAnims; ++i)
    {
        string mapname(m_OverrideAnimations[i].GetAnimString());
        if ((strstr(mapname.c_str(), searchname.c_str()) != 0) || (strstr(searchname.c_str(), mapname.c_str()) != 0))
        {
            return m_OverrideAnimations[i];
        }
    }

    return m_MainDesc;
}


void SAnimationDefinition::SetOverrideAnimationSettingsFilename(const string& animationSettingsFilename)
{
    s_OverrideAnimationSettingsFilename = animationSettingsFilename;
}


string SAnimationDefinition::GetAnimationSettingsFilename(const string& animationFilename, EPreferredAnimationSettingsFile flag)
{
    if (flag == ePASF_OVERRIDE_FILE && !s_OverrideAnimationSettingsFilename.empty())
    {
        return s_OverrideAnimationSettingsFilename;
    }

    const char* const ext =
        (flag == ePASF_UPTODATECHECK_FILE)
        ? ".$animsettings"
        : ".animsettings";

    return PathUtil::ReplaceExtension(animationFilename, ext);
}
bool SAnimationDefinition::GetDescFromAnimationSettingsFile(SAnimationDesc* desc, bool* errorReported, bool* usesNameContains, IPakSystem* pPakSystem, ICryXML* pXmlParser, const string& animationFilename, const std::vector<string>& jointNames)
{
    if (pXmlParser == NULL || pPakSystem == NULL)
    {
        return false;
    }

    const string animationSettingsFilename = GetAnimationSettingsFilename(animationFilename, ePASF_OVERRIDE_FILE);

    if (!FileUtil::FileExists(animationSettingsFilename.c_str()))
    {
        return false;
    }

    SAnimSettings animSettings;
    if (!animSettings.Load(animationSettingsFilename, jointNames, pXmlParser, pPakSystem))
    {
        RCLogError("Failed to load animsettings: %s", animationSettingsFilename.c_str());
        return false;
    }

    if (usesNameContains)
    {
        *usesNameContains = animSettings.build.compression.m_usesNameContainsInPerBoneSettings;
    }

    *desc = SAnimationDesc();
    {
        // The skip SkipSaveToDatabase value makes no sense in the context of the animation settings file.
        // Since we're creating an SAnimationDesc structure from the settings file and that information is simply not there,
        // but it's needed to fully initialize the SAnimationDesc structure properly, we use the value incoming from the
        // defaultDesc description instead.
        // TODO: Remove this, it should have no meaning!
        desc->m_bSkipSaveToDatabase = false;
    }

    desc->m_bAdditiveAnimation = animSettings.build.additive;
    desc->m_skeletonName = animSettings.build.skeletonAlias;


    if (animSettings.build.compression.m_useNewFormatWithDefaultSettings)
    {
        // This case is used only for compatibility with old AnimSettings that miss
        // CompressionSettings block.
        desc->m_bNewFormat = true;
        desc->format.newFmt.m_autodeletePosEps = 0.0f;
        desc->format.newFmt.m_autodeleteRotEps = 0.0f;
        desc->format.newFmt.m_compressPosEps = 0.0f;
        desc->format.newFmt.m_compressRotEps = 0.0f;
    }
    else
    {
        desc->m_bNewFormat = false;
        desc->format.oldFmt.m_CompressionQuality = animSettings.build.compression.m_compressionValue;
        desc->format.oldFmt.m_fROT_EPSILON = animSettings.build.compression.m_rotationEpsilon;
        desc->format.oldFmt.m_fPOS_EPSILON = animSettings.build.compression.m_positionEpsilon;
    }
    desc->m_tags = animSettings.build.tags;

    const SCompressionSettings::TControllerSettings& controllers = animSettings.build.compression.m_controllerCompressionSettings;
    for (SCompressionSettings::TControllerSettings::const_iterator it = controllers.begin(); it != controllers.end(); ++it)
    {
        const char* controllerName = it->first.c_str();
        const SControllerCompressionSettings& controller = it->second;

        SBoneCompressionDesc boneDesc;
        boneDesc.m_namePattern = controllerName;
        boneDesc.format.oldFmt.m_mult = controller.multiply;
        boneDesc.m_eDeletePos = controller.state == eCES_ForceDelete ? SBoneCompressionValues::eDelete_Yes : SBoneCompressionValues::eDelete_Auto;
        boneDesc.m_eDeleteRot = boneDesc.m_eDeletePos;
        desc->m_perBoneCompressionDesc.push_back(boneDesc);
    }

    return true;
}

bool SAnimationDefinition::FindIdentical(const string& name, bool checkLen)
{
    string searchname(name);
    UnifyPath(searchname);

    uint32 numOverAnims = m_OverrideAnimations.size();
    for (uint32 i = 0; i < numOverAnims; ++i)
    {
        string mapname(m_OverrideAnimations[i].GetAnimString());
        if ((strstr(mapname.c_str(), searchname.c_str()) != 0) || (strstr(searchname.c_str(), mapname.c_str()) != 0))
        {
            if (checkLen)
            {
                if (mapname.length() == searchname.length())
                {
                    return true;
                }
            }
            else
            {
                return true;
            }
        }
    }

    return false;
}

const SAnimationDefinition* CAnimationInfoLoader::GetAnimationDefinition(const string& name) const
{
    string searchname(name);
    UnifyPath(searchname);

    const SAnimationDefinition* result = 0;
    size_t bestPathLen = 0;

    for (uint32 i = 0; i < m_ADefinitions.size(); ++i)
    {
        const string& mapname = m_ADefinitions[i].GetUnifiedAnimationPath();

        if ((strstr(mapname.c_str(), searchname.c_str()) != 0) ||
            (strstr(searchname.c_str(), mapname.c_str()) != 0))
        {
            if (mapname.size() > bestPathLen)
            {
                result = &m_ADefinitions[i];
                bestPathLen = mapname.size();
            }
        }
    }

    return result;
}

const SPlatformAnimationSetup* CAnimationInfoLoader::GetPlatformSetup(const char* pName) const
{
    for (size_t i = 0; i < m_platformSetups.size(); ++i)
    {
        if (_stricmp(m_platformSetups[i].m_platform.c_str(), pName) == 0)
        {
            return &m_platformSetups[i];
        }
    }
    return 0;
}

