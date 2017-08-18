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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_WRITER_XMLCPB_WRITERINTERFACE_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_WRITER_XMLCPB_WRITERINTERFACE_H
#pragma once

#include "XMLCPB_Writer.h"

namespace XMLCPB {
    // this class is just to give a clear access to the intended interface functions and only them, without using a bunch of friend declarations
    // CWriter should not be used directly from outside
    class CWriterInterface
    {
    public:

        void                                    Init(const char* pRootName, const char* pFileName) { m_writer.Init(pRootName, pFileName); }
        CNodeLiveWriterRef      GetRoot() { return m_writer.GetRoot(); }
        void                                    Done() { m_writer.Done(); }     // should be called when everything is added and finished.
        bool                                    FinishWritingFile() { return m_writer.FinishWritingFile(); }
        bool                                    WriteAllIntoMemory(uint8*& rpData, uint32& outSize) { return m_writer.WriteAllIntoMemory(rpData, outSize); }

    private:

        CWriter                             m_writer;
    };
}  // end namespace


#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_WRITER_XMLCPB_WRITERINTERFACE_H
