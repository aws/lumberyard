exports.config = {
    specs: ['./e2e/**/*.e2e-spec.js'],
    seleniumServerJar: '../node_modules/selenium-server-standalone-jar/jar/selenium-server-standalone-3.4.0.jar',    
    multiCapabilities: [
        // { 'browserName': 'firefox' }, 
        { 'browserName': 'chrome' }, 
        // { 'browserName': 'MicrosoftEdge' }
    ],
    framework: 'jasmine',
    getPageTimeout: 120000,
    // useAllAngular2AppRoots: true,
    onPrepare: function () {
        browser.ignoreSynchronization = true;
    }
};