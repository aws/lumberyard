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

#ifndef CRYINCLUDE_EDITOR_UTIL_PAKFILE_H
#define CRYINCLUDE_EDITOR_UTIL_PAKFILE_H
#pragma once

// forward references.
struct ICryArchive;
TYPEDEF_AUTOPTR(ICryArchive);

#include "CryMemFile.h"         // CCryMemFile

/*! CPakFile Wraps game implementation of ICryArchive.
        Used for storing multiple files into zip archive file.
*/
class SANDBOX_API CPakFile
{
public:
    CPakFile();
    CPakFile(ICryPak* pCryPak);
    ~CPakFile();
    //! Opens archive for writing.
    explicit CPakFile(const char* filename);
    //! Opens archive for writing.
    bool Open(const char* filename, bool bAbsolutePath = true);
    //! Opens archive for reading only.
    bool OpenForRead(const char* filename);

    void Close();
    //! Adds or update file in archive.
    bool UpdateFile(const char* filename, CCryMemFile& file, bool bCompress = true);
    //! Adds or update file in archive.
    bool UpdateFile(const char* filename, CMemoryBlock& mem, bool bCompress = true, int nCompressLevel = ICryArchive::LEVEL_BETTER);
    //! Adds or update file in archive.
    bool UpdateFile(const char* filename, void* pBuffer, int nSize, bool bCompress = true, int nCompressLevel = ICryArchive::LEVEL_BETTER);
    //! Remove file from archive.
    bool RemoveFile(const char* filename);
    //! Remove dir from archive.
    bool RemoveDir(const char* directory);

    //! Return archive of this pak file wrapper.
    ICryArchive* GetArchive() { return m_pArchive; };
private:
    ICryArchive_AutoPtr m_pArchive;
    ICryPak* m_pCryPak;
};

#endif // CRYINCLUDE_EDITOR_UTIL_PAKFILE_H
