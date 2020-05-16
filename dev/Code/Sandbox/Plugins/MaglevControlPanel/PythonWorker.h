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

#include <CloudCanvasPythonWorkerInterface.h>

#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QVariant>
#include <QThread>
#include <QSharedPointer>
#include <QScopedPointer>
#include <Include/PythonWorkerBus.h>
// MSVC warns that these typedef members are being used and therefore must be acknowledged by using the following macro
#pragma push_macro("_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
#ifndef _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#endif

// strdup is undefined in Code/CryEngine/CryCommon/platform.h, but it's required by PyBind11
// Restore the function for the PythonWorker
#pragma push_macro("strdup")
#undef strdup
#define strdup _strdup
// Qt defines slots, which interferes with the use here.
#pragma push_macro("slots")
#undef slots
#include <Python.h>
#include <pybind11/pybind11.h>
#pragma pop_macro("slots")
#pragma pop_macro("strdup")

#pragma pop_macro("_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")

class CloudCanvasLogger;

namespace py = PYBIND11_NAMESPACE;

class PythonWorker
    : public QObject
    , protected PythonWorkerRequestsInterface
{
    Q_OBJECT

public:

    explicit PythonWorker(
        QSharedPointer<CloudCanvasLogger> logger,
        const char* rootDirectoryPath,
        const char* gameDirectoryPath,
        const char* userDirectoryPath);
    ~PythonWorker();

    void Start();
    bool IsStarted() override { return m_isStarted; }

    PythonWorkerRequestId AllocateRequestId() override;

    void ExecuteAsync(PythonWorkerRequestId requestId, const char* command, const QVariantMap& args = QVariantMap{}) override;

    // End the current interpreter and create a new one.
    //
    // WARNING: If there are any active threads ending the current interpreter
    // will fail and the IDE will crash. This method is only intended to be
    // used for debugging.
    //
    // This is useful for reloading all the Python code when running the IDE
    // in the debugger. Set a breakpoint you can trigger at will and then
    // use the immediate window to execute this method.
    void ResetAsync();

signals:
    void Output(PythonWorkerRequestId requestId, const QString& key, const QVariant& value);

private slots:
    void Execute(PythonWorkerRequestId requestId, QString command, QVariantMap args);
    void Reset();

private:

    QThread m_thread;
    bool m_isStarted = false;

    QString m_rootDirectoryPathQ;
    py::str m_rootDirectoryPath;
    py::str m_gameDirectoryPath;
    py::str m_userDirectoryPath;

    QSharedPointer<CloudCanvasLogger> m_logger;

    PyThreadState* m_threadState {
        nullptr
    };

    py::object m_isinstance;
    py::object m_basestring;
    py::object m_dict;
    py::object m_list;
    py::object m_int;

    py::object m_execute;
    py::object m_viewOutputFunction;

    void LogPythonException();

    QString MakeRootPath(const char* relativePath, bool& ok);

    void LockPython();
    void UnlockPython();

    bool CreateInterpreter(const QString& pythonPath);
    bool InitializeInterpreter(const QString& pythonPath);
    void DestroyInterpreter();

    py::dict QVariantMapToPyDict(const QVariantMap& map);
    py::list QVariantListToPyList(const QVariantList& list);
    py::object QVariantToPyObject(const QVariant& variant, const QString& name = "(none)");

    QVariant PyObjectToQVariant(const py::object& object, const QString& key = "(none)");
    QVariantList PyListToQVariantList(const py::list& list);
    QVariantMap PyDictToQVariantMap(const py::dict& dict);

    void OnViewOutput(PythonWorkerRequestId requestId, const char* key, py::object value);

    QString ExtractQString(const py::object& object);

    const PythonWorkerRequestId DEFAULT_REQUEST_ID  = 0;
    const PythonWorkerRequestId FIRST_REQUEST_ID    = 1;
    const PythonWorkerRequestId LAST_REQUEST_ID     = INT_MAX;
    PythonWorkerRequestId m_nextRequestId{ FIRST_REQUEST_ID };

};

QString toDebugString(const QVariantMap& map);
QString toDebugString(const QVariantList& list);
QString toDebugString(const QVariant& variant);


