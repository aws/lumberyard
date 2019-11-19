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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_NOISE_H
#define CRYINCLUDE_EDITOR_TERRAIN_NOISE_H
#pragma once


class CDynamicArray2D;

// Parameter structures
struct SNoiseParams
{
    unsigned int iWidth;
    unsigned int iHeight;
    unsigned int iCover;
    unsigned int iPasses;
    float fFrequencyStep;
    float fFrequency;
    unsigned int iSmoothness;
    unsigned int iRandom;
    float fFade;
    float iSharpness;

    // Note: Only used by the Cloud Generator algorithm, to color the background pixels blue.
    // TODO: Remove this; does not belong here
    bool bBlueSky;

    // Used internally to verify serialized data, no need to set it from outside the class
    // TODO: Remove this; does not belong here
    bool bValid;
};

// Basis matrix for spline interpolation
#define CR00    -0.5f
#define CR01    1.5f
#define CR02    -1.5f
#define CR03    0.5f
#define CR10    1.0f
#define CR11    -2.5f
#define CR12    2.0f
#define CR13    -0.5f
#define CR20    -0.5f
#define CR21    0.0f
#define CR22    0.5f
#define CR23    0.0f
#define CR30    0.0f
#define CR31    1.0f
#define CR32    0.0f
#define CR33    0.0f

class CNoise
{
public:
    CNoise();
    virtual ~CNoise();

    void FracSynthPass(CDynamicArray2D* hBuf, float freq, float zscale, int xres, int zres, bool bLoop);
    float Spline(float x, /*int nknots,*/ float* knot);
};

#endif // CRYINCLUDE_EDITOR_TERRAIN_NOISE_H
