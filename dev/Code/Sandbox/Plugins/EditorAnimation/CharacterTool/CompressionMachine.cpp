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
#include "platform.h"
#include <smartptr.h>
#include "Include/IBackgroundTaskManager.h"
#include "CafCompressionHelper.h"
#include "CompressionMachine.h"
#include "../Shared/AnimSettings.h"
#include <ISystem.h>
#include <ICryAnimation.h>
#include <IEditor.h>
#include <Util/PathUtil.h>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <CryPath.h>
#include "Serialization.h"
#include "Serialization/BinArchive.h"


static bool IsAimAnimation(ICharacterInstance* pCharacterInstance, const char* animationName)
{
    assert(pCharacterInstance != NULL);

    IAnimationSet* pAnimationSet = pCharacterInstance->GetIAnimationSet();
    assert(pAnimationSet != NULL);

    const int localAnimationId = pAnimationSet->GetAnimIDByName(animationName);
    if (localAnimationId == -1)
    {
        return false;
    }

    const uint32 animationFlags = pAnimationSet->GetAnimationFlags(localAnimationId);
    const bool isAimAnimation = (animationFlags & CA_AIMPOSE);

    return isAimAnimation;
}

static bool SetCafReferenceByName(SCAFReference* ref, ICharacterInstance* character, const char* animationName)
{
    IAnimationSet* animationSet = character->GetIAnimationSet();
    if (!animationSet)
    {
        return false;
    }
    const int localAnimationId = animationSet->GetAnimIDByName(animationName);
    if (localAnimationId < 0)
    {
        return false;
    }
    ref->reset(animationSet->GetFilePathCRCByAnimID(localAnimationId), animationName);
    return true;
}


static string GetAnimSettingsFilename(const string& animationPath)
{
    const string gameFolderPath = PathUtil::AddSlash(Path::GetEditingGameDataFolder().c_str());
    const string animationFilePath = gameFolderPath + animationPath;

    const string animSettingsFilename = PathUtil::ReplaceExtension(animationFilePath.c_str(), "animsettings");
    return animSettingsFilename;
}

void AddAnimationToAnimationSet(ICharacterInstance& characterInstance, const char* animationName, const char* animationPath)
{
    if (!animationName)
    {
        return;
    }
    if (!animationPath)
    {
        return;
    }

    IAnimationSet* pAnimationSet = characterInstance.GetIAnimationSet();

    const int localAnimationId = pAnimationSet->GetAnimIDByName(animationName);
    const bool animationExists = (localAnimationId != -1);
    if (animationExists)
    {
        return;
    }

    IDefaultSkeleton& defaultSkeleton = characterInstance.GetIDefaultSkeleton();
    pAnimationSet->AddAnimationByPath(animationName, animationPath, &defaultSkeleton);
}


void SCAFReference::reset(uint32 pathCRC, const char* animationName)
{
    if (this->pathCRC != pathCRC)
    {
        if (this->pathCRC)
        {
            gEnv->pCharacterManager->CAF_Release(this->pathCRC);
        }
        this->pathCRC = pathCRC;
        if (this->pathCRC)
        {
            gEnv->pCharacterManager->CAF_AddRef(pathCRC);
        }
    }

    this->animationName = animationName;
}

namespace CharacterTool
{
    static bool SerializedEquivalent(const Serialization::SStruct& a, const Serialization::SStruct& b)
    {
        Serialization::BinOArchive oaA;
        a(oaA);
        Serialization::BinOArchive oaB;
        b(oaB);

        if (oaB.length() != oaA.length())
        {
            return false;
        }
        return memcmp(oaA.buffer(), oaB.buffer(), oaA.length()) == 0;
    }

    class CompressionMachine::BackgroundTaskCompressPreview
        : public IBackgroundTask
    {
    public:
        BackgroundTaskCompressPreview(CompressionMachine* pEditor, int sessionIndex, const char* animationPath, const char* outputAnimationPath, const SAnimSettings& animSettings, bool ignorePresets)
            : m_pEditor(pEditor)
            , m_animSettings(animSettings)
            , m_animationPath(animationPath)
            , m_outputAnimationPath(outputAnimationPath)
            , m_outputFileSize(0)
            , m_compressedCafSize(0)
            , m_sessionIndex(sessionIndex)
            , m_ignorePresets(ignorePresets)
            , m_failed(false)
        {
            m_description = "Preview for '";
            m_description += m_animationPath;
            ;
            m_description += "'";
        }

        const char* Description() const override { return m_description.c_str(); }
        const char* ErrorMessage() const override { return m_errorMessage.c_str(); }

        ETaskResult Work() override
        {
            ICVar* ca_useIMG_CAF = gEnv->pSystem->GetIConsole()->GetCVar("ca_useIMG_CAF");
            if (!ca_useIMG_CAF || ca_useIMG_CAF->GetIVal() != 0)
            {
                m_errorMessage = "Failed to create preview, 'ca_UseIMG_CAF' has value other than zero.";
                m_failed = true;
                return eTaskResult_Failed;
            }

            CafCompressionHelper::RemoveCompressionTarget(&m_errorMessage, m_outputAnimationPath.c_str());

            if (!CafCompressionHelper::CompressAnimationForPreview(&m_createdFile, &m_errorMessage, m_animationPath, m_animSettings, m_ignorePresets, m_sessionIndex))
            {
                m_failed = true;
                return eTaskResult_Failed;
            }


            auto filename = Path::GamePathToFullPath(m_createdFile.c_str());
            m_outputFileSize = gEnv->pCryPak->GetFileSizeOnDisk(filename.toUtf8().data());

            m_compressedCafSize = gEnv->pCryPak->FGetSize(m_animationPath.c_str(), true);

            return eTaskResult_Completed;
        }

        void Delete() { delete this; }

        bool MoveResult(string* outErrorMessage)
        {
            if (!m_createdFile.empty())
            {
                return CafCompressionHelper::MoveCompressionResult(outErrorMessage, m_createdFile.c_str(), m_outputAnimationPath.c_str());
            }

            return true;
        }

        void Finalize() override
        {
            m_pEditor->OnCompressed(this);
            CafCompressionHelper::CleanUpCompressionResult(m_createdFile.c_str());
        }

        bool Failed() const{ return m_failed; }

        SAnimSettings m_animSettings;
        string m_animationPath;

        _smart_ptr<CompressionMachine> m_pEditor;
        uint64 m_outputFileSize;
        uint64 m_compressedCafSize;
        string m_errorMessage;
        string m_description;
        string m_createdFile;
        string m_outputAnimationPath;
        bool m_ignorePresets;
        bool m_failed;
        int m_sessionIndex;
    };

    struct CompressionMachine::AnimationSetExtender
        : IAnimationSetListener
    {
        CompressionMachine* m_machine;
        vector<std::pair<string, string> > m_animations;
        ICharacterInstance* m_character;

        AnimationSetExtender(CompressionMachine* machine)
            : m_machine(machine)
            , m_character(0) { }

        void AddAnimation(const char* name, const char* path)
        {
            for (size_t i = 0; i < m_animations.size(); ++i)
            {
                if (m_animations[i].first == name)
                {
                    assert(m_animations[i].second == path);
                    return;
                }
            }

            m_animations.push_back(std::make_pair(name, path));
            if (m_pIAnimationSet)
            {
                AddAnimationToAnimationSet(*m_character, m_animations.back().first, m_animations.back().second);
            }
        }

        void SetCharacterInstance(ICharacterInstance* character)
        {
            m_character = character;
            IAnimationSet* animationSet = character ? character->GetIAnimationSet() : 0;
            if (animationSet)
            {
                animationSet->RegisterListener(this);
            }
            else if (m_pIAnimationSet)
            {
                m_pIAnimationSet->UnregisterListener(this);
            }

            OnAnimationSetReload();
        }

        void OnAnimationSetReload() override
        {
            if (m_pIAnimationSet)
            {
                assert(m_character);
                for (size_t i = 0; i < m_animations.size(); ++i)
                {
                    AddAnimationToAnimationSet(*m_character, m_animations[i].first, m_animations[i].second);
                }

                m_machine->StartPreview(false, false);
            }
        }
    };

    static void FormatReferenceName(string* name, string* path, int index)
    {
        if (name)
        {
            name->Format("_CompressionEditor_Reference_%d", index);
        }
        if (path)
        {
            path->Format("animations/editor/reference_preview_%d.caf", index);
        }
    }

    static void FormatPreviewName(string* name, string* path, int index)
    {
        if (name)
        {
            name->Format("_CompressionEditor_Preview_%d", index);
        }
        if (path)
        {
            path->Format("animations/editor/compression_preview_%d.caf", index);
        }
    }

    // ---------------------------------------------------------------------------

    const char* CompressionMachine::c_editorAnimDestination = "animations/editor";

    CompressionMachine::CompressionMachine()
        : m_state(eState_Idle)
        , m_showOriginalAnimation(false)
        , m_previewReloadListener(new AnimationSetExtender(this))
        , m_referenceReloadListener(new AnimationSetExtender(this))
        , m_normalizedStartTime(0.0f)
        , m_playbackSpeed(1.0f)
        , m_compressionSessionIndex(0)
        , m_compressedCharacter(0)
        , m_uncompressedCharacter(0)
    {
        // Connect for AssetSystemBus::Handler
        BusConnect(AZ::Crc32(CRY_CHARACTER_ANIMATION_FILE_EXT));
        RemoveAnimationEditorFolder();
    }

    void CompressionMachine::SetCharacters(ICharacterInstance* uncompressedCharacter, ICharacterInstance* compressedCharacter)
    {
        m_compressedCharacter = compressedCharacter;
        m_uncompressedCharacter = uncompressedCharacter;

        m_previewReloadListener->SetCharacterInstance(compressedCharacter);
        m_referenceReloadListener->SetCharacterInstance(uncompressedCharacter);

        SetState(eState_Idle);
    }

    static bool EquivalentAnimSettings(const vector<SAnimSettings>& a, const vector<SAnimSettings>& b)
    {
        if (a.size() != b.size())
        {
            return false;
        }
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!SerializedEquivalent(Serialization::SStruct(a[i]), Serialization::SStruct(b[i])))
            {
                return false;
            }
        }
        return true;
    }

    void CompressionMachine::PreviewAnimation(const PlaybackLayers& layers, const vector<bool>& isModified, bool showOriginalAnimation, const vector<SAnimSettings>& animSettings, float normalizedTime, bool forceRecompile, bool expectToReloadChrparams)
    {
        if (m_compressedCharacter == 0)
        {
            return;
        }

        bool hasNonEmptyLayers = false;
        for (size_t i = 0; i < layers.layers.size(); ++i)
        {
            if (!layers.layers[i].animation.empty())
            {
                hasNonEmptyLayers = true;
            }
        }

        if (!hasNonEmptyLayers)
        {
            Reset();
            return;
        }

        m_normalizedStartTime = normalizedTime;

        if (!forceRecompile && layers.ContainsSameAnimations(m_playbackLayers) &&
            EquivalentAnimSettings(m_animSettings, animSettings) &&
            showOriginalAnimation == m_showOriginalAnimation &&
            (m_state == eState_PreviewAnimation || m_state == eState_PreviewCompressedOnly))
        {
            // make sure we don't recompress animations for all layer parameters changes
            m_playbackLayers = layers;
            for (size_t i = 0; i < layers.layers.size(); ++i)
            {
                m_animations[i].enabled = layers.layers[i].enabled && !layers.layers[i].animation.empty();
            }

            Play(normalizedTime);
            return;
        }

        m_showOriginalAnimation = showOriginalAnimation;
        m_playbackLayers = layers;
        m_layerAnimationsModified = isModified;
        m_animSettings = animSettings;

        StartPreview(forceRecompile, expectToReloadChrparams);
    }

    void CompressionMachine::StartPreview(bool forceRecompile, bool expectToReloadChrparams)
    {
        m_animations.clear();
        m_animations.resize(m_playbackLayers.layers.size());

        if (!m_compressedCharacter || !m_compressedCharacter->GetISkeletonAnim())
        {
            return;
        }

        if (m_uncompressedCharacter)
        {
            m_uncompressedCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();
        }

        m_compressedCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();
        SetState(eState_Waiting);

        for (size_t i = 0; i < m_playbackLayers.layers.size(); ++i)
        {
            Animation& animation = m_animations[i];
            const PlaybackLayer& layer = m_playbackLayers.layers[i];
            animation.path = layer.path;
            animation.name = layer.animation;
            animation.state = eAnimationState_Compression;

            if (!layer.enabled || animation.name.empty())
            {
                animation.enabled = false;
            }

            if (animation.name.empty())
            {
                continue;
            }

            FormatPreviewName(&animation.compressedAnimationName, &animation.compressedAnimationPath, int(i));
            if (m_uncompressedCharacter)
            {
                FormatReferenceName(&animation.uncompressedAnimationName, &animation.uncompressedAnimationPath, int(i));
            }
            animation.type = animation.UNKNOWN;

            const char* extension = PathUtil::GetExt(animation.path.c_str());
            if (azstricmp(extension, "bspace") == 0 || azstricmp(extension, "comb") == 0)
            {
                animation.compressedAnimationName = animation.name;
                animation.uncompressedAnimationName = animation.name;
                animation.state = eAnimationState_Ready;
                animation.type = animation.BLEND_SPACE;
                animation.hasSourceFile = true;
                AnimationStateChanged();
                continue;
            }
            else if (azstricmp(extension, "anm") == 0 || azstricmp(extension, "cga") == 0)
            {
                animation.compressedAnimationName = animation.name;
                animation.uncompressedAnimationName = animation.name;
                SetCafReferenceByName(&animation.compressedCaf, m_compressedCharacter, animation.name);
                if (m_uncompressedCharacter)
                {
                    SetCafReferenceByName(&animation.uncompressedCaf, m_uncompressedCharacter, animation.name);
                }
                animation.state = eAnimationState_Ready;
                animation.type = animation.ANM;
                animation.hasSourceFile = true;
                AnimationStateChanged();
                continue;
            }
            else if (azstricmp(extension, "caf") == 0)
            {
                string iCafPath = Path::GamePathToFullPath(animation.path.c_str()).toUtf8().data();
                animation.hasSourceFile = gEnv->pCryPak->IsFileExist(iCafPath.c_str(), ICryPak::eFileLocation_OnDisk);

                if (IsAimAnimation(m_uncompressedCharacter, animation.name.c_str()))
                {
                    animation.type = animation.AIMPOSE;
                    animation.compressedAnimationName = animation.name;
                    animation.compressedAnimationPath = animation.path;
                    animation.uncompressedAnimationName = animation.name;
                    animation.uncompressedAnimationPath = animation.path;

                    SetCafReferenceByName(&animation.compressedCaf, m_compressedCharacter, animation.compressedAnimationName);

                    if (m_showOriginalAnimation && m_uncompressedCharacter)
                    {
                        SetCafReferenceByName(&animation.uncompressedCaf, m_uncompressedCharacter, animation.uncompressedAnimationName);
                    }

                    animation.state = eAnimationState_Ready;
                }
                else
                {
                    animation.type = animation.CAF;
                    if ((!m_showOriginalAnimation && !forceRecompile && !m_layerAnimationsModified[i]) || !animation.hasSourceFile)
                    {
                        animation.uncompressedCaf.reset();
                        animation.compressedAnimationName = animation.name;
                        if (SetCafReferenceByName(&animation.compressedCaf, m_compressedCharacter, animation.compressedAnimationName))
                        {
                            animation.state = eAnimationState_Ready;
                        }
                        else if (expectToReloadChrparams)
                        {
                            animation.state = eAnimationState_WaitingToReloadChrparams;
                            SetState(eState_Waiting);
                        }
                        else
                        {
                            animation.state = eAnimationState_Failed;
                            animation.failMessage = "Error: Animation is missing from animation set. Please assign\n"
                                " the animation to a skeleton alias in the properties window\n"
                                " and click the \"Import\" button.";
                            SetState(eState_Failed);
                        }


                        AnimationStateChanged();
                        continue;
                    }

                    if (m_layerAnimationsModified[i])
                    {
                        RemoveSource(animation.compressedAnimationPath.c_str());
                        //generate the compressed anim
                        animation.previewTask = new BackgroundTaskCompressPreview(this, m_compressionSessionIndex++, animation.path.c_str(), animation.compressedAnimationPath.c_str(), m_animSettings[i], false);
                        GetIEditor()->GetBackgroundTaskManager()->AddTask(animation.previewTask, eTaskPriority_RealtimePreview, eTaskThreadMask_IO);
                    }
                    else
                    {
                        animation.compressedAnimationName = animation.name;
                        animation.compressedAnimationPath = animation.path;
                        SetCafReferenceByName(&animation.compressedCaf, m_compressedCharacter, animation.compressedAnimationName);
                        animation.hasPreviewCompressed = true;
                    }

                    if (m_showOriginalAnimation)
                    {
                        //generate the uncompressed reference anim
                        SAnimSettings zeroAnimSettings = m_animSettings[i];
                        zeroAnimSettings.build.compression = SCompressionSettings();
                        zeroAnimSettings.build.compression.SetZeroCompression();
                        RemoveSource(animation.uncompressedAnimationPath.c_str());
                        animation.referenceTask = new BackgroundTaskCompressPreview(this, m_compressionSessionIndex++, animation.path.c_str(), animation.uncompressedAnimationPath.c_str(), zeroAnimSettings, true);
                        GetIEditor()->GetBackgroundTaskManager()->AddTask(animation.referenceTask, eTaskPriority_RealtimePreview, eTaskThreadMask_IO);
                    }
                    // TODO: cache recompressed reference animations
                    //
                    // if (!SetCafReferenceByName(&animation.uncompressedCaf, m_uncompressedCharacter, animation.referenceAnimationName))
                    // {
                    //  animation.state = eAnimationState_Failed;
                    //  animation.failMessage =  "Missing animation in animation set";
                    // }
                }
                continue;
            }
            else
            {
                animation.failMessage = "Unknown animation format";
                animation.state = eAnimationState_Failed;
                animation.type = animation.UNKNOWN;
                continue;
            }
        }

        AnimationStateChanged();
    }

    bool CompressionMachine::IsPlaying() const
    {
        return m_state == eState_PreviewAnimation || m_state == eState_PreviewCompressedOnly;
    }

    void CompressionMachine::Reset()
    {
        if (m_uncompressedCharacter)
        {
            m_uncompressedCharacter->GetISkeletonPose()->SetDefaultPose();
        }
        if (m_compressedCharacter)
        {
            m_compressedCharacter->GetISkeletonPose()->SetDefaultPose();
        }
        SetState(eState_Idle);
    }


    void CompressionMachine::OnCompressed(BackgroundTaskCompressPreview* pTask)
    {
        Animation* animation = 0;
        for (size_t i = 0; i < m_animations.size(); ++i)
        {
            if (m_animations[i].previewTask == pTask)
            {
                animation = &m_animations[i];
                animation->previewTask = 0;
            }
            else if (m_animations[i].referenceTask == pTask)
            {
                animation = &m_animations[i];
                animation->referenceTask = 0;
            }
        }

        if (!animation)
        {
            return;
        }

        if (pTask->IsCanceled())
        {
            animation->state = eAnimationState_Canceled;
            AnimationStateChanged();
            return;
        }

        if (pTask->Failed())
        {
            animation->failMessage = pTask->ErrorMessage();
            animation->state = eAnimationState_Failed;
            AnimationStateChanged();
            return;
        }

        if (!pTask->MoveResult(&animation->failMessage))
        {
            animation->state = eAnimationState_Failed;
            AnimationStateChanged();
            return;
        }

        if (pTask->m_ignorePresets) // for reference
        {
            m_uncompressedAnimations.push_back(animation);
            animation->compressionInfo.uncompressedSize = pTask->m_outputFileSize;
            animation->compressionInfo.compressedCafSize = pTask->m_compressedCafSize;
        }
        else
        {
            animation->compressionInfo.compressedPreviewSize = pTask->m_outputFileSize;
            animation->compressionInfo.compressedCafSize = pTask->m_compressedCafSize;
            animation->path = pTask->m_animationPath;

            if (m_state == eState_Waiting)
            {
                if (pTask->m_outputFileSize == 0 || animation->path.empty())
                {
                    animation->failMessage = pTask->m_errorMessage;
                }
            }
        }
    }

    void CompressionMachine::AnimationStateChanged()
    {
        if (m_state == eState_Waiting)
        {
            bool hasCanceled = false;
            bool hasFailed = false;
            bool hasIncomplete = false;
            bool hasCompressedOnly = false;

            for (size_t i = 0; i < m_animations.size(); ++i)
            {
                const Animation& a = m_animations[i];
                if (!a.enabled)
                {
                    continue;
                }
                if (a.state == eAnimationState_Canceled)
                {
                    hasCanceled = true;
                }
                else if (a.state == eAnimationState_Failed)
                {
                    hasFailed = true;
                }
                if (a.state != eAnimationState_Ready)
                {
                    hasIncomplete = true;
                }
                if (!a.hasSourceFile)
                {
                    hasCompressedOnly = true;
                }
            }

            if (hasFailed)
            {
                SetState(eState_Failed);
            }
            else if (hasCanceled)
            {
                SetState(eState_Idle);
            }
            else if (!hasIncomplete)
            {
                if (hasCompressedOnly)
                {
                    SetState(eState_PreviewCompressedOnly);
                }
                else
                {
                    SetState(eState_PreviewAnimation);
                }

                Play(m_normalizedStartTime);
            }
        }
    }


    void CompressionMachine::SetState(State state)
    {
        m_state = state;

        if (m_state == eState_Failed)
        {
            if (m_uncompressedCharacter)
            {
                m_uncompressedCharacter->GetISkeletonPose()->SetDefaultPose();
            }
            if (m_compressedCharacter)
            {
                m_compressedCharacter->GetISkeletonPose()->SetDefaultPose();
            }
        }
    }

    static void PlayLayer(ICharacterInstance* character, const PlaybackLayer& layer, const char* animationName,
        float normalizedTime, float playbackSpeed, bool loop)
    {
        ISkeletonAnim& skeletonAnim = *character->GetISkeletonAnim();
        int localAnimID = character->GetIAnimationSet()->GetAnimIDByName(animationName);
        if (localAnimID < 0)
        {
            return;
        }
        character->SetPlaybackScale(playbackSpeed);

        CryCharAnimationParams params;
        params.m_fPlaybackSpeed = 1.0f;
        params.m_fTransTime = 0;
        params.m_fPlaybackWeight = layer.weight;
        params.m_nLayerID = layer.layerId;
        params.m_fKeyTime = normalizedTime;
        params.m_nFlags = CA_FORCE_SKELETON_UPDATE | CA_ALLOW_ANIM_RESTART;
        if (loop)
        {
            params.m_nFlags |= CA_LOOP_ANIMATION;
        }
        else
        {
            params.m_nFlags |= CA_REPEAT_LAST_KEY;
        }
        skeletonAnim.StartAnimationById(localAnimID, params);
    }

    void CompressionMachine::Play(float normalizedTime)
    {
        if (m_state != eState_PreviewAnimation && m_state != eState_PreviewCompressedOnly)
        {
            return;
        }

        for (size_t i = 0; i < m_playbackLayers.layers.size(); ++i)
        {
            const PlaybackLayer& layer = m_playbackLayers.layers[i];
            Animation& animation = m_animations[i];
            if (!animation.enabled)
            {
                m_compressedCharacter->GetISkeletonAnim()->StopAnimationInLayer(layer.layerId, 0.0f);
                if (m_uncompressedCharacter)
                {
                    m_uncompressedCharacter->GetISkeletonAnim()->StopAnimationInLayer(layer.layerId, 0.0f);
                }
            }
            else
            {
                PlayLayer(m_compressedCharacter, layer, animation.compressedAnimationName.c_str(), normalizedTime, m_playbackSpeed, m_loop);
                if (animation.uncompressedCaf.PathCRC())
                {
                    PlayLayer(m_uncompressedCharacter, layer, animation.uncompressedAnimationName.c_str(), normalizedTime, m_playbackSpeed, m_loop);
                }
            }
        }

        SignalAnimationStarted();
    }

    void CompressionMachine::SetLoop(bool loop)
    {
        if (m_loop != loop)
        {
            m_loop = loop;
            if (IsPlaying())
            {
                int layer = 0; // TODO add layers supports
                ISkeletonAnim& skeletonAnim = *m_compressedCharacter->GetISkeletonAnim();
                float normalizedTime = skeletonAnim.GetAnimationNormalizedTime(&skeletonAnim.GetAnimFromFIFO(layer, 0));
                Play(normalizedTime);
            }
        }
    }

    void CompressionMachine::SetPlaybackSpeed(float playbackSpeed)
    {
        if (m_playbackSpeed != playbackSpeed)
        {
            m_playbackSpeed = playbackSpeed;
            if (IsPlaying())
            {
                int layer = 0; // TODO add layers supports
                ISkeletonAnim& skeletonAnim = *m_compressedCharacter->GetISkeletonAnim();
                float normalizedTime = skeletonAnim.GetAnimationNormalizedTime(&skeletonAnim.GetAnimFromFIFO(layer, 0));
                Play(normalizedTime);
            }
        }
    }

    void CompressionMachine::GetAnimationStateText(vector<StateText>* lines, bool compressedCharacter)
    {
        lines->clear();

        if (compressedCharacter)
        {
            ICVar* ca_useIMG_CAF = gEnv->pSystem->GetIConsole()->GetCVar("ca_useIMG_CAF");
            if (ca_useIMG_CAF->GetIVal() != 0)
            {
                StateText line;
                line.type = line.WARNING;
                line.text = "ca_useIMG_CAF has value other than 0.";
                lines->push_back(line);
                line.text = " Only animations compiled during the build can be previewed";
                lines->push_back(line);
            }

            if (m_state == eState_Waiting)
            {
                StateText line;
                line.text = "Compressing...";
                line.type = line.PROGRESS;

                for (size_t i = 0; i < m_animations.size(); ++i)
                {
                    if (m_animations[i].state == eAnimationState_WaitingToReloadChrparams)
                    {
                        line.text = "Reloading ChrParams...";
                        break;
                    }
                }
                lines->push_back(line);
            }
            else if (m_state == eState_Failed)
            {
                for (size_t i = 0; i < m_animations.size(); ++i)
                {
                    const Animation& animation = m_animations[i];
                    if (!animation.enabled)
                    {
                        continue;
                    }
                    if (animation.state == eAnimationState_Failed)
                    {
                        StateText line;
                        line.type = line.FAIL;
                        line.animation = animation.name;
                        line.text = animation.failMessage;
                        lines->push_back(line);
                    }
                }
            }
            else if (m_state == eState_PreviewAnimation)
            {
                for (size_t i = 0; i < m_animations.size(); ++i)
                {
                    StateText line;
                    line.type = line.INFO;

                    const Animation& animation = m_animations[i];
                    if (!animation.enabled)
                    {
                        continue;
                    }

                    switch (animation.type)
                    {
                    case Animation::AIMPOSE:
                    {
                        line.animation = animation.name;
                        line.text = "AIM/Look Pose";
                        lines->push_back(line);
                        break;
                    }
                    case Animation::BLEND_SPACE:
                    {
                        line.animation = animation.name;
                        line.text = "BlendSpace";
                        lines->push_back(line);
                        break;
                    }
                    case Animation::ANM:
                    {
                        line.animation = animation.name;
                        line.text = "ANM format does not support compression";
                        lines->push_back(line);
                        break;
                    }
                    case Animation::CAF:
                    {
                        if (m_showOriginalAnimation)
                        {
                            if (animation.hasSourceFile)
                            {
                                uint64 compressedSize = animation.compressionInfo.compressedPreviewSize;
                                if (compressedSize == 0)
                                {
                                    compressedSize = animation.compressionInfo.compressedCafSize;
                                }
                                if (animation.compressionInfo.uncompressedSize > 0)
                                {
                                    float percent =  float(compressedSize) * 100.0f / float(animation.compressionInfo.uncompressedSize);
                                    line.animation = animation.name;
                                    line.text.Format("Compressed %.0f->%.0fKB (%.1f%%)",
                                        animation.compressionInfo.uncompressedSize / 1024.0f,
                                        compressedSize / 1024.0f,
                                        percent);
                                    lines->push_back(line);
                                }
                                else
                                {
                                    line.animation = animation.name;
                                    line.text = "Compressed";
                                    lines->push_back(line);
                                }
                            }
                            else
                            {
                                line.animation = animation.name;
                                line.text = "Precompiled animation (missing source i_caf)";
                                lines->push_back(line);
                            }
                        }
                        break;
                    }
                    }
                }
            }
        }
        else
        {
            if (m_state == eState_PreviewAnimation || m_state == eState_PreviewCompressedOnly)
            {
                bool hasMissingSources = false;

                for (size_t i = 0; i < m_animations.size(); ++i)
                {
                    const Animation& animation = m_animations[i];
                    if (!animation.hasSourceFile)
                    {
                        StateText line;
                        line.type = line.WARNING;
                        line.animation = animation.name;
                        line.text = "Missing source i_caf";
                        hasMissingSources = true;
                        lines->push_back(line);
                    }
                }

                if (!hasMissingSources)
                {
                    StateText line;
                    line.type = line.INFO;
                    line.text = "Original";
                    lines->push_back(line);
                }

                for (size_t i = 0; i < m_animations.size(); ++i)
                {
                    const Animation& animation = m_animations[i];
                    if (animation.type == animation.BLEND_SPACE)
                    {
                        StateText line;
                        line.type = line.WARNING;
                        line.animation = animation.name;
                        line.text = "BlendSpace (compressed)";
                        lines->push_back(line);
                    }
                }
            }
        }
    }


    const char* CompressionMachine::AnimationPathConsideringPreview(const char* inputCaf) const
    {
        for (size_t i = 0; i < m_animations.size(); ++i)
        {
            if (m_animations[i].hasSourceFile && m_animations[i].path == inputCaf)
            {
                return m_animations[i].compressedAnimationPath.c_str();
            }
        }
        return inputCaf;
    }

    void CompressionMachine::OnFileChanged(AZStd::string assetPath)
    {
        QMetaObject::invokeMethod(this, "FileChanged_MainThread", Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(assetPath.c_str())));
    }

    void CompressionMachine::FileChanged_MainThread(QString assetPath)
    {
        // assetID always have the form "textures/whatever.dds", never absolute.
        string animationPath = assetPath.toUtf8().data();

        string animationFileName = PathUtil::GetFileName(animationPath);

        for (size_t i = 0; i < m_animations.size(); ++i)
        {
            if (m_animations[i].compressedAnimationPath == animationPath)
            {
                if (m_state == eState_Waiting)
                {
                    if (m_animations[i].compressionInfo.compressedCafSize != 0 && !m_animations[i].path.empty())
                    {
                        InstallPreviewAnim(i);
                    }
                    else
                    {
                        SetState(eState_Failed);
                    }
                }
                AnimationStateChanged();
                return;
            }
            if (m_animations[i].uncompressedAnimationPath == animationPath)
            {
                if (m_state == eState_Waiting)
                {
                    if (m_animations[i].compressionInfo.uncompressedSize != 0 && !m_animations[i].path.empty())
                    {
                        InstallReferenceAnim(i);
                    }
                    else
                    {
                        SetState(eState_Failed);
                    }
                }

                AnimationStateChanged();
                return;
            }
        }
    }

    void CompressionMachine::RemoveSource(string assetPath)
    {
        assetPath = Path::GamePathToFullPath(assetPath.c_str()).toUtf8().data();
        QFile::remove(assetPath.c_str());
    }


    void CompressionMachine::RemoveAnimationEditorFolder()
    {
        AZStd::string editorAnimationDirPath = Path::MakeModPathFromGamePath(c_editorAnimDestination);

        QDir editorAnimationDir(QString(editorAnimationDirPath.c_str()));
        editorAnimationDir.removeRecursively();
    }


    void CompressionMachine::InstallReferenceAnim(int animationIndex)
    {
        Animation* animation = &m_animations[animationIndex];

        m_previewReloadListener->AddAnimation(animation->uncompressedAnimationName.c_str(), animation->uncompressedAnimationPath.c_str());
        SetCafReferenceByName(&animation->uncompressedCaf, m_uncompressedCharacter, animation->uncompressedAnimationName);

        ICharacterManager* pCharacterManager = GetISystem()->GetIAnimationSystem();
        if (!pCharacterManager->ReloadCAF(animation->uncompressedAnimationPath))
        {
            animation->state = eAnimationState_Failed;
            animation->failMessage = "Reloading failed";
        }
        else
        {
            animation->hasReferenceCompressed = true;
        }

        if (animation->hasReferenceCompressed && animation->hasPreviewCompressed)
        {
            animation->state = eAnimationState_Ready;
        }
        RemoveSource(animation->uncompressedAnimationPath.c_str());
    }


    void CompressionMachine::InstallPreviewAnim(int animationIndex)
    {
        Animation* animation = &m_animations[animationIndex];
        m_previewReloadListener->AddAnimation(animation->compressedAnimationName.c_str(), animation->compressedAnimationPath.c_str());
        SetCafReferenceByName(&animation->compressedCaf, m_compressedCharacter, animation->compressedAnimationName);

        ICharacterManager* pCharacterManager = GetISystem()->GetIAnimationSystem();
        if (!pCharacterManager->ReloadCAF(animation->compressedAnimationPath))
        {
            animation->state = eAnimationState_Failed;
            animation->failMessage = "Reloading failed";
        }
        else
        {
            animation->hasPreviewCompressed = true;
        }

        if (animation->hasPreviewCompressed  && (animation->hasReferenceCompressed || !m_showOriginalAnimation))
        {
            animation->state = eAnimationState_Ready;
        }
        RemoveSource(animation->compressedAnimationPath.c_str());
    }
} // CharacterTool

#include <CharacterTool/CompressionMachine.moc>
