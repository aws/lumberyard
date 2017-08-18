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

#include "StdAfx.h"
#include "WaveFileReader.h"
#include "WaveFile.h"
#include "IStreamEngine.h"

//////////////////////////////////////////////////////////////////////////
CWaveFileReader::CWaveFileReader()
{
	// Audio: is this class still needed?
	m_bLoaded=false;
}

//////////////////////////////////////////////////////////////////////////
CWaveFileReader::~CWaveFileReader()
{
	DestroyData();
}

bool CWaveFileReader::LoadFile(const char *sFileName)
{
	DestroyData();
	SetLoaded(false);

	CCryFile file;
	if (!file.Open( sFileName,"rb "))
		return false;

	int filesize = file.GetLength();
	char *buf = new char[filesize+16];

	file.ReadRaw( buf,filesize );

	void* pSample = LoadAsSample( buf,filesize );
	if (!pSample)
	{
		delete []buf;
		return false;
	}		

	SetLoaded(true);

	return true;
}

//////////////////////////////////////////////////////////////////////////
void* CWaveFileReader::LoadAsSample(const char *AssetDataPtr, int nLength)
{
	ATG::WaveFileMemory MemWaveFile;
	ATG::WAVEFORMATEXTENSIBLE wfx;
	DWORD dwNewSize;
	//void* pSoundData = NULL;

	// Initialize the WaveFile Class and read all the chunks
	MemWaveFile.Init(AssetDataPtr, nLength);
	MemWaveFile.GetFormat(&wfx);

	// Create new memory for the sound data
	MemWaveFile.GetDuration( &dwNewSize );

	if (!m_MemBlock.Allocate(nLength))
		return NULL;

	// read the sound data from the file
	MemWaveFile.ReadSample(0, m_MemBlock.GetBuffer(), dwNewSize, &dwNewSize);

	return (m_MemBlock.GetBuffer());
}

//void SwapTyp( uint16 &x )
//{
//	x = (x<<8) | (x>>8);
//}

int32 CWaveFileReader::GetSample(uint32 nPos)
{
	int32 nData = 0;
			
	return nData;
}

//////////////////////////////////////////////////////////////////////////
f32 CWaveFileReader::GetSampleNormalized(uint32 nPos)
{

	int32 nData = GetSample(nPos);
	f32 fData = 0;

	return fData;
}

//////////////////////////////////////////////////////////////////////////
void  CWaveFileReader::GetSamplesMinMax( int nPos,int nSamplesCount,float &vmin,float &vmax )
{
}

//////////////////////////////////////////////////////////////////////////
void CWaveFileReader::DestroyData()
{
	m_MemBlock.Free();

	m_nVolume = 0;
	m_bLoaded = 0;
}
