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


#include <QFileInfo>

#include <AzCore/Component/Entity.h>

#include "native/assetprocessor.h"
#include "native/utilities/assetUtils.h"
#include "AssetBuilderInfo.h"

namespace AssetProcessor
{
    ExternalModuleAssetBuilderInfo::ExternalModuleAssetBuilderInfo(const QString& modulePath)
        : m_builderName(modulePath)
        , m_entity(nullptr)
        , m_componentDescriptorList()
        , m_initializeModuleFunction(nullptr)
        , m_moduleRegisterDescriptorsFunction(nullptr)
        , m_moduleAddComponentsFunction(nullptr)
        , m_uninitializeModuleFunction(nullptr)
        , m_modulePath(modulePath)
        , m_library(modulePath)
    {
    }

    ExternalModuleAssetBuilderInfo::~ExternalModuleAssetBuilderInfo()
    {
    }

    const QString& ExternalModuleAssetBuilderInfo::GetName() const
    {
        return this->m_builderName;
    }

    //! Sanity check for the module's status
    bool ExternalModuleAssetBuilderInfo::IsLoaded() const
    {
        return this->m_library.isLoaded();
    }

    void ExternalModuleAssetBuilderInfo::Initialize()
    {
        AZ_Error(AssetProcessor::ConsoleChannel, this->IsLoaded(), "External module %s not loaded.", m_builderName.toUtf8().data());

        this->m_initializeModuleFunction(AZ::Environment::GetInstance());

        this->m_moduleRegisterDescriptorsFunction();

        QFileInfo moduleFileName(this->m_modulePath);

        AZStd::string entityName = AZStd::string::format("%s Entity", GetName().toUtf8().data());
        this->m_entity = aznew AZ::Entity(entityName.c_str());

        this->m_moduleAddComponentsFunction(this->m_entity);

        AZ_TracePrintf(AssetProcessor::DebugChannel, "Init Entity %s", GetName().toUtf8().data());
        this->m_entity->Init();

        //Activate all the components
        this->m_entity->Activate();
    }


    void ExternalModuleAssetBuilderInfo::UnInitialize()
    {
        AZ_Error(AssetProcessor::ConsoleChannel, this->IsLoaded(), "External module %s not loaded.", m_builderName.toUtf8().data());

        AZ_TracePrintf(AssetProcessor::DebugChannel, "Uninitializing builder: %s\n", this->m_modulePath.toUtf8().data());

        if (this->m_entity)
        {
            this->m_entity->Deactivate();
            delete this->m_entity;
            this->m_entity = nullptr;
        }

        for (AZ::ComponentDescriptor* componentDesc : this->m_componentDescriptorList)
        {
            EBUS_EVENT(AssetBuilderRegistrationBus, UnRegisterComponentDescriptor, componentDesc);
            delete componentDesc;
        }
        this->m_componentDescriptorList.clear();

        for (const AZ::Uuid& builderDescID : m_registeredBuilderDescriptorIDs)
        {
            EBUS_EVENT(AssetBuilderRegistrationBus, UnRegisterBuilderDescriptor, builderDescID);
        }
        this->m_registeredBuilderDescriptorIDs.clear();

        this->m_uninitializeModuleFunction();

        if (this->m_library.isLoaded())
        {
            this->m_library.unload();
        }
    }

    bool ExternalModuleAssetBuilderInfo::Load()
    {
        if (this->IsLoaded())
        {
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "External module %s already.", m_builderName.toUtf8().data());
            return true;
        }

        this->m_library.setFileName(this->m_modulePath);
        if (!this->m_library.load())
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to load builder : %s\n", GetName().toUtf8().data());
            return false;
        }

        QStringList missingFunctionsList;
        QFunctionPointer isAssetBuilderAddress = ResolveModuleFunction<QFunctionPointer>("IsAssetBuilder", missingFunctionsList);
        InitializeModuleFunction initializeModuleAddress = ResolveModuleFunction<InitializeModuleFunction>("InitializeModule", missingFunctionsList);
        ModuleRegisterDescriptorsFunction moduleRegisterDescriptorsAddress = ResolveModuleFunction<ModuleRegisterDescriptorsFunction>("ModuleRegisterDescriptors", missingFunctionsList);
        ModuleAddComponentsFunction moduleAddComponentsAddress = ResolveModuleFunction<ModuleAddComponentsFunction>("ModuleAddComponents", missingFunctionsList);
        UninitializeModuleFunction uninitializeModuleAddress = ResolveModuleFunction<UninitializeModuleFunction>("UninitializeModule", missingFunctionsList);

        if (missingFunctionsList.size() == 0)
        {
            //if we are here than it is a builder
            this->m_initializeModuleFunction = initializeModuleAddress;
            this->m_moduleRegisterDescriptorsFunction = moduleRegisterDescriptorsAddress;
            this->m_moduleAddComponentsFunction = moduleAddComponentsAddress;
            this->m_uninitializeModuleFunction = uninitializeModuleAddress;
            return true;
        }
        else
        {
            // Not a valid builder module
            QString errorMessage = QString("Builder library %1 is missing one or more exported functions: %2").arg(QString(this->GetName())).arg(missingFunctionsList.join(','));
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "One or more builder functions is missing in the library: %s\n", errorMessage.toUtf8().data());
            this->m_library.unload();
            return false;
        }
    }

    void ExternalModuleAssetBuilderInfo::RegisterBuilderDesc(const AZ::Uuid& builderDescID)
    {
        if (m_registeredBuilderDescriptorIDs.find(builderDescID) != m_registeredBuilderDescriptorIDs.end())
        {
            AZ_Warning(AssetBuilderSDK::InfoWindow,
                false,
                "Builder description id '%s' already registered to external builder module %s",
                builderDescID.ToString<AZStd::string>().c_str(),
                this->m_builderName.toUtf8().data());
            return;
        }
        m_registeredBuilderDescriptorIDs.insert(builderDescID);
    }

    void ExternalModuleAssetBuilderInfo::RegisterComponentDesc(AZ::ComponentDescriptor* descriptor)
    {
        this->m_componentDescriptorList.push_back(descriptor);
    }

    template<typename T>
    T ExternalModuleAssetBuilderInfo::ResolveModuleFunction(const char* functionName, QStringList& missingFunctionsList)
    {
        T functionAddr = reinterpret_cast<T>(this->m_library.resolve(functionName));
        if (!functionAddr)
        {
            missingFunctionsList.append(QString(functionName));
        }
        return functionAddr;
    }
}


