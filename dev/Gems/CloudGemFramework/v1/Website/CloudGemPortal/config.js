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
    "app": "./app"
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

  map: {
    "@angular/animations": "npm:@angular/animations@4.3.1",
    "@angular/animations/browser": "npm:@angular/animations@4.3.1/bundles/animations-browser.umd.js",
    "@angular/common": "npm:@angular/common@4.3.1",
    "@angular/compiler": "npm:@angular/compiler@4.3.1",
    "@angular/core": "npm:@angular/core@4.3.1",
    "@angular/forms": "npm:@angular/forms@4.3.1",
    "@angular/http": "npm:@angular/http@4.3.1",
    "@angular/platform-browser": "npm:@angular/platform-browser@4.3.1",
    "@angular/platform-browser-dynamic": "npm:@angular/platform-browser-dynamic@4.3.1",
    "@angular/platform-browser/animations": "npm:@angular/platform-browser@4.3.1/bundles/platform-browser-animations.umd.js",
    "@angular/router": "npm:@angular/router@4.3.1",
    "@ng-bootstrap/ng-bootstrap": "npm:@ng-bootstrap/ng-bootstrap@1.0.0-alpha.29",
    "capaj/systemjs-hot-reloader": "github:alexisvincent/systemjs-hot-reloader@0.5.9",
    "core-js": "npm:core-js@2.4.1",
    "crypto-js": "npm:crypto-js@3.1.9-1",
    "d3": "npm:d3@4.10.0",
    "dragula": "npm:dragula@3.7.2",
    "html": "github:Hypercubed/systemjs-plugin-html@0.0.8",
    "module": "npm:jspm-nodelibs-module@0.2.1",
    "net": "npm:jspm-nodelibs-net@0.2.1",
    "ng2-dragula": "npm:ng2-dragula@1.5.0",
    "ng2-toastr": "npm:ng2-toastr@4.1.2",
    "process-finder": "npm:process-finder@1.0.0",
    "reflect-metadata": "npm:reflect-metadata@0.1.10",
    "rxjs": "npm:rxjs@5.4.2",
    "scss": "github:KevCJones/plugin-scss@0.2.11",
    "systemjs-hot-reloader": "npm:systemjs-hot-reloader@1.0.0",
    "text": "github:systemjs/plugin-text@0.0.11",
    "through2": "npm:through2@2.0.3",
    "ts": "github:frankwallis/plugin-typescript@7.0.6",
    "ts-clipboard": "npm:ts-clipboard@1.0.13",
    "ts-md5": "npm:ts-md5@1.2.2",
    "ts-runtime": "npm:babel-runtime@5.8.38",
    "typescript": "npm:typescript@2.4.2",
    "zone.js": "npm:zone.js@0.8.14",
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
      "debug": "npm:debug@2.6.8",
      "socket.io-client": "github:socketio/socket.io-client@1.7.4",
      "weakee": "npm:weakee@1.0.0"
    },
    "github:jspm/nodelibs-assert@0.1.0": {
      "assert": "npm:assert@1.4.1"
    },
    "github:jspm/nodelibs-buffer@0.1.1": {
      "buffer": "npm:buffer@5.0.7"
    },
    "github:jspm/nodelibs-constants@0.1.0": {
      "constants-browserify": "npm:constants-browserify@0.0.1"
    },
    "github:jspm/nodelibs-crypto@0.1.0": {
      "crypto-browserify": "npm:crypto-browserify@3.11.1"
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
    "npm:@angular/animations@4.3.1": {
      "@angular/core": "npm:@angular/core@4.3.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.7.1"
    },
    "npm:@angular/common@4.3.1": {
      "@angular/core": "npm:@angular/core@4.3.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.7.1"
    },
    "npm:@angular/compiler@4.3.1": {
      "@angular/core": "npm:@angular/core@4.3.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.7.1"
    },
    "npm:@angular/core@4.3.1": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "rxjs": "npm:rxjs@5.4.2",
      "tslib": "npm:tslib@1.7.1",
      "zone.js": "npm:zone.js@0.8.14"
    },
    "npm:@angular/forms@4.3.1": {
      "@angular/common": "npm:@angular/common@4.3.1",
      "@angular/core": "npm:@angular/core@4.3.1",
      "@angular/platform-browser": "npm:@angular/platform-browser@4.3.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.7.1"
    },
    "npm:@angular/http@4.3.1": {
      "@angular/core": "npm:@angular/core@4.3.1",
      "@angular/platform-browser": "npm:@angular/platform-browser@4.3.1",
      "rxjs": "npm:rxjs@5.4.2",
      "tslib": "npm:tslib@1.7.1"
    },
    "npm:@angular/platform-browser-dynamic@4.3.1": {
      "@angular/common": "npm:@angular/common@4.3.1",
      "@angular/compiler": "npm:@angular/compiler@4.3.1",
      "@angular/core": "npm:@angular/core@4.3.1",
      "@angular/platform-browser": "npm:@angular/platform-browser@4.3.1",
      "tslib": "npm:tslib@1.7.1"
    },
    "npm:@angular/platform-browser@4.3.1": {
      "@angular/common": "npm:@angular/common@4.3.1",
      "@angular/core": "npm:@angular/core@4.3.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "tslib": "npm:tslib@1.7.1"
    },
    "npm:@angular/router@4.3.1": {
      "@angular/common": "npm:@angular/common@4.3.1",
      "@angular/core": "npm:@angular/core@4.3.1",
      "@angular/platform-browser": "npm:@angular/platform-browser@4.3.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "rxjs": "npm:rxjs@5.4.2",
      "tslib": "npm:tslib@1.7.1"
    },
    "npm:@ng-bootstrap/ng-bootstrap@1.0.0-alpha.29": {
      "@angular/common": "npm:@angular/common@4.3.1",
      "@angular/core": "npm:@angular/core@4.3.1",
      "@angular/forms": "npm:@angular/forms@4.3.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:asn1.js@4.9.1": {
      "bn.js": "npm:bn.js@4.11.7",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "inherits": "npm:inherits@2.0.1",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.0",
      "vm": "github:jspm/nodelibs-vm@0.1.0"
    },
    "npm:assert@1.4.1": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "npm:util@0.10.3"
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
    "npm:babel-runtime@5.8.38": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:bn.js@4.11.7": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1"
    },
    "npm:browserify-aes@1.0.6": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "buffer-xor": "npm:buffer-xor@1.0.3",
      "cipher-base": "npm:cipher-base@1.0.4",
      "create-hash": "npm:create-hash@1.1.3",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "evp_bytestokey": "npm:evp_bytestokey@1.0.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "inherits": "npm:inherits@2.0.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:browserify-cipher@1.0.0": {
      "browserify-aes": "npm:browserify-aes@1.0.6",
      "browserify-des": "npm:browserify-des@1.0.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "evp_bytestokey": "npm:evp_bytestokey@1.0.0"
    },
    "npm:browserify-des@1.0.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "cipher-base": "npm:cipher-base@1.0.4",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "des.js": "npm:des.js@1.0.0",
      "inherits": "npm:inherits@2.0.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:browserify-rsa@4.0.1": {
      "bn.js": "npm:bn.js@4.11.7",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "constants": "github:jspm/nodelibs-constants@0.1.0",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "randombytes": "npm:randombytes@2.0.5",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:browserify-sign@4.0.4": {
      "bn.js": "npm:bn.js@4.11.7",
      "browserify-rsa": "npm:browserify-rsa@4.0.1",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "create-hash": "npm:create-hash@1.1.3",
      "create-hmac": "npm:create-hmac@1.1.6",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "elliptic": "npm:elliptic@6.4.0",
      "inherits": "npm:inherits@2.0.1",
      "parse-asn1": "npm:parse-asn1@5.1.0",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:browserify-zlib@0.1.4": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "pako": "npm:pako@0.2.9",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "readable-stream": "npm:readable-stream@2.3.3",
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
    "npm:buffer-xor@1.0.3": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:buffer@5.0.7": {
      "base64-js": "npm:base64-js@1.2.1",
      "ieee754": "npm:ieee754@1.1.8"
    },
    "npm:chalk@1.1.3": {
      "ansi-styles": "npm:ansi-styles@2.2.1",
      "escape-string-regexp": "npm:escape-string-regexp@1.0.5",
      "has-ansi": "npm:has-ansi@2.0.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "strip-ansi": "npm:strip-ansi@3.0.1",
      "supports-color": "npm:supports-color@2.0.0"
    },
    "npm:cipher-base@1.0.4": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "inherits": "npm:inherits@2.0.1",
      "safe-buffer": "npm:safe-buffer@5.1.1",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "string_decoder": "github:jspm/nodelibs-string_decoder@0.1.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:commander@2.11.0": {
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:constants-browserify@0.0.1": {
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:contra@1.9.4": {
      "atoa": "npm:atoa@1.0.0",
      "ticky": "npm:ticky@1.0.1"
    },
    "npm:core-js@2.4.1": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:core-util-is@1.0.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1"
    },
    "npm:create-ecdh@4.0.0": {
      "bn.js": "npm:bn.js@4.11.7",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "elliptic": "npm:elliptic@6.4.0"
    },
    "npm:create-hash@1.1.3": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "cipher-base": "npm:cipher-base@1.0.4",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "inherits": "npm:inherits@2.0.1",
      "ripemd160": "npm:ripemd160@2.0.1",
      "sha.js": "npm:sha.js@2.4.8"
    },
    "npm:create-hmac@1.1.6": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "cipher-base": "npm:cipher-base@1.0.4",
      "create-hash": "npm:create-hash@1.1.3",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "inherits": "npm:inherits@2.0.1",
      "ripemd160": "npm:ripemd160@2.0.1",
      "safe-buffer": "npm:safe-buffer@5.1.1",
      "sha.js": "npm:sha.js@2.4.8"
    },
    "npm:crossvent@1.5.4": {
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "custom-event": "npm:custom-event@1.0.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:crypto-browserify@3.11.1": {
      "browserify-cipher": "npm:browserify-cipher@1.0.0",
      "browserify-sign": "npm:browserify-sign@4.0.4",
      "create-ecdh": "npm:create-ecdh@4.0.0",
      "create-hash": "npm:create-hash@1.1.3",
      "create-hmac": "npm:create-hmac@1.1.6",
      "diffie-hellman": "npm:diffie-hellman@5.0.2",
      "inherits": "npm:inherits@2.0.1",
      "pbkdf2": "npm:pbkdf2@3.0.12",
      "public-encrypt": "npm:public-encrypt@4.0.0",
      "randombytes": "npm:randombytes@2.0.5",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:crypto-js@3.1.9-1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
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
    "npm:d3-dsv@1.0.5": {
      "commander": "npm:commander@2.11.0",
      "iconv-lite": "npm:iconv-lite@0.4.18",
      "rw": "npm:rw@1.3.3"
    },
    "npm:d3-force@1.0.6": {
      "d3-collection": "npm:d3-collection@1.0.4",
      "d3-dispatch": "npm:d3-dispatch@1.0.3",
      "d3-quadtree": "npm:d3-quadtree@1.0.3",
      "d3-timer": "npm:d3-timer@1.0.6",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:d3-geo@1.6.4": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "d3-array": "npm:d3-array@1.2.0"
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
    "npm:d3-time-format@2.0.5": {
      "d3-time": "npm:d3-time@1.0.7"
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
    "npm:debug@2.6.8": {
      "ms": "npm:ms@2.0.0"
    },
    "npm:des.js@1.0.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "inherits": "npm:inherits@2.0.1",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.0"
    },
    "npm:diffie-hellman@5.0.2": {
      "bn.js": "npm:bn.js@4.11.7",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "miller-rabin": "npm:miller-rabin@4.0.0",
      "randombytes": "npm:randombytes@2.0.5",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:dragula@3.7.2": {
      "contra": "npm:contra@1.9.4",
      "crossvent": "npm:crossvent@1.5.4"
    },
    "npm:elliptic@6.4.0": {
      "bn.js": "npm:bn.js@4.11.7",
      "brorand": "npm:brorand@1.1.0",
      "hash.js": "npm:hash.js@1.1.3",
      "hmac-drbg": "npm:hmac-drbg@1.0.1",
      "inherits": "npm:inherits@2.0.1",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.0",
      "minimalistic-crypto-utils": "npm:minimalistic-crypto-utils@1.0.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:evp_bytestokey@1.0.0": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "create-hash": "npm:create-hash@1.1.3",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:has-ansi@2.0.0": {
      "ansi-regex": "npm:ansi-regex@2.1.1"
    },
    "npm:has-flag@1.0.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:hash-base@2.0.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "inherits": "npm:inherits@2.0.1",
      "stream": "github:jspm/nodelibs-stream@0.1.0"
    },
    "npm:hash.js@1.1.3": {
      "inherits": "npm:inherits@2.0.3",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.0"
    },
    "npm:hmac-drbg@1.0.1": {
      "hash.js": "npm:hash.js@1.1.3",
      "minimalistic-assert": "npm:minimalistic-assert@1.0.0",
      "minimalistic-crypto-utils": "npm:minimalistic-crypto-utils@1.0.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:https-browserify@0.0.0": {
      "http": "github:jspm/nodelibs-http@1.7.1"
    },
    "npm:iconv-lite@0.4.18": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "string_decoder": "github:jspm/nodelibs-string_decoder@0.1.0",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:inherits@2.0.1": {
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:inherits@2.0.3": {
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:isarray@1.0.0": {
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:js-base64@2.1.9": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1"
    },
    "npm:miller-rabin@4.0.0": {
      "bn.js": "npm:bn.js@4.11.7",
      "brorand": "npm:brorand@1.1.0"
    },
    "npm:ng2-dragula@1.5.0": {
      "@angular/common": "npm:@angular/common@4.3.1",
      "@angular/compiler": "npm:@angular/compiler@4.3.1",
      "@angular/core": "npm:@angular/core@4.3.1",
      "@angular/forms": "npm:@angular/forms@4.3.1",
      "dragula": "npm:dragula@3.7.2",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:ng2-toastr@4.1.2": {
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2",
      "zlib": "github:jspm/nodelibs-zlib@0.1.0"
    },
    "npm:os-browserify@0.1.2": {
      "os": "github:jspm/nodelibs-os@0.1.0"
    },
    "npm:pako@0.2.9": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:parse-asn1@5.1.0": {
      "asn1.js": "npm:asn1.js@4.9.1",
      "browserify-aes": "npm:browserify-aes@1.0.6",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "create-hash": "npm:create-hash@1.1.3",
      "evp_bytestokey": "npm:evp_bytestokey@1.0.0",
      "pbkdf2": "npm:pbkdf2@3.0.12",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:path-browserify@0.0.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:pbkdf2@3.0.12": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "create-hash": "npm:create-hash@1.1.3",
      "create-hmac": "npm:create-hmac@1.1.6",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "ripemd160": "npm:ripemd160@2.0.1",
      "safe-buffer": "npm:safe-buffer@5.1.1",
      "sha.js": "npm:sha.js@2.4.8"
    },
    "npm:postcss@5.2.17": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "chalk": "npm:chalk@1.1.3",
      "js-base64": "npm:js-base64@2.1.9",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "source-map": "npm:source-map@0.5.6",
      "supports-color": "npm:supports-color@3.2.3"
    },
    "npm:process-finder@1.0.0": {
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "minimist": "npm:minimist@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "util": "github:jspm/nodelibs-util@0.1.0"
    },
    "npm:process-nextick-args@1.0.7": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:process@0.11.10": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "vm": "github:jspm/nodelibs-vm@0.1.0"
    },
    "npm:public-encrypt@4.0.0": {
      "bn.js": "npm:bn.js@4.11.7",
      "browserify-rsa": "npm:browserify-rsa@4.0.1",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "create-hash": "npm:create-hash@1.1.3",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "parse-asn1": "npm:parse-asn1@5.1.0",
      "randombytes": "npm:randombytes@2.0.5"
    },
    "npm:punycode@1.3.2": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:randombytes@2.0.5": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "safe-buffer": "npm:safe-buffer@5.1.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:readable-stream@1.1.14": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "core-util-is": "npm:core-util-is@1.0.2",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "inherits": "npm:inherits@2.0.3",
      "isarray": "npm:isarray@0.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "stream-browserify": "npm:stream-browserify@1.0.0",
      "string_decoder": "npm:string_decoder@0.10.31"
    },
    "npm:readable-stream@2.3.3": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "core-util-is": "npm:core-util-is@1.0.2",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "inherits": "npm:inherits@2.0.3",
      "isarray": "npm:isarray@1.0.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "process-nextick-args": "npm:process-nextick-args@1.0.7",
      "safe-buffer": "npm:safe-buffer@5.1.1",
      "stream": "github:jspm/nodelibs-stream@0.1.0",
      "string_decoder": "npm:string_decoder@1.0.3",
      "util-deprecate": "npm:util-deprecate@1.0.2"
    },
    "npm:ripemd160@2.0.1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "hash-base": "npm:hash-base@2.0.2",
      "inherits": "npm:inherits@2.0.1"
    },
    "npm:rw@1.3.3": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:rxjs@5.4.2": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "symbol-observable": "npm:symbol-observable@1.0.4"
    },
    "npm:safe-buffer@5.1.1": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:sass.js@0.9.13": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:sha.js@2.4.8": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "inherits": "npm:inherits@2.0.1",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:source-map-support@0.4.15": {
      "assert": "github:jspm/nodelibs-assert@0.1.0",
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "child_process": "github:jspm/nodelibs-child_process@0.1.0",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "module": "github:jspm/nodelibs-module@0.1.0",
      "path": "github:jspm/nodelibs-path@0.1.0",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "querystring": "github:jspm/nodelibs-querystring@0.1.0",
      "source-map": "npm:source-map@0.5.6",
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:source-map@0.5.6": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:stream-browserify@1.0.0": {
      "events": "github:jspm/nodelibs-events@0.1.1",
      "inherits": "npm:inherits@2.0.3",
      "readable-stream": "npm:readable-stream@1.1.14"
    },
    "npm:string_decoder@0.10.31": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1"
    },
    "npm:string_decoder@1.0.3": {
      "buffer": "github:jspm/nodelibs-buffer@0.1.1",
      "safe-buffer": "npm:safe-buffer@5.1.1"
    },
    "npm:strip-ansi@3.0.1": {
      "ansi-regex": "npm:ansi-regex@2.1.1"
    },
    "npm:supports-color@2.0.0": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:supports-color@3.2.3": {
      "has-flag": "npm:has-flag@1.0.0",
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:systemjs-hot-reloader@1.0.0": {
      "systemjs-hmr": "npm:systemjs-hmr@2.0.9"
    },
    "npm:through2@2.0.3": {
      "process": "github:jspm/nodelibs-process@0.1.2",
      "readable-stream": "npm:readable-stream@2.3.3",
      "util": "github:jspm/nodelibs-util@0.1.0",
      "xtend": "npm:xtend@4.0.1"
    },
    "npm:ticky@1.0.1": {
      "process": "github:jspm/nodelibs-process@0.1.2"
    },
    "npm:timers-browserify@1.4.2": {
      "process": "npm:process@0.11.10"
    },
    "npm:typescript@2.4.2": {
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "net": "github:jspm/nodelibs-net@0.1.2",
      "os": "github:jspm/nodelibs-os@0.1.0",
      "source-map-support": "npm:source-map-support@0.4.15"
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
    "npm:vm-browserify@0.0.4": {
      "indexof": "npm:indexof@0.0.1"
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
    "npm:xtend@4.0.1": {
      "systemjs-json": "github:systemjs/plugin-json@0.1.2"
    },
    "npm:zone.js@0.8.14": {
      "crypto": "github:jspm/nodelibs-crypto@0.1.0",
      "events": "github:jspm/nodelibs-events@0.1.1",
      "fs": "github:jspm/nodelibs-fs@0.1.2",
      "process": "github:jspm/nodelibs-process@0.1.2",
      "timers": "github:jspm/nodelibs-timers@0.1.0"
    }
  }
});
