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

#ifndef __PROCEDURAL_CLIP_FACTORY__H__
#define __PROCEDURAL_CLIP_FACTORY__H__

#include "ICryMannequinProceduralClipFactory.h"

class CProceduralClipFactory
    : public IProceduralClipFactory
{
public:
    CProceduralClipFactory();
    virtual ~CProceduralClipFactory();

    // IProceduralClipFactory
    virtual const char* FindTypeName(const THash& typeNameHash) const;

    virtual size_t GetRegisteredCount() const;
    virtual THash GetTypeNameHashByIndex(const size_t index) const;
    virtual const char* GetTypeNameByIndex(const size_t index) const;

    virtual bool Register(const char* const typeName, const SProceduralClipFactoryRegistrationInfo& registrationInfo);

    virtual IProceduralParamsPtr CreateProceduralClipParams(const char* const typeName) const;
    virtual IProceduralParamsPtr CreateProceduralClipParams(const THash& typeNameHash) const;

    virtual IProceduralClipPtr CreateProceduralClip(const char* const typeName) const;
    virtual IProceduralClipPtr CreateProceduralClip(const THash& typeNameHash) const;
    // ~IProceduralClipFactory

private:
    const SProceduralClipFactoryRegistrationInfo* FindRegistrationInfo(const THash& typeNameHash) const;

private:
    typedef VectorMap< THash, string > THashToNameMap;
    THashToNameMap m_typeHashToName;

    typedef VectorMap< THash, SProceduralClipFactoryRegistrationInfo > TTypeHashToRegistrationInfoMap;
    TTypeHashToRegistrationInfoMap m_typeHashToRegistrationInfo;
};

#endif