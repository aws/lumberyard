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

// Description : CMannequinDialog Implementation file.


#include "StdAfx.h"

#include "FragmentTrack.h"
#include "MannequinDialog.h"
#include "MannTransitionSettings.h"
#include "Controls/TransitionBrowser.h"

#include <IGameFramework.h>
#include <ICryMannequinEditor.h>
#include "Serialization/CRCRef.h"
#include <Serialization/IArchiveHost.h>
#include <Serialization/Enum.h>

#include <QMenu>

// Define the following to get basic editing of the blendout time (only on the last animation clip of the first animation layer of a fragment)
//#define MANNEQUIN_BLENDOUT_EDITING

uint32 CFragmentTrack::s_sharedKeyID = 1;
bool   CFragmentTrack::s_distributingKey = false;



ColorB FRAG_TRACK_COLOUR(200, 240, 200);
ColorB CLIP_TRACK_COLOUR(220, 220, 220);
ColorB PROC_CLIP_TRACK_COLOUR(200, 200, 240);
ColorB TAG_TRACK_COLOUR(220, 220, 220);

SKeyColour FRAG_KEY_COLOUR                      = {
    {020, 240, 020}, {100, 250, 100}, {250, 250, 250}
};
SKeyColour FRAG_TRAN_KEY_COLOUR             = {
    {220, 120, 020}, {250, 150, 100}, {250, 250, 250}
};
SKeyColour FRAG_TRANOUTRO_KEY_COLOUR    = {
    {220, 180, 040}, {250, 200, 120}, {250, 250, 250}
};
SKeyColour CLIP_KEY_COLOUR                      = {
    {220, 220, 020}, {250, 250, 100}, {250, 250, 250}
};
SKeyColour CLIP_TRAN_KEY_COLOUR             = {
    {220, 120, 020}, {250, 150, 100}, {250, 250, 250}
};
SKeyColour CLIP_TRANOUTRO_KEY_COLOUR    = {
    {200, 160, 020}, {220, 180, 100}, {250, 250, 250}
};
SKeyColour CLIP_KEY_COLOUR_INVALID      = {
    {220, 0, 0}, {250, 0, 0}, {250, 100, 100}
};
SKeyColour PROC_CLIP_KEY_COLOUR             = {
    {162, 208, 248}, {192, 238, 248}, {250, 250, 250}
};
SKeyColour PROC_CLIP_TRAN_KEY_COLOUR    = {
    {220, 120, 060}, {250, 150, 130}, {250, 250, 250}
};
SKeyColour PROC_CLIP_TRANOUTRO_KEY_COLOUR   = {
    {200, 160, 060}, {220, 180, 130}, {250, 250, 250}
};
SKeyColour TAG_KEY_COLOUR                           = {
    {220, 220, 220}, {250, 250, 250}, {250, 250, 250}
};

//////////////////////////////////////////////////////////////////////////
CFragmentTrack::CFragmentTrack(SScopeData& scopeData, EMannequinEditorMode editorMode)
    : m_scopeData(scopeData)
    , m_history(NULL)
    , m_editorMode(editorMode)
{
}


ColorB CFragmentTrack::GetColor() const
{
    return FRAG_TRACK_COLOUR;
}


const SKeyColour& CFragmentTrack::GetKeyColour(int key) const
{
    const CFragmentKey& fragKey = m_keys[key];
    if (fragKey.tranFlags & SFragmentBlend::ExitTransition)
    {
        return FRAG_TRANOUTRO_KEY_COLOUR;
    }
    else if (fragKey.transition)
    {
        return FRAG_TRAN_KEY_COLOUR;
    }
    else
    {
        return FRAG_KEY_COLOUR;
    }
}


const SKeyColour& CFragmentTrack::GetBlendColour(int key) const
{
    const CFragmentKey& fragKey = m_keys[key];
    if (fragKey.tranFlags & SFragmentBlend::ExitTransition)
    {
        return FRAG_TRANOUTRO_KEY_COLOUR;
    }
    else if (fragKey.transition)
    {
        return FRAG_TRAN_KEY_COLOUR;
    }
    else
    {
        return FRAG_KEY_COLOUR;
    }
}

void CFragmentTrack::SetHistory(SFragmentHistoryContext& history)
{
    m_history = &history;
}


int CFragmentTrack::GetNumSecondarySelPts(int key) const
{
    const CFragmentKey& fragKey = m_keys[key];

    if (fragKey.transition && IsKeySelected(key))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


int CFragmentTrack::GetSecondarySelectionPt(int key, float timeMin, float timeMax) const
{
    const CFragmentKey& fragKey = m_keys[key];

    if (fragKey.transition)
    {
        if ((fragKey.tranSelectTime >= timeMin) && (fragKey.tranSelectTime <= timeMax))
        {
            return eSK_SELECT_TIME;
        }
    }

    return 0;
}


int CFragmentTrack::FindSecondarySelectionPt(int& key, float timeMin, float timeMax) const
{
    const int numKeys = GetNumKeys();
    for (uint32 i = 0; i < numKeys; i++)
    {
        if (IsKeySelected(i))
        {
            const int ret = GetSecondarySelectionPt(i, timeMin, timeMax);
            if (ret != 0)
            {
                key = i;
                return ret;
            }
        }
    }

    return 0;
}


void CFragmentTrack::SetSecondaryTime(int key, int idx, float time)
{
    CFragmentKey& fragKey = m_keys[key];

    if (fragKey.transition)
    {
        int prevKey = GetPrecedingFragmentKey(key, true);
        if (prevKey >= 0)
        {
            CFragmentKey& fragKeyPrev   = m_keys[prevKey];

            if (idx == eSK_SELECT_TIME)
            {
                fragKey.tranSelectTime = max(time, fragKeyPrev.time);
            }
        }
    }
}


float CFragmentTrack::GetSecondaryTime(int key, int idx) const
{
    const CFragmentKey& fragKey = m_keys[key];

    if (fragKey.transition)
    {
        if (idx == eSK_SELECT_TIME)
        {
            return fragKey.tranSelectTime;
        }
    }

    return 0.0f;
}

int CFragmentTrack::GetNextFragmentKey(int key) const
{
    for (int i = key + 1; i < m_keys.size(); i++)
    {
        if (!m_keys[i].transition)
        {
            return i;
        }
    }

    return -1;
}

int CFragmentTrack::GetPrecedingFragmentKey(int key, bool includeTransitions) const
{
    const CFragmentKey& fragKey = m_keys[key];
    const bool isTransition = fragKey.transition;
    const int historyItem = fragKey.historyItem;

    for (int i = key - 1; i >= 0; i--)
    {
        const CFragmentKey& fragKeyPrev = m_keys[i];
        //--- Skip the enclosing fragment key, but not any transition clips
        if ((fragKeyPrev.transition && includeTransitions)
            || (!fragKeyPrev.transition && (fragKeyPrev.historyItem != historyItem)))
        {
            return i;
        }
    }

    return -1;
}

int CFragmentTrack::GetParentFragmentKey(int key) const
{
    const CFragmentKey& fragKey = m_keys[key];
    const bool isTransition = fragKey.transition;
    const int historyItem = fragKey.historyItem;

    for (int i = key - 1; i >= 0; i--)
    {
        const CFragmentKey& fragKeyPrev = m_keys[i];
        //--- Skip the enclosing fragment key, but not any transition clips
        if (!fragKeyPrev.transition && (fragKeyPrev.historyItem == historyItem))
        {
            return i;
        }
    }

    return -1;
}


QString CFragmentTrack::GetSecondaryDescription(int key, int idx) const
{
    if (idx == eSK_SELECT_TIME)
    {
        return "Select";
    }
    else if (idx == eSK_START_TIME)
    {
        return "Start";
    }
    return "";
}

void CFragmentTrack::InsertKeyMenuOptions(QMenu* menu, int keyID)
{
    menu->addSeparator();
    CFragmentKey keyFrag;
    bool addSeparator = false;
    GetKey(keyID, &keyFrag);
    if (keyFrag.transition == false)
    {
        menu->addAction(QObject::tr("Edit Fragment"))->setData(EDIT_FRAGMENT);
        addSeparator = true;
    }
    if (keyFrag.context)
    {
        if (keyFrag.transition)
        {
            menu->addAction(QObject::tr("Edit Transition"))->setData(EDIT_TRANSITION);
            addSeparator = true;

            int prevFragKey = GetPrecedingFragmentKey(keyID, false);
            if (prevFragKey >= 0)
            {
                CFragmentKey& keyFragFrom = m_keys[prevFragKey];

                bool isMostSpecific = (keyFrag.tranTagTo == keyFrag.tagStateFull) && (keyFrag.tranTagFrom == keyFragFrom.tagStateFull)
                    && (keyFrag.tranFragFrom != FRAGMENT_ID_INVALID) && (keyFrag.tranFragTo != FRAGMENT_ID_INVALID);
                if (!isMostSpecific)
                {
                    menu->addAction(QObject::tr("Insert More Specific Transition"))->setData(INSERT_TRANSITION);
                }
                else
                {
                    menu->addAction(QObject::tr("Insert Additional Transition"))->setData(INSERT_TRANSITION);
                }
            }
        }
        else if (keyID > 0)
        {
            menu->addAction(QObject::tr("Insert Transition"))->setData(INSERT_TRANSITION);
            addSeparator = true;
        }
    }

    if (addSeparator)
    {
        menu->addSeparator();
    }
    menu->addAction(QObject::tr("Find fragment transitions"))->setData(FIND_FRAGMENT_REFERENCES);
    menu->addAction(QObject::tr("Find tag transitions"))->setData(FIND_TAG_REFERENCES);
}

void CFragmentTrack::OnKeyMenuOption(int menuOption, int keyID)
{
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

    CFragmentKey& key = m_keys[keyID];
    SMannequinContexts* contexts = m_scopeData.mannContexts;
    const SControllerDef& contDef = *m_scopeData.mannContexts->m_controllerDef;

    switch (menuOption)
    {
    case EDIT_FRAGMENT:
        CMannequinDialog::GetCurrentInstance()->EditFragmentByKey(key, m_scopeData.contextID);
        break;
    case EDIT_TRANSITION:
        CMannequinDialog::GetCurrentInstance()->EditTransitionByKey(key, m_scopeData.contextID);
        break;
    case INSERT_TRANSITION:
    {
        int prevFragKey = GetPrecedingFragmentKey(keyID, false);
        CFragmentKey& lastKey = m_keys[prevFragKey];
        FragmentID fragFrom = lastKey.fragmentID;
        FragmentID fragTo   = key.fragmentID;
        SFragTagState    tagsFrom = lastKey.tagState;
        SFragTagState    tagsTo   = key.tagState;

        //--- Setup better defaults for to & from none fragment
        if (fragFrom == FRAGMENT_ID_INVALID)
        {
            tagsFrom = key.tagState;
        }
        else if (fragTo == FRAGMENT_ID_INVALID)
        {
            tagsTo = lastKey.tagState;
        }

        CMannTransitionSettingsDlg transitionInfoDlg("Insert Transition", fragFrom, fragTo, tagsFrom, tagsTo);
        if (transitionInfoDlg.exec() == CMannTransitionSettingsDlg::Accepted)
        {
            key.tranFragFrom = fragFrom;
            key.tranFragTo   = fragTo;
            key.tranTagFrom  = tagsFrom;
            key.tranTagTo        = tagsTo;

            //--- Add the first key from the target fragment here!
            CFragment fragmentNew;
            if (key.hasFragment || lastKey.hasFragment)
            {
                const CFragment* fragmentFrom = key.context->database->GetBestEntry(SFragmentQuery(lastKey.fragmentID, lastKey.tagState));
                const CFragment* fragmentTo     = key.context->database->GetBestEntry(SFragmentQuery(key.fragmentID, key.tagState));
                if (fragmentFrom || fragmentTo)
                {
                    const uint32 numLayersFrom = fragmentFrom ? fragmentFrom->m_animLayers.size() : 0;
                    const uint32 numLayersTo   = fragmentTo ? fragmentTo->m_animLayers.size() : 0;
                    const uint32 numLayers = max(numLayersFrom, numLayersTo);
                    fragmentNew.m_animLayers.resize(numLayers);
                    for (uint32 i = 0; i < numLayers; i++)
                    {
                        fragmentNew.m_animLayers[i].resize(1);

                        SAnimClip& animClipNew = fragmentNew.m_animLayers[i][0];

                        if (i < numLayersTo)
                        {
                            animClipNew.blend = fragmentTo->m_animLayers[i][0].blend;
                        }
                        else
                        {
                            animClipNew.blend = SAnimBlend();
                        }

                        animClipNew.animation.animRef.SetByString(NULL);
                        animClipNew.animation.flags = 0;
                        animClipNew.animation.playbackSpeed  = 1.0f;
                        animClipNew.animation.playbackWeight = 1.0f;
                    }
                }
            }

            SFragmentBlend fragBlend;
            fragBlend.selectTime = 0.0f;
            fragBlend.startTime = 0.0f;
            fragBlend.enterTime = 0.0f;
            fragBlend.pFragment = &fragmentNew;
            key.tranBlendUid = MannUtils::GetMannequinEditorManager().AddBlend(key.context->database, key.tranFragFrom, key.tranFragTo, key.tranTagFrom, key.tranTagTo, fragBlend);
            key.context->changeCount++;

            // Refresh the Transition Browser tree and show the new fragment in the Transition Editor
            CMannequinDialog::GetCurrentInstance()->TransitionBrowser()->Refresh();
            CMannequinDialog::GetCurrentInstance()->TransitionBrowser()->SelectAndOpenRecord(TTransitionID(fragFrom, fragTo, tagsFrom, tagsTo, key.tranBlendUid));
            CMannequinDialog::GetCurrentInstance()->ShowPane<CTransitionEditorPage*>();
        }
    }
    break;
    case FIND_FRAGMENT_REFERENCES:
    {
        QString description;
        float duration;
        GetKeyInfo(keyID, description, duration);
        QString fragmentName = description;

        int index = fragmentName.indexOf('(');
        fragmentName.remove(index, fragmentName.length() - index);

        CMannequinDialog::GetCurrentInstance()->FindFragmentReferences(fragmentName);
    } break;
    case FIND_TAG_REFERENCES:
    {
        QString description;
        float duration;
        GetKeyInfo(keyID, description, duration);
        QString tagName = description;

        int startIndex = tagName.indexOf('(') + 1;
        int endIndex = tagName.indexOf(')');
        tagName.remove(endIndex, tagName.length() - endIndex);
        tagName.remove(0, startIndex);

        CMannequinDialog::GetCurrentInstance()->FindTagReferences(tagName);
    } break;
    }
}

void CFragmentTrack::GetKeyInfo(int key, QString& description, float& duration)
{
    CFragmentKey keyFrag;
    GetKey(key, &keyFrag);

    const SControllerDef& contDef = *m_scopeData.mannContexts->m_controllerDef;

    if (keyFrag.transition)
    {
        duration = keyFrag.clipDuration;

        const char* fragNameFrom = (keyFrag.tranFragFrom != FRAGMENT_ID_INVALID) ? contDef.m_fragmentIDs.GetTagName(keyFrag.tranFragFrom) : NULL;
        const char* fragNameTo =   (keyFrag.tranFragTo != FRAGMENT_ID_INVALID) ? contDef.m_fragmentIDs.GetTagName(keyFrag.tranFragTo) : NULL;

        if (fragNameFrom && fragNameTo)
        {
            description = QObject::tr("%1->%2").arg(fragNameFrom).arg(fragNameTo);
        }
        else if (fragNameFrom)
        {
            description = QObject::tr("%1 to <Any>").arg(fragNameFrom);
        }
        else if (fragNameTo)
        {
            description = QObject::tr("<Any> to %1").arg(fragNameTo);
        }
        else
        {
            description = QObject::tr("ERROR");
        }
    }
    else
    {
        if (keyFrag.fragmentID == FRAGMENT_ID_INVALID)
        {
            description = QObject::tr("none");
        }
        else
        {
            QString tags;
            const char* fragName = contDef.m_fragmentIDs.GetTagName(keyFrag.fragmentID);
            if (keyFrag.hasFragment)
            {
                MannUtils::FlagsToTagList(tags, keyFrag.tagState, keyFrag.fragmentID, contDef);
            }
            else
            {
                tags = QObject::tr("<no match>");
            }
            if (keyFrag.fragOptionIdx < OPTION_IDX_RANDOM)
            {
                description = QString::fromLatin1("%1(%2 - %3").arg(fragName).arg(tags).arg(keyFrag.fragOptionIdx + 1);
            }
            else
            {
                description = QString::fromLatin1("%1(%2)").arg(fragName).arg(tags);
            }
        }

        int nextKey = GetNextFragmentKey(key);
        const float nextKeyTime = (nextKey >= 0) ? GetKeyTime(nextKey) : GetTimeRange().end;

        duration = nextKeyTime - keyFrag.time;
        if (!keyFrag.isLooping)
        {
            float transitionTime = max(0.0f, keyFrag.tranStartTime - keyFrag.time);
            duration = min(duration, keyFrag.clipDuration + transitionTime);
        }
    }
}

float CFragmentTrack::GetKeyDuration(const int key) const
{
    float duration = 0.0f;
    CFragmentKey keyFrag;
    GetKey(key, &keyFrag);

    const SControllerDef& contDef = *m_scopeData.mannContexts->m_controllerDef;

    if (keyFrag.transition)
    {
        duration = keyFrag.clipDuration;
    }
    else
    {
        const int nextKey = GetNextFragmentKey(key);
        const float nextKeyTime = (nextKey >= 0) ? GetKeyTime(nextKey) : GetTimeRange().end;

        duration = nextKeyTime - keyFrag.time;
        if (!keyFrag.isLooping)
        {
            const float transitionTime = max(0.0f, keyFrag.tranStartTime - keyFrag.time);
            duration = min(duration, keyFrag.clipDuration + transitionTime);
        }
    }

    return duration;
}

void CFragmentTrack::SerializeKey(CFragmentKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        key.context = m_scopeData.context[m_editorMode];
        keyNode->getAttr("fragment", key.fragmentID);
        keyNode->getAttr("fragOptionIdx", key.fragOptionIdx);
        keyNode->getAttr("scopeMask", key.scopeMask);
        keyNode->getAttr("sharedID", key.sharedID);
        keyNode->getAttr("historyItem", key.historyItem);
        keyNode->getAttr("clipDuration", key.clipDuration);
        keyNode->getAttr("transitionTime", key.transitionTime);
        keyNode->getAttr("tranSelectTime", key.tranSelectTime);
        keyNode->getAttr("tranStartTime", key.tranStartTime);
        keyNode->getAttr("tranStartTimeValue", key.tranStartTimeValue);
        keyNode->getAttr("tranStartTimeRelative", key.tranStartTimeRelative);
        keyNode->getAttr("tranFragFrom", key.tranFragFrom);
        keyNode->getAttr("tranFragTo", key.tranFragTo);
        keyNode->getAttr("tranLastClipEffectiveStart", key.tranLastClipEffectiveStart);
        keyNode->getAttr("tranLastClipDuration", key.tranLastClipDuration);
        keyNode->getAttr("hasFragment", key.hasFragment);
        keyNode->getAttr("isLooping", key.isLooping);
        keyNode->getAttr("trumpPrevious", key.trumpPrevious);
        keyNode->getAttr("transition", key.transition);
        keyNode->getAttr("tranFlags", key.tranFlags);
        keyNode->getAttr("fragIndex", key.fragIndex);

        if ((key.scopeMask & BIT(m_scopeData.scopeID)) == 0)
        {
            key.scopeMask = BIT(m_scopeData.scopeID);
            key.fragmentID = FRAGMENT_ID_INVALID;
        }
    }
    else
    {
        // N.B. not serializing out key.context, that is set from the track information on load
        keyNode->setAttr("fragment", key.fragmentID);
        keyNode->setAttr("fragOptionIdx", key.fragOptionIdx);
        keyNode->setAttr("scopeMask", key.scopeMask);
        keyNode->setAttr("sharedID", key.sharedID);
        keyNode->setAttr("historyItem", key.historyItem);
        keyNode->setAttr("clipDuration", key.clipDuration);
        keyNode->setAttr("transitionTime", key.transitionTime);
        keyNode->setAttr("tranSelectTime", key.tranSelectTime);
        keyNode->setAttr("tranStartTime", key.tranStartTime);
        keyNode->setAttr("tranStartTimeValue", key.tranStartTimeValue);
        keyNode->setAttr("tranStartTimeRelative", key.tranStartTimeRelative);
        keyNode->setAttr("tranFragFrom", key.tranFragFrom);
        keyNode->setAttr("tranFragTo", key.tranFragTo);
        keyNode->setAttr("tranLastClipEffectiveStart", key.tranLastClipEffectiveStart);
        keyNode->setAttr("tranLastClipDuration", key.tranLastClipDuration);
        keyNode->setAttr("hasFragment", key.hasFragment);
        keyNode->setAttr("isLooping", key.isLooping);
        keyNode->setAttr("trumpPrevious", key.trumpPrevious);
        keyNode->setAttr("transition", key.transition);
        keyNode->setAttr("tranFlags", key.tranFlags);
        keyNode->setAttr("fragIndex", key.fragIndex);

        // Add some nodes to be populated below
        keyNode->addChild(XmlHelpers::CreateXmlNode("tagState"));
        keyNode->addChild(XmlHelpers::CreateXmlNode("tagStateFull"));
        keyNode->addChild(XmlHelpers::CreateXmlNode("tranTagFrom"));
        keyNode->addChild(XmlHelpers::CreateXmlNode("tranTagTo"));
    }

    const SControllerDef contDef = *(m_scopeData.mannContexts->m_controllerDef);
    MannUtils::SerializeFragTagState(key.tagState, contDef, key.fragmentID, keyNode->findChild("tagState"), bLoading);
    MannUtils::SerializeFragTagState(key.tagStateFull, contDef, key.fragmentID, keyNode->findChild("tagStateFull"), bLoading);
    MannUtils::SerializeFragTagState(key.tranTagFrom, contDef, key.fragmentID, keyNode->findChild("tranTagFrom"), bLoading);
    MannUtils::SerializeFragTagState(key.tranTagTo, contDef, key.fragmentID, keyNode->findChild("tranTagTo"), bLoading);
}

void CFragmentTrack::SelectKey(int keyID, bool select)
{
    TSequencerTrack<CFragmentKey>::SelectKey(keyID, select);

    CFragmentKey& key = m_keys[keyID];
    if (!s_distributingKey && (key.sharedID != 0))
    {
        s_distributingKey = true;
        const uint32 numFragTracks = m_history->m_tracks.size();
        CFragmentKey& key = m_keys[keyID];
        for (uint32 f = 0; f < numFragTracks; f++)
        {
            CSequencerTrack* pTrack = m_history->m_tracks[f];

            if (pTrack && (pTrack != this) && (pTrack->GetParameterType() == SEQUENCER_PARAM_FRAGMENTID))
            {
                CFragmentTrack* fragTrack = (CFragmentTrack*)pTrack;
                const uint32 scopeID = fragTrack->GetScopeData().scopeID;
                const int numKeys = fragTrack->GetNumKeys();
                for (int nKey = 0; nKey < numKeys; nKey++)
                {
                    if (fragTrack->m_keys[nKey].sharedID == key.sharedID)
                    {
                        fragTrack->CloneKey(nKey, key);
                        break;
                    }
                }
            }
        }
        s_distributingKey = false;
    }
}

void CFragmentTrack::CloneKey(int nKey, const CFragmentKey& key)
{
    CFragmentKey keyClone;
    GetKey(nKey, &keyClone);
    //--- Copy key details
    keyClone.fragmentID = key.fragmentID;
    keyClone.historyItem = key.historyItem;
    keyClone.sharedID = key.sharedID;
    keyClone.flags = key.flags;
    keyClone.tagStateFull.fragmentTags = key.tagStateFull.fragmentTags;
    keyClone.scopeMask = key.scopeMask;
    SetKey(nKey, &keyClone);
}

void CFragmentTrack::DistributeSharedKey(int keyID)
{
    s_distributingKey = true;
    const uint32 numScopes = m_scopeData.mannContexts->m_scopeData.size();

    CFragmentKey& key = m_keys[keyID];

    uint32 numSetScopes = 0;
    for (uint32 i = 0; i < numScopes; i++)
    {
        if (key.scopeMask & BIT(i))
        {
            numSetScopes++;
        }
    }

    const bool isSharedKey  = (numSetScopes > 1);
    const bool wasSharedKey = (key.sharedID != 0);
    int sharedID = key.sharedID;
    if (isSharedKey && !wasSharedKey)
    {
        key.sharedID = s_sharedKeyID++;
        sharedID = key.sharedID;
    }
    else if (!isSharedKey && wasSharedKey)
    {
        key.sharedID = 0;
    }

    if (isSharedKey || wasSharedKey)
    {
        const uint32 numFragTracks = m_history->m_tracks.size();
        for (uint32 f = 0; f < numFragTracks; f++)
        {
            CSequencerTrack* pTrack = m_history->m_tracks[f];

            if (pTrack && (pTrack != this) && (pTrack->GetParameterType() == SEQUENCER_PARAM_FRAGMENTID))
            {
                CFragmentTrack* fragTrack = (CFragmentTrack*)pTrack;
                const uint32 scopeID = fragTrack->GetScopeData().scopeID;
                const int numKeys = fragTrack->GetNumKeys();
                int nKey;
                for (nKey = 0; (nKey < numKeys) && (fragTrack->m_keys[nKey].sharedID != sharedID); nKey++)
                {
                    ;
                }

                const bool needsKey = ((key.scopeMask & BIT(scopeID)) != 0);
                const bool hasKey       = (nKey != numKeys) && (sharedID != 0);
                if (needsKey)
                {
                    if (!hasKey)
                    {
                        //--- Insert key
                        nKey = fragTrack->CreateKey(key.time);
                    }

                    fragTrack->CloneKey(nKey, key);
                }
                else
                {
                    if (hasKey)
                    {
                        //--- Delete key
                        fragTrack->RemoveKey(nKey);
                    }
                }

                if (needsKey)
                {
                    //--- HACK -> ensuring that we are sorted stops glitches
                    fragTrack->MakeValid();
                }
            }
        }
    }

    s_distributingKey = false;
}


void CFragmentTrack::SetKey(int index, CSequencerKey* _key)
{
    CFragmentKey& key = *(CFragmentKey*)_key;
    uint32 lastID = m_keys[index].sharedID;

    if (key.scopeMask == 0)
    {
        key.scopeMask = BIT(m_scopeData.scopeID);
    }
    TSequencerTrack<CFragmentKey>::SetKey(index, _key);

    if (!s_distributingKey)
    {
        if (lastID != m_keys[index].sharedID)
        {
            //--- Cloning might try to dupe the sharedID, don't allow it
            m_keys[index].sharedID = 0;
        }
        DistributeSharedKey(index);
    }
}

//! Set time of specified key.

void CFragmentTrack::SetKeyTime(int index, float time)
{
    TSequencerTrack<CFragmentKey>::SetKeyTime(index, time);

    //--- HACK -> ensuring that we are sorted stops glitches
    MakeValid();
}

SScopeData& CFragmentTrack::GetScopeData()
{
    return m_scopeData;
}

bool CFragmentTrack::CanEditKey(int key) const
{
    const CFragmentKey& fragmentKey = m_keys[key];
    if (m_editorMode == eMEM_TransitionEditor)
    {
        return false;
    }
    else
    {
        return !fragmentKey.transition;
    }
}

bool CFragmentTrack::CanMoveKey(int key) const
{
    const CFragmentKey& fragmentKey = m_keys[key];
    if (m_editorMode == eMEM_TransitionEditor)
    {
        return key > 0;
    }
    else
    {
        return !fragmentKey.transition;
    }
}

bool CFragmentTrack::CanAddKey(float time) const
{
    return (m_editorMode == eMEM_Previewer);
}

bool CFragmentTrack::CanRemoveKey(int key) const
{
    return (m_editorMode == eMEM_Previewer);
}

//////////////////////////////////////////////////////////////////////////
CClipKey::CClipKey()
    : CSequencerKey()
    , historyItem(HISTORY_ITEM_INVALID)
    , startTime(0.0f)
    , playbackSpeed(1.0f)
    , playbackWeight(1.0f)
    , duration(0.0f)
    , blendDuration(0.2f)
    , blendOutDuration(0.2f)
    , blendCurveType(ECurveType::Linear)
    , animFlags(0)
    , alignToPrevious(false)
    , clipType(eCT_Normal)
    , blendType(eCT_Normal)
    , fragIndexMain(0)
    , fragIndexBlend(0)
    , jointMask(0)
    , referenceLength(-1.0f)
    , animIsAdditive(FALSE)
    , m_animSet(NULL)
{
    memset(blendChannels, 0, sizeof(blendChannels));
}

CClipKey::~CClipKey()
{
}

void CClipKey::Set(const SAnimClip& animClip, const IAnimationSet* pAnimSet, const EClipType transitionType[SFragmentData::PART_TOTAL])
{
    animFlags               = animClip.animation.flags | animClip.blend.flags;
    startTime               = animClip.blend.startTime;
    playbackSpeed       = animClip.animation.playbackSpeed;
    playbackWeight  = animClip.animation.playbackWeight;
    for (uint32 i = 0; i < MANN_NUMBER_BLEND_CHANNELS; i++)
    {
        blendChannels[i] = animClip.animation.blendChannels[i];
    }
    blendDuration       = animClip.blend.duration;
    blendCurveType      = animClip.blend.curveType;
    fragIndexMain       = animClip.part;
    fragIndexBlend  = animClip.blendPart;
    clipType                = transitionType[fragIndexMain];
    blendType               = transitionType[fragIndexBlend];
    jointMask               = animClip.animation.weightList;
    SetAnimation(animClip.animation.animRef.c_str(), pAnimSet);
    if (animClip.isVariableLength)
    {
        referenceLength = duration;
        duration = animClip.referenceLength;
    }
}

void CClipKey::SetupAnimClip(SAnimClip& animClip, float lastTime, int fragPart)
{
    const uint32 TRANSITION_ANIM_FLAGS = CA_TRANSITION_TIMEWARPING | CA_IDLE2MOVE | CA_MOVE2IDLE;

    animClip.animation.flags                    = animFlags & ~TRANSITION_ANIM_FLAGS;
    animClip.blend.startTime                    = startTime;
    animClip.blend.flags                            = animFlags & TRANSITION_ANIM_FLAGS;
    animClip.animation.playbackSpeed  = playbackSpeed;
    animClip.animation.playbackWeight = playbackWeight;
    for (uint32 i = 0; i < MANN_NUMBER_BLEND_CHANNELS; i++)
    {
        animClip.animation.blendChannels[i] = blendChannels[i];
    }
    animClip.animation.animRef              = animRef;
    animClip.animation.weightList           = jointMask;
    animClip.blend.duration                     = blendDuration;
    animClip.blend.exitTime                     = alignToPrevious ? -1.0f : time - lastTime;
    animClip.blend.curveType                    = blendCurveType;
    animClip.part                                           = fragIndexMain;
    animClip.blendPart                              = fragIndexBlend;
    animClip.blend.terminal                     = (fragIndexMain != fragPart);
}

const char* CClipKey::GetDBAPath() const
{
    if (m_animSet)
    {
        const int animId = m_animSet->GetAnimIDByCRC(animRef.crc);
        if (animId >= 0)
        {
            return m_animSet->GetDBAFilePath(animId);
        }
    }

    return NULL;
}

void CClipKey::SetAnimation(const QString& animName, const IAnimationSet* animSet)
{
    animRef.SetByString(animName.toUtf8().data());
    const int id = animSet ? animSet->GetAnimIDByCRC(animRef.crc) : -1;
    m_animSet = animSet;
    if (id >= 0)
    {
        duration = animSet->GetDuration_sec(id);
        string animPath = animSet->GetFilePathByID(id);
        m_fileName = animPath.substr(animPath.find_last_of('/') + 1);
        m_filePath = animPath.substr(0, animPath.find_last_of('/') + 1);
        m_fileName = PathUtil::ReplaceExtension(m_fileName.toUtf8().data(), "");
        animExtension = animPath.substr(animPath.find_last_of('.') + 1);

        m_animCache.Set(id, animSet);
    }
    else
    {
        duration = 0.0f;
        m_animCache.Set(-1, NULL);
    }

    UpdateFlags();
}

void CClipKey::UpdateFlags()
{
    const int id = m_animSet ? m_animSet->GetAnimIDByCRC(animRef.crc) : -1;
    if (id >= 0)
    {
        // if an asset is in a DB, then we flag it as though it's in a PAK, because we can't hot load it anyway.
        string animPath = m_animSet->GetFilePathByID(id);
        const char* pFilePathDBA = m_animSet->GetDBAFilePath(id);
        const bool bIsOnDisk = gEnv->pCryPak->IsFileExist(animPath, ICryPak::eFileLocation_OnDisk);
        const bool bIsInsidePak = (pFilePathDBA || bIsOnDisk) ? false : gEnv->pCryPak->IsFileExist(animPath, ICryPak::eFileLocation_InPak);
        const bool bIsAdditive = (m_animSet->GetAnimationFlags(id) & CA_ASSET_ADDITIVE) != 0;
        const bool bIsLMG = (m_animSet->GetAnimationFlags(id) & CA_ASSET_LMG) != 0;

        m_fileState = eHasFile;
        m_fileState = (ESequencerKeyFileState)(m_fileState | (bIsInsidePak ? eIsInsidePak : eNone));
        m_fileState = (ESequencerKeyFileState)(m_fileState | (pFilePathDBA ? eIsInsideDB : eNone));
        m_fileState = (ESequencerKeyFileState)(m_fileState | (bIsOnDisk ? eIsOnDisk : eNone));
        animIsAdditive = bIsAdditive;
        animIsLMG = bIsLMG;
    }
    else
    {
        animIsAdditive = FALSE;
        m_fileState = eNone;
    }
}

void CClipKey::GetExtensions(QStringList& extensions, QString& editableExtension) const
{
    if (animIsLMG)
    {
        // .lmg .comb and .bspace are all LMG type
        extensions.clear();
        extensions.reserve(1);
        extensions.push_back('.' + animExtension);

        editableExtension = QString::fromLatin1(".") + animExtension;
    }
    else
    {
        extensions.clear();
        extensions.reserve(3);
        extensions.push_back(".i_caf");
        extensions.push_back(".ma");
        extensions.push_back(".animsettings");

        editableExtension = QStringLiteral(".ma");
    }
}

//////////////////////////////////////////////////////////////////////////
CClipTrack::CClipTrack(SScopeContextData* pContext, EMannequinEditorMode editorMode)
    : m_mainAnimTrack(false)
    , m_pContext(pContext)
    , m_editorMode(editorMode)
{
}

ColorB  CClipTrack::GetColor() const
{
    return CLIP_TRACK_COLOUR;
}


const SKeyColour& CClipTrack::GetKeyColour(int key) const
{
    const CClipKey& clipKey = m_keys[key];

    if (!clipKey.HasValidFile())
    {
        return CLIP_KEY_COLOUR_INVALID;
    }
    else if (clipKey.clipType == eCT_TransitionOutro)
    {
        return CLIP_TRANOUTRO_KEY_COLOUR;
    }
    else if (clipKey.clipType == eCT_Transition)
    {
        return CLIP_TRAN_KEY_COLOUR;
    }
    else
    {
        return CLIP_KEY_COLOUR;
    }
}


const SKeyColour& CClipTrack::GetBlendColour(int key) const
{
    const CClipKey& clipKey = m_keys[key];

    if (clipKey.blendType == eCT_TransitionOutro)
    {
        return CLIP_TRANOUTRO_KEY_COLOUR;
    }
    else if (clipKey.blendType == eCT_Transition)
    {
        return CLIP_TRAN_KEY_COLOUR;
    }
    else
    {
        return CLIP_KEY_COLOUR;
    }
}


int CClipTrack::GetNumSecondarySelPts(int key) const
{
    if (key >= m_keys.size() || key < 0)
    {
        return eCKSS_None;
    }
#ifdef MANNEQUIN_BLENDOUT_EDITING
    else if (m_editorMode == eMEM_FragmentEditor && key == m_keys.size() - 1 && m_mainAnimTrack)
    {
        const bool isLooping = 0 != (m_keys.back().animFlags & CA_LOOP_ANIMATION);
        if (!isLooping)
        {
            return eCKSS_BlendOut;
        }
    }
#endif

    return eCKSS_BlendIn;
}


int CClipTrack::GetSecondarySelectionPt(int key, float timeMin, float timeMax) const
{
    const CClipKey& clipKey = m_keys[key];

    const float blendTime = clipKey.time + clipKey.blendDuration;
    if (inrange(clipKey.time + clipKey.blendDuration, timeMin, timeMax))
    {
        return eCKSS_BlendIn;
    }
    else if (GetNumSecondarySelPts(key) >= eCKSS_BlendOut && inrange(clipKey.time + clipKey.GetBlendOutTime(), timeMin, timeMax))
    {
        return eCKSS_BlendOut;
    }
    else
    {
        return 0;
    }
}


int CClipTrack::FindSecondarySelectionPt(int& key, float timeMin, float timeMax) const
{
    const int numKeys = GetNumKeys();
    for (uint32 i = 0; i < numKeys; i++)
    {
        const int ret = GetSecondarySelectionPt(i, timeMin, timeMax);
        if (ret != 0)
        {
            key = i;
            return ret;
        }
    }

    return 0;
}

void CClipTrack::SetSecondaryTime(int key, int idx, float time)
{
    CClipKey& clipKey = m_keys[key];

    if (idx == eCKSS_BlendIn)
    {
        clipKey.blendDuration = max(time - clipKey.time, 0.0f);
    }
    else if (idx == eCKSS_BlendOut)
    {
        CRY_ASSERT(m_editorMode == eMEM_FragmentEditor && key == m_keys.size() - 1 && !(clipKey.animFlags & CA_LOOP_ANIMATION));
        clipKey.blendOutDuration = clamp_tpl(clipKey.time + clipKey.GetDuration() - time, 0.0f, clipKey.GetDuration());
    }
}

float CClipTrack::GetSecondaryTime(int key, int id) const
{
    const CClipKey& clipKey = m_keys[key];
    if (id == eCKSS_BlendIn)
    {
        return clipKey.time + clipKey.blendDuration;
    }
    else if (id == eCKSS_BlendOut)
    {
        CRY_ASSERT(key  == m_keys.size() - 1);
        return clipKey.time + clipKey.GetBlendOutTime();
    }
    else
    {
        return -1.0f;
    }
}

bool CClipTrack::CanMoveSecondarySelection(int key, int id) const
{
    return CanEditKey(key);
}

ECurveType CClipTrack::GetBlendCurveType(int key) const
{
    const CClipKey& clipKey = m_keys[key];
    return clipKey.blendCurveType;
}

void CClipTrack::InsertKeyMenuOptions(QMenu* menu, int keyID)
{
    menu->addSeparator();
    menu->addAction(QObject::tr("Find anim clip transitions"))->setData(FIND_FRAGMENT_REFERENCES);
}

void CClipTrack::ClearKeyMenuOptions(QMenu* menu, int keyID)
{
}

void CClipTrack::OnKeyMenuOption(int menuOption, int keyID)
{
    CClipKey& key = m_keys[keyID];
    switch (menuOption)
    {
    case FIND_FRAGMENT_REFERENCES:
        CMannequinDialog::GetCurrentInstance()->FindClipReferences(key);
        break;
    }
}

bool CClipTrack::CanAddKey(float time) const
{
    switch (m_editorMode)
    {
    case eMEM_Previewer:
        return false;
    case eMEM_FragmentEditor:
        return true;
    case eMEM_TransitionEditor:
    {
        const int numKeys = m_keys.size();
        bool wasLastKeyTransition = false;

        for (int i = 0; i < numKeys; i++)
        {
            const CClipKey& clipKey = m_keys[i];

            wasLastKeyTransition = (clipKey.blendType != eCT_Normal);
            if (clipKey.time > time)
            {
                if (wasLastKeyTransition)
                {
                    return true;
                }
            }
        }
        return wasLastKeyTransition;
    }
    default:
        assert(false);
        return false;
    }
}

bool CClipTrack::CanRemoveKey(int key) const
{
    switch (m_editorMode)
    {
    case eMEM_Previewer:
        return false;
    case eMEM_FragmentEditor:
        return true;
    case eMEM_TransitionEditor:
    {
        const CClipKey& clipKey = m_keys[key];
        return (clipKey.blendType != eCT_Normal);
    }
    default:
        assert(false);
        return false;
    }
}

bool CClipTrack::CanEditKey(int key) const
{
    switch (m_editorMode)
    {
    case eMEM_Previewer:
        return false;
    case eMEM_FragmentEditor:
        return true;
    case eMEM_TransitionEditor:
    {
        const CClipKey& clipKey = m_keys[key];
        return (clipKey.blendType != eCT_Normal);
    }
    default:
        assert(false);
        return false;
    }
}

bool CClipTrack::CanMoveKey(int key) const
{
    switch (m_editorMode)
    {
    case eMEM_Previewer:
        return false;
    case eMEM_FragmentEditor:
        return true;
    case eMEM_TransitionEditor:
    {
        const CClipKey& clipKey = m_keys[key];
        return (clipKey.blendType != eCT_Normal);
    }
    default:
        assert(false);
        return false;
    }
}


int CClipTrack::CreateKey(float time)
{
    int keyID = TSequencerTrack<CClipKey>::CreateKey(time);

    if (m_editorMode != eMEM_FragmentEditor)
    {
        CClipKey& newClipKey = m_keys[keyID];
        const int numKeys = m_keys.size();

        for (int i = 0; i < numKeys; i++)
        {
            const CClipKey& clipKey = m_keys[i];

            newClipKey.historyItem = clipKey.historyItem;
            if (clipKey.time > time)
            {
                newClipKey.fragIndexMain = newClipKey.fragIndexBlend = clipKey.fragIndexBlend;
                newClipKey.blendType = newClipKey.clipType = clipKey.blendType;
                break;
            }
            else
            {
                newClipKey.fragIndexMain = newClipKey.fragIndexBlend = clipKey.fragIndexMain;
                newClipKey.blendType = newClipKey.clipType = clipKey.clipType;
            }
        }
    }

    return keyID;
}

void CClipTrack::CheckKeyForSnappingToPrevious(int index)
{
    if (index > 0)
    {
        CClipKey& clipKey = m_keys[index];
        CClipKey& clipKeyLast = m_keys[index - 1];

        float lastEndTime = clipKeyLast.time + clipKeyLast.GetDuration();
        float timeDiff = fabs_tpl(clipKey.time - lastEndTime);
        clipKey.alignToPrevious = (timeDiff <= LOCK_TIME_DIFF) && ((clipKeyLast.animFlags & CA_LOOP_ANIMATION) == 0) && (clipKeyLast.playbackSpeed > 0.0f);
    }
}


void CClipTrack::SetKey(int index, CSequencerKey* _key)
{
    TSequencerTrack<CClipKey>::SetKey(index, _key);

    CheckKeyForSnappingToPrevious(index);
}


void CClipTrack::SetKeyTime(int index, float time)
{
    TSequencerTrack<CClipKey>::SetKeyTime(index, time);

    CheckKeyForSnappingToPrevious(index);
}


void CClipTrack::GetKeyInfo(int key, QString& description, float& duration)
{
    static string desc;
    desc.clear();

    const CClipKey& clipKey = m_keys[key];

    duration = max(clipKey.GetDuration(), clipKey.blendDuration);
    if (clipKey.animRef.IsEmpty())
    {
        desc = clipKey.blendDuration > 0.0f ? "Blend out" : "<None>";
    }
    else
    {
        desc = clipKey.animRef.c_str();
    }

    description = desc.c_str();

    const float nextKeyTime = (key + 1 < m_keys.size()) ? GetKeyTime(key + 1) : GetTimeRange().end;
    const float durationToNextKey = nextKeyTime - clipKey.time;
    if (clipKey.animRef.IsEmpty() || (clipKey.animFlags & CA_LOOP_ANIMATION))
    {
        duration = durationToNextKey;
    }
}

void CClipTrack::GetTooltip(int key, QString& description, float& duration)
{
    QString desc;

    GetKeyInfo(key, desc, duration);

    static const char* const additiveString = "\nAdditive";
    static const char* const pakString = "\nFrom PAK";
    static const char* const lmgString = "\nBlend-space";
    const CClipKey& clipKey = m_keys[key];
    if (!clipKey.animRef.IsEmpty())
    {
        desc += QObject::tr("\nPath: %1%2").arg(clipKey.GetFilePath(), clipKey.GetFileName());
        if (clipKey.IsAdditive())
        {
            desc += additiveString;
        }
        if (clipKey.IsLMG())
        {
            desc += lmgString;
        }
        if (clipKey.IsFileInsideDB())
        {
            if (const char* szDBAPath = clipKey.GetDBAPath())
            {
                desc += string().Format("\nDBA: %s", szDBAPath);
            }
        }
        else
        {
            desc += "\nNot in a DBA";
        }
        if (clipKey.IsFileInsidePak())
        {
            desc += pakString;
        }
    }

    description = desc;
}

float CClipTrack::GetKeyDuration(const int key) const
{
    const CClipKey& clipKey = m_keys[key];

    float duration = max(clipKey.GetDuration(), clipKey.blendDuration);
    const float nextKeyTime = (key + 1 < m_keys.size()) ? GetKeyTime(key + 1) : GetTimeRange().end;
    const float durationToNextKey = nextKeyTime - clipKey.time;
    if (clipKey.animRef.IsEmpty() || (clipKey.animFlags & CA_LOOP_ANIMATION))
    {
        duration = durationToNextKey;
    }

    return duration;
}

void CClipTrack::SerializeKey(CClipKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* animName = keyNode->getAttr("animName");
        IAnimationSet* animSet = m_pContext ? m_pContext->animSet : NULL;
        key.SetAnimation(animName, animSet);
    }
    else
    {
        keyNode->setAttr("animName", key.animRef.c_str());
    }
    key.Serialize(keyNode, bLoading);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/* TODO Jean: Port this to new audio system
struct SQuerySoundDurationHelper
    : public ISoundEventListener
{
    SQuerySoundDurationHelper(const char* const soundName, const bool isVoice)
        : m_duration(-1.f)
        , m_nAudioControlID(INVALID_AUDIO_CONTROL_ID)
    {
        const int voiceFlags = isVoice ? FLAG_SOUND_VOICE : 0;
        const int loadingFlags = FLAG_SOUND_DEFAULT_3D | FLAG_SOUND_LOAD_SYNCHRONOUSLY | voiceFlags;

        gEnv->pAudioSystem->GetAudioTriggerID(soundName, m_nAudioControlID);
        if (m_nAudioControlID != INVALID_AUDIO_CONTROL_ID)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> oRequestData(m_nAudioControlID, 0.0f);
            oRequest.nFlags = eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING;
            oRequest.pData = &oRequestData;
            gEnv->pAudioSystem->PushRequest(oRequest);
        }
    }

    ~SQuerySoundDurationHelper()
    {
        ClearSound();
    }

    virtual void OnSoundEvent(ESoundCallbackEvent event, ISound* pSound) override
    {
        assert(pSound == m_pSound.get());
        if (event == SOUND_EVENT_ON_INFO || event == SOUND_EVENT_ON_LOADED)
        {
            const bool looping = ((pSound->GetFlags() & FLAG_SOUND_LOOP) != 0);
            if (!looping)
            {
                const int lengthMs = pSound->GetLengthMs();
                if (lengthMs > 0)
                {
                    m_duration = pSound->GetLengthMs() / 1000.f;
                }
            }

            ClearSound();
        }
    }

    float GetDuration() const
    {
        return m_duration;
    }

private:
    void ClearSound()
    {
        if (m_pSound)
        {
            m_pSound->RemoveEventListener(this);
            m_pSound->Stop(ESoundStopMode_AtOnce);
            m_pSound.reset();
        }
    }

private:
    float m_duration;
    TAudioControlID m_nAudioControlID;
    _smart_ptr<ISound> m_pSound;
};
*/

//////////////////////////////////////////////////////////////////////////
CProcClipKey::CProcClipKey()
    : duration(0.0f)
    , blendDuration(0.2f)
    , blendCurveType(ECurveType::Linear)
    , historyItem(HISTORY_ITEM_INVALID)
    , clipType(eCT_Normal)
    , blendType(eCT_Normal)
    , fragIndexMain(0)
    , fragIndexBlend(0)
{
}


template <typename T>
T FindXmlProceduralClipParamValue(const XmlNodeRef& pXmlNode, const char* const paramName, const T& defaultValue)
{
    assert(pXmlNode);
    assert(paramName);
    XmlNodeRef pXmlParamNode = pXmlNode->findChild(paramName);
    if (!pXmlParamNode)
    {
        return defaultValue;
    }

    T value;
    const bool valueFound = pXmlParamNode->getAttr("value", value);
    if (!valueFound)
    {
        return defaultValue;
    }
    return value;
}

// TODO: Remove and give a more robust and maintainable solution to query duration of procedural clips in general!
// This function is a workaround to be able to display in the editor proper duration of sound clips.
void CProcClipKey::UpdateDurationBasedOnParams()
{
    /* TODO Jean: port to new audio system
    static const IProceduralClipFactory::THash s_playSoundClipHash("PlaySound");
    if (typeNameHash != s_playSoundClipHash)
    {
        return;
    }

    duration = -1;

    XmlNodeRef pXml = Serialization::SaveXmlNode(*pParams, "Params");
    if (!pXml)
    {
        return;
    }

    const XmlString soundName = FindXmlProceduralClipParamValue<XmlString>(pXml, "Sound", "");
    if (soundName.empty())
    {
        return;
    }

    const bool isVoice = FindXmlProceduralClipParamValue<bool>(pXml, "IsVoice", false);

    const SQuerySoundDurationHelper querySoundDurationHelper(soundName.c_str(), isVoice);
    duration = querySoundDurationHelper.GetDuration();
    */
}


IProceduralParamsPtr CloneProcClipParams(const IProceduralClipFactory::THash& typeNameHash, const IProceduralParamsPtr& pParams)
{
    if (!pParams)
    {
        return IProceduralParamsPtr();
    }

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IProceduralParamsPtr pNewParams = mannequinSys.GetProceduralClipFactory().CreateProceduralClipParams(typeNameHash);
    if (!pNewParams)
    {
        return IProceduralParamsPtr();
    }

    if (!Serialization::CloneBinary(*pNewParams, *pParams))
    {
        return IProceduralParamsPtr();
    }
    return pNewParams;
}


void CProcClipKey::FromProceduralEntry(const SProceduralEntry& procClip, const EClipType transitionType[SFragmentData::PART_TOTAL])
{
    typeNameHash   = procClip.typeNameHash;
    blendDuration  = procClip.blend.duration;
    blendCurveType = procClip.blend.curveType;
    fragIndexMain  = procClip.part;
    fragIndexBlend = procClip.blendPart;
    clipType       = transitionType[fragIndexMain];
    blendType      = transitionType[fragIndexBlend];
    pParams        = CloneProcClipParams(procClip.typeNameHash, procClip.pProceduralParams);
    UpdateDurationBasedOnParams();
}

void CProcClipKey::ToProceduralEntry(SProceduralEntry& procClip, const float lastTime, const int fragPart)
{
    procClip.typeNameHash = typeNameHash;

    procClip.blend.duration = blendDuration;
    procClip.blend.exitTime = time - lastTime;
    procClip.blend.terminal = (fragIndexMain != fragPart);
    procClip.blend.curveType = blendCurveType;

    procClip.part      = fragIndexMain;
    procClip.blendPart = fragIndexBlend;

    procClip.pProceduralParams = CloneProcClipParams(typeNameHash, pParams);
}

//////////////////////////////////////////////////////////////////////////
CProcClipTrack::CProcClipTrack(SScopeContextData* pContext, EMannequinEditorMode editorMode)
    : m_pContext(pContext)
    , m_editorMode(editorMode)
{
}

ColorB  CProcClipTrack::GetColor() const
{
    return PROC_CLIP_TRACK_COLOUR;
}

const SKeyColour& CProcClipTrack::GetKeyColour(int key) const
{
    const CProcClipKey& clipKey = m_keys[key];

    if (clipKey.clipType == eCT_Transition)
    {
        return PROC_CLIP_TRAN_KEY_COLOUR;
    }
    else if (clipKey.clipType == eCT_TransitionOutro)
    {
        return PROC_CLIP_TRANOUTRO_KEY_COLOUR;
    }
    return PROC_CLIP_KEY_COLOUR;
}

const SKeyColour& CProcClipTrack::GetBlendColour(int key) const
{
    const CProcClipKey& clipKey = m_keys[key];

    if (clipKey.blendType == eCT_Transition)
    {
        return PROC_CLIP_TRAN_KEY_COLOUR;
    }
    else if (clipKey.blendType == eCT_TransitionOutro)
    {
        return PROC_CLIP_TRANOUTRO_KEY_COLOUR;
    }
    return PROC_CLIP_KEY_COLOUR;
}

int CProcClipTrack::GetNumSecondarySelPts(int key) const
{
    return 1;
}

int CProcClipTrack::GetSecondarySelectionPt(int key, float timeMin, float timeMax) const
{
    const CProcClipKey& clipKey = m_keys[key];

    const float blendTime = clipKey.time + clipKey.blendDuration;
    if ((blendTime >= timeMin) && (blendTime <= timeMax))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int CProcClipTrack::FindSecondarySelectionPt(int& key, float timeMin, float timeMax) const
{
    const int numKeys = GetNumKeys();
    for (uint32 i = 0; i < numKeys; i++)
    {
        const int ret = GetSecondarySelectionPt(i, timeMin, timeMax);
        if (ret != 0)
        {
            key = i;
            return ret;
        }
    }

    return 0;
}

void CProcClipTrack::SetSecondaryTime(int key, int idx, float time)
{
    CProcClipKey& clipKey = m_keys[key];

    clipKey.blendDuration = max(time - clipKey.time, 0.0f);
}

float CProcClipTrack::GetSecondaryTime(int key, int id) const
{
    const CProcClipKey& clipKey = m_keys[key];
    return clipKey.time + clipKey.blendDuration;
}

bool CProcClipTrack::CanMoveSecondarySelection(int key, int id) const
{
    assert(id == 1);
    return CanEditKey(key);
}

ECurveType CProcClipTrack::GetBlendCurveType(int key) const
{
    const CProcClipKey& clipKey = m_keys[key];
    return clipKey.blendCurveType;
}

void CProcClipTrack::InsertKeyMenuOptions(QMenu* menu, int keyID)
{
}
void CProcClipTrack::ClearKeyMenuOptions(QMenu* menu, int keyID)
{
}
void CProcClipTrack::OnKeyMenuOption(int menuOption, int keyID)
{
}

bool CProcClipTrack::CanAddKey(float time) const
{
    switch (m_editorMode)
    {
    case eMEM_Previewer:
        return false;
    case eMEM_FragmentEditor:
        return true;
    case eMEM_TransitionEditor:
    {
        const int numKeys = m_keys.size();
        bool wasLastKeyTransition = false;

        for (int i = 0; i < numKeys; i++)
        {
            const CProcClipKey& clipKey = m_keys[i];

            wasLastKeyTransition = (clipKey.blendType != eCT_Normal);
            if (clipKey.time > time)
            {
                if (wasLastKeyTransition)
                {
                    return true;
                }
            }
        }
        return wasLastKeyTransition;
    }
    default:
        assert(false);
        return false;
    }
}

bool CProcClipTrack::CanEditKey(int key) const
{
    switch (m_editorMode)
    {
    case eMEM_Previewer:
        return false;
    case eMEM_FragmentEditor:
        return true;
    case eMEM_TransitionEditor:
    {
        const CProcClipKey& clipKey = m_keys[key];
        return (clipKey.blendType != eCT_Normal);
    }
    default:
        assert(false);
        return false;
    }

    return true;
}

bool CProcClipTrack::CanMoveKey(int key) const
{
    switch (m_editorMode)
    {
    case eMEM_Previewer:
        return false;
    case eMEM_FragmentEditor:
        return true;
    case eMEM_TransitionEditor:
    {
        const CProcClipKey& clipKey = m_keys[key];
        return (clipKey.blendType != eCT_Normal);
    }
    default:
        assert(false);
        return false;
    }

    return true;
}

int CProcClipTrack::CreateKey(float time)
{
    int keyID = TSequencerTrack<CProcClipKey>::CreateKey(time);

    if (m_editorMode != eMEM_FragmentEditor)
    {
        CProcClipKey& newClipKey = m_keys[keyID];
        const int numKeys = m_keys.size();

        for (int i = 0; i < numKeys; i++)
        {
            const CProcClipKey& clipKey = m_keys[i];

            newClipKey.historyItem = clipKey.historyItem;
            if (clipKey.time > time)
            {
                newClipKey.fragIndexMain = newClipKey.fragIndexBlend = clipKey.fragIndexBlend;
                newClipKey.blendType = newClipKey.clipType = clipKey.blendType;
                break;
            }
            else
            {
                newClipKey.fragIndexMain = newClipKey.fragIndexBlend = clipKey.fragIndexMain;
                newClipKey.blendType = newClipKey.clipType = clipKey.clipType;
            }
        }
    }

    return keyID;
}

void CProcClipTrack::GetKeyInfo(int key, QString& description, float& duration)
{
    static char desc[128];
    CProcClipKey& clipKey = m_keys[key];

    const char* name = "<None>";
    if (clipKey.blendDuration > 0.0f)
    {
        name = "Blend out";
    }

    if (!clipKey.typeNameHash.IsEmpty())
    {
        IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
        name = mannequinSys.GetProceduralClipFactory().FindTypeName(clipKey.typeNameHash);
    }

    {
        IProceduralParams::StringWrapper data;
        if (clipKey.pParams)
        {
            clipKey.pParams->GetExtraDebugInfo(data);
        }

        if (data.IsEmpty())
        {
            CRY_ASSERT(name);
            sprintf_s(desc, 128, "%s", name);
        }
        else
        {
            CRY_ASSERT(name);
            sprintf_s(desc, 128, "%s : %s", name, data.c_str());
        }
    }

    description = desc;

    duration = GetKeyDuration(key);
}

float CProcClipTrack::GetKeyDuration(const int key) const
{
    const CProcClipKey& clipKey = m_keys[key];

    const float nextKeyTime = (key + 1 < m_keys.size()) ? GetKeyTime(key + 1) : GetTimeRange().end;
    const float durationToNextKey = nextKeyTime - clipKey.time;

    const bool clipHasValidDuration = (0.f < clipKey.duration);
    if (clipHasValidDuration)
    {
        const float clipDuration = max(clipKey.blendDuration, clipKey.duration);
        const float duration = min(clipDuration, durationToNextKey);
        return duration;
    }
    else
    {
        const float duration = durationToNextKey;
        return duration;
    }
}

SERIALIZATION_ENUM_BEGIN(ECurveType, "CurveType")
SERIALIZATION_ENUM(ECurveType::Linear, "Linear", "Linear")
SERIALIZATION_ENUM(ECurveType::EaseIn, "EaseIn", "Ease In")
SERIALIZATION_ENUM(ECurveType::EaseOut, "EaseOut", "Ease Out")
SERIALIZATION_ENUM(ECurveType::EaseInOut, "EaseInOut", "Ease In/Out")
SERIALIZATION_ENUM_END()

void CProcClipKey::Serialize(Serialization::IArchive& ar)
{
    ar(typeNameHash, "TypeNameHash");
    ar(duration, "Duration");
    ar(blendDuration, "BlendDuration");
    ar(blendCurveType, "BlendCurveType");
    ar(historyItem, "HistoryItem");
    ar(clipType, "ClipType");
    ar(blendType, "BlendType");
    ar(fragIndexMain, "FragIndexMain");
    ar(fragIndexBlend, "FragIndexBlend");

    if (ar.IsInput())
    {
        pParams.reset();
        if (!typeNameHash.IsEmpty())
        {
            IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
            pParams = mannequinSys.GetProceduralClipFactory().CreateProceduralClipParams(typeNameHash);
        }
    }

    if (pParams)
    {
        ar(*pParams, "Params");
    }
}

void CProcClipTrack::SerializeKey(CProcClipKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        Serialization::LoadXmlNode(key, keyNode);
    }
    else
    {
        Serialization::SaveXmlNode(keyNode, key);
    }
}

//////////////////////////////////////////////////////////////////////////
CTagTrack::CTagTrack(const CTagDefinition& tagDefinition)
    : m_tagDefinition(tagDefinition)
{
}

void CTagTrack::SetKey(int index, CSequencerKey* _key)
{
    TSequencerTrack<CTagKey>::SetKey(index, _key);
}

void CTagTrack::GetKeyInfo(int key, QString& description, float& duration)
{
    CTagKey& tagKey = m_keys[key];
    duration = 0.0f;

    CryStackStringT<char, 512> buffer;
    m_tagDefinition.FlagsToTagList(tagKey.tagState, buffer);
    description = buffer.c_str();
}

float CTagTrack::GetKeyDuration(const int key) const
{
    return 0.0f;
}

void CTagTrack::InsertKeyMenuOptions(QMenu* menu, int keyID)
{
}

void CTagTrack::ClearKeyMenuOptions(QMenu* menu, int keyID)
{
}

void CTagTrack::OnKeyMenuOption(int menuOption, int keyID)
{
}

void CTagTrack::SerializeKey(CTagKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    CryStackStringT<char, 512> buffer;
    if (bLoading)
    {
        buffer = keyNode->getAttr("tagState");
        m_tagDefinition.TagListToFlags(buffer, key.tagState);
    }
    else
    {
        m_tagDefinition.FlagsToTagList(key.tagState, buffer);
        keyNode->setAttr("tagState", buffer.c_str());
    }
}

ColorB  CTagTrack::GetColor() const
{
    return TAG_TRACK_COLOUR;
}

const SKeyColour& CTagTrack::GetKeyColour(int key) const
{
    return TAG_KEY_COLOUR;
}

const SKeyColour& CTagTrack::GetBlendColour(int key) const
{
    return TAG_KEY_COLOUR;
}

const CTagDefinition& CTagTrack::GetTagDef() const
{
    return m_tagDefinition;
}

//////////////////////////////////////////////////////////////////////////
CTransitionPropertyTrack::CTransitionPropertyTrack(SScopeData& scopeData)
    : m_scopeData(scopeData)
{
}

void CTransitionPropertyTrack::SetKey(int index, CSequencerKey* _key)
{
    TSequencerTrack<CTransitionPropertyKey>::SetKey(index, _key);

    CTransitionPropertyKey& tagKey = m_keys[index];
}

void CTransitionPropertyTrack::GetKeyInfo(int key, QString& description, float& duration)
{
    CTransitionPropertyKey& propKey = m_keys[key];
    duration = propKey.duration;

    const SControllerDef& contDef = *m_scopeData.mannContexts->m_controllerDef;

    const char* fragNameFrom = (propKey.blend.fragmentFrom != FRAGMENT_ID_INVALID) ? contDef.m_fragmentIDs.GetTagName(propKey.blend.fragmentFrom) : NULL;
    const char* fragNameTo =   (propKey.blend.fragmentTo != FRAGMENT_ID_INVALID) ? contDef.m_fragmentIDs.GetTagName(propKey.blend.fragmentTo) : NULL;

    const char* pszPropName[CTransitionPropertyKey::eTP_Count] = {
        "Select Time",
        "Earliest Start Time",
        "Transition",
    };
    const char* propName = (propKey.prop >= 0 && propKey.prop < CTransitionPropertyKey::eTP_Count) ? pszPropName[propKey.prop] : NULL;

    if (propName && fragNameFrom && fragNameTo)
    {
        description = QObject::tr("%1->%2 %3").arg(fragNameFrom).arg(fragNameTo).arg(propName);
    }
    else if (propName && fragNameFrom)
    {
        description = QObject::tr("%1 to <Any>").arg(fragNameFrom);
    }
    else if (propName && fragNameTo)
    {
        description = QObject::tr("<Any> to %1").arg(fragNameTo);
    }
    else
    {
        description = QObject::tr("ERROR");
    }
}

float CTransitionPropertyTrack::GetKeyDuration(const int key) const
{
    const CTransitionPropertyKey& propKey = m_keys[key];
    return propKey.duration;
}

bool CTransitionPropertyTrack::CanEditKey(int key) const
{
    return true;
}

bool CTransitionPropertyTrack::CanMoveKey(int key) const
{
    const CTransitionPropertyKey& propKey = m_keys[key];
    return propKey.prop != CTransitionPropertyKey::eTP_Transition;
}

void CTransitionPropertyTrack::InsertKeyMenuOptions(QMenu* menu, int keyID)
{
}

void CTransitionPropertyTrack::ClearKeyMenuOptions(QMenu* menu, int keyID)
{
}

void CTransitionPropertyTrack::OnKeyMenuOption(int menuOption, int keyID)
{
}

void CTransitionPropertyTrack::SerializeKey(CTransitionPropertyKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        // Nobody should be copying or pasting this track - change callback will restore it
        OnChange();
    }
}

ColorB  CTransitionPropertyTrack::GetColor() const
{
    return TAG_TRACK_COLOUR;
}

const SKeyColour& CTransitionPropertyTrack::GetKeyColour(int key) const
{
    return TAG_KEY_COLOUR;
}

const SKeyColour& CTransitionPropertyTrack::GetBlendColour(int key) const
{
    return TAG_KEY_COLOUR;
}

const int CTransitionPropertyTrack::GetNumBlendsForKey(int index) const
{
    const CTransitionPropertyKey& propKey = m_keys[index];
    if (m_scopeData.context[eMEM_TransitionEditor] == NULL)
    {
        return 0;
    }
    return m_scopeData.context[eMEM_TransitionEditor]->database->GetNumBlends(propKey.blend.fragmentFrom, propKey.blend.fragmentTo, propKey.blend.tagStateFrom, propKey.blend.tagStateTo);
}

const SFragmentBlend* CTransitionPropertyTrack::GetBlendForKey(int index) const
{
    const CTransitionPropertyKey& propKey = m_keys[index];
    if (m_scopeData.context[eMEM_TransitionEditor] == NULL)
    {
        return 0;
    }
    return m_scopeData.context[eMEM_TransitionEditor]->database->GetBlend(propKey.blend.fragmentFrom, propKey.blend.fragmentTo, propKey.blend.tagStateFrom, propKey.blend.tagStateTo, propKey.blend.blendIdx);
}

const SFragmentBlend* CTransitionPropertyTrack::GetAlternateBlendForKey(int index, int blendIdx) const
{
    const CTransitionPropertyKey& propKey = m_keys[index];
    if (m_scopeData.context[eMEM_TransitionEditor] == NULL)
    {
        return 0;
    }
    return m_scopeData.context[eMEM_TransitionEditor]->database->GetBlend(propKey.blend.fragmentFrom, propKey.blend.fragmentTo, propKey.blend.tagStateFrom, propKey.blend.tagStateTo, blendIdx);
}

void CTransitionPropertyTrack::UpdateBlendForKey(int index, SFragmentBlend& blend)
{
    CTransitionPropertyKey& propKey = m_keys[index];
    MannUtils::GetMannequinEditorManager().SetBlend(m_scopeData.context[eMEM_TransitionEditor]->database, propKey.blend.fragmentFrom, propKey.blend.fragmentTo, propKey.blend.tagStateFrom, propKey.blend.tagStateTo, propKey.blend.blendUid, blend);

    CMannequinDialog& mannDialog = *CMannequinDialog::GetCurrentInstance();
    mannDialog.TransitionBrowser()->Refresh();
}

//////////////////////////////////////////////////////////////////////////
CParamTrack::CParamTrack()
{
}

void CParamTrack::GetKeyInfo(int key, QString& description, float& duration)
{
    static char desc[128];

    CParamKey& paramKey = m_keys[key];
    duration = 0.0f;
    description = paramKey.name;
}

float CParamTrack::GetKeyDuration(const int key) const
{
    return 0.0f;
}

void CParamTrack::InsertKeyMenuOptions(QMenu* menu, int keyID)
{
}
void CParamTrack::ClearKeyMenuOptions(QMenu* menu, int keyID)
{
}
void CParamTrack::OnKeyMenuOption(int menuOption, int keyID)
{
}
void CParamTrack::SerializeKey(CParamKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        keyNode->getAttr("paramCRC", key.parameter.crc);
        keyNode->getAttr("paramQuatVec", key.parameter.value.q.v);
        keyNode->getAttr("paramQuatScalar", key.parameter.value.q.w);
        keyNode->getAttr("paramVec", key.parameter.value.t);
        key.name = keyNode->getAttr("name");
        keyNode->getAttr("historyItem", key.historyItem);
        keyNode->getAttr("isLocation", key.isLocation);
    }
    else
    {
        keyNode->setAttr("paramCRC", key.parameter.crc);
        keyNode->setAttr("paramQuatVec", key.parameter.value.q.v);
        keyNode->setAttr("paramQuatScalar", key.parameter.value.q.w);
        keyNode->setAttr("paramVec", key.parameter.value.t);
        keyNode->setAttr("name", key.name.toUtf8().data());
        keyNode->setAttr("historyItem", key.historyItem);
        keyNode->setAttr("isLocation", key.isLocation);
    }
}

ColorB  CParamTrack::GetColor() const { return TAG_TRACK_COLOUR; }
const SKeyColour& CParamTrack::GetKeyColour(int key) const
{
    return TAG_KEY_COLOUR;
}
const SKeyColour& CParamTrack::GetBlendColour(int key) const
{
    return TAG_KEY_COLOUR;
}

