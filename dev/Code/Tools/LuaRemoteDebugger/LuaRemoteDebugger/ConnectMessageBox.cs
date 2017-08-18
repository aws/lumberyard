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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using LuaRemoteDebugger.Properties;
using System.Runtime.InteropServices;
using System.IO;

namespace LuaRemoteDebugger
{
	public partial class ConnectMessageBox : Form
	{
		private class TargetItem
		{
			public string Name { get; set; }
			public string IpAddress { get; set; }
			public int Port { get; set; }
			public TargetItem(string name, string ipAddress, int port) { Name = name; IpAddress = ipAddress; Port = port; }

			public override string ToString()
			{
				return Name;
			}
		}

		MainForm parent;
		bool suppressChangeEvents = false;	// Used to prevent feedback loop
		//tatic ConnectMessageBox currentConnectMessageBox; // shows the popup box for who we're connected to

		public string IpAddress { get { return textBoxIpAddress.Text; } }
		public int Port
		{
			get
			{
				int result = CryNotificationNetworkClient.NN_PORT;
				int.TryParse(textBoxPort.Text, out result);
				return result;
			}
		}

		#region Kernel32Functions

		[DllImport("kernel32.dll")]
		private static extern IntPtr LoadLibrary(string dllFilePath);

		[DllImport("kernel32.dll")]
		private static extern IntPtr GetProcAddress(IntPtr hModule, String procname);

		[DllImport("kernel32.dll")]
		private static extern Int32 GetLastError();

		[DllImport("kernel32.dll")]
		private static extern Int32 SetDllDirectory(string directory);

		#endregion


		public ConnectMessageBox(MainForm parent)
		{
			InitializeComponent();

			this.parent = parent;
			string currentIp = Settings.Default.IpAddress;
			int currentPort = Settings.Default.Port;
			textBoxIpAddress.Text = currentIp;
			textBoxPort.Text = currentPort.ToString();

			PopulateTargets();
			FindMatchingTarget(currentIp, currentPort);
		}

		private void buttonConnect_Click(object sender, EventArgs e)
		{
			Settings.Default.IpAddress = textBoxIpAddress.Text;
			Settings.Default.Port = Port;
		}

		private void PopulateTargets()
		{
			// Add local host
			comboBoxTarget.Items.Add(new TargetItem("PC (Game)", "127.0.0.1", CryNotificationNetworkClient.NN_PORT));
			comboBoxTarget.Items.Add(new TargetItem("PC (Editor)", "127.0.0.1", CryNotificationNetworkClient.NN_PORT_EDITOR));

            // removed:  XEDK
		}


		private void FindMatchingTarget(string ipAddress, int port)
		{
			foreach (TargetItem item in comboBoxTarget.Items)
			{
				if (item.IpAddress == ipAddress && item.Port == port)
				{
					comboBoxTarget.SelectedItem = item;
					return;
				}
			}
			// None found
			comboBoxTarget.SelectedItem = null;
		}

		private void comboBoxTarget_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (!suppressChangeEvents)
			{
				suppressChangeEvents = true;
				TargetItem selectedItem = comboBoxTarget.SelectedItem as TargetItem;
				if (selectedItem != null)
				{
					textBoxIpAddress.Text = selectedItem.IpAddress;
					textBoxPort.Text = selectedItem.Port.ToString();
				}
				else
				{
					textBoxIpAddress.Text = string.Empty;
					textBoxPort.Text = CryNotificationNetworkClient.NN_PORT.ToString();
				}
				suppressChangeEvents = false;
			}
		}

		private void textBoxIpAddress_TextChanged(object sender, EventArgs e)
		{
			if (!suppressChangeEvents)
			{
				suppressChangeEvents = true;
				FindMatchingTarget(textBoxIpAddress.Text, Port);
				suppressChangeEvents = false;
			}
		}

		private void textBoxPort_TextChanged(object sender, EventArgs e)
		{
			if (!suppressChangeEvents)
			{
				suppressChangeEvents = true;
				FindMatchingTarget(textBoxIpAddress.Text, Port);
				suppressChangeEvents = false;
			}
		}
	}
}
