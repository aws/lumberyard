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
#ifndef UNITTEST_H
#define UNITTEST_H

#if defined(UNIT_TEST)

#include <QList>
#include <QOBject>


class UnitTestRun;
/** This Class is responsible for running the Unit Tests for the Asset Processors
 */
class UnitTestWorker
    : public QObject
{
    Q_OBJECT
public:
    explicit UnitTestWorker(QObject* parent = 0);
    virtual ~UnitTestWorker();
Q_SIGNALS:
    void UnitTestCompleted(int);


public Q_SLOTS:

    void Process();
    void RunTests();

private:
    QList<UnitTestRun*> m_Tests;
    void RegisterTest(UnitTestRun* test);
};

#endif // #if defined(UNIT_TEST)

#endif
