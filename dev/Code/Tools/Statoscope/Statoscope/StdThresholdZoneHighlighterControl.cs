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
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Statoscope
{
	partial class StdThresholdZoneHighlighterControl : DesignableBaseSTZIIC
	{
		public StdThresholdZoneHighlighterControl(LogControl logControl, ZoneHighlighterRDI zrdiTree, RDICheckboxTreeView<ZoneHighlighterRDI> zrdiCTV)
			: base(logControl, zrdiTree, zrdiCTV)
		{
			InitializeComponent();
			ItemPathLabel = itemPathLabel;
		}

		protected override StdThresholdZRDI ZRDI
		{
			set
			{
				base.ZRDI = value;

				SetPathText();

				ZHIResetEnabled = false;

				nameTextBox.Text = ZRDI.Name;
				nameTextBox.Enabled = !ZRDI.NameFromPath;
				nameFromPathCheckBox.Checked = ZRDI.NameFromPath;
				itemColourButton.BackColor = ZRDI.Colour.ConvertToSysDrawCol();
				minValueNumericUpDown.Value = Convert.ToDecimal(ZRDI.MinThreshold);
				maxValueNumericUpDown.Value = Convert.ToDecimal(ZRDI.MaxThreshold);
				fpsValuesCheckBox.Checked = ZRDI.ValuesAreFPS;
				trackedPathTextBox.Text = ZRDI.TrackedPath;

				ZHIResetEnabled = true;
			}
		}

		protected override TextBox NameTextBox
		{
			get { return nameTextBox; }
		}

		protected override void SetNameText()
		{
			nameTextBox.Text = ZRDI.NameFromPath ? NameFromPath : ZRDI.Name;
			nameTextBox.Enabled = !ZRDI.NameFromPath;
			SetNameTextBoxBackColor();
		}

		string NameFromPath
		{
			get { return trackedPathTextBox.Text.Name(); }
		}

		protected override void SetItemColour(RGB colour)
		{
			base.SetItemColour(colour);
			itemColourButton.BackColor = colour.ConvertToSysDrawCol();
			ResetZHIs();
		}

		private void itemColourButton_Click(object sender, EventArgs e)
		{
			ItemColourButton_Click();
		}

		private void itemColourRandomButton_Click(object sender, EventArgs e)
		{
			ItemColourRandomButton_Click();
		}

		private void minValueNumericUpDown_ValueChanged(object sender, EventArgs e)
		{
			ZRDI.MinThreshold = (float)minValueNumericUpDown.Value;
			ResetZHIs();
		}

		private void maxValueNumericUpDown_ValueChanged(object sender, EventArgs e)
		{
			ZRDI.MaxThreshold = (float)maxValueNumericUpDown.Value;
			ResetZHIs();
		}

		private void nameTextBox_TextChanged(object sender, EventArgs e)
		{
			NameTextBox_TextChanged();
		}

		private void trackedPathTextBox_TextChanged(object sender, EventArgs e)
		{
			ZRDI.TrackedPath = trackedPathTextBox.Text;
			if (nameFromPathCheckBox.Checked)
				nameTextBox.Text = NameFromPath;
			ResetZHIs();
		}

		private void fpsValuesCheckBox_CheckedChanged(object sender, EventArgs e)
		{
			ZRDI.ValuesAreFPS = fpsValuesCheckBox.Checked;
			ResetZHIs();
		}

		private void nameFromPathCheckBox_CheckedChanged(object sender, EventArgs e)
		{
			if (!ZRDI.NameFromPath && nameFromPathCheckBox.Checked)
				nameTextBox.Text = NameFromPath;
			ZRDI.NameFromPath = nameFromPathCheckBox.Checked;
			nameTextBox.Enabled = !nameFromPathCheckBox.Checked;
		}

		private void trackedPathTextBox_DragEnter(object sender, DragEventArgs e)
		{
			if (e.Data.GetDataPresent(DataFormats.Text))
				e.Effect = DragDropEffects.Copy;
		}

		private void trackedPathTextBox_DragDrop(object sender, DragEventArgs e)
		{
			if (e.Data.GetDataPresent(DataFormats.Text))
				trackedPathTextBox.Text = (string)e.Data.GetData(DataFormats.Text);
		}
	}

	class DesignableBaseSTZIIC : ZoneHighlighterControl<StdThresholdZRDI>
	{
		public DesignableBaseSTZIIC()
			: this(null, null, null)
		{
		}

		public DesignableBaseSTZIIC(LogControl logControl, ZoneHighlighterRDI zrdiTree, RDICheckboxTreeView<ZoneHighlighterRDI> zrdiCTV)
			: base(logControl, zrdiTree, zrdiCTV)
		{
		}
	}
}
