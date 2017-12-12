var testAccounts = require('./test-accounts.js');
var session = require('./session-helper.js');

describe('User Administration', function() {
  var until = protractor.ExpectedConditions;

  beforeAll(function() {
      browser.get('http://localhost:3000');
      session.loginAsAdministrator();
      browser.wait(until.presenceOf($('a[routerLink="admin"]')), 10000, "Can't find user administration navigation bar link");
      $('a[routerLink="admin"]').click();
  }); 

  afterAll(function() {
     session.signOut();
  });

  describe('Integration Tests', function() {
    it('should be able to login and create a user', function() {
      createTestUser();
    });

    it('should be able to search for user by username', function() {
      browser.wait(until.presenceOf($('.search-dropdown')), 10000, "Can't find user administation search dropdown");
      $(".search-dropdown").click();
      browser.wait(until.presenceOf($$('button.dropdown-item').first()), 10000, "Can't access dropdown options for user administration search filtering");
      $$("button.dropdown-item").first().click();
      $(".search-text").sendKeys(testAccounts.user.username);
      browser.driver.sleep(1000);
      // There should be two table rows.  One for the header row and one for the result
      expect($$("tr").count()).toEqual(2);
    });

    it('should be able to search for user by email', function() {
      browser.wait(until.presenceOf($('.search-dropdown')), 10000, "Can't find user administation search dropdown");
      $(".search-dropdown").click();
      $$("button.dropdown-item").get(1).click();
      $(".search-text").sendKeys(testAccounts.user.email);
      browser.driver.sleep(1000);
      // There should be more than 1 row for this email
      expect($$("tr").count()).toEqual(2);
    });

    // logout of admin account and in with user account
    it('should be able to login with new user', function() {
      session.signOut();
      loginAsNewTestUser();
      // verify that the Admin tab doesn't exist anymore
      $$('a[routerlink=admin]').then(function(adminLinks) {
        expect(adminLinks.length).toBe(0);
      })

      session.signOut();

      // login as the admin account again
      session.loginAsAdministrator();
      browser.wait(until.presenceOf($('a[routerLink=admin]')), 10000, "Can't find user administration navigation bar link");
      $('a[routerLink=admin]').click();
    });
    
    it('should be able to delete a user', function() {
      var testUsernameClass = $('#' + testAccounts.user.username + ' .fa.fa-trash-o');
      browser.wait(until.presenceOf(testUsernameClass), 10000, "Can't find test user accounts.  Ensure that the users are not split up over two pages.");
      testUsernameClass.click();
      $('button[type="submit"]').click();
    });

    it('should have a roles tab with Users and Administrator counts', function() {
      // click Roles tab
      browser.driver.sleep(2000);
      browser.wait(until.presenceOf($$('.tab').get(1)), 10000, "Can't find facet for user and administrator account totals");
      $$('.tab').get(1).click();
      // verify that there are three roles
      expect($$('tr').count()).toEqual(3)
    });

  });
});


function createTestUser() {
  $('#add-new-user').click();
  $('#username').sendKeys(testAccounts.user.username);
  $('#email').sendKeys(testAccounts.user.email);
  // clear the password field then once promise returns enter in custom password
  $('#password').clear().then(function() {
    $('#password').sendKeys(testAccounts.user.password);
  });
  $('button[type="submit"]').click();
  browser.driver.sleep(5000);
}

function loginAsNewTestUser() {
    // login with user account
  $('#username').sendKeys(testAccounts.user.username);
  $('#password').sendKeys(testAccounts.user.password);
  $('button[type="submit"]').click();
  browser.driver.sleep(2500);
  // create new password 
  $('#password').sendKeys(testAccounts.user.newPassword);
  $('#password-verify').sendKeys(testAccounts.user.newPassword);
  $('button[type="submit"]').click();
}