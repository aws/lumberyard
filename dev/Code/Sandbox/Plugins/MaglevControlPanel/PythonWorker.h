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

#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QVariant>
#include <QThread>
#include <QSharedPointer>
#include <QScopedPointer>

#include <AzCore/EBus/EBus.h>

// MSVC warns that these typedef members are being used and therefore must be acknowledged by using the following macro
#pragma push_macro("_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
#ifndef _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#endif

// Disabling C++17 warning - warning C5033: 'register' is no longer a supported storage class which occurs in Python 2.7
// Suppress warning in pymath.h where a conflict with round exists with VS 12.0 math.h ::  pymath.h(22) : warning C4273: 'round' : inconsistent dll linkage
// Suppress warning in boost/python/opaque_pointer_converter.hpp, boost/python/return_opaque_pointer.hpp and python/detail/dealloc.hpp
// for non UTF-8 characters.
AZ_PUSH_DISABLE_WARNING(4068 4273 4828 5033, "-Wregister")
AZ_PUSH_DISABLE_WARNING(, "-Wunused-local-typedef")
AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
#include <boost/python.hpp>
AZ_POP_DISABLE_WARNING
AZ_POP_DISABLE_WARNING
AZ_POP_DISABLE_WARNING

#pragma pop_macro("_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")

class CloudCanvasLogger;

namespace py = boost::python;

using PythonWorkerRequestId = int;

class PythonWorkerEvents : public AZ::EBusTraits
{
public:
    using Bus = AZ::EBus<PythonWorkerEvents>;

    // if any OnOutput returns true, it means the command was handled and the worker won't process any further
    virtual bool OnPythonWorkerOutput(PythonWorkerRequestId requestId, const QString& key, const QVariant& value) = 0;
};

class PythonWorkerRequests : public AZ::EBusTraits
{
public:
    using Bus = AZ::EBus<PythonWorkerRequests>;

    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

    virtual PythonWorkerRequestId AllocateRequestId() = 0;
    virtual void ExecuteAsync(PythonWorkerRequestId requestId, const char* command, const QVariantMap& args = QVariantMap{}) = 0;
    virtual bool IsStarted() = 0;
};

class PythonWorker
    : public QObject
    , private PythonWorkerRequests::Bus::Handler
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

    class IoRedirect
    {
    public:

        void Write(std::string const& str);
    };

    QScopedPointer<py::class_<IoRedirect> > m_ioRedirectDef;

    IoRedirect m_ioRedirect {};

    void LogPythonException();

    QString MakeRootPath(const char* relativePath, bool& ok);

    void LockPython();
    void UnlockPython();

    bool CreateInterpreter();
    bool InitializeInterpreter();
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


