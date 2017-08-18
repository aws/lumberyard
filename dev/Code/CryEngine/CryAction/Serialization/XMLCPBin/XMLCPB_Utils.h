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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_XMLCPB_UTILS_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_XMLCPB_UTILS_H
#pragma once

#include "Reader/XMLCPB_NodeLiveReader.h"
#include <IPlatformOS.h>

namespace XMLCPB {
#ifdef XMLCPB_DEBUGUTILS

    class CDebugUtils
        : public IPlatformOS::IPlatformListener
    {
    public:

        static void Create()
        {
            if (!s_pThis)
            {
                ScopedSwitchToGlobalHeap useGlobalHeap;
                s_pThis = new CDebugUtils();
            }
        }

        static void Destroy()
        {
            SAFE_DELETE(s_pThis);
        }

        static XmlNodeRef BinaryFileToXml(const char* pBinaryFileName);
        static void DumpToXmlFile(CNodeLiveReaderRef BRoot, const char* pXmlFileName);
        static void DumpToLog(CNodeLiveReaderRef BRoot);
        static void SetLastFileNameSaved(const char* pFileName);

        // IPlatformOS::IPlatformListener
        virtual void OnPlatformEvent(const IPlatformOS::SPlatformEvent& event);
        // ~IPlatformOS::IPlatformListener

    private:

        CDebugUtils();
        virtual ~CDebugUtils();
        static void RecursiveCopyAttrAndChildsIntoXmlNode(XmlNodeRef xmlNode, const CNodeLiveReaderRef& BNode);
        static void GenerateXMLFromLastSaveCmd(IConsoleCmdArgs* args);
        static void GenerateXmlFileWithSizeInformation(const char* pBinaryFileName, const char* pXmlFileName);

    private:
        string m_lastFileNameSaved;
        static CDebugUtils* s_pThis;
    };

#else   //XMLCPB_DEBUGUTILS
    class CDebugUtils
    {
    public:
        static void Create() {}
        static void Destroy() {};
    };
#endif
} // end namespace

#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_XMLCPB_UTILS_H
