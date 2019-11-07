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

//////////////////////////////////////////////////////////////////////////
// Declarations of the class used to parse and cache Zipped directory.
// This class is actually an auto-pointer to the instance of the cache, so it can
// be easily passed by value.
// The cache instance contains the optimized for memory usage and fast search tree
// of the files/directories inside the zip; each file has a descriptor with the
// info about where its compressed data lies within the file

#ifndef CRYINCLUDE_CRYSYSTEM_ZIPDIRCACHE_H
#define CRYINCLUDE_CRYSYSTEM_ZIPDIRCACHE_H
#pragma once


/////////////////////////////////////////////////////////////
// THe Zip Dir uses a special memory layout for keeping the structure of zip file.
// This layout is optimized for small memory footprint (for big zip files)
// and quick binary-search access to the individual files.
//
// The serialized layout consists of a number of directory records.
// Each directory record starts with the DirHeader structure, then
// it has an array of DirEntry structures (sorted by name),
// array of FileEntry structures (sorted by name) and then
// the pool of names, followed by pad bytes to align the whole directory
// record on 4-byte boundray.

struct FileExt;

namespace ZipDir
{
    // this is the header of the instance data allocated dynamically
    // it contains the AZ::IO::HandleType : it owns it and closes upon destruction
    struct Cache
    {
        void AddRef() { CryInterlockedIncrement(m_pRefCount); }
        void Release()
        {
            const int nRefs = CryInterlockedDecrement(m_pRefCount);
            assert(nRefs >= 0);
            if (nRefs == 0)
            {
                Delete();
            }
        }
        int  NumRefs() const { return *m_pRefCount; }

        // looks for the given file record in the Central Directory. If there's none, returns NULL.
        // if there is some, returns the pointer to it.
        // the Path must be the relative path to the file inside the Zip
        // if the file handle is passed, it will be used to find the file data offset, if one hasn't been initialized yet
        // if bFull is true, then the full information about the file is returned (the offset to the data may be unknown at this point)-
        // if needed, the file is accessed and the information is loaded
        FileEntry* FindFile (const char* szPath, bool bFullInfo = false);

        // loads the given file into the pCompressed buffer (the actual compressed data)
        // if the pUncompressed buffer is supplied, uncompresses the data there
        // buffers must have enough memory allocated, according to the info in the FileEntry
        // NOTE: there's no need to decompress if the method is 0 (store)
        // returns 0 if successful or error code if couldn't do something
        // when nDataReadSize and nDataOffset are 0, will assume whole file must be read.
        ErrorEnum ReadFile (FileEntry* pFileEntry, void* pCompressed, void* pUncompressed, const bool decompress = true, int64 nDataOffset = 0, int64 nDataReadSize = -1, const bool decrypt = true);
        ErrorEnum ReadFileStreaming (FileEntry* pFileEntry, void* pOut, int64 nDataOffset, int64 nDataReadSize);

        // decompress compressed file
        ErrorEnum DecompressFile (FileEntry* pFileEntry, void* pCompressed, void* pUncompressed);

        // loads and unpacks the file into a newly created buffer (that must be subsequently freed with
        // Free()) Returns NULL if failed
        void* AllocAndReadFile (FileEntry* pFileEntry);

        // frees the memory block that was previously allocated by AllocAndReadFile
        void Free (void*);

        // refreshes information about the given file entry into this file entry
        ErrorEnum Refresh (FileEntry* pFileEntry);

        // Return FileEntity data offset inside zip file.
        uint32 GetFileDataOffset(FileEntry* pFileEntry);

        const char* GetFileEntryName(FileEntry* pFileEntry);

        // returns the root directory record;
        // through this directory record, user can traverse the whole tree
        DirHeader* GetRoot() const { return m_pRootData; }

        void* GetDataPointer() const { return (DirHeader*)(this + 1); }

        // returns the size of memory occupied by the instance referred to by this cache
        // must be exact, because it's used by CacheRW to reallocate this cache
        size_t GetSize() const;

        // QUICK check to determine whether the file entry belongs to this object
        bool IsOwnerOf (const FileEntry* pFileEntry) const;

        // returns the string - path to the zip file from which this object was constructed.
        // this will be "" if the object was constructed with a factory that wasn't created with FLAGS_MEMORIZE_ZIP_PATH
        const char* GetFilePath() const
        {
            return ((const char*)(this + 1)) + m_nZipPathOffset;
        }

        ILINE AZ::IO::HandleType GetFileHandle() { return m_zipFile.m_fileHandle; }

        size_t GetZipFileSize() const { return m_nFileSize; };
        void   SetZipFileSize(size_t nSize) { m_nFileSize = nSize; };

        bool ReOpen(const char* filePath);

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            // to account for the full memory, see ZipDir::CacheFactory::MakeCache for this cause of this calculation
            pSizer->AddObject(this, m_pCacheData->m_pHeap->PersistentAllocSize(m_nAllocatedSize));
            pSizer->AddObject(m_pCacheData);
        }

        bool IsInMemory() const
        {
            return m_zipFile.IsInMemory();
        }

        //explicitly sets the priority
        uint64 SetPakFileOffsetOnMedia(uint64 off) { m_nPakFileOffsetOnMedia = off; return m_nPakFileOffsetOnMedia; }
        uint64 GetPakFileOffsetOnMedia() { return m_nPakFileOffsetOnMedia; }

        //offsets for cdr data in a .pak file for anti-cheat purposes
        void SetCDROffsetSize(uint32 offset, uint32 size) { m_cdrOffset = offset; m_cdrSize = size; }
        void GetCDROffsetSize(uint32& offset, uint32& size) { offset = m_cdrOffset; size = m_cdrSize; }

        friend class CacheFactory; // the factory class creates instances of this class
        friend class CacheRW; // the Read-Write 2-way cache can modify this cache directly during write operations


    public:
#ifndef _RELEASE
        // Validate that other zip cache do not have duplicate file hashes
        static void ValidateFilenameHash(uint32 nHash, const char* filename, const char* pakFile);
#endif

        ZipFile::CryCustomEncryptionHeader& GetEncryptionHeader() { return m_headerEncryption; }
        ZipFile::CrySignedCDRHeader& GetSignedHeader() { return m_headerSignature; }
        ZipFile::CryCustomExtendedHeader& GetExtendedHeader() { return m_headerExtended; }
#ifdef SUPPORT_RSA_AND_STREAMCIPHER_PAK_ENCRYPTION
        unsigned char* GetBlockCipherKeyTable(const int index) { return m_block_cipher_keys_table[index]; }
#endif

    protected:
        volatile signed int* m_pRefCount; // the reference count
        CZipFile m_zipFile; // the opened file
        uint64 m_nPakFileOffsetOnMedia;

        size_t m_nFileSize;

        // Flags from Cache Factory
        unsigned int m_nCacheFactoryFlags;

        // the size of the serialized data following this instance (not including the extra fields after the serialized tree data)
        size_t m_nDataSize;

        // actual size of allocated memory(for correct tracking)
        size_t m_nAllocatedSize;

        // the offset to the path/name of the zip file relative to (char*)(this+1) pointer in bytes
        size_t m_nZipPathOffset;

        uint32  m_cdrOffset;
        uint32  m_cdrSize;

        // Zip Headers
        ZipFile::CryCustomEncryptionHeader m_headerEncryption;
        ZipFile::CrySignedCDRHeader m_headerSignature;
        ZipFile::CryCustomExtendedHeader m_headerExtended;

#ifdef SUPPORT_RSA_AND_STREAMCIPHER_PAK_ENCRYPTION
        unsigned char m_block_cipher_keys_table[ZipFile::BLOCK_CIPHER_NUM_KEYS][ZipFile::BLOCK_CIPHER_KEY_LENGTH];
#endif

        DirHeader* m_pRootData;

        // cache internal data
        // need to assemble into one struct to have pointer on it
        struct CacheData
        {
            // compatible with zlib memory-management functions used both
            // to allocate/free this instance and for decompression operations
            CMTSafeHeap* m_pHeap;

            // file I/O guard: guarantees thread-safe read/seek/write operations and safe header reading.
            // The lock should be globally unique when all pak files are archived in a single obb file.
            CryCriticalSection m_csCacheIOLock;

            void GetMemoryUsage(ICrySizer* pSizer) const
            {
                pSizer->AddObject(this, sizeof(*this));
            }
        };

        // need to have pointer on the structure
        // because of fixed size of the class
        CacheData*      m_pCacheData;
        ZipFile::EHeaderEncryptionType m_encryptedHeaders;
        ZipFile::EHeaderSignatureType m_signedHeaders;

    public:
        // initializes the instance structure
        void Construct(CZipFile& fNew, CMTSafeHeap* pHeap, size_t nDataSize, unsigned int nFactoryFlags, size_t nAllocatedSize);
        void Delete();
        void PreloadToMemory(IMemoryBlock* pMemoryBlock = NULL);
        void UnloadFromMemory();
    private:
        // the constructor/destructor cannot be called at all - everything will go through the factory class
        Cache() { m_nCacheFactoryFlags = 0; m_nPakFileOffsetOnMedia = 0; }
        ~Cache(){}
    };

    TYPEDEF_AUTOPTR(Cache);

    typedef Cache_AutoPtr CachePtr;

    // initializes this object from the given Zip file: caches the central directory
    // returns 0 if successfully parsed, error code if an error has occured
    CachePtr NewCache(const char* szFilePath, CMTSafeHeap* pHeap, InitMethodEnum nInitMethod = ZD_INIT_FAST);
}

#endif // CRYINCLUDE_CRYSYSTEM_ZIPDIRCACHE_H
