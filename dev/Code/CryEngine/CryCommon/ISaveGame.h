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

#ifndef CRYINCLUDE_CRYACTION_ISAVEGAME_H
#define CRYINCLUDE_CRYACTION_ISAVEGAME_H
#pragma once

#include "SerializeFwd.h"

struct ISaveGame
{
    virtual ~ISaveGame(){}
    // initialize - set output path
    virtual bool Init(const char* name) = 0;

    // set some basic meta-data
    virtual void AddMetadata(const char* tag, const char* value) = 0;
    virtual void AddMetadata(const char* tag, int value) = 0;
    // create a serializer for some data section
    virtual TSerialize AddSection(const char* section) = 0;
    // set a thumbnail.
    // if imageData == 0: only reserves memory and returns ptr to local data
    // if imageData != 0: copies data from imageData into local buffer
    // imageData is in BGR or BGRA
    // returns ptr to internal data storage (size=width*height*depth) if Thumbnail supported,
    // 0 otherwise
    virtual uint8* SetThumbnail(const uint8* imageData, int width, int height, int depth) = 0;

    // set a thumbnail from an already present bmp file
    // file will be read on function call
    // returns true if successful, false otherwise
    virtual bool SetThumbnailFromBMP(const char* filename) = 0;

    // finish - indicate success (negative success *must* remove file)
    // also calls delete this;
    virtual bool Complete(bool successfulSoFar) = 0;

    // returns the filename of this savegame
    virtual const char* GetFileName() const = 0;

    // save game reason
    virtual void SetSaveGameReason(ESaveGameReason reason) = 0;
    virtual ESaveGameReason GetSaveGameReason() const = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
};

#endif // CRYINCLUDE_CRYACTION_ISAVEGAME_H
