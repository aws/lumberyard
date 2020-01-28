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
#include <Launcher_precompiled.h>
#include <Launcher.h>

#if AZ_TESTS_ENABLED

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/XML/rapidxml.h>
#include <AzTest/AzTest.h>
#include <AzTest/Platform.h>

#include <AzGameFramework/Application/GameApplication.h>

#include <CryLibrary.h>
#include <IEditorGame.h>
#include <IGameFramework.h>
#include <IGameStartup.h>
#include <ITimer.h>
#include <LegacyAllocator.h>
#include <ParseEngineConfig.h>

namespace LumberyardLauncher
{
    static const char* s_appDescriptorRootNodeName = "ObjectStream";

    /// Local class that manages the allocators and objects created from the allocators with the scope of the 
    /// lifetime of the object instantiated from this class
    class AppDescriptorParserManager 
    {
    public:
        /// Constructor
        ///
        /// Create the allocators necessary to use the tools we need to parse an application descriptor
        AppDescriptorParserManager()
        {
            AZ_Assert(!AZ::AllocatorInstance<CryStringAllocator>::IsReady(), "Expected allocator to not be initialized, hunt down the static that is initializing it");
            AZ::AllocatorInstance<CryStringAllocator>::Create();

            AZ_Assert(!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady(), "Expected allocator to not be initialized, hunt down the static that is initializing it");
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

            m_xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator);
        }

        /// Destructor
        ///
        /// Free up the objects and destroy the allocators that we are using for this session
        ~AppDescriptorParserManager()
        {
            azdestroy(m_xmlDoc);
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            AZ::AllocatorInstance<CryStringAllocator>::Destroy();
        }

        /// Parse the contents of an app descriptor file with the XML reader and return the xml root node if successful
        ///
        /// @param xmlData Pointer to the cooked convex mesh data.
        /// @return The rapidxml root node for the application descriptor if its a valid application descriptor XML, otherwise return a nullptr
        AZ::rapidxml::xml_node<char>* ReadDynamicModulesFromDescriptor(char* xmlData)
        {
            if (!m_xmlDoc->parse<0>(xmlData))
            {
                return nullptr;
            }
            AZ::rapidxml::xml_node<char>* xmlRootNode = m_xmlDoc->first_node();
            if (!xmlRootNode || azstricmp(xmlRootNode->name(), s_appDescriptorRootNodeName))
            {
                return nullptr;
            }
            return xmlRootNode;
        }
    private:
        AZ::rapidxml::xml_document<char>*   m_xmlDoc;
    };

    /// Calculate the path to the app descriptor based on the current bootstrap.cfg
    /// 
    /// @param outputPathBuffer     Pointer to the character buffer to receive the application descriptor path
    /// @param outputPathBufferSize The size of the buffer to receive the application descriptor path
    /// @return true if we were able to determine the path to the application descriptor file for the game, false if not
    static bool CalculateAppDescriptorPath(char *outputPathBuffer, size_t outputPathBufferSize)
    {
        SSystemInitParams systemInitParams;
        char pathToGameDescriptorFile[AZ_MAX_PATH_LEN] = { 0 };
        {
            // Use the Engine Config Parser to get the base path to the game assets
            CEngineConfig   engineConfig;

            memset(&systemInitParams, 0, sizeof(SSystemInitParams));
            engineConfig.CopyToStartupParams(systemInitParams);

            // Get the path to the game descriptor file
            AzGameFramework::GameApplication::GetGameDescriptorPath(pathToGameDescriptorFile, engineConfig.m_gameFolder);
        }

        char fullPathToGameDescriptorFile[AZ_MAX_PATH_LEN] = { 0 };
        const char* candidateRoots[] =
        {
            systemInitParams.rootPathCache,
            systemInitParams.rootPath,
            "."
        };

        for (const char* candidateRoot : candidateRoots)
        {
            // this is mostly to account for an oddity on Android when the assets are packed inside the APK,
            // in which the value returned from GetAppResourcePath contains a necessary trailing slash.
            const char* pathSep = "/";
            size_t pathLen = strlen(candidateRoot);
            if (pathLen >= 1 && candidateRoot[pathLen - 1] == '/')
            {
                pathSep = "";
            }
            azsnprintf(fullPathToGameDescriptorFile, AZ_MAX_PATH_LEN, "%s%s%s", candidateRoot, pathSep, pathToGameDescriptorFile);
            if (AZ::IO::SystemFile::Exists(fullPathToGameDescriptorFile))
            {
                azstrncpy(outputPathBuffer, outputPathBufferSize, fullPathToGameDescriptorFile, strlen(fullPathToGameDescriptorFile) + 1);
                return true;
            }
        }
        return false;
    }

    static const char* s_dynamicModuleNodeTypeValue = "{189CC2ED-FDDE-5680-91D4-9F630A79187F}";
    static const char* s_dynamicModuleNodeFieldValue = "dynamicLibraryPath";

    static const char* s_xmlAttributeType = "type";
    static const char* s_xmlAttributeValue = "value";
    static const char* s_xmlAttributeField = "field";


    /// Recursively search through an xml document for 'dynamicLibraryPaths' and collect the values
    ///
    /// @param node                 The root rapidxml node of the application descriptor
    /// @param baseExePath          The folder of the current executable that will be used to resolve the path to the dynamic modules
    /// @param dynamicModuleList    The list of resolved dynamic modules to add the results to
    /// @return The number of resolved dynamic modules that were discovered
    static size_t ScanAndCollectDynamicModulePaths(AZ::rapidxml::xml_node<char>* node, const char* baseExePath, std::list<std::string>& dynamicModuleList)
    {
        AZ::rapidxml::xml_attribute<char>* attr_type = node->first_attribute("type");
        AZ::rapidxml::xml_attribute<char>* attr_value = node->first_attribute("value");
        AZ::rapidxml::xml_attribute<char>* attr_field = node->first_attribute("field");

        size_t moduleCount = 0;

        if ((attr_type && (azstricmp(attr_type->value(), s_dynamicModuleNodeTypeValue) == 0)) &&
            (attr_field && (azstricmp(attr_field->value(), s_dynamicModuleNodeFieldValue) == 0)) &&
            attr_value)
        {
            char dynamicModuleFullPath[AZ_MAX_PATH_LEN] = { '\0' };
            azsnprintf(dynamicModuleFullPath, AZ_ARRAY_SIZE(dynamicModuleFullPath), "%s%s", baseExePath, attr_value->value());
            dynamicModuleList.push_back(std::string(dynamicModuleFullPath));
            moduleCount++;
        }

        for (AZ::rapidxml::xml_node<char>* childNode = node->first_node(); childNode; childNode = childNode->next_sibling())
        {
            moduleCount += ScanAndCollectDynamicModulePaths(childNode, baseExePath, dynamicModuleList);
        }
        return moduleCount;
    }


    /// For the current enabled game, determine the modules enabled to run unit tests on
    ///
    /// @param baseExePath          The folder of the current executable that will be used to resolve the path to the dynamic modules
    /// @param dynamicModuleList    The list of resolved dynamic modules to add the results to
    /// @return true if we could resolve dynamic modules to process for unit testing, false if not
    static bool CollectDynamicModulePathsFromCurrentGame(const char* baseExePath, std::list<std::string>& dynamicModuleList)
    {
        // Start with any modules that are not declared in the app descriptor
        // For now prepare a hard-coded static list. In the future we can either generate another configuration/manifest for these
        // modules
        const char* additionalTestModules[] =
        {
            "AzCoreTests",
            "GridMateTests"
        };
        char additionalModuleFileName[AZ_MAX_PATH_LEN] = { '\0' };
        char additionalModuleFullPath[AZ_MAX_PATH_LEN] = { '\0' };

        for (const char* additionalTestModule : additionalTestModules)
        {
            // Form the full path to the module and add it to the list if it exists
            azsnprintf(additionalModuleFullPath, AZ_ARRAY_SIZE(additionalModuleFullPath), "%s%s", baseExePath, additionalTestModule);
            dynamicModuleList.emplace_back(additionalModuleFullPath);
        }

        AppDescriptorParserManager  xmlDocManager;

        // Calculate the app descriptor path if possible
        char appDescriptorPath[AZ_MAX_PATH_LEN] = { '\0' };
        if (!CalculateAppDescriptorPath(appDescriptorPath, AZ_ARRAY_SIZE(appDescriptorPath)))
        {
            // Cannot determine the app descriptor path
            AZ_TracePrintf("UnitTestLauncher", "Unable to resolve the path to the game application descriptor.");
            return false;
        }

        // Read the app descritor's contents into a buffer for parsing and evaluating
        uint64_t fileSize = AZ::IO::SystemFile::Length(appDescriptorPath);
        if (fileSize == 0)
        {
            // The app descriptor path represents an invalid blank file
            AZ_TracePrintf("UnitTestLauncher", "Unable to open the game application descriptor file '%s'.", appDescriptorPath);
            return false;
        }
        std::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = 0;
        if (!AZ::IO::SystemFile::Read(appDescriptorPath, buffer.data()))
        {
            // Cannot read the contents of the app descriptor
            AZ_TracePrintf("UnitTestLauncher", "Unable to read the game application descriptor file '%s'.", appDescriptorPath);
            return false;
        }

        // Parse and validate the app descriptor XML into an root xml node
        AZ::rapidxml::xml_node<char>* appDescriptorRootNode = xmlDocManager.ReadDynamicModulesFromDescriptor(buffer.data());
        if (!appDescriptorRootNode)
        {
            // Cannot parse the XML contents of the app descriptor file (invalid xml or not in the schema we expect)
            AZ_TracePrintf("UnitTestLauncher", "Unable to parse the game application descriptor file '%s'.", appDescriptorPath);
            return false;
        }

        if (ScanAndCollectDynamicModulePaths(appDescriptorRootNode, baseExePath, dynamicModuleList)==0)
        {
            // Unable to collect any dynamic modules from the app descriptor
            AZ_TracePrintf("UnitTestLauncher", "Unable to find any unit test modules to process from the game application descriptor file '%s'.", appDescriptorPath);
            return false;
        }

        return true;
    }

    static const char* s_unitTestArgumentFilterModule = "--filter-module";
    static const char* s_unitTestArgumentExcludeModule = "--exclude-module";

    /// Perform preprocessing on the command line arguments to extract out arguments that are pertinent to this unit test launcher only
    /// 
    /// @param argc              The number of arguments from argc on input. On output, the number of arguments after the preprocessed arguments were stripped out
    /// @param argv              The array of arguments passed in from the command line to preprocess for our specific arguments
    /// @param baseExePath       The folder of the current executable that will be used to resolve the path to any filter or exclude modules from the arguments
    /// @param filterModules     The set of 'filter' modules (for inclusion) that was extrapolated from the arguments
    /// @param excludeModules    the set of 'exclude' modules (for exclusion) that was extrapolated from the arguments
    /// @return A newly allocated argument array list that represents the original argument list with the unit test launcher specific arguments extracted out. Note that this array must be deleted by the caller.
    char** PreProcessFilterModuleArgs(int &argc, char *argv[], const char* baseExePath, std::set<std::string>& filterModules, std::set<std::string>& excludeModules)
    {
        enum class CurrentArgumentType
        {
            Child,
            FilterModule,
            ExcludeModule
        };
        CurrentArgumentType currentArgFlag = CurrentArgumentType::Child;

        char filterModuleFullPath[AZ_MAX_PATH_LEN] = { '\0' };

        char** evaluatedArgVs = new char*[argc];

        int skippedArgIndex = 0;

        for (int arg_index = 0; arg_index < argc; arg_index++)
        {
            if (azstricmp(argv[arg_index], s_unitTestArgumentFilterModule) == 0)
            {
                // This the start of a 'filter-module' parameter. Expect the next value to be the filter module
                currentArgFlag = CurrentArgumentType::FilterModule;
            }
            else if (azstricmp(argv[arg_index], s_unitTestArgumentExcludeModule) == 0)
            {
                currentArgFlag = CurrentArgumentType::ExcludeModule;
            }
            else if (currentArgFlag == CurrentArgumentType::Child)
            {
                // The argument is an argument meant to be passed to the child unit test modules
                evaluatedArgVs[skippedArgIndex++] = argv[arg_index];
            }
            else
            {
                // Form the full path to the module and add it to the appropriate list
                azsnprintf(filterModuleFullPath, AZ_ARRAY_SIZE(filterModuleFullPath), "%s%s", baseExePath, argv[arg_index]);
                switch (currentArgFlag)
                {
                case CurrentArgumentType::FilterModule:
                    filterModules.emplace(filterModuleFullPath);
                    break;
                case CurrentArgumentType::ExcludeModule:
                    excludeModules.emplace(filterModuleFullPath);
                    break;
                }
            }
        }
        // Nullify the rest of the pre-allocated argumennt list
        int newArgC = skippedArgIndex;
        while (skippedArgIndex < argc)
        {
            evaluatedArgVs[skippedArgIndex++] = nullptr;
        }
        // Update the new size of the argument list
        argc = newArgC;
        return evaluatedArgVs;
    }

    /// Run all the unit tests applicable for the current game project
    ///
    /// @param baseExePath      The folder of the current executable that will be used to resolve the paths for the unit test modules
    /// @param argv             The number of arguments from argc on input.
    /// @param argc             The array of arguments passed in from the command line 
    ReturnCode RunUnitTests(const char* baseExePath, int argc, char *argv[])
    {
#if defined(AZ_MONOLITHIC_BUILD)
        // Launching modules is not support in monolithic mode
        AZ_TracePrintf("UnitTestLauncher", "The UnitTest Launcher is disabled for monolithic builds.");
        return ReturnCode::ErrUnitTestNotSupported;
#else
        // Collect the potential modules to attempt to run the unit test hooks from
        std::list<std::string> moduleToList;
        if (!CollectDynamicModulePathsFromCurrentGame(baseExePath, moduleToList))
        {
            return ReturnCode::ErrAppDescriptor;
        }

        // Extract the arguments that are specific to the unit test launcher for filtering (--filter-module) and exclusion (--exclude-module)
        std::set<std::string>  filterModules;
        std::set<std::string>  excludeModules;
        int                    processedArgC = argc;
        char**                 evaluatedArgV = PreProcessFilterModuleArgs(processedArgC, argv, baseExePath, filterModules, excludeModules);


        // Execute the unit test calls on all the modules discovered and defined and track the success and failures
        ReturnCode result = ReturnCode::Success;

        std::list<std::string> successfulTestModules;
        std::list<std::string> failedTestModules;
        std::list<std::string> missingModules;
        std::list<std::string> invalidModules;

        AZ::Test::Platform testPlatform = AZ::Test::GetPlatform();
        for (std::string& module : moduleToList)
        {
            if ((filterModules.size() > 0) && (filterModules.find(module) == filterModules.end()))
            {
                // A module filter was set, only allow modules that conform to this filter
                continue;
            }
            if (excludeModules.find(module) != excludeModules.end())
            {
                // An exclude filter was set, skip any modules that are in the exclusion set
                continue;
            }
            auto testModule = testPlatform.GetModule(module);
            if (!testModule->IsValid())
            {
                // The module was invalid (or doesnt exist)
                AZ_TracePrintf("UnitTestLauncher", "Unable to run unit test on '%s'. Moduile does not exist or is invalid.", module.c_str());
                missingModules.push_back(module);
                continue;
            }

            auto testFunction = testModule->GetFunction("AzRunUnitTests");
            if (!testFunction->IsValid())
            {
                AZ_TracePrintf("UnitTestLauncher", "Unable to run unit test on '%s'. Module is not a valid unit test module.", module.c_str());
                invalidModules.push_back(module);
                continue;
            }

            auto testResult = (*testFunction)(processedArgC, evaluatedArgV);
            if (testResult == 0)
            {
                successfulTestModules.push_back(module);
            }
            else
            {
                failedTestModules.push_back(module);
                result = ReturnCode::ErrUnitTestFailure;
            }
        }

        // Report the results
        AZ_TracePrintf("UnitTestLauncher", "[GTEST] Test Results\n\n");

        for (std::string& successfulTestModule : successfulTestModules)
        {
            AZ_TracePrintf("UnitTestLauncher", "[GTEST][PASSED] %s\n", successfulTestModule.c_str());
        }
        for (std::string& failedTestModule : failedTestModules)
        {
            AZ_TracePrintf("UnitTestLauncher", "[GTEST][FAILED] %s\n", failedTestModule.c_str());
        }
        for (std::string& invalidModule : invalidModules)
        {
            AZ_TracePrintf("UnitTestLauncher", "[GTEST][INVALID] %s\n", invalidModule.c_str());
        }
        for (std::string& missingModule : missingModules)
        {
            AZ_TracePrintf("UnitTestLauncher", "[GTEST][MISSING] %s\n", missingModule.c_str());
        }

        AZ_TracePrintf("UnitTestLauncher", "[GTEST] %d Modules Succeeded\n", successfulTestModules.size());
        AZ_TracePrintf("UnitTestLauncher", "[GTEST] %d Modules Failed\n", failedTestModules.size());
        if (invalidModules.size() > 0)
        {
            AZ_TracePrintf("UnitTestLauncher", "[GTEST] %d Modules Invalid\n", invalidModules.size());
        }
        if (missingModules.size() > 0)
        {
            AZ_TracePrintf("UnitTestLauncher", "[GTEST] %d Modules Missing\n", missingModules.size());
        }
        delete[] evaluatedArgV;
        AZ_TracePrintf("UnitTestLauncher", "[GTEST][COMPLETE] Unit test runs completed\n");

        return result;
#endif // defined(AZ_MONOLITHIC_BUILD)
    }
}
#endif // AZ_TESTS_ENABLED

