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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_COMPRESSION_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_COMPRESSION_H
#pragma once



#include "Wavelet.h"
#include "DaubechiWavelet.h"

//int Compress(CWaveletData &, int* &, int=0, int=0,
//                       const double=1., const double=1., int=10, int=10);


typedef std::vector<int> TCompressetWaveletData;


namespace Wavelet
{
    int Compress(CWaveletData& in, TCompressetWaveletData& out, int Lwt, int Lbt,
        const TWavereal g1, const TWavereal g2, int np1, int np2);

    int UnCompress(TCompressetWaveletData& in, CWaveletData& out);

    // Compression uses DCT
    int CompressDCT(CWaveletData& in, TCompressetWaveletData& out, const TWavereal g);
    // decompression uses DCT
    int UnCompressDCT(TCompressetWaveletData& in, CWaveletData& out);
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_COMPRESSION_H
