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

#ifndef CRYINCLUDE_CRYACTION_ILOADGAME_H
#define CRYINCLUDE_CRYACTION_ILOADGAME_H
#pragma once

struct ILoadGame
{
    virtual ~ILoadGame(){}
    // initialize - set name of game
    virtual bool Init(const char* name) = 0;

    virtual IGeneralMemoryHeap* GetHeap() = 0;

    // get some basic meta-data
    virtual const char* GetMetadata(const char* tag) = 0;
    virtual bool GetMetadata(const char* tag, int& value) = 0;
    virtual bool HaveMetadata(const char* tag) = 0;
    // create a serializer for some data section
    virtual std::unique_ptr<TSerialize> GetSection(const char* section) = 0;
    virtual bool HaveSection(const char* section) = 0;

    // finish - indicate success (negative success *must* remove file)
    // also calls delete this;
    virtual void Complete() = 0;

    // returns the filename of this savegame
    virtual const char* GetFileName() const = 0;
};

#endif // CRYINCLUDE_CRYACTION_ILOADGAME_H
