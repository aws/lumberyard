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

using Dia2Lib;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace LuaRemoteDebugger
{
	public abstract class SymbolsManager
	{
		Dictionary<UInt32, CallStackItem> symbolCache = new Dictionary<UInt32, CallStackItem>();

		public List<CallStackItem> GetSymbolNamesFromAddresses(List<UInt32> addresses)
		{
			// Get a list of addresses that need to be resolved
			List<UInt32> uncachedAddresses = addresses.FindAll(x => !symbolCache.ContainsKey(x));
			if (uncachedAddresses.Count > 0)
			{
				// Resolve uncached addresses
				List<CallStackItem> items = new List<CallStackItem>(GetSymbolNamesFromAddressesInternal(uncachedAddresses));
				Debug.Assert(items.Count == uncachedAddresses.Count);
				for (int i = 0; i < items.Count; ++i)
				{
					symbolCache[uncachedAddresses[i]] = items[i];
				}
			}
			// Return results
			List<CallStackItem> results = new List<CallStackItem>();
			foreach (UInt32 address in addresses)
			{
				results.Add(symbolCache[address]);
			}
			return results;
		}

		protected abstract List<CallStackItem> GetSymbolNamesFromAddressesInternal(List<UInt32> addresses);
	}

	public class XboxSymbolsManager : SymbolsManager // ACCEPTED_USE
	{
		private List<XboxSymbolResolver> symbolResolvers = new List<XboxSymbolResolver>(); // ACCEPTED_USE

		public void AddModuleInformation(string path, uint baseAddress, uint size, Guid guid, uint age)
		{
			try
			{
				IDiaSession diaSession;
				DiaSourceClass diaSource = new DiaSourceClass();
				diaSource.loadAndValidateDataFromPdb(path, ref guid, 0, age);
				diaSource.openSession(out diaSession);
				diaSession.loadAddress = baseAddress;

				XboxSymbolResolver symbolResolver = new XboxSymbolResolver(diaSession, size); // ACCEPTED_USE
				symbolResolvers.Add(symbolResolver);

				Marshal.ReleaseComObject(diaSource);
			}
			catch (System.Runtime.InteropServices.COMException ex)
			{
				switch ((UInt32)ex.ErrorCode)
				{
					// look in dia.h for error codes
					case 0x806D0005:
						Console.WriteLine("Couldn't find pdb: " + path);
						break;
					case 0x806D0006:
						Console.WriteLine("Signature invalid for: " + path);
						break;
					default:
						Console.WriteLine("Unknown pdb load error for: " + path + ", errorcode: " + ex.ErrorCode);
						break;
				}
			}
		}

		protected override List<CallStackItem> GetSymbolNamesFromAddressesInternal(List<UInt32> addresses)
		{
			List<CallStackItem> items = new List<CallStackItem>();
			foreach (UInt32 address in addresses)
			{
				bool found = false;
				foreach (XboxSymbolResolver symbolResolver in symbolResolvers) // ACCEPTED_USE
				{
					if (symbolResolver.AddressIsInRange(address))
					{
						found = true;
						items.Add(symbolResolver.GetSymbolNameFromAddress(address));
					}
				}
				if (!found)
				{
					CallStackItem result = new CallStackItem();
					result.Description = "<symbol 0x" + Convert.ToString(address, 16) + " not found>";
					items.Add(result);
				}
			}
			return items;
		}
	}

	public class XboxSymbolResolver : IDisposable // ACCEPTED_USE
	{
		IDiaSession session;
		UInt32 size;

		public XboxSymbolResolver(IDiaSession session, UInt32 size) // ACCEPTED_USE
		{
			this.session = session;
			this.size = size;
		}

		~XboxSymbolResolver() // ACCEPTED_USE
		{
			Dispose(false);
		}

		public bool AddressIsInRange(UInt32 address)
		{
			return (session.loadAddress <= address) && (address < session.loadAddress + size);
		}

		public CallStackItem GetSymbolNameFromAddress(UInt32 address)
		{
			CallStackItem result = new CallStackItem();
			if (AddressIsInRange(address))
			{
				IDiaSymbol diaSymbol;
				session.findSymbolByVA(address, SymTagEnum.SymTagFunction, out diaSymbol);

				result.Description = diaSymbol.name + "()";

				IDiaEnumLineNumbers lineNumbers;
				session.findLinesByVA(address, 4, out lineNumbers);

				IDiaLineNumber lineNum;
				uint celt;
				lineNumbers.Next(1, out lineNum, out celt);

				if (celt == 1)
				{
					string baseFilename = lineNum.sourceFile.fileName.Substring(lineNum.sourceFile.fileName.LastIndexOf('\\') + 1);
					result.Source = baseFilename;
					result.Line = (int)lineNum.lineNumber;
				}
			}
			else
			{
				result.Description = "<address out of range>";
			}
			return result;
		}

    #region IDisposable Members

    public void Dispose()
    {
			Dispose(true);
			GC.SuppressFinalize(this);
    }

		protected virtual void Dispose(bool disposing)
		{
			if (disposing)
			{
				// free the state of any contained objects
				// we don't contain any other objects!
			}
			// free my own state
			if (session != null)
			{
				Marshal.ReleaseComObject(session);
				session = null;
			}
		}

    #endregion
	}
}
