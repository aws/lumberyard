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

namespace Statoscope
{
	partial class ConfluenceLoginForm : Form
	{
		ConfluenceService m_confluenceService;

		public ConfluenceLoginForm(ConfluenceService confluenceService, string initialUsername, string initialPassword)
		{
			m_confluenceService = confluenceService;

			FormBorderStyle = FormBorderStyle.FixedDialog;
			InitializeComponent();

			UsernameTextBox.Text = initialUsername;
			PasswordTextBox.Text = initialPassword;
		}

		private void LoginButton_Click(object sender, EventArgs e)
		{
			LoginInfoLabel.Text = "Logging in...";
			LoginInfoLabel.Refresh();

			if (m_confluenceService.Login(UsernameTextBox.Text, PasswordTextBox.Text))
			{
				Close();
			}
			else
			{
				LoginInfoLabel.Text = "Please check username and password";
				MessageBox.Show("Login failed :(", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
		}
	}
}
