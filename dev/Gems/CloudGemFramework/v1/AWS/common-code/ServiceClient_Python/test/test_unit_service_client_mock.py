#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #4 $

import unittest
import mock

import cgf_service_client
import cgf_service_client.mock

class UnitTest_CloudGemFramework_ServiceClient_mock(unittest.TestCase):

    def test_patch_works_as_function_decorator(self):

        origional_Path = cgf_service_client.Path

        class PatchTarget(object):

            def a():
                pass

            def b():
                pass

            def c():
                pass

        pt = PatchTarget()

        # mock.patch seems to do funky things with ordering the parameters it produces
        # The next effect is that if our patch is mixed with their patches, our patch
        # object will be passed after all the origional args but before all the mock
        # args.

        @mock.patch.object(pt, 'a')
        @cgf_service_client.mock.patch()
        @mock.patch.object(pt, 'b')
        @mock.patch.object(pt, 'c')
        def f(*args, **kwargs):
            print 'args', args
            self.assertEquals(args[0], 1)
            self.assertEquals(args[1], 2)
            self.assertIsInstance(args[2], cgf_service_client.mock.MockServiceClient)
            self.assertIsInstance(args[3], mock.MagicMock)
            self.assertIsInstance(args[4], mock.MagicMock)
            self.assertIsInstance(args[5], mock.MagicMock)
            self.assertIsNot(cgf_service_client.Path, origional_Path)
            self.assertEquals(kwargs, { 'a': 3, 'b': 4 })

        f(1, 2, a=3, b=4)

        self.assertIs(cgf_service_client.Path, origional_Path)


    def test_patch_works_as_class_function_decorator(self):

        origional_Path = cgf_service_client.Path

        test_case = self

        class Test(object):

            @cgf_service_client.mock.patch()
            def f(self, *args, **kwargs):
                test_case.assertIsInstance(self, Test)
                test_case.assertEquals(args[0], 1)
                test_case.assertEquals(args[1], 2)
                test_case.assertIsInstance(args[2], cgf_service_client.mock.MockServiceClient)
                test_case.assertIsNot(cgf_service_client.Path, origional_Path)
                test_case.assertEquals(kwargs, { 'a': 3, 'b': 4 })

        Test().f(1, 2, a=3, b=4)

        self.assertIs(cgf_service_client.Path, origional_Path)


    def test_patch_works_as_context_object(self):

        origional_Path = cgf_service_client.Path

        with cgf_service_client.mock.patch() as mock_client:
            self.assertIsInstance(mock_client, cgf_service_client.mock.MockServiceClient)
            self.assertIsNot(cgf_service_client.Path, origional_Path)

        self.assertIs(cgf_service_client.Path, origional_Path)


    @cgf_service_client.mock.patch()
    def test_mock_methods_called(self, mock_service_client):

        setup_path = cgf_service_client.Path('test-url')
           
        self.assertIsInstance(setup_path.GET, cgf_service_client.mock.HttpMethodMock)
        self.assertIsInstance(setup_path.PUT, cgf_service_client.mock.HttpMethodMock)
        self.assertIsInstance(setup_path.POST, cgf_service_client.mock.HttpMethodMock)
        self.assertIsInstance(setup_path.DELETE, cgf_service_client.mock.HttpMethodMock)

        setup_path.GET.return_value = 'GET return'
        setup_path.PUT.return_value = 'PUT return'
        setup_path.POST.return_value = 'POST return'
        setup_path.DELETE.return_value = 'DELETE return'

        target_path_a = cgf_service_client.Path('test-url', A = 10)

        self.assertEquals(target_path_a.GET(B = 20), 'GET return')
        self.assertEquals(target_path_a.PUT('PUT arg', B = 30), 'PUT return')
        self.assertEquals(target_path_a.POST('POST arg', B = 40), 'POST return')
        self.assertEquals(target_path_a.DELETE(B = 50), 'DELETE return')

        setup_path.GET.assert_called_once_with(B = 20)
        setup_path.PUT.assert_called_once_with('PUT arg', B = 30)
        setup_path.POST.assert_called_once_with('POST arg', B = 40)
        setup_path.DELETE.assert_called_once_with(B = 50)

        setup_path.GET.assert_all_calls_have_config_containing(A = 10)
        setup_path.PUT.assert_all_calls_have_config_containing(A = 10)
        setup_path.POST.assert_all_calls_have_config_containing(A = 10)
        setup_path.DELETE.assert_all_calls_have_config_containing(A = 10)

        target_path_b = cgf_service_client.Path('test-url', A = 100)

        self.assertEquals(target_path_b.GET(B = 200), 'GET return')

        setup_path.GET.assert_has_calls([mock.call(B = 20), mock.call(B = 200)])

        with self.assertRaises(AssertionError):
            setup_path.GET.assert_all_calls_have_config_containing(A = 10)


    @cgf_service_client.mock.patch()
    def test_same_mock_used_for_same_url(self, mock_service_client):

        url = 'test-url'

        a = mock_service_client.for_url(url)
        b = mock_service_client.for_url(url)

        a.GET()
        a.POST({'a': 10})
        a.PUT({'a': 10})
        a.DELETE()

        b.GET.assert_called_once_with()
        b.POST.assert_called_once_with({'a': 10})
        b.PUT.assert_called_once_with({'a': 10})
        b.DELETE.assert_called_once_with()

        aa = a.navigate('a')
        bb = b.navigate('b')

        aa.GET()
        aa.POST({'a': 10})
        aa.PUT({'a': 10})
        aa.DELETE()

        bb.GET.assert_not_called()
        bb.POST.assert_not_called()
        bb.PUT.assert_not_called()
        bb.DELETE.assert_not_called()




if __name__ == '__main__':
    unittest.main()
