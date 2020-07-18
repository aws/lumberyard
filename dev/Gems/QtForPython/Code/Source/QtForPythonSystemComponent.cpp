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
#include <QtForPythonSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

#include <AzCore/IO/SystemFile.h>
#include <AzFramework/IO/LocalFileIO.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // (qwidget.h) 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QWidget>
AZ_POP_DISABLE_WARNING

namespace QtForPython
{
    void QtForPythonSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<QtForPythonSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            serialize->RegisterGenericType<QWidget>();
        }

        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->EBus<QtForPythonRequestBus>("QtForPythonRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "qt")
                ->Event("IsActive", &QtForPythonRequestBus::Events::IsActive)
                ->Event("GetQtBootstrapParameters", &QtForPythonRequestBus::Events::GetQtBootstrapParameters)
                ;

            behavior->Class<QtBootstrapParameters>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "qt")
                ->Property("qtBinaryFolder", BehaviorValueProperty(&QtBootstrapParameters::m_qtBinaryFolder))
                ->Property("qtPluginsFolder", BehaviorValueProperty(&QtBootstrapParameters::m_qtPluginsFolder))
                ->Property("mainWindowId", BehaviorValueProperty(&QtBootstrapParameters::m_mainWindowId))
                ->Property("pySidePackageFolder", BehaviorValueProperty(&QtBootstrapParameters::m_pySidePackageFolder))
                ;
        }
    }

    void QtForPythonSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("QtForPythonService", 0x946c0ae9));
    }

    void QtForPythonSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("QtForPythonService", 0x946c0ae9));
    }

    void QtForPythonSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("PythonSystemService", 0x98e7cd4d));
    }

    void QtForPythonSystemComponent::Activate()
    {
        QtForPythonRequestBus::Handler::BusConnect();
    }

    void QtForPythonSystemComponent::Deactivate()
    {
        QtForPythonRequestBus::Handler::BusDisconnect();
    }

    bool QtForPythonSystemComponent::IsActive() const
    {
        return AzToolsFramework::EditorPythonRunnerRequestBus::HasHandlers();
    }

    QtBootstrapParameters QtForPythonSystemComponent::GetQtBootstrapParameters() const
    {
        QtBootstrapParameters params;

        char devroot[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@devroot@", devroot, AZ_MAX_PATH_LEN);
       
#if defined(Q_OS_WIN)
        const char* platform = "windows";
#else
#error Unsupported OS platform for this QtForPython gem
#endif

#if defined(AZ_DEBUG_BUILD)
        const char* build = "debug";
#else
        const char* build = "release";
#endif
        // prepare the platform and build specific PySide2 package folder
        AZ::StringFunc::Path::Join(devroot, "Gems/QtForPython/3rdParty/pyside2", params.m_pySidePackageFolder);
        AZ::StringFunc::Path::Join(params.m_pySidePackageFolder.c_str(), platform, params.m_pySidePackageFolder);
        AZ::StringFunc::Path::Join(params.m_pySidePackageFolder.c_str(), build, params.m_pySidePackageFolder);

        params.m_mainWindowId = 0;
        using namespace AzToolsFramework;
        QWidget* activeWindow = nullptr;
        EditorRequestBus::BroadcastResult(activeWindow, &EditorRequests::GetMainWindow);
        if (activeWindow)
        {
            // store the Qt main window so that scripts can hook into the main menu and/or docking framework
            params.m_mainWindowId = aznumeric_cast<AZ::u64>(activeWindow->winId());
        }

        // prepare the folder where the build system placed the QT binary files
        AZ::ComponentApplicationBus::BroadcastResult(params.m_qtBinaryFolder, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);

        // prepare the QT plugins folder
        AZ::StringFunc::Path::Join(params.m_qtBinaryFolder.c_str(), "qtlibs/plugins", params.m_qtPluginsFolder);
        return params;
    }
}
