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

#include <Configuration/MaglevConfig.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <ICryPak.h>
#include <mutex>
#include <atomic>

namespace Aws
{
    namespace Utils
    {
        namespace Json
        {
            class JsonValue;
        }
    }
}

namespace LmbrAWS
{

    class MaglevConfigPersistenceLayer
    {
    public:
        virtual ~MaglevConfigPersistenceLayer() = default;
        virtual Aws::Utils::Json::JsonValue LoadJsonDoc() const = 0;
        virtual void PersistJsonDoc(const Aws::Utils::Json::JsonValue& value) = 0;
        virtual void Purge() = 0;
    };

    class MaglevConfigPersistenceLayerCryPakImpl
        : public MaglevConfigPersistenceLayer
    {
    public:
        virtual ~MaglevConfigPersistenceLayerCryPakImpl() = default;
        Aws::Utils::Json::JsonValue LoadJsonDoc() const override;
        void PersistJsonDoc(const Aws::Utils::Json::JsonValue& value) override;
        void Purge() override;

    protected:
        //a little extra help for testing.
        virtual ICryPak* FindCryPak() const;
        virtual string GetConfigFilePath() const;
    };

    class MaglevConfigImpl
        : public MaglevConfig
    {
    public:
        MaglevConfigImpl(const AZStd::shared_ptr<MaglevConfigPersistenceLayer>& persistenceLayer);

        void SaveConfig() override;
        void LoadConfig() override;

    private:
        AZStd::shared_ptr<MaglevConfigPersistenceLayer> m_persistenceLayer;
        std::mutex m_persistenceMutex;
    };
}