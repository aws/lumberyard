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
 * @file ConsoleLogSystem.hpp
 * @brief
 *
 */

#pragma once

#include "util/logging/FormattedLogSystem.hpp"
#include "util/Core_EXPORTS.hpp"

namespace awsiotsdk
{
    namespace util
    {
        namespace Logging
        {
            /**
             * Log system interface that logs to std::cout
             */
            class AWS_API_EXPORT ConsoleLogSystem : public FormattedLogSystem
            {
            public:

                using Base = FormattedLogSystem;

                ConsoleLogSystem(LogLevel logLevel) :
                    Base(logLevel)
                {}

                virtual ~ConsoleLogSystem() {}

            protected:

                virtual void ProcessFormattedStatement(util::String&& statement) override;
            };

        } // namespace Logging
    } // namespace util
} // namespace awsiotsdk
