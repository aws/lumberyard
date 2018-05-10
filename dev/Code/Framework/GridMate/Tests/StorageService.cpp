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
#include "Tests.h"

#include <GridMate/Storage/GridStorageService.h>

#include <AzCore/std/parallel/thread.h>


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define STORAGESERVICE_CPP_SECTION_1 1
#define STORAGESERVICE_CPP_SECTION_2 2
#define STORAGESERVICE_CPP_SECTION_3 3
#define STORAGESERVICE_CPP_SECTION_4 4
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STORAGESERVICE_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(StorageService_cpp, AZ_RESTRICTED_PLATFORM)
#endif

using namespace GridMate;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STORAGESERVICE_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(StorageService_cpp, AZ_RESTRICTED_PLATFORM)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STORAGESERVICE_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(StorageService_cpp, AZ_RESTRICTED_PLATFORM)
#endif

AZ_TEST_SUITE(StorageSuite)

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STORAGESERVICE_CPP_SECTION_4
#include AZ_RESTRICTED_FILE(StorageService_cpp, AZ_RESTRICTED_PLATFORM)
#endif
AZ_TEST_SUITE_END
