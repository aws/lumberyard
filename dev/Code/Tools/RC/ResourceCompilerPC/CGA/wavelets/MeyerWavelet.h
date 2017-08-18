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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_MEYERWAVELET_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_MEYERWAVELET_H
#pragma once



#include "Wavelet.h"


// Daubechi wavelet realization
class CMeyerDWT
    : public CDWT
{
public:

    //ctors
    CMeyerDWT(bool tree);
    CMeyerDWT(int num, bool tree);
    // num - number of samples, tree - tree partitioning
    //  CMeyerDWT(int num, int ndw, bool tree);
    // num - number of samples, ndw - type of wavelet, tree - tree partitioning, border - border type
    //  CMeyerDWT(int num ,int ndw,bool tree,EBorderBehavior border);

    // One step of decomposition
    virtual void Decompose(int, int);
    // One step of reconstruction
    virtual void Reconstruct(int, int);

    //private:

    void    Init(int);

    //private:
    TWavereal* m_Filter;
};


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_MEYERWAVELET_H
