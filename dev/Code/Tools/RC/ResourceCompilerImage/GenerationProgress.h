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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_GENERATIONPROGRESS_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_GENERATIONPROGRESS_H
#pragma once

class CImageCompiler;

class CGenerationProgress
{
public:

    CGenerationProgress(CImageCompiler& rImageCompiler);

    float Get();
    void Start();
    void Finish();

    void Phase1();
    void Phase2(const uint32 dwY, const uint32 dwHeight);
    void Phase3(const uint32 dwY, const uint32 dwHeight, const int iTemp);
    void Phase4(const uint32 dwMip, const int iBlockLines);

private: // ------------------------------------------------------------------

    void Set(float fProgress);
    void Increment(float fProgress);

    float                                   m_fProgress;
    CImageCompiler&             m_rImageCompiler;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_GENERATIONPROGRESS_H
