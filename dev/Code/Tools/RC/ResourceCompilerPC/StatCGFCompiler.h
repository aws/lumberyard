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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_STATCGFCOMPILER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_STATCGFCOMPILER_H
#pragma once


#include "Cry_Color.h"
#include "IConvertor.h"
#include "../../CryEngine/Cry3DEngine/MeshCompiler/MeshCompiler.h"

struct ConvertContext;
class CContentCGF;
class CChunkFile;
class CPhysicsInterface;
struct CMaterialCGF;

class CStatCGFCompiler
    : public IConvertor
    , public ICompiler
{
public:
    class Error
    {
    public:
        Error (int nCode);
        Error (const char* szFormat, ...);
        const char* c_str() const { return m_strReason.c_str(); }
    protected:
        string m_strReason;
    };

    CStatCGFCompiler();
    ~CStatCGFCompiler();

    // IConvertor methods.
    virtual ICompiler* CreateCompiler();
    virtual bool SupportsMultithreading() const;
    virtual const char* GetExt(int index) const
    {
        switch (index)
        {
        case 0:
            return "cga";
        case 1:
            return "cgf";
        case 2:
            return "i_cgf";
        default:
            return 0;
        }
    }

    // ICompiler methods.
    virtual void BeginProcessing(const IConfig* config) { }
    virtual void EndProcessing() { }
    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();

    // ICompiler + IConvertor methods.
    virtual void Release();

private:
    string GetOutputFileNameOnly() const;
    string GetOutputPath() const;
    void DeleteOldChunks(CContentCGF* pCGF, CChunkFile& chunkFile);
    bool IsLodFile(const string& filename) const;

private:
    CPhysicsInterface* m_pPhysicsInterface;
    ConvertContext m_CC;

    int m_refCount;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_STATCGFCOMPILER_H
