/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <Source/CopyDependencyBuilderWorker.h>

namespace CopyDependencyBuilder
{
    //! The copy dependency builder is copy job that examines asset files for asset references,
    //! to output product dependencies.
    class ParticlePreloadLibsBuilderWorker
        : public CopyDependencyBuilderWorker
    {
    public:
        AZ_RTTI(ParticlePreloadLibsBuilderWorker, "{ACFC4966-6F96-4392-8918-25EA35225D38}", CopyDependencyBuilderWorker);

        ParticlePreloadLibsBuilderWorker();

        void RegisterBuilderWorker() override;
        void UnregisterBuilderWorker() override;
        bool ParseProductDependencies(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies) override;

    private:
        void SetExtension(AZStd::string& file, const char* newExtension) const;
        void SetToGlobalPath(AZStd::string& file) const;
    };
}