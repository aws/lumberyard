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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_WRITER_XMLCPB_ZLIBCOMPRESSOR_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_WRITER_XMLCPB_ZLIBCOMPRESSOR_H
#pragma once

#include <IPlatformOS.h>
#include "../XMLCPB_Common.h"


namespace XMLCPB {
    bool InitializeCompressorThread();
    void ShutdownCompressorThread();


    //////////////////////////////////////////////////////////////////////////


    struct SZLibBlock
    {
        SZLibBlock(class CZLibCompressor* pCompressor);
        ~SZLibBlock();

        CZLibCompressor*                        m_pCompressor;
        uint8*                                          m_pZLibBuffer;                      // data that is going to be compressed is stored here
        uint32                                          m_ZLibBufferSizeUsed;           // how much of m_pZLibBuffer is currently used
    };


    class CZLibCompressor
    {
    public:
        CZLibCompressor(const char* pFileName);
        ~CZLibCompressor();

        void                                            WriteDataIntoFile(void* pSrc, uint32 numBytes);
        void                                            AddDataToZLibBuffer(uint8*& pSrc, uint32& numBytes);
        void                                                CompressAndWriteZLibBufferIntoFile(bool bFlush);
        void                                                FlushZLibBuffer();
        SFileHeader&                                GetFileHeader() { return m_fileHeader; }

    public:

        SFileHeader                                 m_fileHeader;                           // actually a footer
        class CFile*                                m_pFile;
        SZLibBlock*                                 m_currentZlibBlock;
        bool                                                m_bUseZLibCompression;
        bool                                                m_errorWritingIntoFile;
    };
}  // end namespace

#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_WRITER_XMLCPB_ZLIBCOMPRESSOR_H
