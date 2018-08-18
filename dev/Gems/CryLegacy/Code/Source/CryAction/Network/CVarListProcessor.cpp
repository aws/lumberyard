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

#include "CryLegacy_precompiled.h"
#include "CVarListProcessor.h"

CCVarListProcessor::CCVarListProcessor(const char* path)
    : m_fileName(path)
{
}

CCVarListProcessor::~CCVarListProcessor()
{
}

void CCVarListProcessor::Process(ICVarListProcessorCallback* cb)
{
    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(m_fileName, "rt");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return;
    }

    static const int BUFSZ = 4096;
    char buf[BUFSZ];

    size_t nRead;
    string cvar;
    bool comment = false;
    do
    {
        cvar.resize(0);
        buf[0] = '\0';
        nRead = gEnv->pCryPak->FRead(buf, BUFSZ, fileHandle);

        for (size_t i = 0; i < nRead; i++)
        {
            char c = buf[i];
            if (comment)
            {
                if (c == '\r' || c == '\n')
                {
                    comment = false;
                }
            }
            else
            {
                if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                {
                    cvar += c;
                }
                else if (c == '\t' || c == '\r' || c == '\n' || c == ' ')
                {
                    if (ICVar* pV = gEnv->pConsole->GetCVar(cvar.c_str()))
                    {
                        cb->OnCVar(pV);
                        //CryLog( "Unprotecting '%s'",cvar.c_str());
                    }
                    cvar.resize(0);
                }
                else if (c == '#')
                {
                    comment = true;
                }
            }
        }
    } while (nRead != 0);

    if (!cvar.empty())
    {
        if (ICVar* pV = gEnv->pConsole->GetCVar(cvar.c_str()))
        {
            cb->OnCVar(pV);
            //CryLog( "Unprotecting '%s'",cvar.c_str());
        }
    }

    gEnv->pCryPak->FClose(fileHandle);
}