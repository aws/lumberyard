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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Statoscope
{
	class FileLogProcessor
	{
		FileSessionInfo SessionInfo;

		public FileLogProcessor(string logFilename) : this(new FileSessionInfo(logFilename))
		{
		}

		public FileLogProcessor(FileSessionInfo sessionInfo)
		{
			SessionInfo = sessionInfo;
		}

		public LogData ProcessLog()
		{
			string logFilename = SessionInfo.LocalLogFile;
			Console.WriteLine("Reading log file {0}", logFilename);

			LogData logData = new LogData(SessionInfo);

			if (!logFilename.EndsWith(".log"))
			{
				using (FileLogBinaryDataStream logDataStream = new FileLogBinaryDataStream(logFilename))
				{
					BinaryLogParser logParser = new BinaryLogParser(logDataStream, logData, logFilename);
					logParser.ProcessDataStream();
				}
			}
			else
			{
				using (FileLogDataStream logDataStream = new FileLogDataStream(logFilename))
				{
					LogParser logParser = new LogParser(logDataStream, logData);
					logParser.ProcessDataStream();
				}
			}

			logData.ProcessRecords();

			return logData;
		}
	}
}
