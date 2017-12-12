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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_BASICUTILS_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_BASICUTILS_H
#pragma once


#include <PRT/PRTTypes.h>

namespace NSH
{
    //!< node memory pool manager
    template<class TNode>
    class CNodeMemoryPool_tpl
    {
    public:
        CNodeMemoryPool_tpl()
            : m_pNodeMemoryPool(NULL)
            , m_NodeCount(0)
            , m_AllocatedNodeCount(0){}
        TNode* AllocateNext()               //!< retrieves and allocates a new node
        {
            if (!m_pNodeMemoryPool)
            {
                assert(false);
                return NULL;
            }
            m_AllocatedNodeCount++;
            if (m_AllocatedNodeCount > m_NodeCount)
            {
                assert(false);
                return NULL;
            }
            return &(m_pNodeMemoryPool[m_AllocatedNodeCount - 1]);
        }

        void Construct(const uint32 cNodeCount) //!< construct pool
        {
            assert(cNodeCount > 0);
            assert(m_pNodeMemoryPool == NULL && m_AllocatedNodeCount == 0);
            m_pNodeMemoryPool = new TNode[cNodeCount];
            m_NodeCount = cNodeCount;
        }

        ~CNodeMemoryPool_tpl()
        {
            if (m_pNodeMemoryPool)
            {
                delete [] m_pNodeMemoryPool;
                m_pNodeMemoryPool = NULL;
            }
        }
    private:
        TNode* m_pNodeMemoryPool;                           //!< node memory pool
        uint32      m_NodeCount;                        //!< allocated node count
        uint32      m_AllocatedNodeCount;       //!< currently allocated node count
    };

    /************************************************************************************************************************************************/

    //!< vector containing pointers to vector and defining an iterator hiding the separate vectors
    //!< defines only the necessary basic functionality
    // follows naming convention from stl
    template< class T >
    class vectorpvector
    {
    public:
        typedef std::vector<T, CSHAllocator<T> > TElemVec;
        typedef std::vector<TElemVec*, CSHAllocator<TElemVec*> > TDataVec;

        template< class U >
        class CVectorPVectorIterator
        {
        public:
            CVectorPVectorIterator(typename TElemVec::iterator rOrigIter, const vectorpvector<T>* cpParentVector, const size_t cElem = 0)
                : m_VectorIter(rOrigIter)
                , m_pParentVector(cpParentVector)
                , m_CurrentVectorElem(cElem){}

            CVectorPVectorIterator(const CVectorPVectorIterator<T>& rIter)
                : m_VectorIter(rIter.m_VectorIter)
                , m_pParentVector(rIter.m_pParentVector)
                , m_CurrentVectorElem(rIter.m_CurrentVectorElem){}

            CVectorPVectorIterator<U>& operator ++()                                                //!< type the sample iterator, ++ operator
            {
                if (++m_VectorIter == (m_pParentVector->m_Data[m_CurrentVectorElem])->end())
                {
                    //go to next vector pointer
                    if (m_CurrentVectorElem >= m_pParentVector->m_Data.size() - 1)
                    {
                        m_VectorIter = m_pParentVector->m_Data[m_pParentVector->m_Data.size() - 1]->end();
                    }
                    else
                    {
                        size_t index = m_CurrentVectorElem + 1;
                        while (index < m_pParentVector->m_Data.size() - 1 && m_pParentVector->m_Data[index]->empty())
                        {
                            index++;
                        }
                        m_CurrentVectorElem = index;
                        if (index == m_pParentVector->m_Data.size() - 1 && m_pParentVector->m_Data[index]->empty())
                        {
                            m_VectorIter = m_pParentVector->m_Data[index]->end();//end is reached
                        }
                        else
                        {
                            m_VectorIter = m_pParentVector->m_Data[index]->begin();
                        }
                    }
                }
                return *this;
            }

            const bool operator ==(const CVectorPVectorIterator<U>& rIter) const //!< type the sample iterator, comparison operator )for == End())
            {
                return ((m_CurrentVectorElem == rIter.m_CurrentVectorElem) && (m_VectorIter == rIter.m_VectorIter));
            }

            const bool operator !=(const CVectorPVectorIterator<U>& rIter) const //!< type the sample iterator, comparison operator )for == End())
            {
                return ((m_CurrentVectorElem != rIter.m_CurrentVectorElem) || (m_VectorIter != rIter.m_VectorIter));
            }

            T& operator*()                                                                          //!< type the sample iterator, dereference operator
            {
                return *m_VectorIter;
            }

            const CVectorPVectorIterator& operator=(const CVectorPVectorIterator& rCopyFrom)//!< assignment operator
            {
                m_VectorIter                    = rCopyFrom.m_VectorIter;
                m_pParentVector             = rCopyFrom.m_pParentVector;
                m_CurrentVectorElem     = rCopyFrom.m_CurrentVectorElem;
                return *this;
            }

        private:
            typename TElemVec::iterator     m_VectorIter;       //!< reference to the vector iterator
            const vectorpvector<U>* m_pParentVector;                        //!< reference to the parent vector class to iterate on the vectors
            size_t  m_CurrentVectorElem;                                                //!< current vector iterator lives within m_rParentVector

            CVectorPVectorIterator();                                                       //!< forbidden default constructor
        };

        typedef CVectorPVectorIterator<T> iterator;

        vectorpvector()
            : m_Size(0)
            , m_Empty(true)                                                 //!< default constructor adds valid empty element which is removed after inserting a valid one
        {
            m_Data.push_back(&m_sEndVector);
        }

        const size_t size() const
        {
            if (m_Empty)
            {
                return 0;
            }
            return m_Size;
        }       //!< retrieves number of contained elements of type T

        void push_back(const TElemVec* p)       //!< adds a pointer to a vector of contained elements of type T
        {
            if (m_Empty)
            {
                m_Data.clear();
                m_Empty = false;
            }
            m_Data.push_back((TElemVec*)p);
            m_Size += p->size();
        }

        void clear()        //!< clears the vector
        {
            m_Size = 0;
            m_Data.clear();
            m_Empty = true;
            m_Data.push_back(&m_sEndVector);
        }

        const iterator begin() //!< retrieves the begin iterator
        {
            int index = 0;
            while (m_Data[index]->empty())
            {
                index++;
            }
            return iterator(m_Data[index]->begin(), (const vectorpvector<T>*) this, (size_t)index);
        }

        const iterator end() //!< retrieves the end iterator
        {
            if (m_Empty)
            {
                return iterator(m_Data[0]->end(), this, (size_t)0);
            }
            return iterator(m_Data[m_Data.size() - 1]->end(), this, m_Data.size() - 1);
        }

        TDataVec& GetData()
        {
            return m_Data;
        }

        TElemVec& operator[](const size_t cIndex)
        {
            assert(cIndex < m_Data.size());
            return *(m_Data[cIndex]);
        }

        const size_t vector_size() const
        {
            if (m_Empty)
            {
                return 0;
            }
            return m_Data.size();
        }       //!< retrieves number of contained vectors

    private:
        TElemVec m_sEndVector;                                  //!< end vector to have valid iterators

        size_t m_Size;                                                  //!< absolute count of contained elements of type T
        TDataVec m_Data;                                                //!< vector containing the vector pointers
        bool m_Empty;                                                       //!< indicates whether the vector is still empty

        friend class CVectorPVectorIterator<T>;
    };

    typedef vectorpvector<CSample_tpl<SCoeffList_tpl<TScalarCoeff> > > TScalarVecVec;
}

#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_BASICUTILS_H
