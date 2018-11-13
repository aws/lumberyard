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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_COLLADACOMPILER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_COLLADACOMPILER_H
#pragma once


#include "IConvertor.h"
#include "ColladaLoader.h"

class ICryXML;
class CContentCGF;
class CPhysicsInterface;
struct sMaterialLibrary;
struct IPakSystem;
struct ExportFile;

class ColladaCompiler
    : public IConvertor
    , public ICompiler
{
public:
    ColladaCompiler(ICryXML* pCryXML, IPakSystem* pPakSystem);
    virtual ~ColladaCompiler();

    // IConvertor methods.
    virtual ICompiler* CreateCompiler();
    virtual const char* GetExt(int index) const;

    // ICompiler methods.
    virtual void BeginProcessing(const IConfig* config) { }
    virtual void EndProcessing() { }
    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();
    string GetOutputFileNameOnly() const;

    // ICompiler + IConvertor methods.
    virtual void Release();

private:
    void RecordMetrics(const std::vector<ExportFile>& exportFileList) const;
    void WriteMaterialLibrary(sMaterialLibrary& materialLibrary, const string& matFileName);

    void SetExportInfo(CContentCGF* const pCompiledCGF, ExportFile& exportFile);

    bool CompileToCGF(ExportFile& exportFile, const string& exportFileName);
    bool CompileToCHR(ExportFile& exportFile, const string& exportFileName);
    bool CompileToANM(ExportFile& exportFile, const string& exportFileName);
    bool CompileToCAF(ExportFile& exportFile, const string& exportFileName);

    void PrepareCGF(CContentCGF* pCGF);
    void FindUsedMaterialIDs(std::vector<int>& usedMaterialIDs, CContentCGF* pCGF);

private:
    ConvertContext m_CC;
    ICryXML* pCryXML;
    IPakSystem* pPakSystem;
    CPhysicsInterface* pPhysicsInterface;
    int m_refCount;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_COLLADACOMPILER_H
