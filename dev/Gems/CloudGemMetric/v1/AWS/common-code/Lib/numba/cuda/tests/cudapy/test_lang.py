"""
Test basic language features

"""
from __future__ import print_function, absolute_import, division

import numpy as np
from numba import cuda, float64
from numba.cuda.testing import unittest


class TestLang(unittest.TestCase):
    def test_enumerate(self):
        tup = (1., 2.5, 3.)

        @cuda.jit("void(float64[:])")
        def foo(a):
            for i, v in enumerate(tup):
                a[i] = v

        a = np.zeros(len(tup))
        foo(a)
        self.assertTrue(np.all(a == tup))

    def test_zip(self):
        t1 = (1, 2, 3)
        t2 = (4.5, 5.6, 6.7)

        @cuda.jit("void(float64[:])")
        def foo(a):
            c = 0
            for i, j in zip(t1, t2):
                c += i + j
            a[0] = c

        a = np.zeros(1)
        foo(a)
        b = np.array(t1)
        c = np.array(t2)
        self.assertTrue(np.all(a == (b + c).sum()))

    def test_issue_872(self):
        '''
        Ensure that macro expansion works for more than one block (issue #872)
        '''

        @cuda.jit("void(float64[:,:])")
        def macros_in_multiple_blocks(ary):
            for i in range(2):
                tx = cuda.threadIdx.x
            for j in range(3):
                ty = cuda.threadIdx.y
            sm = cuda.shared.array((2, 3), float64)
            sm[tx, ty] = 1.0
            ary[tx, ty] = sm[tx, ty]

        a = np.zeros((2, 3))
        macros_in_multiple_blocks[1, (2, 3)](a)


if __name__ == '__main__':
    unittest.main()

