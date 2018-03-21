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
#include "stdafx.h"
#include "FileClient.h"
#include <CryCrc32.h>
#include <functional>
#include "IEngineConnection.h"
#include "CrySocks.h"
#include "IResourceCompilerHelper.h"
#include "CryEndian.h"

#include <IPlatformOS.h>
#include "StringUtils.h"

//#define FILE_CLIENT_LOGGING

#define FILE_DESC_MAX_PATH 260

//////////////////////////////////////////////////////////////////////////
class Serializable
{
public:
    virtual size_t numBytes() const = 0;

    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) = 0;
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) = 0;

    virtual void readFile(FILE* pFile, bool bSwapEndian = eLittleEndian)
    {
        size_t sizeBytes = numBytes();
        char* buffer = new char[numBytes()];
        if (fread(buffer, 1, sizeBytes, pFile) != sizeBytes)
        {
            CryFatalError("File Read Error");
        }
        read(buffer, sizeBytes, bSwapEndian);
        delete buffer;
    }

    virtual void writeFile(FILE* pFile, bool bSwapEndian = eLittleEndian)
    {
        size_t sizeBytes = numBytes();
        char* buffer = new char[sizeBytes];
        write(buffer, sizeBytes, bSwapEndian);
        if (fwrite(buffer, 1, sizeBytes, pFile) != sizeBytes)
        {
            CryFatalError("File Write Error");
        }
        delete buffer;
    }

    virtual void append(std::vector<char>& dBuffer, bool bSwapEndian = eLittleEndian)
    {
        size_t size = dBuffer.size();
        dBuffer.resize(size + numBytes());
        write(dBuffer.data() + size, bSwapEndian);
    }
};

//////////////////////////////////////////////////////////////////////////
class FileDescMsg
    : public Serializable
{
public:
    char fileName[FILE_DESC_MAX_PATH + 1];
    uint32_t nAttrib;
    uint64_t nSize;
    uint64_t tAccess;
    uint64_t tCreate;
    uint64_t tWrite;

    FileDescMsg()
        : nAttrib(0)
        , nSize(0)
        , tAccess(0)
        , tCreate(0)
        , tWrite(0)
    {
        memset(fileName, 0, sizeof(fileName));
    }

    virtual size_t numBytes() const override
    {
        return sizeof(fileName) +
               sizeof(nAttrib) +
               sizeof(nSize) +
               sizeof(tAccess) +
               sizeof(tCreate) +
               sizeof(tWrite);
    }
    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        memcpy(fileName, buffer, sizeof(fileName));
        buffer += sizeof(fileName);

        uint32_t sAttrib;
        memcpy(&sAttrib, buffer, sizeof(sAttrib));
        buffer += sizeof(sAttrib);
        nAttrib = SwapEndianValue(sAttrib, bSwapEndian);

        uint64_t sSize;
        memcpy(&sSize, buffer, sizeof(sSize));
        buffer += sizeof(sSize);
        nSize = SwapEndianValue(sSize, bSwapEndian);

        uint64_t sAccess;
        memcpy(&sAccess, buffer, sizeof(sAccess));
        buffer += sizeof(sAccess);
        tAccess = SwapEndianValue(sAccess, bSwapEndian);

        uint64_t sCreate;
        memcpy(&sCreate, buffer, sizeof(sCreate));
        buffer += sizeof(sCreate);
        tCreate = SwapEndianValue(sCreate, bSwapEndian);

        uint64_t sWrite;
        memcpy(&sWrite, buffer, sizeof(sWrite));
        buffer += sizeof(sWrite);
        tWrite = SwapEndianValue(sWrite, bSwapEndian);

        return buffer;
    }
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        memcpy(buffer, fileName, sizeof(fileName));
        buffer += sizeof(fileName);

        uint32_t sAttrib = SwapEndianValue(nAttrib, bSwapEndian);
        memcpy(buffer, &sAttrib, sizeof(sAttrib));
        buffer += sizeof(sAttrib);

        uint64_t sSize = SwapEndianValue(nSize, bSwapEndian);
        memcpy(buffer, &sSize, sizeof(sSize));
        buffer += sizeof(sSize);

        uint64_t sAccess = SwapEndianValue(tAccess, bSwapEndian);
        memcpy(buffer, &sAccess, sizeof(sAccess));
        buffer += sizeof(sAccess);

        uint64_t sCreate = SwapEndianValue(tCreate, bSwapEndian);
        memcpy(buffer, &sCreate, sizeof(sCreate));
        buffer += sizeof(sCreate);

        uint64_t sWrite = SwapEndianValue(tWrite, bSwapEndian);
        memcpy(buffer, &sWrite, sizeof(sWrite));
        buffer += sizeof(sWrite);

        return buffer;
    }
};

void FileDescToFileDescMsg(const IFileClient::FileDesc& desc, FileDescMsg& msg, bool bSwapEndian = eLittleEndian)
{
    assert(desc.m_Name.length() < FILE_DESC_MAX_PATH);
    strncpy_s(msg.fileName, desc.m_Name.c_str(), desc.m_Name.length());
    msg.nAttrib = (uint32_t)SwapEndianValue(desc.nAttrib, bSwapEndian);
    msg.nSize = (uint64_t)SwapEndianValue(desc.nSize, bSwapEndian);
    msg.tAccess = (uint64_t)SwapEndianValue(desc.tAccess, bSwapEndian);
    msg.tCreate = (uint64_t)SwapEndianValue(desc.tCreate, bSwapEndian);
    msg.tWrite = (uint64_t)SwapEndianValue(desc.tWrite, bSwapEndian);
}

void FileDescMsgToFileDesc(const FileDescMsg& msg, IFileClient::FileDesc& desc, bool bSwapEndian = eLittleEndian)
{
    desc.m_Name = msg.fileName;
    desc.nAttrib = (unsigned int)SwapEndianValue(msg.nAttrib, bSwapEndian);
    desc.nSize = (size_t)SwapEndianValue(msg.nSize, bSwapEndian);
    desc.tAccess = (time_t)SwapEndianValue(msg.tAccess, bSwapEndian);
    desc.tCreate = (time_t)SwapEndianValue(msg.tCreate, bSwapEndian);
    desc.tWrite = (time_t)SwapEndianValue(msg.tWrite, bSwapEndian);
}

//////////////////////////////////////////////////////////////////////////
class FileInfoRequest
    : public Serializable
{
public:
    char path[FILE_DESC_MAX_PATH + 1];

    FileInfoRequest()
    {
        memset(path, 0, sizeof(path));
    }

    virtual size_t numBytes() const override
    {
        return sizeof(path);
    }
    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        (void)bSwapEndian;

        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        memcpy(path, buffer, sizeof(path));
        buffer += sizeof(path);
        return buffer;
    }
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        (void)bSwapEndian;

        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        memcpy(buffer, path, sizeof(path));
        buffer += sizeof(path);
        return buffer;
    }
};

//////////////////////////////////////////////////////////////////////////
class FileInfoResponse
    : public Serializable
{
public:
    int32_t pathCrc;
    FileDescMsg fileDesc;

    FileInfoResponse()
        : pathCrc(0)
    {
    }

    virtual size_t numBytes() const override
    {
        return sizeof(pathCrc) + fileDesc.numBytes();
    }
    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        int32_t sPathCrc;
        memcpy(&sPathCrc, buffer, sizeof(sPathCrc));
        buffer += sizeof(sPathCrc);
        pathCrc = SwapEndianValue(sPathCrc, bSwapEndian);
        return fileDesc.read(buffer, bSwapEndian);
    }
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        int32_t sPathCrc = SwapEndianValue(pathCrc, bSwapEndian);
        memcpy(buffer, &sPathCrc, sizeof(sPathCrc));
        buffer += sizeof(sPathCrc);
        return fileDesc.write(buffer, bSwapEndian);
    }
};

//////////////////////////////////////////////////////////////////////////
class FileDownloadRequest
    : public Serializable
{
public:
    char path[FILE_DESC_MAX_PATH + 1];

    FileDownloadRequest()
    {
        memset(path, 0, sizeof(path));
    }

    virtual size_t numBytes() const override
    {
        return sizeof(path);
    }
    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        (void)bSwapEndian;

        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        memcpy(&path, buffer, sizeof(path));
        buffer += sizeof(path);
        return buffer;
    }
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        (void)bSwapEndian;

        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        memcpy(buffer, &path, sizeof(path));
        buffer += sizeof(path);
        return buffer;
    }
};

//////////////////////////////////////////////////////////////////////////
class FileDownloadResponse
    : public Serializable
{
public:
    int32_t pathCrc;

    FileDownloadResponse()
        : pathCrc(0)
    {
    }

    virtual size_t numBytes() const override
    {
        return sizeof(pathCrc);
    }
    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        int32_t sPathCrc;
        memcpy(&sPathCrc, buffer, sizeof(sPathCrc));
        buffer += sizeof(sPathCrc);
        pathCrc = SwapEndianValue(sPathCrc, bSwapEndian);
        return buffer;
    }
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        int32_t sPathCrc = SwapEndianValue(pathCrc, bSwapEndian);
        memcpy(buffer, &sPathCrc, sizeof(sPathCrc));
        buffer += sizeof(sPathCrc);
        return buffer;
    }
};

//////////////////////////////////////////////////////////////////////////
class ScanFilesRequest
    : public Serializable
{
public:
    char searchString[FILE_DESC_MAX_PATH + 1];

    ScanFilesRequest()
    {
        memset(searchString, 0, sizeof(searchString));
    }

    virtual size_t numBytes() const override
    {
        return sizeof(searchString);
    }
    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        (void)bSwapEndian;

        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        memcpy(&searchString, buffer, sizeof(searchString));
        buffer += sizeof(searchString);
        return buffer;
    }
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        (void)bSwapEndian;

        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        memcpy(buffer, &searchString, sizeof(searchString));
        buffer += sizeof(searchString);
        return buffer;
    }
};

//////////////////////////////////////////////////////////////////////////
class ScanFilesResponse
    : public Serializable
{
public:
    int32_t searchCrc;
    uint32_t numFileDesc;
    std::vector<FileDescMsg> dFileDesc;

    ScanFilesResponse()
        : searchCrc(0)
        , numFileDesc(0)
    {
    }

    virtual size_t numBytes() const override
    {
        size_t numFilesDescBytes = 0;
        for (auto& item : dFileDesc)
        {
            numFilesDescBytes += item.numBytes();
        }

        return sizeof(searchCrc) + sizeof(numFileDesc) + numFilesDescBytes;
    }
    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        int32_t sSearchCrc;
        memcpy(&sSearchCrc, buffer, sizeof(sSearchCrc));
        bufferLength -= sizeof(sSearchCrc);
        buffer += sizeof(sSearchCrc);
        searchCrc = SwapEndianValue(sSearchCrc, bSwapEndian);

        uint32_t sNumFileDesc;
        memcpy(&sNumFileDesc, buffer, sizeof(sNumFileDesc));
        bufferLength -= sizeof(sNumFileDesc);
        buffer += sizeof(sNumFileDesc);
        numFileDesc = SwapEndianValue(sNumFileDesc, bSwapEndian);

        dFileDesc.resize(sNumFileDesc);

        for (uint32_t i = 0; i < numFileDesc; ++i)
        {
            buffer = dFileDesc[i].read(buffer, bufferLength, bSwapEndian);
            bufferLength -= dFileDesc[i].numBytes();
        }

        return buffer;
    }
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        int32_t sSearchCrc = SwapEndianValue(searchCrc, bSwapEndian);
        memcpy(buffer, &sSearchCrc, sizeof(sSearchCrc));
        bufferLength -= sizeof(sSearchCrc);
        buffer += sizeof(sSearchCrc);

        uint32_t sNumFileDesc = SwapEndianValue(numFileDesc, bSwapEndian);
        memcpy(buffer, &sNumFileDesc, sizeof(sNumFileDesc));
        bufferLength -= sizeof(sNumFileDesc);
        buffer += sizeof(sNumFileDesc);

        for (uint32_t i = 0; i < numFileDesc; ++i)
        {
            buffer = dFileDesc[i].write(buffer, bufferLength, bSwapEndian);
            bufferLength -= dFileDesc[i].numBytes();
        }

        return buffer;
    }
};

//////////////////////////////////////////////////////////////////////////
class FileChunk
    : public Serializable
{
public:
    int32_t pathCrc;
    uint32_t chunkSize;
    const char* chunk;

    FileChunk()
        : pathCrc(0)
        , chunkSize(0)
        , chunk(nullptr)
    {
    }

    virtual size_t numBytes() const override
    {
        return sizeof(pathCrc) + sizeof(chunkSize) + chunkSize;
    }
    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        int32_t sPathCrc;
        if (bufferLength < sizeof(sPathCrc))
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }
        memcpy(&sPathCrc, buffer, sizeof(sPathCrc));
        buffer += sizeof(sPathCrc);
        bufferLength -= sizeof(sPathCrc);
        pathCrc = SwapEndianValue(sPathCrc, bSwapEndian);

        uint32_t sChunkSize;
        if (bufferLength < sizeof(sChunkSize))
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }
        memcpy(&sChunkSize, buffer, sizeof(sChunkSize));
        buffer += sizeof(sChunkSize);
        bufferLength -= sizeof(sChunkSize);
        chunkSize = SwapEndianValue(sChunkSize, bSwapEndian);

        if (bufferLength < sChunkSize)
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }
        chunk = buffer;
        buffer += chunkSize;

        return buffer;
    }
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        int32_t sPathCrc = SwapEndianValue(pathCrc, bSwapEndian);
        if (bufferLength < sizeof(sPathCrc))
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }
        memcpy(buffer, &sPathCrc, sizeof(sPathCrc));
        buffer += sizeof(sPathCrc);
        bufferLength -= sizeof(sPathCrc);

        uint32_t sChunkSize = SwapEndianValue(chunkSize, bSwapEndian);
        if (bufferLength < sizeof(sChunkSize))
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }
        memcpy(buffer, &sChunkSize, sizeof(sChunkSize));
        buffer += sizeof(sChunkSize);
        bufferLength -= sizeof(sChunkSize);

        if (bufferLength < sChunkSize)
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }
        memcpy(buffer, chunk, chunkSize);
        buffer += chunkSize;

        return buffer;
    }
};

//////////////////////////////////////////////////////////////////////////
class RemoveResponse
    : public Serializable
{
public:
    int32_t crc;

    RemoveResponse()
        : crc(0)
    {
    }

    virtual size_t numBytes() const override
    {
        return sizeof(crc);
    }
    virtual const char* read(const char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        int32_t sCrc;
        memcpy(&sCrc, buffer, sizeof(sCrc));
        buffer += sizeof(sCrc);
        crc = SwapEndianValue(sCrc, bSwapEndian);
        return buffer;
    }
    virtual char* write(char* buffer, size_t bufferLength, bool bSwapEndian = eLittleEndian) override
    {
        if (bufferLength < numBytes())
        {
            CryFatalError("Buffer Overrun");
            return buffer;
        }

        int32_t sCrc = SwapEndianValue(crc, bSwapEndian);
        memcpy(buffer, &sCrc, sizeof(sCrc));
        buffer += sizeof(sCrc);
        return buffer;
    }
};

//////////////////////////////////////////////////////////////////////////
CFileClient::CFileClient()
{
    using namespace std::placeholders;  // for _1, _2, _3...

    //register message I want
    gEnv->pEngineConnection->AddTypeCallback(CCrc32::Compute("FileClient::FileInfoResponse"), std::bind(&CFileClient::processFileInfoResponse, this, _1, _2, _3));

    gEnv->pEngineConnection->AddTypeCallback(CCrc32::Compute("FileClient::FileDownloadResponse"), std::bind(&CFileClient::processFileDownloadResponse, this, _1, _2, _3));

    gEnv->pEngineConnection->AddTypeCallback(CCrc32::Compute("FileClient::ScanFilesResponse"), std::bind(&CFileClient::processScanFilesResponse, this, _1, _2, _3));

    gEnv->pEngineConnection->AddTypeCallback(CCrc32::Compute("FileClient::FileChunk"), std::bind(&CFileClient::processFileChunk, this, _1, _2, _3));

    gEnv->pEngineConnection->AddTypeCallback(CCrc32::Compute("FileClient::RemoveFileInfo"), std::bind(&CFileClient::processRemoveInfo, this, _1, _2, _3));

    gEnv->pEngineConnection->AddTypeCallback(CCrc32::Compute("FileClient::RemoveDone"), std::bind(&CFileClient::processRemoveDone, this, _1, _2, _3));

    gEnv->pEngineConnection->AddTypeCallback(CCrc32::Compute("FileClient::ClearCache"), std::bind(&CFileClient::processClearCache, this, _1, _2, _3));

    //reconstruct the cache
    readEngineInfo();
}

CFileClient::~CFileClient()
{
}

//make sure we have the file info for this file, if not query the server for it
void CFileClient::DownloadFileInfo(const char* path, bool bForce)
{
    char rootPath[FILE_DESC_MAX_PATH];
    CryGetCurrentDirectory(sizeof(rootPath), rootPath);
    std::string strRootPath = rootPath;
    std::transform(strRootPath.begin(), strRootPath.end(), strRootPath.begin(), ::tolower);

    std::string thePath = path;
    std::transform(thePath.begin(), thePath.end(), thePath.begin(), ::tolower);

    int32_t pos = (int32_t)thePath.find(strRootPath);
    if (pos != (int32_t)std::string::npos)
    {
        thePath.erase(0, strRootPath.length() + 1);
    }

    int32_t pathCrc = (int32_t)CCrc32::ComputeLowercase(thePath.c_str());

    //lets see if we queried this file already in this session
    {
        CryAutoLock<CryMutex> lock(m_fileInfoProtector);
        if (bForce)
        {
            auto iter = m_fileInfo.find(pathCrc);
            if (iter != m_fileInfo.end())
            {
                m_fileInfo.erase(iter);
            }
        }
        if (m_fileInfo.find(pathCrc) != m_fileInfo.end())
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::DownloadFileInfo( %s : %d) Already Downloaded", path, pathCrc);
#endif
            return;
        }
    }

    std::shared_ptr<MultiWait> pMultiWait = nullptr;

    //wait if we are in progress else request from the server
    {
        //lock
        CryAutoLock<CryMutex> lock(m_fileInfoInProgressProtector);

        auto inProgressIter = m_fileInfoInProgress.find(pathCrc);
        if (inProgressIter != m_fileInfoInProgress.end())
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::DownloadFileInfo( %s : %d ) Already in Progress... Waiting", path, pathCrc);
#endif
            //get a reference to the shared ptr
            pMultiWait = inProgressIter->second;
            pMultiWait->m_mutex.Lock();
        }
        else
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::DownloadFileInfo( %s : %d ) Sending request... Waiting", path, pathCrc);
#endif
            //create a new multi wait and lock it
            pMultiWait = std::shared_ptr<MultiWait>(new MultiWait, [](MultiWait* s) { delete s; });
            pMultiWait->m_mutex.Lock();

            //insert this into the map
            m_fileInfoInProgress.insert(std::make_pair(pathCrc, pMultiWait));

            //create the request
            FileInfoRequest req;
            if (thePath.size() > FILE_DESC_MAX_PATH)
            {
                //shouldn't get here... something went wrong...
                CryFatalError("Error");
            }
            strncpy_s(req.path, thePath.c_str(), thePath.size());

            //write the request to a buffer
            std::vector<char> dMsg;
            req.append(dMsg);

            //send the request
            if (!gEnv->pEngineConnection->SendMsg(CCrc32::Compute("FileServer::FileInfoRequest"), dMsg.data(), dMsg.size()))
            {
                //shouldn't get here... something went wrong...
                CryFatalError("Error");
            }
        }
    }

    //wait for the request to finish
    pMultiWait->m_condition.Wait(pMultiWait->m_mutex);

    //no longer in progress, see if it succeeded
    {
        CryAutoLock<CryMutex> lock(m_fileInfoProtector);
        auto iter = m_fileInfo.find(pathCrc);
        if (iter != m_fileInfo.end())
        {
#ifdef FILE_CLIENT_LOGGING
            if (iter->second.nAttrib == 0 &&
                iter->second.nSize == 0 &&
                iter->second.tAccess == 0 &&
                iter->second.tCreate == 0 &&
                iter->second.tWrite == 0)
            {
                CryLog("FileClient::DownloadFileInfo( %s : %d ) Done -> File Doesn't Exist", path, pathCrc);
            }
            else
            {
                CryLog("FileClient::DownloadFileInfo( %s : %d ) Done -> File Exists", path, pathCrc);
            }
#endif
            return;
        }
        else
        {
            //shouldn't get here... something went wrong...
            CryFatalError("Error");
        }
    }
}

//use the file info to determine if the file exists
bool CFileClient::FileExists(const char* path, bool bForce)
{
    char rootPath[FILE_DESC_MAX_PATH];
    CryGetCurrentDirectory(sizeof(rootPath), rootPath);
    std::string strRootPath = rootPath;
    std::transform(strRootPath.begin(), strRootPath.end(), strRootPath.begin(), ::tolower);

    std::string thePath = path;
    std::transform(thePath.begin(), thePath.end(), thePath.begin(), ::tolower);

    int32_t pos = (int32_t)thePath.find(strRootPath);
    if (pos != (int32_t)std::string::npos)
    {
        thePath.erase(0, strRootPath.length() + 1);
    }

    int32_t pathCrc = (int32_t)CCrc32::ComputeLowercase(thePath.c_str());

    DownloadFileInfo(thePath.c_str(), bForce);
    {
        CryAutoLock<CryMutex> lock(m_fileInfoProtector);
        auto iter = m_fileInfo.find(pathCrc);
        if (iter != m_fileInfo.end())
        {
            if (iter->second.nAttrib == 0 &&
                iter->second.nSize == 0 &&
                iter->second.tAccess == 0 &&
                iter->second.tCreate == 0 &&
                iter->second.tWrite == 0)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
    }

    //should never get here
    return false;
}

//use file info to determine the file size
CFileClient::fileSize_t CFileClient::GetFileSize(const char* path, bool bForce)
{
    char rootPath[FILE_DESC_MAX_PATH];
    CryGetCurrentDirectory(sizeof(rootPath), rootPath);
    std::string strRootPath = rootPath;
    std::transform(strRootPath.begin(), strRootPath.end(), strRootPath.begin(), ::tolower);

    std::string thePath = path;
    std::transform(thePath.begin(), thePath.end(), thePath.begin(), ::tolower);

    int32_t pos = (int32_t)thePath.find(strRootPath);
    if (pos != (int32_t)std::string::npos)
    {
        thePath.erase(0, strRootPath.length() + 1);
    }

    int32_t pathCrc = (int32_t)CCrc32::ComputeLowercase(thePath.c_str());

    DownloadFileInfo(thePath.c_str(), bForce);
    {
        CryAutoLock<CryMutex> lock(m_fileInfoProtector);
        auto iter = m_fileInfo.find(pathCrc);
        if (iter != m_fileInfo.end())
        {
            if (iter->second.nAttrib == 0 &&
                iter->second.nSize == 0 &&
                iter->second.tAccess == 0 &&
                iter->second.tCreate == 0 &&
                iter->second.tWrite == 0)
            {
                return IFileClient::FILE_NOT_PRESENT;
            }
            else
            {
                return (CFileClient::fileSize_t)(iter->second.nSize);
            }
        }
    }

    //should never get here
    return IFileClient::FILE_NOT_PRESENT;
}

//make sure we have the file, if not download it from the file server
bool CFileClient::GetFile(const char* path, bool bForce)
{
    char rootPath[FILE_DESC_MAX_PATH];
    CryGetCurrentDirectory(sizeof(rootPath), rootPath);
    std::string strRootPath = rootPath;
    std::transform(strRootPath.begin(), strRootPath.end(), strRootPath.begin(), ::tolower);

    std::string thePath = path;
    std::transform(thePath.begin(), thePath.end(), thePath.begin(), ::tolower);

    int32_t pos = (int32_t)thePath.find(strRootPath);
    if (pos != (int32_t)std::string::npos)
    {
        thePath.erase(0, strRootPath.length() + 1);
    }

    int32_t pathCrc = (int32_t)CCrc32::ComputeLowercase(thePath.c_str());

    if (!FileExists(thePath.c_str(), bForce))
    {
        return false;
    }

    //are we downloaded
    {
        CryAutoLock<CryMutex> lock(m_fileDownloadedProtector);
        if (bForce)
        {
            auto iter = m_fileDownloaded.find(pathCrc);
            if (iter != m_fileDownloaded.end())
            {
                m_fileDownloaded.erase(iter);
            }
        }
        if (m_fileDownloaded.find(pathCrc) != m_fileDownloaded.end())
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::GetFile( %s : %d ) Already Downloaded", thePath.c_str(), pathCrc);
#endif
            return true;
        }
    }

    std::shared_ptr<MultiWaitFile> pMultiWaitFile = nullptr;

    //wait if we are in progress
    {
        //lock
        CryAutoLock<CryMutex> lock(m_fileDownloadInProgessProtector);

        auto inProgressIter = m_fileDownloadInProgess.find(pathCrc);
        if (inProgressIter != m_fileDownloadInProgess.end())
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::GetFile( %s : %d ) Already In Progess... Waiting", thePath.c_str(), pathCrc);
#endif
            //get a reference to the shared ptr
            pMultiWaitFile = inProgressIter->second;
            pMultiWaitFile->m_mutex.Lock();
        }
        else
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::GetFile( %s : %d ) Sending Request... Waiting", path, pathCrc);
#endif
            pMultiWaitFile = std::shared_ptr<MultiWaitFile>(new MultiWaitFile, [](MultiWaitFile* s) { delete s; });
            pMultiWaitFile->m_mutex.Lock();

            //create the file names
            pMultiWaitFile->m_fileName = thePath.c_str();

            std::string tempFileName = thePath.c_str();
            tempFileName += ".temp";

            //make sure the file structure exists
            RCPathUtil::CreateDirectoryPath(RCPathUtil::GetPath(thePath.c_str()).c_str());

            //open the file
            pMultiWaitFile->m_file = fopen(tempFileName.c_str(), "wb");
            if (!pMultiWaitFile->m_file)
            {
                //shouldn't get here... something went wrong...
                CryFatalError("Error");
            }

            //insert this into the map
            m_fileDownloadInProgess.insert(std::make_pair(pathCrc, pMultiWaitFile));

            //create the request
            FileDownloadRequest req;
            if (thePath.size() > FILE_DESC_MAX_PATH)
            {
                assert(false);//fatal?
            }
            strncpy_s(req.path, thePath.c_str(), thePath.size());

            //create the msg
            std::vector<char> dMsg;
            req.append(dMsg);

            //send the download request
            if (!gEnv->pEngineConnection->SendMsg(CCrc32::Compute("FileServer::FileDownloadRequest"), dMsg.data(), dMsg.size()))
            {
                //shouldn't get here... something went wrong...
                CryFatalError("Error");
            }
        }
    }

    //wait till done
    pMultiWaitFile->m_condition.Wait(pMultiWaitFile->m_mutex);

    //are we downloaded
    {
        CryAutoLock<CryMutex> lock(m_fileDownloadedProtector);
        if (m_fileDownloaded.find(pathCrc) != m_fileDownloaded.end())
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::GetFile( %s : %d ) File Downloaded.", thePath.c_str(), pathCrc);
#endif
            return true;
        }
        else
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::GetFile( %s : %d ) File Download ***FAILED***.", thePath.c_str(), pathCrc);
#endif
            return false;
        }
    }
}

//call a function for each of the returned results of a file scan of a directory, if the results of the
//scan are not known query the server for them
void CFileClient::ScanFiles(const char* searchString, IFileClient::fileDescCallback callback, bool bForce)
{
    char rootPath[FILE_DESC_MAX_PATH];
    CryGetCurrentDirectory(sizeof(rootPath), rootPath);
    std::string strRootPath = rootPath;
    std::transform(strRootPath.begin(), strRootPath.end(), strRootPath.begin(), ::tolower);

    std::string theSearchString = searchString;
    std::transform(theSearchString.begin(), theSearchString.end(), theSearchString.begin(), ::tolower);

    int32_t pos = (int32_t)theSearchString.find(strRootPath);
    if (pos != std::string::npos)
    {
        theSearchString.erase(0, strRootPath.length() + 1);
    }

    int32_t searchCrc = (int32_t)CCrc32::ComputeLowercase(theSearchString.c_str());

    //see if we searched
    {
        bool bFound = false;
        {
            CryAutoLock<CryMutex> lock(m_scanFilesSearchProtector);
            auto iter = m_scanFilesSearch.find(searchCrc);
            if (iter != m_scanFilesSearch.end())
            {
                bFound = true;
            }

            if (bForce && iter != m_scanFilesSearch.end())
            {
                m_scanFilesSearch.erase(iter);
                bFound = false;
            }
        }

        CryAutoLock<CryMutex> lock(m_scanFilesProtector);
        auto range = m_scanFilesDesc.equal_range(searchCrc);

        if (bForce && range.first != range.second)
        {
            m_scanFilesDesc.erase(range.first, range.second);
            range.first = m_scanFilesDesc.end();
            range.second = m_scanFilesDesc.end();
        }

#ifdef FILE_CLIENT_LOGGING
        if (range.first != m_scanFilesDesc.end() && range.second != m_scanFilesDesc.end())
        {
            CryLog("FileClient::ScanFiles( %s : %d ) Already Done, Found matches!", theSearchString.c_str(), searchCrc);
        }
#endif
        for (auto iter = range.first; iter != range.second; ++iter)
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::ScanFiles( %s : %d ) Calling callback on %s", theSearchString.c_str(), searchCrc, iter->second.m_Name.c_str());
#endif
            callback(iter->second);
        }

        if (bFound)
        {
            return;
        }
    }

    std::shared_ptr<MultiWait> pMultiWait = nullptr;

    //wait if we are in progress else request from the server
    {
        //lock
        CryAutoLock<CryMutex> lock(m_scanFilesInProgressProtector);

        auto inProgressIter = m_scanFileInProgress.find(searchCrc);
        if (inProgressIter != m_scanFileInProgress.end())
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::ScanFiles( %s : %d ) Already in Progress... Waiting", searchString, searchCrc);
#endif
            //get a reference to the shared ptr
            pMultiWait = inProgressIter->second;
            pMultiWait->m_mutex.Lock();
        }
        else
        {
            {
                CryAutoLock<CryMutex> lock(m_scanFilesSearchProtector);
                m_scanFilesSearch.insert(std::make_pair(searchCrc, theSearchString.c_str()));
            }

#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::ScanFiles( %s : %d ) Sending Request... Waiting", searchString, searchCrc);
#endif
            //create a new multi wait and lock it
            pMultiWait = std::shared_ptr<MultiWait>(new MultiWait, [](MultiWait* s) { delete s; });
            pMultiWait->m_mutex.Lock();

            //insert this into the map
            m_scanFileInProgress.insert(std::make_pair(searchCrc, pMultiWait));

            //create a new file download request
            ScanFilesRequest req;
            if (theSearchString.size() > FILE_DESC_MAX_PATH)
            {
                CryFatalError("Error");
            }
            strncpy_s(req.searchString, theSearchString.c_str(), theSearchString.size());

            //write request to a buffer
            std::vector<char> dMsg;
            req.append(dMsg);

            //send the request
            if (!gEnv->pEngineConnection->SendMsg(CCrc32::Compute("FileServer::ScanFileRequest"), dMsg.data(), dMsg.size()))
            {
                //shouldn't get here... something went wrong...
                CryFatalError("Error");
            }
        }
    }

    //wait for the request to finish
    pMultiWait->m_condition.Wait(pMultiWait->m_mutex);

    //call the callbacks
    {
        CryAutoLock<CryMutex> lock(m_scanFilesProtector);
        auto range = m_scanFilesDesc.equal_range(searchCrc);
#ifdef FILE_CLIENT_LOGGING
        if (range.first != range.second)
        {
            CryLog("FileClient::ScanFiles( %s : %d ) Done, Found matches!", theSearchString.c_str(), searchCrc);
        }
        else
        {
            CryLog("FileClient::ScanFiles( %s : %d ) Done, no matches found.", theSearchString.c_str(), searchCrc);
        }
#endif
        for (auto iter = range.first; iter != range.second; ++iter)
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::ScanFiles( %s : %d ) Calling callback on %s", theSearchString.c_str(), searchCrc, iter->second.m_Name.c_str());
#endif
            callback(iter->second);
        }
    }
}

//we should get a file info response from the server after a successful query
void CFileClient::processFileInfoResponse(unsigned int type, const void* payload, unsigned int size)
{
    FileInfoResponse res;
    res.read((const char*)payload, size);

    assert((int32_t)CCrc32::ComputeLowercase(res.fileDesc.fileName) == res.pathCrc);

#ifdef FILE_CLIENT_LOGGING
    CryLog("FileClient::processFileInfoResponse( %s : %d )", desc.m_Name.c_str(), pathCrc);
#endif

    FileDesc desc;
    FileDescMsgToFileDesc(res.fileDesc, desc);
    {
        CryAutoLock<CryMutex> lock(m_fileInfoProtector);
        auto iter = m_fileInfo.find(res.pathCrc);
        if (iter == m_fileInfo.end())
        {
            m_fileInfo.insert(std::make_pair(res.pathCrc, desc));
        }
        else
        {
            iter->second = desc;
        }
    }

    std::shared_ptr<MultiWait> pMultiWait = nullptr;

    //unlock the file info in progress
    {
        CryAutoLock<CryMutex> lock(m_fileInfoInProgressProtector);
        auto inProgressIter = m_fileInfoInProgress.find(res.pathCrc);
        assert(inProgressIter != m_fileInfoInProgress.end());

        pMultiWait = inProgressIter->second;

        //erase the in progress entry
        m_fileInfoInProgress.erase(inProgressIter);
    }
#ifdef FILE_CLIENT_LOGGING
    CryLog("FileClient::processFileInfoResponse Notify( %s : %d )", desc.m_Name.c_str(), pathCrc);
#endif
    pMultiWait->m_mutex.Lock();
    pMultiWait->m_condition.Notify();
    pMultiWait->m_mutex.Unlock();
}

//we should get a file download response when a file has finished downloading
void CFileClient::processFileDownloadResponse(unsigned int type, const void* payload, unsigned int size)
{
    FileDownloadResponse res;
    res.read((const char*)payload, size);

    std::shared_ptr<MultiWaitFile> pMultiWaitFile = nullptr;

    //unlock the file download in progress
    {
        CryAutoLock<CryMutex> lock(m_fileDownloadInProgessProtector);
        auto inProgressIter = m_fileDownloadInProgess.find(res.pathCrc);
        assert(inProgressIter != m_fileDownloadInProgess.end());

        pMultiWaitFile = inProgressIter->second;
        assert(pMultiWaitFile);

        assert((int32_t)CCrc32::ComputeLowercase(pMultiWaitFile->m_fileName.c_str()) == res.pathCrc);

        std::string tempFileName = pMultiWaitFile->m_fileName.c_str();
        tempFileName += ".temp";

        //close the temp file
        fclose(pMultiWaitFile->m_file);
        pMultiWaitFile->m_file = nullptr;

        //rename the temp file to the real file name
        remove(pMultiWaitFile->m_fileName.c_str());
        if (rename(tempFileName.c_str(), pMultiWaitFile->m_fileName.c_str()))
        {
            //something went wrong
            CryFatalError("Error");
        }
        else
        {
            CryAutoLock<CryMutex> lock(m_fileDownloadedProtector);
            m_fileDownloaded.insert(res.pathCrc);
        }
#ifdef FILE_CLIENT_LOGGING
        CryLog("FileClient::processFileDownloadResponse( %s : %d )", pMultiWaitFile->m_fileName.c_str(), res.pathCrc);
#endif
        //erase the in progress entry
        m_fileDownloadInProgess.erase(inProgressIter);
    }

#ifdef FILE_CLIENT_LOGGING
    CryLog("FileClient::processFileDownloadResponse Notify( %s : %d )", pMultiWaitFile->m_fileName.c_str(), res.pathCrc);
#endif
    pMultiWaitFile->m_mutex.Lock();
    pMultiWaitFile->m_condition.Notify();
    pMultiWaitFile->m_mutex.Unlock();
}

//we should get a Scan file response from the server after a scan file query
void CFileClient::processScanFilesResponse(unsigned int type, const void* payload, unsigned int size)
{
    ScanFilesResponse res;
    res.read((const char*)payload, size);

#ifdef FILE_CLIENT_LOGGING
    CryLog("FileClient::processScanFilesResponse( %d ) = %d found", res.searchCrc, res.numFileDesc);
#endif
    {
        CryAutoLock<CryMutex> lock(m_scanFilesProtector);
        for (int i = 0; i < res.numFileDesc; ++i)
        {
#ifdef FILE_CLIENT_LOGGING
            CryLog("FileClient::processScanFilesResponse( %d ) -> %s", res.searchCrc, res.dFileDesc[i].m_Name.c_str());
#endif
            FileDesc desc;
            FileDescMsgToFileDesc(res.dFileDesc[i], desc);

            m_scanFilesDesc.insert(std::make_pair(res.searchCrc, desc));
        }
    }

    std::shared_ptr<MultiWait> pMultiWait = nullptr;

    //unlock the scan file in progress
    {
        CryAutoLock<CryMutex> lock(m_scanFilesInProgressProtector);
        auto inProgressIter = m_scanFileInProgress.find(res.searchCrc);
        assert(inProgressIter != m_scanFileInProgress.end());
        pMultiWait = inProgressIter->second;

        //erase the in progress entry
        m_scanFileInProgress.erase(inProgressIter);
    }

#ifdef FILE_CLIENT_LOGGING
    CryLog("FileClient::processScanFilesResponse Notify( %d )", res.searchCrc);
#endif
    pMultiWait->m_mutex.Lock();
    pMultiWait->m_condition.Notify();
    pMultiWait->m_mutex.Unlock();
}

//file downloads are chunked so as not to clog the pipe for high priority traffic
//we could receive many chunks per file download request
void CFileClient::processFileChunk(unsigned int type, const void* payload, unsigned int size)
{
    FileChunk res;
    res.read((const char*)payload, size);

    std::shared_ptr<MultiWaitFile> pMultiWaitFile = nullptr;
    {
        CryAutoLock<CryMutex> lock(m_fileDownloadInProgessProtector);
        auto inProgressIter = m_fileDownloadInProgess.find(res.pathCrc);
        assert(inProgressIter != m_fileDownloadInProgess.end());

        pMultiWaitFile = inProgressIter->second;
        assert(pMultiWaitFile->m_file);

        assert((int32_t)CCrc32::ComputeLowercase(pMultiWaitFile->m_fileName.c_str()) == res.pathCrc);
    }

#ifdef FILE_CLIENT_LOGGING
    CryLog("FileClient::processFileChunk( %s : %d ) chunksize: %d", pMultiWaitFile->m_fileName.c_str(), pathCrc, chunkSize);
#endif

    if (fwrite(res.chunk, 1, res.chunkSize, pMultiWaitFile->m_file) != res.chunkSize)
    {
        CryFatalError("Write Error");
    }
}

//writes the caching file called EngineInfo.bin to the disk based on current state.
//this information is read on session start, verified locally and by the asset processor.
//any entry that fails verification is deleted locally or via processRemoveInfo
void CFileClient::writeEngineInfo()
{
    CryAutoLock<CryMutex> lock1(m_fileInfoProtector);
    CryAutoLock<CryMutex> lock2(m_fileDownloadedProtector);
    CryAutoLock<CryMutex> lock3(m_scanFilesProtector);
    CryAutoLock<CryMutex> lock4(m_scanFilesSearchProtector);

    FILE* pFile = fopen("EngineInfo.bin", "wb");
    assert(pFile);

    uint32_t num = (uint32_t)SwapEndianValue(m_fileDownloaded.size());
    if (fwrite(&num, 1, sizeof(num), pFile) != sizeof(num))
    {
        CryFatalError("Write Error");
    }

    for (auto& item : m_fileDownloaded)
    {
        int32_t crc = SwapEndianValue(item);
        if (fwrite(&crc, 1, sizeof(crc), pFile) != sizeof(crc))
        {
            CryFatalError("Write Error");
        }

        auto iter = m_fileInfo.find(item);
        assert(iter != m_fileInfo.end());

        assert((int32_t)CCrc32::ComputeLowercase(iter->second.m_Name.c_str()) == item);

        FileDescMsg desc;
        FileDescToFileDescMsg(iter->second, desc);
        desc.writeFile(pFile);
    }

    num = (uint32_t)SwapEndianValue(m_fileInfo.size());
    if (fwrite(&num, 1, sizeof(num), pFile) != sizeof(num))
    {
        CryFatalError("Write Error");
    }

    for (auto& item : m_fileInfo)
    {
        int32_t crc = SwapEndianValue(item.first);
        if (fwrite(&crc, 1, sizeof(crc), pFile) != sizeof(crc))
        {
            CryFatalError("Write Error");
        }

        assert((int32_t)CCrc32::ComputeLowercase(item.second.m_Name.c_str()) == item.first);

        FileDescMsg desc;
        FileDescToFileDescMsg(item.second, desc);
        desc.writeFile(pFile);
    }

    num = (uint32_t)SwapEndianValue(m_scanFilesSearch.size());
    if (fwrite(&num, 1, sizeof(num), pFile) != sizeof(num))
    {
        CryFatalError("Write Error");
    }

    for (auto& item : m_scanFilesSearch)
    {
        int32_t crc = SwapEndianValue(item.first);
        if (fwrite(&crc, 1, sizeof(crc), pFile) != sizeof(crc))
        {
            CryFatalError("Write Error");
        }

        if (item.second.length() > FILE_DESC_MAX_PATH)
        {
            assert(false);//fatal?
        }

        char str[FILE_DESC_MAX_PATH];
        strncpy_s(str, item.second.c_str(), item.second.length());
        if (fwrite(str, 1, sizeof(str), pFile) != sizeof(str))
        {
            CryFatalError("Write Error");
        }

        assert((int32_t)CCrc32::ComputeLowercase(item.second.c_str()) == item.first);
    }

    num = (uint32_t)SwapEndianValue(m_scanFilesDesc.size());
    if (fwrite(&num, 1, sizeof(num), pFile) != sizeof(num))
    {
        CryFatalError("Write Error");
    }

    for (auto& item : m_scanFilesDesc)
    {
        int32_t crc = SwapEndianValue(item.first);
        if (fwrite(&crc, 1, sizeof(crc), pFile) != sizeof(crc))
        {
            CryFatalError("Write Error");
        }

        FileDescMsg desc;
        FileDescToFileDescMsg(item.second, desc);
        desc.writeFile(pFile);
    }

    fclose(pFile);
}

//reads the caching file called EngineInfo.bin and reconstructs the current state of the cache.
//This information is read on session start, verified locally and by the asset processor.
//Any entry that fails verification is deleted locally or via processRemoveInfo
void CFileClient::readEngineInfo()
{
    char* buffer = nullptr;
    unsigned int fileSize = 0;
    {
        CryAutoLock<CryMutex> lock1(m_fileInfoProtector);
        CryAutoLock<CryMutex> lock2(m_fileDownloadedProtector);
        CryAutoLock<CryMutex> lock3(m_scanFilesSearchProtector);
        CryAutoLock<CryMutex> lock4(m_scanFilesProtector);

        if (FILE* pFile = fopen("EngineInfo.bin", "rb"))
        {
            fseek(pFile, 0, SEEK_END);
            fileSize = (unsigned int)ftell(pFile);
            fseek(pFile, 0, SEEK_SET);
            buffer = new char[fileSize];
            if (fread(buffer, 1, fileSize, pFile) != fileSize)
            {
                CryFatalError("Read Error");
            }

            fclose(pFile);
            {
                const char* pCur = buffer;
                size_t remainingBytes = fileSize;

                uint32_t num = SwapEndianValue(*(uint32_t*)pCur);
                if (sizeof(num) < remainingBytes)
                {
                    CryFatalError("Buffer Overrun");
                }

                pCur += sizeof(num);
                remainingBytes -= sizeof(num);

                for (uint32_t i = 0; i < num; ++i)
                {
                    int32_t crc = SwapEndianValue(*(int32_t*)pCur);
                    if (sizeof(crc) < remainingBytes)
                    {
                        CryFatalError("Buffer Overrun");
                    }

                    pCur += sizeof(crc);
                    remainingBytes -= sizeof(crc);

                    FileDescMsg desc;
                    pCur = desc.read(pCur, remainingBytes);
                    remainingBytes -= desc.numBytes();

                    assert(crc == (int32_t)CCrc32::ComputeLowercase(desc.fileName));

                    size_t fileSize = 0;
                    if (FILE* pFile = fopen(desc.fileName, "rb"))
                    {
                        fseek(pFile, 0, SEEK_END);
                        fileSize = (size_t)ftell(pFile);
                        fclose(pFile);
                    }

                    if (fileSize == desc.nSize)
                    {
                        m_fileDownloaded.insert(crc);
                    }
                }

                num = SwapEndianValue(*(uint32_t*)pCur);
                if (sizeof(num) < remainingBytes)
                {
                    CryFatalError("Buffer Overrun");
                }

                pCur += sizeof(num);
                remainingBytes -= sizeof(num);

                for (uint32_t i = 0; i < num; ++i)
                {
                    int32_t crc = SwapEndianValue(*(int32_t*)pCur);
                    if (sizeof(crc) < remainingBytes)
                    {
                        CryFatalError("Buffer Overrun");
                    }

                    pCur += sizeof(crc);
                    remainingBytes -= sizeof(crc);

                    FileDescMsg descMsg;
                    pCur = descMsg.read(pCur, remainingBytes);
                    remainingBytes -= descMsg.numBytes();

                    assert(crc == (int32_t)CCrc32::ComputeLowercase(descMsg.fileName));

                    size_t fileSize = 0;
                    if (FILE* pFile = fopen(descMsg.fileName, "rb"))
                    {
                        fseek(pFile, 0, SEEK_END);
                        fileSize = (size_t)ftell(pFile);
                        fclose(pFile);
                    }

                    if (fileSize == descMsg.nSize)
                    {
                        FileDesc desc;
                        FileDescMsgToFileDesc(descMsg, desc);
                        m_fileInfo.insert(std::make_pair(crc, desc));
                    }
                }

                num = SwapEndianValue(*(uint32_t*)pCur);
                if (sizeof(num) < remainingBytes)
                {
                    CryFatalError("Buffer Overrun");
                }

                pCur += sizeof(num);
                remainingBytes -= sizeof(num);

                for (uint32_t i = 0; i < num; ++i)
                {
                    int32_t crc = SwapEndianValue(*(int32_t*)pCur);
                    if (sizeof(crc) < remainingBytes)
                    {
                        CryFatalError("Buffer Overrun");
                    }

                    pCur += sizeof(crc);
                    remainingBytes -= sizeof(crc);

                    char searchString[FILE_DESC_MAX_PATH];
                    if (sizeof(searchString) < remainingBytes)
                    {
                        CryFatalError("Buffer Overrun");
                    }

                    memcpy(&searchString, pCur, sizeof(searchString));
                    pCur += sizeof(searchString);
                    remainingBytes -= sizeof(searchString);

                    m_scanFilesSearch.insert(std::make_pair(crc, searchString));
                }

                num = SwapEndianValue(*(uint32_t*)pCur);
                if (sizeof(num) < remainingBytes)
                {
                    CryFatalError("Buffer Overrun");
                }

                pCur += sizeof(num);
                remainingBytes -= sizeof(num);

                for (uint32_t i = 0; i < num; ++i)
                {
                    int32_t crc = SwapEndianValue(*(int32_t*)pCur);
                    if (sizeof(crc) < remainingBytes)
                    {
                        CryFatalError("Buffer Overrun");
                    }

                    pCur += sizeof(crc);
                    remainingBytes -= sizeof(crc);

                    FileDescMsg descMsg;
                    pCur = descMsg.read(pCur, remainingBytes);
                    remainingBytes -= descMsg.numBytes();

                    FileDesc desc;
                    FileDescMsgToFileDesc(descMsg, desc);

                    m_scanFilesDesc.insert(std::make_pair(crc, desc));
                }
            }
        }

        ////////////////////////////////////////////////////////
        //only one file should never be downloaded... system.cfg
        //so if it is not in the cache put it in so it never downloads it
        int32_t crcSystemCfg = (int32_t)CCrc32::ComputeLowercase("system.cfg");
        if (m_fileDownloaded.find(crcSystemCfg) == m_fileDownloaded.end())
        {
            m_fileDownloaded.insert(crcSystemCfg);
        }

        FileDesc desc;
        desc.m_Name = "system.cfg";
        desc.nAttrib = 0;
        desc.tAccess = 0;
        desc.tCreate = 0;
        desc.tWrite = 0;
        desc.nSize = 1;
        if (FILE* pFile = fopen("system.cfg", "rb"))
        {
            fseek(pFile, 0, SEEK_END);
            desc.nSize = (size_t)ftell(pFile);
            fclose(pFile);
        }
        if (desc.nSize == 0)
        {
            desc.nSize = 1; //not a real size, just has to be non 0
        }

        if (m_fileInfo.find(crcSystemCfg) == m_fileInfo.end())
        {
            m_fileInfo.insert(std::make_pair(crcSystemCfg, desc));
        }
        /////////////////////////////
    }

    //if we have a buffer to send, send it
    if (buffer)
    {
        //lock
        m_engineInfoWait.m_mutex.Lock();

        //send the engine info
        if (!gEnv->pEngineConnection->SendMsg(CCrc32::Compute("FileServer::EngineInfoRequest"), buffer, fileSize))
        {
            //shouldn't get here... something went wrong...
            CryFatalError("Error");
        }

        //wait
        m_engineInfoWait.m_condition.Wait(m_engineInfoWait.m_mutex);

        //delete the buffer
        delete buffer;
    }
}

//if the asset processor determines the cache of the client is inconsistent it sends remove info responses
//that delete those entries in the cache. Then when the operation occurs next it will query the server as
//it is no longer in the cache
void CFileClient::processRemoveInfo(unsigned int type, const void* payload, unsigned int size)
{
    RemoveResponse res;
    res.read((const char*)payload, size);

    //we will receive system.cfg because we never download it.. we must ignore its removal
    int32_t crcSystemCfg = (int32_t)CCrc32::ComputeLowercase("system.cfg");
    if (res.crc == crcSystemCfg)
    {
        return;
    }

#ifdef FILE_CLIENT_LOGGING
    CryLog("FileClient::processRemoveInfo( %d )", crc);
#endif
    {
        CryAutoLock<CryMutex> lock(m_fileInfoProtector);
        auto iter = m_fileInfo.find(res.crc);
        if (iter != m_fileInfo.end())
        {
            assert(res.crc == (int32_t)CCrc32::ComputeLowercase(iter->second.m_Name.c_str()));

            remove(iter->second.m_Name.c_str());
            m_fileInfo.erase(iter);
        }
    }

    {
        CryAutoLock<CryMutex> lock(m_fileDownloadedProtector);
        auto iter = m_fileDownloaded.find(res.crc);
        if (iter != m_fileDownloaded.end())
        {
            m_fileDownloaded.erase(iter);
        }
    }

    {
        CryAutoLock<CryMutex> lock(m_scanFilesProtector);
        auto iter = m_scanFilesSearch.find(res.crc);
        if (iter != m_scanFilesSearch.end())
        {
            m_scanFilesSearch.erase(iter);
        }
        auto range = m_scanFilesDesc.equal_range(res.crc);
        if (range.first != m_scanFilesDesc.end() || range.second != m_scanFilesDesc.end())
        {
            m_scanFilesDesc.erase(range.first, range.second);
        }
    }
}

//when the asset processor has completed its verification of the client cache we will recieve
//a remove done response
void CFileClient::processRemoveDone(unsigned int type, const void* payload, unsigned int size)
{
#ifdef FILE_CLIENT_LOGGING
    CryLog("FileClient::processRemoveDone Notify()");
#endif
    m_engineInfoWait.m_mutex.Lock();
    m_engineInfoWait.m_condition.Notify();
    m_engineInfoWait.m_mutex.Unlock();
}

//when the user presses the clear cache button on the asset servers client connection tab, this client
//will delete its internal cache and delete the EngineIno.bin file
void CFileClient::processClearCache(unsigned int type, const void* payload, unsigned int size)
{
#ifdef FILE_CLIENT_LOGGING
    CryLog("FileClient::processClearCache Notify()");
#endif
    {
        CryAutoLock<CryMutex> lock1(m_fileInfoProtector);
        CryAutoLock<CryMutex> lock2(m_fileDownloadedProtector);
        CryAutoLock<CryMutex> lock3(m_scanFilesProtector);

        m_fileInfo.clear();
        m_fileDownloaded.clear();
        m_scanFilesDesc.clear();

        remove("EngineInfo.bin");
        writeEngineInfo();
    }
}

//when want to automatically write the current cache state to disk on engine load and level load
void CFileClient::OnLevelLoaded()
{
    writeEngineInfo();
}

void CFileClient::OnEngineLoaded()
{
    writeEngineInfo();
}

////////////////////////////////////////////////////////////////////
size_t CFileClient::FRead(void* pData, size_t nSize, size_t nCount, IFileClient::fileHandle_t hFile)
{
    // Code copied from FileIOWrapper
    return ::fread(pData, nSize, nCount, hFile);
}

int CFileClient::FClose(IFileClient::fileHandle_t hFile)
{
    // Code copied from FileIOWrapper
    return fclose(hFile);
}

IFileClient::fileHandle_t CFileClient::FOpen(const char* file, const char* mode)
{
    std::string actualFilePath = convertToRealPath(file);

    // Code copied from FileIOWrapper
    FILE* f = NULL;
    IPlatformOS* pOS = gEnv->pSystem->GetPlatformOS();
    if (pOS->IsFileAvailable(actualFilePath.c_str()))
    {
#if defined(WIN32) || defined(WIN64)
        f = ::_wfopen(CryStringUtils::UTF8ToWStr(actualFilePath.c_str()), CryStringUtils::UTF8ToWStr(mode));
#else
        f = ::fopen(file, mode);
#endif
    }

    return f;
}

int64 CFileClient::FTell(IFileClient::fileHandle_t hFile)
{
    // Code copied from FileIOWrapper
#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_FTELL_NOT_FTELLI64
    return (int64)ftell(hFile);
#else
    return _ftelli64(hFile);
#endif
}

int CFileClient::FSeek(IFileClient::fileHandle_t hFile, int64 seek, int mode)
{
    // Code copied from FileIOWrapper
    return fseek(hFile, (long)seek, mode);
}

int CFileClient::FEof(IFileClient::fileHandle_t hFile)
{
    // Code copied from FileIOWrapper
    return feof(hFile);
}

int CFileClient::FError(IFileClient::fileHandle_t hFile)
{
    // Code copied from FileIOWrapper
    return ferror(hFile);
}

std::string CFileClient::convertToRealPath(const char* partialPath) const
{
    if (RCPathUtil::IsRelativePath(partialPath))
    {
        char rootPath[FILE_DESC_MAX_PATH];
        CryGetCurrentDirectory(sizeof(rootPath), rootPath);
        std::string strRootPath = rootPath;
        std::transform(strRootPath.begin(), strRootPath.end(), strRootPath.begin(), ::tolower);

        std::string path;
        path.append(strRootPath);
        path.append("\\");
        path.append(partialPath);
        return path;
    }

    // todo:  Strip the full path?

    return std::string(partialPath);
}

//////////////////////////////////////////////////////////////////////////
