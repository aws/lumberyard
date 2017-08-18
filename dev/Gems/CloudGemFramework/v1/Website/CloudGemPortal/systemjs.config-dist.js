(function (global) {
    var map = {
        'app': './dist/app',

        "rxjs": "node_modules/rxjs",
        "@angular": "node_modules/@angular",        
        "@ng-bootstrap/ng-bootstrap": "node_modules/@ng-bootstrap/ng-bootstrap/bundles/ng-bootstrap.js",
        "typescript": "node_modules/typescript/lib/typescript.js",
        "ts": "node_modules/plugin-typescript/lib/plugin.js",
        'dragula': 'node_modules/dragula/dist/dragula.js',
        'ng2-dragula': 'node_modules/ng2-dragula',
        'ts-clipboard': 'node_modules/ts-clipboard/ts-clipboard.js',
        'web-animations-js': 'node_modules/web-animations-js/web-animations.min.js'
    };

    System.config({
        map: map,
        transpiler: "ts",
        transcriptOptions: {
            "tsconfig": true
        },
        defaultJSExtensions: true,
        meta: {
            typescript: {
                exports: "ts"
            }
        },
        packages: {
            'app': {
                main: './main.js',                
                defaultExtension: 'js'
            },         
            'rxjs': {
                defaultExtension: 'js'
            },            
            '@angular/core': { main: 'index.js', defaultExtension: 'js' },
            '@angular/common': { main: 'index.js', defaultExtension: 'js' },
            '@angular/compiler': { main: 'index.js', defaultExtension: 'js' },
            '@angular/platform-browser': { main: 'index.js', defaultExtension: 'js' },
            '@angular/platform-browser-dynamic': { main: 'index.js', defaultExtension: 'js' },
            '@angular/http': { main: 'index.js', defaultExtension: 'js' },
            '@angular/router': { main: 'index.js', defaultExtension: 'js' },
            '@angular/forms': { main: 'index.js', defaultExtension: 'js' }
        }
    });
})(this);
