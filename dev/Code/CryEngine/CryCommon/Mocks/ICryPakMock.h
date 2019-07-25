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

#ifndef CRYINCLUDE_CRYCOMMON_ICRYPAKMOCK_H
#define CRYINCLUDE_CRYCOMMON_ICRYPAKMOCK_H
#pragma once

#include <ICryPak.h>
#include <AzTest/AzTest.h>
#include <AzCore/IO/FileIO.h>

struct CryPakMock
    : public ICryPak
{
    MOCK_CONST_METHOD0(GetDirectoryDelimiter, const char*());
    MOCK_METHOD5(AdjustFileNameImpl, const char*(const char* src, char* dst, size_t dstSize, unsigned nFlags, bool skipMods));
    MOCK_METHOD1(Init, bool(const char* szBasePath));
    MOCK_METHOD0(Release, void());
    MOCK_CONST_METHOD1(IsInstalledToHDD, bool(const char* acFilePath));
    MOCK_METHOD4(OpenPack, bool(const char* pName, unsigned nFlags, IMemoryBlock * pData, CryFixedStringT<ICryPak::g_nMaxPath>* pFullPath));
    MOCK_METHOD5(OpenPack, bool(const char* pBindingRoot, const char* pName, unsigned nFlags, IMemoryBlock * pData, CryFixedStringT<ICryPak::g_nMaxPath>* pFullPath));
    MOCK_METHOD2(ClosePack, bool(const char* pName, unsigned nFlags));
    MOCK_METHOD3(OpenPacks, bool(const char* pWildcard, unsigned nFlags, std::vector< CryFixedStringT<ICryPak::g_nMaxPath> >* pFullPaths));
    MOCK_METHOD4(OpenPacks, bool(const char* pBindingRoot, const char* pWildcard, unsigned nFlags, std::vector< CryFixedStringT<ICryPak::g_nMaxPath> >* pFullPaths));
    MOCK_METHOD2(ClosePacks, bool(const char* pWildcard, unsigned nFlags));
    MOCK_METHOD1(FindPacks, bool(const char* pWildcardIn));
    MOCK_METHOD3(SetPacksAccessible, bool(bool bAccessible, const char* pWildcard, unsigned nFlags));
    MOCK_METHOD3(SetPackAccessible, bool(bool bAccessible, const char* pName, unsigned nFlags));
    MOCK_METHOD1(SetPacksAccessibleForLevel, void(const char* sLevelName));
    MOCK_METHOD3(LoadPakToMemory, bool(const char* pName, EInMemoryPakLocation eLoadToMemory, IMemoryBlock * pMemoryBlock));
    MOCK_METHOD2(LoadPaksToMemory, void(int nMaxPakSize, bool bLoadToMemory));
    MOCK_METHOD1(AddMod, void(const char* szMod));
    MOCK_METHOD1(RemoveMod, void(const char* szMod));
    MOCK_METHOD1(GetMod, const char*(int index));
    MOCK_METHOD1(ParseAliases, void(const char* szCommandLine));
    MOCK_METHOD3(SetAlias, void(const char* szName, const char* szAlias, bool bAdd));
    MOCK_METHOD2(GetAlias, const char*(const char* szName, bool bReturnSame));
    MOCK_METHOD0(Lock, void());
    MOCK_METHOD0(Unlock, void());
    MOCK_METHOD1(LockReadIO, void(bool bValue));
    MOCK_METHOD1(SetGameFolder, void(const char* szFolder));
    MOCK_CONST_METHOD0(GetGameFolder, const char*());
    MOCK_METHOD1(SetLocalizationFolder, void(const char* sLocalizationFolder));
    MOCK_CONST_METHOD0(GetLocalizationFolder, const char*());
    MOCK_CONST_METHOD0(GetLocalizationRoot, const char*());
    MOCK_METHOD3(GetCachedPakCDROffsetSize, void(const char* szName, uint32 & offset, uint32 & size));
    MOCK_METHOD0(GetPakInfo, ICryPak::PakInfo * ());
    MOCK_METHOD1(FreePakInfo, void(PakInfo*));
    MOCK_METHOD3(FOpen, AZ::IO::HandleType(const char* pName, const char* mode, unsigned nFlags));
    MOCK_METHOD4(FOpen, AZ::IO::HandleType(const char* pName, const char* mode, char* szFileGamePath, int nLen));
    MOCK_METHOD2(FOpenRaw, AZ::IO::HandleType(const char* pName, const char* mode));
    MOCK_METHOD4(FReadRaw, size_t(void* data, size_t length, size_t elems, AZ::IO::HandleType handle));
    MOCK_METHOD3(FReadRawAll, size_t(void* data, size_t nFileSize, AZ::IO::HandleType handle));
    MOCK_METHOD2(FGetCachedFileData, void*(AZ::IO::HandleType handle, size_t & nFileSize));
    MOCK_METHOD4(FWrite, size_t(const void* data, size_t length, size_t elems, AZ::IO::HandleType handle));
    MOCK_METHOD3(FGets, char*(char*, int, AZ::IO::HandleType));
    MOCK_METHOD1(Getc, int(AZ::IO::HandleType));
    MOCK_METHOD1(FGetSize, size_t(AZ::IO::HandleType f));
    MOCK_METHOD2(FGetSize, size_t(const char* pName, bool bAllowUseFileSystem));
    MOCK_METHOD2(Ungetc, int(int c, AZ::IO::HandleType));
    MOCK_METHOD1(IsInPak, bool(AZ::IO::HandleType handle));
    MOCK_METHOD1(RemoveFile, bool(const char* pName));
    MOCK_METHOD1(RemoveDir, bool(const char* pName));
    MOCK_METHOD1(IsAbsPath, bool(const char* pPath));
    MOCK_METHOD3(CopyFileOnDisk, bool(const char* source, const char* dest, bool bFailIfExist));
    MOCK_METHOD3(FSeek, size_t(AZ::IO::HandleType handle, long seek, int mode));
    MOCK_METHOD1(FTell, long(AZ::IO::HandleType handle));
    MOCK_METHOD1(FClose, int(AZ::IO::HandleType handle));
    MOCK_METHOD1(FEof, int(AZ::IO::HandleType handle));
    MOCK_METHOD1(FError, int(AZ::IO::HandleType handle));
    MOCK_METHOD0(FGetErrno, int());
    MOCK_METHOD1(FFlush, int(AZ::IO::HandleType handle));
    MOCK_METHOD1(PoolMalloc, void*(size_t size));
    MOCK_METHOD1(PoolFree, void(void* p));
    MOCK_METHOD3(PoolAllocMemoryBlock, IMemoryBlock * (size_t nSize, const char* sUsage, size_t nAlign));
    MOCK_METHOD4(FindFirst, intptr_t(const char* pDir, _finddata_t * fd, unsigned int nFlags, bool bAllOwUseFileSystem));
    MOCK_METHOD2(FindNext, int(intptr_t handle, _finddata_t * fd));
    MOCK_METHOD1(FindClose, int(intptr_t handle));
    MOCK_METHOD1(GetModificationTime, ICryPak::FileTime(AZ::IO::HandleType f));
    MOCK_METHOD2(IsFileExist, bool(const char* sFilename, EFileSearchLocation));
    MOCK_METHOD1(IsFolder, bool(const char* sPath));
    MOCK_METHOD1(GetFileSizeOnDisk, ICryPak::SignedFileSize(const char* filename));
    MOCK_METHOD1(IsFileCompressed, bool(const char* filename));
    MOCK_METHOD2(MakeDir, bool(const char* szPath, bool bGamePathMapping));
    MOCK_METHOD4(OpenArchive, ICryArchive * (const char* szPath, const char* bindRoot, unsigned int nFlags, IMemoryBlock * pData));
    MOCK_METHOD1(GetFileArchivePath, const char* (AZ::IO::HandleType f));
    MOCK_METHOD5(RawCompress, int(const void* pUncompressed, unsigned long* pDestSize, void* pCompressed, unsigned long nSrcSize, int nLevel));
    MOCK_METHOD4(RawUncompress, int(void* pUncompressed, unsigned long* pDestSize, const void* pCompressed, unsigned long nSrcSize));
    MOCK_METHOD1(RecordFileOpen, void(ERecordFileOpenList eList));
    MOCK_METHOD2(RecordFile, void(AZ::IO::HandleType in, const char* szFilename));
    MOCK_METHOD1(GetResourceList, IResourceList * (ERecordFileOpenList eList));
    MOCK_METHOD2(SetResourceList, void(ERecordFileOpenList eList, IResourceList * pResourceList));
    MOCK_METHOD0(GetRecordFileOpenList, ICryPak::ERecordFileOpenList());
    MOCK_METHOD2(ComputeCRC, uint32(const char* szPath, uint32 nFileOpenFlags));
    MOCK_METHOD4(ComputeMD5, bool(const char* szPath, unsigned char* md5, uint32 nFileOpenFlags, bool useDirectAccess));
    MOCK_METHOD2(GenerateMD5FromName, bool(const char* szName, unsigned char* md5));
    MOCK_METHOD3(ComputeCachedPakCDR_CRC, int(const char* filename, bool useCryFile, IMemoryBlock * pData));
    MOCK_METHOD1(RegisterFileAccessSink, void(ICryPakFileAcesssSink * pSink));
    MOCK_METHOD1(UnregisterFileAccessSink, void(ICryPakFileAcesssSink * pSink));
    MOCK_CONST_METHOD0(GetLvlResStatus, bool());
    MOCK_METHOD1(DisableRuntimeFileAccess, void(bool status));
    MOCK_METHOD2(DisableRuntimeFileAccess, bool(bool status, threadID threadId));
    MOCK_METHOD2(CheckFileAccessDisabled, bool(const char* name, const char* mode));
    MOCK_METHOD1(SetRenderThreadId, void(threadID renderThreadId));
    MOCK_METHOD0(GetPakPriority, int());
    MOCK_METHOD1(GetFileOffsetOnMedia, uint64(const char* szName));
    MOCK_METHOD1(GetFileMediaType, EStreamSourceMediaType(const char* szName));
    MOCK_METHOD0(CreatePerfHUDWidget, void());

    // Implementations required for variadic functions
    virtual int FPrintf(AZ::IO::HandleType handle, const char* format, ...) PRINTF_PARAMS(3, 4)
    {
        return 0;
    }
};

#endif // CRYINCLUDE_CRYCOMMON_ICRYPAK_H
