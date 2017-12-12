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
using System.IO;

namespace Statoscope
{
	class FileLogDataStream : ILogDataStream
	{
    int m_lineNumber; // For debugging
		StreamReader m_streamReader;

		public FileLogDataStream(string fileName)
		{
			m_streamReader = new StreamReader(fileName);
      m_lineNumber = 0;
		}

		public void Dispose()
		{
			m_streamReader.Dispose();
			GC.SuppressFinalize(this);
		}

		public int Peek()
		{
			return m_streamReader.Peek();
		}

		public string ReadLine()
		{
      string line = m_streamReader.ReadLine();
      if (line != null)
      {
        ++m_lineNumber;
      }
			return line;
		}

		public bool IsEndOfStream
		{
			get
			{
				return m_streamReader.EndOfStream;
			}
		}
	}
}
