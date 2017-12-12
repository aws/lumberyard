var session = require('./session-helper.js');

var num_messages = 7;

/**
 * Message of the day integration tests
 * For this to run correctly delete all your messages from Active and Planned sections
 */
describe('Authentication without a bootstrap', function () {
    var until = protractor.ExpectedConditions;
    beforeAll(function() {
        browser.get('http://localhost:3000');        
    }); 

    afterAll(function() {
        
    });
    describe('integration tests', function() {
        it('should display a informational bootstrap modal', function() {
            browser.wait(until.presenceOf($('div.modal-header h2')), 20000, "Can't find modal missing bootstrap header");            
            $('div.modal-header h2').getText().then(
                function(text){                                        
                    expect(text).toEqual("Missing Cloud Gem Portal bootstrap file.")
                    $('#modal-dismiss').click();
                },
                function(err){                    
                    console.log(err)
                }
                );
            
            
        });
    });
});