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

#include "LogReader.h"

namespace ScriptCanvas
{
    namespace Debugger
    {
        LogReader::LogReader()
        {

        }

        LogReader::~LogReader()
        {

        }

        void LogReader::Visit(ExecutionThreadEnd& loggableEvent)
        {

        }

        void LogReader::Visit(ExecutionThreadBeginning& loggableEvent)
        {

        }

        void LogReader::Visit(GraphActivation& loggableEvent)
        {

        }

        void LogReader::Visit(GraphDeactivation& loggableEvent)
        {

        }

        void LogReader::Visit(NodeStateChange& loggableEvent)
        {

        }

        void LogReader::Visit(InputSignal& loggableEvent)
        {

        }

        void LogReader::Visit(OutputSignal& loggableEvent)
        {

        }

        void LogReader::Visit(VariableChange& loggableEvent)
        {

        }
    }
}
