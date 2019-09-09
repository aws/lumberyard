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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_IMAGECONVERTOR_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_IMAGECONVERTOR_H
#pragma once

#include "IConvertor.h"  // IConvertor
#include "StlUtils.h"    // stl::less_stricmp

struct IResourceCompiler;

class CImageConvertor
    : public IConvertor
{
public:
    // Maps aliases of presets to real names of presets
    typedef std::map<string, string, stl::less_stricmp<string> > PresetAliases;

public:
    CImageConvertor(IResourceCompiler* pRC);
    virtual ~CImageConvertor();

    // interface IConvertor ----------------------------------------------------
    virtual void Release();
    virtual void Init(const ConvertorInitContext& context);
    virtual ICompiler* CreateCompiler();
    virtual const char* GetExt(int index) const;
    // -------------------------------------------------------------------------

private:
    PresetAliases m_presetAliases;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_IMAGECONVERTOR_H
