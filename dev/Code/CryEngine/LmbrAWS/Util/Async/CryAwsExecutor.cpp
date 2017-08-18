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
#include "StdAfx.h"

#include "Util/Async/CryAwsExecutor.h"

#include <atomic>
#include <condition_variable>
#include <queue>
#include <mutex>

namespace LmbrAWS
{
    /*
    This turned out to be a lot more complicated than I expected, especially given that this is a temporary solution.

    CryTek's threading solution supports two main use cases:

    (1) Binding a task PERMANENTLY to a thread within a fixed set of threads and repeatedly calling OnUpdate on the task
    (2) Creating a new thread that executes the task

    The upshot is that neither of these options is appropriate to perform a one-shot AWS call:

    (2) is what we're trying to avoid in the first place, and (1) doesn't make sense since we're not recurrent

    So instead of the task representing an individual AWS call, we end up making a task that permanently processes submitted AWS requests

    */
    class AwsRequestProcessorTask
        : public IThreadTask
    {
    public:

        using ProcessRequestFunctor = std::function<void()>;
        using FunctorList = std::queue< ProcessRequestFunctor >;

        AwsRequestProcessorTask()
            : m_tasks()
            , m_tasksMutex()
            , m_tasksSignal()
            , m_taskInfo()
            , m_stopped(false)
        {}

        virtual void OnUpdate() override
        {
            bool done = false;

            while (!done)
            {
                std::unique_lock<std::mutex> tasksLock(m_tasksMutex);
                m_tasksSignal.wait(tasksLock, [&](){ return m_stopped.load() == true || m_tasks.size() > 0; });

                // drain the queue within the lock
                FunctorList tasksToExecute;
                while (!m_tasks.empty())
                {
                    tasksToExecute.push(std::move(m_tasks.front()));
                    m_tasks.pop();
                }

                done = m_stopped.load() && m_tasks.size() == 0;
                tasksLock.unlock();

                // now execute the long-running tasks outside of the lock
                while (!tasksToExecute.empty())
                {
                    tasksToExecute.front()();
                    tasksToExecute.pop();
                }
            }

            // When you unregister a task, the CryTek implementation will
            //   (1) Call RemoveTask on the thread; there is no guarantee the tasks will be done executing when this call returns
            //   (2) Call Stop on the task itself;
            //   (3) (Since the task is marked as blocking) Call Cancel on the task's dedicated thread
            //
            //   Step (3) means that the thread will not be terminated early while we're in the middle of something, so there's no need for us
            //    to add additional synchronization logic between Stop() and the OnUpdate() function that causes Stop to wait until OnUpdate exits
            //   It also means there may be a block on the main thread for some amount of time, but it is likely that this only occurs during complete engine shutdown
        }

        virtual void Stop() override
        {
            m_stopped = true;
            m_tasksSignal.notify_one();
        }

        virtual struct SThreadTaskInfo* GetTaskInfo() override { return &m_taskInfo; }

        void SubmitTask(ProcessRequestFunctor&& task)
        {
            // If we've been stopped, just execute synchronously
            if (m_stopped)
            {
                task();
                return;
            }

            {
                std::lock_guard<std::mutex> tasksLock(m_tasksMutex);
                m_tasks.push(std::move(task));
            }

            m_tasksSignal.notify_one();
        }

    private:

        FunctorList m_tasks;
        std::mutex m_tasksMutex;
        std::condition_variable m_tasksSignal;

        SThreadTaskInfo m_taskInfo;

        std::atomic<bool> m_stopped;
    };

    static const char* SUBJECT_TAG = "AwsCalls";

    CryAwsExecutor::CryAwsExecutor()
        : m_processingTask(nullptr)
    {
        IThreadTaskManager* pThreadTaskManager = gEnv->pSystem->GetIThreadTaskManager();
        assert(pThreadTaskManager);

        m_processingTask = std::make_unique<AwsRequestProcessorTask>();
        auto* taskInfo = m_processingTask->GetTaskInfo();
        taskInfo->m_params.name = SUBJECT_TAG;
        taskInfo->m_params.nFlags = THREAD_TASK_BLOCKING;
        taskInfo->m_pThread = NULL;

        pThreadTaskManager->RegisterTask(m_processingTask.get(), m_processingTask->GetTaskInfo()->m_params);
    }

    CryAwsExecutor::~CryAwsExecutor()
    {
        IThreadTaskManager* pThreadTaskManager = gEnv->pSystem->GetIThreadTaskManager();
        assert(pThreadTaskManager);

        pThreadTaskManager->UnregisterTask(m_processingTask.get());
    }

    bool CryAwsExecutor::SubmitToThread(std::function<void()>&& task)
    {
        m_processingTask->SubmitTask(std::move(task));

        return true;
    }
} // namespace LmbrAWS