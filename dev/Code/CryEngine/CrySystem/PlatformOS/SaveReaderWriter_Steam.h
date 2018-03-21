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

#ifndef __SAVE_READER_WRITER_STEAM_H__
#define __SAVE_READER_WRITER_STEAM_H__

#if USE_STEAM

#include <IPlatformOS.h>

class CSaveReader_Steam
    : public IPlatformOS::ISaveReader
{
public:
    CSaveReader_Steam(const char* fileName);

    // ISaveReader
    virtual IPlatformOS::EFileOperationCode Seek(long seek, ESeekMode mode);
    virtual IPlatformOS::EFileOperationCode GetFileCursor(long& fileCursor);
    virtual IPlatformOS::EFileOperationCode ReadBytes(void* data, size_t numBytes);
    virtual IPlatformOS::EFileOperationCode GetNumBytes(size_t& numBytes);
    virtual IPlatformOS::EFileOperationCode Close();
    virtual IPlatformOS::EFileOperationCode LastError() const { return m_lastError; }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void TouchFile();
    //~ISaveReader
private:
    string m_fileName;
    std::vector<uint8> m_data;
    IPlatformOS::EFileOperationCode m_lastError;
    uint32 m_filePointer;
};

DECLARE_SMART_POINTERS(CSaveReader_Steam);

class CSaveWriter_Steam
    : public IPlatformOS::ISaveWriter
{
public:
    CSaveWriter_Steam(const char* fileName);

    // ISaveWriter
    virtual IPlatformOS::EFileOperationCode AppendBytes(const void* data, size_t length);
    virtual IPlatformOS::EFileOperationCode Close();
    virtual IPlatformOS::EFileOperationCode LastError() const { return m_lastError; }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    //~ISaveWriter

private:
    IPlatformOS::EFileOperationCode m_lastError;
    string m_fileName;
    std::vector<uint8> m_data;
    uint32 m_filePos;
};

DECLARE_SMART_POINTERS(CSaveWriter_Steam);


class CFileFinderSteam
    : public IPlatformOS::IFileFinder
{
public:
    CFileFinderSteam();
    virtual EFileState      FileExists(const char* path);
    virtual intptr_t        FindFirst(const char* filePattern, _finddata_t* fd);
    virtual int             FindNext(intptr_t handle, _finddata_t* fd);
    virtual int             FindClose(intptr_t handle);
    virtual void            GetMemoryUsage(ICrySizer* pSizer) const { pSizer->Add(*this); }

private:
    struct SFileInfo
    {
        SFileInfo(string name, uint32 size, uint32 flags = FILE_ATTRIBUTE_NORMAL)
        {
            m_name = name;
            m_size = size;
            m_flags = flags;
        }

        string m_name;
        uint32 m_size;
        uint32 m_flags;
    };

    struct SFindResult
    {
        std::vector<SFileInfo> m_files;
        int m_currentIteratorIdx;
        string m_searchPath;
    };

    void FillFindData(uint32 handle, _finddata_t* fd);

private:
    std::vector<SFileInfo> m_files;
    std::map<uint32, SFindResult> m_matchingFiles;
    int32 m_nextFileHandle;
};


#endif // USE_STEAM

#endif //__SAVE_READER_WRITER_STEAM_H__