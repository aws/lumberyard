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

// Description : Generalized Octree used by telemetry to cluster data


#ifndef CRYINCLUDE_TELEMETRYPLUGIN_OCTREE_H
#define CRYINCLUDE_TELEMETRYPLUGIN_OCTREE_H
#pragma once


//////////////////////////////////////////////////////////////////////////
// OctreeAgent concept
//////////////////////////////////////////////////////////////////////////

/*

template<typename T>
struct OctreeAgent
{
    Vec3 getPosition(const T& element) const;
};

*/

//////////////////////////////////////////////////////////////////////////


template<typename T, typename AgentT>
class COctreeCell;
template<typename T, typename AgentT>
class COctreeBranch;
template<typename T, typename AgentT>
class COctreeLeaf;
template<typename T, typename AgentT>
class COctree;
template<typename T, typename AgentT>
class IOctreeVisitor;


//////////////////////////////////////////////////////////////////////////
// COctreeCell
//////////////////////////////////////////////////////////////////////////

template<typename T, typename AgentT>
class COctreeCell
{
public:
    typedef COctree<T, AgentT> octree_type;
    typedef COctreeCell<T, AgentT> cell_type;
    typedef COctreeBranch<T, AgentT> branch_type;
    typedef COctreeLeaf<T, AgentT> leaf_type;

    COctreeCell(octree_type& octree, const AABB& box, branch_type* parent, size_t depth)
        : m_box(box)
        , m_octree(octree)
        , m_parent(parent)
        , m_depth(depth)
    { }

    virtual ~COctreeCell()
    { }

    size_t getDepth() const
    {
        return m_depth;
    }

    branch_type* getParent() const
    {
        return m_parent;
    }

    const AABB& getBox() const
    {
        return m_box;
    }

    octree_type& getOctree() const
    {
        return m_octree;
    }

    const AgentT& getAgent() const
    {
        return m_octree.getAgent();
    }

    virtual bool Insert(const T& element, const Vec3& pos) = 0;

    virtual void AcceptVisitor(IOctreeVisitor<T, AgentT>& visitor) = 0;

private:
    AABB m_box;
    octree_type& m_octree;
    branch_type* m_parent;
    size_t m_depth;
};

//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// COctreeCell
//////////////////////////////////////////////////////////////////////////

/*
                        3---7
                     /|  /|             y
                    2---6 |             |  z
                    | 1-|-5             | /
                    |/  |/              |/
                    0---4                   |-----x
*/

template<typename T, typename AgentT>
class COctreeBranch
    : public COctreeCell<T, AgentT>
{
public:

    COctreeBranch(octree_type& octree, const AABB& box, branch_type* parent)
        : COctreeCell<T, AgentT>(octree, box, parent, parent ? parent->getDepth() + 1 : 0)
    {
        Vec3 halfSize = getBox().max - getBox().min;
        halfSize *= 0.5f;

        // Populates the children
        Vec3 lcorner(ZERO);
        for (size_t ix = 0; ix != 2; ++ix)
        {
            lcorner.x = halfSize.x * ix;

            for (size_t iy = 0; iy != 2; ++iy)
            {
                lcorner.y = halfSize.y * iy;

                for (size_t iz = 0; iz != 2; ++iz)
                {
                    lcorner.z = halfSize.z * iz;
                    Vec3 finalCorner = lcorner + getBox().min;
                    m_children[(ix << 2) + (iy << 1) + iz] = new leaf_type(getOctree(), AABB(finalCorner, finalCorner + halfSize), this);
                }
            }
        }// for
    }

    ~COctreeBranch()
    {
        for (size_t i = 0; i != 8; ++i)
        {
            delete m_children[i];
        }
    }

    virtual bool Insert(const T& element, const Vec3& pos)
    {
        if (!getBox().IsContainPoint(pos))
        {
            return false;
        }

        for (size_t i = 0; i != 8; ++i)
        {
            if (m_children[i]->Insert(element, pos))
            {
                return true;
            }
        }

        CRY_ASSERT_MESSAGE(false, "error while inserting element to octree");
        return false;
    }

    virtual void AcceptVisitor(IOctreeVisitor<T, AgentT>& visitor)
    {
        if (visitor.EnterBranch(*this))
        {
            for (size_t i = 0; i != 8; ++i)
            {
                m_children[i]->AcceptVisitor(visitor);
            }

            visitor.LeaveBranch(*this);
        }
    }

    // Splits the leaf node and inserts the data to it
    void _splitChild(leaf_type* leaf, const T& element, const Vec3& pos)
    {
        branch_type* newBranch = new branch_type(getOctree(), leaf->getBox(), this);

        // Insert old data
        T* data = leaf->getData();
        for (size_t i = 0, size = leaf->getDataSize(); i != size; ++i)
        {
            newBranch->Insert(data[i], getAgent().getPosition(data[i]));
        }

        // Insert new elem
        newBranch->Insert(element, pos);

        // Replace old element
        for (size_t i = 0; i != 8; ++i)
        {
            if (m_children[i] == leaf)
            {
                m_children[i] = newBranch;
            }
        }

        delete leaf;
    }

private:
    cell_type* m_children[8];
};

//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// COctreeLeaf
//////////////////////////////////////////////////////////////////////////

template<typename T, typename AgentT>
class COctreeLeaf
    : public COctreeCell<T, AgentT>
{
public:

    COctreeLeaf(octree_type& octree, const AABB& box, branch_type* parent)
        : COctreeCell<T, AgentT>(octree, box, parent, parent ? parent->getDepth() + 1 : 0)
    { }

    size_t getDataSize() const
    {
        return m_data.size();
    }

    T* getData()
    {
        return m_data.size() != 0 ? &m_data[0] : 0;
    }

    virtual bool Insert(const T& element, const Vec3& pos)
    {
        if (!getBox().IsContainPoint(pos))
        {
            return false;
        }

        if (ShouldSplit())
        {
            // Deletes this node!
            getParent()->_splitChild(this, element, pos);
            return true;
        }

        m_data.push_back(element);
        return true;
    }

    virtual void AcceptVisitor(IOctreeVisitor<T, AgentT>& visitor)
    {
        visitor.VisitLeaf(*this);
    }

private:

    bool ShouldSplit() const
    {
        if (m_data.size() < getOctree().getMaxChildren())
        {
            return false;
        }

        if (getDepth() >= getOctree().getMaxDepth())
        {
            return false;
        }

        float minSize = getOctree().getMinCellSize();
        Vec3 delta = getBox().max - getBox().min;
        return delta.x > minSize && delta.y > minSize && delta.z > minSize;
    }

private:
    std::vector<T> m_data;
};

//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// IOctreeVisitor
//////////////////////////////////////////////////////////////////////////

template<typename T, typename AgentT>
class IOctreeVisitor
{
public:
    typedef COctreeBranch<T, AgentT> branch_type;
    typedef COctreeLeaf<T, AgentT> leaf_type;

    virtual bool EnterBranch(branch_type& branch) = 0;
    virtual void LeaveBranch(branch_type& branch) = 0;
    virtual void VisitLeaf(leaf_type& leaf) = 0;
};

//////////////////////////////////////////////////////////////////////////

template<typename T, typename AgentT>
class COctreeRebuildVisitor
    : IOctreeVisitor<T, AgentT>
{
    COctree<T, AgentT>& m_octree;

public:
    COctreeRebuildVisitor(COctree<T, AgentT>& octree)
        : m_octree(octree)
    { }

    virtual bool EnterBranch(branch_type& branch) { return true; }
    virtual void LeaveBranch(branch_type& branch) { }
    virtual void VisitLeaf(leaf_type& leaf)
    {
        T* data = leaf.getData();
        for (size_t i = 0, size = leaf.getDataSize(); i != size; ++i)
        {
            m_octree.Insert(data[i]);
        }
    }
};

//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// COctree
//////////////////////////////////////////////////////////////////////////

template<typename T, typename AgentT>
class COctree
{
public:
    typedef COctreeCell<T, AgentT> cell_type;
    typedef COctreeBranch<T, AgentT> branch_type;
    typedef COctreeLeaf<T, AgentT> leaf_type;

    COctree()
        : m_box(AABB::RESET)
        , m_maxChildren(10)
        , m_maxDepth(50)
        , m_minClellSize(0.001f)
    { }

    COctree(AgentT agent, const AABB& world, size_t maxChildren, size_t maxDepth, float minCellSize)
        : m_box(world)
        , m_agent(agent)
        , m_maxChildren(maxChildren)
        , m_maxDepth(maxDepth)
        , m_minClellSize(minCellSize)
    { }

    const AgentT& getAgent() const { return m_agent; }
    void setAgent(const AgentT& agent) { m_agent = agent; }

    const AABB& getWorldBox() const { return m_box; }
    void setWorldBox(const AABB& box) { m_box = box; }

    size_t getMaxChildren() const { return m_maxChildren; }
    void setMaxChildren(size_t maxChildren) { m_maxChildren = maxChildren; }

    size_t getMaxDepth() const { return m_maxDepth; }
    void setMaxDepth(size_t maxDepth) { m_maxDepth = maxDepth; }

    float getMinCellSize() const { return m_minClellSize; }
    void setMinCellSize(float minCellSize) { m_minClellSize = minCellSize; }

    branch_type* getRootCell() { return m_root.get(); }

    void Rebuild()
    {
        std::auto_ptr<branch_type> oldTree(m_root);
        m_root.reset(new branch_type(*this, m_box, 0));

        if (oldTree.get())
        {
            COctreeRebuildVisitor<T, AgentT> rv(*this);
            oldTree->AcceptVisitor(rv);
        }
    }

    void Clear()
    {
        m_root.reset();
    }

    void AcceptVisitor(IOctreeVisitor<T, AgentT>& visitor)
    {
        if (m_root.get())
        {
            m_root->AcceptVisitor(visitor);
        }
    }

    bool Insert(const T& element)
    {
        if (!m_root.get())
        {
            m_root.reset(new branch_type(*this, m_box, 0));
        }

        return m_root->Insert(element, m_agent.getPosition(element));
    }

private:
    std::auto_ptr<branch_type> m_root;
    AABB m_box;
    AgentT m_agent;
    size_t m_maxChildren;
    size_t m_maxDepth;
    float m_minClellSize;
};

//////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_TELEMETRYPLUGIN_OCTREE_H
