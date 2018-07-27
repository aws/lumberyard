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

#ifndef CRYINCLUDE_CRYANIMATION_FACIALANIMATION_LIPSYNC_H
#define CRYINCLUDE_CRYANIMATION_FACIALANIMATION_LIPSYNC_H
#pragma once

#include <IFacialAnimation.h>
#include "VectorSet.h"

class CFacialAnimationContext;

//////////////////////////////////////////////////////////////////////////
struct SPhoneme
{
    wchar_t codeIPA;     // IPA (International Phonetic Alphabet) code.
    char    ASCII[4];    // ASCII name for this phoneme (SAMPA for English).
    string  description; // Phoneme description.
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(codeIPA);
        pSizer->AddObject(ASCII);
        pSizer->AddObject(description);
    }
};

//////////////////////////////////////////////////////////////////////////
class CPhonemesLibrary
    : public IPhonemeLibrary
{
public:
    CPhonemesLibrary();

    //////////////////////////////////////////////////////////////////////////
    // IPhonemeLibrary
    //////////////////////////////////////////////////////////////////////////
    virtual int GetPhonemeCount() const;
    virtual bool GetPhonemeInfo(int nIndex, SPhonemeInfo& phoneme);
    virtual int FindPhonemeByName(const char* sPhonemeName);
    //////////////////////////////////////////////////////////////////////////

    SPhoneme& GetPhoneme(int nIndex);
    void LoadPhonemes(const char* filename);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_phonemes);
    }

private:
    std::vector<SPhoneme> m_phonemes;
};

//////////////////////////////////////////////////////////////////////////
class CFacialSentence
    : public IFacialSentence
    , public _reference_target_t
{
public:
    struct SFaceIdentifierHandleLess
        : public std::binary_function<CFaceIdentifierHandle, CFaceIdentifierHandle, bool>
    {
        bool operator()(const CFaceIdentifierHandle& left, const CFaceIdentifierHandle& right) const
        {
            return left.GetCRC32() < right.GetCRC32();
        };
    };
    typedef VectorSet<CFaceIdentifierHandle, SFaceIdentifierHandleLess > OverridesMap;

    CFacialSentence();

    //////////////////////////////////////////////////////////////////////////
    // IFacialSentence
    //////////////////////////////////////////////////////////////////////////
    IPhonemeLibrary* GetPhonemeLib();
    virtual void SetText(const char* text) { m_text = text; };
    virtual const char* GetText() { return m_text; };
    virtual void ClearAllPhonemes() { m_phonemes.clear(); m_words.clear(); ++m_nValidateID; };
    virtual int  GetPhonemeCount() { return (int)m_phonemes.size(); };
    virtual bool GetPhoneme(int index, Phoneme& ph);
    virtual int  AddPhoneme(const Phoneme& ph);

    virtual void ClearAllWords() { m_words.clear(); ++m_nValidateID; };
    virtual int GetWordCount() { return (int)m_words.size(); };
    virtual bool GetWord(int index, Word& wrd);
    virtual void AddWord(const Word& wrd);

    virtual int Evaluate(float fTime, float fInputPhonemeStrength, int maxSamples, ChannelSample* samples);
    //////////////////////////////////////////////////////////////////////////

    int GetPhonemeFromTime(int timeMs, int nFirst = 0);

    bool GetPhonemeInfo(int phonemeId, SPhonemeInfo& phonemeInfo) const;
    void Serialize(XmlNodeRef& node, bool bLoading);

    Phoneme& GetPhoneme(int index) { return m_phonemes[index]; };

    void Animate(const QuatTS& rAnimLocationNext, CFacialAnimationContext* pAnimContext, float fTime, float fPhonemeStrength, const OverridesMap& overriddenPhonemes);

    int GetValidateID() {return m_nValidateID; }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(&m_phonemes, sizeof(m_phonemes) + m_phonemes.capacity() * sizeof(Phoneme));
        pSizer->AddObject(&m_words, sizeof(m_words) + m_words.capacity() * sizeof(Word));
        pSizer->AddObject(m_nValidateID);
    }

private:
    struct WordRec
    {
        string text;
        int startTime;
        int endTime;
        std::vector<Phoneme> phonemes;
        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(this, sizeof(*this));
            pSizer->AddObject(text);
            pSizer->AddObject(startTime);
            pSizer->AddObject(endTime);
            pSizer->AddObject(&phonemes, phonemes.capacity() * sizeof(Phoneme));
        };
    };
    string m_text;
    std::vector<Phoneme> m_phonemes;
    std::vector<WordRec> m_words;

    // If this value has changed, then the sentence has changed.
    int m_nValidateID;
};

#endif // CRYINCLUDE_CRYANIMATION_FACIALANIMATION_LIPSYNC_H
