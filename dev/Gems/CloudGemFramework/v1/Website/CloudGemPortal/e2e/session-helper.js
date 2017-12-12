var testAccounts = require('./test-accounts.js');

var sessionHelper = module.exports = {

     loginAsAdministrator: function(){
        var until = protractor.ExpectedConditions;
        browser.wait(until.presenceOf($('#username')), 10000, "Unable to login- couldn't find text input with id=username");
        $('#username').sendKeys(testAccounts.admin.username);
        $('#password').sendKeys(testAccounts.admin.password);
        $('button[type="submit"]').click();
    },

    signOut: function(){
        var until = protractor.ExpectedConditions;
        browser.wait(until.presenceOf($('#account-dropdown')), 10000, "Can't find account dropdown");
        $('#account-dropdown').click();
        browser.wait(until.presenceOf($('#sign-out')), 10000, "Can't find sign out link");
        $('#sign-out').click();
    }
}