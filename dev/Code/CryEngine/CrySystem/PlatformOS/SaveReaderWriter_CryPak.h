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

// Description : Implementation of the ISaveReader and ISaveWriter
//               interfaces using CryPak

#ifndef CRYINCLUDE_CRYSYSTEM_PLATFORMOS_SAVEREADERWRITER_CRYPAK_H
#define CRYINCLUDE_CRYSYSTEM_PLATFORMOS_SAVEREADERWRITER_CRYPAK_H
#pragma once


#include <IPlatformOS.h>

class CCryPakFile
{
protected:
    CCryPakFile(const char* fileName, const char* szMode);
    virtual ~CCryPakFile();

    IPlatformOS::EFileOperationCode CloseImpl();

    std::vector<uint8> m_data;
    string m_fileName;
    AZ::IO::HandleType m_fileHandle;
    size_t m_filePos;
    IPlatformOS::EFileOperationCode m_eLastError;
    bool m_bWriting;
    static const int s_dataBlockSize = 128 * 1024;

    NO_INLINE_WEAK static AZ::IO::HandleType FOpen(const char* pName, const char* mode, unsigned nFlags = 0);
    NO_INLINE_WEAK static int    FClose(AZ::IO::HandleType fileHandle);
    NO_INLINE_WEAK static size_t FGetSize(AZ::IO::HandleType fileHandle);
    NO_INLINE_WEAK static size_t FReadRaw(void* data, size_t length, size_t elems, AZ::IO::HandleType fileHandle);
    NO_INLINE_WEAK static size_t FWrite(const void* data, size_t length, size_t elems, AZ::IO::HandleType fileHandle);
    NO_INLINE_WEAK static size_t FSeek(AZ::IO::HandleType fileHandle, long offset, int mode);

private:
    CDebugAllowFileAccess m_allowFileAccess;
};

class CSaveReader_CryPak
    : public IPlatformOS::ISaveReader
    , public CCryPakFile
{
public:
    CSaveReader_CryPak(const char* fileName);

    // ISaveReader
    virtual IPlatformOS::EFileOperationCode Seek(long seek, ESeekMode mode);
    virtual IPlatformOS::EFileOperationCode GetFileCursor(long& fileCursor);
    virtual IPlatformOS::EFileOperationCode ReadBytes(void* data, size_t numBytes);
    virtual IPlatformOS::EFileOperationCode GetNumBytes(size_t& numBytes);
    virtual IPlatformOS::EFileOperationCode Close() { return CloseImpl(); }
    virtual IPlatformOS::EFileOperationCode LastError() const { return m_eLastError; }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void TouchFile();
    //~ISaveReader
};

DECLARE_SMART_POINTERS(CSaveReader_CryPak);

class CSaveWriter_CryPak
    : public IPlatformOS::ISaveWriter
    , public CCryPakFile
{
public:
    CSaveWriter_CryPak(const char* fileName);

    // ISaveWriter
    virtual IPlatformOS::EFileOperationCode AppendBytes(const void* data, size_t length);
    virtual IPlatformOS::EFileOperationCode Close() { return CloseImpl(); }
    virtual IPlatformOS::EFileOperationCode LastError() const { return m_eLastError; }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    //~ISaveWriter
};

DECLARE_SMART_POINTERS(CSaveWriter_CryPak);

#endif // CRYINCLUDE_CRYSYSTEM_PLATFORMOS_SAVEREADERWRITER_CRYPAK_H
