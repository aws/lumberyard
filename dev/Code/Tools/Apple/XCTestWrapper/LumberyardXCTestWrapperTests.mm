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
#import <XCTest/XCTest.h>
#import <UIKit/UIKit.h>

@interface LumberyardXCTestWrapperTests : XCTestCase
{

@private
    UIWindow* window;
    UIViewController* viewController;

    NSObject<UIApplicationDelegate>* delegate;
}
@end

static void PumpEventLoop()
{
    SInt32 result;
    do
    {
        result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, DBL_EPSILON, TRUE);
    }
    while (result == kCFRunLoopRunHandledSource);
}


@implementation LumberyardXCTestWrapperTests

- (void)setUp
{
    [super setUp];

    // since the lumberyard runtime creates the window and view controller, we need to manually
    // create them in order to show any dialogs
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    window = [[UIWindow alloc] initWithFrame: screenBounds];

    viewController = [[UIViewController alloc] init];
    window.rootViewController = viewController;

    delegate = [[UIApplication sharedApplication] delegate];
}

- (void)tearDown
{
    [super tearDown];

    window.rootViewController = nil;
    [viewController release];
    [window release];
}

- (void)testRuntime
{
    int exitCode = -1;
    SEL launchLumberyardSelector = NSSelectorFromString(@"runLumberyardApplication");
    if ([delegate respondsToSelector:launchLumberyardSelector])
    {
        NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:[[delegate class] instanceMethodSignatureForSelector:launchLumberyardSelector]];
        [invocation setSelector:launchLumberyardSelector];
        [invocation setTarget:delegate];

        [invocation invoke];

        [invocation getReturnValue:&exitCode];
    }
    XCTAssertTrue(exitCode == 0, "The Lumberyard runtime either exited with a non-zero return code or was unable to start");
}

- (void)testInstall
{
    XCTAssertNotNil(window, "Failed to create the install window");
    XCTAssertNotNil(viewController, "Failed to create the install view controller");

    [window makeKeyAndVisible];

    NSString* nsTitle = [NSString stringWithUTF8String:"Notice"];
    NSString* nsMessage = [NSString stringWithUTF8String:"Your Lumberyard app has been installed!"];
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:nsTitle message:nsMessage preferredStyle:UIAlertControllerStyleAlert];

    __block bool noticeAccepted = false;
    UIAlertAction* okAction = [UIAlertAction actionWithTitle:[NSString stringWithUTF8String:"OK"] style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) { noticeAccepted = true; }];
    [alert addAction:okAction];

    [viewController presentViewController:alert animated:YES completion:nil];
    while (!noticeAccepted)
    {
        PumpEventLoop();
    }
}

@end
