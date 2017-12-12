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

#include "pch.h"
#include "AnimationContent.h"
#include "EditorCompressionPresetTable.h"
#include "EditorDBATable.h"
#include "Shared/AnimationFilter.h"
#include "CharacterDocument.h"
#include <ICryAnimation.h>
#include "Serialization.h"
#include "Serialization/Decorators/EditorActionButton.h"
#include "EntryList.h"
#include <AzFramework/StringFunc/StringFunc.h> 
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include "CharacterDefinition.h"
#include "CharacterToolSystem.h"
#include <AzCore/std/containers/unordered_set.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace CharacterTool {
    AnimationContent::AnimationContent()
        : type(ANIMATION)
        , size(-1)
        , importState(NOT_SET)
        , loadedInEngine(false)
        , loadedAsAdditive(false)
        , animationId(-1)
        , delayApplyUntilStart(false)
        , system(nullptr)
    {
    }

    bool AnimationContent::HasAudioEvents() const
    {
        for (size_t i = 0; i < events.size(); ++i)
        {
            if (IsAudioEventType(events[i].type.c_str()))
            {
                return true;
            }
        }
        return false;
    }

    void AnimationContent::ApplyToCharacter(bool* triggerPreview, ICharacterInstance* characterInstance, const char* animationPath, bool startingAnimation)
    {
        if (startingAnimation)
        {
            if (!delayApplyUntilStart)
            {
                return;
            }
        }

        IAnimEvents* animEvents = gEnv->pCharacterManager->GetIAnimEvents();

        if (IAnimEventList* animEventList = animEvents->GetAnimEventList(animationPath))
        {
            animEventList->Clear();
            for (size_t i = 0; i < events.size(); ++i)
            {
                CAnimEventData animEvent;
                events[i].ToData(&animEvent);
                animEventList->Append(animEvent);
            }
            delayApplyUntilStart = false;
        }
        else
        {
            delayApplyUntilStart = !startingAnimation;
        }

        animEvents->InitializeSegmentationDataFromAnimEvents(animationPath);

        if (type == BLEND_SPACE)
        {
            XmlNodeRef xml = blendSpace.SaveToXml();
            _smart_ptr<IXmlStringData> content = xml->getXMLData();
            gEnv->pCharacterManager->InjectBSPACE(animationPath, content->GetString(), content->GetStringLength());
            gEnv->pCharacterManager->ReloadLMG(animationPath);
            gEnv->pCharacterManager->ClearBSPACECache();
            if (triggerPreview)
            {
                *triggerPreview = true;
            }
        }
        if (type == COMBINED_BLEND_SPACE)
        {
            XmlNodeRef xml = combinedBlendSpace.SaveToXml();
            _smart_ptr<IXmlStringData> content = xml->getXMLData();
            gEnv->pCharacterManager->InjectBSPACE(animationPath, content->GetString(), content->GetStringLength());
            gEnv->pCharacterManager->ReloadLMG(animationPath);
            gEnv->pCharacterManager->ClearBSPACECache();
            if (triggerPreview)
            {
                *triggerPreview = true;
            }
        }
    }

    void AnimationContent::UpdateBlendSpaceMotionParameters(IAnimationSet* animationSet, IDefaultSkeleton* skeleton)
    {
        for (size_t i = 0; i < blendSpace.m_examples.size(); ++i)
        {
            BlendSpaceExample& e = blendSpace.m_examples[i];
            Vec4 v;
            if (animationSet->GetMotionParameters(animationId, i, skeleton, v))
            {
                if (!e.specified[0])
                {
                    e.parameters.x = v.x;
                }
                if (!e.specified[1])
                {
                    e.parameters.y = v.y;
                }
                if (!e.specified[2])
                {
                    e.parameters.z = v.z;
                }
                if (!e.specified[3])
                {
                    e.parameters.w = v.w;
                }
            }
        }
    }

    bool IsSkeletonProductFromAssetImporterSource(const char* skeletonPath)
    {
        bool fullPathfound = false;
        AZStd::string assetSourcePath;
        AZStd::string assetProductPath(skeletonPath);
        AzFramework::StringFunc::Strip(assetProductPath, '.', true, true);
        EBUS_EVENT_RESULT(fullPathfound, AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, assetProductPath, assetSourcePath);

        if (fullPathfound)
        {
            AZStd::unordered_set<AZStd::string> extensions;
            EBUS_EVENT(AZ::SceneAPI::Events::AssetImportRequestBus, GetSupportedFileExtensions, extensions);
            for (const AZStd::string& extension : extensions)
            {
                return AzFramework::StringFunc::Find(assetSourcePath.c_str(), extension.c_str(), 0, true, false) != AZStd::string::npos;
            }
        }

        return false;
    }

    void AnimationContent::Serialize(Serialization::IArchive& ar)
    {
        if (type == ANIMATION && importState == COMPILED_BUT_NO_ANIMSETTINGS)
        {
            if (system && IsSkeletonProductFromAssetImporterSource(system->document->GetLoadedCharacterDefinition()->skeleton.c_str()))
            {
                ar.Warning(this, "Compression settings not available for animations generated from an fbx file.");
            }
            else
            {
                ar.Warning(this, "AnimSettings file used to compile the animation is missing. You may need to obtain it from version control.\n\nAlternatively you can create a new AnimSettings file.");
                bool createNewAnimSettings = false;
                ar(Serialization::ToggleButton(createNewAnimSettings), "createButton", "<Create New AnimSettings");
                if (createNewAnimSettings)
                {
                    importState = IMPORTED;
                }
            }
        }
        if (type == ANIMATION && (importState == NEW_ANIMATION || importState == IMPORTED))
        {
            ar(settings.build.additive, "additive", "Additive");
        }
        if (type == ANIMATION && importState == NEW_ANIMATION)
        {
            ar(SkeletonAlias(newAnimationSkeleton), "skeletonAlias", "Skeleton Alias");
            if (newAnimationSkeleton.empty())
            {
                ar.Warning(newAnimationSkeleton, "Skeleton alias should be specified in order to import animation.");
            }
        }

        if ((type == ANIMATION && (importState == IMPORTED || importState == WAITING_FOR_CHRPARAMS_RELOAD)) || type == AIMPOSE || type == LOOKPOSE)
        {
            ar(SkeletonAlias(settings.build.skeletonAlias), "skeletonAlias", "Skeleton Alias");
            if (settings.build.skeletonAlias.empty())
            {
                ar.Error(settings.build.skeletonAlias, "Skeleton alias is not specified for the animation.");
            }
            ar(TagList(settings.build.tags), "tags", "Tags");
        }

        if (type == ANIMATION && (importState == IMPORTED || importState == WAITING_FOR_CHRPARAMS_RELOAD))
        {
            bool presetApplied = false;
            if (ar.IsEdit() && ar.IsOutput())
            {
                if (EntryBase* entryBase = ar.FindContext<EntryBase>())
                {
                    SAnimationFilterItem item;
                    item.path = entryBase->path;
                    item.skeletonAlias = settings.build.skeletonAlias;
                    item.tags = settings.build.tags;

                    if (EditorCompressionPresetTable* presetTable = ar.FindContext<EditorCompressionPresetTable>())
                    {
                        const EditorCompressionPreset* preset = presetTable->FindPresetForAnimation(item);
                        if (preset)
                        {
                            presetApplied = true;
                            if (ar.OpenBlock("automaticCompressionSettings", "+!Compression Preset"))
                            {
                                ar(preset->entry.name, "preset", "!^");
                                const_cast<EditorCompressionPreset*>(preset)->entry.settings.Serialize(ar);
                                ar.CloseBlock();
                            }
                        }
                    }

                    if (EditorDBATable* dbaTable = ar.FindContext<EditorDBATable>())
                    {
                        int dbaIndex = dbaTable->FindDBAForAnimation(item);
                        if (dbaIndex >= 0)
                        {
                            string dbaName = dbaTable->GetEntryByIndex(dbaIndex)->entry.path;
                            ar(dbaName, "dbaName", "<!DBA");
                        }
                    }
                }
            }
            if (!ar.IsEdit() || !presetApplied)
            {
                int oldFilter = ar.GetFilter();
                ar.SetFilter(ar.GetFilter() | SERIALIZE_COMPRESSION_SETTINGS_AS_TREE);
                ar(settings.build.compression, "compression", "Compression");
                ar.SetFilter(oldFilter);
            }
        }
        if (type == BLEND_SPACE)
        {
            if (ar.IsEdit() && ar.IsOutput())
            {
                if (ICharacterInstance* characterInstance = ar.FindContext<ICharacterInstance>())
                {
                    UpdateBlendSpaceMotionParameters(characterInstance->GetIAnimationSet(), &characterInstance->GetIDefaultSkeleton());
                }
            }
            blendSpace.Serialize(ar);
        }
        if (type == COMBINED_BLEND_SPACE)
        {
            combinedBlendSpace.Serialize(ar);
        }
        if (type == ANM)
        {
            string msg = "Contains no properties.";
            ar(msg, "msg", "<!");
        }
        if (type != ANM && type != AIMPOSE && type != LOOKPOSE && (importState == IMPORTED || importState == WAITING_FOR_CHRPARAMS_RELOAD || importState == COMPILED_BUT_NO_ANIMSETTINGS))
        {
            ar(events, "events", "Animation Events");
        }
        ar(size, "size");
    }

    void AnimationContent::Reset()
    {
        *this = AnimationContent();
    }
}
