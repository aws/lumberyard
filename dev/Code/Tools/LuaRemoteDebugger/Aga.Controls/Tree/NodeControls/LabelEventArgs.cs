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
using System.Text;

namespace Aga.Controls.Tree.NodeControls
{
	public class LabelEventArgs : EventArgs
	{
		private object _subject;
		public object Subject
		{
			get { return _subject; }
		}

		private string _oldLabel;
		public string OldLabel
		{
			get { return _oldLabel; }
		}

		private string _newLabel;
		public string NewLabel
		{
			get { return _newLabel; }
		}

		public LabelEventArgs(object subject, string oldLabel, string newLabel)
		{
			_subject = subject;
			_oldLabel = oldLabel;
			_newLabel = newLabel;
		}
	}
}
