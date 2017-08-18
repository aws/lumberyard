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

#include "StdAfx.h"
#include "PhonemeAnalyzer.h"
#include <CryLibrary.h>

typedef ILipSyncPhonemeRecognizer* (*CreatePhonemeParserFunc)();

//////////////////////////////////////////////////////////////////////////
CPhonemesAnalyzer::CPhonemesAnalyzer()
{
	m_hDLL = 0;
	m_pPhonemeParser = 0;
}

//////////////////////////////////////////////////////////////////////////
CPhonemesAnalyzer::~CPhonemesAnalyzer()
{
	if (m_pPhonemeParser)
		m_pPhonemeParser->Release();
	if (m_hDLL)
		FreeLibrary(m_hDLL);
}

//////////////////////////////////////////////////////////////////////////
bool CPhonemesAnalyzer::Analyze(  const char *wavfile,const char *text,ILipSyncPhonemeRecognizer::SSentance** pOutSetence )
{
	if (m_pPhonemeParser)
		m_pPhonemeParser->Release();

	CString strText = text;
	strText.Replace( '\r',' ' );
	strText.Replace( '\n',' ' );
	strText.Replace( '\t',' ' );

	bool bRes = false;
	WCHAR szCurDir[1024];
	GetCurrentDirectoryW(sizeof(szCurDir),szCurDir);
	SetCurrentDirectoryW( L"Editor\\Plugins\\LipSync\\Annosoft" );
	m_hDLL = CryLoadLibrary( "LipSync_Annosoft.dll" );
	if (m_hDLL)
	{
		CreatePhonemeParserFunc func = (CreatePhonemeParserFunc)::GetProcAddress(m_hDLL,"CreatePhonemeParser");
		if (func)
		{
			m_pPhonemeParser = func();

			// Find the full path.
			CStringW currentDirectory = szCurDir;
			if (!(currentDirectory.GetLength() > 0 && (currentDirectory[currentDirectory.GetLength() - 1] == '\\' || currentDirectory[currentDirectory.GetLength() - 1] == '/')))
				currentDirectory.Append(L"\\");
			CStringW fullPath = currentDirectory + wavfile;
			char fullPathASCIIBuffer[2048] = "";
			WideCharToMultiByte(CP_ACP, 0, fullPath.GetString(), fullPath.GetLength(), fullPathASCIIBuffer, sizeof(fullPathASCIIBuffer) / sizeof(fullPathASCIIBuffer[0]), 0, 0);

			bRes = m_pPhonemeParser->RecognizePhonemes( fullPathASCIIBuffer,strText,pOutSetence );
			m_LastError = m_pPhonemeParser->GetLastError();
		}
	}
	// Restore current directory.
	SetCurrentDirectoryW( szCurDir );
	return bRes;
}

//////////////////////////////////////////////////////////////////////////
const char* CPhonemesAnalyzer::GetLastError()
{
	return m_LastError;
}
