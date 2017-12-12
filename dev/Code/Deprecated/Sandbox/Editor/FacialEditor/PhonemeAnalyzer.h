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

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_PHONEMEANALYZER_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_PHONEMEANALYZER_H
#pragma once

//////////////////////////////////////////////////////////////////////////
struct ILipSyncPhonemeRecognizer
{
public:
	struct SPhoneme
	{
		enum { MAX_PHONEME_LENGTH = 8 };

		int startTime;
		int endTime;
		int	nPhonemeCode;
		char sPhoneme[MAX_PHONEME_LENGTH];
		float intensity;
	};

	struct SWord
	{
		int startTime;
		int endTime;
		char *sWord;
	};

	struct SSentance
	{
		char *sSentence;

		int nWordCount;
		SWord* pWords; // Array of words.

		int nPhonemeCount;
		SPhoneme* pPhonemes; // Array of phonemes.
	};


	virtual void Release() = 0;
	virtual bool RecognizePhonemes( const char *wavfile,const char *text,SSentance** pOutSetence ) = 0;
	virtual const char* GetLastError() = 0;
};


//////////////////////////////////////////////////////////////////////////
class CPhonemesAnalyzer
{
public:
	CPhonemesAnalyzer();
	~CPhonemesAnalyzer();

	// Analyze wav file and extract phonemes out of it.
	bool Analyze(  const char *wavfile,const char *text,ILipSyncPhonemeRecognizer::SSentance** pOutSetence );
	const char* GetLastError();

private:
	HMODULE m_hDLL;
	ILipSyncPhonemeRecognizer* m_pPhonemeParser;
	string m_LastError;
};

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_PHONEMEANALYZER_H
