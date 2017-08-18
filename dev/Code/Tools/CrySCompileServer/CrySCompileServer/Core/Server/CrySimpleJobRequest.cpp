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

#include "CrySimpleJobRequest.hpp"
#include "ShaderList.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <Core/Common.h>
#include <Core/tinyxml/tinyxml.h>

#include <AzCore/Debug/Trace.h>

CCrySimpleJobRequest::CCrySimpleJobRequest(uint32_t requestIP)
    : CCrySimpleJob(requestIP)
{
}

bool CCrySimpleJobRequest::Execute(const TiXmlElement* pElement)
{
    const char* pPlatform                       = pElement->Attribute("Platform");
    const char* pShaderRequestLine  =   pElement->Attribute("ShaderRequest");
    if (!pPlatform)
    {
        State(ECSJS_ERROR_INVALID_PLATFORM);
        CrySimple_ERROR("Missing Platform for shader request");
        return false;
    }
    if (!pShaderRequestLine)
    {
        State(ECSJS_ERROR_INVALID_SHADERREQUESTLINE);
        CrySimple_ERROR("Missing shader request line");
        return false;
    }

    std::string ShaderRequestLine(pShaderRequestLine);
    tdEntryVec Toks;
    CSTLHelper::Tokenize(Toks, ShaderRequestLine, ";");
    for (size_t a = 0, S = Toks.size(); a < S; a++)
    {
        CShaderList::Instance().Add(pPlatform, Toks[a].c_str());
    }

    //  CShaderList::Instance().Add(pPlatform,pShaderRequestLine );
    State(ECSJS_DONE);

    return true;
}
