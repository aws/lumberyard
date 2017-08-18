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
    PythonWorkerRequests::Bus::Handler::BusConnect();
}

PythonWorker::~PythonWorker()
{
    PythonWorkerRequests::Bus::Handler::BusDisconnect();
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
    if (CreateInterpreter())
    {
        if (!InitializeInterpreter())
        {
            DestroyInterpreter();
        }
    }
}

bool PythonWorker::CreateInterpreter()
{
    if (!Py_IsInitialized())
    {
        m_logger->LogError("Disabled because Python is not initialized.");
        return false;
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

void PythonWorker::IoRedirect::Write(std::string const& str)
{
    if (str != "\n")
    {
        qDebug() << "PythonWorker:" << str.c_str();
    }
}

bool PythonWorker::InitializeInterpreter()
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

        py::object main = py::import("__main__");
        py::object globals = main.attr("__dict__");

        // Setup sys.path
        PyRun_SimpleString("import sys");
        PyRun_SimpleString(QString("sys.path.append('%1')").arg(lmbr_aws_path).toStdString().c_str());

        // Redirect output

        if (!m_ioRedirectDef)
        {
            // Only do this once even if Reset is called. Otherwise an error
            // will occur because the type converter is already registered.
            m_ioRedirectDef.reset(new py::class_<IoRedirect>("IoRedirect", py::init<>()));
            m_ioRedirectDef->def("write", &IoRedirect::Write);
        }

        globals["IoRedirect"] = *m_ioRedirectDef;
        py::import("sys").attr("stderr") = m_ioRedirect;
        py::import("sys").attr("stdout") = m_ioRedirect;

        // Import the gui interface module

        py::object gui = py::import("gui");
        m_execute = gui.attr("execute");

        // Create a Python wrapper function for OnViewOutput

        auto func = std::function<void(PythonWorkerRequestId, const char*, py::object)>(
            [this](PythonWorkerRequestId requestId, const char* key, py::object value) {
                OnViewOutput(requestId, key, value); 
            }
        );
        auto call_policies = py::default_call_policies();
        typedef boost::mpl::vector<void, int, const char*, py::object> func_sig;
        m_viewOutputFunction = py::make_function(func, call_policies, func_sig());
    }
    catch (py::error_already_set)
    {
        successful = false;
        m_logger->LogError("An error occured when initializing the Python interpreter.");
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

    QMetaObject::invokeMethod(this, 
        "Execute", 
        Qt::QueuedConnection,
        Q_ARG(PythonWorkerRequestId, requestId),
        Q_ARG(QString, QString {command}), 
        Q_ARG(QVariantMap, args));
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
        m_logger->LogError("An error occured when executing command %s.", command.toStdString().c_str());
        LogPythonException();
    }

    UnlockPython();
}

void PythonWorker::OnViewOutput(PythonWorkerRequestId requestId, const char* key, py::object value)
{

    QString keyParam{ key };
    QVariant valueParam{ PyObjectToQVariant(value) };

    bool wasHandled = false;
    EBUS_EVENT_RESULT(wasHandled, PythonWorkerEvents::Bus, OnPythonWorkerOutput, requestId, keyParam, valueParam);

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
        return py::object {
                   variant.toString().toStdString().c_str()
        };

    case QVariant::Type::Bool:
        return py::object {
                   variant.toBool()
        };

    case QVariant::Type::Int:
        return py::object {
                   variant.toInt()
        };

    default:
        m_logger->LogError("Unsupported type %s on property %s in QVariantToPyObject", variant.typeName(), name.toStdString().c_str());
        return py::object {};
    }
}

QVariant PythonWorker::PyObjectToQVariant(const py::object& object, const QString& name)
{
    QString className = ExtractQString(object.attr("__class__").attr("__name__"));

    if (className == "dict")
    {
        return PyDictToQVariantMap(py::extract<py::dict>(object));
    }
    else if (className == "list")
    {
        return PyListToQVariantList(py::extract<py::list>(object));
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
                   ((bool)py::extract<bool>(object))
        };
    }
    else if (className == "int")
    {
        return QVariant {
                   ((int)py::extract<int>(object))
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

    auto keys = pyDict.keys();
    auto count = py::len(keys);
    for (auto i = 0; i < count; ++i)
    {
        QString key = ExtractQString(keys[i]);
        qMap[key] = PyObjectToQVariant(pyDict[keys[i]], key);
    }

    return qMap;
}

QString PythonWorker::ExtractQString(const py::object& object)
{
    std::string s = py::extract<std::string>(py::str(object).encode("utf-8"));
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

        py::handle<> hexc(exc), hval(py::allow_null(val)), htb(py::allow_null(tb));
        if (!hval)
        {
            std::string msg = py::extract<std::string>(py::str(hexc));
            m_logger->LogError(msg.c_str());
        }
        else
        {
            py::object traceback(py::import("traceback"));
            py::object format_exception(traceback.attr("format_exception"));
            py::list formatted_list(format_exception(hexc, hval, htb));
            for (int count = 0; count < len(formatted_list); ++count)
            {
                std::string msg  = py::extract<std::string>(formatted_list[count].slice(0, -1));
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
