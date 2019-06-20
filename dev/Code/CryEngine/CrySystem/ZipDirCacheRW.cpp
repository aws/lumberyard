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
#include <smartptr.h>
#include "MTSafeAllocator.h"
#include "ZipFileFormat.h"
#include "ZipDirStructures.h"
#include "ZipDirTree.h"
#include "ZipDirList.h"
#include "ZipDirCache.h"
#include "ZipDirCacheRW.h"
#include "ZipDirCacheFactory.h"
#include "ZipDirFindRW.h"
#include "CryPak.h"
#include "ZipEncrypt.h"
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/FileOperations.h>

#ifndef OPTIMIZED_READONLY_ZIP_ENTRY

#include "CryZlib.h"  // declaration of Z_OK for ZipRawDecompress

//////////////////////////////////////////////////////////////////////////
void ZipDir::CacheRW::AddRef()
{
    CryInterlockedIncrement(&m_nRefCount);
}

//////////////////////////////////////////////////////////////////////////
void ZipDir::CacheRW::Release()
{
    if (CryInterlockedDecrement(&m_nRefCount) <= 0)
    {
        delete this;
    }
}

void ZipDir::CacheRW::Close()
{
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        if (!(m_nFlags & FLAGS_READ_ONLY))
        {
            if ((m_nFlags & FLAGS_UNCOMPACTED) && !(m_nFlags & FLAGS_DONT_COMPACT))
            {
                if (!RelinkZip())
                {
                    WriteCDR();
                }
            }
            else
            if (m_nFlags & FLAGS_CDR_DIRTY)
            {
                WriteCDR();
            }
        }

        if (m_fileHandle != AZ::IO::InvalidHandle)                      // RelinkZip() might have closed the file
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Close(m_fileHandle);
            m_fileHandle = AZ::IO::InvalidHandle;
        }
    }
    m_pHeap = NULL;
    m_treeDir.Clear();
}

//////////////////////////////////////////////////////////////////////////
char* ZipDir::CacheRW::UnifyPath(char* const str, const char* pPath)
{
    assert(str);
    const char* src = pPath;
    char* trg = str;
    while (*src)
    {
        if (*src != '/')
        {
            *trg++ = ::tolower(*src++);
        }
        else
        {
            *trg++ = '\\';
            src++;
        }
    }
    *trg = 0;
    return str;
}

bool ZipDir::CacheRW::WriteCompressedData(const char* data, size_t size, bool encrypt)
{
    if (size == 0)
    {
        return true;
    }

    std::vector<char> buffer;
    if (encrypt)
    {
#if defined(SUPPORT_XTEA_PAK_ENCRYPTION)
        buffer.resize(size);
        memcpy(&buffer[0], data, size);
        ZipDir::Encrypt(&buffer[0], size);
        data = &buffer[0];
#else
#if !defined(_RELEASE)
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR_DBGBRK, "Attempt to use XTEA pak encryption when it is disabled.");
#endif
        return false;
#endif
    }

    // Danny - changed this because writing a single large chunk (more than 6MB?) causes
    // Windows fwrite to (silently?!) fail. If it's big then break up the write into
    // chunks
    const unsigned long maxChunk = 1 << 20;
    unsigned long sizeLeft = size;
    unsigned long sizeToWrite;
    char* ptr = (char*)data;
    while ((sizeToWrite = (std::min)(sizeLeft, maxChunk)) != 0)
    {
        if (!AZ::IO::FileIOBase::GetDirectInstance()->Write(m_fileHandle, ptr, sizeToWrite))
        {
            return false;
        }

        ptr += sizeToWrite;
        sizeLeft -= sizeToWrite;
    }

    return true;
}

bool ZipDir::CacheRW::WriteNullData(size_t size)
{
    // Danny - changed this because writing a single large chunk (more than 6MB?) causes
    // Windows fwrite to (silently?!) fail. If it's big then break up the write into
    // chunks
    const unsigned long maxChunk = 1 << 20;
    unsigned long sizeLeft = size;
    unsigned long sizeToWrite;
    char* ptr = new char[maxChunk];
    if (NULL == ptr)
    {
        return ZD_ERROR_IO_FAILED;
    }

    memset(ptr, 0, sizeof(char) * maxChunk);

    while ((sizeToWrite = (std::min)(sizeLeft, maxChunk)) != 0)
    {
        if (!AZ::IO::FileIOBase::GetDirectInstance()->Write(m_fileHandle, ptr, sizeToWrite))
        {
            delete[] ptr;
            return false;
        }
        sizeLeft -= sizeToWrite;
    }

    delete [] ptr;
    return true;
}

//////////////////////////////////////////////////////////////////////////
char* ZipDir::CacheRW::AllocPath(const char* pPath)
{
    char str[_MAX_PATH];
    char* temp = UnifyPath(str, pPath);
    temp = m_tempStringPool.Append(temp, strlen(temp));
    return temp;
}

// Adds a new file to the zip or update an existing one
// adds a directory (creates several nested directories if needed)
ZipDir::ErrorEnum ZipDir::CacheRW::UpdateFile (const char* szRelativePathSrc, void* pUncompressed, unsigned nSize, unsigned nCompressionMethod, int nCompressionLevel)
{
    char str[_MAX_PATH];
    char* szRelativePath = UnifyPath(str, szRelativePathSrc);

    SmartPtr pBufferDestroyer(m_pHeap);

    // we'll need the compressed data
    void* pCompressed;
    unsigned long nSizeCompressed;
    int nError;

    if (nSize == 0)
    {
        nCompressionMethod = ZipFile::METHOD_STORE;
    }

    switch (nCompressionMethod)
    {
    case ZipFile::METHOD_DEFLATE:
        // allocate memory for compression. Min is nSize * 1.001 + 12
        nSizeCompressed = nSize + (nSize >> 3) + 32;
        pCompressed = m_pHeap->TempAlloc(nSizeCompressed, "ZipDir::CacheRW::UpdateFile");
        pBufferDestroyer.Attach(pCompressed);
        nError = ZipRawCompress(m_pHeap, pUncompressed, &nSizeCompressed, pCompressed, nSize, nCompressionLevel);
        if (Z_OK != nError)
        {
            return ZD_ERROR_ZLIB_FAILED;
        }
        break;

    case ZipFile::METHOD_STORE:
        pCompressed = pUncompressed;
        nSizeCompressed = nSize;
        break;

    default:
        return ZD_ERROR_UNSUPPORTED;
    }

    // create or find the file entry.. this object will rollback (delete the object
    // if the operation fails) if needed.
    FileEntryTransactionAdd pFileEntry(this, AllocPath(szRelativePathSrc));

    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_PATH;
    }

    pFileEntry->OnNewFileData (pUncompressed, nSize, nSizeCompressed, nCompressionMethod, false);
    // since we changed the time, we'll have to update CDR
    m_nFlags |= FLAGS_CDR_DIRTY;

    // the new CDR position, if the operation completes successfully
    unsigned lNewCDROffset = m_lCDROffset;

    if (pFileEntry->IsInitialized())
    {
        // this file entry is already allocated in CDR

        // check if the new compressed data fits into the old place
        unsigned nFreeSpace = pFileEntry->nEOFOffset - pFileEntry->nFileHeaderOffset - (unsigned)sizeof(ZipFile::LocalFileHeader) - (unsigned)strlen(szRelativePath);

        if (nFreeSpace != nSizeCompressed)
        {
            m_nFlags |= FLAGS_UNCOMPACTED;
        }

        if (nFreeSpace >= nSizeCompressed)
        {
            // and we can just override the compressed data in the file
            ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, szRelativePath, m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA);
            if (e != ZD_ERROR_SUCCESS)
            {
                return e;
            }
        }
        else
        {
            // we need to write the file anew - in place of current CDR
            pFileEntry->nFileHeaderOffset = m_lCDROffset;
            ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, szRelativePath, m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA);
            lNewCDROffset = pFileEntry->nEOFOffset;
            if (e != ZD_ERROR_SUCCESS)
            {
                return e;
            }
        }
    }
    else
    {
        pFileEntry->nFileHeaderOffset = m_lCDROffset;
        ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, szRelativePath, m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA);
        if (e != ZD_ERROR_SUCCESS)
        {
            return e;
        }

        lNewCDROffset = pFileEntry->nFileDataOffset + nSizeCompressed;

        m_nFlags |= FLAGS_CDR_DIRTY;
    }

    // now we have the fresh local header and data offset
    if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileDataOffset, AZ::IO::SeekType::SeekFromStart))
    {
        return ZD_ERROR_IO_FAILED;
    }
    // Danny - changed this because writing a single large chunk (more than 6MB?) causes
    // Windows fwrite to (silently?!) fail. If it's big then break up the write into
    // chunks
    const unsigned long maxChunk = 1 << 20;
    unsigned long sizeLeft = nSizeCompressed;
    unsigned long sizeToWrite;
    char* ptr = (char*) pCompressed;
    while ((sizeToWrite = min(sizeLeft, maxChunk)) != 0)
    {
        if (!AZ::IO::FileIOBase::GetDirectInstance()->Write(m_fileHandle, ptr, sizeToWrite))
        {
            char error[1024];
            azstrerror_s(error, 1024, errno);
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR,
                "Cannot write to zip file!! error = (%d): %s", errno, error);
            return ZD_ERROR_IO_FAILED;
        }
        ptr += sizeToWrite;
        sizeLeft -= sizeToWrite;
    }


    //  if (fseek (m_pFile, pFileEntry->nFileDataOffset, SEEK_SET)!=0
    //      ||fwrite (pCompressed, nSizeCompressed, 1, m_pFile) != 1)
    //  if (fseek (m_pFile, pFileEntry->nFileDataOffset, SEEK_SET)!=0
    //      ||fwrite (pCompressed, 1, nSizeCompressed, m_pFile) != nSizeCompressed)
    //  {
    // we need to rollback the transaction: file write failed in the middle, the old
    // data is unrecoverably damaged, and we need to destroy this file entry
    // the CDR is already marked dirty
    //      return ZD_ERROR_IO_FAILED;
    //  }

    // since we wrote the file successfully, update the new CDR position
    m_lCDROffset = lNewCDROffset;
    pFileEntry.Commit();

    return ZD_ERROR_SUCCESS;
}

//   Adds a new file to the zip or update an existing one if it is not compressed - just stored  - start a big file
ZipDir::ErrorEnum ZipDir::CacheRW::StartContinuousFileUpdate(const char* szRelativePathSrc, unsigned nSize)
{
    char str[_MAX_PATH];
    char* szRelativePath = UnifyPath(str, szRelativePathSrc);

    SmartPtr pBufferDestroyer(m_pHeap);

    // create or find the file entry.. this object will rollback (delete the object
    // if the operation fails) if needed.
    FileEntryTransactionAdd pFileEntry(this, AllocPath(szRelativePathSrc));

    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_PATH;
    }

    pFileEntry->OnNewFileData (NULL, nSize, nSize, ZipFile::METHOD_STORE, false);
    // since we changed the time, we'll have to update CDR
    m_nFlags |= FLAGS_CDR_DIRTY;

    // the new CDR position, if the operation completes successfully
    unsigned lNewCDROffset = m_lCDROffset;
    if (pFileEntry->IsInitialized())
    {
        // check if the new compressed data fits into the old place
        unsigned nFreeSpace = pFileEntry->nEOFOffset - pFileEntry->nFileHeaderOffset - (unsigned)sizeof(ZipFile::LocalFileHeader) - (unsigned)strlen(szRelativePath);

        if (nFreeSpace != nSize)
        {
            m_nFlags |= FLAGS_UNCOMPACTED;
        }

        if (nFreeSpace >= nSize)
        {
            // and we can just override the compressed data in the file
            ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, szRelativePath, m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA);
            if (e != ZD_ERROR_SUCCESS)
            {
                return e;
            }
        }
        else
        {
            // we need to write the file anew - in place of current CDR
            pFileEntry->nFileHeaderOffset = m_lCDROffset;
            ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, szRelativePath, m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA);
            lNewCDROffset = pFileEntry->nEOFOffset;
            if (e != ZD_ERROR_SUCCESS)
            {
                return e;
            }
        }
    }
    else
    {
        pFileEntry->nFileHeaderOffset = m_lCDROffset;
        ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, szRelativePath, m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA);
        if (e != ZD_ERROR_SUCCESS)
        {
            return e;
        }

        lNewCDROffset = pFileEntry->nFileDataOffset + nSize;

        m_nFlags |= FLAGS_CDR_DIRTY;
    }

    if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileDataOffset, AZ::IO::SeekType::SeekFromStart))
    {
        return ZD_ERROR_IO_FAILED;
    }

    if (!WriteNullData(nSize))
    {
        return ZD_ERROR_IO_FAILED;
    }

    pFileEntry->nEOFOffset = pFileEntry->nFileDataOffset;

    // since we wrote the file successfully, update the new CDR position
    m_lCDROffset = lNewCDROffset;
    pFileEntry.Commit();

    return ZD_ERROR_SUCCESS;
}

// Adds a new file to the zip or update an existing's segment if it is not compressed - just stored
// adds a directory (creates several nested directories if needed)
ZipDir::ErrorEnum ZipDir::CacheRW::UpdateFileContinuousSegment (const char* szRelativePathSrc, unsigned nSize, void* pUncompressed, unsigned nSegmentSize, unsigned nOverwriteSeekPos)
{
    char str[_MAX_PATH];
    char* szRelativePath = UnifyPath(str, szRelativePathSrc);

    SmartPtr pBufferDestroyer(m_pHeap);

    // create or find the file entry.. this object will rollback (delete the object
    // if the operation fails) if needed.
    FileEntryTransactionAdd pFileEntry(this, AllocPath(szRelativePathSrc));

    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_PATH;
    }

    pFileEntry->OnNewFileData (pUncompressed, nSegmentSize, nSegmentSize, ZipFile::METHOD_STORE, true);
    // since we changed the time, we'll have to update CDR
    m_nFlags |= FLAGS_CDR_DIRTY;

    // this file entry is already allocated in CDR
    unsigned lSegmentOffset = pFileEntry->nEOFOffset;

    if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileHeaderOffset, AZ::IO::SeekType::SeekFromStart))
    {
        return ZD_ERROR_IO_FAILED;
    }

    // and we can just override the compressed data in the file
    ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, szRelativePath, m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA);
    if (e != ZD_ERROR_SUCCESS)
    {
        return e;
    }

    if (nOverwriteSeekPos != 0xffffffff)
    {
        lSegmentOffset = pFileEntry->nFileDataOffset + nOverwriteSeekPos;
    }

    if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, lSegmentOffset, AZ::IO::SeekType::SeekFromStart))
    {
        return ZD_ERROR_IO_FAILED;
    }

    const bool encrypt = false; // we do not support encription for continous file update
    if (!WriteCompressedData((char*)pUncompressed, nSegmentSize, encrypt))
    {
        char error[1024];
        azstrerror_s(error, 1024, errno);
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR,
            "Cannot write to zip file!! error = (%d): %s", errno, error);
        return ZD_ERROR_IO_FAILED;
    }

    if (nOverwriteSeekPos == 0xffffffff)
    {
        pFileEntry->nEOFOffset = lSegmentOffset + nSegmentSize;
    }

    // since we wrote the file successfully, update CDR
    pFileEntry.Commit();
    return ZD_ERROR_SUCCESS;
}


ZipDir::ErrorEnum ZipDir::CacheRW::UpdateFileCRC (const char* szRelativePathSrc, unsigned dwCRC32)
{
    char str[_MAX_PATH];
    char* szRelativePath = UnifyPath(str, szRelativePathSrc);

    SmartPtr pBufferDestroyer(m_pHeap);

    // create or find the file entry.. this object will rollback (delete the object
    // if the operation fails) if needed.
    FileEntryTransactionAdd pFileEntry(this, AllocPath(szRelativePathSrc));

    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_PATH;
    }

    // since we changed the time, we'll have to update CDR
    m_nFlags |= FLAGS_CDR_DIRTY;

    pFileEntry->desc.lCRC32 = dwCRC32;

    if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileHeaderOffset, AZ::IO::SeekType::SeekFromStart))
    {
        return ZD_ERROR_IO_FAILED;
    }

    // and we can just override the compressed data in the file
    ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, szRelativePath, m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA);
    if (e != ZD_ERROR_SUCCESS)
    {
        return e;
    }

    // since we wrote the file successfully, update
    pFileEntry.Commit();
    return ZD_ERROR_SUCCESS;
}


// deletes the file from the archive
ZipDir::ErrorEnum ZipDir::CacheRW::RemoveFile (const char* szRelativePathSrc)
{
    char str[_MAX_PATH];
    char* szRelativePath = UnifyPath(str, szRelativePathSrc);

    // find the last slash in the path
    const char* pSlash = max(strrchr(szRelativePath, '/'), strrchr(szRelativePath, '\\'));

    const char* pFileName; // the name of the file to delete

    FileEntryTree* pDir; // the dir from which the subdir will be deleted

    if (pSlash)
    {
        FindDirRW fd (GetRoot());
        // the directory to remove
        pDir = fd.FindExact(string (szRelativePath, pSlash - szRelativePath).c_str());
        if (!pDir)
        {
            return ZD_ERROR_DIR_NOT_FOUND;// there is no such directory
        }
        pFileName = pSlash + 1;
    }
    else
    {
        pDir = GetRoot();
        pFileName = szRelativePath;
    }

    ErrorEnum e = pDir->RemoveFile (pFileName);
    if (e == ZD_ERROR_SUCCESS)
    {
        m_nFlags |= FLAGS_UNCOMPACTED | FLAGS_CDR_DIRTY;
    }
    return e;
}


// deletes the directory, with all its descendants (files and subdirs)
ZipDir::ErrorEnum ZipDir::CacheRW::RemoveDir (const char* szRelativePathSrc)
{
    char str[_MAX_PATH];
    char* szRelativePath = UnifyPath(str, szRelativePathSrc);

    // find the last slash in the path
    const char* pSlash = max(strrchr(szRelativePath, '/'), strrchr(szRelativePath, '\\'));

    const char* pDirName; // the name of the dir to delete

    FileEntryTree* pDir; // the dir from which the subdir will be deleted

    if (pSlash)
    {
        FindDirRW fd (GetRoot());
        // the directory to remove
        pDir = fd.FindExact(string (szRelativePath, pSlash - szRelativePath).c_str());
        if (!pDir)
        {
            return ZD_ERROR_DIR_NOT_FOUND;// there is no such directory
        }
        pDirName = pSlash + 1;
    }
    else
    {
        pDir = GetRoot();
        pDirName = szRelativePath;
    }

    ErrorEnum e = pDir->RemoveDir (pDirName);
    if (e == ZD_ERROR_SUCCESS)
    {
        m_nFlags |= FLAGS_UNCOMPACTED | FLAGS_CDR_DIRTY;
    }
    return e;
}

// deletes all files and directories in this archive
ZipDir::ErrorEnum ZipDir::CacheRW::RemoveAll()
{
    ErrorEnum e = m_treeDir.RemoveAll();
    if (e == ZD_ERROR_SUCCESS)
    {
        m_nFlags |= FLAGS_UNCOMPACTED | FLAGS_CDR_DIRTY;
    }
    return e;
}

ZipDir::ErrorEnum ZipDir::CacheRW::ReadFile (FileEntry* pFileEntry, void* pCompressed, void* pUncompressed)
{
    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_CALL;
    }

    if (pFileEntry->desc.lSizeUncompressed == 0)
    {
        // note that even in a compressed zip file, 0 bytes MUST be stored using "STORE" compression and thus will remain 0
        assert (pFileEntry->desc.lSizeCompressed == 0);
        return ZD_ERROR_SUCCESS;
    }

    assert (pFileEntry->desc.lSizeCompressed > 0);

    ErrorEnum nError = Refresh(pFileEntry);
    if (nError != ZD_ERROR_SUCCESS)
    {
        return nError;
    }

    if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileDataOffset, AZ::IO::SeekType::SeekFromStart))
    {
        return ZD_ERROR_IO_FAILED;
    }

    SmartPtr pBufferDestroyer(m_pHeap);

    void* pBuffer = pCompressed; // the buffer where the compressed data will go

    if (pFileEntry->nMethod == 0 && pUncompressed)
    {
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

        pBuffer = m_pHeap->TempAlloc(pFileEntry->desc.lSizeCompressed, "ZipDir::Cache::ReadFile");
        pBufferDestroyer.Attach(pBuffer); // we want it auto-freed once we return
    }

    if (!AZ::IO::FileIOBase::GetDirectInstance()->Read(m_fileHandle, pBuffer, pFileEntry->desc.lSizeCompressed, true))
    {
        return ZD_ERROR_IO_FAILED;
    }

    //Encryption techniques are stored on a per-file basis
    if (pFileEntry->IsEncrypted())
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
            ZipDir::Decrypt((char*)pBuffer, pFileEntry->desc.lSizeCompressed);
        }
#endif //SUPPORT_XTEA_PAK_ENCRYPTION
#if !defined(OPTIMIZED_READONLY_ZIP_ENTRY) && defined(SUPPORT_STREAMCIPHER_PAK_ENCRYPTION)
        else if (pFileEntry->nMethod == METHOD_DEFLATE_AND_STREAMCIPHER)
        {
            ZipDir::StreamCipher((char*)pBuffer, pFileEntry);
        }
#endif  //!OPTIMIZED_READONLY_ZIP_ENTRY && SUPPORT_STREAMCIPHER_PAK_ENCRYPTION
        else
        {
#if !defined(_RELEASE)
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Tried to load a file from an archive using an unsupported method");
#endif  //!_RELEASE
            return ZD_ERROR_CORRUPTED_DATA;
        }
    }

    // if there's a buffer for uncompressed data, uncompress it to that buffer
    if (pUncompressed)
    {
        if (pFileEntry->nMethod == 0)
        {
            assert (pBuffer == pUncompressed);
            //assert (pFileEntry->nSizeCompressed == pFileEntry->nSizeUncompressed);
            //memcpy (pUncompressed, pBuffer, pFileEntry->nSizeCompressed);
        }
        else
        {
            unsigned long nSizeUncompressed = pFileEntry->desc.lSizeUncompressed;
            if (Z_OK != ZipRawUncompress(m_pHeap, pUncompressed, &nSizeUncompressed, pBuffer, pFileEntry->desc.lSizeCompressed))
            {
                return ZD_ERROR_CORRUPTED_DATA;
            }
        }
    }

    return ZD_ERROR_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// finds the file by exact path
ZipDir::FileEntry* ZipDir::CacheRW::FindFile (const char* szPathSrc, bool bFullInfo)
{
    char str[_MAX_PATH];
    char* szPath = UnifyPath(str, szPathSrc);

    ZipDir::FindFileRW fd (GetRoot());
    if (!fd.FindExact(szPath))
    {
        assert (!fd.GetFileEntry());
        return NULL;
    }
    assert (fd.GetFileEntry());
    return fd.GetFileEntry();
}

// returns the size of memory occupied by the instance referred to by this cache
size_t ZipDir::CacheRW::GetSize() const
{
    return sizeof(*this) + m_strFilePath.capacity() + m_treeDir.GetSize() - sizeof(m_treeDir);
}

// loads and unpacks the file into a newly created buffer (that must be subsequently freed with
// Free()) Returns NULL if failed
void* ZipDir::CacheRW::AllocAndReadFile (FileEntry* pFileEntry)
{
    if (!pFileEntry)
    {
        return NULL;
    }

    void* pData = m_pHeap->TempAlloc(pFileEntry->desc.lSizeUncompressed, "ZipDir::CacheRW::AllocAndReadFile");
    if (pData)
    {
        if (ZD_ERROR_SUCCESS != ReadFile (pFileEntry, NULL, pData))
        {
            m_pHeap->FreeTemporary(pData);
            pData = NULL;
        }
    }
    return pData;
}

// refreshes information about the given file entry into this file entry
ZipDir::ErrorEnum ZipDir::CacheRW::Refresh (FileEntry* pFileEntry)
{
    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_CALL;
    }

    if (pFileEntry->nFileDataOffset != pFileEntry->INVALID_DATA_OFFSET)
    {
        return ZD_ERROR_SUCCESS; // the data offset has been successfully read..
    }
    CZipFile tmp;
    tmp.m_fileHandle = m_fileHandle;
    return ZipDir::Refresh(&tmp, pFileEntry);
}


// writes the CDR to the disk
bool ZipDir::CacheRW::WriteCDR(AZ::IO::HandleType fTarget)
{
    if (fTarget == AZ::IO::InvalidHandle)
    {
        return false;
    }

    if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(fTarget, m_lCDROffset, AZ::IO::SeekType::SeekFromStart))
    {
        return false;
    }

    FileRecordList arrFiles(GetRoot());
    //arrFiles.SortByFileOffset();
    size_t nSizeCDR = arrFiles.GetStats().nSizeCDR;
    void* pCDR = m_pHeap->TempAlloc(nSizeCDR, "ZipDir::CacheRW::WriteCDR");
    size_t nSizeCDRSerialized = arrFiles.MakeZipCDR(m_lCDROffset, pCDR, m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA);
    assert (nSizeCDRSerialized == nSizeCDR);
    if (m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA)
    {
#if defined(SUPPORT_XTEA_PAK_ENCRYPTION)
        // We do not encrypt CDREnd, so we can find it by signature
        ZipDir::Encrypt((char*)pCDR, nSizeCDR - sizeof(ZipFile::CDREnd));
#else
#if !defined(_RELEASE)
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR_DBGBRK, "Attempt to use XTEA pak encryption when it is disabled.");
#endif
        m_pHeap->FreeTemporary(pCDR);
        return false;
#endif
    }

    bool success = AZ::IO::FileIOBase::GetDirectInstance()->Write(fTarget, pCDR, nSizeCDR);
    m_pHeap->FreeTemporary(pCDR);
    return success;
}

// generates random file name
string ZipDir::CacheRW::GetRandomName(int nAttempt)
{
    if (nAttempt)
    {
        char szBuf[8];
        int i;
        for (i = 0; i < sizeof(szBuf) - 1; ++i)
        {
            int r = cry_random(0, 10 + 'z' - 'a');
            szBuf[i] = r > 9 ? (r - 10) + 'a' : '0' + r;
        }
        szBuf[i] = '\0';
        return szBuf;
    }
    else
    {
        return string();
    }
}

bool ZipDir::CacheRW::RelinkZip()
{
    AZ::IO::FileIOBase* fileSystem = AZ::IO::FileIOBase::GetDirectInstance();

    for (int nAttempt = 0; nAttempt < 32; ++nAttempt)
    {
        string strNewFilePath = m_strFilePath + "$" + GetRandomName(nAttempt);
        if (CryGetFileAttributes (strNewFilePath.c_str()) != ~0)
        {
            continue; //  we don't want to overwrite the old temp files for safety reasons
        }
        AZ::IO::HandleType fileHandle;
        if (fileSystem->Open(strNewFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, fileHandle))
        {
            bool bOk = RelinkZip(fileHandle);
            fileSystem->Close(fileHandle);

            if (!bOk)
            {
                // we don't need the temporary file
                fileSystem->Remove(strNewFilePath.c_str());
                return false;
            }

            // we successfully relinked, now copy the temporary file to the original file
            fileSystem->Close(m_fileHandle);
            m_fileHandle = AZ::IO::InvalidHandle;

            fileSystem->Remove(m_strFilePath.c_str());
            if (AZ::IO::Move(strNewFilePath.c_str(), m_strFilePath.c_str()))
            {
                // successfully renamed - reopen
                return fileSystem->Open(m_strFilePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeUpdate | AZ::IO::OpenMode::ModeBinary, m_fileHandle);
            }
            else
            {
                // could not rename
                return false;
            }
        }
    }

    // couldn't open temp file
    return false;
}

bool ZipDir::CacheRW::RelinkZip(AZ::IO::HandleType fTmp)
{
    FileRecordList arrFiles(GetRoot());
    arrFiles.SortByFileOffset();
    FileRecordList::ZipStats Stats = arrFiles.GetStats();

    // we back up our file entries, because we'll need to restore them
    // in case the operation fails
    std::vector<FileEntry> arrFileEntryBackup;
    arrFiles.Backup (arrFileEntryBackup);

    // this is the set of files that are to be written out - compressed data and the file record iterator
    std::vector<FileDataRecordPtr> queFiles;
    queFiles.reserve (g_nMaxItemsRelinkBuffer);

    AZ::IO::FileIOBase* fileSystem = AZ::IO::FileIOBase::GetDirectInstance();

    // the total size of data in the queue
    unsigned nQueueSize = 0;

    for (FileRecordList::iterator it = arrFiles.begin(); it != arrFiles.end(); ++it)
    {
        // find the file data offset
        if (ZD_ERROR_SUCCESS != Refresh (it->pFileEntry))
        {
            return false;
        }

        // go to the file data
        if (!fileSystem->Seek(m_fileHandle, it->pFileEntry->nFileDataOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return false;
        }

        // allocate memory for the file compressed data
        FileDataRecordPtr pFile = FileDataRecord::New (*it, m_pHeap);

        if (!pFile)
        {
            return false;
        }

        // read the compressed data
        if (it->pFileEntry->desc.lSizeCompressed && !fileSystem->Read(m_fileHandle, pFile->GetData(), it->pFileEntry->desc.lSizeCompressed, true))
        {
            return false;
        }

        //Encryption techniques are stored on a per-file basis
        if (it->pFileEntry->IsEncrypted())
        {
            if (0)
            {
                //Intentionally empty block
            }
#ifdef SUPPORT_RSA_AND_STREAMCIPHER_PAK_ENCRYPTION
            else if (it->pFileEntry->nMethod == ZipFile::METHOD_STORE_AND_STREAMCIPHER_KEYTABLE || it->pFileEntry->nMethod == ZipFile::METHOD_DEFLATE_AND_STREAMCIPHER_KEYTABLE)
            {
                unsigned char IV[ZipFile::BLOCK_CIPHER_KEY_LENGTH]; //16 byte
                int nKeyIndex = ZipEncrypt::GetEncryptionKeyIndex(it->pFileEntry);
                ZipEncrypt::GetEncryptionInitialVector(it->pFileEntry, IV);
                if (!ZipEncrypt::DecryptBufferWithStreamCipher((unsigned char*)(char*)pFile->GetData(), it->pFileEntry->desc.lSizeCompressed, m_block_cipher_keys_table[nKeyIndex], IV))
                {
                    // Decryption of CDR failed.
                    return false;
                }
            }
#endif  //SUPPORT_RSA_AND_STREAMCIPHER_PAK_ENCRYPTION
#if defined(SUPPORT_XTEA_PAK_ENCRYPTION)
            else if (it->pFileEntry->nMethod == METHOD_DEFLATE_AND_ENCRYPT)
            {
                ZipDir::Decrypt((char*)pFile->GetData(), it->pFileEntry->desc.lSizeCompressed)
            }
#endif //SUPPORT_XTEA_PAK_ENCRYPTION
#if !defined(OPTIMIZED_READONLY_ZIP_ENTRY) && defined(SUPPORT_STREAMCIPHER_PAK_ENCRYPTION)
            else if (it->pFileEntry->nMethod == METHOD_DEFLATE_AND_STREAMCIPHER)
            {
                ZipDir::StreamCipher((char*)pFile->GetData(), it->pFileEntry);
            }
#endif //!OPTIMIZED_READONLY_ZIP_ENTRY && SUPPORT_STREAMCIPHER_PAK_ENCRYPTION
            else
            {
#if !defined(_RELEASE)
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Tried to load a file from an archive using an unsupported method");
#endif  //!_RELEASE
                return false;
            }
        }

        // put the file into the queue for copying (writing)
        queFiles.push_back(pFile);
        nQueueSize += it->pFileEntry->desc.lSizeCompressed;

        // if the queue is big enough, write it out
        if (nQueueSize > g_nSizeRelinkBuffer || queFiles.size() >= g_nMaxItemsRelinkBuffer)
        {
            nQueueSize = 0;
            if (!WriteZipFiles(queFiles, fTmp))
            {
                return false;
            }
        }
    }

    if (!WriteZipFiles(queFiles, fTmp))
    {
        return false;
    }

    uint32 lOldCDROffset = m_lCDROffset;
    // the file data has now been written out. Now write the CDR
    AZ::u64 tellOffset = 0;
    fileSystem->Tell(fTmp, tellOffset);
    m_lCDROffset = static_cast<uint32>(tellOffset);

    if (WriteCDR(fTmp) && AZ::IO::FileIOBase::GetDirectInstance()->Flush(fTmp))
    {
        // the new file positions are already there - just discard the backup and return
        return true;
    }
    // recover from backup
    arrFiles.Restore (arrFileEntryBackup);
    m_lCDROffset = lOldCDROffset;
    return false;
}

// writes out the file data in the queue into the given file. Empties the queue
bool ZipDir::CacheRW::WriteZipFiles(std::vector<FileDataRecordPtr>& queFiles, AZ::IO::HandleType fTmp)
{
    AZ::IO::FileIOBase* fileSystem = AZ::IO::FileIOBase::GetDirectInstance();
    for (std::vector<FileDataRecordPtr>::iterator it = queFiles.begin(); it != queFiles.end(); ++it)
    {
        // set the new header offset to the file entry - we won't need it
        AZ::u64 tellOffset = 0;
        fileSystem->Tell(fTmp, tellOffset);
        (*it)->pFileEntry->nFileHeaderOffset = static_cast<uint32>(tellOffset);

        // while writing the local header, the data offset will also be calculated
        if (ZD_ERROR_SUCCESS != WriteLocalHeader(fTmp, (*it)->pFileEntry, (*it)->strPath.c_str(), m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA))
        {
            return false;
        }

        // write the compressed file data
        if ((*it)->pFileEntry->desc.lSizeCompressed && !fileSystem->Write(fTmp, (*it)->GetData(), (*it)->pFileEntry->desc.lSizeCompressed))
        {
            return false;
        }

        tellOffset = 0;
        fileSystem->Tell(fTmp, tellOffset);
        assert((*it)->pFileEntry->nEOFOffset == tellOffset);
    }
    queFiles.clear();
    queFiles.reserve (g_nMaxItemsRelinkBuffer);
    return true;
}

#endif //OPTIMIZED_READONLY_ZIP_ENTRY
