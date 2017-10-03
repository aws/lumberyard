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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CONVERTCONTEXT_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CONVERTCONTEXT_H
#pragma once

#include "PathHelpers.h"
#include "string.h"
#include "IMultiplatformConfig.h"
#include "IResCompiler.h"

class IConfig;

// IConvertContext is a description of what and how should be processed by compiler
struct IConvertContext
{
    virtual void SetConvertorExtension(const char* convertorExtension) = 0;

    virtual void SetSourceFolder(const char* sourceFolder) = 0;
    virtual void SetSourceFileNameOnly(const char* sourceFileNameOnly) = 0;
    virtual void SetOutputFolder(const char* sOutputFolder) = 0;

    virtual void SetRC(IResourceCompiler* pRC) = 0;
    virtual void SetMultiplatformConfig(IMultiplatformConfig* pMultiConfig) = 0;
    virtual void SetPlatformIndex(int platformIndex) = 0;
    virtual void SetThreads(int threads) = 0;
    virtual void SetForceRecompiling(bool bForceRecompiling) = 0;

    virtual void CopyTo(IConvertContext* context) = 0;

};

struct ConvertContext
    : public IConvertContext
{
    //////////////////////////////////////////////////////////////////////////
    // Interface IConvertContext

    virtual void SetConvertorExtension(const char* convertorExtension)
    {
        this->convertorExtension = convertorExtension;
    }

    virtual void SetSourceFolder(const char* sourceFolder)
    {
        this->sourceFolder = sourceFolder;
    }
    virtual void SetSourceFileNameOnly(const char* sourceFileNameOnly)
    {
        this->sourceFileNameOnly = sourceFileNameOnly;
    }
    virtual void SetOutputFolder(const char* sOutputFolder)
    {
        this->outputFolder = sOutputFolder;
    }

    virtual void SetRC(IResourceCompiler* pRC)
    {
        this->pRC = pRC;
    }
    virtual void SetMultiplatformConfig(IMultiplatformConfig* pMultiConfig)
    {
        this->multiConfig = pMultiConfig;
        this->config = &pMultiConfig->getConfig();
        this->platform = pMultiConfig->getActivePlatform();
    }
    virtual void SetPlatformIndex(int platformIndex)
    {        
        this->platform = platformIndex;
        multiConfig->setActivePlatform(platformIndex);
    }
    virtual void SetThreads(int threads)
    {
        this->threads = threads;
    }
    virtual void SetForceRecompiling(bool bForceRecompiling)
    {
        this->bForceRecompiling = bForceRecompiling;
    }

    virtual void CopyTo(IConvertContext* context)
    {
        context->SetConvertorExtension(convertorExtension);
        context->SetSourceFolder(sourceFolder);
        context->SetSourceFileNameOnly(sourceFileNameOnly);
        context->SetOutputFolder(outputFolder);
        context->SetRC(pRC);
        context->SetMultiplatformConfig(multiConfig);
        context->SetThreads(threads);
        context->SetForceRecompiling(bForceRecompiling);
    }
    //////////////////////////////////////////////////////////////////////////

    const string GetSourcePath() const
    {
        return PathHelpers::Join(sourceFolder, sourceFileNameOnly).c_str();
    }

    const string& GetOutputFolder() const
    {
        return outputFolder;
    }

public:
    // Convertor will assume that the source file has content matching this extension
    // (the sourceFileNameOnly can have a different extension, say 'tmp').
    string convertorExtension;

    // Source file's folder.
    string sourceFolder;
    // Source file that needs to be converted, for example "test.tif".
    // Contains filename only, the folder is stored in sourceFolder.
    string sourceFileNameOnly;

    // Pointer to resource compiler interface.
    IResourceCompiler* pRC;

    // Configuration settings.
    IMultiplatformConfig* multiConfig;
    // Platform to which file must be processed.
    int platform;
    // Platform's config.
    const IConfig* config;

    // Number of threads.
    int threads;

    // true if compiler is requested to skip up-to-date checks
    bool bForceRecompiling;

private:
    // Output folder.
    string outputFolder;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CONVERTCONTEXT_H
