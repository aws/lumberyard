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

#include "stdafx.h"
#include "IResCompiler.h"
#include "XMLConverter.h"
#include "IRCLog.h"
#include "XML/xml.h"
#include <CryLibrary.h>

// Must be included only once in DLL module.
#include <platform_implRC.h>

XmlStrCmpFunc g_pXmlStrCmp = &azstricmp;

ICryXML* LoadICryXML()
{
    HMODULE hXMLLibrary = CryLoadLibraryDefName("CryXML");
    if (NULL == hXMLLibrary)
    {
        RCLogError("Unable to load xml library (CryXML.dll).");
        return 0;
    }

    FnGetICryXML pfnGetICryXML = (FnGetICryXML)CryGetProcAddress(hXMLLibrary, "GetICryXML");
    if (pfnGetICryXML == 0)
    {
        RCLogError("Unable to load xml library (CryXML.dll) - cannot find exported function GetICryXML().");
        return 0;
    }

    return pfnGetICryXML();
}


extern "C" DLL_EXPORT void __stdcall RegisterConvertors(IResourceCompiler* pRC)
{
    SetRCLog(pRC->GetIRCLog());

    ICryXML* pCryXML = LoadICryXML();
    if (pCryXML == 0)
    {
        RCLogError("Loading xml library failed - not registering xml converter.");
        return;
    }

    pCryXML->AddRef();

    pRC->RegisterConvertor("XMLConverter", new XMLConverter(pCryXML));

    pRC->RegisterKey("xmlFilterFile", "specify file with special commands to filter out unneeded XML elements and attributes");

    pCryXML->Release();
}

