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
using System.ServiceModel;

namespace Statoscope
{
	class WCF
	{
		[ServiceContract]
		interface IStatoscopeRPC
		{
			[OperationContract]
			void LoadLog(string logURI);
		}

		[ServiceBehavior(InstanceContextMode = InstanceContextMode.Single)]
		class CStatoscopeRPC : IStatoscopeRPC
		{
			StatoscopeForm m_ssForm;

			public CStatoscopeRPC(StatoscopeForm ssForm)
			{
				m_ssForm = ssForm;
			}

			public void LoadLog(string logURI)
			{
				System.Console.WriteLine("Received RPC to load: " + logURI);
				m_ssForm.OpenLog(logURI);
			}
		}

		static ServiceHost s_serviceHost;
		const string PipeAddress = "net.pipe://localhost/Statoscope";
		const string PipeServiceName = "StatoscopeRPC";
		const string PipeEndpointAddress = PipeAddress + "/" + PipeServiceName;

		public static void CreateServiceHost(StatoscopeForm ssForm)
		{
			try
			{
				s_serviceHost = new ServiceHost(new CStatoscopeRPC(ssForm), new Uri[] { new Uri(PipeAddress) });
				s_serviceHost.AddServiceEndpoint(typeof(IStatoscopeRPC), new NetNamedPipeBinding(), PipeServiceName);
				s_serviceHost.Open();
			}
			catch (AddressAlreadyInUseException)
			{
			}
		}

		public static bool CheckForExistingToolProcess(string logURI)
		{
			try
			{
				ChannelFactory<IStatoscopeRPC> ssRPCFactory = new ChannelFactory<IStatoscopeRPC>(new NetNamedPipeBinding(), new EndpointAddress(PipeEndpointAddress));
				IStatoscopeRPC ssRPCProxy = ssRPCFactory.CreateChannel();
				ssRPCProxy.LoadLog(logURI);
			}
			catch (EndpointNotFoundException)
			{
				return false;
			}

			return true;
		}
	}
}
