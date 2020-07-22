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

#include <PythonSystemComponent.h>

#include <EditorPythonBindings/EditorPythonBindingsBus.h>

#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <osdefs.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/API/BootstrapReaderBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/CommandLine/CommandRegistrationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

// this is called the first time a Python script contains "import azlmbr"
PYBIND11_EMBEDDED_MODULE(azlmbr, m)
{
    EditorPythonBindings::EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindings::EditorPythonBindingsNotificationBus::Events::OnImportModule, m.ptr());
}

namespace RedirectOutput
{
    using RedirectOutputFunc = AZStd::function<void(const char*)>;

    struct RedirectOutput
    {
        PyObject_HEAD
        RedirectOutputFunc write;
    };

    PyObject* RedirectWrite(PyObject* self, PyObject* args)
    {
        std::size_t written(0);
        RedirectOutput* selfimpl = reinterpret_cast<RedirectOutput*>(self);
        if (selfimpl->write)
        {
            char* data;
            if (!PyArg_ParseTuple(args, "s", &data))
            {
                return PyLong_FromSize_t(0);
            }
            selfimpl->write(data);
            written = strlen(data);
        }
        return PyLong_FromSize_t(written);
    }

    PyObject* RedirectFlush(PyObject* self, PyObject* args)
    {
        // no-op
        return Py_BuildValue("");
    }

    PyMethodDef RedirectMethods[] =
    {
        {"write", RedirectWrite, METH_VARARGS, "sys.stdout.write"},
        {"flush", RedirectFlush, METH_VARARGS, "sys.stdout.flush"},
        {"write", RedirectWrite, METH_VARARGS, "sys.stderr.write"},
        {"flush", RedirectFlush, METH_VARARGS, "sys.stderr.flush"},
        {0, 0, 0, 0} // sentinel
    };

    PyTypeObject RedirectOutputType =
    {
        PyVarObject_HEAD_INIT(0, 0)
        "azlmbr_redirect.RedirectOutputType", // tp_name
        sizeof(RedirectOutput), /* tp_basicsize */
        0,                    /* tp_itemsize */
        0,                    /* tp_dealloc */
        0,                    /* tp_print */
        0,                    /* tp_getattr */
        0,                    /* tp_setattr */
        0,                    /* tp_reserved */
        0,                    /* tp_repr */
        0,                    /* tp_as_number */
        0,                    /* tp_as_sequence */
        0,                    /* tp_as_mapping */
        0,                    /* tp_hash  */
        0,                    /* tp_call */
        0,                    /* tp_str */
        0,                    /* tp_getattro */
        0,                    /* tp_setattro */
        0,                    /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,   /* tp_flags */
        "azlmbr_redirect objects", /* tp_doc */
        0,                    /* tp_traverse */
        0,                    /* tp_clear */
        0,                    /* tp_richcompare */
        0,                    /* tp_weaklistoffset */
        0,                    /* tp_iter */
        0,                    /* tp_iternext */
        RedirectMethods,      /* tp_methods */
        0,                    /* tp_members */
        0,                    /* tp_getset */
        0,                    /* tp_base */
        0,                    /* tp_dict */
        0,                    /* tp_descr_get */
        0,                    /* tp_descr_set */
        0,                    /* tp_dictoffset */
        0,                    /* tp_init */
        0,                    /* tp_alloc */
        0                     /* tp_new */
    };

    PyModuleDef RedirectOutputModule = { PyModuleDef_HEAD_INIT, "azlmbr_redirect", 0, -1, 0, };

    // Internal state
    PyObject* g_redirect_stdout = nullptr;
    PyObject* g_redirect_stdout_saved = nullptr;
    PyObject* g_redirect_stderr = nullptr;
    PyObject* g_redirect_stderr_saved = nullptr;

    PyMODINIT_FUNC PyInit_RedirectOutput(void)
    {
        g_redirect_stdout = nullptr;
        g_redirect_stdout_saved = nullptr;
        g_redirect_stderr = nullptr;
        g_redirect_stderr_saved = nullptr;

        RedirectOutputType.tp_new = PyType_GenericNew;
        if (PyType_Ready(&RedirectOutputType) < 0)
        { 
            return 0;
        }

        PyObject* m = PyModule_Create(&RedirectOutputModule);
        if (m)
        {
            Py_INCREF(&RedirectOutputType);
            PyModule_AddObject(m, "Redirect", reinterpret_cast<PyObject*>(&RedirectOutputType));
        }
        return m;
    }

    void SetRedirection(const char* funcname, PyObject*& saved, PyObject*& current, RedirectOutputFunc func)
    {
        if (PyType_Ready(&RedirectOutputType) < 0)
        {
            AZ_Warning("python", false, "RedirectOutputType not ready!");
            return;
        }

        if (!current)
        {
            saved = PySys_GetObject(funcname); // borrowed
            current = RedirectOutputType.tp_new(&RedirectOutputType, 0, 0);
        }

        RedirectOutput* redirectOutput = reinterpret_cast<RedirectOutput*>(current);
        redirectOutput->write = func;
        PySys_SetObject(funcname, current);
    }

    void ResetRedirection(const char* funcname, PyObject*& saved, PyObject*& current)
    {
        if (current)
        { 
            PySys_SetObject(funcname, saved);
        }
        Py_XDECREF(current);
        current = nullptr;
    }

    PyObject* s_RedirectModule = nullptr;

    void Intialize(PyObject* module)
    {
        using namespace AzToolsFramework;

        s_RedirectModule = module;

        SetRedirection("stdout", g_redirect_stdout_saved, g_redirect_stdout, [](const char* msg) 
        {
            EditorPythonConsoleNotificationBus::Broadcast(&EditorPythonConsoleNotificationBus::Events::OnTraceMessage, msg);
        });

        SetRedirection("stderr", g_redirect_stderr_saved, g_redirect_stderr, [](const char* msg)
        {
            EditorPythonConsoleNotificationBus::Broadcast(&EditorPythonConsoleNotificationBus::Events::OnErrorMessage, msg);
        });

        PySys_WriteStdout("RedirectOutput installed");
    }

    void Shutdown()
    {
        ResetRedirection("stdout", g_redirect_stdout_saved, g_redirect_stdout);
        ResetRedirection("stderr", g_redirect_stderr_saved, g_redirect_stderr);
        Py_XDECREF(s_RedirectModule);
        s_RedirectModule = nullptr;
    }
} // namespace RedirectOutput

namespace EditorPythonBindings
{
    AzFramework::CommandResult Command_ExecuteByFilename(const AZStd::vector<AZStd::string_view>& args)
    {
        if (args.size() < 2)
        {
            // We expect at least "pyRunFile" and the filename
            return AzFramework::CommandResult::ErrorWrongNumberOfArguments;
        }
        else if (args.size() == 2)
        {
            // If we only have "pyRunFile filename", there are no args to pass through.
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilename, args[1]);
        }
        else
        {
            // We have "pyRunFile filename x y z", so copy everything past filename into a new vector
            AZStd::vector<AZStd::string_view> trimmedArgs(&args[2], args.end());
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, args[1], trimmedArgs);
        }

        return AzFramework::CommandResult::Success;
    }

    void PythonSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PythonSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<PythonSystemComponent>("PythonSystemComponent", "The Python interpreter")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void PythonSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PythonSystemService", 0x98e7cd4d));
    }

    void PythonSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PythonSystemService", 0x98e7cd4d));
    }

    void PythonSystemComponent::Activate()
    {
        AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Register(this);
        AzToolsFramework::EditorPythonRunnerRequestBus::Handler::BusConnect();
    }

    void PythonSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorPythonRunnerRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Unregister(this);
        StopPython();
    }

    bool PythonSystemComponent::StartPython()
    {
        struct ReleaseInitalizeWaiterScope final
        {
            using ReleaseFunction = AZStd::function<void(void)>;

            ReleaseInitalizeWaiterScope(ReleaseFunction releaseFunction)
            {
                m_releaseFunction = AZStd::move(releaseFunction);
            }
            ~ReleaseInitalizeWaiterScope()
            {
                m_releaseFunction();
            }
            ReleaseFunction m_releaseFunction;
        };
        ReleaseInitalizeWaiterScope scope([this]()
        {
            m_initalizeWaiter.release(m_initalizeWaiterCount);
            m_initalizeWaiterCount = 0;
        });

        if (Py_IsInitialized())
        {
            AZ_Warning("python", false, "Python is already active!");
            return false;
        }

        PythonPathStack pythonPathStack;
        DiscoverPythonPaths(pythonPathStack);

        EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindingsNotificationBus::Events::OnPreInitialize);
        if (StartPythonInterpreter(pythonPathStack))
        {
            using namespace AzFramework;
            bool result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::RegisterCommand, "pyRunFile", "Runs the Python script from the project.", CommandFlags::Development, Command_ExecuteByFilename);
            AZ_Warning("python", result, "Failed to register console command 'pyRunFile'");

            EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindingsNotificationBus::Events::OnPostInitialize);

            // initialize internal base module and bootstrap scripts
            ExecuteByString("import azlmbr");
            ExecuteBoostrapScripts(pythonPathStack);
            return true;
        }
        return false;
    }

    bool PythonSystemComponent::StopPython()
    {
        if (!Py_IsInitialized())
        {
            AZ_Warning("python", false, "Python is not active!");
            return false;
        }

        using namespace AzFramework;
        bool result = false;
        CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::UnregisterCommand, "pyRunFile");
        AZ_Warning("python", result, "Failed to unregister console command 'pyRunFile'");

        EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindingsNotificationBus::Events::OnPreFinalize);
        AzToolsFramework::EditorPythonRunnerRequestBus::Handler::BusDisconnect();

        result = StopPythonInterpreter();
        EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindingsNotificationBus::Events::OnPostFinalize);
        return result;
    }

    void PythonSystemComponent::WaitForInitialization()
    {
        m_initalizeWaiterCount++;
        m_initalizeWaiter.acquire();
    }

    void PythonSystemComponent::DiscoverPythonPaths(PythonPathStack& pythonPathStack)
    {
        // the order of the Python paths is the order the Python bootstrap scripts will execute

        AZStd::string gameFolder;
        AzFramework::BootstrapReaderRequestBus::Broadcast(&AzFramework::BootstrapReaderRequestBus::Events::SearchConfigurationForKey, "sys_game_folder", false, gameFolder);
        if (gameFolder.empty())
        {
            return;
        }

        auto resolveScriptPath = [&pythonPathStack](AZStd::string_view path)
        {
            AZStd::string editorScriptsPath;
            AzFramework::StringFunc::Path::Join(path.data(), "Editor/Scripts", editorScriptsPath);
            if (AZ::IO::SystemFile::Exists(editorScriptsPath.c_str()))
            {
                pythonPathStack.emplace_back(editorScriptsPath);
            }
        };

        // The discovery order will be:
        //   - engine
        //   - gems
        //   - project
        //   - user(dev)

        // engine
        const char* engineRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
        resolveScriptPath(engineRoot);

        // gems
        auto moduleCallback = [this, &pythonPathStack, resolveScriptPath, engineRoot](const AZ::ModuleData& moduleData) -> bool
        {
            if (moduleData.GetDynamicModuleHandle())
            {
                const AZ::OSString& modulePath = moduleData.GetDynamicModuleHandle()->GetFilename();
                AZStd::string fileName;
                AzFramework::StringFunc::Path::GetFileName(modulePath.c_str(), fileName);

                AZStd::vector<AZStd::string> tokens;
                AzFramework::StringFunc::Tokenize(fileName.c_str(), tokens, '.');
                if (tokens.size() > 2 && tokens[0] == "Gem")
                {
                    resolveScriptPath(AZStd::string::format("%s/Gems/%s", engineRoot, tokens[1].c_str()));
                }
            }
            return true;
        };
        AZ::ModuleManagerRequestBus::Broadcast(&AZ::ModuleManagerRequestBus::Events::EnumerateModules, moduleCallback);

        // project
        const char* appRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);
        resolveScriptPath(AZStd::string::format("%s/%s", appRoot, gameFolder.c_str()));

        // user
        AZStd::string assetsType;
        AzFramework::BootstrapReaderRequestBus::Broadcast(&AzFramework::BootstrapReaderRequestBus::Events::SearchConfigurationForKey, "assets", false, assetsType);
        if (!assetsType.empty())
        {
            // the pattern to user path is <appRoot>/Cache/<gameFolder>/<assetsType>/user e.g. c:/myroot/dev/Cache/MyProject/pc/user
            AZStd::string userRelativePath = AZStd::string::format("Cache/%s/%s/user", gameFolder.c_str(), assetsType.c_str());
            AZStd::string userCachePath;
            AzFramework::StringFunc::Path::Join(appRoot, userRelativePath.c_str(), userCachePath);
            resolveScriptPath(userCachePath);
        }
    }

    void PythonSystemComponent::ExecuteBoostrapScripts(const PythonPathStack& pythonPathStack)
    {
        for(const auto& path : pythonPathStack)
        {
            AZStd::string bootstrapPath;
            AzFramework::StringFunc::Path::Join(path.c_str(), "bootstrap.py", bootstrapPath);
            if (AZ::IO::SystemFile::Exists(bootstrapPath.c_str()))
            {
                ExecuteByFilename(bootstrapPath);
            }
        }
    }

    bool PythonSystemComponent::StartPythonInterpreter(const PythonPathStack& pythonPathStack)
    {
        AZStd::unordered_set<AZStd::string> pyPackageSites(pythonPathStack.begin(), pythonPathStack.end());

        // If the passed in root path was relative, assume the user wants it in the app root folder and then generate an absolute path.
        const char* appRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);
        pyPackageSites.insert(appRoot);

        const char* engineRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
        pyPackageSites.insert(engineRoot);

        // set PYTHON_PATH
        char bufferPath[AZ_MAX_PATH_LEN];
        azsnprintf(bufferPath, AZ_MAX_PATH_LEN, "Tools/Python/3.7.5/%s/Lib", PY_PLATFORM);
        AZStd::string libPath;
        AzFramework::StringFunc::Path::Join(engineRoot, bufferPath, libPath);
        if (!AZ::IO::SystemFile::Exists(libPath.c_str()))
        {
            AZ_Warning("python", false, "Python library path must exist! path:%s", libPath.c_str());
            return false;
        }
        pyPackageSites.insert(libPath);

        // set path to dynamic link libraries (.pyd, .dll files)
        azsnprintf(bufferPath, AZ_MAX_PATH_LEN, "Tools/Python/3.7.5/%s/DLLs", PY_PLATFORM);
        AZStd::string pydPath;
        AzFramework::StringFunc::Path::Join(engineRoot, bufferPath, pydPath);
        if (!AZ::IO::SystemFile::Exists(pydPath.c_str()))
        {
            AZ_Warning("python", false, "Python dynamic library path must exist! path:%s", pydPath.c_str());
            return false;
        }
        pyPackageSites.insert(pydPath);

        const char pathSeparator[] = { DELIM, 0 };
        AZStd::string fullPyPath;
        AzFramework::StringFunc::Join(fullPyPath, pyPackageSites.begin(), pyPackageSites.end(), pathSeparator);

        AZStd::wstring pyPathName;
        AZStd::to_wstring(pyPathName, fullPyPath.c_str());
        Py_SetPath(pyPathName.c_str());

        // set PYTHON_HOME
        azsnprintf(bufferPath, AZ_MAX_PATH_LEN, "Tools/Python/3.7.5/%s", PY_PLATFORM);
        AZStd::string pyBasePath;
        AzFramework::StringFunc::Path::Join(engineRoot, bufferPath, pyBasePath);
        if (!AZ::IO::SystemFile::Exists(pyBasePath.c_str()))
        {
            AZ_Warning("python", false, "Python home path must exist! path:%s", pyBasePath.c_str());
            return false;
        }
        AZStd::wstring pyHomePath;
        AZStd::to_wstring(pyHomePath, pyBasePath);
        Py_SetPythonHome(const_cast<wchar_t*>(pyHomePath.c_str()));

        // display basic Python information
        AZ_TracePrintf("python", "Py_GetVersion=%s \n", Py_GetVersion());
        AZ_TracePrintf("python", "Py_GetPath=%ls \n", Py_GetPath());
        AZ_TracePrintf("python", "Py_GetExecPrefix=%ls \n", Py_GetExecPrefix());
        AZ_TracePrintf("python", "Py_GetProgramFullPath=%ls \n", Py_GetProgramFullPath());

        PyImport_AppendInittab("azlmbr_redirect", RedirectOutput::PyInit_RedirectOutput);
        try
        {
            const bool initializeSignalHandlers = true;
            pybind11::initialize_interpreter(initializeSignalHandlers);

            RedirectOutput::Intialize(PyImport_ImportModule("azlmbr_redirect"));

            // print Python version using AZ logging
            const int verRet = PyRun_SimpleStringFlags("import sys \nprint (sys.version) \n", nullptr);
            AZ_Error("python", verRet == 0, "Error trying to fetch the version number in Python!");
            return verRet == 0 && !PyErr_Occurred();
        }
        catch (const std::exception& e)
        {
            AZ_Warning("python", false, "Py_Initialize() failed with %s!", e.what());
            return false;
        }
    }

    bool PythonSystemComponent::StopPythonInterpreter()
    {
        if (Py_IsInitialized())
        {
            RedirectOutput::Shutdown();
            pybind11::finalize_interpreter();
        }
        else
        {
            AZ_Warning("python", false, "Did not finalize since Py_IsInitialized() was false.");
        }
        return !PyErr_Occurred();
    }

    void PythonSystemComponent::ExecuteByString(AZStd::string_view script)
    {
        if (!Py_IsInitialized())
        {
            AZ_Error("python", false, "Can not ExecuteByString() since the embeded Python VM is not ready.");
            return;
        }

        if (!script.empty())
        {
            // Acquire GIL before calling Python code
            pybind11::gil_scoped_acquire acquire;

            PyCompilerFlags flags;
            flags.cf_flags = 0;
            const int returnCode = PyRun_SimpleStringFlags(script.data(), &flags);
            if (returnCode != 0)
            {
                AZ_Warning("python", false, "Detected script failure with return code %d.", returnCode);
            }
        }
    }

    void PythonSystemComponent::ExecuteByFilename(AZStd::string_view filename)
    {
        AZStd::vector<AZStd::string_view> args;
        ExecuteByFilenameWithArgs(filename, args);
    }

    void PythonSystemComponent::ExecuteByFilenameAsTest(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args)
    {
        const Result evalResult = EvaluateFile(filename, args);
        if (evalResult == Result::Okay)
        {
            // all good, the test script will need to exit the application now
            return;
        }
        else
        {
            // something when wrong with executing the test script
            AZ::Debug::Trace::Terminate(1);
        }
    }

    void PythonSystemComponent::ExecuteByFilenameWithArgs(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args)
    {
        EvaluateFile(filename, args);
    }

    PythonSystemComponent::Result PythonSystemComponent::EvaluateFile(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args)
    {
        if (!Py_IsInitialized())
        {
            AZ_Error("python", false, "Can not evaluate file since the embedded Python VM is not ready.");
            return Result::Error_IsNotInitialized;
        }

        if (filename.empty())
        {
            AZ_Error("python", false, "Invalid empty filename detected.");
            return Result::Error_InvalidFilename;
        }

        // support the alias version of a script such as @devroot@/Editor/Scripts/select_story_anim_objects.py
        AZStd::string theFilename(filename);
        {
            char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(theFilename.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
            theFilename = resolvedPath;
        }

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(theFilename.c_str()))
        {
            AZ_Error("python", false, "Missing Python file named (%s)", theFilename.c_str());
            return Result::Error_MissingFile;
        }

        FILE* file = _Py_fopen(theFilename.data(), "rb");
        if (!file)
        {
            AZ_Error("python", false, "Missing Python file named (%s)", theFilename.c_str());
            return Result::Error_FileOpenValidation;
        }

        Result pythonScriptResult = Result::Okay;
        try
        {
            // Acquire GIL before calling Python code
            pybind11::gil_scoped_acquire acquire;
        
            // Create standard "argc" / "argv" command-line parameters to pass in to the Python script via sys.argv.
            // argc = number of parameters.  This will always be at least 1, since the first parameter is the script name.
            // argv = the list of parameters, in wchar format.
            // Our expectation is that the args passed into this function does *not* already contain the script name.
            int argc = aznumeric_cast<int>(args.size()) + 1;

            // Note:  This allocates from PyMem to ensure that Python has access to the memory.
            wchar_t** argv = static_cast<wchar_t**>(PyMem_Malloc(argc * sizeof(wchar_t*)));

            // Python 3.x is expecting wchar* strings for the command-line args.
            argv[0] = Py_DecodeLocale(theFilename.c_str(), nullptr);

            for (int arg = 0; arg < args.size(); arg++)
            {
                argv[arg + 1] = Py_DecodeLocale(args[arg].data(), nullptr);
            }

            // Tell Python the command-line args.
            // Note that this has a side effect of adding the script's path to the set of directories checked for "import" commands.
            PySys_SetArgv(argc, argv);

            PyCompilerFlags flags;
            flags.cf_flags = 0;
            const int bAutoCloseFile = true;
            const int returnCode = PyRun_SimpleFileExFlags(file, theFilename.c_str(), bAutoCloseFile, &flags);
            if (returnCode != 0)
            {
                AZStd::string message = AZStd::string::format("Detected script failure in Python script(%s); return code %d!", theFilename.c_str(), returnCode);
                AZ_Warning("python", false, message.c_str());
                using namespace AzToolsFramework;
                EditorPythonConsoleNotificationBus::Broadcast(&EditorPythonConsoleNotificationBus::Events::OnExceptionMessage, message.c_str());
                pythonScriptResult = Result::Error_PythonException;
            }

            // Free any memory allocated for the command-line args.
            for (int arg = 0; arg < argc; arg++)
            {
                PyMem_RawFree(argv[arg]);
            }
            PyMem_Free(argv);
        }
        catch (const std::exception& e)
        {
            AZ_Error("python", false, "Detected an internal exception %s while running script (%s)!", e.what(), theFilename.c_str());
            return Result::Error_InternalException;
        }
        return pythonScriptResult;
    }

} // namespace EditorPythonBindings
