function wait(time) {
    return new Promise(function (resolve, reject) {
        var req = setTimeout(function () { resolve(); }, time);
    });
}


exports.config = {
    specs: ['./e2e/**/*.e2e-spec.js'],
    seleniumServerJar: '../e2e/driver/selenium-server-standalone-3.6.0.jar',        
    //multiCapabilities: [
    //    { 'browserName': 'firefox' }, 
    //    { 'browserName': 'chrome' }, 
    //    { 'browserName': 'MicrosoftEdge' }
    //],   
    chromeOnly: true,
    capabilities: {
        'browserName': 'chrome',
        'chromeOptions': {
            //'args': ["--headless", "--disable-gpu", "--window-size=1280x800",  "--no-sandbox"]
            //'args': ['show-fps-counter=true']
            'args': [
                '--no-sandbox',
                "--test-type",
                '--disable-infobars',
                '--disable-extensions',
                'verbose'                
            ]
        },
        'prefs': {            
            'profile.password_manager_enabled': false,
            'credentials_enable_service': false,
            'password_manager_enabled': false
        },
        'loggingPrefs': {
            'browser': "ALL"
        },
    },
    framework: 'jasmine2',          
    baseUrl: '',
    
    getPageTimeout: 40000,
    allScriptsTimeout: 30000,
    jasmineNodeOpts: {
        showTiming: true,
        showColors: true,
        isVerbose: true,
        includeStackTrace: true,
        defaultTimeoutInterval: 400000,
        displaySpecDuration: true
    },
   // directConnect: true,
    onPrepare: function () {        
        global.until = protractor.ExpectedConditions;
        browser.driver.manage().window().maximize();        
        browser.ignoreSynchronization = true;        
    },
    //Enabling these will break the test framework
    //webDriverLogDir: 'f:\\',
    //highlightDelay: 2000,
/**
   * Angular 2 configuration
   *
   * useAllAngular2AppRoots: tells Protractor to wait for any angular2 apps on the page instead of just the one matching
   * `rootEl`
   */
    useAllAngular2AppRoots: true,
    SELENIUM_PROMISE_MANAGER: true,
    troubleshoot: true,
    //debug: true
};