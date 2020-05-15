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

#include "stdafx.h"
#include "PythonWorker.h"
#include "CloudCanvasLogger.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>

#include <AzCore/EBus/Results.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

#include <PythonWorker.moc>

PythonWorker::PythonWorker(
    QSharedPointer<CloudCanvasLogger> logger,
    const char* rootDirectoryPath,
    const char* gameDirectoryPath,
    const char* userDirectoryPath)
    : QObject()
    , m_logger(logger)
    , m_rootDirectoryPathQ{rootDirectoryPath}
    , m_rootDirectoryPath{rootDirectoryPath}
    , m_gameDirectoryPath{gameDirectoryPath}
    , m_userDirectoryPath{userDirectoryPath}
{
    qRegisterMetaType<PythonWorkerRequestId>("PythonWorkerRequestId");
    moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, this, &PythonWorker::Reset);
    AZ::Interface<PythonWorkerRequestsInterface>::Register(this);
}

PythonWorker::~PythonWorker()
{
    AZ::Interface<PythonWorkerRequestsInterface>::Unregister(this);
    if (m_thread.isRunning())
    {
        m_thread.quit();
        m_thread.wait();
    }
}

void PythonWorker::Start()
{
    m_thread.start();
    m_isStarted = true;
}

QString PythonWorker::MakeRootPath(const char* relativePath, bool& ok)
{
    QDir root {
        m_rootDirectoryPathQ
    };
    QDir child {
        root.absoluteFilePath(relativePath)
    };

    if (!child.exists())
    {
        m_logger->LogError(QString {"Disabled because %1 does not exist."}.arg(child.absolutePath()));
        ok = false;
    }

    return child.absolutePath();
}

void PythonWorker::LockPython()
{
    PyEval_AcquireThread(m_threadState);
}

void PythonWorker::UnlockPython()
{
    PyEval_ReleaseThread(m_threadState);
}

void PythonWorker::ResetAsync()
{
    QMetaObject::invokeMethod(this, "Reset", Qt::QueuedConnection);
}

void PythonWorker::Reset()
{
    DestroyInterpreter();

    bool ok = true;
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    QString pythonPath = MakeRootPath(QString(DEFAULT_LY_PYTHONHOME_RELATIVE).toUtf8().data(), ok);
#else
#error Unsupported Platform for the Editor
#endif
    if (!ok)
    {
        AZ_Warning("MaglevControlPanel", false, "Could not detect Python path.");
        return;
    }

    if (CreateInterpreter(pythonPath))
    {
        if (!InitializeInterpreter(pythonPath))
        {
            DestroyInterpreter();
        }
    }
}

bool PythonWorker::CreateInterpreter(const QString& pythonPath)
{
    auto&& editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
    if (editorPythonEventsInterface)
    {
        editorPythonEventsInterface->WaitForInitialization();
    }

    if (!Py_IsInitialized())
    {
        Py_SetProgramName(QString("%1/Lib").arg(pythonPath).toStdWString().c_str());
        Py_SetPythonHome(pythonPath.toStdWString().c_str());

        // Init without importing site.py, this will be lazyly added when paths are appended
        Py_NoSiteFlag = 1;
         
        Py_Initialize();
        PyEval_InitThreads();
        PyEval_ReleaseLock();

        if (!Py_IsInitialized())
        {
            m_logger->LogError("Disabled because Python is not initialized.");
            return false;
        }
    }

    if (!PyEval_ThreadsInitialized())
    {
        m_logger->LogError("Disabled because Python threading is not initialized.");
        return false;
    }

    PyEval_AcquireLock();

    m_threadState = Py_NewInterpreter();

    if (!m_threadState)
    {
        m_logger->LogError("Disabled because a new Python interpreter could not be created.");
        PyEval_ReleaseLock();
        return false;
    }

    PyThreadState_Swap(NULL);

    PyEval_ReleaseLock();

    return true;
}

void PythonWorker::DestroyInterpreter()
{
    if (m_threadState)
    {
        LockPython();
        PyThreadState_Clear(m_threadState);
        Py_EndInterpreter(m_threadState);
        PyEval_ReleaseLock(); // Don't do UnlockPython because we no longer have a thread state.
        m_threadState = nullptr;
    }
}

bool PythonWorker::InitializeInterpreter(const QString& pythonPath)
{
    // Locate needed Python libraries

    bool ok = true;
    QString lmbr_aws_path = MakeRootPath("Tools/lmbr_aws", ok);
    if (!ok)
    {
        return false;
    }

    // Load and initialize the Python libraries

    bool successful = true;

    LockPython();

    try
    {
        // Load main.

        py::object main = py::module::import("__main__");
        py::object globals = main.attr("__dict__");

        // Setup sys.path
        int result = PyRun_SimpleString("import sys");
        AZ_Warning("MaglevControlPanel", result != -1, "import sys failed");
        result = PyRun_SimpleString(QString("sys.path.append('%1')").arg(lmbr_aws_path).toUtf8().data());
        AZ_Warning("MaglevControlPanel", result != -1, "path append lmbr_aws failed");
        result = PyRun_SimpleString(QString("sys.path.append('%1/Lib')").arg(pythonPath).toUtf8().data());
        AZ_Warning("MaglevControlPanel", result != -1, "path append lib failed");
        result = PyRun_SimpleString(QString("sys.path.append('%1/Lib/site-packages')").arg(pythonPath).toUtf8().data());
        AZ_Warning("MaglevControlPanel", result != -1, "path append site-packages");
        result = PyRun_SimpleString(QString("sys.path.append('%1/DLLs')").arg(pythonPath).toUtf8().data());
        AZ_Warning("MaglevControlPanel", result != -1, "path append DLLs");

        // Import the gui interface module
        py::object gui = py::module::import("gui");
        m_execute = gui.attr("execute");

        // Create a Python wrapper function for OnViewOutput
        auto func = std::function<void(PythonWorkerRequestId, const char*, py::object)>(
            [this](PythonWorkerRequestId requestId, const char* key, py::object value) {
                OnViewOutput(requestId, key, value); 
            }
        );

        auto call_policies = py::return_value_policy::automatic;
        UnlockPython();
        m_viewOutputFunction = py::cpp_function(func, call_policies);
        LockPython();
    }
    catch (py::error_already_set)
    {
        successful = false;
        m_logger->LogError("An error occurred when initializing the Python interpreter.");
        LogPythonException();
    }

    UnlockPython();

    return successful;
}

PythonWorkerRequestId PythonWorker::AllocateRequestId()
{
    if (m_nextRequestId == LAST_REQUEST_ID)
    {
        m_nextRequestId = FIRST_REQUEST_ID;
        return LAST_REQUEST_ID;
    }
    else
    {
        return m_nextRequestId++;
    }
}

void PythonWorker::ExecuteAsync(PythonWorkerRequestId requestId, const char* command,  const QVariantMap& args)
{

    AZ_Assert(requestId != DEFAULT_REQUEST_ID, "Request id not initialized using AllocateRequestId.");

    bool invokeResult = QMetaObject::invokeMethod(this, 
        "Execute", 
        Qt::QueuedConnection,
        Q_ARG(PythonWorkerRequestId, requestId),
        Q_ARG(QString, QString {command}), 
        Q_ARG(QVariantMap, args));
    AZ_Warning("MaglevControlPanel", invokeResult, "Invoke request for command %s failed", command);
}

void PythonWorker::Execute(PythonWorkerRequestId requestId, QString command, QVariantMap args)
{
    if (!m_threadState)
    {
        m_logger->LogWarning("Not executing command %s because Python is not initialized.", command.toStdString().c_str());
        return;
    }

    LockPython();

    try
    {
        py::dict pyArgs = QVariantMapToPyDict(args);

        pyArgs["root_directory"] = m_rootDirectoryPath;
        pyArgs["game_directory"] = m_gameDirectoryPath;
        pyArgs["user_directory"] = m_userDirectoryPath;
        pyArgs["view_output_function"] = m_viewOutputFunction;
        pyArgs["request_id"] = requestId;

        m_execute(command.toStdString().c_str(), pyArgs);
    }
    catch (py::error_already_set)
    {
        m_logger->LogError("An error occurred when executing command %s.", command.toUtf8().data());
        LogPythonException();
    }

    UnlockPython();
}

void PythonWorker::OnViewOutput(PythonWorkerRequestId requestId, const char* key, py::object value)
{

    QString keyParam{ key };
    QVariant valueParam{ PyObjectToQVariant(value) };

    bool wasHandled = false;
    auto pythonWorkerEventsInterface = AZ::Interface<PythonWorkerEventsInterface>::Get();
    if (pythonWorkerEventsInterface)
    {
        wasHandled = pythonWorkerEventsInterface->OnPythonWorkerOutput(requestId, keyParam, valueParam);
    }

    if (!wasHandled)
    {
        Output(requestId, keyParam, valueParam);
    }

}

py::dict PythonWorker::QVariantMapToPyDict(const QVariantMap& qMap)
{
    py::dict pyDict;

    for (auto it = qMap.constBegin(); it != qMap.constEnd(); ++it)
    {
        pyDict[it.key().toStdString().c_str()] = QVariantToPyObject(it.value(), it.key());
    }

    return pyDict;
}

py::list PythonWorker::QVariantListToPyList(const QVariantList& qList)
{
    py::list pyList;

    for (auto it = qList.constBegin(); it != qList.constEnd(); ++it)
    {
        pyList.append(QVariantToPyObject(*it));
    }

    return pyList;
}

py::object PythonWorker::QVariantToPyObject(const QVariant& variant, const QString& name)
{
    switch (variant.type())
    {
    case QVariant::Type::Map:
        return QVariantMapToPyDict(variant.toMap());

    case QVariant::Type::List:
        return QVariantListToPyList(variant.toList());

    case QVariant::Type::String:
        return py::cast(variant.toString().toStdString().c_str());

    case QVariant::Type::Bool:
        return py::cast(variant.toBool());

    case QVariant::Type::Int:
        return py::cast(variant.toInt());

    default:
        m_logger->LogError("Unsupported type %s on property %s in QVariantToPyObject", variant.typeName(), name.toStdString().c_str());
        return py::object {};
    }
}

QVariant PythonWorker::PyObjectToQVariant(const py::object& object, const QString& name)
{
    QString className = ExtractQString(object.attr("__class__").attr("__name__"));

    if (className == "dict" || className == "OrderedDict")
    {
        return PyDictToQVariantMap(object.cast<py::dict>());
    }
    else if (className == "list")
    {
        return PyListToQVariantList(object.cast<py::list>());
    }
    else if (className == "str")
    {
        return QVariant {
                   ExtractQString(object)
        };
    }
    else if (className == "unicode")
    {
        return QVariant {
                   ExtractQString(object)
        };
    }
    else if (className == "bool")
    {
        return QVariant {
                   (object.cast<bool>())
        };
    }
    else if (className == "int")
    {
        return QVariant {
                   (object.cast<int>())
        };
    }
    else if (className == "datetime")
    {
        auto str = ExtractQString(object.attr("isoformat")());
        auto dt = QDateTime::fromString(str, Qt::ISODate);
        dt.setTimeSpec(Qt::UTC);
        return QVariant {
                   dt
        };
    }
    else if (className == "NoneType")
    {
        return QVariant {};
    }
    else
    {
        m_logger->LogError(QString {"Unsupported type %1 on property %2 in PyObjectToQVariant."}.arg(className).arg(name));
        return QVariant {};
    }
}

QVariantList PythonWorker::PyListToQVariantList(const py::list& pyList)
{
    QVariantList qList;

    auto count = py::len(pyList);
    for (auto i = 0; i < count; ++i)
    {
        qList.append(PyObjectToQVariant(pyList[i]));
    }

    return qList;
}

QVariantMap PythonWorker::PyDictToQVariantMap(const py::dict& pyDict)
{
    QVariantMap qMap;
    for (auto itr = pyDict.begin(); itr != pyDict.end(); ++itr)
    {
        QString key = ExtractQString(py::cast<py::object>(itr->first));
        qMap[key] = PyObjectToQVariant(py::cast<py::object>(itr->second), key);
    }

    return qMap;
}

QString PythonWorker::ExtractQString(const py::object& object)
{
    std::string s = py::str(object).cast<std::string>();
    return QString {
               s.c_str()
    };
}

void PythonWorker::LogPythonException()
{
    try
    {
        PyObject* exc, * val, * tb;
        PyErr_Fetch(&exc, &val, &tb);
        PyErr_NormalizeException(&exc, &val, &tb);

        py::handle hexc(exc), hval(val), htb(tb);
        if (!hval)
        {
            std::string msg = py::str(hexc).cast<std::string>();
            m_logger->LogError(msg.c_str());
        }
        else
        {
            py::object traceback(py::module::import("traceback"));
            py::object format_exception(traceback.attr("format_exception"));
            py::list formatted_list(format_exception(hexc, hval, htb));
            for (int count = 0; count < len(formatted_list); ++count)
            {
                std::string msg  = formatted_list[count].cast<std::string>();
                m_logger->LogError(msg.c_str());
            }
        }
    }
    catch (py::error_already_set)
    {
        m_logger->LogError("Could not log python exception.");
    }
}

QString toDebugString(const QVariantMap& map)
{
    QString result;

    result += "{";
    bool first = true;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
    {
        if (!first)
        {
            result += ",";
        }
        first = false;
        result += " ";
        result += it.key();
        result += ": ";
        result += toDebugString(it.value());
    }
    result += " }";

    return result;
}

QString toDebugString(const QVariantList& list)
{
    QString result;

    result += "[";
    bool first = true;
    for (auto it = list.constBegin(); it != list.constEnd(); ++it)
    {
        if (!first)
        {
            result += ",";
        }
        first = false;
        result += " ";
        result += toDebugString(*it);
    }
    result += " ]";

    return result;
}

QString toDebugString(const QVariant& variant)
{
    switch (variant.type())
    {
    case QVariant::Type::Map:
        return toDebugString(variant.value<QVariantMap>());
    case QVariant::Type::List:
        return toDebugString(variant.value<QVariantList>());
    case QVariant::Type::Bool:
        return variant.value<bool>() ? "true" : "false";
    case QVariant::Type::String:
        return QString {
                   "\"%1\""
        }.arg(variant.value<QString>());
    default:
        return QString {
                   "(unsupported type %1)"
        }.arg(variant.typeName());
    }
}
