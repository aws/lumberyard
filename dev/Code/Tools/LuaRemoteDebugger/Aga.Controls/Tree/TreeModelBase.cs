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

namespace Aga.Controls.Tree
{
	public abstract class TreeModelBase: ITreeModel
	{
		public abstract System.Collections.IEnumerable GetChildren(TreePath treePath);
		public abstract bool IsLeaf(TreePath treePath);


		public event EventHandler<TreeModelEventArgs> NodesChanged;
		protected void OnNodesChanged(TreeModelEventArgs args)
		{
			if (NodesChanged != null)
				NodesChanged(this, args);
		}

		public event EventHandler<TreePathEventArgs> StructureChanged;
		protected void OnStructureChanged(TreePathEventArgs args)
		{
			if (StructureChanged != null)
				StructureChanged(this, args);
		}

		public event EventHandler<TreeModelEventArgs> NodesInserted;
		protected void OnNodesInserted(TreeModelEventArgs args)
		{
			if (NodesInserted != null)
				NodesInserted(this, args);
		}

		public event EventHandler<TreeModelEventArgs> NodesRemoved;
		protected void OnNodesRemoved(TreeModelEventArgs args)
		{
			if (NodesRemoved != null)
				NodesRemoved(this, args);
		}

		public virtual void Refresh()
		{
			OnStructureChanged(new TreePathEventArgs(TreePath.Empty));
		}
	}
}
