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

#include <AzTest/AzTest.h>

#include <AzToolsFramework/Undo/UndoSystem.h>

using namespace AZ;
using namespace AzToolsFramework;
using namespace AzToolsFramework::UndoSystem;

namespace UnitTest
{
    class UndoDestructorTest : public URSequencePoint
    {
    public:
        UndoDestructorTest(bool* completedFlag)
            : URSequencePoint("UndoDestructorTest", 0)
            , m_completedFlag(completedFlag)
        {
            *m_completedFlag = false;
        }

        ~UndoDestructorTest()
        {
            *m_completedFlag = true;
        }

        bool Changed() const override { return true; }

    private:
        bool* m_completedFlag;
    };

    TEST(UndoStack, UndoRedoMemory)
    {
        UndoStack undoStack(nullptr);

        bool flag = false;
        undoStack.Post(aznew UndoDestructorTest(&flag));

        undoStack.Undo();
        undoStack.Slice();

        EXPECT_EQ(flag, true);

        //AZ_TEST_ASSERT(v0 == 5.0f);

        //EXPECT_EQ(1, 1);
    }

    class UndoIntSetter : public URSequencePoint
    {
    public:
        UndoIntSetter(int* value, int newValue)
            : URSequencePoint("UndoIntSetter", 0)
            , m_value(value)
            , m_newValue(newValue)
            , m_oldValue(*value)
        {
            Redo();
        }

        void Undo() override
        {
            *m_value = m_oldValue;
        }

        void Redo() override
        {
            *m_value = m_newValue;
        }

        bool Changed() const override { return true; }

    private:
        int* m_value;
        int m_newValue;
        int m_oldValue;
    };

    TEST(UndoStack, UndoRedoSequence)
    {
        UndoStack undoStack(nullptr);

        int tracker = 0;

        undoStack.Post(aznew UndoIntSetter(&tracker, 1));
        EXPECT_EQ(tracker, 1);

        undoStack.Undo();
        EXPECT_EQ(tracker, 0);

        undoStack.Redo();
        EXPECT_EQ(tracker, 1);

        undoStack.Undo();
        EXPECT_EQ(tracker, 0);

        undoStack.Redo();
        EXPECT_EQ(tracker, 1);

        undoStack.Post(aznew UndoIntSetter(&tracker, 100));
        EXPECT_EQ(tracker, 100);

        undoStack.Undo();
        EXPECT_EQ(tracker, 1);

        undoStack.Undo();
        EXPECT_EQ(tracker, 0);

        undoStack.Redo();
        EXPECT_EQ(tracker, 1);
    }

    TEST(UndoStack, UndoRedoLotsOfUndos)
    {
        UndoStack undoStack(nullptr);

        int tracker = 0;

        const int numUndos = 1000;
        for (int i = 0; i < 1000; i++)
        {
            undoStack.Post(aznew UndoIntSetter(&tracker, i + 1));
            EXPECT_EQ(tracker, i + 1);
        }

        int counter = 0;
        while (undoStack.CanUndo())
        {
            undoStack.Undo();
            counter++;
        }

        EXPECT_EQ(numUndos, counter);
        EXPECT_EQ(tracker, 0);

        counter = 0;
        while (undoStack.CanRedo())
        {
            undoStack.Redo();
            counter++;
        }

        EXPECT_EQ(numUndos, counter);
        EXPECT_EQ(tracker, numUndos);
    }
}