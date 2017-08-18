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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLSAVEGAME_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLSAVEGAME_H
#pragma once

#include "ISaveGame.h"

class CXmlSaveGame
    : public ISaveGame
{
public:
    CXmlSaveGame();
    virtual ~CXmlSaveGame();

    // ISaveGame
    virtual bool Init(const char* name);
    virtual void AddMetadata(const char* tag, const char* value);
    virtual void AddMetadata(const char* tag, int value);
    virtual uint8* SetThumbnail(const uint8* imageData, int width, int height, int depth);
    virtual bool SetThumbnailFromBMP(const char* filename);
    virtual TSerialize AddSection(const char* section);
    virtual bool Complete(bool successfulSoFar);
    virtual const char* GetFileName() const;
    virtual void SetSaveGameReason(ESaveGameReason reason) { m_eReason = reason; }
    virtual ESaveGameReason GetSaveGameReason() const { return m_eReason; }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    // ~ISaveGame

protected:
    virtual bool Write(const char* filename, XmlNodeRef data);
    XmlNodeRef GetMetadataXmlNode() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
    ESaveGameReason m_eReason;
};

#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLSAVEGAME_H
