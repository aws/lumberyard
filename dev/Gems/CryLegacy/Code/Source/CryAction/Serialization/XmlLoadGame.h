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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLLOADGAME_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLLOADGAME_H
#pragma once

#include "ILoadGame.h"

class CXmlLoadGame
    : public ILoadGame
{
public:
    CXmlLoadGame();
    virtual ~CXmlLoadGame();

    // ILoadGame
    virtual bool Init(const char* name);
    virtual IGeneralMemoryHeap* GetHeap() { return NULL; }
    virtual const char* GetMetadata(const char* tag);
    virtual bool GetMetadata(const char* tag, int& value);
    virtual bool HaveMetadata(const char* tag);
    virtual AZStd::unique_ptr<TSerialize> GetSection(const char* section);
    virtual bool HaveSection(const char* section);
    virtual void Complete();
    virtual const char* GetFileName() const;
    // ~ILoadGame

protected:
    bool Init(const XmlNodeRef& root, const char* fileName);

private:
    struct Impl;
    AZStd::unique_ptr<Impl> m_pImpl;
};

#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLLOADGAME_H
