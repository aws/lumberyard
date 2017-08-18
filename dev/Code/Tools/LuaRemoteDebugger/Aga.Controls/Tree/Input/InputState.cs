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
using System.Windows.Forms;
namespace Aga.Controls.Tree
{
	internal abstract class InputState
	{
		private TreeViewAdv _tree;

		public TreeViewAdv Tree
		{
			get { return _tree; }
		}

		public InputState(TreeViewAdv tree)
		{
			_tree = tree;
		}

		public abstract void KeyDown(System.Windows.Forms.KeyEventArgs args);
		public abstract void MouseDown(TreeNodeAdvMouseEventArgs args);
		public abstract void MouseUp(TreeNodeAdvMouseEventArgs args);

		/// <summary>
		/// handle OnMouseMove event
		/// </summary>
		/// <param name="args"></param>
		/// <returns>true if event was handled and should be dispatched</returns>
		public virtual bool MouseMove(MouseEventArgs args)
		{
			return false;
		}

		public virtual void MouseDoubleClick(TreeNodeAdvMouseEventArgs args)
		{
		}
	}
}
