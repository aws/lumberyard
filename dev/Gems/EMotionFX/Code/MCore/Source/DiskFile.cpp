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

#include <stdio.h>
#include "DiskFile.h"
#include "UnicodeString.h"

#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(DiskFile_cpp)
#else
#define DISKFILE_CPP_TRAIT_USE_POSIX 1
#endif
namespace MCore
{
    // constructor
    DiskFile::DiskFile()
        : File()
        , mFile(nullptr)
    {
    }


    // destructor
    DiskFile::~DiskFile()
    {
        Close();
    }


    // return the unique type identification number
    uint32 DiskFile::GetType() const
    {
        return TYPE_ID;
    }


    // try to open the file
    bool DiskFile::Open(const char* fileName, EMode mode)
    {
        // if the file already is open, close it first
        if (mFile)
        {
            Close();
        }

        //String fileMode;
        char fileMode[4];
        uint32 numChars = 0;

        if (mode == READ)
        {
            fileMode[0] = 'r';
            numChars = 1;
        }                                                                                   // open for reading, file must exist
        if (mode == WRITE)
        {
            fileMode[0] = 'w';
            numChars = 1;
        }                                                                                   // open for writing, file will be overwritten if it already exists
        if (mode == READWRITE)
        {
            fileMode[0] = 'r';
            fileMode[1] = '+';
            numChars = 2;
        }                                                                                   // open for reading and writing, file must exist
        if (mode == READWRITECREATE)
        {
            fileMode[0] = 'w';
            fileMode[1] = '+';
            numChars = 2;
        }                                                                                   // open for reading and writing, file will be overwritten when already exists, or created when it doesn't
        if (mode == APPEND)
        {
            fileMode[0] = 'a';
            numChars = 1;
        }                                                                                   // open for writing at the end of the file, file will be created when it doesn't exist
        if (mode == READWRITEAPPEND)
        {
            fileMode[0] = 'a';
            fileMode[1] = '+';
            numChars = 2;
        }                                                                                   // open for reading and appending (writing), file will be created if it doesn't exist

        // construct the filemode string
        fileMode[numChars++] = 'b'; // open in binary mode
        fileMode[numChars++] = '\0';

        // set the file mode we used
        mFileMode = mode;

        // set the filename
        mFileName = fileName;

        // try to open the file
    #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
        errno_t err = fopen_s(&mFile, fileName, fileMode);
        if (err != 0)
        {
            return false;
        }
    #else
        mFile = fopen(fileName, fileMode);
    #endif

        // check on success
        return (mFile != nullptr);
    }


    // close the file
    void DiskFile::Close()
    {
        if (mFile)
        {
            fclose(mFile);
            mFile = nullptr;
        }
    }


    // flush the file
    void DiskFile::Flush()
    {
        MCORE_ASSERT(mFile);
        fflush(mFile);
    }


    bool DiskFile::GetIsOpen() const
    {
        return (mFile != nullptr);
    }



    // return true when we have reached the end of the file
    bool DiskFile::GetIsEOF() const
    {
        MCORE_ASSERT(mFile);
        return (feof(mFile) != 0);
    }


    // returns the next byte in the file
    uint8 DiskFile::GetNextByte()
    {
        MCORE_ASSERT(mFile);
        MCORE_ASSERT((mFileMode == READ) || (mFileMode == READWRITE) || (mFileMode == READWRITEAPPEND) || (mFileMode == APPEND) || (mFileMode == READWRITECREATE)); // make sure we opened the file in read mode
        return static_cast<uint8>(fgetc(mFile));
    }


    // returns the position (offset) in the file in bytes
    size_t DiskFile::GetPos() const
    {
        MCORE_ASSERT(mFile);

    #ifdef MCORE_PLATFORM_WINDOWS
        #ifdef MCORE_ARCHITECTURE_64BIT
        return _ftelli64(mFile);
        #else
        return ftell(mFile);
        #endif
    #elif defined(MCORE_PLATFORM_POSIX) && DISKFILE_CPP_TRAIT_USE_POSIX
        return ftello(mFile);
    #else
        return ftell(mFile);
    #endif
    }


    // write a given byte to the file
    bool DiskFile::WriteByte(uint8 value)
    {
        MCORE_ASSERT(mFile);
        MCORE_ASSERT((mFileMode == WRITE) || (mFileMode == READWRITE) || (mFileMode == READWRITEAPPEND) || (mFileMode == READWRITECREATE)); // make sure we opened the file in write mode

        if (fputc(value, mFile) == EOF)
        {
            return false;
        }

        return true;
    }


    // seek a given number of bytes ahead from it's current position
    bool DiskFile::Forward(size_t numBytes)
    {
        MCORE_ASSERT(mFile);

    #ifdef MCORE_PLATFORM_WINDOWS
        #ifdef MCORE_ARCHITECTURE_64BIT
        if (_fseeki64(mFile, numBytes, SEEK_CUR) != 0)
        {
            return false;
        }
        #else
        if (fseek(mFile, numBytes, SEEK_CUR) != 0)
        {
            return false;
        }
        #endif
    #elif defined(MCORE_PLATFORM_POSIX) && DISKFILE_CPP_TRAIT_USE_POSIX
        if (fseeko(mFile, numBytes, SEEK_CUR) != 0)
        {
            return false;
        }
    #else
        if (fseek(mFile, (long)numBytes, SEEK_CUR) != 0)
        {
            return false;
        }
    #endif

        return true;
    }


    // seek to an absolute position in the file (offset in bytes)
    bool DiskFile::Seek(size_t offset)
    {
        MCORE_ASSERT(mFile);

    #if defined(MCORE_PLATFORM_WINDOWS)
        #ifdef MCORE_ARCHITECTURE_64BIT
        if (_fseeki64(mFile, offset, SEEK_SET) != 0)
        {
            return false;
        }
        #else
        if (fseek(mFile, offset, SEEK_SET) != 0)
        {
            return false;
        }
        #endif
    #elif defined(MCORE_PLATFORM_POSIX) && DISKFILE_CPP_TRAIT_USE_POSIX
        if (fseeko(mFile, offset, SEEK_SET) != 0)
        {
            return false;
        }
    #else
        if (fseek(mFile, (long)offset, SEEK_SET) != 0)
        {
            return false;
        }
    #endif

        return true;
    }


    // write data to the file
    size_t DiskFile::Write(const void* data, size_t length)
    {
        MCORE_ASSERT(mFile);
        MCORE_ASSERT((mFileMode == WRITE) || (mFileMode == READWRITE) || (mFileMode == READWRITEAPPEND) || (mFileMode == READWRITECREATE)); // make sure we opened the file in write mode

        if (fwrite(data, length, 1, mFile) == 0)
        {
            return 0;
        }

        return length;
    }


    // read data from the file
    size_t DiskFile::Read(void* data, size_t length)
    {
        MCORE_ASSERT(mFile);
        MCORE_ASSERT((mFileMode == READ) || (mFileMode == READWRITE) || (mFileMode == READWRITEAPPEND) || (mFileMode == APPEND) || (mFileMode == READWRITECREATE)); // make sure we opened the file in read mode

        if (fread(data, length, 1, mFile) == 0)
        {
            return 0;
        }

        return length;
    }


    // returns the filesize in bytes
    size_t DiskFile::GetFileSize() const
    {
        MCORE_ASSERT(mFile);
        if (mFile == nullptr)
        {
            return 0;
        }

        // get the current file position
        size_t curPos = GetPos();

        // seek to the end of the file
    #if defined(MCORE_PLATFORM_WINDOWS)
        #ifdef MCORE_ARCHITECTURE_64BIT
        _fseeki64(mFile, 0, SEEK_END);
        #else
        fseek(mFile, 0, SEEK_END);
        #endif
    #elif defined(MCORE_PLATFORM_POSIX) && DISKFILE_CPP_TRAIT_USE_POSIX
        fseeko(mFile, 0, SEEK_END);
    #else
        fseek(mFile, 0, SEEK_END);
    #endif

        // get the position, whis is the size of the file
        size_t fileSize = GetPos();

        // seek back to the original position
    #if defined(MCORE_PLATFORM_WINDOWS)
        #ifdef MCORE_ARCHITECTURE_64BIT
        _fseeki64(mFile, curPos, SEEK_SET);
        #else
        fseek(mFile, (long)curPos, SEEK_SET);
        #endif
    #elif defined(MCORE_PLATFORM_POSIX) && DISKFILE_CPP_TRAIT_USE_POSIX
        fseeko(mFile, curPos, SEEK_SET);
    #else
        fseek(mFile, (long)curPos, SEEK_SET);
    #endif

        // return the size of the file
        return fileSize;
    }


    // get the file mode
    DiskFile::EMode DiskFile::GetFileMode() const
    {
        return mFileMode;
    }


    // get the file name
    String DiskFile::GetFileName() const
    {
        return mFileName;
    }
} // namespace MCore
