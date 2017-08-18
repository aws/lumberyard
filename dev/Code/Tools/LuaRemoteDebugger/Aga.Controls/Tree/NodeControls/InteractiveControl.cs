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
using System.ComponentModel;

namespace Aga.Controls.Tree.NodeControls
{
	public abstract class InteractiveControl : BindableControl
	{
		private bool _editEnabled = false;
		[DefaultValue(false)]
		public bool EditEnabled
		{
			get { return _editEnabled; }
			set { _editEnabled = value; }
		}

		protected bool IsEditEnabled(TreeNodeAdv node)
		{
			if (EditEnabled)
			{
				NodeControlValueEventArgs args = new NodeControlValueEventArgs(node);
				args.Value = true;
				OnIsEditEnabledValueNeeded(args);
				return Convert.ToBoolean(args.Value);
			}
			else
				return false;
		}

		public event EventHandler<NodeControlValueEventArgs> IsEditEnabledValueNeeded;
		private void OnIsEditEnabledValueNeeded(NodeControlValueEventArgs args)
		{
			if (IsEditEnabledValueNeeded != null)
				IsEditEnabledValueNeeded(this, args);
		}
	}
}
