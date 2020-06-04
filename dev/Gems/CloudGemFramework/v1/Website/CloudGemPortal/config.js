System.config({
  baseURL: ".",
  defaultJSExtensions: true,
  transpiler: "ts",
  typescriptOptions: {
    "tsconfig": true
  },
  paths: {
    "github:*": "jspm_packages/github/*",
    "npm:*": "jspm_packages/npm/*",
    "app": "./app",
    "nouislider": "node_modules/nouislider/distribute/nouislider.js",
    "ng2-nouislider": "node_modules/ng2-nouislider/src/nouislider.js"
  },
  rootDir: "./",

  packages: {
    "app": {
      "defaultExtension": "ts",
      "main": "./main.ts",
      "meta": {
        "*.ts": {
          "loader": "ts"
        },
        "*.html": {
          "loader": "text"
        },
        "*.scss": {
          "loader": "scss"
        }
      }
    }
  },

  meta: {
    "nouislider": {
      "exports": "nouislider",
      "format": "cjs"
    },
    "ng2-nouislider": {
      "exports": "ng2-nouislider",
      "format": "cjs"
    }
  },

  map: {
    "@angular/animations": "npm:@angular/animations@4.4.4",
    "@angular/animations/browser": "npm:@angular/animations@4.4.4/bundles/animations-browser.umd.js",
    "@angular/common": "npm:@angular/common@4.4.4",
    "@angular/compiler": "npm:@angular/compiler@4.4.4",
    "@angular/core": "npm:@angular/core@4.4.4",
    "@angular/forms": "npm:@angular/forms@4.4.4",
    "@angular/http": "npm:@angular/http@4.4.4",
    "@angular/platform-browser": "npm:@angular/platform-browser@4.4.4",
    "@angular/platform-browser-dynamic": "npm:@angular/platform-browser-dynamic@4.4.4",
    "@angular/platform-browser/animations": "npm:@angular/platform-browser@4.4.4/bundles/platform-browser-animations.umd.js",
    "@angular/router": "npm:@angular/router@4.4.4",
    "@asymmetrik/ngx-leaflet": "npm:@asymmetrik/ngx-leaflet@2.5.3",
    "@ng-bootstrap/ng-bootstrap": "npm:@ng-bootstrap/ng-bootstrap@1.0.0-beta.5",
    "@swimlane/ngx-charts": "npm:@swimlane/ngx-charts@6.1.0",
    "@swimlane/ngx-datatable": "npm:@swimlane/ngx-datatable@10.2.3",
    "@yaga/leaflet-ng2": "npm:@yaga/leaflet-ng2@1.0.0-rc8",
    "ajv": "npm:ajv@6.4.0",
    "angular-datamaps": "npm:angular-datamaps@0.1.2",
    "capaj/systemjs-hot-reloader": "github:alexisvincent/systemjs-hot-reloader@0.5.9",
    "core-js": "npm:core-js@2.5.1",
    "d3": "npm:d3@4.10.0",
    "d3-array": "npm:d3-array@1.2.1",
    "d3-brush": "npm:d3-brush@1.0.4",
    "d3-color": "npm:d3-color@1.0.3",
    "d3-force": "npm:d3-force@1.1.0",
    "d3-format": "npm:d3-format@1.2.0",
    "d3-hierarchy": "npm:d3-hierarchy@1.1.5",
    "d3-interpolate": "npm:d3-interpolate@1.1.5",
    "d3-scale": "npm:d3-scale@1.0.6",
    "d3-selection": "npm:d3-selection@1.1.0",
    "d3-shape": "npm:d3-shape@1.2.0",
    "d3-svg-legend": "npm:d3-svg-legend@2.24.1",
    "d3-time-format": "npm:d3-time-format@2.1.0",
    "d3-umd": "npm:d3@4.11.0",
    "dragula": "npm:dragula@3.7.2",
    "find-process": "npm:find-process@1.4.3",
    "gulp-open": "npm:gulp-open@3.0.1",
    "html": "github:Hypercubed/systemjs-plugin-html@0.0.8",
    "leaflet": "github:Leaflet/Leaflet@1.2.0",
    "leaflet-easyprint": "npm:leaflet-easyprint@2.1.9",
    "leaflet.heat": "npm:leaflet.heat@0.2.0",
    "module": "npm:jspm-nodelibs-module@0.2.1",
    "net": "npm:jspm-nodelibs-net@0.2.1",
    "ng2-dragula": "npm:ng2-dragula@1.5.0",
    "ng2-mqtt": "npm:ng2-mqtt@0.1.2",
    "ng2-slider-component": "npm:ng2-slider-component@1.0.9",
    "ng2-toastr": "npm:ng2-toastr@4.1.2",
    "ngx-datatable": "npm:ngx-datatable@1.0.3",
    "protractor": "npm:protractor@5.4.4",
    "reflect-metadata": "npm:reflect-metadata@0.1.10",
    "rxjs": "npm:rxjs@5.5.6",
    "scss": "github:KevCJones/plugin-scss@0.2.11",
    "systemjs-hot-reloader": "npm:systemjs-hot-reloader@1.1.0",
    "text": "github:systemjs/plugin-text@0.0.11",
    "through2": "npm:through2@2.0.5",
    "ts": "github:frankwallis/plugin-typescript@7.0.6",
    "ts-clipboard": "npm:ts-clipboard@1.0.13",
    "ts-md5": "npm:ts-md5@1.2.2",
    "ts-runtime": "npm:babel-runtime@5.8.38",
    "typescript": "npm:typescript@2.5.3",
    "validate-typescript": "npm:validate-typescript@2.0.1",
    "validator": "npm:validator@9.4.1",
    "validator.ts": "npm:validator.ts@0.2.2",
    "victor": "npm:victor@1.1.0",
    "zone.js": "npm:zone.js@0.8.18",
    "github:Hypercubed/systemjs-plugin-html@0.0.8": {
      "webcomponentsjs": "github:webcomponents/webcomponentsjs@0.7.24"
    },
    "github:KevCJones/plugin-scss@0.2.11": {
      "autoprefixer": "npm:autoprefixer@6.7.7",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "lodash": "npm:lodash@4.17.4",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "postcss": "npm:postcss@5.2.17",
      "reqwest": "github:ded/reqwest@2.0.5",
      "sass.js": "npm:sass.js@0.9.13",
      "url": "github:jspm/nodelibs-url@0.1.0"
    },
    "github:alexisvincent/systemjs-hot-reloader@0.5.9": {
      "debug": "npm:debug@2.6.9",
      "socket.io-client": "github:socketio/socket.io-client@1.7.4",
      "weakee": "npm:weakee@1.0.0"
    },
    "github:jspm/nodelibs-assert@0.1.0": {
      "assert": "npm:assert@1.5.0"
    },
    "github:jspm/nodelibs-buffer@0.1.1": {
      "buffer": "npm:buffer@5.6.0"
    },
    "github:jspm/nodelibs-constants@0.1.0": {
      "constants-browserify": "npm:constants-browserify@0.0.1"
    },
    "github:jspm/nodelibs-crypto@0.1.0": {
      "crypto-browserify": "npm:crypto-browserify@3.12.0"
    },
    "github:jspm/nodelibs-events@0.1.1": {
      "events": "npm:events@1.0.2"
    },
    "github:jspm/nodelibs-http@1.7.1": {
      "Base64": "npm:Base64@0.2.1",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "inherits": "npm:inherits@2.0.1",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "github:jspm/nodelibs-https@0.1.0": {
      "https-browserify": "npm:https-browserify@0.0.0"
    },
    "github:jspm/nodelibs-net@0.1.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "net": "github:jspm/nodelibs-net@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "timers": "github:jspm/nodelibs-timers@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "github:jspm/nodelibs-os@0.1.0": {
      "os-browserify": "npm:os-browserify@0.1.2"
    },
    "github:jspm/nodelibs-path@0.1.0": {
      "path-browserify": "npm:path-browserify@0.0.0"
    },
    "github:jspm/nodelibs-process@0.1.2": {
      "process": "npm:process@0.11.10"
    },
    "github:jspm/nodelibs-punycode@0.1.0": {
      "punycode": "npm:punycode@1.3.2"
    },
    "github:jspm/nodelibs-querystring@0.1.0": {
      "querystring": "npm:querystring@0.2.0"
    },
    "github:jspm/nodelibs-stream@0.1.0": {
      "stream-browserify": "npm:stream-browserify@1.0.0"
    },
    "github:jspm/nodelibs-string_decoder@0.1.0": {
      "string_decoder": "npm:string_decoder@0.10.31"
    },
    "github:jspm/nodelibs-timers@0.1.0": {
      "timers-browserify": "npm:timers-browserify@1.4.2"
    },
    "github:jspm/nodelibs-tty@0.1.0": {
      "tty-browserify": "npm:tty-browserify@0.0.0"
    },
    "github:jspm/nodelibs-url@0.1.0": {
      "url": "npm:url@0.10.3"
    },
    "github:jspm/nodelibs-util@0.1.0": {
      "util": "npm:util@0.10.3"
    },
    "github:jspm/nodelibs-vm@0.1.0": {
      "vm-browserify": "npm:vm-browserify@0.0.4"
    },
    "github:jspm/nodelibs-zlib@0.1.0": {
      "browserify-zlib": "npm:browserify-zlib@0.1.4"
    },
    "npm:@angular/animations@4.4.4": {
      "@angular/core": "npm:@angular/core@4.4.4",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.8.1"
    },
    "npm:@angular/common@2.4.10": {
      "@angular/core": "npm:@angular/core@2.4.10"
    },
    "npm:@angular/common@4.4.4": {
      "@angular/core": "npm:@angular/core@4.4.4",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.8.1"
    },
    "npm:@angular/compiler@2.4.10": {
      "@angular/core": "npm:@angular/core@2.4.10",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:@angular/compiler@4.4.4": {
      "@angular/core": "npm:@angular/core@4.4.4",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.8.1"
    },
    "npm:@angular/core@2.4.10": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "rxjs": "npm:rxjs@5.5.6",
      "zone.js": "npm:zone.js@0.7.8"
    },
    "npm:@angular/core@4.4.4": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "rxjs": "npm:rxjs@5.5.6",
      "tslib": "npm:tslib@1.8.1",
      "zone.js": "npm:zone.js@0.8.18"
    },
    "npm:@angular/forms@2.4.10": {
      "@angular/common": "npm:@angular/common@2.4.10",
      "@angular/core": "npm:@angular/core@2.4.10",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:@angular/forms@4.4.4": {
      "@angular/common": "npm:@angular/common@4.4.4",
      "@angular/core": "npm:@angular/core@4.4.4",
      "@angular/platform-browser": "npm:@angular/platform-browser@4.4.4",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.8.1"
    },
    "npm:@angular/http@4.4.4": {
      "@angular/core": "npm:@angular/core@4.4.4",
      "@angular/platform-browser": "npm:@angular/platform-browser@4.4.4",
      "rxjs": "npm:rxjs@5.5.6",
      "tslib": "npm:tslib@1.8.1"
    },
    "npm:@angular/platform-browser-dynamic@4.4.4": {
      "@angular/common": "npm:@angular/common@4.4.4",
      "@angular/compiler": "npm:@angular/compiler@4.4.4",
      "@angular/core": "npm:@angular/core@4.4.4",
      "@angular/platform-browser": "npm:@angular/platform-browser@4.4.4",
      "tslib": "npm:tslib@1.8.1"
    },
    "npm:@angular/platform-browser@4.4.4": {
      "@angular/common": "npm:@angular/common@4.4.4",
      "@angular/core": "npm:@angular/core@4.4.4",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.8.1"
    },
    "npm:@angular/router@4.4.4": {
      "@angular/common": "npm:@angular/common@4.4.4",
      "@angular/core": "npm:@angular/core@4.4.4",
      "@angular/platform-browser": "npm:@angular/platform-browser@4.4.4",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "rxjs": "npm:rxjs@5.5.6",
      "tslib": "npm:tslib@1.8.1"
    },
    "npm:@asymmetrik/ngx-leaflet@2.5.3": {
      "@angular/common": "npm:@angular/common@4.4.4",
      "@angular/core": "npm:@angular/core@4.4.4",
      "leaflet": "npm:leaflet@1.2.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:@ng-bootstrap/ng-bootstrap@1.0.0-beta.5": {
      "@angular/common": "npm:@angular/common@4.4.4",
      "@angular/core": "npm:@angular/core@4.4.4",
      "@angular/forms": "npm:@angular/forms@4.4.4",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:@swimlane/ngx-charts@6.1.0": {
      "@angular/common": "npm:@angular/common@4.4.4",
      "@angular/core": "npm:@angular/core@4.4.4",
      "d3": "npm:d3@4.11.0",
      "rxjs": "npm:rxjs@5.5.6"
    },
    "npm:@swimlane/ngx-datatable@10.2.3": {
      "@angular/common": "npm:@angular/common@4.4.4",
      "@angular/compiler": "npm:@angular/compiler@4.4.4",
      "@angular/core": "npm:@angular/core@4.4.4",
      "@angular/platform-browser": "npm:@angular/platform-browser@4.4.4",
      "@angular/platform-browser-dynamic": "npm:@angular/platform-browser-dynamic@4.4.4",
      "core-js": "npm:core-js@2.5.1",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "rxjs": "npm:rxjs@5.5.6",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:@types/leaflet@1.2.3": {
      "@types/geojson": "npm:@types/geojson@0.0.32"
    },
    "npm:@yaga/generic-geojson@1.0.0": {
      "@types/geojson": "npm:@types/geojson@0.0.32"
    },
    "npm:@yaga/leaflet-ng2@1.0.0-rc8": {
      "@angular/core": "npm:@angular/core@4.4.4",
      "@types/leaflet": "npm:@types/leaflet@1.2.3",
      "@yaga/generic-geojson": "npm:@yaga/generic-geojson@1.0.0",
      "leaflet": "npm:leaflet@1.2.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:adm-zip@0.4.14": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "zlib": "github:jspm/nodelibs-zlib@0.1.0"
    },
    "npm:agent-base@4.3.0": {
      "es6-promisify": "npm:es6-promisify@5.0.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "https": "github:jspm/nodelibs-https@0.1.0",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:ajv@6.12.2": {
      "fast-deep-equal": "npm:fast-deep-equal@3.1.1",
      "fast-json-stable-stringify": "npm:fast-json-stable-stringify@2.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "json-schema-traverse": "npm:json-schema-traverse@0.4.1",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2",
      "uri-js": "npm:uri-js@4.2.2"
    },
    "npm:ajv@6.4.0": {
      "fast-deep-equal": "npm:fast-deep-equal@1.0.0",
      "fast-json-stable-stringify": "npm:fast-json-stable-stringify@2.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "json-schema-traverse": "npm:json-schema-traverse@0.3.1",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2",
      "uri-js": "npm:uri-js@3.0.2"
    },
    "npm:ansi-styles@3.2.1": {
      "color-convert": "npm:color-convert@1.9.3"
    },
    "npm:array-union@1.0.2": {
      "array-uniq": "npm:array-uniq@1.0.3"
    },
    "npm:asn1.js@4.10.1": {
      "bn.js": "npm:bn.js@4.11.9",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "inherits": "npm:inherits@2.0.4",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.1",
      "vm": "github:jspm/nodelibs-vm@0.1.0"
    },
    "npm:asn1@0.2.4": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "safer-buffer": "npm:safer-buffer@2.1.2"
    },
    "npm:assert-plus@1.0.0": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:assert@1.5.0": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "object-assign": "npm:object-assign@4.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "npm:util@0.10.3"
    },
    "npm:asynckit@0.4.0": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:autoprefixer@6.7.7": {
      "browserslist": "npm:browserslist@1.7.7",
      "caniuse-db": "npm:caniuse-db@1.0.30000706",
      "normalize-range": "npm:normalize-range@0.1.2",
      "num2fraction": "npm:num2fraction@1.2.2",
      "postcss": "npm:postcss@5.2.17",
      "postcss-value-parser": "npm:postcss-value-parser@3.3.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:aws-sign2@0.7.0": {
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "url": "github:jspm/nodelibs-url@0.1.0"
    },
    "npm:aws4@1.9.1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "querystring": "github:jspm/nodelibs-querystring@0.1.0",
      "url": "github:jspm/nodelibs-url@0.1.0"
    },
    "npm:babel-runtime@5.8.38": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:bcrypt-pbkdf@1.0.2": {
      "tweetnacl": "npm:tweetnacl@0.14.5"
    },
    "npm:blocking-proxy@1.0.1": {
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "minimist": "npm:minimist@1.2.5",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "url": "github:jspm/nodelibs-url@0.1.0"
    },
    "npm:brace-expansion@1.1.11": {
      "balanced-match": "npm:balanced-match@1.0.0",
      "concat-map": "npm:concat-map@0.0.1"
    },
    "npm:browserify-aes@1.2.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "buffer-xor": "npm:buffer-xor@1.0.3",
      "cipher-base": "npm:cipher-base@1.0.4",
      "create-hash": "npm:create-hash@1.2.0",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "evp_bytestokey": "npm:evp_bytestokey@1.0.3",
      "inherits": "npm:inherits@2.0.4",
      "safe-buffer": "npm:safe-buffer@5.2.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:browserify-cipher@1.0.1": {
      "browserify-aes": "npm:browserify-aes@1.2.0",
      "browserify-des": "npm:browserify-des@1.0.2",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "evp_bytestokey": "npm:evp_bytestokey@1.0.3"
    },
    "npm:browserify-des@1.0.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "cipher-base": "npm:cipher-base@1.0.4",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "des.js": "npm:des.js@1.0.1",
      "inherits": "npm:inherits@2.0.4",
      "safe-buffer": "npm:safe-buffer@5.2.1"
    },
    "npm:browserify-rsa@4.0.1": {
      "bn.js": "npm:bn.js@4.11.9",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "constants": "github:jspm/nodelibs-constants@0.1.0",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "randombytes": "npm:randombytes@2.1.0"
    },
    "npm:browserify-sign@4.2.0": {
      "bn.js": "npm:bn.js@5.1.2",
      "browserify-rsa": "npm:browserify-rsa@4.0.1",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "create-hash": "npm:create-hash@1.2.0",
      "create-hmac": "npm:create-hmac@1.1.7",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "elliptic": "npm:elliptic@6.5.2",
      "inherits": "npm:inherits@2.0.4",
      "parse-asn1": "npm:parse-asn1@5.1.5",
      "readable-stream": "npm:readable-stream@3.6.0",
      "safe-buffer": "npm:safe-buffer@5.2.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:browserify-zlib@0.1.4": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "pako": "npm:pako@0.2.9",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "readable-stream": "npm:readable-stream@2.3.7",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:browserslist@1.7.7": {
      "caniuse-db": "npm:caniuse-db@1.0.30000706",
      "electron-to-chromium": "npm:electron-to-chromium@1.3.16",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:browserstack@1.6.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "https": "github:jspm/nodelibs-https@0.1.0",
      "https-proxy-agent": "npm:https-proxy-agent@2.2.4",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "querystring": "github:jspm/nodelibs-querystring@0.1.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:buffer-xor@1.0.3": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:buffer@5.6.0": {
      "base64-js": "npm:base64-js@1.3.1",
      "ieee754": "npm:ieee754@1.1.13"
    },
    "npm:camelcase-keys@2.1.0": {
      "camelcase": "npm:camelcase@2.1.1",
      "map-obj": "npm:map-obj@1.0.1"
    },
    "npm:chalk@1.1.3": {
      "ansi-styles": "npm:ansi-styles@2.2.1",
      "escape-string-regexp": "npm:escape-string-regexp@1.0.5",
      "has-ansi": "npm:has-ansi@2.0.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "strip-ansi": "npm:strip-ansi@3.0.1",
      "supports-color": "npm:supports-color@2.0.0"
    },
    "npm:chalk@2.4.2": {
      "ansi-styles": "npm:ansi-styles@3.2.1",
      "escape-string-regexp": "npm:escape-string-regexp@1.0.5",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "supports-color": "npm:supports-color@5.5.0"
    },
    "npm:cipher-base@1.0.4": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "inherits": "npm:inherits@2.0.4",
      "safe-buffer": "npm:safe-buffer@5.2.1",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "string_decoder": "github:jspm/nodelibs-string_decoder@0.1.0"
    },
    "npm:cliui@4.1.0": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "string-width": "npm:string-width@2.1.1",
      "strip-ansi": "npm:strip-ansi@4.0.0",
      "wrap-ansi": "npm:wrap-ansi@2.1.0"
    },
    "npm:color-convert@1.9.3": {
      "color-name": "npm:color-name@1.1.3"
    },
    "npm:color-name@1.1.3": {
      "assert": "github:jspm/nodelibs-assert@0.1.0"
    },
    "npm:colors@1.4.0": {
      "os": "github:jspm/nodelibs-os@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:combined-stream@1.0.8": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "delayed-stream": "npm:delayed-stream@1.0.0",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:commander@2.20.3": {
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:constants-browserify@0.0.1": {
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:contra@1.9.4": {
      "atoa": "npm:atoa@1.0.0",
      "ticky": "npm:ticky@1.0.1"
    },
    "npm:core-js@2.5.1": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:core-util-is@1.0.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1"
    },
    "npm:create-ecdh@4.0.3": {
      "bn.js": "npm:bn.js@4.11.9",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "elliptic": "npm:elliptic@6.5.2"
    },
    "npm:create-hash@1.2.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "cipher-base": "npm:cipher-base@1.0.4",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "inherits": "npm:inherits@2.0.4",
      "md5.js": "npm:md5.js@1.3.5",
      "ripemd160": "npm:ripemd160@2.0.2",
      "sha.js": "npm:sha.js@2.4.11"
    },
    "npm:create-hmac@1.1.7": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "cipher-base": "npm:cipher-base@1.0.4",
      "create-hash": "npm:create-hash@1.2.0",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "inherits": "npm:inherits@2.0.4",
      "ripemd160": "npm:ripemd160@2.0.2",
      "safe-buffer": "npm:safe-buffer@5.2.1",
      "sha.js": "npm:sha.js@2.4.11"
    },
    "npm:cross-spawn@6.0.5": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "nice-try": "npm:nice-try@1.0.5",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "path-key": "npm:path-key@2.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "semver": "npm:semver@5.7.1",
      "shebang-command": "npm:shebang-command@1.2.0",
      "which": "npm:which@1.3.1"
    },
    "npm:crossvent@1.5.4": {
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "custom-event": "npm:custom-event@1.0.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:crypto-browserify@3.12.0": {
      "browserify-cipher": "npm:browserify-cipher@1.0.1",
      "browserify-sign": "npm:browserify-sign@4.2.0",
      "create-ecdh": "npm:create-ecdh@4.0.3",
      "create-hash": "npm:create-hash@1.2.0",
      "create-hmac": "npm:create-hmac@1.1.7",
      "diffie-hellman": "npm:diffie-hellman@5.0.3",
      "inherits": "npm:inherits@2.0.4",
      "pbkdf2": "npm:pbkdf2@3.0.17",
      "public-encrypt": "npm:public-encrypt@4.0.3",
      "randombytes": "npm:randombytes@2.1.0",
      "randomfill": "npm:randomfill@1.0.4"
    },
    "npm:currently-unhandled@0.4.1": {
      "array-find-index": "npm:array-find-index@1.0.2",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:d3-brush@1.0.4": {
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-drag": "npm:d3-drag@1.1.1",
      "d3-interpolate": "npm:d3-interpolate@1.1.5",
      "d3-selection": "npm:d3-selection@1.1.0",
      "d3-transition": "npm:d3-transition@1.1.0"
    },
    "npm:d3-chord@1.0.4": {
      "d3-array": "npm:d3-array@1.2.0",
      "d3-path": "npm:d3-path@1.0.5"
    },
    "npm:d3-drag@1.1.1": {
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-selection": "npm:d3-selection@1.1.0"
    },
    "npm:d3-drag@1.2.1": {
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-selection": "npm:d3-selection@1.1.0"
    },
    "npm:d3-dsv@1.0.5": {
      "commander": "npm:commander@2.20.3",
      "iconv-lite": "npm:iconv-lite@0.4.24",
      "rw": "npm:rw@1.3.3"
    },
    "npm:d3-dsv@1.0.7": {
      "commander": "npm:commander@2.20.3",
      "iconv-lite": "npm:iconv-lite@0.4.24",
      "rw": "npm:rw@1.3.3"
    },
    "npm:d3-force@1.0.6": {
      "d3-collection": "npm:d3-collection@1.0.4",
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-quadtree": "npm:d3-quadtree@1.0.3",
      "d3-timer": "npm:d3-timer@1.0.6",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:d3-force@1.1.0": {
      "d3-collection": "npm:d3-collection@1.0.4",
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-quadtree": "npm:d3-quadtree@1.0.3",
      "d3-timer": "npm:d3-timer@1.0.7",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:d3-geo@1.6.4": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "d3-array": "npm:d3-array@1.2.0"
    },
    "npm:d3-geo@1.8.1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "d3-array": "npm:d3-array@1.2.1"
    },
    "npm:d3-interpolate@1.1.5": {
      "d3-color": "npm:d3-color@1.0.3"
    },
    "npm:d3-request@1.0.5": {
      "d3-collection": "npm:d3-collection@1.0.4",
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-dsv": "npm:d3-dsv@1.0.5",
      "xmlhttprequest": "npm:xmlhttprequest@1.8.0"
    },
    "npm:d3-request@1.0.6": {
      "d3-collection": "npm:d3-collection@1.0.4",
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-dsv": "npm:d3-dsv@1.0.7",
      "xmlhttprequest": "npm:xmlhttprequest@1.8.0"
    },
    "npm:d3-scale@1.0.3": {
      "d3-array": "npm:d3-array@1.2.0",
      "d3-collection": "npm:d3-collection@1.0.4",
      "d3-color": "npm:d3-color@1.0.3",
      "d3-format": "npm:d3-format@1.2.0",
      "d3-interpolate": "npm:d3-interpolate@1.1.5",
      "d3-time": "npm:d3-time@1.0.7",
      "d3-time-format": "npm:d3-time-format@2.0.5"
    },
    "npm:d3-scale@1.0.6": {
      "d3-array": "npm:d3-array@1.2.0",
      "d3-collection": "npm:d3-collection@1.0.4",
      "d3-color": "npm:d3-color@1.0.3",
      "d3-format": "npm:d3-format@1.2.0",
      "d3-interpolate": "npm:d3-interpolate@1.1.5",
      "d3-time": "npm:d3-time@1.0.7",
      "d3-time-format": "npm:d3-time-format@2.0.5"
    },
    "npm:d3-shape@1.2.0": {
      "d3-path": "npm:d3-path@1.0.5"
    },
    "npm:d3-svg-legend@2.24.1": {
      "@types/d3-selection": "npm:@types/d3-selection@1.0.10",
      "d3-array": "npm:d3-array@1.0.1",
      "d3-dispatch": "npm:d3-dispatch@1.0.1",
      "d3-format": "npm:d3-format@1.0.2",
      "d3-scale": "npm:d3-scale@1.0.3",
      "d3-selection": "npm:d3-selection@1.0.2",
      "d3-transition": "npm:d3-transition@1.0.3"
    },
    "npm:d3-time-format@2.0.5": {
      "d3-time": "npm:d3-time@1.0.7"
    },
    "npm:d3-time-format@2.1.0": {
      "d3-time": "npm:d3-time@1.0.7"
    },
    "npm:d3-transition@1.0.3": {
      "d3-color": "npm:d3-color@1.0.3",
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-ease": "npm:d3-ease@1.0.3",
      "d3-interpolate": "npm:d3-interpolate@1.1.5",
      "d3-selection": "npm:d3-selection@1.1.0",
      "d3-timer": "npm:d3-timer@1.0.6"
    },
    "npm:d3-transition@1.1.0": {
      "d3-color": "npm:d3-color@1.0.3",
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-ease": "npm:d3-ease@1.0.3",
      "d3-interpolate": "npm:d3-interpolate@1.1.5",
      "d3-selection": "npm:d3-selection@1.1.0",
      "d3-timer": "npm:d3-timer@1.0.6"
    },
    "npm:d3-zoom@1.5.0": {
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-drag": "npm:d3-drag@1.1.1",
      "d3-interpolate": "npm:d3-interpolate@1.1.5",
      "d3-selection": "npm:d3-selection@1.1.0",
      "d3-transition": "npm:d3-transition@1.1.0"
    },
    "npm:d3-zoom@1.6.0": {
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-drag": "npm:d3-drag@1.2.1",
      "d3-interpolate": "npm:d3-interpolate@1.1.5",
      "d3-selection": "npm:d3-selection@1.1.0",
      "d3-transition": "npm:d3-transition@1.1.0"
    },
    "npm:d3@4.10.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "d3-array": "npm:d3-array@1.2.0",
      "d3-axis": "npm:d3-axis@1.0.8",
      "d3-brush": "npm:d3-brush@1.0.4",
      "d3-chord": "npm:d3-chord@1.0.4",
      "d3-collection": "npm:d3-collection@1.0.4",
      "d3-color": "npm:d3-color@1.0.3",
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-drag": "npm:d3-drag@1.1.1",
      "d3-dsv": "npm:d3-dsv@1.0.5",
      "d3-ease": "npm:d3-ease@1.0.3",
      "d3-force": "npm:d3-force@1.0.6",
      "d3-format": "npm:d3-format@1.2.0",
      "d3-geo": "npm:d3-geo@1.6.4",
      "d3-hierarchy": "npm:d3-hierarchy@1.1.5",
      "d3-interpolate": "npm:d3-interpolate@1.1.5",
      "d3-path": "npm:d3-path@1.0.5",
      "d3-polygon": "npm:d3-polygon@1.0.3",
      "d3-quadtree": "npm:d3-quadtree@1.0.3",
      "d3-queue": "npm:d3-queue@3.0.7",
      "d3-random": "npm:d3-random@1.1.0",
      "d3-request": "npm:d3-request@1.0.5",
      "d3-scale": "npm:d3-scale@1.0.6",
      "d3-selection": "npm:d3-selection@1.1.0",
      "d3-shape": "npm:d3-shape@1.2.0",
      "d3-time": "npm:d3-time@1.0.7",
      "d3-time-format": "npm:d3-time-format@2.0.5",
      "d3-timer": "npm:d3-timer@1.0.6",
      "d3-transition": "npm:d3-transition@1.1.0",
      "d3-voronoi": "npm:d3-voronoi@1.1.2",
      "d3-zoom": "npm:d3-zoom@1.5.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:d3@4.11.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "d3-array": "npm:d3-array@1.2.1",
      "d3-axis": "npm:d3-axis@1.0.8",
      "d3-brush": "npm:d3-brush@1.0.4",
      "d3-chord": "npm:d3-chord@1.0.4",
      "d3-collection": "npm:d3-collection@1.0.4",
      "d3-color": "npm:d3-color@1.0.3",
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-drag": "npm:d3-drag@1.2.1",
      "d3-dsv": "npm:d3-dsv@1.0.7",
      "d3-ease": "npm:d3-ease@1.0.3",
      "d3-force": "npm:d3-force@1.1.0",
      "d3-format": "npm:d3-format@1.2.0",
      "d3-geo": "npm:d3-geo@1.8.1",
      "d3-hierarchy": "npm:d3-hierarchy@1.1.5",
      "d3-interpolate": "npm:d3-interpolate@1.1.5",
      "d3-path": "npm:d3-path@1.0.5",
      "d3-polygon": "npm:d3-polygon@1.0.3",
      "d3-quadtree": "npm:d3-quadtree@1.0.3",
      "d3-queue": "npm:d3-queue@3.0.7",
      "d3-random": "npm:d3-random@1.1.0",
      "d3-request": "npm:d3-request@1.0.6",
      "d3-scale": "npm:d3-scale@1.0.6",
      "d3-selection": "npm:d3-selection@1.1.0",
      "d3-shape": "npm:d3-shape@1.2.0",
      "d3-time": "npm:d3-time@1.0.7",
      "d3-time-format": "npm:d3-time-format@2.0.5",
      "d3-timer": "npm:d3-timer@1.0.7",
      "d3-transition": "npm:d3-transition@1.1.0",
      "d3-voronoi": "npm:d3-voronoi@1.1.2",
      "d3-zoom": "npm:d3-zoom@1.6.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:dashdash@1.14.1": {
      "assert-plus": "npm:assert-plus@1.0.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:dateformat@1.0.12": {
      "get-stdin": "npm:get-stdin@4.0.1",
      "meow": "npm:meow@3.7.0"
    },
    "npm:debug@2.6.9": {
      "ms": "npm:ms@2.0.0"
    },
    "npm:debug@3.2.6": {
      "ms": "npm:ms@2.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tty": "github:jspm/nodelibs-tty@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:del@2.2.2": {
      "globby": "npm:globby@5.0.0",
      "is-path-cwd": "npm:is-path-cwd@1.0.0",
      "is-path-in-cwd": "npm:is-path-in-cwd@1.0.1",
      "object-assign": "npm:object-assign@4.1.1",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "pify": "npm:pify@2.3.0",
      "pinkie-promise": "npm:pinkie-promise@2.0.1",
      "rimraf": "npm:rimraf@2.7.1"
    },
    "npm:delayed-stream@1.0.0": {
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:des.js@1.0.1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "inherits": "npm:inherits@2.0.4",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.1"
    },
    "npm:diffie-hellman@5.0.3": {
      "bn.js": "npm:bn.js@4.11.9",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "miller-rabin": "npm:miller-rabin@4.0.1",
      "randombytes": "npm:randombytes@2.1.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:dom-to-image@2.6.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:dragula@3.7.2": {
      "contra": "npm:contra@1.9.4",
      "crossvent": "npm:crossvent@1.5.4"
    },
    "npm:ecc-jsbn@0.1.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "jsbn": "npm:jsbn@0.1.1",
      "safer-buffer": "npm:safer-buffer@2.1.2"
    },
    "npm:elliptic@6.5.2": {
      "bn.js": "npm:bn.js@4.11.9",
      "brorand": "npm:brorand@1.1.0",
      "hash.js": "npm:hash.js@1.1.7",
      "hmac-drbg": "npm:hmac-drbg@1.0.1",
      "inherits": "npm:inherits@2.0.4",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.1",
      "minimalistic-crypto-utils": "npm:minimalistic-crypto-utils@1.0.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:end-of-stream@1.4.4": {
      "once": "npm:once@1.4.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:error-ex@1.3.2": {
      "is-arrayish": "npm:is-arrayish@0.2.1",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:es6-promise@4.2.8": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:es6-promisify@5.0.0": {
      "es6-promise": "npm:es6-promise@4.2.8"
    },
    "npm:evp_bytestokey@1.0.3": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "md5.js": "npm:md5.js@1.3.5",
      "safe-buffer": "npm:safe-buffer@5.2.1"
    },
    "npm:execa@1.0.0": {
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "cross-spawn": "npm:cross-spawn@6.0.5",
      "get-stream": "npm:get-stream@4.1.0",
      "is-stream": "npm:is-stream@1.1.0",
      "npm-run-path": "npm:npm-run-path@2.0.2",
      "p-finally": "npm:p-finally@1.0.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "signal-exit": "npm:signal-exit@3.0.3",
      "strip-eof": "npm:strip-eof@1.0.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:exit@0.1.2": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:extsprintf@1.3.0": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:fast-deep-equal@1.0.0": {
      "assert": "github:jspm/nodelibs-assert@0.1.0"
    },
    "npm:fast-json-stable-stringify@2.1.0": {
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:find-process@1.4.3": {
      "chalk": "npm:chalk@2.4.2",
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "commander": "npm:commander@2.20.3",
      "debug": "npm:debug@2.6.9",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:find-up@1.1.2": {
      "path": "github:jspm/nodelibs-path@0.1.0",
      "path-exists": "npm:path-exists@2.1.0",
      "pinkie-promise": "npm:pinkie-promise@2.0.1"
    },
    "npm:find-up@3.0.0": {
      "locate-path": "npm:locate-path@3.0.0",
      "path": "github:jspm/nodelibs-path@0.1.0"
    },
    "npm:forever-agent@0.6.1": {
      "http": "github:jspm/nodelibs-http@1.7.1",
      "https": "github:jspm/nodelibs-https@0.1.0",
      "net": "github:jspm/nodelibs-net@0.1.2",
      "tls": "github:jspm/nodelibs-tls@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:form-data@2.3.3": {
      "asynckit": "npm:asynckit@0.4.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "combined-stream": "npm:combined-stream@1.0.8",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "https": "github:jspm/nodelibs-https@0.1.0",
      "mime-types": "npm:mime-types@2.1.27",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:fs.realpath@1.0.0": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:get-stdin@4.0.1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:get-stream@4.1.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "pump": "npm:pump@3.0.0",
      "stream": "github:jspm/nodelibs-stream@0.1.0"
    },
    "npm:getpass@0.1.7": {
      "assert-plus": "npm:assert-plus@1.0.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tty": "github:jspm/nodelibs-tty@0.1.0"
    },
    "npm:glob@7.1.6": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "fs.realpath": "npm:fs.realpath@1.0.0",
      "inflight": "npm:inflight@1.0.6",
      "inherits": "npm:inherits@2.0.4",
      "minimatch": "npm:minimatch@3.0.4",
      "once": "npm:once@1.4.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "path-is-absolute": "npm:path-is-absolute@1.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:globby@5.0.0": {
      "array-union": "npm:array-union@1.0.2",
      "arrify": "npm:arrify@1.0.1",
      "glob": "npm:glob@7.1.6",
      "object-assign": "npm:object-assign@4.1.1",
      "pify": "npm:pify@2.3.0",
      "pinkie-promise": "npm:pinkie-promise@2.0.1"
    },
    "npm:graceful-fs@4.2.4": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "constants": "github:jspm/nodelibs-constants@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:gulp-open@3.0.1": {
      "colors": "npm:colors@1.4.0",
      "opn": "npm:opn@5.2.0",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "plugin-log": "npm:plugin-log@0.1.0",
      "through2": "npm:through2@2.0.5"
    },
    "npm:har-schema@2.0.0": {
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:har-validator@5.1.3": {
      "ajv": "npm:ajv@6.12.2",
      "har-schema": "npm:har-schema@2.0.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:has-ansi@2.0.0": {
      "ansi-regex": "npm:ansi-regex@2.1.1"
    },
    "npm:has-flag@1.0.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:has-flag@3.0.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:hash-base@3.1.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "inherits": "npm:inherits@2.0.4",
      "readable-stream": "npm:readable-stream@3.6.0",
      "safe-buffer": "npm:safe-buffer@5.2.1"
    },
    "npm:hash.js@1.1.7": {
      "inherits": "npm:inherits@2.0.4",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.1"
    },
    "npm:hmac-drbg@1.0.1": {
      "hash.js": "npm:hash.js@1.1.7",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.1",
      "minimalistic-crypto-utils": "npm:minimalistic-crypto-utils@1.0.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:hosted-git-info@2.8.8": {
      "url": "github:jspm/nodelibs-url@0.1.0"
    },
    "npm:http-signature@1.2.0": {
      "assert-plus": "npm:assert-plus@1.0.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "jsprim": "npm:jsprim@1.4.1",
      "sshpk": "npm:sshpk@1.16.1",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:https-browserify@0.0.0": {
      "http": "github:jspm/nodelibs-http@1.7.1"
    },
    "npm:https-proxy-agent@2.2.4": {
      "agent-base": "npm:agent-base@4.3.0",
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "debug": "npm:debug@3.2.6",
      "net": "github:jspm/nodelibs-net@0.1.2",
      "tls": "github:jspm/nodelibs-tls@0.1.0",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:iconv-lite@0.4.24": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "safer-buffer": "npm:safer-buffer@2.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "string_decoder": "github:jspm/nodelibs-string_decoder@0.1.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:immediate@3.0.6": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:indent-string@2.1.0": {
      "repeating": "npm:repeating@2.0.1"
    },
    "npm:inflight@1.0.6": {
      "once": "npm:once@1.4.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "wrappy": "npm:wrappy@1.0.2"
    },
    "npm:inherits@2.0.1": {
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:inherits@2.0.4": {
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:ini@1.3.5": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:is-fullwidth-code-point@1.0.0": {
      "number-is-nan": "npm:number-is-nan@1.0.1"
    },
    "npm:is-path-cwd@1.0.0": {
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:is-path-in-cwd@1.0.1": {
      "is-path-inside": "npm:is-path-inside@1.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:is-path-inside@1.0.1": {
      "path": "github:jspm/nodelibs-path@0.1.0",
      "path-is-inside": "npm:path-is-inside@1.0.2"
    },
    "npm:is-wsl@1.1.0": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:isarray@1.0.0": {
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:isexe@2.0.0": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:isstream@0.1.2": {
      "events": "github:jspm/nodelibs-events@0.1.1",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:jasmine-core@2.8.0": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:jasmine@2.8.0": {
      "exit": "npm:exit@0.1.2",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "glob": "npm:glob@7.1.6",
      "jasmine-core": "npm:jasmine-core@2.8.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:js-base64@2.1.9": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1"
    },
    "npm:json-schema-traverse@0.3.1": {
      "assert": "github:jspm/nodelibs-assert@0.1.0"
    },
    "npm:json-schema-traverse@0.4.1": {
      "assert": "github:jspm/nodelibs-assert@0.1.0"
    },
    "npm:jsprim@1.4.1": {
      "assert-plus": "npm:assert-plus@1.0.0",
      "extsprintf": "npm:extsprintf@1.3.0",
      "json-schema": "npm:json-schema@0.2.3",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0",
      "verror": "npm:verror@1.10.0"
    },
    "npm:jszip@3.4.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "lie": "npm:lie@3.3.0",
      "pako": "npm:pako@1.0.11",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "set-immediate-shim": "npm:set-immediate-shim@1.0.1",
      "stream": "github:jspm/nodelibs-stream@0.1.0"
    },
    "npm:lcid@2.0.0": {
      "invert-kv": "npm:invert-kv@2.0.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:leaflet-easyprint@2.1.9": {
      "dom-to-image": "npm:dom-to-image@2.6.0",
      "file-saver": "npm:file-saver@1.3.3"
    },
    "npm:leaflet.heat@0.2.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:lie@3.3.0": {
      "immediate": "npm:immediate@3.0.6",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:load-json-file@1.1.0": {
      "graceful-fs": "npm:graceful-fs@4.2.4",
      "parse-json": "npm:parse-json@2.2.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "pify": "npm:pify@2.3.0",
      "pinkie-promise": "npm:pinkie-promise@2.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "strip-bom": "npm:strip-bom@2.0.0"
    },
    "npm:locate-path@3.0.0": {
      "p-locate": "npm:p-locate@3.0.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "path-exists": "npm:path-exists@3.0.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:loud-rejection@1.6.0": {
      "currently-unhandled": "npm:currently-unhandled@0.4.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "signal-exit": "npm:signal-exit@3.0.3",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:map-age-cleaner@0.1.3": {
      "p-defer": "npm:p-defer@1.0.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:md5.js@1.3.5": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "hash-base": "npm:hash-base@3.1.0",
      "inherits": "npm:inherits@2.0.4",
      "safe-buffer": "npm:safe-buffer@5.2.1"
    },
    "npm:mem@4.3.0": {
      "map-age-cleaner": "npm:map-age-cleaner@0.1.3",
      "mimic-fn": "npm:mimic-fn@2.1.0",
      "p-is-promise": "npm:p-is-promise@2.1.0"
    },
    "npm:meow@3.7.0": {
      "camelcase-keys": "npm:camelcase-keys@2.1.0",
      "decamelize": "npm:decamelize@1.2.0",
      "loud-rejection": "npm:loud-rejection@1.6.0",
      "map-obj": "npm:map-obj@1.0.1",
      "minimist": "npm:minimist@1.2.5",
      "normalize-package-data": "npm:normalize-package-data@2.5.0",
      "object-assign": "npm:object-assign@4.1.1",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "read-pkg-up": "npm:read-pkg-up@1.0.1",
      "redent": "npm:redent@1.0.0",
      "trim-newlines": "npm:trim-newlines@1.0.0"
    },
    "npm:miller-rabin@4.0.1": {
      "bn.js": "npm:bn.js@4.11.9",
      "brorand": "npm:brorand@1.1.0"
    },
    "npm:mime-db@1.44.0": {
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:mime-types@2.1.27": {
      "mime-db": "npm:mime-db@1.44.0",
      "path": "github:jspm/nodelibs-path@0.1.0"
    },
    "npm:minimatch@3.0.4": {
      "brace-expansion": "npm:brace-expansion@1.1.11",
      "path": "github:jspm/nodelibs-path@0.1.0"
    },
    "npm:ng2-dragula@1.5.0": {
      "@angular/common": "npm:@angular/common@4.4.4",
      "@angular/compiler": "npm:@angular/compiler@4.4.4",
      "@angular/core": "npm:@angular/core@4.4.4",
      "@angular/forms": "npm:@angular/forms@4.4.4",
      "dragula": "npm:dragula@3.7.2",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:ng2-mqtt@0.1.2": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:ng2-slideable-directive@1.0.13": {
      "ng2-styled-directive": "npm:ng2-styled-directive@1.0.5"
    },
    "npm:ng2-slider-component@1.0.9": {
      "ng2-slideable-directive": "npm:ng2-slideable-directive@1.0.13",
      "ng2-styled-directive": "npm:ng2-styled-directive@1.0.5"
    },
    "npm:ng2-toastr@4.1.2": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2",
      "zlib": "github:jspm/nodelibs-zlib@0.1.0"
    },
    "npm:ngx-datatable@1.0.3": {
      "@angular/common": "npm:@angular/common@2.4.10",
      "@angular/compiler": "npm:@angular/compiler@2.4.10",
      "@angular/core": "npm:@angular/core@2.4.10",
      "@angular/forms": "npm:@angular/forms@2.4.10"
    },
    "npm:normalize-package-data@2.5.0": {
      "hosted-git-info": "npm:hosted-git-info@2.8.8",
      "resolve": "npm:resolve@1.17.0",
      "semver": "npm:semver@5.7.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0",
      "validate-npm-package-license": "npm:validate-npm-package-license@3.0.4"
    },
    "npm:npm-run-path@2.0.2": {
      "path": "github:jspm/nodelibs-path@0.1.0",
      "path-key": "npm:path-key@2.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:oauth-sign@0.9.0": {
      "crypto": "github:jspm/nodelibs-crypto@0.1.0"
    },
    "npm:once@1.4.0": {
      "wrappy": "npm:wrappy@1.0.2"
    },
    "npm:opn@5.2.0": {
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "is-wsl": "npm:is-wsl@1.1.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:os-browserify@0.1.2": {
      "os": "github:jspm/nodelibs-os@0.1.0"
    },
    "npm:os-locale@3.1.0": {
      "execa": "npm:execa@1.0.0",
      "lcid": "npm:lcid@2.0.0",
      "mem": "npm:mem@4.3.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:os-tmpdir@1.0.2": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:p-limit@2.3.0": {
      "p-try": "npm:p-try@2.2.0"
    },
    "npm:p-locate@3.0.0": {
      "p-limit": "npm:p-limit@2.3.0"
    },
    "npm:pako@0.2.9": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:pako@1.0.11": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:parse-asn1@5.1.5": {
      "asn1.js": "npm:asn1.js@4.10.1",
      "browserify-aes": "npm:browserify-aes@1.2.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "create-hash": "npm:create-hash@1.2.0",
      "evp_bytestokey": "npm:evp_bytestokey@1.0.3",
      "pbkdf2": "npm:pbkdf2@3.0.17",
      "safe-buffer": "npm:safe-buffer@5.2.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:parse-json@2.2.0": {
      "error-ex": "npm:error-ex@1.3.2"
    },
    "npm:path-browserify@0.0.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:path-exists@2.1.0": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "pinkie-promise": "npm:pinkie-promise@2.0.1"
    },
    "npm:path-exists@3.0.0": {
      "fs": "github:jspm/nodelibs-fs@0.1.2"
    },
    "npm:path-is-absolute@1.0.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:path-is-inside@1.0.2": {
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:path-key@2.0.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:path-parse@1.0.6": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:path-type@1.1.0": {
      "graceful-fs": "npm:graceful-fs@4.2.4",
      "pify": "npm:pify@2.3.0",
      "pinkie-promise": "npm:pinkie-promise@2.0.1"
    },
    "npm:pbkdf2@3.0.17": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "create-hash": "npm:create-hash@1.2.0",
      "create-hmac": "npm:create-hmac@1.1.7",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "ripemd160": "npm:ripemd160@2.0.2",
      "safe-buffer": "npm:safe-buffer@5.2.1",
      "sha.js": "npm:sha.js@2.4.11"
    },
    "npm:performance-now@2.1.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:pify@2.3.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:pinkie-promise@2.0.1": {
      "pinkie": "npm:pinkie@2.0.4"
    },
    "npm:plugin-log@0.1.0": {
      "chalk": "npm:chalk@1.1.3",
      "dateformat": "npm:dateformat@1.0.12",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:postcss@5.2.17": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "chalk": "npm:chalk@1.1.3",
      "js-base64": "npm:js-base64@2.1.9",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "source-map": "npm:source-map@0.5.7",
      "supports-color": "npm:supports-color@3.2.3"
    },
    "npm:process-nextick-args@2.0.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:process@0.11.10": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "vm": "github:jspm/nodelibs-vm@0.1.0"
    },
    "npm:protractor@5.4.4": {
      "@types/q": "npm:@types/q@0.0.32",
      "@types/selenium-webdriver": "npm:@types/selenium-webdriver@3.0.17",
      "blocking-proxy": "npm:blocking-proxy@1.0.1",
      "browserstack": "npm:browserstack@1.6.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "chalk": "npm:chalk@1.1.3",
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "glob": "npm:glob@7.1.6",
      "jasmine": "npm:jasmine@2.8.0",
      "jasminewd2": "npm:jasminewd2@2.2.0",
      "net": "github:jspm/nodelibs-net@0.1.2",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "q": "npm:q@1.4.1",
      "repl": "github:jspm/nodelibs-repl@0.1.0",
      "saucelabs": "npm:saucelabs@1.5.0",
      "selenium-webdriver": "npm:selenium-webdriver@3.6.0",
      "source-map-support": "npm:source-map-support@0.4.18",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0",
      "vm": "github:jspm/nodelibs-vm@0.1.0",
      "webdriver-js-extender": "npm:webdriver-js-extender@2.1.0",
      "webdriver-manager": "npm:webdriver-manager@12.1.7",
      "yargs": "npm:yargs@12.0.5"
    },
    "npm:psl@1.8.0": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "punycode": "github:jspm/nodelibs-punycode@0.1.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:public-encrypt@4.0.3": {
      "bn.js": "npm:bn.js@4.11.9",
      "browserify-rsa": "npm:browserify-rsa@4.0.1",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "create-hash": "npm:create-hash@1.2.0",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "parse-asn1": "npm:parse-asn1@5.1.5",
      "randombytes": "npm:randombytes@2.1.0",
      "safe-buffer": "npm:safe-buffer@5.2.1"
    },
    "npm:pump@3.0.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "end-of-stream": "npm:end-of-stream@1.4.4",
      "once": "npm:once@1.4.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0"
    },
    "npm:punycode@1.3.2": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:punycode@2.1.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:q@1.4.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:randombytes@2.1.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "safe-buffer": "npm:safe-buffer@5.2.1"
    },
    "npm:randomfill@1.0.4": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "randombytes": "npm:randombytes@2.1.0",
      "safe-buffer": "npm:safe-buffer@5.2.1"
    },
    "npm:read-pkg-up@1.0.1": {
      "find-up": "npm:find-up@1.1.2",
      "read-pkg": "npm:read-pkg@1.1.0"
    },
    "npm:read-pkg@1.1.0": {
      "load-json-file": "npm:load-json-file@1.1.0",
      "normalize-package-data": "npm:normalize-package-data@2.5.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "path-type": "npm:path-type@1.1.0"
    },
    "npm:readable-stream@1.1.14": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "core-util-is": "npm:core-util-is@1.0.2",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "inherits": "npm:inherits@2.0.1",
      "isarray": "npm:isarray@0.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream-browserify": "npm:stream-browserify@1.0.0",
      "string_decoder": "npm:string_decoder@0.10.31"
    },
    "npm:readable-stream@2.3.7": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "core-util-is": "npm:core-util-is@1.0.2",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "inherits": "npm:inherits@2.0.4",
      "isarray": "npm:isarray@1.0.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "process-nextick-args": "npm:process-nextick-args@2.0.1",
      "safe-buffer": "npm:safe-buffer@5.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "string_decoder": "npm:string_decoder@1.1.1",
      "util-deprecate": "npm:util-deprecate@1.0.2"
    },
    "npm:readable-stream@3.6.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "inherits": "npm:inherits@2.0.4",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "string_decoder": "npm:string_decoder@1.1.1",
      "util-deprecate": "npm:util-deprecate@1.0.2"
    },
    "npm:redent@1.0.0": {
      "indent-string": "npm:indent-string@2.1.0",
      "strip-indent": "npm:strip-indent@1.0.1"
    },
    "npm:repeating@2.0.1": {
      "is-finite": "npm:is-finite@1.1.0"
    },
    "npm:request@2.88.2": {
      "aws-sign2": "npm:aws-sign2@0.7.0",
      "aws4": "npm:aws4@1.9.1",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "caseless": "npm:caseless@0.12.0",
      "combined-stream": "npm:combined-stream@1.0.8",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "extend": "npm:extend@3.0.2",
      "forever-agent": "npm:forever-agent@0.6.1",
      "form-data": "npm:form-data@2.3.3",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "har-validator": "npm:har-validator@5.1.3",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "http-signature": "npm:http-signature@1.2.0",
      "https": "github:jspm/nodelibs-https@0.1.0",
      "is-typedarray": "npm:is-typedarray@1.0.0",
      "isstream": "npm:isstream@0.1.2",
      "json-stringify-safe": "npm:json-stringify-safe@5.0.1",
      "mime-types": "npm:mime-types@2.1.27",
      "oauth-sign": "npm:oauth-sign@0.9.0",
      "performance-now": "npm:performance-now@2.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "qs": "npm:qs@6.5.2",
      "querystring": "github:jspm/nodelibs-querystring@0.1.0",
      "safe-buffer": "npm:safe-buffer@5.2.1",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "tough-cookie": "npm:tough-cookie@2.5.0",
      "tunnel-agent": "npm:tunnel-agent@0.6.0",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0",
      "uuid": "npm:uuid@3.4.0",
      "zlib": "github:jspm/nodelibs-zlib@0.1.0"
    },
    "npm:require-directory@2.1.1": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0"
    },
    "npm:require-main-filename@1.0.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:resolve@1.17.0": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "path-parse": "npm:path-parse@1.0.6",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:rimraf@2.7.1": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "glob": "npm:glob@7.1.6",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:ripemd160@2.0.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "hash-base": "npm:hash-base@3.1.0",
      "inherits": "npm:inherits@2.0.4"
    },
    "npm:rw@1.3.3": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:rxjs@5.5.6": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "symbol-observable": "npm:symbol-observable@1.0.1"
    },
    "npm:safe-buffer@5.1.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1"
    },
    "npm:safe-buffer@5.2.1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1"
    },
    "npm:safer-buffer@2.1.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:sass.js@0.9.13": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:saucelabs@1.5.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "https": "github:jspm/nodelibs-https@0.1.0",
      "https-proxy-agent": "npm:https-proxy-agent@2.2.4",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "querystring": "github:jspm/nodelibs-querystring@0.1.0",
      "url": "github:jspm/nodelibs-url@0.1.0"
    },
    "npm:sax@0.6.1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "string_decoder": "github:jspm/nodelibs-string_decoder@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:selenium-webdriver@3.6.0": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "https": "github:jspm/nodelibs-https@0.1.0",
      "jszip": "npm:jszip@3.4.0",
      "net": "github:jspm/nodelibs-net@0.1.2",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "rimraf": "npm:rimraf@2.7.1",
      "tmp": "npm:tmp@0.0.30",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0",
      "vm": "github:jspm/nodelibs-vm@0.1.0",
      "xml2js": "npm:xml2js@0.4.23"
    },
    "npm:semver@5.7.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:set-blocking@2.0.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:sha.js@2.4.11": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "inherits": "npm:inherits@2.0.4",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "safe-buffer": "npm:safe-buffer@5.2.1"
    },
    "npm:shebang-command@1.2.0": {
      "shebang-regex": "npm:shebang-regex@1.0.0"
    },
    "npm:signal-exit@3.0.3": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:source-map-support@0.4.18": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "module": "github:jspm/nodelibs-module@0.1.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "source-map": "npm:source-map@0.5.7"
    },
    "npm:source-map@0.5.7": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:spdx-correct@3.1.0": {
      "spdx-expression-parse": "npm:spdx-expression-parse@3.0.1",
      "spdx-license-ids": "npm:spdx-license-ids@3.0.5"
    },
    "npm:spdx-expression-parse@3.0.1": {
      "spdx-exceptions": "npm:spdx-exceptions@2.3.0",
      "spdx-license-ids": "npm:spdx-license-ids@3.0.5"
    },
    "npm:sshpk@1.16.1": {
      "asn1": "npm:asn1@0.2.4",
      "assert-plus": "npm:assert-plus@1.0.0",
      "bcrypt-pbkdf": "npm:bcrypt-pbkdf@1.0.2",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "dashdash": "npm:dashdash@1.14.1",
      "ecc-jsbn": "npm:ecc-jsbn@0.1.2",
      "getpass": "npm:getpass@0.1.7",
      "jsbn": "npm:jsbn@0.1.1",
      "safer-buffer": "npm:safer-buffer@2.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "tweetnacl": "npm:tweetnacl@0.14.5",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:stream-browserify@1.0.0": {
      "events": "github:jspm/nodelibs-events@0.1.1",
      "inherits": "npm:inherits@2.0.1",
      "readable-stream": "npm:readable-stream@1.1.14"
    },
    "npm:string-width@1.0.2": {
      "code-point-at": "npm:code-point-at@1.1.0",
      "is-fullwidth-code-point": "npm:is-fullwidth-code-point@1.0.0",
      "strip-ansi": "npm:strip-ansi@3.0.1"
    },
    "npm:string-width@2.1.1": {
      "is-fullwidth-code-point": "npm:is-fullwidth-code-point@2.0.0",
      "strip-ansi": "npm:strip-ansi@4.0.0"
    },
    "npm:string_decoder@0.10.31": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1"
    },
    "npm:string_decoder@1.1.1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "safe-buffer": "npm:safe-buffer@5.1.2"
    },
    "npm:strip-ansi@3.0.1": {
      "ansi-regex": "npm:ansi-regex@2.1.1"
    },
    "npm:strip-ansi@4.0.0": {
      "ansi-regex": "npm:ansi-regex@3.0.0"
    },
    "npm:strip-bom@2.0.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "is-utf8": "npm:is-utf8@0.2.1"
    },
    "npm:strip-indent@1.0.1": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "get-stdin": "npm:get-stdin@4.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:supports-color@2.0.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:supports-color@3.2.3": {
      "has-flag": "npm:has-flag@1.0.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:supports-color@5.5.0": {
      "has-flag": "npm:has-flag@3.0.0",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:systemjs-hot-reloader@1.1.0": {
      "systemjs-hmr": "npm:systemjs-hmr@2.0.9"
    },
    "npm:through2@2.0.5": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "readable-stream": "npm:readable-stream@2.3.7",
      "util": "github:jspm/nodelibs-util@0.1.0",
      "xtend": "npm:xtend@4.0.2"
    },
    "npm:ticky@1.0.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:timers-browserify@1.4.2": {
      "process": "npm:process@0.11.10"
    },
    "npm:tmp@0.0.30": {
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "os-tmpdir": "npm:os-tmpdir@1.0.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:tough-cookie@2.5.0": {
      "net": "github:jspm/nodelibs-net@0.1.2",
      "psl": "npm:psl@1.8.0",
      "punycode": "npm:punycode@2.1.1",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:tunnel-agent@0.6.0": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "https": "github:jspm/nodelibs-https@0.1.0",
      "net": "github:jspm/nodelibs-net@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "safe-buffer": "npm:safe-buffer@5.2.1",
      "tls": "github:jspm/nodelibs-tls@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:typescript@2.5.3": {
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "net": "github:jspm/nodelibs-net@0.1.2",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "source-map-support": "npm:source-map-support@0.4.18"
    },
    "npm:uri-js@3.0.2": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "punycode": "npm:punycode@2.1.1"
    },
    "npm:uri-js@4.2.2": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "punycode": "npm:punycode@2.1.1"
    },
    "npm:url@0.10.3": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "punycode": "npm:punycode@1.3.2",
      "querystring": "npm:querystring@0.2.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:util-deprecate@1.0.2": {
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:util@0.10.3": {
      "inherits": "npm:inherits@2.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:uuid@3.4.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0"
    },
    "npm:validate-npm-package-license@3.0.4": {
      "spdx-correct": "npm:spdx-correct@3.1.0",
      "spdx-expression-parse": "npm:spdx-expression-parse@3.0.1"
    },
    "npm:validate-typescript@2.0.1": {
      "chalk": "npm:chalk@2.4.2"
    },
    "npm:validator.ts@0.2.2": {
      "validator": "npm:validator@5.7.0"
    },
    "npm:verror@1.10.0": {
      "assert-plus": "npm:assert-plus@1.0.0",
      "core-util-is": "npm:core-util-is@1.0.2",
      "extsprintf": "npm:extsprintf@1.3.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:vm-browserify@0.0.4": {
      "indexof": "npm:indexof@0.0.1"
    },
    "npm:webdriver-js-extender@2.1.0": {
      "@types/selenium-webdriver": "npm:@types/selenium-webdriver@3.0.17",
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "selenium-webdriver": "npm:selenium-webdriver@3.6.0"
    },
    "npm:webdriver-manager@12.1.7": {
      "adm-zip": "npm:adm-zip@0.4.14",
      "chalk": "npm:chalk@1.1.3",
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "del": "npm:del@2.2.2",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "glob": "npm:glob@7.1.6",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "ini": "npm:ini@1.3.5",
      "minimist": "npm:minimist@1.2.5",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "q": "npm:q@1.4.1",
      "request": "npm:request@2.88.2",
      "rimraf": "npm:rimraf@2.7.1",
      "semver": "npm:semver@5.7.1",
      "url": "github:jspm/nodelibs-url@0.1.0",
      "xml2js": "npm:xml2js@0.4.23"
    },
    "npm:which@1.3.1": {
      "isexe": "npm:isexe@2.0.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:wrap-ansi@2.1.0": {
      "string-width": "npm:string-width@1.0.2",
      "strip-ansi": "npm:strip-ansi@3.0.1"
    },
    "npm:xml2js@0.4.23": {
      "events": "github:jspm/nodelibs-events@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "sax": "npm:sax@0.6.1",
      "timers": "github:jspm/nodelibs-timers@0.1.0",
      "xmlbuilder": "npm:xmlbuilder@11.0.1"
    },
    "npm:xmlbuilder@11.0.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:xmlhttprequest@1.8.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "http": "github:jspm/nodelibs-http@1.7.1",
      "https": "github:jspm/nodelibs-https@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "url": "github:jspm/nodelibs-url@0.1.0"
    },
    "npm:y18n@4.0.0": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:yargs-parser@11.1.1": {
      "camelcase": "npm:camelcase@5.3.1",
      "decamelize": "npm:decamelize@1.2.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:yargs@12.0.5": {
      "cliui": "npm:cliui@4.1.0",
      "decamelize": "npm:decamelize@1.2.0",
      "find-up": "npm:find-up@3.0.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "get-caller-file": "npm:get-caller-file@1.0.3",
      "os-locale": "npm:os-locale@3.1.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "require-directory": "npm:require-directory@2.1.1",
      "require-main-filename": "npm:require-main-filename@1.0.1",
      "set-blocking": "npm:set-blocking@2.0.0",
      "string-width": "npm:string-width@2.1.1",
      "util": "github:jspm/nodelibs-util@0.1.0",
      "which-module": "npm:which-module@2.0.0",
      "y18n": "npm:y18n@4.0.0",
      "yargs-parser": "npm:yargs-parser@11.1.1"
    },
    "npm:zone.js@0.7.8": {
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "timers": "github:jspm/nodelibs-timers@0.1.0"
    },
    "npm:zone.js@0.8.18": {
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "timers": "github:jspm/nodelibs-timers@0.1.0"
    }
  }
});
