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

// Description : Implementation of simple raw wave file reader

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_AUDIO_WAVEFILEREADER_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_AUDIO_WAVEFILEREADER_H
#pragma once


#include <IStreamEngine.h>

class CWaveFileReader
{
public:

	CWaveFileReader();
	~CWaveFileReader(void);

	bool LoadFile(const char *sFileName);
	int32	GetSample(uint32 nPos);
	float GetSampleNormalized(uint32 nPos);
	void  GetSamplesMinMax( int nPos,int nSamplesCount,float &vmin,float &vmax );
	bool IsLoaded(){ return m_bLoaded; };
	void SetLoaded(bool bLoaded){ m_bLoaded = bLoaded; };

protected:
	void* LoadAsSample(const char *AssetDataPtrOrName, int nLength);

	// Closes down Stream or frees memory of the Sample
	void	DestroyData();

	CMemoryBlock		m_MemBlock;
	bool				m_bLoaded;
	uint32				m_nVolume;
};

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_AUDIO_WAVEFILEREADER_H
