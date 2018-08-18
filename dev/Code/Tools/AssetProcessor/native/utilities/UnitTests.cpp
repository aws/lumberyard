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
#if defined(UNIT_TEST)
#include "native/unittests/UnitTestRunner.h"
#include "native/utilities/UnitTests.h"
#include "native/assetprocessor.h"
#include <AzCore/Debug/Trace.h>
#include <AzCore/base.h>
#include <QMetaObject>
#include <QtAlgorithms>
#include <QCoreApplication>

UnitTestWorker::UnitTestWorker(QObject* parent)
    : QObject(parent)
{
}

UnitTestWorker::~UnitTestWorker()
{
}



void UnitTestWorker::Process()
{
    UnitTestRegistry* currentTest = UnitTestRegistry::first();
    while (currentTest)
    {
        RegisterTest(currentTest->create());
        currentTest = currentTest->next();
    }

    //Sort the List based on priority
    qSort(m_Tests.begin(), m_Tests.end(),
        [](const UnitTestRun* first, const UnitTestRun* second) -> bool
        {
            return first->UnitTestPriority() < second->UnitTestPriority();
        }
        );
    RunTests();
}


void UnitTestWorker::RegisterTest(UnitTestRun* test)
{
    m_Tests.push_back(test);
}


void UnitTestWorker::RunTests()
{
    if (m_Tests.empty())
    {
        Q_EMIT UnitTestCompleted(0);
        return;
    }

    UnitTestRun* front = m_Tests.front();
    m_Tests.pop_front();

    QString frontName(front->GetName());

    // did we specify a specific sub-set of unit tests?  using /unittest alone means do all tests
    // otherwise you can specify a list like
    // /unittest=MyUnitTest,OtherUnitTest
    QStringList args = QCoreApplication::arguments();
    bool foundIt = true;
    for (QString arg : args)
    {
        if (arg.contains("/unittest=", Qt::CaseInsensitive))
        {
            foundIt = false;
            QString rawTestString = arg.split("=")[1];
            QStringList tests = rawTestString.split(",");

            for (QString test : tests)
            {
                if (frontName.contains(test, Qt::CaseInsensitive))
                {
                    foundIt = true;
                    break;
                }
            }
        }
    }
    if (!foundIt)
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "[SKIP]  %s ! \n", frontName.toUtf8().data());
        QMetaObject::invokeMethod(this, "RunTests", Qt::QueuedConnection);
        front->deleteLater();
        return;
    }

    connect(front, &UnitTestRun::UnitTestPassed, this, [front, this]()
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "[OK]  %s ... \n", front->GetName());
            QMetaObject::invokeMethod(this, "RunTests", Qt::QueuedConnection);
            front->deleteLater();
        });
    connect(front, &UnitTestRun::UnitTestFailed, this, [front, this](QString message)
        {
            // note, print the line and file on the next line so that VS recognizes it as clickable
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "[FAIL]  %s:\n%s\n", front->GetName(), message.toUtf8().data());
            front->deleteLater();
            Q_EMIT UnitTestCompleted(1);
        });

    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unit Test starting: %s ...\n", front->GetName());
    front->StartTest();
}

#include <native/utilities/UnitTests.moc>

#endif









