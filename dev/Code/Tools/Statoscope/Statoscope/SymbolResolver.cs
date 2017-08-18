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
using Dia2Lib;
using System.Runtime.InteropServices;

namespace Statoscope
{
	class SymbolResolver : IDisposable
	{
		IDiaSession m_session;
		UInt32 m_size;

		public SymbolResolver(IDiaSession session, UInt32 size)
		{
			m_session = session;
			m_size = size;
		}

		public bool AddressIsInRange(UInt32 address)
		{
			return (m_session.loadAddress <= address) && (address < m_session.loadAddress + m_size);
		}

		public string GetSymbolNameFromAddress(UInt32 address)
		{
			if (AddressIsInRange(address))
			{
				IDiaSymbol diaSymbol;
				m_session.findSymbolByVA(address, SymTagEnum.SymTagFunction, out diaSymbol);

				IDiaEnumLineNumbers lineNumbers;
				m_session.findLinesByVA(address, 4, out lineNumbers);

				IDiaLineNumber lineNum;
				uint celt;
				lineNumbers.Next(1, out lineNum, out celt);

				if (celt == 1)
				{
					string baseFilename = lineNum.sourceFile.fileName.Substring(lineNum.sourceFile.fileName.LastIndexOf('\\') + 1);
					return diaSymbol.name + " (" + baseFilename + ":" + lineNum.lineNumber + ")";
				}
				else
				{
					return diaSymbol.name;
				}
			}
			else
			{
				return "<address out of range>";
			}
		}

    #region IDisposable Members

    public void Dispose()
    {
			if (m_session != null)
			{
	      Marshal.ReleaseComObject(m_session);
				m_session = null;
			}
    }

    #endregion
  }
}
