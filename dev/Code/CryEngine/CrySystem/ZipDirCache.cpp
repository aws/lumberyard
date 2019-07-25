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

#include "StdAfx.h"
#include "MTSafeAllocator.h"
#include <smartptr.h>
#include "ZipFileFormat.h"
#include "ZipDirStructures.h"
#include "ZipDirTree.h"
#include "ZipDirCache.h"
#include "ZipDirFind.h"
#include "ZipDirCacheFactory.h"
#include "CryZlib.h"
#include <IDiskProfiler.h>
#include "CryPak.h"
#include "ZipEncrypt.h"
#include "System.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

// initializes the instance structure
void ZipDir::Cache::Construct(CZipFile& fNew, CMTSafeHeap* pHeap, size_t nDataSizeIn, unsigned int nFactoryFlags, size_t nAllocatedSize)
{
    m_pRefCount = new signed int;
    *m_pRefCount = 0;
    new (&m_zipFile)CZipFile;
    m_zipFile.Swap(fNew);
    m_nDataSize = nDataSizeIn;
    m_nAllocatedSize = nAllocatedSize;
    m_nZipPathOffset = nDataSizeIn;
    m_zipFile.m_szFilename = GetFilePath();
    m_pCacheData = new CacheData;
    m_pCacheData->m_pHeap = pHeap;
    m_nCacheFactoryFlags = nFactoryFlags;
    m_pRootData = (DirHeader*)GetDataPointer();
    m_encryptedHeaders = ZipFile::HEADERS_NOT_ENCRYPTED;
    m_signedHeaders = ZipFile::HEADERS_NOT_SIGNED;
    if (m_nCacheFactoryFlags & CacheFactory::FLAGS_FILENAMES_AS_CRC32)
    {
        m_pRootData = NULL;
    }

    m_nFileSize = 0;
    m_nPakFileOffsetOnMedia = 0;
}

// self-destruct when ref count drops to 0
void ZipDir::Cache::Delete()
{
    UnloadFromMemory();

    m_zipFile.Close();

    CMTSafeHeap* pHeap = m_pCacheData->m_pHeap;
    SAFE_DELETE(m_pCacheData);
    delete m_pRefCount;
    pHeap->FreePersistent(this);
}

void ZipDir::Cache::PreloadToMemory(IMemoryBlock* pMemoryBlock)
{
    // Make sure that fseek/fread by LoadToMemory is atomic (and reads from other threads don't conflict)
    CryAutoCriticalSection lock(m_pCacheData->m_csCacheIOLock);

    m_zipFile.LoadToMemory(pMemoryBlock);
    m_nPakFileOffsetOnMedia = 0;
}

void ZipDir::Cache::UnloadFromMemory()
{
    // Make sure that reads from other threads don't conflict with the unload
    CryAutoCriticalSection lock(m_pCacheData->m_csCacheIOLock);
    m_zipFile.UnloadFromMemory();
}


// initializes this object from the given Zip file: caches the central directory
// returns 0 if successfully parsed, error code if an error has occured
ZipDir::CachePtr ZipDir::NewCache (const char* szFileName, CMTSafeHeap* pHeap, InitMethodEnum nInitMethod)
{
    // try .. catch(Error)
    CacheFactory factory(pHeap, nInitMethod);
    return factory.New(szFileName, (nInitMethod == ZD_INIT_VALIDATE_IN_MEMORY) ? ICryPak::eInMemoryPakLocale_GPU : ICryPak::eInMemoryPakLocale_Unload);
}

//////////////////////////////////////////////////////////////////////////
// nNameOffset of the FileEntry is used as a CRC32 of the filename
//////////////////////////////////////////////////////////////////////////
struct FileEntryNameAsCrc32Predicate
{
    bool operator()(const ZipDir::FileEntry& f1, unsigned int nCrc32) const
    {
        return f1.nNameOffset < nCrc32;
    }
    bool operator()(unsigned int nCrc32, const ZipDir::FileEntry& f1) const
    {
        return nCrc32 < f1.nNameOffset;
    }
};

// looks for the given file record in the Central Directory. If there's none, returns NULL.
// if there is some, returns the pointer to it.
// the Path must be the relative path to the file inside the Zip
// if the file handle is passed, it will be used to find the file data offset, if one hasn't been initialized yet
ZipDir::FileEntry* ZipDir::Cache::FindFile (const char* szPath, bool bRefresh)
{
    if (m_nCacheFactoryFlags & CacheFactory::FLAGS_FILENAMES_AS_CRC32)
    {
        void* pDataStart = (this + 1);
        FileEntry* pFirstEntry = reinterpret_cast<FileEntry*>(pDataStart);
        FileEntry* pLastEntry = pFirstEntry + (m_nDataSize / sizeof(FileEntry));

        uint32 nNameAsCrc32 = ZipDir::FileNameHash(szPath);

        FileEntry* pEntry = std::lower_bound(pFirstEntry, pLastEntry, nNameAsCrc32, FileEntryNameAsCrc32Predicate());
        if (pEntry != pLastEntry && pEntry->nNameOffset == nNameAsCrc32)
        {
            return pEntry;
        }
        return NULL;
    }

    ZipDir::FindFile fd (this);
    if (!fd.FindExact(szPath))
    {
        assert (!fd.GetFileEntry());
        return NULL;
    }
    assert (fd.GetFileEntry());
    return fd.GetFileEntry();
}




// loads the given file into the pCompressed buffer (the actual compressed data)
// if the pUncompressed buffer is supplied, uncompresses the data there
// buffers must have enough memory allocated, according to the info in the FileEntry
// NOTE: there's no need to decompress if the method is 0 (store)
// returns 0 if successful or error code if couldn't do something
ZipDir::ErrorEnum ZipDir::Cache::ReadFile (FileEntry* pFileEntry, void* pCompressed, void* pUncompressed, const bool decompress, int64 nDataOffset, int64 nDataReadSize, const bool decrypt)
{
    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_CALL;
    }

    if (pFileEntry->desc.lSizeUncompressed == 0)
    {
        assert (pFileEntry->desc.lSizeCompressed == 0);
        return ZD_ERROR_SUCCESS;
    }

    assert (pFileEntry->desc.lSizeCompressed > 0);

    ErrorEnum nError = Refresh(pFileEntry);
    if (nError != ZD_ERROR_SUCCESS)
    {
        return nError;
    }


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define ZIPDIRCACHE_CPP_SECTION_1 1
#define ZIPDIRCACHE_CPP_SECTION_2 2
#endif

#if !defined(SUPPORT_UNENCRYPTED_PAKS)
    if (!pFileEntry->IsEncrypted())
    {
#if !defined(_RELEASE)
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Tried to load a non-encrypted file when support for non-encrypted paks is disabled");
#endif  //!_RELEASE
        return ZD_ERROR_CORRUPTED_DATA;
    }
#endif  //!SUPPORT_UNENCRYPTED_PAKS


    SmartPtr pBufferDestroyer(m_pCacheData->m_pHeap);

    if (decrypt && pFileEntry->IsEncrypted())
    {
        assert(nDataOffset == 0 && (nDataReadSize == -1 || nDataReadSize == pFileEntry->desc.lSizeUncompressed));  // Check for invalid parameters, nDataOffset & nDataReadSize only can be applied on encrypted files if the calling code handles the decryption or the entire file is being read.
    }

    int64 nFileOffset = nDataOffset;
    int64 nFileReadSize = (nDataReadSize == -1) ? pFileEntry->desc.lSizeCompressed : nDataReadSize;

    void* pBuffer = pCompressed; // the buffer where the compressed data will go

    if (!pFileEntry->IsCompressed() && pUncompressed)
    {
        nFileOffset = nDataOffset;

        if (nDataReadSize < 0)
        {
            nDataReadSize = pFileEntry->desc.lSizeUncompressed;
        }
        nFileReadSize = nDataReadSize;

        // we can directly read into the uncompress buffer
        pBuffer = pUncompressed;
    }


    if (!pBuffer)
    {
        if (!pUncompressed)
        {
            // what's the sense of it - no buffers at all?
            return ZD_ERROR_INVALID_CALL;
        }

        if (decompress)
        {
            pBuffer = m_pCacheData->m_pHeap->TempAlloc(pFileEntry->desc.lSizeCompressed, "ZipDir::Cache::ReadFile");
            pBufferDestroyer.Attach(pBuffer); // we want it auto-freed once we return
        }
        else    // we can store the data in uncompressed buffer
        {
            pBuffer = pUncompressed;
        }
    }

    {
        CryAutoCriticalSection lock(m_pCacheData->m_csCacheIOLock); // guarantees that fseek() and fread() will be executed together
        FRAME_PROFILER("ZipDir_Cache_ReadFile", gEnv->pSystem, PROFILE_SYSTEM);

        {
            int64 nSeekRes = ZipDir::FSeek(&m_zipFile, nFileOffset + pFileEntry->nFileDataOffset, SEEK_SET);
            if (nSeekRes)
            {
#if defined(APPLE) || defined(LINUX)
                int errnoVal = errno;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION ZIPDIRCACHE_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/ZipDirCache_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/ZipDirCache_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
                int errnoVal = *_errno();
#endif
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR_DBGBRK, "ZipDir::FSeek failed (%p (%i, %lli), %lli, %i, %p) = %i, %i",
                    &m_zipFile, ZipDir::FEof(&m_zipFile), ZipDir::FTell(&m_zipFile),
                    nFileOffset + pFileEntry->nFileDataOffset, SEEK_SET, pFileEntry, (int)GetLastError(), errnoVal);

                return ZD_ERROR_IO_FAILED;
            }
        }
        {
            assert(pBuffer);
            if (pBuffer == NULL) // return failed if buffer allocation failed
            {
                return ZD_ERROR_NO_MEMORY;
            }

            int64 nReadRes = ZipDir::FRead (&m_zipFile, pBuffer, (size_t)nFileReadSize, 1);
            if (nReadRes != 1)
            {
#if defined(APPLE) || defined(LINUX)
                int errnoVal = errno;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION ZIPDIRCACHE_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/ZipDirCache_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/ZipDirCache_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
                int errnoVal = *_errno();
#endif
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR_DBGBRK, "ZipDir::FRead failed (%p (%i, %lli), %p, %i, %i, %p) = %lli, %i, %i",
                    &m_zipFile, ZipDir::FEof(&m_zipFile), ZipDir::FTell(&m_zipFile),
                    pBuffer, (int)nFileReadSize, 1, pFileEntry, nReadRes, (int)GetLastError(), errnoVal);
                return ZD_ERROR_IO_FAILED;
            }
        }
    }

    //Encryption techniques are stored on a per-file basis
    if (decrypt && pFileEntry->IsEncrypted())
    {
        if (0)
        {
            //Intentionally empty block
        }
#ifdef SUPPORT_RSA_AND_STREAMCIPHER_PAK_ENCRYPTION
        else if (pFileEntry->nMethod == ZipFile::METHOD_STORE_AND_STREAMCIPHER_KEYTABLE || pFileEntry->nMethod == ZipFile::METHOD_DEFLATE_AND_STREAMCIPHER_KEYTABLE)
        {
            unsigned char IV[ZipFile::BLOCK_CIPHER_KEY_LENGTH]; //16 byte
            int nKeyIndex = ZipEncrypt::GetEncryptionKeyIndex(pFileEntry);
            ZipEncrypt::GetEncryptionInitialVector(pFileEntry, IV);
            if (!ZipEncrypt::DecryptBufferWithStreamCipher((unsigned char*)pBuffer, pFileEntry->desc.lSizeCompressed, m_block_cipher_keys_table[nKeyIndex], IV))
            {
                // Decryption of CDR failed.
                return ZD_ERROR_CORRUPTED_DATA;
            }
        }
#endif  //SUPPORT_RSA_AND_STREAMCIPHER_PAK_ENCRYPTION
#if defined(SUPPORT_XTEA_PAK_ENCRYPTION)
        else if (pFileEntry->nMethod == METHOD_DEFLATE_AND_ENCRYPT)
        {
            // file is compressed and encrypted.
            ZipDir::Decrypt((char*)pBuffer, pFileEntry->desc.lSizeCompressed);
        }
#endif  //SUPPORT_XTEA_PAK_ENCRYPTION
#if !defined(OPTIMIZED_READONLY_ZIP_ENTRY) && defined(SUPPORT_STREAMCIPHER_PAK_ENCRYPTION)
        else if (pFileEntry->nMethod == METHOD_DEFLATE_AND_STREAMCIPHER)
        {
            ZipDir::StreamCipher((char*)pBuffer, pFileEntry);
        }
#endif
        else
        {
            //Catch all for unsupported encryption type
#if !defined(_RELEASE)
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Tried to load a file from an archive using an unsupported method");
#endif  //!_RELEASE
            return ZD_ERROR_CORRUPTED_DATA;
        }
    }

    // if there's a buffer for uncompressed data, uncompress it to that buffer
    if (pUncompressed && decompress)
    {
        if (!pFileEntry->IsCompressed())
        {
            assert (pBuffer == pUncompressed);
        }
        else
        {
            if (Z_OK != DecompressFile(pFileEntry, pBuffer, pUncompressed, m_pCacheData->m_csCacheIOLock))
            {
                return ZD_ERROR_CORRUPTED_DATA;
            }
        }

#if defined(VERIFY_PAK_ENTRY_CRC)
        //Perform CRC Checking if required
        if (pFileEntry->desc.lCRC32 != 0 && nDataOffset == 0 && (nDataReadSize == -1 || nDataReadSize == pFileEntry->desc.lSizeUncompressed))
        {
            uint32 computedCRC32 = crc32(0, (unsigned char*)pUncompressed, pFileEntry->desc.lSizeUncompressed);
            if (pFileEntry->desc.lCRC32 != computedCRC32)
            {
                //Mismatch detected. Inform the Platform OS in case any global response is needed and return an error
#if !defined(_RELEASE)
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "ZipDir::Cache::ReadFile mismatch detected for file %s: Generated CRC = 0x%8X, Loaded CRC = 0x%8X", GetFileEntryName(pFileEntry), computedCRC32, pFileEntry->desc.lCRC32);
#endif //!_RELEASE
                return ZD_ERROR_CORRUPTED_DATA;
            }
#if defined(CHECK_CRC_ONLY_ONCE)
            pFileEntry->desc.lCRC32 = 0;    //Zero the CRC so that we only check it once. This should only be done if we no longer need the CRC for anything.
#endif //CHECK_CRC_ONLY_ONCE
        }
#endif //VERIFY_PAK_ENTRY_CRC
    }

    return ZD_ERROR_SUCCESS;
}

extern SSystemCVars g_cvars;

ZipDir::ErrorEnum ZipDir::Cache::ReadFileStreaming (FileEntry* pFileEntry, void* pOut, int64 nDataOffset, int64 nDataReadSize)
{
    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_CALL;
    }

    if (pFileEntry->desc.lSizeUncompressed == 0)
    {
        assert (pFileEntry->desc.lSizeCompressed == 0);
        return ZD_ERROR_SUCCESS;
    }

    assert (pFileEntry->desc.lSizeCompressed > 0);

#if !defined(SUPPORT_UNENCRYPTED_PAKS)
    if (!pFileEntry->IsEncrypted())
    {
#if !defined(_RELEASE)
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Tried to load a non-encrypted file when support for non-encrypted paks is disabled");
#endif  //!_RELEASE
        return ZD_ERROR_CORRUPTED_DATA;
    }
#endif  //!SUPPORT_UNENCRYPTED_PAKS

    bool bRead = false;

#ifdef SUPPORT_UNBUFFERED_IO
    if (!m_zipFile.IsInMemory() && g_cvars.pakVars.nUncachedStreamReads)
    {
        CryAutoCriticalSection lock(m_pCacheData->m_csCacheIOLock); // guarantees that fseek() and fread() will be executed together
        FRAME_PROFILER("ZipDir_Cache_ReadFile", gEnv->pSystem, PROFILE_SYSTEM);

        if (m_zipFile.m_unbufferedFile != INVALID_HANDLE_VALUE)
        {
            if (!nDataReadSize)
            {
                nDataReadSize = pFileEntry->desc.lSizeCompressed;
            }
            nDataOffset += pFileEntry->nFileDataOffset;

            // Align the offset to the sector start.
            int64 nSectorOffset = nDataOffset & ~(m_zipFile.m_nSectorSize - 1);
            int64 nSectorPad = nDataOffset - nSectorOffset;
            int64 nSectorReadSize = Align(nDataReadSize + nSectorPad, m_zipFile.m_nSectorSize);

            LONG nOffsLow = nSectorOffset & 0xffffffff;
            LONG nOffsHigh = static_cast<uint64>(nSectorOffset) >> 32;
            SetFilePointer(m_zipFile.m_unbufferedFile, nOffsLow, &nOffsHigh, FILE_BEGIN);

            ptrdiff_t nBufferOffs = -(ptrdiff_t)nSectorPad;

            while (nBufferOffs < (ptrdiff_t)nDataReadSize)
            {
                DWORD nToRead = (DWORD)min(128LL * 1024LL, nSectorReadSize);
                DWORD nRead = 0;
                if (!::ReadFile(m_zipFile.m_unbufferedFile, m_zipFile.m_pReadTarget, nToRead, &nRead, NULL))
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR_DBGBRK, "ZipDir::ReadFileStreaming failed (%p %p) = %i",
                        &m_zipFile, pFileEntry, GetLastError());
                    return ZD_ERROR_IO_FAILED;
                }

                ptrdiff_t srcLeft = 0;
                ptrdiff_t srcRight = nRead;
                ptrdiff_t dstLeft = nBufferOffs;
                ptrdiff_t dstRight = (ptrdiff_t)(nBufferOffs + nRead);

                if (dstLeft < 0)
                {
                    srcLeft += -dstLeft;
                    dstLeft = 0;
                }
                if (dstRight > nDataReadSize)
                {
                    dstRight -= dstRight - (ptrdiff_t)nDataReadSize;
                    dstRight = (ptrdiff_t)nDataReadSize;
                }

                memcpy(reinterpret_cast<char*>(pOut) + dstLeft, reinterpret_cast<char*>(m_zipFile.m_pReadTarget) + srcLeft, dstRight - dstLeft);

                nSectorReadSize -= nRead;
                nBufferOffs += nRead;
            }

            bRead = true;
        }
    }
#endif

    if (!bRead)
    {
        return ReadFile(pFileEntry, pOut, NULL, false, nDataOffset, nDataReadSize, false);
    }

    return ZD_ERROR_SUCCESS;
}

// decompress compressed file
ZipDir::ErrorEnum ZipDir::Cache::DecompressFile (FileEntry* pFileEntry, void* pCompressed, void* pUncompressed, CryCriticalSection& csDecmopressLock)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);
    if (pUncompressed == NULL)
    {
        return ZD_ERROR_INVALID_CALL;
    }

    PROFILE_DISK_DECOMPRESS;
    unsigned long nSizeUncompressed = pFileEntry->desc.lSizeUncompressed;

    SmartPtr pBufferDestroyer(m_pCacheData->m_pHeap);

    void* pBuffer = pCompressed;
    if (pBuffer == pUncompressed && pFileEntry->nMethod != 0)    // it means that we stored compressed data in the uncompressed buffer
    {
        pBuffer = m_pCacheData->m_pHeap->TempAlloc(pFileEntry->desc.lSizeCompressed, "ZipDir::Cache::DecompressFile");
        pBufferDestroyer.Attach(pBuffer); // we want it auto-freed once we return
        memcpy (pBuffer, pCompressed, pFileEntry->desc.lSizeCompressed);
    }

    AUTO_LOCK_CS(csDecmopressLock);
    if (Z_OK != ZipRawUncompress(m_pCacheData->m_pHeap, pUncompressed, &nSizeUncompressed, pBuffer, pFileEntry->desc.lSizeCompressed))
    {
        return ZD_ERROR_CORRUPTED_DATA;
    }
    return ZD_ERROR_SUCCESS;
}


// loads and unpacks the file into a newly created buffer (that must be subsequently freed with
// Free()) Returns NULL if failed
void* ZipDir::Cache::AllocAndReadFile (FileEntry* pFileEntry)
{
    if (!pFileEntry)
    {
        return NULL;
    }

    void* pData = m_pCacheData->m_pHeap->TempAlloc(pFileEntry->desc.lSizeUncompressed, "ZipDir::Cache::AllocAndReadFile");
    if (pData)
    {
        if (ZD_ERROR_SUCCESS != ReadFile (pFileEntry, NULL, pData))
        {
            m_pCacheData->m_pHeap->FreeTemporary(pData);
            pData = NULL;
        }
    }
    return pData;
}

// frees the memory block that was previously allocated by AllocAndReadFile
void ZipDir::Cache::Free (void* pData)
{
    m_pCacheData->m_pHeap->FreeTemporary(pData);
}

// refreshes information about the given file entry into this file entry
ZipDir::ErrorEnum ZipDir::Cache::Refresh (FileEntry* pFileEntry)
{
    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_CALL;
    }

    if (pFileEntry->nFileDataOffset != pFileEntry->INVALID_DATA_OFFSET)
    {
        return ZD_ERROR_SUCCESS; // the data offset has been successfully read..
    }
    CryAutoCriticalSection lock(m_pCacheData->m_csCacheIOLock); // Not thread safe without this
    return ZipDir::Refresh(&m_zipFile, pFileEntry);
}

//////////////////////////////////////////////////////////////////////////
uint32 ZipDir::Cache::GetFileDataOffset(FileEntry* pFileEntry)
{
    if (pFileEntry->nFileDataOffset == pFileEntry->INVALID_DATA_OFFSET)
    {
        CryAutoCriticalSection lock(m_pCacheData->m_csCacheIOLock); // Not thread safe without this
        ZipDir::Refresh (&m_zipFile, pFileEntry);
    }
    return pFileEntry->nFileDataOffset;
}

// returns the size of memory occupied by the instance referred to by this cache
// must be exact, because it's used by CacheRW to reallocate this cache
size_t ZipDir::Cache::GetSize() const
{
    return m_nDataSize + sizeof(Cache) + strlen(GetFilePath());
}


// QUICK check to determine whether the file entry belongs to this object
bool ZipDir::Cache::IsOwnerOf (const FileEntry* pFileEntry) const
{
    if (m_nCacheFactoryFlags & CacheFactory::FLAGS_FILENAMES_AS_CRC32)
    {
        // Assume true, this function only used in asserts
        return true;
    }

    // just check whether the pointer is within the memory block of this cache instance
    return ((ULONG_PTR)pFileEntry >= (ULONG_PTR)(GetRoot() + 1)   && (ULONG_PTR)pFileEntry <= ((ULONG_PTR)GetRoot()) + m_nDataSize - sizeof(FileEntry));
}


bool ZipDir::Cache::ReOpen(const char* filePath)
{
    CryAutoCriticalSection lock(m_pCacheData->m_csCacheIOLock);

    m_zipFile.Close(false);
    AZ::IO::HandleType fileHandle;
    if (AZ::IO::FileIOBase::GetDirectInstance()->Open(filePath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle))
    {
        m_zipFile.m_fileHandle = fileHandle;

#ifdef SUPPORT_UNBUFFERED_IO
        char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);
        if (!m_zipFile.OpenUnbuffered(resolvedPath))
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Failed to reopen '%s' for unbuffered IO.", filePath);
        }
#endif

        return true;
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////
const char* ZipDir::Cache::GetFileEntryName(FileEntry* pFileEntry)
{
    if (m_pRootData)
    {
        return pFileEntry->GetName(m_pRootData->GetNamePool());
    }
    return "crc32";
}

#ifndef _RELEASE
struct SValidateHashEntry
{
    string filename;
    string pak;
};
void ZipDir::Cache::ValidateFilenameHash(uint32 nHash, const char* filename, const char* pakFile)
{
    static std::map<uint32, SValidateHashEntry*> validateHashMap;

    if (filename[0] == 0)
    {
        return;
    }

    SValidateHashEntry* hashEntry = stl::find_in_map(validateHashMap, nHash, 0);
    if (hashEntry)
    {
        // Same hash, may be the same filename.
        if (hashEntry->filename.compareNoCase(filename) != 0)
        {
            CryFatalError("Filename Hash function collision, Files from different pak files generate same hash code [0x%X]\r\nFile1: (%s) %s\r\nFile2: (%s) %s",
                nHash, hashEntry->pak.c_str(), hashEntry->filename.c_str(), pakFile, filename);
        }
    }
    else
    {
        hashEntry = new SValidateHashEntry;
        hashEntry->filename = filename;
        hashEntry->pak = pakFile;
        validateHashMap[nHash] = hashEntry;
    }
}
#endif //_RELEASE

//////////////////////////////////////////////////////////////////////////
int64 ZipDir::FSeek(CZipFile* file, int64 origin, int command)
{
    if (file->IsInMemory())
    {
        int64 retCode = -1;
        int64 newPos;
        switch (command)
        {
        case SEEK_SET:
            newPos = origin;
            if (newPos >= 0 && newPos <= file->m_nSize)
            {
                file->m_nCursor = newPos;
                retCode = 0;
            }
            break;
        case SEEK_CUR:
            newPos = origin + file->m_nCursor;
            if (newPos >= 0 && newPos <= file->m_nSize)
            {
                file->m_nCursor =  newPos;
                retCode = 0;
            }
            break;
        case SEEK_END:
            newPos = file->m_nSize - origin;
            if (newPos >= 0 && newPos <= file->m_nSize)
            {
                file->m_nCursor =  newPos;
                retCode = 0;
            }
            break;
        default:
            // Not valid disk operation!
            assert(0);
        }
        return retCode;
    }
    else
    {
        if (AZ::IO::FileIOBase::GetDirectInstance()->Seek(file->m_fileHandle, origin, AZ::IO::GetSeekTypeFromFSeekMode(command)))
        {
            return 0;
        }
        return 1;
    }
}

int64 ZipDir::FRead(CZipFile* file, void* data, size_t elementSize, size_t count)
{
    if (file->IsInMemory())
    {
        int64 nRead = count * elementSize;
        int64 nCanBeRead = file->m_nSize - file->m_nCursor;
        if (nRead > nCanBeRead)
        {
            nRead = nCanBeRead;
        }

        file->m_pInMemoryData->CopyMemoryRegion(data, (size_t)file->m_nCursor, (size_t)nRead);
        file->m_nCursor += nRead;
        return nRead / elementSize;
    }
    else
    {
        AZ::IO::HandleType fileHandle = file->m_fileHandle;
        AZ::u64 bytesRead = 0;
        AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, data, elementSize * count, false, &bytesRead);
        return bytesRead / elementSize;
    }
}

int64 ZipDir::FTell(CZipFile* file)
{
    if (file->IsInMemory())
    {
        return file->m_nCursor;
    }
    else
    {
        AZ::u64 tellResult = 0;
        AZ::IO::FileIOBase::GetDirectInstance()->Tell(file->m_fileHandle, tellResult);
        return tellResult;
    }
}

int ZipDir::FEof(CZipFile* zipFile)
{
    if (zipFile->IsInMemory())
    {
        return zipFile->m_nCursor == zipFile->m_nSize;
    }
    else
    {
        return AZ::IO::FileIOBase::GetDirectInstance()->Eof(zipFile->m_fileHandle);
    }
}
