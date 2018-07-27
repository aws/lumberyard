/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file ActionThread.hpp
 * @brief
 *
 */

#pragma once

#include <functional>
#include <thread>
#include <atomic>

#include "util/Core_EXPORTS.hpp"

#include "NetworkConnection.hpp"
#include "Action.hpp"

namespace awsiotsdk {
	namespace util {
		namespace Threading {
			enum class DestructorAction {
				JOIN,
				DETACH
			};

			class AWS_API_EXPORT ThreadTask {
			public:
				ThreadTask(DestructorAction destructor_action, std::shared_ptr<std::atomic_bool> sync_point,
						   util::String thread_descriptor);
				~ThreadTask();

				// Rule of 5 stuff.
				// Don't copy or move
				ThreadTask(const ThreadTask &) = delete;
				ThreadTask &operator=(const ThreadTask &) = delete;
				ThreadTask(ThreadTask &&) = delete;
				ThreadTask &operator=(ThreadTask &&) = delete;

				template<class Fn, class ... Args>
				void Run(Fn&& fn, Args&& ... args) {
					m_thread_ = std::thread(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...));
				}

				void Stop();
			private:
				DestructorAction destructor_action_;
				std::shared_ptr<std::atomic_bool> m_continue_;
				std::thread m_thread_;
				util::String thread_descriptor_;
			};
		}
	}
}
