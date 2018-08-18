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

// Description : console variables list processor

#ifndef CRYINCLUDE_CRYACTION_NETWORK_CVARLISTPROCESSOR_H
#define CRYINCLUDE_CRYACTION_NETWORK_CVARLISTPROCESSOR_H
#pragma once

struct ICVar;

struct ICVarListProcessorCallback
{
    virtual ~ICVarListProcessorCallback(){}
    virtual void OnCVar(ICVar*) = 0;
};

class CCVarListProcessor
{
public:
    CCVarListProcessor(const char* path);
    ~CCVarListProcessor();

    void Process(ICVarListProcessorCallback* cb);
private:
    string m_fileName;
};

#endif // CRYINCLUDE_CRYACTION_NETWORK_CVARLISTPROCESSOR_H
