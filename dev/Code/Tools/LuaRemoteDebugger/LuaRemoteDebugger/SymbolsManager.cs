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
}
