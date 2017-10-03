var session = require('./session-helper.js');

var num_messages = 7;

/**
 * Message of the day integration tests
 * For this to run correctly delete all your messages from Active and Planned sections
 */
describe('Authentication welcome page', function () {
    var until = protractor.ExpectedConditions;
    beforeAll(function() {
        browser.get('http://localhost:3000');        
    }); 

    afterAll(function() {
        
    });
    describe('integration tests', function() {
        it('should display a welcome modal', function() {
            browser.wait(until.presenceOf($('div.modal-header h2')), 20000, "Missing welcome modal header");            
            $('div.modal-header h2').getText().then(
                function(text){                                        
                    expect(text).toEqual("First time Cloud Gem Portal user")
                    expect(element(by.id('welcome-option1-img')).isDisplayed()).toEqual(true)
                    expect(element(by.id('welcome-option2-img')).isDisplayed()).toEqual(true)
                    $('#modal-dismiss').click();
                },
                function(err){                    
                    console.log(err)
                }
                );    
        });
    });
});