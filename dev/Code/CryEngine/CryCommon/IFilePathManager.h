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
////////////////////////////////////////////////////////////////////////////
//
// This interface exists for game systems running inside the editor to
// convert a relative asset ID into a full source asset path. It's here
// for legacy support. Game component in general should not be dealing
// with full paths and that code should be eventually moved into tools
// where possible.
//
////////////////////////////////////////////////////////////////////////////

#ifndef CRYINCLUDE_CRYCOMMON_IFILEPATHMANAGER_H
#define CRYINCLUDE_CRYCOMMON_IFILEPATHMANAGER_H
#pragma once

// this interface depends on AZCore and is not available in MAX and MAYA and such

#if !defined(RESOURCE_COMPILER)

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>

class IFilePathManager
{
public:
    virtual ~IFilePathManager() = 0;
    virtual AZStd::string PrepareAssetIDForWriting(const char* name) const = 0;
};

AZ_FORCE_INLINE IFilePathManager::~IFilePathManager()
{
}

#else // empty implementation for tools like MAX/MAYA that need vs2010 support still

class IFilePathManager
{
};

#endif //RESOURCE_COMPILER

#endif // CRYINCLUDE_CRYCOMMON_IFILEPATHMANAGER_H