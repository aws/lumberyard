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
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include "AzCore/Slice/SliceAsset.h"

namespace UnitTest
{
    //! Returns a test folder path 
    AZStd::string GetTestFolderPath();

    void MakePathFromTestFolder(char* buffer, int bufferLen, const char* fileName);
}

namespace AZ
{
    class Component;

    namespace Test
    {
        //! Creates an entity, assigns the given component to it and then creates a slice asset containing the entity
        AZ::Data::Asset<AZ::SliceAsset> CreateSliceFromComponent(AZ::Component* assetComponent, AZ::Data::AssetId sliceAssetId);
 
        // the Assert Absorber can be used to absorb asserts, errors and warnings during unit tests.
        class AssertAbsorber
            : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            AssertAbsorber();
            ~AssertAbsorber();

            bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override;
            bool OnAssert(const char* message) override;
            bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
            bool OnError(const char* window, const char* message) override;
            bool OnWarning(const char* window, const char* message) override;
            bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        };

    } //namespace Test

}// namespace AZ
