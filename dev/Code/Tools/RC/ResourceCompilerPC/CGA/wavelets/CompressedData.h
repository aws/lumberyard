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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_COMPRESSEDDATA_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_COMPRESSEDDATA_H
#pragma once



#include "Wavelet.h"

class CWaveRDC
    : public CWaveletData
{
public:


    CWaveRDC();

    virtual ~CWaveRDC();


    virtual int DumpZ(const char*, int = 0);

    void ResizeZ(int n);

    int Compress(const CWaveletData&, bool fast = false);
    //{
    //  return 1;
    //}

    int UnCompress(CWaveletData&, int level = 1);
    //{
    //  return 1;
    //}

    void Dir(int v = 1);

private:

    // push data to compress stream
    int Push(int*, int, unsigned int*, int&, int, int);
    void Push(unsigned int&, unsigned int*, int&, int);
    int  Pop(int*, int, int&, int, int);
    void Pop(unsigned int&, int&, int);
    inline int wabs(int& i){return i > 0 ? i : -i; }

public:
    int m_nLayers;      // number of layers in compressed array dataz
    int m_Options;      // current layer compression options
    std::vector<unsigned int> m_DataZ; // compressed data array

private:
    // number of 32-bits layer service words (lsw)
    int m_nLSW;
    // free bits in the last word of current block
    int m_nFreeBits;
    // current layer encoding bit length for large integers
    int m_nLargeInts;
    // current layer encoding bit length for small integers
    int m_nSmallInts;
    // length of the Block Service Word
    int m_nBSW;
    // current layer shift subtracted from original data
    short int m_iShift;
    // number that encodes 0;
    short int m_iZero;
};



#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_COMPRESSEDDATA_H
