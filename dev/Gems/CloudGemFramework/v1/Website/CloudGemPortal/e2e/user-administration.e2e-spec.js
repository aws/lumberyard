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

var testAccounts = require('./test-accounts.js');
var session = require('./session-helper.js');

describe('User Administration', function () {    
    var page = {
        search: {
            dropdown: $('.search-dropdown'),
            options: $$('button.dropdown-item'),
            input: $(".search-text")            
        },
        link: $('a.admin-user-mgt'),
        linkClass: 'a.admin-user-mgt'
    }
    
    beforeAll(function () {
        browser.get(browser.params.url);
        console.log("Waiting for the Cloud Gem Portal to load.")
        browser.wait(until.urlContains(session.loginPath), 10000, "urlContains"); // Checks that the current URL contains the expected text                   
        session.loginAsAdministrator();
        browser.wait(until.presenceOf(page.link), 20000, "Can't find user administration navigation bar link");
        browser.driver.sleep(2000)
        page.link.click();
    });

    describe('Integration Tests', function () {
        it('should be able to login and create a user', function () {
            createTestUser();            
        });
        
        it('should be able to search for user by username', function () {            
            browser.wait(until.presenceOf(page.search.dropdown), 10000, "Can't find user administation search dropdown");            
            page.search.dropdown.click();
            browser.wait(until.presenceOf(page.search.options.first()), 10000, "Can't access dropdown options for user administration search filtering");
            page.search.options.first().click();
            page.search.input.sendKeys(testAccounts.user.username);
            browser.driver.sleep(1000);
            // There should be two table rows.  One for the header row and one for the result
            expect($$("tr").count()).toEqual(2);
        });
        
        it('should be able to search for user by email', function () {
            browser.wait(until.presenceOf(page.search.dropdown), 10000, "Can't find user administation search dropdown");
            page.search.dropdown.click();
            page.search.options.get(1).click();
            page.search.input.sendKeys(testAccounts.user.email);
            browser.driver.sleep(1000);
            // There should be more than 1 row for this email
            expect($$("tr").count()).toEqual(2);
        });
        
        // logout of admin account and in with user account
        it('should be able to login with new user', function () {
            session.signOut();
            loginAsNewTestUser();
            // verify that the Admin tab doesn't exist anymore
            $$(page.linkClass).then(function (adminLinks) {
                expect(adminLinks.length).toBe(0);
            })
            
            session.signOut();
            
            // login as the admin account again
            session.loginAsAdministrator();
            browser.wait(until.presenceOf(page.link), 10000, "Can't find user administration navigation bar link");
            browser.driver.sleep(2000)
            page.link.click();
        });
        
        it('should be able to delete a user', function () {
            var testUsernameClass = $('#' + testAccounts.user.username + ' .fa.fa-trash-o');
            console.log("Searching for username=>" + testAccounts.user.username)
            browser.wait(until.presenceOf(page.search.dropdown), 10000, "Can't find user administation search dropdown");
            page.search.dropdown.click();
            browser.wait(until.presenceOf(page.search.options.first()), 10000, "Can't access dropdown options for user administration search filtering");
            page.search.options.first().click();
            page.search.input.sendKeys(testAccounts.user.username);
            browser.wait(until.presenceOf(testUsernameClass), 10000, "Can't find test user accounts.  Ensure that the users are not split up over two pages.");
            testUsernameClass.click();
            $('button[type="submit"]').click();
            browser.driver.sleep(1000)
        });
        
        it('should have a roles tab with Users and Administrator counts', function () {
            // click Roles tab                  
            browser.wait(until.presenceOf($('.tab')), 10000, "Can't find facet for user and administrator account totals");
            $$('.tab').get(1).click();
            // verify that there are three roles
            expect($$('tr').count()).toEqual(3)
        });

    });
});


function createTestUser() {
    browser.wait(until.presenceOf($('h2.user-count')), 10000, "Can't find user count");            
    $('#add-new-user').click();
    $('#username').sendKeys(testAccounts.user.username);
    $('#email').sendKeys(testAccounts.user.email);
    // clear the password field then once promise returns enter in custom password
    $('#password').clear().then(function () {
        $('#password').sendKeys(testAccounts.user.password);
    });
    $('button[type="submit"]').click();
    browser.driver.sleep(1000)
}

function loginAsNewTestUser() {
    // login with user account
    $('#username').sendKeys(testAccounts.user.username);
    $('#password').sendKeys(testAccounts.user.password);
    $('button[type="submit"]').click();
    browser.wait(protractor.ExpectedConditions.presenceOf($('#password')), 20000, "Unable to find the password field.");
    // create new password 
    $('#password').sendKeys(testAccounts.user.newPassword);
    $('#password-verify').sendKeys(testAccounts.user.newPassword);
    $('button[type="submit"]').click();
}