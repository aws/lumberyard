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

namespace LuaRemoteDebugger
{
	public partial class FindTextDialog : Form
	{
		[Category("Action")]
		public event EventHandler<FindTextEventArgs> FindText;

		public RichTextBoxFinds FindOptions
		{
			get
			{
				RichTextBoxFinds findOptions = RichTextBoxFinds.None;
				if (checkBoxMatchCase.Checked)
				{
					findOptions |= RichTextBoxFinds.MatchCase;
				}
				if (checkBoxWholeWord.Checked)
				{
					findOptions |= RichTextBoxFinds.WholeWord;
				}
				if (checkBoxSearchUp.Checked)
				{
					findOptions |= RichTextBoxFinds.Reverse;
				}
				return findOptions;
			}
			set
			{
				checkBoxMatchCase.Checked = (value & RichTextBoxFinds.MatchCase) != 0;
				checkBoxWholeWord.Checked = (value & RichTextBoxFinds.WholeWord) != 0;
				checkBoxSearchUp.Checked = (value & RichTextBoxFinds.Reverse) != 0;
			}
		}

		public FindTextDialog()
		{
			InitializeComponent();
		}

		private void buttonCancel_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void buttonFindNext_Click(object sender, EventArgs e)
		{
			textBoxFind.SelectionStart = 0;
			textBoxFind.SelectionLength = textBoxFind.TextLength;
			if (FindText != null)
			{
				FindText(this, new FindTextEventArgs(textBoxFind.Text, FindOptions));
			}
		}
	}

	public class FindTextEventArgs : EventArgs
	{
		string text;
		RichTextBoxFinds options;

		public FindTextEventArgs(string text, RichTextBoxFinds options) { this.text = text; this.options = options; }

		public string Text { get { return text; } }
		public RichTextBoxFinds Options { get { return options; } }
	}
}
