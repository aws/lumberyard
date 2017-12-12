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
#include "UnitTestRunner.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QTime>
#include <QCoreApplication>

UnitTestRegistry* UnitTestRegistry::s_first = nullptr;

UnitTestRegistry::UnitTestRegistry(const char* name)
    : m_name(name)
{
    m_next = s_first;
    s_first = this;
}

int UnitTestRun::UnitTestPriority() const
{
    return 0;
}

const char* UnitTestRun::GetName() const
{
    return m_name;
}
void UnitTestRun::SetName(const char* name)
{
    m_name = name;
}

namespace UnitTestUtils
{
    bool CreateDummyFile(const QString& fullPathToFile, QString contents)
    {
        QFileInfo fi(fullPathToFile);
        QDir fp(fi.path());
        fp.mkpath(".");
        QFile writer(fullPathToFile);
        if (!writer.open(QFile::WriteOnly))
        {
            return false;
        }

        {
            QTextStream ts(&writer);
            ts.setCodec("UTF-8");
            ts << contents;
        }
        return true;
    }

    bool BlockUntil(bool& varToWatch, int millisecondsMax)
    {
        QTime limit;
        limit.start();
        varToWatch = false;
        while ((!varToWatch) && (limit.elapsed() < millisecondsMax))
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }

        // and then once more, so that any queued events as a result of the above finish.
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

        return (varToWatch);
    }

}
#include <native/unittests/UnitTestRunner.moc>

