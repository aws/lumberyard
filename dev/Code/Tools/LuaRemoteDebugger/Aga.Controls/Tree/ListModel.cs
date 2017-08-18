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
using System.Collections;

namespace Aga.Controls.Tree
{
	public class ListModel : TreeModelBase
	{
		private IList _list;

		public int Count
		{
			get { return _list.Count; }
		}

		public ListModel()
		{
			_list = new List<object>();
		}

		public ListModel(IList list)
		{
			_list = list;
		}

		public override IEnumerable GetChildren(TreePath treePath)
		{
			return _list;
		}

		public override bool IsLeaf(TreePath treePath)
		{
			return true;
		}

		public void AddRange(IEnumerable items)
		{
			foreach (object obj in items)
				_list.Add(obj);
			OnStructureChanged(new TreePathEventArgs(TreePath.Empty));
		}

		public void Add(object item)
		{
			_list.Add(item);
			OnNodesInserted(new TreeModelEventArgs(TreePath.Empty, new int[] { _list.Count - 1 }, new object[] { item }));
		}

		public void Clear()
		{
			_list.Clear();
			OnStructureChanged(new TreePathEventArgs(TreePath.Empty));
		}
	}
}
