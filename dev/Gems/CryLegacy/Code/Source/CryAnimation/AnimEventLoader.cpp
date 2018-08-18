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

#include "CryLegacy_precompiled.h"
#include "AnimEventLoader.h"
#include "Model.h"
#include "CharacterManager.h"


namespace AnimEventLoader
{
    void DoAdditionalAnimEventInitialization(const char* animationFilePath, CAnimEventData& animEvent);
}


static bool s_bPreLoadParticleEffects = true;

void AnimEventLoader::SetPreLoadParticleEffects(bool bPreLoadParticleEffects)
{
    s_bPreLoadParticleEffects = bPreLoadParticleEffects;
}


bool AnimEventLoader::LoadAnimationEventDatabase(CDefaultSkeleton* pDefaultSkeleton, const char* pFileName)
{
    if (pFileName == 0 || pFileName[ 0 ] == 0)
    {
        return false;
    }

    CRY_DEFINE_ASSET_SCOPE("AnimEvents", pFileName);

    // Parse the xml.
    XmlNodeRef root = g_pISystem->LoadXmlFromFile(pFileName);
    if (!root)
    {
        //AnimWarning("Animation Event Database (.animevent) file \"%s\" could not be read (associated with model \"%s\")", pFileName, pDefaultSkeleton->GetFile() );
        return false;
    }

    // Load the events from the xml.
    uint32 numAnimations = root->getChildCount();
    for (uint32 nAnimationNode = 0; nAnimationNode < numAnimations; ++nAnimationNode)
    {
        XmlNodeRef animationRoot = root->getChild(nAnimationNode);

        // Check whether this is an animation.
        if (stack_string("animation") != animationRoot->getTag())
        {
            continue;
        }

        const uint32 numEvents = animationRoot->getChildCount();
        if (numEvents == 0)
        {
            continue;
        }

        XmlString animationFilePath = animationRoot->getAttr("name");

        IAnimEventList* pAnimEventList = g_AnimationManager.GetAnimEventList(animationFilePath.c_str());
        if (!pAnimEventList)
        {
            continue;
        }

        const uint32 currentAnimationEventCount = pAnimEventList->GetCount();
        if (currentAnimationEventCount != 0)
        {
            continue;
        }

        for (uint32 nEventNode = 0; nEventNode < numEvents; ++nEventNode)
        {
            XmlNodeRef eventNode = animationRoot->getChild(nEventNode);

            CAnimEventData animEvent;
            const bool couldLoadXmlData = g_AnimationManager.LoadAnimEventFromXml(eventNode, animEvent);
            if (!couldLoadXmlData)
            {
                continue;
            }

            pAnimEventList->Append(animEvent);

            DoAdditionalAnimEventInitialization(animationFilePath, animEvent);
        }

        g_AnimationManager.InitializeSegmentationDataFromAnimEvents(animationFilePath.c_str());
    }

    return true;
}


void AnimEventLoader::DoAdditionalAnimEventInitialization(const char* animationFilePath, CAnimEventData& animEvent)
{
    // NB. Currently, this is only executed when the events are created from an xml file, so for example the editor will
    // not be executing logic contained here while editing animation events.
    assert(animationFilePath);

    static const uint32 s_crc32_effect = CCrc32::ComputeLowercase("effect");
    const uint32 eventNameCRC32 = animEvent.GetNameLowercaseCRC32();

    if (s_bPreLoadParticleEffects)
    {
        if (eventNameCRC32 == s_crc32_effect)
        {
            const char* effectName = animEvent.GetCustomParameter();
            gEnv->pParticleManager->FindEffect(effectName);
        }
    }
}

