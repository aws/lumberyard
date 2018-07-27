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
#ifndef UNITTESTRUNNER_H
#define UNITTESTRUNNER_H

#include "native/project.h"
#include <QObject>
#include <QMetaObject>
#include <QString>
#include <QDebug>
#include <AzCore/Debug/TraceMessageBus.h>
#include <QDir>
#include <QTemporaryDir>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>

//! These macros can be used for checking your unit tests,
//! you can check AssetScannerUnitTest.cpp for usage
#define UNIT_TEST_EXPECT_TRUE(condition) \
    UNIT_TEST_CHECK((condition))

#define UNIT_TEST_EXPECT_FALSE(condition) \
    UNIT_TEST_CHECK(!(condition))

#define UNIT_TEST_CHECK(condition)                                                                          \
    if (!(condition))                                                                                       \
    {                                                                                                       \
        QString failMessage = QString("%1(%2): ---- FAIL: %3").arg(__FILE__).arg(__LINE__).arg(#condition); \
        Q_EMIT UnitTestFailed(failMessage);                                                                 \
        return;                                                                                             \
    }

//! This is the base class.  Derive from this class, implement StartTest
//! and emit UnitTestPassed when done or UnitTestFailed when you fail
//! You must emit either one or the other for the next unit test to start.
class UnitTestRun
    : public QObject
{
    Q_OBJECT
public:
    virtual ~UnitTestRun() {}
    ///implement all your unit tests in this function
    virtual void StartTest() = 0;
    ///Unit tests having higher priority will run first,
    ///negative value means higher priority,deafult priority is zero
    virtual int UnitTestPriority() const;
    const char* GetName() const;
    void SetName(const char* name);

Q_SIGNALS:
    void UnitTestFailed(QString message);
    void UnitTestPassed();

private:
    const char* m_name = nullptr; // expected to be static memory!
};

// after deriving from this class, place the following somewhere
// REGISTER_UNIT_TEST(YourClassName)

// ----------------- UTILITY FUNCTIONS --------------------
namespace UnitTestUtils
{
    //! Create a dummy file, with optional contents.  Will create directories for it too.
    bool CreateDummyFile(const QString& fullPathToFile, QString contents = "");
    //! This function pumps the Qt event queue until either the varToWatch becomes true or the specified millisecond elapse.
    bool BlockUntil(bool& varToWatch, int millisecondsMax);

    // the Assert Absorber here is used to absorb asserts and errors during unit tests.
    // it only absorbs asserts spawned by this thread;
    class AssertAbsorber : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        AssertAbsorber()
        {
            // only absorb asserts when this object is on scope in the thread that this object is on scope in.
            BusConnect();
        }

        bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override
        {
            ++m_numWarningsAbsorbed;
            return true;
        }

        bool OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override
        {
            ++m_numAssertsAbsorbed;
            return true; // I handled this, do not forward it
        }

        bool OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/)
        {
            ++m_numErrorsAbsorbed;
            return true; // I handled this, do not forward it
        }

        bool OnPrintf(const char* /*window*/, const char* /*message*/) override
        {
            ++m_numMessagesAbsorbed;
            return true;
        }

        ~AssertAbsorber()
        {
        }

        void Clear()
        {
            m_numMessagesAbsorbed = 0;
            m_numWarningsAbsorbed = 0;
            m_numAssertsAbsorbed = 0;
            m_numErrorsAbsorbed = 0;
        }

        int m_numMessagesAbsorbed = 0;
        int m_numWarningsAbsorbed = 0;
        int m_numAssertsAbsorbed = 0;
        int m_numErrorsAbsorbed = 0;
    };

    //! Automatically restore current directory when this leaves scope:
    class ScopedDir
    {
    public:
        ScopedDir() = default;

        ScopedDir(QString newDir)
        {
            Setup(newDir);
        }

        void Setup(QString newDir)
        {
            m_originalDir = QDir::currentPath();
            newDir = QDir::cleanPath(newDir);
            QDir::setCurrent(newDir);

            m_localFileIO = aznew AZ::IO::LocalFileIO();

            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            if (m_priorFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
            }

            AZ::IO::FileIOBase::SetInstance(m_localFileIO);

            m_localFileIO->SetAlias("@assets@", (newDir + QString("/ALIAS/assets")).toUtf8().constData());
            m_localFileIO->SetAlias("@log@", (newDir + QString("/ALIAS/logs")).toUtf8().constData());
            m_localFileIO->SetAlias("@cache@", (newDir + QString("/ALIAS/cache")).toUtf8().constData());
            m_localFileIO->SetAlias("@user@", (newDir + QString("/ALIAS/user")).toUtf8().constData());
            m_localFileIO->SetAlias("@root@", (newDir + QString("/ALIAS/root")).toUtf8().constData());
        }

        ~ScopedDir()
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            m_localFileIO = nullptr;
            if (m_priorFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            }
            QDir::setCurrent(m_originalDir);
        }

    private:
        QString m_originalDir;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;
    };
}



// ----------------------------------- IMPLEMENTATION DETAILS ------------------------
class UnitTestRegistry
{
public:
    explicit UnitTestRegistry(const char* name);
    virtual ~UnitTestRegistry() {}
    static UnitTestRegistry* first() { return s_first; }
    UnitTestRegistry* next() const { return m_next; }
    virtual UnitTestRun* create() = 0;
protected:
    // it forms a static linked-list using the following internal:
    static UnitTestRegistry* s_first;
    UnitTestRegistry* m_next;
    const char* m_name = nullptr; // expected to be static memory
};

template<typename T>
class UnitTestRegistryEntry
    : public UnitTestRegistry
{
public:
    UnitTestRegistryEntry(const char* name)
        : UnitTestRegistry(name) {}
    virtual UnitTestRun* create()
    {
        static_assert(std::is_base_of<UnitTestRun, T>::value, "You must derive from UnitTestRun if you want to use REGISTER_UNIT_TEST");
        T* created = new T();
        created->SetName(m_name);
        return created;
    }
};

/// Derive from UnitTestRun, then put the following macro in your cpp file:
#define REGISTER_UNIT_TEST(classType)                                          \
    UnitTestRegistryEntry<classType> g_autoRegUnitTest##classType(#classType); \
    EXPORT_STATIC_LINK_VARIABLE(g_autoRegUnitTest##classType)                  \

#endif // UNITTESTRUNNER_H
