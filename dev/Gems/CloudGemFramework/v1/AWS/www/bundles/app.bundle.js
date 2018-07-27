System.register("dist/app/app.routes.js", ["@angular/core", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, router_1, GAME_ROUTES, AppRoutingModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            exports_1("GAME_ROUTES", GAME_ROUTES = [{
                path: '**',
                redirectTo: 'login',
                pathMatch: 'full'
            }]);
            AppRoutingModule = /** @class */function () {
                function AppRoutingModule() {}
                AppRoutingModule = __decorate([core_1.NgModule({
                    imports: [router_1.RouterModule.forRoot(GAME_ROUTES)],
                    exports: [router_1.RouterModule]
                })], AppRoutingModule);
                return AppRoutingModule;
            }();
            exports_1("AppRoutingModule", AppRoutingModule);
        }
    };
});
System.register("dist/app/app.component.js", ["@angular/core", "app/shared/service/index", "app/aws/aws.service", "ng2-toastr"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, index_1, aws_service_1, ng2_toastr_1, AppComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }],
        execute: function () {
            AppComponent = /** @class */function () {
                function AppComponent(ds, aws, urlService, toastr, vcr) {
                    this.toastr = toastr;
                    toastr.setRootViewContainerRef(vcr);
                    urlService.parseLocationHref(location.href);
                    if (ds.isTest) {
                        localStorage.clear();
                    }
                    // if (ds.isProd)
                    //     window.onbeforeunload = function (e) {
                    //         var e = e || window.event;
                    //         var msg = "Do you really want to leave this page?"
                    //         // For IE and Firefox
                    //         if (e) {
                    //             e.returnValue = msg;
                    //         }
                    //         // For Safari / chrome
                    //         return msg;
                    //     };
                    aws.init(bootstrap.userPoolId, bootstrap.clientId, bootstrap.identityPoolId, bootstrap.projectConfigBucketId, bootstrap.region, ds);
                }
                AppComponent = __decorate([core_1.Component({
                    selector: 'cc-app',
                    template: '<router-outlet></router-outlet>',
                    styles: ["/*!  * Bootstrap v4.0.0-alpha.6 (https://getbootstrap.com)  * Copyright 2011-2017 The Bootstrap Authors  * Copyright 2011-2017 Twitter, Inc.  * Licensed under MIT (https://github.com/twbs/bootstrap/blob/master/LICENSE)  *//*! normalize.css v5.0.0 | MIT License | github.com/necolas/normalize.css */html{font-family:sans-serif;line-height:1.15;-ms-text-size-adjust:100%;-webkit-text-size-adjust:100%}body{margin:0}article,aside,footer,header,nav,section{display:block}h1{font-size:2em;margin:0.67em 0}figcaption,figure,main{display:block}figure{margin:1em 40px}hr{box-sizing:content-box;height:0;overflow:visible}pre{font-family:monospace, monospace;font-size:1em}a{background-color:transparent;-webkit-text-decoration-skip:objects}a:active,a:hover{outline-width:0}abbr[title]{border-bottom:none;text-decoration:underline;text-decoration:underline dotted}b,strong{font-weight:inherit}b,strong{font-weight:bolder}code,kbd,samp{font-family:monospace, monospace;font-size:1em}dfn{font-style:italic}mark{background-color:#ff0;color:#000}small{font-size:80%}sub,sup{font-size:75%;line-height:0;position:relative;vertical-align:baseline}sub{bottom:-0.25em}sup{top:-0.5em}audio,video{display:inline-block}audio:not([controls]){display:none;height:0}img{border-style:none}svg:not(:root){overflow:hidden}button,input,optgroup,select,textarea{font-family:sans-serif;font-size:100%;line-height:1.15;margin:0}button,input{overflow:visible}button,select{text-transform:none}button,html [type=\"button\"],[type=\"reset\"],[type=\"submit\"]{-webkit-appearance:button}button::-moz-focus-inner,[type=\"button\"]::-moz-focus-inner,[type=\"reset\"]::-moz-focus-inner,[type=\"submit\"]::-moz-focus-inner{border-style:none;padding:0}button:-moz-focusring,[type=\"button\"]:-moz-focusring,[type=\"reset\"]:-moz-focusring,[type=\"submit\"]:-moz-focusring{outline:1px dotted ButtonText}fieldset{border:1px solid #c0c0c0;margin:0 2px;padding:0.35em 0.625em 0.75em}legend{box-sizing:border-box;color:inherit;display:table;max-width:100%;padding:0;white-space:normal}progress{display:inline-block;vertical-align:baseline}textarea{overflow:auto}[type=\"checkbox\"],[type=\"radio\"]{box-sizing:border-box;padding:0}[type=\"number\"]::-webkit-inner-spin-button,[type=\"number\"]::-webkit-outer-spin-button{height:auto}[type=\"search\"]{-webkit-appearance:textfield;outline-offset:-2px}[type=\"search\"]::-webkit-search-cancel-button,[type=\"search\"]::-webkit-search-decoration{-webkit-appearance:none}::-webkit-file-upload-button{-webkit-appearance:button;font:inherit}details,menu{display:block}summary{display:list-item}canvas{display:inline-block}template{display:none}[hidden]{display:none}@media print{*,*::before,*::after,p::first-letter,div::first-letter,blockquote::first-letter,li::first-letter,p::first-line,div::first-line,blockquote::first-line,li::first-line{text-shadow:none !important;box-shadow:none !important}a,a:visited{text-decoration:underline}abbr[title]::after{content:\" (\" attr(title) \")\"}pre{white-space:pre-wrap !important}pre,blockquote{border:1px solid #999;page-break-inside:avoid}thead{display:table-header-group}tr,img{page-break-inside:avoid}p,h2,h3{orphans:3;widows:3}h2,h3{page-break-after:avoid}.navbar{display:none}.badge{border:1px solid #000}.table{border-collapse:collapse !important}.table td,.table th{background-color:#fff !important}.table-bordered th,.table-bordered td{border:1px solid #ddd !important}}html{box-sizing:border-box}*,*::before,*::after{box-sizing:inherit}@-ms-viewport{width:device-width}html{-ms-overflow-style:scrollbar;-webkit-tap-highlight-color:transparent}body{font-family:-apple-system,system-ui,BlinkMacSystemFont,\"Segoe UI\",Roboto,\"Helvetica Neue\",Arial,sans-serif;font-size:1rem;font-weight:normal;line-height:1.5;color:#292b2c;background-color:#fff}[tabindex=\"-1\"]:focus{outline:none !important}h1,h2,h3,h4,h5,h6{margin-top:0;margin-bottom:.5rem}p{margin-top:0;margin-bottom:1rem}abbr[title],abbr[data-original-title]{cursor:help}address{margin-bottom:1rem;font-style:normal;line-height:inherit}ol,ul,dl{margin-top:0;margin-bottom:1rem}ol ol,ul ul,ol ul,ul ol{margin-bottom:0}dt{font-weight:bold}dd{margin-bottom:.5rem;margin-left:0}blockquote{margin:0 0 1rem}a{color:#0275d8;text-decoration:none}a:focus,a:hover{color:#014c8c;text-decoration:underline}a:not([href]):not([tabindex]){color:inherit;text-decoration:none}a:not([href]):not([tabindex]):focus,a:not([href]):not([tabindex]):hover{color:inherit;text-decoration:none}a:not([href]):not([tabindex]):focus{outline:0}pre{margin-top:0;margin-bottom:1rem;overflow:auto}figure{margin:0 0 1rem}img{vertical-align:middle}[role=\"button\"]{cursor:pointer}a,area,button,[role=\"button\"],input,label,select,summary,textarea{-ms-touch-action:manipulation;touch-action:manipulation}table{border-collapse:collapse;background-color:transparent}caption{padding-top:.75rem;padding-bottom:.75rem;color:#636c72;text-align:left;caption-side:bottom}th{text-align:left}label{display:inline-block;margin-bottom:.5rem}button:focus{outline:1px dotted;outline:5px auto -webkit-focus-ring-color}input,button,select,textarea{line-height:inherit}input[type=\"radio\"]:disabled,input[type=\"checkbox\"]:disabled{cursor:not-allowed}input[type=\"date\"],input[type=\"time\"],input[type=\"datetime-local\"],input[type=\"month\"]{-webkit-appearance:listbox}textarea{resize:vertical}fieldset{min-width:0;padding:0;margin:0;border:0}legend{display:block;width:100%;padding:0;margin-bottom:.5rem;font-size:1.5rem;line-height:inherit}input[type=\"search\"]{-webkit-appearance:none}output{display:inline-block}[hidden]{display:none !important}h1,h2,h3,h4,h5,h6,.h1,.h2,.h3,.h4,.h5,.h6{margin-bottom:.5rem;font-family:inherit;font-weight:500;line-height:1.1;color:inherit}h1,.h1{font-size:2.5rem}h2,.h2{font-size:2rem}h3,.h3{font-size:1.75rem}h4,.h4{font-size:1.5rem}h5,.h5{font-size:1.25rem}h6,.h6{font-size:1rem}.lead{font-size:1.25rem;font-weight:300}.display-1{font-size:6rem;font-weight:300;line-height:1.1}.display-2{font-size:5.5rem;font-weight:300;line-height:1.1}.display-3{font-size:4.5rem;font-weight:300;line-height:1.1}.display-4{font-size:3.5rem;font-weight:300;line-height:1.1}hr{margin-top:1rem;margin-bottom:1rem;border:0;border-top:1px solid rgba(0,0,0,0.1)}small,.small{font-size:80%;font-weight:normal}mark,.mark{padding:.2em;background-color:#fcf8e3}.list-unstyled{padding-left:0;list-style:none}.list-inline{padding-left:0;list-style:none}.list-inline-item{display:inline-block}.list-inline-item:not(:last-child){margin-right:5px}.initialism{font-size:90%;text-transform:uppercase}.blockquote{padding:.5rem 1rem;margin-bottom:1rem;font-size:1.25rem;border-left:.25rem solid #eceeef}.blockquote-footer{display:block;font-size:80%;color:#636c72}.blockquote-footer::before{content:\"\\2014 \\x000A0\"}.blockquote-reverse{padding-right:1rem;padding-left:0;text-align:right;border-right:.25rem solid #eceeef;border-left:0}.blockquote-reverse .blockquote-footer::before{content:\"\"}.blockquote-reverse .blockquote-footer::after{content:\"\\x000A0 \\2014\"}.img-fluid{max-width:100%;height:auto}.img-thumbnail{padding:.25rem;background-color:#fff;border:1px solid #ddd;border-radius:.25rem;transition:all 0.2s ease-in-out;max-width:100%;height:auto}.figure{display:inline-block}.figure-img{margin-bottom:.5rem;line-height:1}.figure-caption{font-size:90%;color:#636c72}code,kbd,pre,samp{font-family:Menlo,Monaco,Consolas,\"Liberation Mono\",\"Courier New\",monospace}code{padding:.2rem .4rem;font-size:90%;color:#bd4147;background-color:#f7f7f9;border-radius:.25rem}a>code{padding:0;color:inherit;background-color:inherit}kbd{padding:.2rem .4rem;font-size:90%;color:#fff;background-color:#292b2c;border-radius:.2rem}kbd kbd{padding:0;font-size:100%;font-weight:bold}pre{display:block;margin-top:0;margin-bottom:1rem;font-size:90%;color:#292b2c}pre code{padding:0;font-size:inherit;color:inherit;background-color:transparent;border-radius:0}.pre-scrollable{max-height:340px;overflow-y:scroll}.container{position:relative;margin-left:auto;margin-right:auto;padding-right:15px;padding-left:15px}@media (min-width: 576px){.container{padding-right:15px;padding-left:15px}}@media (min-width: 768px){.container{padding-right:15px;padding-left:15px}}@media (min-width: 992px){.container{padding-right:15px;padding-left:15px}}@media (min-width: 1200px){.container{padding-right:15px;padding-left:15px}}@media (min-width: 576px){.container{width:540px;max-width:100%}}@media (min-width: 768px){.container{width:720px;max-width:100%}}@media (min-width: 992px){.container{width:960px;max-width:100%}}@media (min-width: 1200px){.container{width:1140px;max-width:100%}}.container-fluid{position:relative;margin-left:auto;margin-right:auto;padding-right:15px;padding-left:15px}@media (min-width: 576px){.container-fluid{padding-right:15px;padding-left:15px}}@media (min-width: 768px){.container-fluid{padding-right:15px;padding-left:15px}}@media (min-width: 992px){.container-fluid{padding-right:15px;padding-left:15px}}@media (min-width: 1200px){.container-fluid{padding-right:15px;padding-left:15px}}.row{display:-ms-flexbox;display:flex;-ms-flex-wrap:wrap;flex-wrap:wrap;margin-right:-15px;margin-left:-15px}@media (min-width: 576px){.row{margin-right:-15px;margin-left:-15px}}@media (min-width: 768px){.row{margin-right:-15px;margin-left:-15px}}@media (min-width: 992px){.row{margin-right:-15px;margin-left:-15px}}@media (min-width: 1200px){.row{margin-right:-15px;margin-left:-15px}}.no-gutters{margin-right:0;margin-left:0}.no-gutters>.col,.no-gutters>[class*=\"col-\"]{padding-right:0;padding-left:0}.col-1,.col-2,.col-3,.col-4,.col-5,.col-6,.col-7,.col-8,.col-9,.col-10,.col-11,.col-12,.col,.col-sm-1,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm,.col-md-1,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9,.col-md-10,.col-md-11,.col-md-12,.col-md,.col-lg-1,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg,.col-xl-1,.col-xl-2,.col-xl-3,.col-xl-4,.col-xl-5,.col-xl-6,.col-xl-7,.col-xl-8,.col-xl-9,.col-xl-10,.col-xl-11,.col-xl-12,.col-xl{position:relative;width:100%;min-height:1px;padding-right:15px;padding-left:15px}@media (min-width: 576px){.col-1,.col-2,.col-3,.col-4,.col-5,.col-6,.col-7,.col-8,.col-9,.col-10,.col-11,.col-12,.col,.col-sm-1,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm,.col-md-1,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9,.col-md-10,.col-md-11,.col-md-12,.col-md,.col-lg-1,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg,.col-xl-1,.col-xl-2,.col-xl-3,.col-xl-4,.col-xl-5,.col-xl-6,.col-xl-7,.col-xl-8,.col-xl-9,.col-xl-10,.col-xl-11,.col-xl-12,.col-xl{padding-right:15px;padding-left:15px}}@media (min-width: 768px){.col-1,.col-2,.col-3,.col-4,.col-5,.col-6,.col-7,.col-8,.col-9,.col-10,.col-11,.col-12,.col,.col-sm-1,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm,.col-md-1,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9,.col-md-10,.col-md-11,.col-md-12,.col-md,.col-lg-1,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg,.col-xl-1,.col-xl-2,.col-xl-3,.col-xl-4,.col-xl-5,.col-xl-6,.col-xl-7,.col-xl-8,.col-xl-9,.col-xl-10,.col-xl-11,.col-xl-12,.col-xl{padding-right:15px;padding-left:15px}}@media (min-width: 992px){.col-1,.col-2,.col-3,.col-4,.col-5,.col-6,.col-7,.col-8,.col-9,.col-10,.col-11,.col-12,.col,.col-sm-1,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm,.col-md-1,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9,.col-md-10,.col-md-11,.col-md-12,.col-md,.col-lg-1,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg,.col-xl-1,.col-xl-2,.col-xl-3,.col-xl-4,.col-xl-5,.col-xl-6,.col-xl-7,.col-xl-8,.col-xl-9,.col-xl-10,.col-xl-11,.col-xl-12,.col-xl{padding-right:15px;padding-left:15px}}@media (min-width: 1200px){.col-1,.col-2,.col-3,.col-4,.col-5,.col-6,.col-7,.col-8,.col-9,.col-10,.col-11,.col-12,.col,.col-sm-1,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm,.col-md-1,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9,.col-md-10,.col-md-11,.col-md-12,.col-md,.col-lg-1,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg,.col-xl-1,.col-xl-2,.col-xl-3,.col-xl-4,.col-xl-5,.col-xl-6,.col-xl-7,.col-xl-8,.col-xl-9,.col-xl-10,.col-xl-11,.col-xl-12,.col-xl{padding-right:15px;padding-left:15px}}.col{-ms-flex-preferred-size:0;flex-basis:0;-ms-flex-positive:1;flex-grow:1;max-width:100%}.col-auto{-ms-flex:0 0 auto;flex:0 0 auto;width:auto}.col-1{-ms-flex:0 0 8.33333%;flex:0 0 8.33333%;max-width:8.33333%}.col-2{-ms-flex:0 0 16.66667%;flex:0 0 16.66667%;max-width:16.66667%}.col-3{-ms-flex:0 0 25%;flex:0 0 25%;max-width:25%}.col-4{-ms-flex:0 0 33.33333%;flex:0 0 33.33333%;max-width:33.33333%}.col-5{-ms-flex:0 0 41.66667%;flex:0 0 41.66667%;max-width:41.66667%}.col-6{-ms-flex:0 0 50%;flex:0 0 50%;max-width:50%}.col-7{-ms-flex:0 0 58.33333%;flex:0 0 58.33333%;max-width:58.33333%}.col-8{-ms-flex:0 0 66.66667%;flex:0 0 66.66667%;max-width:66.66667%}.col-9{-ms-flex:0 0 75%;flex:0 0 75%;max-width:75%}.col-10{-ms-flex:0 0 83.33333%;flex:0 0 83.33333%;max-width:83.33333%}.col-11{-ms-flex:0 0 91.66667%;flex:0 0 91.66667%;max-width:91.66667%}.col-12{-ms-flex:0 0 100%;flex:0 0 100%;max-width:100%}.pull-0{right:auto}.pull-1{right:8.33333%}.pull-2{right:16.66667%}.pull-3{right:25%}.pull-4{right:33.33333%}.pull-5{right:41.66667%}.pull-6{right:50%}.pull-7{right:58.33333%}.pull-8{right:66.66667%}.pull-9{right:75%}.pull-10{right:83.33333%}.pull-11{right:91.66667%}.pull-12{right:100%}.push-0{left:auto}.push-1{left:8.33333%}.push-2{left:16.66667%}.push-3{left:25%}.push-4{left:33.33333%}.push-5{left:41.66667%}.push-6{left:50%}.push-7{left:58.33333%}.push-8{left:66.66667%}.push-9{left:75%}.push-10{left:83.33333%}.push-11{left:91.66667%}.push-12{left:100%}.offset-1{margin-left:8.33333%}.offset-2{margin-left:16.66667%}.offset-3{margin-left:25%}.offset-4{margin-left:33.33333%}.offset-5{margin-left:41.66667%}.offset-6{margin-left:50%}.offset-7{margin-left:58.33333%}.offset-8{margin-left:66.66667%}.offset-9{margin-left:75%}.offset-10{margin-left:83.33333%}.offset-11{margin-left:91.66667%}@media (min-width: 576px){.col-sm{-ms-flex-preferred-size:0;flex-basis:0;-ms-flex-positive:1;flex-grow:1;max-width:100%}.col-sm-auto{-ms-flex:0 0 auto;flex:0 0 auto;width:auto}.col-sm-1{-ms-flex:0 0 8.33333%;flex:0 0 8.33333%;max-width:8.33333%}.col-sm-2{-ms-flex:0 0 16.66667%;flex:0 0 16.66667%;max-width:16.66667%}.col-sm-3{-ms-flex:0 0 25%;flex:0 0 25%;max-width:25%}.col-sm-4{-ms-flex:0 0 33.33333%;flex:0 0 33.33333%;max-width:33.33333%}.col-sm-5{-ms-flex:0 0 41.66667%;flex:0 0 41.66667%;max-width:41.66667%}.col-sm-6{-ms-flex:0 0 50%;flex:0 0 50%;max-width:50%}.col-sm-7{-ms-flex:0 0 58.33333%;flex:0 0 58.33333%;max-width:58.33333%}.col-sm-8{-ms-flex:0 0 66.66667%;flex:0 0 66.66667%;max-width:66.66667%}.col-sm-9{-ms-flex:0 0 75%;flex:0 0 75%;max-width:75%}.col-sm-10{-ms-flex:0 0 83.33333%;flex:0 0 83.33333%;max-width:83.33333%}.col-sm-11{-ms-flex:0 0 91.66667%;flex:0 0 91.66667%;max-width:91.66667%}.col-sm-12{-ms-flex:0 0 100%;flex:0 0 100%;max-width:100%}.pull-sm-0{right:auto}.pull-sm-1{right:8.33333%}.pull-sm-2{right:16.66667%}.pull-sm-3{right:25%}.pull-sm-4{right:33.33333%}.pull-sm-5{right:41.66667%}.pull-sm-6{right:50%}.pull-sm-7{right:58.33333%}.pull-sm-8{right:66.66667%}.pull-sm-9{right:75%}.pull-sm-10{right:83.33333%}.pull-sm-11{right:91.66667%}.pull-sm-12{right:100%}.push-sm-0{left:auto}.push-sm-1{left:8.33333%}.push-sm-2{left:16.66667%}.push-sm-3{left:25%}.push-sm-4{left:33.33333%}.push-sm-5{left:41.66667%}.push-sm-6{left:50%}.push-sm-7{left:58.33333%}.push-sm-8{left:66.66667%}.push-sm-9{left:75%}.push-sm-10{left:83.33333%}.push-sm-11{left:91.66667%}.push-sm-12{left:100%}.offset-sm-0{margin-left:0%}.offset-sm-1{margin-left:8.33333%}.offset-sm-2{margin-left:16.66667%}.offset-sm-3{margin-left:25%}.offset-sm-4{margin-left:33.33333%}.offset-sm-5{margin-left:41.66667%}.offset-sm-6{margin-left:50%}.offset-sm-7{margin-left:58.33333%}.offset-sm-8{margin-left:66.66667%}.offset-sm-9{margin-left:75%}.offset-sm-10{margin-left:83.33333%}.offset-sm-11{margin-left:91.66667%}}@media (min-width: 768px){.col-md{-ms-flex-preferred-size:0;flex-basis:0;-ms-flex-positive:1;flex-grow:1;max-width:100%}.col-md-auto{-ms-flex:0 0 auto;flex:0 0 auto;width:auto}.col-md-1{-ms-flex:0 0 8.33333%;flex:0 0 8.33333%;max-width:8.33333%}.col-md-2{-ms-flex:0 0 16.66667%;flex:0 0 16.66667%;max-width:16.66667%}.col-md-3{-ms-flex:0 0 25%;flex:0 0 25%;max-width:25%}.col-md-4{-ms-flex:0 0 33.33333%;flex:0 0 33.33333%;max-width:33.33333%}.col-md-5{-ms-flex:0 0 41.66667%;flex:0 0 41.66667%;max-width:41.66667%}.col-md-6{-ms-flex:0 0 50%;flex:0 0 50%;max-width:50%}.col-md-7{-ms-flex:0 0 58.33333%;flex:0 0 58.33333%;max-width:58.33333%}.col-md-8{-ms-flex:0 0 66.66667%;flex:0 0 66.66667%;max-width:66.66667%}.col-md-9{-ms-flex:0 0 75%;flex:0 0 75%;max-width:75%}.col-md-10{-ms-flex:0 0 83.33333%;flex:0 0 83.33333%;max-width:83.33333%}.col-md-11{-ms-flex:0 0 91.66667%;flex:0 0 91.66667%;max-width:91.66667%}.col-md-12{-ms-flex:0 0 100%;flex:0 0 100%;max-width:100%}.pull-md-0{right:auto}.pull-md-1{right:8.33333%}.pull-md-2{right:16.66667%}.pull-md-3{right:25%}.pull-md-4{right:33.33333%}.pull-md-5{right:41.66667%}.pull-md-6{right:50%}.pull-md-7{right:58.33333%}.pull-md-8{right:66.66667%}.pull-md-9{right:75%}.pull-md-10{right:83.33333%}.pull-md-11{right:91.66667%}.pull-md-12{right:100%}.push-md-0{left:auto}.push-md-1{left:8.33333%}.push-md-2{left:16.66667%}.push-md-3{left:25%}.push-md-4{left:33.33333%}.push-md-5{left:41.66667%}.push-md-6{left:50%}.push-md-7{left:58.33333%}.push-md-8{left:66.66667%}.push-md-9{left:75%}.push-md-10{left:83.33333%}.push-md-11{left:91.66667%}.push-md-12{left:100%}.offset-md-0{margin-left:0%}.offset-md-1{margin-left:8.33333%}.offset-md-2{margin-left:16.66667%}.offset-md-3{margin-left:25%}.offset-md-4{margin-left:33.33333%}.offset-md-5{margin-left:41.66667%}.offset-md-6{margin-left:50%}.offset-md-7{margin-left:58.33333%}.offset-md-8{margin-left:66.66667%}.offset-md-9{margin-left:75%}.offset-md-10{margin-left:83.33333%}.offset-md-11{margin-left:91.66667%}}@media (min-width: 992px){.col-lg{-ms-flex-preferred-size:0;flex-basis:0;-ms-flex-positive:1;flex-grow:1;max-width:100%}.col-lg-auto{-ms-flex:0 0 auto;flex:0 0 auto;width:auto}.col-lg-1{-ms-flex:0 0 8.33333%;flex:0 0 8.33333%;max-width:8.33333%}.col-lg-2{-ms-flex:0 0 16.66667%;flex:0 0 16.66667%;max-width:16.66667%}.col-lg-3{-ms-flex:0 0 25%;flex:0 0 25%;max-width:25%}.col-lg-4{-ms-flex:0 0 33.33333%;flex:0 0 33.33333%;max-width:33.33333%}.col-lg-5{-ms-flex:0 0 41.66667%;flex:0 0 41.66667%;max-width:41.66667%}.col-lg-6{-ms-flex:0 0 50%;flex:0 0 50%;max-width:50%}.col-lg-7{-ms-flex:0 0 58.33333%;flex:0 0 58.33333%;max-width:58.33333%}.col-lg-8{-ms-flex:0 0 66.66667%;flex:0 0 66.66667%;max-width:66.66667%}.col-lg-9{-ms-flex:0 0 75%;flex:0 0 75%;max-width:75%}.col-lg-10{-ms-flex:0 0 83.33333%;flex:0 0 83.33333%;max-width:83.33333%}.col-lg-11{-ms-flex:0 0 91.66667%;flex:0 0 91.66667%;max-width:91.66667%}.col-lg-12{-ms-flex:0 0 100%;flex:0 0 100%;max-width:100%}.pull-lg-0{right:auto}.pull-lg-1{right:8.33333%}.pull-lg-2{right:16.66667%}.pull-lg-3{right:25%}.pull-lg-4{right:33.33333%}.pull-lg-5{right:41.66667%}.pull-lg-6{right:50%}.pull-lg-7{right:58.33333%}.pull-lg-8{right:66.66667%}.pull-lg-9{right:75%}.pull-lg-10{right:83.33333%}.pull-lg-11{right:91.66667%}.pull-lg-12{right:100%}.push-lg-0{left:auto}.push-lg-1{left:8.33333%}.push-lg-2{left:16.66667%}.push-lg-3{left:25%}.push-lg-4{left:33.33333%}.push-lg-5{left:41.66667%}.push-lg-6{left:50%}.push-lg-7{left:58.33333%}.push-lg-8{left:66.66667%}.push-lg-9{left:75%}.push-lg-10{left:83.33333%}.push-lg-11{left:91.66667%}.push-lg-12{left:100%}.offset-lg-0{margin-left:0%}.offset-lg-1{margin-left:8.33333%}.offset-lg-2{margin-left:16.66667%}.offset-lg-3{margin-left:25%}.offset-lg-4{margin-left:33.33333%}.offset-lg-5{margin-left:41.66667%}.offset-lg-6{margin-left:50%}.offset-lg-7{margin-left:58.33333%}.offset-lg-8{margin-left:66.66667%}.offset-lg-9{margin-left:75%}.offset-lg-10{margin-left:83.33333%}.offset-lg-11{margin-left:91.66667%}}@media (min-width: 1200px){.col-xl{-ms-flex-preferred-size:0;flex-basis:0;-ms-flex-positive:1;flex-grow:1;max-width:100%}.col-xl-auto{-ms-flex:0 0 auto;flex:0 0 auto;width:auto}.col-xl-1{-ms-flex:0 0 8.33333%;flex:0 0 8.33333%;max-width:8.33333%}.col-xl-2{-ms-flex:0 0 16.66667%;flex:0 0 16.66667%;max-width:16.66667%}.col-xl-3{-ms-flex:0 0 25%;flex:0 0 25%;max-width:25%}.col-xl-4{-ms-flex:0 0 33.33333%;flex:0 0 33.33333%;max-width:33.33333%}.col-xl-5{-ms-flex:0 0 41.66667%;flex:0 0 41.66667%;max-width:41.66667%}.col-xl-6{-ms-flex:0 0 50%;flex:0 0 50%;max-width:50%}.col-xl-7{-ms-flex:0 0 58.33333%;flex:0 0 58.33333%;max-width:58.33333%}.col-xl-8{-ms-flex:0 0 66.66667%;flex:0 0 66.66667%;max-width:66.66667%}.col-xl-9{-ms-flex:0 0 75%;flex:0 0 75%;max-width:75%}.col-xl-10{-ms-flex:0 0 83.33333%;flex:0 0 83.33333%;max-width:83.33333%}.col-xl-11{-ms-flex:0 0 91.66667%;flex:0 0 91.66667%;max-width:91.66667%}.col-xl-12{-ms-flex:0 0 100%;flex:0 0 100%;max-width:100%}.pull-xl-0{right:auto}.pull-xl-1{right:8.33333%}.pull-xl-2{right:16.66667%}.pull-xl-3{right:25%}.pull-xl-4{right:33.33333%}.pull-xl-5{right:41.66667%}.pull-xl-6{right:50%}.pull-xl-7{right:58.33333%}.pull-xl-8{right:66.66667%}.pull-xl-9{right:75%}.pull-xl-10{right:83.33333%}.pull-xl-11{right:91.66667%}.pull-xl-12{right:100%}.push-xl-0{left:auto}.push-xl-1{left:8.33333%}.push-xl-2{left:16.66667%}.push-xl-3{left:25%}.push-xl-4{left:33.33333%}.push-xl-5{left:41.66667%}.push-xl-6{left:50%}.push-xl-7{left:58.33333%}.push-xl-8{left:66.66667%}.push-xl-9{left:75%}.push-xl-10{left:83.33333%}.push-xl-11{left:91.66667%}.push-xl-12{left:100%}.offset-xl-0{margin-left:0%}.offset-xl-1{margin-left:8.33333%}.offset-xl-2{margin-left:16.66667%}.offset-xl-3{margin-left:25%}.offset-xl-4{margin-left:33.33333%}.offset-xl-5{margin-left:41.66667%}.offset-xl-6{margin-left:50%}.offset-xl-7{margin-left:58.33333%}.offset-xl-8{margin-left:66.66667%}.offset-xl-9{margin-left:75%}.offset-xl-10{margin-left:83.33333%}.offset-xl-11{margin-left:91.66667%}}.table{width:100%;max-width:100%;margin-bottom:1rem}.table th,.table td{padding:.75rem;vertical-align:top;border-top:1px solid #eceeef}.table thead th{vertical-align:bottom;border-bottom:2px solid #eceeef}.table tbody+tbody{border-top:2px solid #eceeef}.table .table{background-color:#fff}.table-sm th,.table-sm td{padding:.3rem}.table-bordered{border:1px solid #eceeef}.table-bordered th,.table-bordered td{border:1px solid #eceeef}.table-bordered thead th,.table-bordered thead td{border-bottom-width:2px}.table-striped tbody tr:nth-of-type(odd){background-color:rgba(0,0,0,0.05)}.table-hover tbody tr:hover{background-color:rgba(0,0,0,0.075)}.table-active,.table-active>th,.table-active>td{background-color:rgba(0,0,0,0.075)}.table-hover .table-active:hover{background-color:rgba(0,0,0,0.075)}.table-hover .table-active:hover>td,.table-hover .table-active:hover>th{background-color:rgba(0,0,0,0.075)}.table-success,.table-success>th,.table-success>td{background-color:#dff0d8}.table-hover .table-success:hover{background-color:#d0e9c6}.table-hover .table-success:hover>td,.table-hover .table-success:hover>th{background-color:#d0e9c6}.table-info,.table-info>th,.table-info>td{background-color:#d9edf7}.table-hover .table-info:hover{background-color:#c4e3f3}.table-hover .table-info:hover>td,.table-hover .table-info:hover>th{background-color:#c4e3f3}.table-warning,.table-warning>th,.table-warning>td{background-color:#fcf8e3}.table-hover .table-warning:hover{background-color:#faf2cc}.table-hover .table-warning:hover>td,.table-hover .table-warning:hover>th{background-color:#faf2cc}.table-danger,.table-danger>th,.table-danger>td{background-color:#f2dede}.table-hover .table-danger:hover{background-color:#ebcccc}.table-hover .table-danger:hover>td,.table-hover .table-danger:hover>th{background-color:#ebcccc}.thead-inverse th{color:#fff;background-color:#292b2c}.thead-default th{color:#464a4c;background-color:#eceeef}.table-inverse{color:#fff;background-color:#292b2c}.table-inverse th,.table-inverse td,.table-inverse thead th{border-color:#fff}.table-inverse.table-bordered{border:0}.table-responsive{display:block;width:100%;overflow-x:auto;-ms-overflow-style:-ms-autohiding-scrollbar}.table-responsive.table-bordered{border:0}.form-control{display:block;width:100%;padding:.5rem .75rem;font-size:1rem;line-height:1.25;color:#464a4c;background-color:#fff;background-image:none;background-clip:padding-box;border:1px solid rgba(0,0,0,0.15);border-radius:.25rem;transition:border-color ease-in-out 0.15s,box-shadow ease-in-out 0.15s}.form-control::-ms-expand{background-color:transparent;border:0}.form-control:focus{color:#464a4c;background-color:#fff;border-color:#5cb3fd;outline:none}.form-control:-ms-input-placeholder{color:#636c72;opacity:1}.form-control::placeholder{color:#636c72;opacity:1}.form-control:disabled,.form-control[readonly]{background-color:#eceeef;opacity:1}.form-control:disabled{cursor:not-allowed}select.form-control:not([size]):not([multiple]){height:calc(2.25rem + 2px)}select.form-control:focus::-ms-value{color:#464a4c;background-color:#fff}.form-control-file,.form-control-range{display:block}.col-form-label{padding-top:calc(.5rem - 1px * 2);padding-bottom:calc(.5rem - 1px * 2);margin-bottom:0}.col-form-label-lg{padding-top:calc(.75rem - 1px * 2);padding-bottom:calc(.75rem - 1px * 2);font-size:1.25rem}.col-form-label-sm{padding-top:calc(.25rem - 1px * 2);padding-bottom:calc(.25rem - 1px * 2);font-size:.875rem}.col-form-legend{padding-top:.5rem;padding-bottom:.5rem;margin-bottom:0;font-size:1rem}.form-control-static{padding-top:.5rem;padding-bottom:.5rem;margin-bottom:0;line-height:1.25;border:solid transparent;border-width:1px 0}.form-control-static.form-control-sm,.input-group-sm>.form-control-static.form-control,.input-group-sm>.form-control-static.input-group-addon,.input-group-sm>.input-group-btn>.form-control-static.btn,.form-control-static.form-control-lg,.input-group-lg>.form-control-static.form-control,.input-group-lg>.form-control-static.input-group-addon,.input-group-lg>.input-group-btn>.form-control-static.btn{padding-right:0;padding-left:0}.form-control-sm,.input-group-sm>.form-control,.input-group-sm>.input-group-addon,.input-group-sm>.input-group-btn>.btn{padding:.25rem .5rem;font-size:.875rem;border-radius:.2rem}select.form-control-sm:not([size]):not([multiple]),.input-group-sm>select.form-control:not([size]):not([multiple]),.input-group-sm>select.input-group-addon:not([size]):not([multiple]),.input-group-sm>.input-group-btn>select.btn:not([size]):not([multiple]){height:1.8125rem}.form-control-lg,.input-group-lg>.form-control,.input-group-lg>.input-group-addon,.input-group-lg>.input-group-btn>.btn{padding:.75rem 1.5rem;font-size:1.25rem;border-radius:.3rem}select.form-control-lg:not([size]):not([multiple]),.input-group-lg>select.form-control:not([size]):not([multiple]),.input-group-lg>select.input-group-addon:not([size]):not([multiple]),.input-group-lg>.input-group-btn>select.btn:not([size]):not([multiple]){height:3.16667rem}.form-group{margin-bottom:1rem}.form-text{display:block;margin-top:.25rem}.form-check{position:relative;display:block;margin-bottom:.5rem}.form-check.disabled .form-check-label{color:#636c72;cursor:not-allowed}.form-check-label{padding-left:1.25rem;margin-bottom:0;cursor:pointer}.form-check-input{position:absolute;margin-top:.25rem;margin-left:-1.25rem}.form-check-input:only-child{position:static}.form-check-inline{display:inline-block}.form-check-inline .form-check-label{vertical-align:middle}.form-check-inline+.form-check-inline{margin-left:.75rem}.form-control-feedback{margin-top:.25rem}.form-control-success,.form-control-warning,.form-control-danger{padding-right:2.25rem;background-repeat:no-repeat;background-position:center right .5625rem;background-size:1.125rem 1.125rem}.has-success .form-control-feedback,.has-success .form-control-label,.has-success .col-form-label,.has-success .form-check-label,.has-success .custom-control{color:#5cb85c}.has-success .form-control{border-color:#5cb85c}.has-success .input-group-addon{color:#5cb85c;border-color:#5cb85c;background-color:#eaf6ea}.has-success .form-control-success{background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 8 8'%3E%3Cpath fill='%235cb85c' d='M2.3 6.73L.6 4.53c-.4-1.04.46-1.4 1.1-.8l1.1 1.4 3.4-3.8c.6-.63 1.6-.27 1.2.7l-4 4.6c-.43.5-.8.4-1.1.1z'/%3E%3C/svg%3E\")}.has-warning .form-control-feedback,.has-warning .form-control-label,.has-warning .col-form-label,.has-warning .form-check-label,.has-warning .custom-control{color:#f0ad4e}.has-warning .form-control{border-color:#f0ad4e}.has-warning .input-group-addon{color:#f0ad4e;border-color:#f0ad4e;background-color:#fff}.has-warning .form-control-warning{background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 8 8'%3E%3Cpath fill='%23f0ad4e' d='M4.4 5.324h-.8v-2.46h.8zm0 1.42h-.8V5.89h.8zM3.76.63L.04 7.075c-.115.2.016.425.26.426h7.397c.242 0 .372-.226.258-.426C6.726 4.924 5.47 2.79 4.253.63c-.113-.174-.39-.174-.494 0z'/%3E%3C/svg%3E\")}.has-danger .form-control-feedback,.has-danger .form-control-label,.has-danger .col-form-label,.has-danger .form-check-label,.has-danger .custom-control{color:#d9534f}.has-danger .form-control{border-color:#d9534f}.has-danger .input-group-addon{color:#d9534f;border-color:#d9534f;background-color:#fdf7f7}.has-danger .form-control-danger{background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='%23d9534f' viewBox='-2 -2 7 7'%3E%3Cpath stroke='%23d9534f' d='M0 0l3 3m0-3L0 3'/%3E%3Ccircle r='.5'/%3E%3Ccircle cx='3' r='.5'/%3E%3Ccircle cy='3' r='.5'/%3E%3Ccircle cx='3' cy='3' r='.5'/%3E%3C/svg%3E\")}.form-inline{display:-ms-flexbox;display:flex;-ms-flex-flow:row wrap;flex-flow:row wrap;-ms-flex-align:center;align-items:center}.form-inline .form-check{width:100%}@media (min-width: 576px){.form-inline label{display:-ms-flexbox;display:flex;-ms-flex-align:center;align-items:center;-ms-flex-pack:center;justify-content:center;margin-bottom:0}.form-inline .form-group{display:-ms-flexbox;display:flex;-ms-flex:0 0 auto;flex:0 0 auto;-ms-flex-flow:row wrap;flex-flow:row wrap;-ms-flex-align:center;align-items:center;margin-bottom:0}.form-inline .form-control{display:inline-block;width:auto;vertical-align:middle}.form-inline .form-control-static{display:inline-block}.form-inline .input-group{width:auto}.form-inline .form-control-label{margin-bottom:0;vertical-align:middle}.form-inline .form-check{display:-ms-flexbox;display:flex;-ms-flex-align:center;align-items:center;-ms-flex-pack:center;justify-content:center;width:auto;margin-top:0;margin-bottom:0}.form-inline .form-check-label{padding-left:0}.form-inline .form-check-input{position:relative;margin-top:0;margin-right:.25rem;margin-left:0}.form-inline .custom-control{display:-ms-flexbox;display:flex;-ms-flex-align:center;align-items:center;-ms-flex-pack:center;justify-content:center;padding-left:0}.form-inline .custom-control-indicator{position:static;display:inline-block;margin-right:.25rem;vertical-align:text-bottom}.form-inline .has-feedback .form-control-feedback{top:0}}.btn{display:inline-block;font-weight:normal;line-height:1.25;text-align:center;white-space:nowrap;vertical-align:middle;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;border:1px solid transparent;padding:.5rem 1rem;font-size:1rem;border-radius:.25rem;transition:all 0.2s ease-in-out}.btn:focus,.btn:hover{text-decoration:none}.btn:focus,.btn.focus{outline:0;box-shadow:0 0 0 2px rgba(2,117,216,0.25)}.btn.disabled,.btn:disabled{cursor:not-allowed;opacity:.65}.btn:active,.btn.active{background-image:none}a.btn.disabled,fieldset[disabled] a.btn{pointer-events:none}.btn-primary{color:#fff;background-color:#0275d8;border-color:#0275d8}.btn-primary:hover{color:#fff;background-color:#025aa5;border-color:#01549b}.btn-primary:focus,.btn-primary.focus{box-shadow:0 0 0 2px rgba(2,117,216,0.5)}.btn-primary.disabled,.btn-primary:disabled{background-color:#0275d8;border-color:#0275d8}.btn-primary:active,.btn-primary.active,.show>.btn-primary.dropdown-toggle{color:#fff;background-color:#025aa5;background-image:none;border-color:#01549b}.btn-secondary{color:#292b2c;background-color:#fff;border-color:#ccc}.btn-secondary:hover{color:#292b2c;background-color:#e6e6e6;border-color:#adadad}.btn-secondary:focus,.btn-secondary.focus{box-shadow:0 0 0 2px rgba(204,204,204,0.5)}.btn-secondary.disabled,.btn-secondary:disabled{background-color:#fff;border-color:#ccc}.btn-secondary:active,.btn-secondary.active,.show>.btn-secondary.dropdown-toggle{color:#292b2c;background-color:#e6e6e6;background-image:none;border-color:#adadad}.btn-info{color:#fff;background-color:#5bc0de;border-color:#5bc0de}.btn-info:hover{color:#fff;background-color:#31b0d5;border-color:#2aabd2}.btn-info:focus,.btn-info.focus{box-shadow:0 0 0 2px rgba(91,192,222,0.5)}.btn-info.disabled,.btn-info:disabled{background-color:#5bc0de;border-color:#5bc0de}.btn-info:active,.btn-info.active,.show>.btn-info.dropdown-toggle{color:#fff;background-color:#31b0d5;background-image:none;border-color:#2aabd2}.btn-success{color:#fff;background-color:#5cb85c;border-color:#5cb85c}.btn-success:hover{color:#fff;background-color:#449d44;border-color:#419641}.btn-success:focus,.btn-success.focus{box-shadow:0 0 0 2px rgba(92,184,92,0.5)}.btn-success.disabled,.btn-success:disabled{background-color:#5cb85c;border-color:#5cb85c}.btn-success:active,.btn-success.active,.show>.btn-success.dropdown-toggle{color:#fff;background-color:#449d44;background-image:none;border-color:#419641}.btn-warning{color:#fff;background-color:#f0ad4e;border-color:#f0ad4e}.btn-warning:hover{color:#fff;background-color:#ec971f;border-color:#eb9316}.btn-warning:focus,.btn-warning.focus{box-shadow:0 0 0 2px rgba(240,173,78,0.5)}.btn-warning.disabled,.btn-warning:disabled{background-color:#f0ad4e;border-color:#f0ad4e}.btn-warning:active,.btn-warning.active,.show>.btn-warning.dropdown-toggle{color:#fff;background-color:#ec971f;background-image:none;border-color:#eb9316}.btn-danger{color:#fff;background-color:#d9534f;border-color:#d9534f}.btn-danger:hover{color:#fff;background-color:#c9302c;border-color:#c12e2a}.btn-danger:focus,.btn-danger.focus{box-shadow:0 0 0 2px rgba(217,83,79,0.5)}.btn-danger.disabled,.btn-danger:disabled{background-color:#d9534f;border-color:#d9534f}.btn-danger:active,.btn-danger.active,.show>.btn-danger.dropdown-toggle{color:#fff;background-color:#c9302c;background-image:none;border-color:#c12e2a}.btn-outline-primary{color:#0275d8;background-image:none;background-color:transparent;border-color:#0275d8}.btn-outline-primary:hover{color:#fff;background-color:#0275d8;border-color:#0275d8}.btn-outline-primary:focus,.btn-outline-primary.focus{box-shadow:0 0 0 2px rgba(2,117,216,0.5)}.btn-outline-primary.disabled,.btn-outline-primary:disabled{color:#0275d8;background-color:transparent}.btn-outline-primary:active,.btn-outline-primary.active,.show>.btn-outline-primary.dropdown-toggle{color:#fff;background-color:#0275d8;border-color:#0275d8}.btn-outline-secondary{color:#ccc;background-image:none;background-color:transparent;border-color:#ccc}.btn-outline-secondary:hover{color:#fff;background-color:#ccc;border-color:#ccc}.btn-outline-secondary:focus,.btn-outline-secondary.focus{box-shadow:0 0 0 2px rgba(204,204,204,0.5)}.btn-outline-secondary.disabled,.btn-outline-secondary:disabled{color:#ccc;background-color:transparent}.btn-outline-secondary:active,.btn-outline-secondary.active,.show>.btn-outline-secondary.dropdown-toggle{color:#fff;background-color:#ccc;border-color:#ccc}.btn-outline-info{color:#5bc0de;background-image:none;background-color:transparent;border-color:#5bc0de}.btn-outline-info:hover{color:#fff;background-color:#5bc0de;border-color:#5bc0de}.btn-outline-info:focus,.btn-outline-info.focus{box-shadow:0 0 0 2px rgba(91,192,222,0.5)}.btn-outline-info.disabled,.btn-outline-info:disabled{color:#5bc0de;background-color:transparent}.btn-outline-info:active,.btn-outline-info.active,.show>.btn-outline-info.dropdown-toggle{color:#fff;background-color:#5bc0de;border-color:#5bc0de}.btn-outline-success{color:#5cb85c;background-image:none;background-color:transparent;border-color:#5cb85c}.btn-outline-success:hover{color:#fff;background-color:#5cb85c;border-color:#5cb85c}.btn-outline-success:focus,.btn-outline-success.focus{box-shadow:0 0 0 2px rgba(92,184,92,0.5)}.btn-outline-success.disabled,.btn-outline-success:disabled{color:#5cb85c;background-color:transparent}.btn-outline-success:active,.btn-outline-success.active,.show>.btn-outline-success.dropdown-toggle{color:#fff;background-color:#5cb85c;border-color:#5cb85c}.btn-outline-warning{color:#f0ad4e;background-image:none;background-color:transparent;border-color:#f0ad4e}.btn-outline-warning:hover{color:#fff;background-color:#f0ad4e;border-color:#f0ad4e}.btn-outline-warning:focus,.btn-outline-warning.focus{box-shadow:0 0 0 2px rgba(240,173,78,0.5)}.btn-outline-warning.disabled,.btn-outline-warning:disabled{color:#f0ad4e;background-color:transparent}.btn-outline-warning:active,.btn-outline-warning.active,.show>.btn-outline-warning.dropdown-toggle{color:#fff;background-color:#f0ad4e;border-color:#f0ad4e}.btn-outline-danger{color:#d9534f;background-image:none;background-color:transparent;border-color:#d9534f}.btn-outline-danger:hover{color:#fff;background-color:#d9534f;border-color:#d9534f}.btn-outline-danger:focus,.btn-outline-danger.focus{box-shadow:0 0 0 2px rgba(217,83,79,0.5)}.btn-outline-danger.disabled,.btn-outline-danger:disabled{color:#d9534f;background-color:transparent}.btn-outline-danger:active,.btn-outline-danger.active,.show>.btn-outline-danger.dropdown-toggle{color:#fff;background-color:#d9534f;border-color:#d9534f}.btn-link{font-weight:normal;color:#0275d8;border-radius:0}.btn-link,.btn-link:active,.btn-link.active,.btn-link:disabled{background-color:transparent}.btn-link,.btn-link:focus,.btn-link:active{border-color:transparent}.btn-link:hover{border-color:transparent}.btn-link:focus,.btn-link:hover{color:#014c8c;text-decoration:underline;background-color:transparent}.btn-link:disabled{color:#636c72}.btn-link:disabled:focus,.btn-link:disabled:hover{text-decoration:none}.btn-lg,.btn-group-lg>.btn{padding:.75rem 1.5rem;font-size:1.25rem;border-radius:.3rem}.btn-sm,.btn-group-sm>.btn{padding:.25rem .5rem;font-size:.875rem;border-radius:.2rem}.btn-block{display:block;width:100%}.btn-block+.btn-block{margin-top:.5rem}input[type=\"submit\"].btn-block,input[type=\"reset\"].btn-block,input[type=\"button\"].btn-block{width:100%}.fade{opacity:0;transition:opacity 0.15s linear}.fade.show{opacity:1}.collapse{display:none}.collapse.show{display:block}tr.collapse.show{display:table-row}tbody.collapse.show{display:table-row-group}.collapsing{position:relative;height:0;overflow:hidden;transition:height 0.35s ease}.dropup,.dropdown{position:relative}.dropdown-toggle::after{display:inline-block;width:0;height:0;margin-left:.3em;vertical-align:middle;content:\"\";border-top:.3em solid;border-right:.3em solid transparent;border-left:.3em solid transparent}.dropdown-toggle:focus{outline:0}.dropup .dropdown-toggle::after{border-top:0;border-bottom:.3em solid}.dropdown-menu{position:absolute;top:100%;left:0;z-index:1000;display:none;float:left;min-width:10rem;padding:.5rem 0;margin:.125rem 0 0;font-size:1rem;color:#292b2c;text-align:left;list-style:none;background-color:#fff;background-clip:padding-box;border:1px solid rgba(0,0,0,0.15);border-radius:.25rem}.dropdown-divider{height:1px;margin:.5rem 0;overflow:hidden;background-color:#eceeef}.dropdown-item{display:block;width:100%;padding:3px 1.5rem;clear:both;font-weight:normal;color:#292b2c;text-align:inherit;white-space:nowrap;background:none;border:0}.dropdown-item:focus,.dropdown-item:hover{color:#1d1e1f;text-decoration:none;background-color:#f7f7f9}.dropdown-item.active,.dropdown-item:active{color:#fff;text-decoration:none;background-color:#0275d8}.dropdown-item.disabled,.dropdown-item:disabled{color:#636c72;cursor:not-allowed;background-color:transparent}.show>.dropdown-menu{display:block}.show>a{outline:0}.dropdown-menu-right{right:0;left:auto}.dropdown-menu-left{right:auto;left:0}.dropdown-header{display:block;padding:.5rem 1.5rem;margin-bottom:0;font-size:.875rem;color:#636c72;white-space:nowrap}.dropdown-backdrop{position:fixed;top:0;right:0;bottom:0;left:0;z-index:990}.dropup .dropdown-menu{top:auto;bottom:100%;margin-bottom:.125rem}.btn-group,.btn-group-vertical{position:relative;display:-ms-inline-flexbox;display:inline-flex;vertical-align:middle}.btn-group>.btn,.btn-group-vertical>.btn{position:relative;-ms-flex:0 1 auto;flex:0 1 auto}.btn-group>.btn:hover,.btn-group-vertical>.btn:hover{z-index:2}.btn-group>.btn:focus,.btn-group>.btn:active,.btn-group>.btn.active,.btn-group-vertical>.btn:focus,.btn-group-vertical>.btn:active,.btn-group-vertical>.btn.active{z-index:2}.btn-group .btn+.btn,.btn-group .btn+.btn-group,.btn-group .btn-group+.btn,.btn-group .btn-group+.btn-group,.btn-group-vertical .btn+.btn,.btn-group-vertical .btn+.btn-group,.btn-group-vertical .btn-group+.btn,.btn-group-vertical .btn-group+.btn-group{margin-left:-1px}.btn-toolbar{display:-ms-flexbox;display:flex;-ms-flex-pack:start;justify-content:flex-start}.btn-toolbar .input-group{width:auto}.btn-group>.btn:not(:first-child):not(:last-child):not(.dropdown-toggle){border-radius:0}.btn-group>.btn:first-child{margin-left:0}.btn-group>.btn:first-child:not(:last-child):not(.dropdown-toggle){border-bottom-right-radius:0;border-top-right-radius:0}.btn-group>.btn:last-child:not(:first-child),.btn-group>.dropdown-toggle:not(:first-child){border-bottom-left-radius:0;border-top-left-radius:0}.btn-group>.btn-group{float:left}.btn-group>.btn-group:not(:first-child):not(:last-child)>.btn{border-radius:0}.btn-group>.btn-group:first-child:not(:last-child)>.btn:last-child,.btn-group>.btn-group:first-child:not(:last-child)>.dropdown-toggle{border-bottom-right-radius:0;border-top-right-radius:0}.btn-group>.btn-group:last-child:not(:first-child)>.btn:first-child{border-bottom-left-radius:0;border-top-left-radius:0}.btn-group .dropdown-toggle:active,.btn-group.open .dropdown-toggle{outline:0}.btn+.dropdown-toggle-split{padding-right:.75rem;padding-left:.75rem}.btn+.dropdown-toggle-split::after{margin-left:0}.btn-sm+.dropdown-toggle-split,.btn-group-sm>.btn+.dropdown-toggle-split{padding-right:.375rem;padding-left:.375rem}.btn-lg+.dropdown-toggle-split,.btn-group-lg>.btn+.dropdown-toggle-split{padding-right:1.125rem;padding-left:1.125rem}.btn-group-vertical{display:-ms-inline-flexbox;display:inline-flex;-ms-flex-direction:column;flex-direction:column;-ms-flex-align:start;align-items:flex-start;-ms-flex-pack:center;justify-content:center}.btn-group-vertical .btn,.btn-group-vertical .btn-group{width:100%}.btn-group-vertical>.btn+.btn,.btn-group-vertical>.btn+.btn-group,.btn-group-vertical>.btn-group+.btn,.btn-group-vertical>.btn-group+.btn-group{margin-top:-1px;margin-left:0}.btn-group-vertical>.btn:not(:first-child):not(:last-child){border-radius:0}.btn-group-vertical>.btn:first-child:not(:last-child){border-bottom-right-radius:0;border-bottom-left-radius:0}.btn-group-vertical>.btn:last-child:not(:first-child){border-top-right-radius:0;border-top-left-radius:0}.btn-group-vertical>.btn-group:not(:first-child):not(:last-child)>.btn{border-radius:0}.btn-group-vertical>.btn-group:first-child:not(:last-child)>.btn:last-child,.btn-group-vertical>.btn-group:first-child:not(:last-child)>.dropdown-toggle{border-bottom-right-radius:0;border-bottom-left-radius:0}.btn-group-vertical>.btn-group:last-child:not(:first-child)>.btn:first-child{border-top-right-radius:0;border-top-left-radius:0}[data-toggle=\"buttons\"]>.btn input[type=\"radio\"],[data-toggle=\"buttons\"]>.btn input[type=\"checkbox\"],[data-toggle=\"buttons\"]>.btn-group>.btn input[type=\"radio\"],[data-toggle=\"buttons\"]>.btn-group>.btn input[type=\"checkbox\"]{position:absolute;clip:rect(0, 0, 0, 0);pointer-events:none}.input-group{position:relative;display:-ms-flexbox;display:flex;width:100%}.input-group .form-control{position:relative;z-index:2;-ms-flex:1 1 auto;flex:1 1 auto;width:1%;margin-bottom:0}.input-group .form-control:focus,.input-group .form-control:active,.input-group .form-control:hover{z-index:3}.input-group-addon,.input-group-btn,.input-group .form-control{display:-ms-flexbox;display:flex;-ms-flex-direction:column;flex-direction:column;-ms-flex-pack:center;justify-content:center}.input-group-addon:not(:first-child):not(:last-child),.input-group-btn:not(:first-child):not(:last-child),.input-group .form-control:not(:first-child):not(:last-child){border-radius:0}.input-group-addon,.input-group-btn{white-space:nowrap;vertical-align:middle}.input-group-addon{padding:.5rem .75rem;margin-bottom:0;font-size:1rem;font-weight:normal;line-height:1.25;color:#464a4c;text-align:center;background-color:#eceeef;border:1px solid rgba(0,0,0,0.15);border-radius:.25rem}.input-group-addon.form-control-sm,.input-group-sm>.input-group-addon,.input-group-sm>.input-group-btn>.input-group-addon.btn{padding:.25rem .5rem;font-size:.875rem;border-radius:.2rem}.input-group-addon.form-control-lg,.input-group-lg>.input-group-addon,.input-group-lg>.input-group-btn>.input-group-addon.btn{padding:.75rem 1.5rem;font-size:1.25rem;border-radius:.3rem}.input-group-addon input[type=\"radio\"],.input-group-addon input[type=\"checkbox\"]{margin-top:0}.input-group .form-control:not(:last-child),.input-group-addon:not(:last-child),.input-group-btn:not(:last-child)>.btn,.input-group-btn:not(:last-child)>.btn-group>.btn,.input-group-btn:not(:last-child)>.dropdown-toggle,.input-group-btn:not(:first-child)>.btn:not(:last-child):not(.dropdown-toggle),.input-group-btn:not(:first-child)>.btn-group:not(:last-child)>.btn{border-bottom-right-radius:0;border-top-right-radius:0}.input-group-addon:not(:last-child){border-right:0}.input-group .form-control:not(:first-child),.input-group-addon:not(:first-child),.input-group-btn:not(:first-child)>.btn,.input-group-btn:not(:first-child)>.btn-group>.btn,.input-group-btn:not(:first-child)>.dropdown-toggle,.input-group-btn:not(:last-child)>.btn:not(:first-child),.input-group-btn:not(:last-child)>.btn-group:not(:first-child)>.btn{border-bottom-left-radius:0;border-top-left-radius:0}.form-control+.input-group-addon:not(:first-child){border-left:0}.input-group-btn{position:relative;font-size:0;white-space:nowrap}.input-group-btn>.btn{position:relative;-ms-flex:1;flex:1}.input-group-btn>.btn+.btn{margin-left:-1px}.input-group-btn>.btn:focus,.input-group-btn>.btn:active,.input-group-btn>.btn:hover{z-index:3}.input-group-btn:not(:last-child)>.btn,.input-group-btn:not(:last-child)>.btn-group{margin-right:-1px}.input-group-btn:not(:first-child)>.btn,.input-group-btn:not(:first-child)>.btn-group{z-index:2;margin-left:-1px}.input-group-btn:not(:first-child)>.btn:focus,.input-group-btn:not(:first-child)>.btn:active,.input-group-btn:not(:first-child)>.btn:hover,.input-group-btn:not(:first-child)>.btn-group:focus,.input-group-btn:not(:first-child)>.btn-group:active,.input-group-btn:not(:first-child)>.btn-group:hover{z-index:3}.custom-control{position:relative;display:-ms-inline-flexbox;display:inline-flex;min-height:1.5rem;padding-left:1.5rem;margin-right:1rem;cursor:pointer}.custom-control-input{position:absolute;z-index:-1;opacity:0}.custom-control-input:checked ~ .custom-control-indicator{color:#fff;background-color:#0275d8}.custom-control-input:focus ~ .custom-control-indicator{box-shadow:0 0 0 1px #fff,0 0 0 3px #0275d8}.custom-control-input:active ~ .custom-control-indicator{color:#fff;background-color:#8fcafe}.custom-control-input:disabled ~ .custom-control-indicator{cursor:not-allowed;background-color:#eceeef}.custom-control-input:disabled ~ .custom-control-description{color:#636c72;cursor:not-allowed}.custom-control-indicator{position:absolute;top:.25rem;left:0;display:block;width:1rem;height:1rem;pointer-events:none;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;background-color:#ddd;background-repeat:no-repeat;background-position:center center;background-size:50% 50%}.custom-checkbox .custom-control-indicator{border-radius:.25rem}.custom-checkbox .custom-control-input:checked ~ .custom-control-indicator{background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 8 8'%3E%3Cpath fill='%23fff' d='M6.564.75l-3.59 3.612-1.538-1.55L0 4.26 2.974 7.25 8 2.193z'/%3E%3C/svg%3E\")}.custom-checkbox .custom-control-input:indeterminate ~ .custom-control-indicator{background-color:#0275d8;background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 4 4'%3E%3Cpath stroke='%23fff' d='M0 2h4'/%3E%3C/svg%3E\")}.custom-radio .custom-control-indicator{border-radius:50%}.custom-radio .custom-control-input:checked ~ .custom-control-indicator{background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='-4 -4 8 8'%3E%3Ccircle r='3' fill='%23fff'/%3E%3C/svg%3E\")}.custom-controls-stacked{display:-ms-flexbox;display:flex;-ms-flex-direction:column;flex-direction:column}.custom-controls-stacked .custom-control{margin-bottom:.25rem}.custom-controls-stacked .custom-control+.custom-control{margin-left:0}.custom-select{display:inline-block;max-width:100%;height:calc(2.25rem + 2px);padding:.375rem 1.75rem .375rem .75rem;line-height:1.25;color:#464a4c;vertical-align:middle;background:#fff url(\"data:image/svg+xml;charset=utf8,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 4 5'%3E%3Cpath fill='%23333' d='M2 0L0 2h4zm0 5L0 3h4z'/%3E%3C/svg%3E\") no-repeat right .75rem center;background-size:8px 10px;border:1px solid rgba(0,0,0,0.15);border-radius:.25rem;-moz-appearance:none;-webkit-appearance:none}.custom-select:focus{border-color:#5cb3fd;outline:none}.custom-select:focus::-ms-value{color:#464a4c;background-color:#fff}.custom-select:disabled{color:#636c72;cursor:not-allowed;background-color:#eceeef}.custom-select::-ms-expand{opacity:0}.custom-select-sm{padding-top:.375rem;padding-bottom:.375rem;font-size:75%}.custom-file{position:relative;display:inline-block;max-width:100%;height:2.5rem;margin-bottom:0;cursor:pointer}.custom-file-input{min-width:14rem;max-width:100%;height:2.5rem;margin:0;filter:alpha(opacity=0);opacity:0}.custom-file-control{position:absolute;top:0;right:0;left:0;z-index:5;height:2.5rem;padding:.5rem 1rem;line-height:1.5;color:#464a4c;pointer-events:none;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;background-color:#fff;border:1px solid rgba(0,0,0,0.15);border-radius:.25rem}.custom-file-control:lang(en)::after{content:\"Choose file...\"}.custom-file-control::before{position:absolute;top:-1px;right:-1px;bottom:-1px;z-index:6;display:block;height:2.5rem;padding:.5rem 1rem;line-height:1.5;color:#464a4c;background-color:#eceeef;border:1px solid rgba(0,0,0,0.15);border-radius:0 .25rem .25rem 0}.custom-file-control:lang(en)::before{content:\"Browse\"}.nav{display:-ms-flexbox;display:flex;padding-left:0;margin-bottom:0;list-style:none}.nav-link{display:block;padding:0.5em 1em}.nav-link:focus,.nav-link:hover{text-decoration:none}.nav-link.disabled{color:#636c72;cursor:not-allowed}.nav-tabs{border-bottom:1px solid #ddd}.nav-tabs .nav-item{margin-bottom:-1px}.nav-tabs .nav-link{border:1px solid transparent;border-top-right-radius:.25rem;border-top-left-radius:.25rem}.nav-tabs .nav-link:focus,.nav-tabs .nav-link:hover{border-color:#eceeef #eceeef #ddd}.nav-tabs .nav-link.disabled{color:#636c72;background-color:transparent;border-color:transparent}.nav-tabs .nav-link.active,.nav-tabs .nav-item.show .nav-link{color:#464a4c;background-color:#fff;border-color:#ddd #ddd #fff}.nav-tabs .dropdown-menu{margin-top:-1px;border-top-right-radius:0;border-top-left-radius:0}.nav-pills .nav-link{border-radius:.25rem}.nav-pills .nav-link.active,.nav-pills .nav-item.show .nav-link{color:#fff;cursor:default;background-color:#0275d8}.nav-fill .nav-item{-ms-flex:1 1 auto;flex:1 1 auto;text-align:center}.nav-justified .nav-item{-ms-flex:1 1 100%;flex:1 1 100%;text-align:center}.tab-content>.tab-pane{display:none}.tab-content>.active{display:block}.navbar{position:relative;display:-ms-flexbox;display:flex;-ms-flex-direction:column;flex-direction:column;padding:.5rem 1rem}.navbar-brand{display:inline-block;padding-top:.25rem;padding-bottom:.25rem;margin-right:1rem;font-size:1.25rem;line-height:inherit;white-space:nowrap}.navbar-brand:focus,.navbar-brand:hover{text-decoration:none}.navbar-nav{display:-ms-flexbox;display:flex;-ms-flex-direction:column;flex-direction:column;padding-left:0;margin-bottom:0;list-style:none}.navbar-nav .nav-link{padding-right:0;padding-left:0}.navbar-text{display:inline-block;padding-top:.425rem;padding-bottom:.425rem}.navbar-toggler{-ms-flex-item-align:start;align-self:flex-start;padding:.25rem .75rem;font-size:1.25rem;line-height:1;background:transparent;border:1px solid transparent;border-radius:.25rem}.navbar-toggler:focus,.navbar-toggler:hover{text-decoration:none}.navbar-toggler-icon{display:inline-block;width:1.5em;height:1.5em;vertical-align:middle;content:\"\";background:no-repeat center center;background-size:100% 100%}.navbar-toggler-left{position:absolute;left:1rem}.navbar-toggler-right{position:absolute;right:1rem;top:20px}@media (max-width: 575px){.navbar-toggleable .navbar-nav .dropdown-menu{position:static;float:none}.navbar-toggleable>.container{padding-right:0;padding-left:0}}@media (min-width: 576px){.navbar-toggleable{-ms-flex-direction:row;flex-direction:row;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable .navbar-nav{-ms-flex-direction:row;flex-direction:row}.navbar-toggleable .navbar-nav .nav-link{padding-right:.5rem;padding-left:.5rem}.navbar-toggleable>.container{display:-ms-flexbox;display:flex;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable .navbar-collapse{display:-ms-flexbox !important;display:flex !important;width:100%}.navbar-toggleable .navbar-toggler{display:none}}@media (max-width: 767px){.navbar-toggleable-sm .navbar-nav .dropdown-menu{position:static;float:none}.navbar-toggleable-sm>.container{padding-right:0;padding-left:0}}@media (min-width: 768px){.navbar-toggleable-sm{-ms-flex-direction:row;flex-direction:row;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable-sm .navbar-nav{-ms-flex-direction:row;flex-direction:row}.navbar-toggleable-sm .navbar-nav .nav-link{padding-right:.5rem;padding-left:.5rem}.navbar-toggleable-sm>.container{display:-ms-flexbox;display:flex;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable-sm .navbar-collapse{display:-ms-flexbox !important;display:flex !important;width:100%}.navbar-toggleable-sm .navbar-toggler{display:none}}@media (max-width: 991px){.navbar-toggleable-md .navbar-nav .dropdown-menu{position:static;float:none}.navbar-toggleable-md>.container{padding-right:0;padding-left:0}}@media (min-width: 992px){.navbar-toggleable-md{-ms-flex-direction:row;flex-direction:row;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable-md .navbar-nav{-ms-flex-direction:row;flex-direction:row}.navbar-toggleable-md .navbar-nav .nav-link{padding-right:.5rem;padding-left:.5rem}.navbar-toggleable-md>.container{display:-ms-flexbox;display:flex;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable-md .navbar-collapse{display:-ms-flexbox !important;display:flex !important;width:100%}.navbar-toggleable-md .navbar-toggler{display:none}}@media (max-width: 1199px){.navbar-toggleable-lg .navbar-nav .dropdown-menu{position:static;float:none}.navbar-toggleable-lg>.container{padding-right:0;padding-left:0}}@media (min-width: 1200px){.navbar-toggleable-lg{-ms-flex-direction:row;flex-direction:row;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable-lg .navbar-nav{-ms-flex-direction:row;flex-direction:row}.navbar-toggleable-lg .navbar-nav .nav-link{padding-right:.5rem;padding-left:.5rem}.navbar-toggleable-lg>.container{display:-ms-flexbox;display:flex;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable-lg .navbar-collapse{display:-ms-flexbox !important;display:flex !important;width:100%}.navbar-toggleable-lg .navbar-toggler{display:none}}.navbar-toggleable-xl{-ms-flex-direction:row;flex-direction:row;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable-xl .navbar-nav .dropdown-menu{position:static;float:none}.navbar-toggleable-xl>.container{padding-right:0;padding-left:0}.navbar-toggleable-xl .navbar-nav{-ms-flex-direction:row;flex-direction:row}.navbar-toggleable-xl .navbar-nav .nav-link{padding-right:.5rem;padding-left:.5rem}.navbar-toggleable-xl>.container{display:-ms-flexbox;display:flex;-ms-flex-wrap:nowrap;flex-wrap:nowrap;-ms-flex-align:center;align-items:center}.navbar-toggleable-xl .navbar-collapse{display:-ms-flexbox !important;display:flex !important;width:100%}.navbar-toggleable-xl .navbar-toggler{display:none}.navbar-light .navbar-brand,.navbar-light .navbar-toggler{color:rgba(0,0,0,0.9)}.navbar-light .navbar-brand:focus,.navbar-light .navbar-brand:hover,.navbar-light .navbar-toggler:focus,.navbar-light .navbar-toggler:hover{color:rgba(0,0,0,0.9)}.navbar-light .navbar-nav .nav-link{color:rgba(0,0,0,0.5)}.navbar-light .navbar-nav .nav-link:focus,.navbar-light .navbar-nav .nav-link:hover{color:rgba(0,0,0,0.7)}.navbar-light .navbar-nav .nav-link.disabled{color:rgba(0,0,0,0.3)}.navbar-light .navbar-nav .open>.nav-link,.navbar-light .navbar-nav .active>.nav-link,.navbar-light .navbar-nav .nav-link.open,.navbar-light .navbar-nav .nav-link.active{color:rgba(0,0,0,0.9)}.navbar-light .navbar-toggler{border-color:rgba(0,0,0,0.1)}.navbar-light .navbar-toggler-icon{background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg viewBox='0 0 32 32' xmlns='http://www.w3.org/2000/svg'%3E%3Cpath stroke='rgba(0,0,0,0.5)' stroke-width='2' stroke-linecap='round' stroke-miterlimit='10' d='M4 8h24M4 16h24M4 24h24'/%3E%3C/svg%3E\")}.navbar-light .navbar-text{color:rgba(0,0,0,0.5)}.navbar-inverse .navbar-brand,.navbar-inverse .navbar-toggler{color:#fff}.navbar-inverse .navbar-brand:focus,.navbar-inverse .navbar-brand:hover,.navbar-inverse .navbar-toggler:focus,.navbar-inverse .navbar-toggler:hover{color:#fff}.navbar-inverse .navbar-nav .nav-link{color:rgba(255,255,255,0.5)}.navbar-inverse .navbar-nav .nav-link:focus,.navbar-inverse .navbar-nav .nav-link:hover{color:rgba(255,255,255,0.75)}.navbar-inverse .navbar-nav .nav-link.disabled{color:rgba(255,255,255,0.25)}.navbar-inverse .navbar-nav .open>.nav-link,.navbar-inverse .navbar-nav .active>.nav-link,.navbar-inverse .navbar-nav .nav-link.open,.navbar-inverse .navbar-nav .nav-link.active{color:#fff}.navbar-inverse .navbar-toggler{border-color:rgba(255,255,255,0.1)}.navbar-inverse .navbar-toggler-icon{background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg viewBox='0 0 32 32' xmlns='http://www.w3.org/2000/svg'%3E%3Cpath stroke='rgba(255,255,255,0.5)' stroke-width='2' stroke-linecap='round' stroke-miterlimit='10' d='M4 8h24M4 16h24M4 24h24'/%3E%3C/svg%3E\")}.navbar-inverse .navbar-text{color:rgba(255,255,255,0.5)}.card{position:relative;display:-ms-flexbox;display:flex;-ms-flex-direction:column;flex-direction:column;background-color:#fff;border:1px solid rgba(0,0,0,0.125);border-radius:.25rem}.card-block{-ms-flex:1 1 auto;flex:1 1 auto;padding:1.25rem}.card-title{margin-bottom:.75rem}.card-subtitle{margin-top:-.375rem;margin-bottom:0}.card-text:last-child{margin-bottom:0}.card-link:hover{text-decoration:none}.card-link+.card-link{margin-left:1.25rem}.card>.list-group:first-child .list-group-item:first-child{border-top-right-radius:.25rem;border-top-left-radius:.25rem}.card>.list-group:last-child .list-group-item:last-child{border-bottom-right-radius:.25rem;border-bottom-left-radius:.25rem}.card-header{padding:.75rem 1.25rem;margin-bottom:0;background-color:#f7f7f9;border-bottom:1px solid rgba(0,0,0,0.125)}.card-header:first-child{border-radius:calc(.25rem - 1px) calc(.25rem - 1px) 0 0}.card-footer{padding:.75rem 1.25rem;background-color:#f7f7f9;border-top:1px solid rgba(0,0,0,0.125)}.card-footer:last-child{border-radius:0 0 calc(.25rem - 1px) calc(.25rem - 1px)}.card-header-tabs{margin-right:-.625rem;margin-bottom:-.75rem;margin-left:-.625rem;border-bottom:0}.card-header-pills{margin-right:-.625rem;margin-left:-.625rem}.card-primary{background-color:#0275d8;border-color:#0275d8}.card-primary .card-header,.card-primary .card-footer{background-color:transparent}.card-success{background-color:#5cb85c;border-color:#5cb85c}.card-success .card-header,.card-success .card-footer{background-color:transparent}.card-info{background-color:#5bc0de;border-color:#5bc0de}.card-info .card-header,.card-info .card-footer{background-color:transparent}.card-warning{background-color:#f0ad4e;border-color:#f0ad4e}.card-warning .card-header,.card-warning .card-footer{background-color:transparent}.card-danger{background-color:#d9534f;border-color:#d9534f}.card-danger .card-header,.card-danger .card-footer{background-color:transparent}.card-outline-primary{background-color:transparent;border-color:#0275d8}.card-outline-secondary{background-color:transparent;border-color:#ccc}.card-outline-info{background-color:transparent;border-color:#5bc0de}.card-outline-success{background-color:transparent;border-color:#5cb85c}.card-outline-warning{background-color:transparent;border-color:#f0ad4e}.card-outline-danger{background-color:transparent;border-color:#d9534f}.card-inverse{color:rgba(255,255,255,0.65)}.card-inverse .card-header,.card-inverse .card-footer{background-color:transparent;border-color:rgba(255,255,255,0.2)}.card-inverse .card-header,.card-inverse .card-footer,.card-inverse .card-title,.card-inverse .card-blockquote{color:#fff}.card-inverse .card-link,.card-inverse .card-text,.card-inverse .card-subtitle,.card-inverse .card-blockquote .blockquote-footer{color:rgba(255,255,255,0.65)}.card-inverse .card-link:focus,.card-inverse .card-link:hover{color:#fff}.card-blockquote{padding:0;margin-bottom:0;border-left:0}.card-img{border-radius:calc(.25rem - 1px)}.card-img-overlay{position:absolute;top:0;right:0;bottom:0;left:0;padding:1.25rem}.card-img-top{border-top-right-radius:calc(.25rem - 1px);border-top-left-radius:calc(.25rem - 1px)}.card-img-bottom{border-bottom-right-radius:calc(.25rem - 1px);border-bottom-left-radius:calc(.25rem - 1px)}@media (min-width: 576px){.card-deck{display:-ms-flexbox;display:flex;-ms-flex-flow:row wrap;flex-flow:row wrap}.card-deck .card{display:-ms-flexbox;display:flex;-ms-flex:1 0 0px;flex:1 0 0;-ms-flex-direction:column;flex-direction:column}.card-deck .card:not(:first-child){margin-left:15px}.card-deck .card:not(:last-child){margin-right:15px}}@media (min-width: 576px){.card-group{display:-ms-flexbox;display:flex;-ms-flex-flow:row wrap;flex-flow:row wrap}.card-group .card{-ms-flex:1 0 0px;flex:1 0 0}.card-group .card+.card{margin-left:0;border-left:0}.card-group .card:first-child{border-bottom-right-radius:0;border-top-right-radius:0}.card-group .card:first-child .card-img-top{border-top-right-radius:0}.card-group .card:first-child .card-img-bottom{border-bottom-right-radius:0}.card-group .card:last-child{border-bottom-left-radius:0;border-top-left-radius:0}.card-group .card:last-child .card-img-top{border-top-left-radius:0}.card-group .card:last-child .card-img-bottom{border-bottom-left-radius:0}.card-group .card:not(:first-child):not(:last-child){border-radius:0}.card-group .card:not(:first-child):not(:last-child) .card-img-top,.card-group .card:not(:first-child):not(:last-child) .card-img-bottom{border-radius:0}}@media (min-width: 576px){.card-columns{column-count:3;column-gap:1.25rem}.card-columns .card{display:inline-block;width:100%;margin-bottom:.75rem}}.breadcrumb{padding:.75rem 1rem;margin-bottom:1rem;list-style:none;background-color:#eceeef;border-radius:.25rem}.breadcrumb::after{display:block;content:\"\";clear:both}.breadcrumb-item{float:left}.breadcrumb-item+.breadcrumb-item::before{display:inline-block;padding-right:.5rem;padding-left:.5rem;color:#636c72;content:\"/\"}.breadcrumb-item+.breadcrumb-item:hover::before{text-decoration:underline}.breadcrumb-item+.breadcrumb-item:hover::before{text-decoration:none}.breadcrumb-item.active{color:#636c72}.pagination{display:-ms-flexbox;display:flex;padding-left:0;list-style:none;border-radius:.25rem}.page-item:first-child .page-link{margin-left:0;border-bottom-left-radius:.25rem;border-top-left-radius:.25rem}.page-item:last-child .page-link{border-bottom-right-radius:.25rem;border-top-right-radius:.25rem}.page-item.active .page-link{z-index:2;color:#fff;background-color:#0275d8;border-color:#0275d8}.page-item.disabled .page-link{color:#636c72;pointer-events:none;cursor:not-allowed;background-color:#fff;border-color:#ddd}.page-link{position:relative;display:block;padding:.5rem .75rem;margin-left:-1px;line-height:1.25;color:#0275d8;background-color:#fff;border:1px solid #ddd}.page-link:focus,.page-link:hover{color:#014c8c;text-decoration:none;background-color:#eceeef;border-color:#ddd}.pagination-lg .page-link{padding:.75rem 1.5rem;font-size:1.25rem}.pagination-lg .page-item:first-child .page-link{border-bottom-left-radius:.3rem;border-top-left-radius:.3rem}.pagination-lg .page-item:last-child .page-link{border-bottom-right-radius:.3rem;border-top-right-radius:.3rem}.pagination-sm .page-link{padding:.25rem .5rem;font-size:.875rem}.pagination-sm .page-item:first-child .page-link{border-bottom-left-radius:.2rem;border-top-left-radius:.2rem}.pagination-sm .page-item:last-child .page-link{border-bottom-right-radius:.2rem;border-top-right-radius:.2rem}.badge{display:inline-block;padding:.25em .4em;font-size:75%;font-weight:bold;line-height:1;color:#fff;text-align:center;white-space:nowrap;vertical-align:baseline;border-radius:.25rem}.badge:empty{display:none}.btn .badge{position:relative;top:-1px}a.badge:focus,a.badge:hover{color:#fff;text-decoration:none;cursor:pointer}.badge-pill{padding-right:.6em;padding-left:.6em;border-radius:10rem}.badge-default{background-color:#636c72}.badge-default[href]:focus,.badge-default[href]:hover{background-color:#4b5257}.badge-primary{background-color:#0275d8}.badge-primary[href]:focus,.badge-primary[href]:hover{background-color:#025aa5}.badge-success{background-color:#5cb85c}.badge-success[href]:focus,.badge-success[href]:hover{background-color:#449d44}.badge-info{background-color:#5bc0de}.badge-info[href]:focus,.badge-info[href]:hover{background-color:#31b0d5}.badge-warning{background-color:#f0ad4e}.badge-warning[href]:focus,.badge-warning[href]:hover{background-color:#ec971f}.badge-danger{background-color:#d9534f}.badge-danger[href]:focus,.badge-danger[href]:hover{background-color:#c9302c}.jumbotron{padding:2rem 1rem;margin-bottom:2rem;background-color:#eceeef;border-radius:.3rem}@media (min-width: 576px){.jumbotron{padding:4rem 2rem}}.jumbotron-hr{border-top-color:#d0d5d8}.jumbotron-fluid{padding-right:0;padding-left:0;border-radius:0}.alert{padding:.75rem 1.25rem;margin-bottom:1rem;border:1px solid transparent;border-radius:.25rem}.alert-heading{color:inherit}.alert-link{font-weight:bold}.alert-dismissible .close{position:relative;top:-.75rem;right:-1.25rem;padding:.75rem 1.25rem;color:inherit}.alert-success{background-color:#dff0d8;border-color:#d0e9c6;color:#3c763d}.alert-success hr{border-top-color:#c1e2b3}.alert-success .alert-link{color:#2b542c}.alert-info{background-color:#d9edf7;border-color:#bcdff1;color:#31708f}.alert-info hr{border-top-color:#a6d5ec}.alert-info .alert-link{color:#245269}.alert-warning{background-color:#fcf8e3;border-color:#faf2cc;color:#8a6d3b}.alert-warning hr{border-top-color:#f7ecb5}.alert-warning .alert-link{color:#66512c}.alert-danger{background-color:#f2dede;border-color:#ebcccc;color:#a94442}.alert-danger hr{border-top-color:#e4b9b9}.alert-danger .alert-link{color:#843534}@keyframes progress-bar-stripes{from{background-position:1rem 0}to{background-position:0 0}}.progress{display:-ms-flexbox;display:flex;overflow:hidden;font-size:.75rem;line-height:1rem;text-align:center;background-color:#eceeef;border-radius:.25rem}.progress-bar{height:1rem;color:#fff;background-color:#0275d8}.progress-bar-striped{background-image:linear-gradient(45deg, rgba(255,255,255,0.15) 25%, transparent 25%, transparent 50%, rgba(255,255,255,0.15) 50%, rgba(255,255,255,0.15) 75%, transparent 75%, transparent);background-size:1rem 1rem}.progress-bar-animated{animation:progress-bar-stripes 1s linear infinite}.media{display:-ms-flexbox;display:flex;-ms-flex-align:start;align-items:flex-start}.media-body{-ms-flex:1;flex:1}.list-group{display:-ms-flexbox;display:flex;-ms-flex-direction:column;flex-direction:column;padding-left:0;margin-bottom:0}.list-group-item-action{width:100%;color:#464a4c;text-align:inherit}.list-group-item-action .list-group-item-heading{color:#292b2c}.list-group-item-action:focus,.list-group-item-action:hover{color:#464a4c;text-decoration:none;background-color:#f7f7f9}.list-group-item-action:active{color:#292b2c;background-color:#eceeef}.list-group-item{position:relative;display:-ms-flexbox;display:flex;-ms-flex-flow:row wrap;flex-flow:row wrap;-ms-flex-align:center;align-items:center;padding:.75rem 1.25rem;margin-bottom:-1px;background-color:#fff;border:1px solid rgba(0,0,0,0.125)}.list-group-item:first-child{border-top-right-radius:.25rem;border-top-left-radius:.25rem}.list-group-item:last-child{margin-bottom:0;border-bottom-right-radius:.25rem;border-bottom-left-radius:.25rem}.list-group-item:focus,.list-group-item:hover{text-decoration:none}.list-group-item.disabled,.list-group-item:disabled{color:#636c72;cursor:not-allowed;background-color:#fff}.list-group-item.disabled .list-group-item-heading,.list-group-item:disabled .list-group-item-heading{color:inherit}.list-group-item.disabled .list-group-item-text,.list-group-item:disabled .list-group-item-text{color:#636c72}.list-group-item.active{z-index:2;color:#fff;background-color:#0275d8;border-color:#0275d8}.list-group-item.active .list-group-item-heading,.list-group-item.active .list-group-item-heading>small,.list-group-item.active .list-group-item-heading>.small{color:inherit}.list-group-item.active .list-group-item-text{color:#daeeff}.list-group-flush .list-group-item{border-right:0;border-left:0;border-radius:0}.list-group-flush:first-child .list-group-item:first-child{border-top:0}.list-group-flush:last-child .list-group-item:last-child{border-bottom:0}.list-group-item-success{color:#3c763d;background-color:#dff0d8}a.list-group-item-success,button.list-group-item-success{color:#3c763d}a.list-group-item-success .list-group-item-heading,button.list-group-item-success .list-group-item-heading{color:inherit}a.list-group-item-success:focus,a.list-group-item-success:hover,button.list-group-item-success:focus,button.list-group-item-success:hover{color:#3c763d;background-color:#d0e9c6}a.list-group-item-success.active,button.list-group-item-success.active{color:#fff;background-color:#3c763d;border-color:#3c763d}.list-group-item-info{color:#31708f;background-color:#d9edf7}a.list-group-item-info,button.list-group-item-info{color:#31708f}a.list-group-item-info .list-group-item-heading,button.list-group-item-info .list-group-item-heading{color:inherit}a.list-group-item-info:focus,a.list-group-item-info:hover,button.list-group-item-info:focus,button.list-group-item-info:hover{color:#31708f;background-color:#c4e3f3}a.list-group-item-info.active,button.list-group-item-info.active{color:#fff;background-color:#31708f;border-color:#31708f}.list-group-item-warning{color:#8a6d3b;background-color:#fcf8e3}a.list-group-item-warning,button.list-group-item-warning{color:#8a6d3b}a.list-group-item-warning .list-group-item-heading,button.list-group-item-warning .list-group-item-heading{color:inherit}a.list-group-item-warning:focus,a.list-group-item-warning:hover,button.list-group-item-warning:focus,button.list-group-item-warning:hover{color:#8a6d3b;background-color:#faf2cc}a.list-group-item-warning.active,button.list-group-item-warning.active{color:#fff;background-color:#8a6d3b;border-color:#8a6d3b}.list-group-item-danger{color:#a94442;background-color:#f2dede}a.list-group-item-danger,button.list-group-item-danger{color:#a94442}a.list-group-item-danger .list-group-item-heading,button.list-group-item-danger .list-group-item-heading{color:inherit}a.list-group-item-danger:focus,a.list-group-item-danger:hover,button.list-group-item-danger:focus,button.list-group-item-danger:hover{color:#a94442;background-color:#ebcccc}a.list-group-item-danger.active,button.list-group-item-danger.active{color:#fff;background-color:#a94442;border-color:#a94442}.embed-responsive{position:relative;display:block;width:100%;padding:0;overflow:hidden}.embed-responsive::before{display:block;content:\"\"}.embed-responsive .embed-responsive-item,.embed-responsive iframe,.embed-responsive embed,.embed-responsive object,.embed-responsive video{position:absolute;top:0;bottom:0;left:0;width:100%;height:100%;border:0}.embed-responsive-21by9::before{padding-top:42.85714%}.embed-responsive-16by9::before{padding-top:56.25%}.embed-responsive-4by3::before{padding-top:75%}.embed-responsive-1by1::before{padding-top:100%}.close{float:right;font-size:1.5rem;font-weight:bold;line-height:1;color:#000;text-shadow:0 1px 0 #fff;opacity:.5}.close:focus,.close:hover{color:#000;text-decoration:none;cursor:pointer;opacity:.75}button.close{padding:0;cursor:pointer;background:transparent;border:0;-webkit-appearance:none}.modal-open{overflow:hidden}.modal{position:fixed;top:0;right:0;bottom:0;left:0;z-index:1050;display:none;overflow:hidden;outline:0}.modal.fade .modal-dialog{transition:transform 0.3s ease-out;transform:translate(0, -25%)}.modal.show .modal-dialog{transform:translate(0, 0)}.modal-open .modal{overflow-x:hidden;overflow-y:auto}.modal-dialog{position:relative;width:auto;margin:10px}.modal-content{position:relative;display:-ms-flexbox;display:flex;-ms-flex-direction:column;flex-direction:column;background-color:#fff;background-clip:padding-box;border:1px solid rgba(0,0,0,0.2);border-radius:.3rem;outline:0}.modal-backdrop{position:fixed;top:0;right:0;bottom:0;left:0;z-index:1040;background-color:#000}.modal-backdrop.fade{opacity:0}.modal-backdrop.show{opacity:.5}.modal-header{display:-ms-flexbox;display:flex;-ms-flex-align:center;align-items:center;-ms-flex-pack:justify;justify-content:space-between;padding:15px;border-bottom:1px solid #eceeef}.modal-title{margin-bottom:0;line-height:1.5}.modal-body{position:relative;-ms-flex:1 1 auto;flex:1 1 auto;padding:15px}.modal-footer{display:-ms-flexbox;display:flex;-ms-flex-align:center;align-items:center;-ms-flex-pack:end;justify-content:flex-end;padding:15px;border-top:1px solid #eceeef}.modal-footer>:not(:first-child){margin-left:.25rem}.modal-footer>:not(:last-child){margin-right:.25rem}.modal-scrollbar-measure{position:absolute;top:-9999px;width:50px;height:50px;overflow:scroll}@media (min-width: 576px){.modal-dialog{max-width:500px;margin:30px auto}.modal-sm{max-width:300px}}@media (min-width: 992px){.modal-lg{max-width:800px}}.tooltip{position:absolute;z-index:1070;display:block;font-family:-apple-system,system-ui,BlinkMacSystemFont,\"Segoe UI\",Roboto,\"Helvetica Neue\",Arial,sans-serif;font-style:normal;font-weight:normal;letter-spacing:normal;line-break:auto;line-height:1.5;text-align:left;text-align:start;text-decoration:none;text-shadow:none;text-transform:none;white-space:normal;word-break:normal;word-spacing:normal;font-size:.875rem;word-wrap:break-word;opacity:0}.tooltip.show{opacity:.9}.tooltip.tooltip-top,.tooltip.bs-tether-element-attached-bottom{padding:5px 0;margin-top:-3px}.tooltip.tooltip-top .tooltip-inner::before,.tooltip.bs-tether-element-attached-bottom .tooltip-inner::before{bottom:0;left:50%;margin-left:-5px;content:\"\";border-width:5px 5px 0;border-top-color:#000}.tooltip.tooltip-right,.tooltip.bs-tether-element-attached-left{padding:0 5px;margin-left:3px}.tooltip.tooltip-right .tooltip-inner::before,.tooltip.bs-tether-element-attached-left .tooltip-inner::before{top:50%;left:0;margin-top:-5px;content:\"\";border-width:5px 5px 5px 0;border-right-color:#000}.tooltip.tooltip-bottom,.tooltip.bs-tether-element-attached-top{padding:5px 0;margin-top:3px}.tooltip.tooltip-bottom .tooltip-inner::before,.tooltip.bs-tether-element-attached-top .tooltip-inner::before{top:0;left:50%;margin-left:-5px;content:\"\";border-width:0 5px 5px;border-bottom-color:#000}.tooltip.tooltip-left,.tooltip.bs-tether-element-attached-right{padding:0 5px;margin-left:-3px}.tooltip.tooltip-left .tooltip-inner::before,.tooltip.bs-tether-element-attached-right .tooltip-inner::before{top:50%;right:0;margin-top:-5px;content:\"\";border-width:5px 0 5px 5px;border-left-color:#000}.tooltip-inner{max-width:200px;padding:3px 8px;color:#fff;text-align:center;background-color:#000;border-radius:.25rem}.tooltip-inner::before{position:absolute;width:0;height:0;border-color:transparent;border-style:solid}.popover{position:absolute;top:0;left:0;z-index:1060;display:block;max-width:276px;padding:1px;font-family:-apple-system,system-ui,BlinkMacSystemFont,\"Segoe UI\",Roboto,\"Helvetica Neue\",Arial,sans-serif;font-style:normal;font-weight:normal;letter-spacing:normal;line-break:auto;line-height:1.5;text-align:left;text-align:start;text-decoration:none;text-shadow:none;text-transform:none;white-space:normal;word-break:normal;word-spacing:normal;font-size:.875rem;word-wrap:break-word;background-color:#fff;background-clip:padding-box;border:1px solid rgba(0,0,0,0.2);border-radius:.3rem}.popover.popover-top,.popover.bs-tether-element-attached-bottom{margin-top:-10px}.popover.popover-top::before,.popover.popover-top::after,.popover.bs-tether-element-attached-bottom::before,.popover.bs-tether-element-attached-bottom::after{left:50%;border-bottom-width:0}.popover.popover-top::before,.popover.bs-tether-element-attached-bottom::before{bottom:-11px;margin-left:-11px;border-top-color:rgba(0,0,0,0.25)}.popover.popover-top::after,.popover.bs-tether-element-attached-bottom::after{bottom:-10px;margin-left:-10px;border-top-color:#fff}.popover.popover-right,.popover.bs-tether-element-attached-left{margin-left:10px}.popover.popover-right::before,.popover.popover-right::after,.popover.bs-tether-element-attached-left::before,.popover.bs-tether-element-attached-left::after{top:50%;border-left-width:0}.popover.popover-right::before,.popover.bs-tether-element-attached-left::before{left:-11px;margin-top:-11px;border-right-color:rgba(0,0,0,0.25)}.popover.popover-right::after,.popover.bs-tether-element-attached-left::after{left:-10px;margin-top:-10px;border-right-color:#fff}.popover.popover-bottom,.popover.bs-tether-element-attached-top{margin-top:10px}.popover.popover-bottom::before,.popover.popover-bottom::after,.popover.bs-tether-element-attached-top::before,.popover.bs-tether-element-attached-top::after{left:50%;border-top-width:0}.popover.popover-bottom::before,.popover.bs-tether-element-attached-top::before{top:-11px;margin-left:-11px;border-bottom-color:rgba(0,0,0,0.25)}.popover.popover-bottom::after,.popover.bs-tether-element-attached-top::after{top:-10px;margin-left:-10px;border-bottom-color:#f7f7f7}.popover.popover-bottom .popover-title::before,.popover.bs-tether-element-attached-top .popover-title::before{position:absolute;top:0;left:50%;display:block;width:20px;margin-left:-10px;content:\"\";border-bottom:1px solid #f7f7f7}.popover.popover-left,.popover.bs-tether-element-attached-right{margin-left:-10px}.popover.popover-left::before,.popover.popover-left::after,.popover.bs-tether-element-attached-right::before,.popover.bs-tether-element-attached-right::after{top:50%;border-right-width:0}.popover.popover-left::before,.popover.bs-tether-element-attached-right::before{right:-11px;margin-top:-11px;border-left-color:rgba(0,0,0,0.25)}.popover.popover-left::after,.popover.bs-tether-element-attached-right::after{right:-10px;margin-top:-10px;border-left-color:#fff}.popover-title{padding:8px 14px;margin-bottom:0;font-size:1rem;background-color:#f7f7f7;border-bottom:1px solid #ebebeb;border-top-right-radius:calc(.3rem - 1px);border-top-left-radius:calc(.3rem - 1px)}.popover-title:empty{display:none}.popover-content{padding:9px 14px}.popover::before,.popover::after{position:absolute;display:block;width:0;height:0;border-color:transparent;border-style:solid}.popover::before{content:\"\";border-width:11px}.popover::after{content:\"\";border-width:10px}.carousel{position:relative}.carousel-inner{position:relative;width:100%;overflow:hidden}.carousel-item{position:relative;display:none;width:100%}@media (-webkit-transform-3d){.carousel-item{transition:transform 0.6s ease-in-out;-webkit-backface-visibility:hidden;backface-visibility:hidden;perspective:1000px}}@supports (transform: translate3d(0, 0, 0)){.carousel-item{transition:transform 0.6s ease-in-out;-webkit-backface-visibility:hidden;backface-visibility:hidden;perspective:1000px}}.carousel-item.active,.carousel-item-next,.carousel-item-prev{display:-ms-flexbox;display:flex}.carousel-item-next,.carousel-item-prev{position:absolute;top:0}@media (-webkit-transform-3d){.carousel-item-next.carousel-item-left,.carousel-item-prev.carousel-item-right{transform:translate3d(0, 0, 0)}.carousel-item-next,.active.carousel-item-right{transform:translate3d(100%, 0, 0)}.carousel-item-prev,.active.carousel-item-left{transform:translate3d(-100%, 0, 0)}}@supports (transform: translate3d(0, 0, 0)){.carousel-item-next.carousel-item-left,.carousel-item-prev.carousel-item-right{transform:translate3d(0, 0, 0)}.carousel-item-next,.active.carousel-item-right{transform:translate3d(100%, 0, 0)}.carousel-item-prev,.active.carousel-item-left{transform:translate3d(-100%, 0, 0)}}.carousel-control-prev,.carousel-control-next{position:absolute;top:0;bottom:0;display:-ms-flexbox;display:flex;-ms-flex-align:center;align-items:center;-ms-flex-pack:center;justify-content:center;width:15%;color:#fff;text-align:center;opacity:.5}.carousel-control-prev:focus,.carousel-control-prev:hover,.carousel-control-next:focus,.carousel-control-next:hover{color:#fff;text-decoration:none;outline:0;opacity:.9}.carousel-control-prev{left:0}.carousel-control-next{right:0}.carousel-control-prev-icon,.carousel-control-next-icon{display:inline-block;width:20px;height:20px;background:transparent no-repeat center center;background-size:100% 100%}.carousel-control-prev-icon{background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='%23fff' viewBox='0 0 8 8'%3E%3Cpath d='M4 0l-4 4 4 4 1.5-1.5-2.5-2.5 2.5-2.5-1.5-1.5z'/%3E%3C/svg%3E\")}.carousel-control-next-icon{background-image:url(\"data:image/svg+xml;charset=utf8,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='%23fff' viewBox='0 0 8 8'%3E%3Cpath d='M1.5 0l-1.5 1.5 2.5 2.5-2.5 2.5 1.5 1.5 4-4-4-4z'/%3E%3C/svg%3E\")}.carousel-indicators{position:absolute;right:0;bottom:10px;left:0;z-index:15;display:-ms-flexbox;display:flex;-ms-flex-pack:center;justify-content:center;padding-left:0;margin-right:15%;margin-left:15%;list-style:none}.carousel-indicators li{position:relative;-ms-flex:1 0 auto;flex:1 0 auto;max-width:30px;height:3px;margin-right:3px;margin-left:3px;text-indent:-999px;cursor:pointer;background-color:rgba(255,255,255,0.5)}.carousel-indicators li::before{position:absolute;top:-10px;left:0;display:inline-block;width:100%;height:10px;content:\"\"}.carousel-indicators li::after{position:absolute;bottom:-10px;left:0;display:inline-block;width:100%;height:10px;content:\"\"}.carousel-indicators .active{background-color:#fff}.carousel-caption{position:absolute;right:15%;bottom:20px;left:15%;z-index:10;padding-top:20px;padding-bottom:20px;color:#fff;text-align:center}.align-baseline{vertical-align:baseline !important}.align-top{vertical-align:top !important}.align-middle{vertical-align:middle !important}.align-bottom{vertical-align:bottom !important}.align-text-bottom{vertical-align:text-bottom !important}.align-text-top{vertical-align:text-top !important}.bg-faded{background-color:#f7f7f7}.bg-primary{background-color:#0275d8 !important}a.bg-primary:focus,a.bg-primary:hover{background-color:#025aa5 !important}.bg-success{background-color:#5cb85c !important}a.bg-success:focus,a.bg-success:hover{background-color:#449d44 !important}.bg-info{background-color:#5bc0de !important}a.bg-info:focus,a.bg-info:hover{background-color:#31b0d5 !important}.bg-warning{background-color:#f0ad4e !important}a.bg-warning:focus,a.bg-warning:hover{background-color:#ec971f !important}.bg-danger{background-color:#d9534f !important}a.bg-danger:focus,a.bg-danger:hover{background-color:#c9302c !important}.bg-inverse{background-color:#292b2c !important}a.bg-inverse:focus,a.bg-inverse:hover{background-color:#101112 !important}.border-0{border:0 !important}.border-top-0{border-top:0 !important}.border-right-0{border-right:0 !important}.border-bottom-0{border-bottom:0 !important}.border-left-0{border-left:0 !important}.rounded{border-radius:.25rem}.rounded-top{border-top-right-radius:.25rem;border-top-left-radius:.25rem}.rounded-right{border-bottom-right-radius:.25rem;border-top-right-radius:.25rem}.rounded-bottom{border-bottom-right-radius:.25rem;border-bottom-left-radius:.25rem}.rounded-left{border-bottom-left-radius:.25rem;border-top-left-radius:.25rem}.rounded-circle{border-radius:50%}.rounded-0{border-radius:0}.clearfix::after{display:block;content:\"\";clear:both}.d-none{display:none !important}.d-inline{display:inline !important}.d-inline-block{display:inline-block !important}.d-block{display:block !important}.d-table{display:table !important}.d-table-cell{display:table-cell !important}.d-flex{display:-ms-flexbox !important;display:flex !important}.d-inline-flex{display:-ms-inline-flexbox !important;display:inline-flex !important}@media (min-width: 576px){.d-sm-none{display:none !important}.d-sm-inline{display:inline !important}.d-sm-inline-block{display:inline-block !important}.d-sm-block{display:block !important}.d-sm-table{display:table !important}.d-sm-table-cell{display:table-cell !important}.d-sm-flex{display:-ms-flexbox !important;display:flex !important}.d-sm-inline-flex{display:-ms-inline-flexbox !important;display:inline-flex !important}}@media (min-width: 768px){.d-md-none{display:none !important}.d-md-inline{display:inline !important}.d-md-inline-block{display:inline-block !important}.d-md-block{display:block !important}.d-md-table{display:table !important}.d-md-table-cell{display:table-cell !important}.d-md-flex{display:-ms-flexbox !important;display:flex !important}.d-md-inline-flex{display:-ms-inline-flexbox !important;display:inline-flex !important}}@media (min-width: 992px){.d-lg-none{display:none !important}.d-lg-inline{display:inline !important}.d-lg-inline-block{display:inline-block !important}.d-lg-block{display:block !important}.d-lg-table{display:table !important}.d-lg-table-cell{display:table-cell !important}.d-lg-flex{display:-ms-flexbox !important;display:flex !important}.d-lg-inline-flex{display:-ms-inline-flexbox !important;display:inline-flex !important}}@media (min-width: 1200px){.d-xl-none{display:none !important}.d-xl-inline{display:inline !important}.d-xl-inline-block{display:inline-block !important}.d-xl-block{display:block !important}.d-xl-table{display:table !important}.d-xl-table-cell{display:table-cell !important}.d-xl-flex{display:-ms-flexbox !important;display:flex !important}.d-xl-inline-flex{display:-ms-inline-flexbox !important;display:inline-flex !important}}.flex-first{-ms-flex-order:-1;order:-1}.flex-last{-ms-flex-order:1;order:1}.flex-unordered{-ms-flex-order:0;order:0}.flex-row{-ms-flex-direction:row !important;flex-direction:row !important}.flex-column{-ms-flex-direction:column !important;flex-direction:column !important}.flex-row-reverse{-ms-flex-direction:row-reverse !important;flex-direction:row-reverse !important}.flex-column-reverse{-ms-flex-direction:column-reverse !important;flex-direction:column-reverse !important}.flex-wrap{-ms-flex-wrap:wrap !important;flex-wrap:wrap !important}.flex-nowrap{-ms-flex-wrap:nowrap !important;flex-wrap:nowrap !important}.flex-wrap-reverse{-ms-flex-wrap:wrap-reverse !important;flex-wrap:wrap-reverse !important}.justify-content-start{-ms-flex-pack:start !important;justify-content:flex-start !important}.justify-content-end{-ms-flex-pack:end !important;justify-content:flex-end !important}.justify-content-center{-ms-flex-pack:center !important;justify-content:center !important}.justify-content-between{-ms-flex-pack:justify !important;justify-content:space-between !important}.justify-content-around{-ms-flex-pack:distribute !important;justify-content:space-around !important}.align-items-start{-ms-flex-align:start !important;align-items:flex-start !important}.align-items-end{-ms-flex-align:end !important;align-items:flex-end !important}.align-items-center{-ms-flex-align:center !important;align-items:center !important}.align-items-baseline{-ms-flex-align:baseline !important;align-items:baseline !important}.align-items-stretch{-ms-flex-align:stretch !important;align-items:stretch !important}.align-content-start{-ms-flex-line-pack:start !important;align-content:flex-start !important}.align-content-end{-ms-flex-line-pack:end !important;align-content:flex-end !important}.align-content-center{-ms-flex-line-pack:center !important;align-content:center !important}.align-content-between{-ms-flex-line-pack:justify !important;align-content:space-between !important}.align-content-around{-ms-flex-line-pack:distribute !important;align-content:space-around !important}.align-content-stretch{-ms-flex-line-pack:stretch !important;align-content:stretch !important}.align-self-auto{-ms-flex-item-align:auto !important;-ms-grid-row-align:auto !important;align-self:auto !important}.align-self-start{-ms-flex-item-align:start !important;align-self:flex-start !important}.align-self-end{-ms-flex-item-align:end !important;align-self:flex-end !important}.align-self-center{-ms-flex-item-align:center !important;-ms-grid-row-align:center !important;align-self:center !important}.align-self-baseline{-ms-flex-item-align:baseline !important;align-self:baseline !important}.align-self-stretch{-ms-flex-item-align:stretch !important;-ms-grid-row-align:stretch !important;align-self:stretch !important}@media (min-width: 576px){.flex-sm-first{-ms-flex-order:-1;order:-1}.flex-sm-last{-ms-flex-order:1;order:1}.flex-sm-unordered{-ms-flex-order:0;order:0}.flex-sm-row{-ms-flex-direction:row !important;flex-direction:row !important}.flex-sm-column{-ms-flex-direction:column !important;flex-direction:column !important}.flex-sm-row-reverse{-ms-flex-direction:row-reverse !important;flex-direction:row-reverse !important}.flex-sm-column-reverse{-ms-flex-direction:column-reverse !important;flex-direction:column-reverse !important}.flex-sm-wrap{-ms-flex-wrap:wrap !important;flex-wrap:wrap !important}.flex-sm-nowrap{-ms-flex-wrap:nowrap !important;flex-wrap:nowrap !important}.flex-sm-wrap-reverse{-ms-flex-wrap:wrap-reverse !important;flex-wrap:wrap-reverse !important}.justify-content-sm-start{-ms-flex-pack:start !important;justify-content:flex-start !important}.justify-content-sm-end{-ms-flex-pack:end !important;justify-content:flex-end !important}.justify-content-sm-center{-ms-flex-pack:center !important;justify-content:center !important}.justify-content-sm-between{-ms-flex-pack:justify !important;justify-content:space-between !important}.justify-content-sm-around{-ms-flex-pack:distribute !important;justify-content:space-around !important}.align-items-sm-start{-ms-flex-align:start !important;align-items:flex-start !important}.align-items-sm-end{-ms-flex-align:end !important;align-items:flex-end !important}.align-items-sm-center{-ms-flex-align:center !important;align-items:center !important}.align-items-sm-baseline{-ms-flex-align:baseline !important;align-items:baseline !important}.align-items-sm-stretch{-ms-flex-align:stretch !important;align-items:stretch !important}.align-content-sm-start{-ms-flex-line-pack:start !important;align-content:flex-start !important}.align-content-sm-end{-ms-flex-line-pack:end !important;align-content:flex-end !important}.align-content-sm-center{-ms-flex-line-pack:center !important;align-content:center !important}.align-content-sm-between{-ms-flex-line-pack:justify !important;align-content:space-between !important}.align-content-sm-around{-ms-flex-line-pack:distribute !important;align-content:space-around !important}.align-content-sm-stretch{-ms-flex-line-pack:stretch !important;align-content:stretch !important}.align-self-sm-auto{-ms-flex-item-align:auto !important;-ms-grid-row-align:auto !important;align-self:auto !important}.align-self-sm-start{-ms-flex-item-align:start !important;align-self:flex-start !important}.align-self-sm-end{-ms-flex-item-align:end !important;align-self:flex-end !important}.align-self-sm-center{-ms-flex-item-align:center !important;-ms-grid-row-align:center !important;align-self:center !important}.align-self-sm-baseline{-ms-flex-item-align:baseline !important;align-self:baseline !important}.align-self-sm-stretch{-ms-flex-item-align:stretch !important;-ms-grid-row-align:stretch !important;align-self:stretch !important}}@media (min-width: 768px){.flex-md-first{-ms-flex-order:-1;order:-1}.flex-md-last{-ms-flex-order:1;order:1}.flex-md-unordered{-ms-flex-order:0;order:0}.flex-md-row{-ms-flex-direction:row !important;flex-direction:row !important}.flex-md-column{-ms-flex-direction:column !important;flex-direction:column !important}.flex-md-row-reverse{-ms-flex-direction:row-reverse !important;flex-direction:row-reverse !important}.flex-md-column-reverse{-ms-flex-direction:column-reverse !important;flex-direction:column-reverse !important}.flex-md-wrap{-ms-flex-wrap:wrap !important;flex-wrap:wrap !important}.flex-md-nowrap{-ms-flex-wrap:nowrap !important;flex-wrap:nowrap !important}.flex-md-wrap-reverse{-ms-flex-wrap:wrap-reverse !important;flex-wrap:wrap-reverse !important}.justify-content-md-start{-ms-flex-pack:start !important;justify-content:flex-start !important}.justify-content-md-end{-ms-flex-pack:end !important;justify-content:flex-end !important}.justify-content-md-center{-ms-flex-pack:center !important;justify-content:center !important}.justify-content-md-between{-ms-flex-pack:justify !important;justify-content:space-between !important}.justify-content-md-around{-ms-flex-pack:distribute !important;justify-content:space-around !important}.align-items-md-start{-ms-flex-align:start !important;align-items:flex-start !important}.align-items-md-end{-ms-flex-align:end !important;align-items:flex-end !important}.align-items-md-center{-ms-flex-align:center !important;align-items:center !important}.align-items-md-baseline{-ms-flex-align:baseline !important;align-items:baseline !important}.align-items-md-stretch{-ms-flex-align:stretch !important;align-items:stretch !important}.align-content-md-start{-ms-flex-line-pack:start !important;align-content:flex-start !important}.align-content-md-end{-ms-flex-line-pack:end !important;align-content:flex-end !important}.align-content-md-center{-ms-flex-line-pack:center !important;align-content:center !important}.align-content-md-between{-ms-flex-line-pack:justify !important;align-content:space-between !important}.align-content-md-around{-ms-flex-line-pack:distribute !important;align-content:space-around !important}.align-content-md-stretch{-ms-flex-line-pack:stretch !important;align-content:stretch !important}.align-self-md-auto{-ms-flex-item-align:auto !important;-ms-grid-row-align:auto !important;align-self:auto !important}.align-self-md-start{-ms-flex-item-align:start !important;align-self:flex-start !important}.align-self-md-end{-ms-flex-item-align:end !important;align-self:flex-end !important}.align-self-md-center{-ms-flex-item-align:center !important;-ms-grid-row-align:center !important;align-self:center !important}.align-self-md-baseline{-ms-flex-item-align:baseline !important;align-self:baseline !important}.align-self-md-stretch{-ms-flex-item-align:stretch !important;-ms-grid-row-align:stretch !important;align-self:stretch !important}}@media (min-width: 992px){.flex-lg-first{-ms-flex-order:-1;order:-1}.flex-lg-last{-ms-flex-order:1;order:1}.flex-lg-unordered{-ms-flex-order:0;order:0}.flex-lg-row{-ms-flex-direction:row !important;flex-direction:row !important}.flex-lg-column{-ms-flex-direction:column !important;flex-direction:column !important}.flex-lg-row-reverse{-ms-flex-direction:row-reverse !important;flex-direction:row-reverse !important}.flex-lg-column-reverse{-ms-flex-direction:column-reverse !important;flex-direction:column-reverse !important}.flex-lg-wrap{-ms-flex-wrap:wrap !important;flex-wrap:wrap !important}.flex-lg-nowrap{-ms-flex-wrap:nowrap !important;flex-wrap:nowrap !important}.flex-lg-wrap-reverse{-ms-flex-wrap:wrap-reverse !important;flex-wrap:wrap-reverse !important}.justify-content-lg-start{-ms-flex-pack:start !important;justify-content:flex-start !important}.justify-content-lg-end{-ms-flex-pack:end !important;justify-content:flex-end !important}.justify-content-lg-center{-ms-flex-pack:center !important;justify-content:center !important}.justify-content-lg-between{-ms-flex-pack:justify !important;justify-content:space-between !important}.justify-content-lg-around{-ms-flex-pack:distribute !important;justify-content:space-around !important}.align-items-lg-start{-ms-flex-align:start !important;align-items:flex-start !important}.align-items-lg-end{-ms-flex-align:end !important;align-items:flex-end !important}.align-items-lg-center{-ms-flex-align:center !important;align-items:center !important}.align-items-lg-baseline{-ms-flex-align:baseline !important;align-items:baseline !important}.align-items-lg-stretch{-ms-flex-align:stretch !important;align-items:stretch !important}.align-content-lg-start{-ms-flex-line-pack:start !important;align-content:flex-start !important}.align-content-lg-end{-ms-flex-line-pack:end !important;align-content:flex-end !important}.align-content-lg-center{-ms-flex-line-pack:center !important;align-content:center !important}.align-content-lg-between{-ms-flex-line-pack:justify !important;align-content:space-between !important}.align-content-lg-around{-ms-flex-line-pack:distribute !important;align-content:space-around !important}.align-content-lg-stretch{-ms-flex-line-pack:stretch !important;align-content:stretch !important}.align-self-lg-auto{-ms-flex-item-align:auto !important;-ms-grid-row-align:auto !important;align-self:auto !important}.align-self-lg-start{-ms-flex-item-align:start !important;align-self:flex-start !important}.align-self-lg-end{-ms-flex-item-align:end !important;align-self:flex-end !important}.align-self-lg-center{-ms-flex-item-align:center !important;-ms-grid-row-align:center !important;align-self:center !important}.align-self-lg-baseline{-ms-flex-item-align:baseline !important;align-self:baseline !important}.align-self-lg-stretch{-ms-flex-item-align:stretch !important;-ms-grid-row-align:stretch !important;align-self:stretch !important}}@media (min-width: 1200px){.flex-xl-first{-ms-flex-order:-1;order:-1}.flex-xl-last{-ms-flex-order:1;order:1}.flex-xl-unordered{-ms-flex-order:0;order:0}.flex-xl-row{-ms-flex-direction:row !important;flex-direction:row !important}.flex-xl-column{-ms-flex-direction:column !important;flex-direction:column !important}.flex-xl-row-reverse{-ms-flex-direction:row-reverse !important;flex-direction:row-reverse !important}.flex-xl-column-reverse{-ms-flex-direction:column-reverse !important;flex-direction:column-reverse !important}.flex-xl-wrap{-ms-flex-wrap:wrap !important;flex-wrap:wrap !important}.flex-xl-nowrap{-ms-flex-wrap:nowrap !important;flex-wrap:nowrap !important}.flex-xl-wrap-reverse{-ms-flex-wrap:wrap-reverse !important;flex-wrap:wrap-reverse !important}.justify-content-xl-start{-ms-flex-pack:start !important;justify-content:flex-start !important}.justify-content-xl-end{-ms-flex-pack:end !important;justify-content:flex-end !important}.justify-content-xl-center{-ms-flex-pack:center !important;justify-content:center !important}.justify-content-xl-between{-ms-flex-pack:justify !important;justify-content:space-between !important}.justify-content-xl-around{-ms-flex-pack:distribute !important;justify-content:space-around !important}.align-items-xl-start{-ms-flex-align:start !important;align-items:flex-start !important}.align-items-xl-end{-ms-flex-align:end !important;align-items:flex-end !important}.align-items-xl-center{-ms-flex-align:center !important;align-items:center !important}.align-items-xl-baseline{-ms-flex-align:baseline !important;align-items:baseline !important}.align-items-xl-stretch{-ms-flex-align:stretch !important;align-items:stretch !important}.align-content-xl-start{-ms-flex-line-pack:start !important;align-content:flex-start !important}.align-content-xl-end{-ms-flex-line-pack:end !important;align-content:flex-end !important}.align-content-xl-center{-ms-flex-line-pack:center !important;align-content:center !important}.align-content-xl-between{-ms-flex-line-pack:justify !important;align-content:space-between !important}.align-content-xl-around{-ms-flex-line-pack:distribute !important;align-content:space-around !important}.align-content-xl-stretch{-ms-flex-line-pack:stretch !important;align-content:stretch !important}.align-self-xl-auto{-ms-flex-item-align:auto !important;-ms-grid-row-align:auto !important;align-self:auto !important}.align-self-xl-start{-ms-flex-item-align:start !important;align-self:flex-start !important}.align-self-xl-end{-ms-flex-item-align:end !important;align-self:flex-end !important}.align-self-xl-center{-ms-flex-item-align:center !important;-ms-grid-row-align:center !important;align-self:center !important}.align-self-xl-baseline{-ms-flex-item-align:baseline !important;align-self:baseline !important}.align-self-xl-stretch{-ms-flex-item-align:stretch !important;-ms-grid-row-align:stretch !important;align-self:stretch !important}}.float-left{float:left !important}.float-right{float:right !important}.float-none{float:none !important}@media (min-width: 576px){.float-sm-left{float:left !important}.float-sm-right{float:right !important}.float-sm-none{float:none !important}}@media (min-width: 768px){.float-md-left{float:left !important}.float-md-right{float:right !important}.float-md-none{float:none !important}}@media (min-width: 992px){.float-lg-left{float:left !important}.float-lg-right{float:right !important}.float-lg-none{float:none !important}}@media (min-width: 1200px){.float-xl-left{float:left !important}.float-xl-right{float:right !important}.float-xl-none{float:none !important}}.fixed-top{position:fixed;top:0;right:0;left:0;z-index:1030}.fixed-bottom{position:fixed;right:0;bottom:0;left:0;z-index:1030}.sticky-top{position:-webkit-sticky;position:sticky;top:0;z-index:1030}.sr-only{position:absolute;width:1px;height:1px;padding:0;margin:-1px;overflow:hidden;clip:rect(0, 0, 0, 0);border:0}.sr-only-focusable:active,.sr-only-focusable:focus{position:static;width:auto;height:auto;margin:0;overflow:visible;clip:auto}.w-25{width:25% !important}.w-50{width:50% !important}.w-75{width:75% !important}.w-100{width:100% !important}.h-25{height:25% !important}.h-50{height:50% !important}.h-75{height:75% !important}.h-100{height:100% !important}.mw-100{max-width:100% !important}.mh-100{max-height:100% !important}.m-0{margin:0 0 !important}.mt-0{margin-top:0 !important}.mr-0{margin-right:0 !important}.mb-0{margin-bottom:0 !important}.ml-0{margin-left:0 !important}.mx-0{margin-right:0 !important;margin-left:0 !important}.my-0{margin-top:0 !important;margin-bottom:0 !important}.m-1{margin:.25rem .25rem !important}.mt-1{margin-top:.25rem !important}.mr-1{margin-right:.25rem !important}.mb-1{margin-bottom:.25rem !important}.ml-1{margin-left:.25rem !important}.mx-1{margin-right:.25rem !important;margin-left:.25rem !important}.my-1{margin-top:.25rem !important;margin-bottom:.25rem !important}.m-2{margin:.5rem .5rem !important}.mt-2{margin-top:.5rem !important}.mr-2{margin-right:.5rem !important}.mb-2{margin-bottom:.5rem !important}.ml-2{margin-left:.5rem !important}.mx-2{margin-right:.5rem !important;margin-left:.5rem !important}.my-2{margin-top:.5rem !important;margin-bottom:.5rem !important}.m-3{margin:1rem 1rem !important}.mt-3{margin-top:1rem !important}.mr-3{margin-right:1rem !important}.mb-3{margin-bottom:1rem !important}.ml-3{margin-left:1rem !important}.mx-3{margin-right:1rem !important;margin-left:1rem !important}.my-3{margin-top:1rem !important;margin-bottom:1rem !important}.m-4{margin:1.5rem 1.5rem !important}.mt-4{margin-top:1.5rem !important}.mr-4{margin-right:1.5rem !important}.mb-4{margin-bottom:1.5rem !important}.ml-4{margin-left:1.5rem !important}.mx-4{margin-right:1.5rem !important;margin-left:1.5rem !important}.my-4{margin-top:1.5rem !important;margin-bottom:1.5rem !important}.m-5{margin:3rem 3rem !important}.mt-5{margin-top:3rem !important}.mr-5{margin-right:3rem !important}.mb-5{margin-bottom:3rem !important}.ml-5{margin-left:3rem !important}.mx-5{margin-right:3rem !important;margin-left:3rem !important}.my-5{margin-top:3rem !important;margin-bottom:3rem !important}.p-0{padding:0 0 !important}.pt-0{padding-top:0 !important}.pr-0{padding-right:0 !important}.pb-0{padding-bottom:0 !important}.pl-0{padding-left:0 !important}.px-0{padding-right:0 !important;padding-left:0 !important}.py-0{padding-top:0 !important;padding-bottom:0 !important}.p-1{padding:.25rem .25rem !important}.pt-1{padding-top:.25rem !important}.pr-1{padding-right:.25rem !important}.pb-1{padding-bottom:.25rem !important}.pl-1{padding-left:.25rem !important}.px-1{padding-right:.25rem !important;padding-left:.25rem !important}.py-1{padding-top:.25rem !important;padding-bottom:.25rem !important}.p-2{padding:.5rem .5rem !important}.pt-2{padding-top:.5rem !important}.pr-2{padding-right:.5rem !important}.pb-2{padding-bottom:.5rem !important}.pl-2{padding-left:.5rem !important}.px-2{padding-right:.5rem !important;padding-left:.5rem !important}.py-2{padding-top:.5rem !important;padding-bottom:.5rem !important}.p-3{padding:1rem 1rem !important}.pt-3{padding-top:1rem !important}.pr-3{padding-right:1rem !important}.pb-3{padding-bottom:1rem !important}.pl-3{padding-left:1rem !important}.px-3{padding-right:1rem !important;padding-left:1rem !important}.py-3{padding-top:1rem !important;padding-bottom:1rem !important}.p-4{padding:1.5rem 1.5rem !important}.pt-4{padding-top:1.5rem !important}.pr-4{padding-right:1.5rem !important}.pb-4{padding-bottom:1.5rem !important}.pl-4{padding-left:1.5rem !important}.px-4{padding-right:1.5rem !important;padding-left:1.5rem !important}.py-4{padding-top:1.5rem !important;padding-bottom:1.5rem !important}.p-5{padding:3rem 3rem !important}.pt-5{padding-top:3rem !important}.pr-5{padding-right:3rem !important}.pb-5{padding-bottom:3rem !important}.pl-5{padding-left:3rem !important}.px-5{padding-right:3rem !important;padding-left:3rem !important}.py-5{padding-top:3rem !important;padding-bottom:3rem !important}.m-auto{margin:auto !important}.mt-auto{margin-top:auto !important}.mr-auto{margin-right:auto !important}.mb-auto{margin-bottom:auto !important}.ml-auto{margin-left:auto !important}.mx-auto{margin-right:auto !important;margin-left:auto !important}.my-auto{margin-top:auto !important;margin-bottom:auto !important}@media (min-width: 576px){.m-sm-0{margin:0 0 !important}.mt-sm-0{margin-top:0 !important}.mr-sm-0{margin-right:0 !important}.mb-sm-0{margin-bottom:0 !important}.ml-sm-0{margin-left:0 !important}.mx-sm-0{margin-right:0 !important;margin-left:0 !important}.my-sm-0{margin-top:0 !important;margin-bottom:0 !important}.m-sm-1{margin:.25rem .25rem !important}.mt-sm-1{margin-top:.25rem !important}.mr-sm-1{margin-right:.25rem !important}.mb-sm-1{margin-bottom:.25rem !important}.ml-sm-1{margin-left:.25rem !important}.mx-sm-1{margin-right:.25rem !important;margin-left:.25rem !important}.my-sm-1{margin-top:.25rem !important;margin-bottom:.25rem !important}.m-sm-2{margin:.5rem .5rem !important}.mt-sm-2{margin-top:.5rem !important}.mr-sm-2{margin-right:.5rem !important}.mb-sm-2{margin-bottom:.5rem !important}.ml-sm-2{margin-left:.5rem !important}.mx-sm-2{margin-right:.5rem !important;margin-left:.5rem !important}.my-sm-2{margin-top:.5rem !important;margin-bottom:.5rem !important}.m-sm-3{margin:1rem 1rem !important}.mt-sm-3{margin-top:1rem !important}.mr-sm-3{margin-right:1rem !important}.mb-sm-3{margin-bottom:1rem !important}.ml-sm-3{margin-left:1rem !important}.mx-sm-3{margin-right:1rem !important;margin-left:1rem !important}.my-sm-3{margin-top:1rem !important;margin-bottom:1rem !important}.m-sm-4{margin:1.5rem 1.5rem !important}.mt-sm-4{margin-top:1.5rem !important}.mr-sm-4{margin-right:1.5rem !important}.mb-sm-4{margin-bottom:1.5rem !important}.ml-sm-4{margin-left:1.5rem !important}.mx-sm-4{margin-right:1.5rem !important;margin-left:1.5rem !important}.my-sm-4{margin-top:1.5rem !important;margin-bottom:1.5rem !important}.m-sm-5{margin:3rem 3rem !important}.mt-sm-5{margin-top:3rem !important}.mr-sm-5{margin-right:3rem !important}.mb-sm-5{margin-bottom:3rem !important}.ml-sm-5{margin-left:3rem !important}.mx-sm-5{margin-right:3rem !important;margin-left:3rem !important}.my-sm-5{margin-top:3rem !important;margin-bottom:3rem !important}.p-sm-0{padding:0 0 !important}.pt-sm-0{padding-top:0 !important}.pr-sm-0{padding-right:0 !important}.pb-sm-0{padding-bottom:0 !important}.pl-sm-0{padding-left:0 !important}.px-sm-0{padding-right:0 !important;padding-left:0 !important}.py-sm-0{padding-top:0 !important;padding-bottom:0 !important}.p-sm-1{padding:.25rem .25rem !important}.pt-sm-1{padding-top:.25rem !important}.pr-sm-1{padding-right:.25rem !important}.pb-sm-1{padding-bottom:.25rem !important}.pl-sm-1{padding-left:.25rem !important}.px-sm-1{padding-right:.25rem !important;padding-left:.25rem !important}.py-sm-1{padding-top:.25rem !important;padding-bottom:.25rem !important}.p-sm-2{padding:.5rem .5rem !important}.pt-sm-2{padding-top:.5rem !important}.pr-sm-2{padding-right:.5rem !important}.pb-sm-2{padding-bottom:.5rem !important}.pl-sm-2{padding-left:.5rem !important}.px-sm-2{padding-right:.5rem !important;padding-left:.5rem !important}.py-sm-2{padding-top:.5rem !important;padding-bottom:.5rem !important}.p-sm-3{padding:1rem 1rem !important}.pt-sm-3{padding-top:1rem !important}.pr-sm-3{padding-right:1rem !important}.pb-sm-3{padding-bottom:1rem !important}.pl-sm-3{padding-left:1rem !important}.px-sm-3{padding-right:1rem !important;padding-left:1rem !important}.py-sm-3{padding-top:1rem !important;padding-bottom:1rem !important}.p-sm-4{padding:1.5rem 1.5rem !important}.pt-sm-4{padding-top:1.5rem !important}.pr-sm-4{padding-right:1.5rem !important}.pb-sm-4{padding-bottom:1.5rem !important}.pl-sm-4{padding-left:1.5rem !important}.px-sm-4{padding-right:1.5rem !important;padding-left:1.5rem !important}.py-sm-4{padding-top:1.5rem !important;padding-bottom:1.5rem !important}.p-sm-5{padding:3rem 3rem !important}.pt-sm-5{padding-top:3rem !important}.pr-sm-5{padding-right:3rem !important}.pb-sm-5{padding-bottom:3rem !important}.pl-sm-5{padding-left:3rem !important}.px-sm-5{padding-right:3rem !important;padding-left:3rem !important}.py-sm-5{padding-top:3rem !important;padding-bottom:3rem !important}.m-sm-auto{margin:auto !important}.mt-sm-auto{margin-top:auto !important}.mr-sm-auto{margin-right:auto !important}.mb-sm-auto{margin-bottom:auto !important}.ml-sm-auto{margin-left:auto !important}.mx-sm-auto{margin-right:auto !important;margin-left:auto !important}.my-sm-auto{margin-top:auto !important;margin-bottom:auto !important}}@media (min-width: 768px){.m-md-0{margin:0 0 !important}.mt-md-0{margin-top:0 !important}.mr-md-0{margin-right:0 !important}.mb-md-0{margin-bottom:0 !important}.ml-md-0{margin-left:0 !important}.mx-md-0{margin-right:0 !important;margin-left:0 !important}.my-md-0{margin-top:0 !important;margin-bottom:0 !important}.m-md-1{margin:.25rem .25rem !important}.mt-md-1{margin-top:.25rem !important}.mr-md-1{margin-right:.25rem !important}.mb-md-1{margin-bottom:.25rem !important}.ml-md-1{margin-left:.25rem !important}.mx-md-1{margin-right:.25rem !important;margin-left:.25rem !important}.my-md-1{margin-top:.25rem !important;margin-bottom:.25rem !important}.m-md-2{margin:.5rem .5rem !important}.mt-md-2{margin-top:.5rem !important}.mr-md-2{margin-right:.5rem !important}.mb-md-2{margin-bottom:.5rem !important}.ml-md-2{margin-left:.5rem !important}.mx-md-2{margin-right:.5rem !important;margin-left:.5rem !important}.my-md-2{margin-top:.5rem !important;margin-bottom:.5rem !important}.m-md-3{margin:1rem 1rem !important}.mt-md-3{margin-top:1rem !important}.mr-md-3{margin-right:1rem !important}.mb-md-3{margin-bottom:1rem !important}.ml-md-3{margin-left:1rem !important}.mx-md-3{margin-right:1rem !important;margin-left:1rem !important}.my-md-3{margin-top:1rem !important;margin-bottom:1rem !important}.m-md-4{margin:1.5rem 1.5rem !important}.mt-md-4{margin-top:1.5rem !important}.mr-md-4{margin-right:1.5rem !important}.mb-md-4{margin-bottom:1.5rem !important}.ml-md-4{margin-left:1.5rem !important}.mx-md-4{margin-right:1.5rem !important;margin-left:1.5rem !important}.my-md-4{margin-top:1.5rem !important;margin-bottom:1.5rem !important}.m-md-5{margin:3rem 3rem !important}.mt-md-5{margin-top:3rem !important}.mr-md-5{margin-right:3rem !important}.mb-md-5{margin-bottom:3rem !important}.ml-md-5{margin-left:3rem !important}.mx-md-5{margin-right:3rem !important;margin-left:3rem !important}.my-md-5{margin-top:3rem !important;margin-bottom:3rem !important}.p-md-0{padding:0 0 !important}.pt-md-0{padding-top:0 !important}.pr-md-0{padding-right:0 !important}.pb-md-0{padding-bottom:0 !important}.pl-md-0{padding-left:0 !important}.px-md-0{padding-right:0 !important;padding-left:0 !important}.py-md-0{padding-top:0 !important;padding-bottom:0 !important}.p-md-1{padding:.25rem .25rem !important}.pt-md-1{padding-top:.25rem !important}.pr-md-1{padding-right:.25rem !important}.pb-md-1{padding-bottom:.25rem !important}.pl-md-1{padding-left:.25rem !important}.px-md-1{padding-right:.25rem !important;padding-left:.25rem !important}.py-md-1{padding-top:.25rem !important;padding-bottom:.25rem !important}.p-md-2{padding:.5rem .5rem !important}.pt-md-2{padding-top:.5rem !important}.pr-md-2{padding-right:.5rem !important}.pb-md-2{padding-bottom:.5rem !important}.pl-md-2{padding-left:.5rem !important}.px-md-2{padding-right:.5rem !important;padding-left:.5rem !important}.py-md-2{padding-top:.5rem !important;padding-bottom:.5rem !important}.p-md-3{padding:1rem 1rem !important}.pt-md-3{padding-top:1rem !important}.pr-md-3{padding-right:1rem !important}.pb-md-3{padding-bottom:1rem !important}.pl-md-3{padding-left:1rem !important}.px-md-3{padding-right:1rem !important;padding-left:1rem !important}.py-md-3{padding-top:1rem !important;padding-bottom:1rem !important}.p-md-4{padding:1.5rem 1.5rem !important}.pt-md-4{padding-top:1.5rem !important}.pr-md-4{padding-right:1.5rem !important}.pb-md-4{padding-bottom:1.5rem !important}.pl-md-4{padding-left:1.5rem !important}.px-md-4{padding-right:1.5rem !important;padding-left:1.5rem !important}.py-md-4{padding-top:1.5rem !important;padding-bottom:1.5rem !important}.p-md-5{padding:3rem 3rem !important}.pt-md-5{padding-top:3rem !important}.pr-md-5{padding-right:3rem !important}.pb-md-5{padding-bottom:3rem !important}.pl-md-5{padding-left:3rem !important}.px-md-5{padding-right:3rem !important;padding-left:3rem !important}.py-md-5{padding-top:3rem !important;padding-bottom:3rem !important}.m-md-auto{margin:auto !important}.mt-md-auto{margin-top:auto !important}.mr-md-auto{margin-right:auto !important}.mb-md-auto{margin-bottom:auto !important}.ml-md-auto{margin-left:auto !important}.mx-md-auto{margin-right:auto !important;margin-left:auto !important}.my-md-auto{margin-top:auto !important;margin-bottom:auto !important}}@media (min-width: 992px){.m-lg-0{margin:0 0 !important}.mt-lg-0{margin-top:0 !important}.mr-lg-0{margin-right:0 !important}.mb-lg-0{margin-bottom:0 !important}.ml-lg-0{margin-left:0 !important}.mx-lg-0{margin-right:0 !important;margin-left:0 !important}.my-lg-0{margin-top:0 !important;margin-bottom:0 !important}.m-lg-1{margin:.25rem .25rem !important}.mt-lg-1{margin-top:.25rem !important}.mr-lg-1{margin-right:.25rem !important}.mb-lg-1{margin-bottom:.25rem !important}.ml-lg-1{margin-left:.25rem !important}.mx-lg-1{margin-right:.25rem !important;margin-left:.25rem !important}.my-lg-1{margin-top:.25rem !important;margin-bottom:.25rem !important}.m-lg-2{margin:.5rem .5rem !important}.mt-lg-2{margin-top:.5rem !important}.mr-lg-2{margin-right:.5rem !important}.mb-lg-2{margin-bottom:.5rem !important}.ml-lg-2{margin-left:.5rem !important}.mx-lg-2{margin-right:.5rem !important;margin-left:.5rem !important}.my-lg-2{margin-top:.5rem !important;margin-bottom:.5rem !important}.m-lg-3{margin:1rem 1rem !important}.mt-lg-3{margin-top:1rem !important}.mr-lg-3{margin-right:1rem !important}.mb-lg-3{margin-bottom:1rem !important}.ml-lg-3{margin-left:1rem !important}.mx-lg-3{margin-right:1rem !important;margin-left:1rem !important}.my-lg-3{margin-top:1rem !important;margin-bottom:1rem !important}.m-lg-4{margin:1.5rem 1.5rem !important}.mt-lg-4{margin-top:1.5rem !important}.mr-lg-4{margin-right:1.5rem !important}.mb-lg-4{margin-bottom:1.5rem !important}.ml-lg-4{margin-left:1.5rem !important}.mx-lg-4{margin-right:1.5rem !important;margin-left:1.5rem !important}.my-lg-4{margin-top:1.5rem !important;margin-bottom:1.5rem !important}.m-lg-5{margin:3rem 3rem !important}.mt-lg-5{margin-top:3rem !important}.mr-lg-5{margin-right:3rem !important}.mb-lg-5{margin-bottom:3rem !important}.ml-lg-5{margin-left:3rem !important}.mx-lg-5{margin-right:3rem !important;margin-left:3rem !important}.my-lg-5{margin-top:3rem !important;margin-bottom:3rem !important}.p-lg-0{padding:0 0 !important}.pt-lg-0{padding-top:0 !important}.pr-lg-0{padding-right:0 !important}.pb-lg-0{padding-bottom:0 !important}.pl-lg-0{padding-left:0 !important}.px-lg-0{padding-right:0 !important;padding-left:0 !important}.py-lg-0{padding-top:0 !important;padding-bottom:0 !important}.p-lg-1{padding:.25rem .25rem !important}.pt-lg-1{padding-top:.25rem !important}.pr-lg-1{padding-right:.25rem !important}.pb-lg-1{padding-bottom:.25rem !important}.pl-lg-1{padding-left:.25rem !important}.px-lg-1{padding-right:.25rem !important;padding-left:.25rem !important}.py-lg-1{padding-top:.25rem !important;padding-bottom:.25rem !important}.p-lg-2{padding:.5rem .5rem !important}.pt-lg-2{padding-top:.5rem !important}.pr-lg-2{padding-right:.5rem !important}.pb-lg-2{padding-bottom:.5rem !important}.pl-lg-2{padding-left:.5rem !important}.px-lg-2{padding-right:.5rem !important;padding-left:.5rem !important}.py-lg-2{padding-top:.5rem !important;padding-bottom:.5rem !important}.p-lg-3{padding:1rem 1rem !important}.pt-lg-3{padding-top:1rem !important}.pr-lg-3{padding-right:1rem !important}.pb-lg-3{padding-bottom:1rem !important}.pl-lg-3{padding-left:1rem !important}.px-lg-3{padding-right:1rem !important;padding-left:1rem !important}.py-lg-3{padding-top:1rem !important;padding-bottom:1rem !important}.p-lg-4{padding:1.5rem 1.5rem !important}.pt-lg-4{padding-top:1.5rem !important}.pr-lg-4{padding-right:1.5rem !important}.pb-lg-4{padding-bottom:1.5rem !important}.pl-lg-4{padding-left:1.5rem !important}.px-lg-4{padding-right:1.5rem !important;padding-left:1.5rem !important}.py-lg-4{padding-top:1.5rem !important;padding-bottom:1.5rem !important}.p-lg-5{padding:3rem 3rem !important}.pt-lg-5{padding-top:3rem !important}.pr-lg-5{padding-right:3rem !important}.pb-lg-5{padding-bottom:3rem !important}.pl-lg-5{padding-left:3rem !important}.px-lg-5{padding-right:3rem !important;padding-left:3rem !important}.py-lg-5{padding-top:3rem !important;padding-bottom:3rem !important}.m-lg-auto{margin:auto !important}.mt-lg-auto{margin-top:auto !important}.mr-lg-auto{margin-right:auto !important}.mb-lg-auto{margin-bottom:auto !important}.ml-lg-auto{margin-left:auto !important}.mx-lg-auto{margin-right:auto !important;margin-left:auto !important}.my-lg-auto{margin-top:auto !important;margin-bottom:auto !important}}@media (min-width: 1200px){.m-xl-0{margin:0 0 !important}.mt-xl-0{margin-top:0 !important}.mr-xl-0{margin-right:0 !important}.mb-xl-0{margin-bottom:0 !important}.ml-xl-0{margin-left:0 !important}.mx-xl-0{margin-right:0 !important;margin-left:0 !important}.my-xl-0{margin-top:0 !important;margin-bottom:0 !important}.m-xl-1{margin:.25rem .25rem !important}.mt-xl-1{margin-top:.25rem !important}.mr-xl-1{margin-right:.25rem !important}.mb-xl-1{margin-bottom:.25rem !important}.ml-xl-1{margin-left:.25rem !important}.mx-xl-1{margin-right:.25rem !important;margin-left:.25rem !important}.my-xl-1{margin-top:.25rem !important;margin-bottom:.25rem !important}.m-xl-2{margin:.5rem .5rem !important}.mt-xl-2{margin-top:.5rem !important}.mr-xl-2{margin-right:.5rem !important}.mb-xl-2{margin-bottom:.5rem !important}.ml-xl-2{margin-left:.5rem !important}.mx-xl-2{margin-right:.5rem !important;margin-left:.5rem !important}.my-xl-2{margin-top:.5rem !important;margin-bottom:.5rem !important}.m-xl-3{margin:1rem 1rem !important}.mt-xl-3{margin-top:1rem !important}.mr-xl-3{margin-right:1rem !important}.mb-xl-3{margin-bottom:1rem !important}.ml-xl-3{margin-left:1rem !important}.mx-xl-3{margin-right:1rem !important;margin-left:1rem !important}.my-xl-3{margin-top:1rem !important;margin-bottom:1rem !important}.m-xl-4{margin:1.5rem 1.5rem !important}.mt-xl-4{margin-top:1.5rem !important}.mr-xl-4{margin-right:1.5rem !important}.mb-xl-4{margin-bottom:1.5rem !important}.ml-xl-4{margin-left:1.5rem !important}.mx-xl-4{margin-right:1.5rem !important;margin-left:1.5rem !important}.my-xl-4{margin-top:1.5rem !important;margin-bottom:1.5rem !important}.m-xl-5{margin:3rem 3rem !important}.mt-xl-5{margin-top:3rem !important}.mr-xl-5{margin-right:3rem !important}.mb-xl-5{margin-bottom:3rem !important}.ml-xl-5{margin-left:3rem !important}.mx-xl-5{margin-right:3rem !important;margin-left:3rem !important}.my-xl-5{margin-top:3rem !important;margin-bottom:3rem !important}.p-xl-0{padding:0 0 !important}.pt-xl-0{padding-top:0 !important}.pr-xl-0{padding-right:0 !important}.pb-xl-0{padding-bottom:0 !important}.pl-xl-0{padding-left:0 !important}.px-xl-0{padding-right:0 !important;padding-left:0 !important}.py-xl-0{padding-top:0 !important;padding-bottom:0 !important}.p-xl-1{padding:.25rem .25rem !important}.pt-xl-1{padding-top:.25rem !important}.pr-xl-1{padding-right:.25rem !important}.pb-xl-1{padding-bottom:.25rem !important}.pl-xl-1{padding-left:.25rem !important}.px-xl-1{padding-right:.25rem !important;padding-left:.25rem !important}.py-xl-1{padding-top:.25rem !important;padding-bottom:.25rem !important}.p-xl-2{padding:.5rem .5rem !important}.pt-xl-2{padding-top:.5rem !important}.pr-xl-2{padding-right:.5rem !important}.pb-xl-2{padding-bottom:.5rem !important}.pl-xl-2{padding-left:.5rem !important}.px-xl-2{padding-right:.5rem !important;padding-left:.5rem !important}.py-xl-2{padding-top:.5rem !important;padding-bottom:.5rem !important}.p-xl-3{padding:1rem 1rem !important}.pt-xl-3{padding-top:1rem !important}.pr-xl-3{padding-right:1rem !important}.pb-xl-3{padding-bottom:1rem !important}.pl-xl-3{padding-left:1rem !important}.px-xl-3{padding-right:1rem !important;padding-left:1rem !important}.py-xl-3{padding-top:1rem !important;padding-bottom:1rem !important}.p-xl-4{padding:1.5rem 1.5rem !important}.pt-xl-4{padding-top:1.5rem !important}.pr-xl-4{padding-right:1.5rem !important}.pb-xl-4{padding-bottom:1.5rem !important}.pl-xl-4{padding-left:1.5rem !important}.px-xl-4{padding-right:1.5rem !important;padding-left:1.5rem !important}.py-xl-4{padding-top:1.5rem !important;padding-bottom:1.5rem !important}.p-xl-5{padding:3rem 3rem !important}.pt-xl-5{padding-top:3rem !important}.pr-xl-5{padding-right:3rem !important}.pb-xl-5{padding-bottom:3rem !important}.pl-xl-5{padding-left:3rem !important}.px-xl-5{padding-right:3rem !important;padding-left:3rem !important}.py-xl-5{padding-top:3rem !important;padding-bottom:3rem !important}.m-xl-auto{margin:auto !important}.mt-xl-auto{margin-top:auto !important}.mr-xl-auto{margin-right:auto !important}.mb-xl-auto{margin-bottom:auto !important}.ml-xl-auto{margin-left:auto !important}.mx-xl-auto{margin-right:auto !important;margin-left:auto !important}.my-xl-auto{margin-top:auto !important;margin-bottom:auto !important}}.text-justify{text-align:justify !important}.text-nowrap{white-space:nowrap !important}.text-truncate{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.text-left{text-align:left !important}.text-right{text-align:right !important}.text-center{text-align:center !important}@media (min-width: 576px){.text-sm-left{text-align:left !important}.text-sm-right{text-align:right !important}.text-sm-center{text-align:center !important}}@media (min-width: 768px){.text-md-left{text-align:left !important}.text-md-right{text-align:right !important}.text-md-center{text-align:center !important}}@media (min-width: 992px){.text-lg-left{text-align:left !important}.text-lg-right{text-align:right !important}.text-lg-center{text-align:center !important}}@media (min-width: 1200px){.text-xl-left{text-align:left !important}.text-xl-right{text-align:right !important}.text-xl-center{text-align:center !important}}.text-lowercase{text-transform:lowercase !important}.text-uppercase{text-transform:uppercase !important}.text-capitalize{text-transform:capitalize !important}.font-weight-normal{font-weight:normal}.font-weight-bold{font-weight:bold}.font-italic{font-style:italic}.text-white{color:#fff !important}.text-muted{color:#636c72 !important}a.text-muted:focus,a.text-muted:hover{color:#4b5257 !important}.text-primary{color:#0275d8 !important}a.text-primary:focus,a.text-primary:hover{color:#025aa5 !important}.text-success{color:#5cb85c !important}a.text-success:focus,a.text-success:hover{color:#449d44 !important}.text-info{color:#5bc0de !important}a.text-info:focus,a.text-info:hover{color:#31b0d5 !important}.text-warning{color:#f0ad4e !important}a.text-warning:focus,a.text-warning:hover{color:#ec971f !important}.text-danger{color:#d9534f !important}a.text-danger:focus,a.text-danger:hover{color:#c9302c !important}.text-gray-dark{color:#292b2c !important}a.text-gray-dark:focus,a.text-gray-dark:hover{color:#101112 !important}.text-hide{font:0/0 a;color:transparent;text-shadow:none;background-color:transparent;border:0}.invisible{visibility:hidden !important}.hidden-xs-up{display:none !important}@media (max-width: 575px){.hidden-xs-down{display:none !important}}@media (min-width: 576px){.hidden-sm-up{display:none !important}}@media (max-width: 767px){.hidden-sm-down{display:none !important}}@media (min-width: 768px){.hidden-md-up{display:none !important}}@media (max-width: 991px){.hidden-md-down{display:none !important}}@media (min-width: 992px){.hidden-lg-up{display:none !important}}@media (max-width: 1199px){.hidden-lg-down{display:none !important}}@media (min-width: 1200px){.hidden-xl-up{display:none !important}}.hidden-xl-down{display:none !important}.visible-print-block{display:none !important}@media print{.visible-print-block{display:block !important}}.visible-print-inline{display:none !important}@media print{.visible-print-inline{display:inline !important}}.visible-print-inline-block{display:none !important}@media print{.visible-print-inline-block{display:inline-block !important}}@media print{.hidden-print{display:none !important}}button.btn,label.btn{font-family:\"AmazonEmber-Light\";font-size:14px;height:32px;padding:6px 8px;border-radius:2px;display:-ms-inline-flexbox;display:inline-flex;-ms-flex-direction:column;flex-direction:column;-ms-flex-pack:center;justify-content:center;cursor:pointer;width:initial}button.btn.l-primary,button.btn .btn-primary,label.btn.l-primary,label.btn .btn-primary{color:#fff;background-color:#6441A5;border-color:#6441A5}button.btn.l-primary:hover,button.btn .btn-primary:hover,label.btn.l-primary:hover,label.btn .btn-primary:hover{color:#fff;background-color:#4e3380;border-color:#493079}button.btn.l-primary:focus,button.btn.l-primary.focus,button.btn .btn-primary:focus,button.btn .btn-primary.focus,label.btn.l-primary:focus,label.btn.l-primary.focus,label.btn .btn-primary:focus,label.btn .btn-primary.focus{box-shadow:0 0 0 2px rgba(100,65,165,0.5)}button.btn.l-primary.disabled,button.btn.l-primary:disabled,button.btn .btn-primary.disabled,button.btn .btn-primary:disabled,label.btn.l-primary.disabled,label.btn.l-primary:disabled,label.btn .btn-primary.disabled,label.btn .btn-primary:disabled{background-color:#6441A5;border-color:#6441A5}button.btn.l-primary:active,button.btn.l-primary.active,.show>button.btn.l-primary.dropdown-toggle,button.btn .btn-primary:active,button.btn .btn-primary.active,.show>button.btn .btn-primary.dropdown-toggle,label.btn.l-primary:active,label.btn.l-primary.active,.show>label.btn.l-primary.dropdown-toggle,label.btn .btn-primary:active,label.btn .btn-primary.active,.show>label.btn .btn-primary.dropdown-toggle{color:#fff;background-color:#4e3380;background-image:none;border-color:#493079}button.btn.l-primary:hover,button.btn.l-primary:focus,button.btn.l-primary.focus,button.btn .btn-primary:hover,button.btn .btn-primary:focus,button.btn .btn-primary.focus,label.btn.l-primary:hover,label.btn.l-primary:focus,label.btn.l-primary.focus,label.btn .btn-primary:hover,label.btn .btn-primary:focus,label.btn .btn-primary.focus{background-color:#7e5bbe;border-color:#8362c1;box-shadow:none}button.btn.l-primary.active.focus,button.btn .btn-primary.active.focus,label.btn.l-primary.active.focus,label.btn .btn-primary.active.focus{background-color:#4e3380;border-color:#493079;box-shadow:none}button.btn.btn-outline-primary,button.btn.l-secondary,label.btn.btn-outline-primary,label.btn.l-secondary{color:#6441A5;background-color:#fff;border-color:#6441A5}button.btn.btn-outline-primary:hover,button.btn.l-secondary:hover,label.btn.btn-outline-primary:hover,label.btn.l-secondary:hover{color:#6441A5;background-color:#e6e6e6;border-color:#493079}button.btn.btn-outline-primary:focus,button.btn.btn-outline-primary.focus,button.btn.l-secondary:focus,button.btn.l-secondary.focus,label.btn.btn-outline-primary:focus,label.btn.btn-outline-primary.focus,label.btn.l-secondary:focus,label.btn.l-secondary.focus{box-shadow:0 0 0 2px rgba(100,65,165,0.5)}button.btn.btn-outline-primary.disabled,button.btn.btn-outline-primary:disabled,button.btn.l-secondary.disabled,button.btn.l-secondary:disabled,label.btn.btn-outline-primary.disabled,label.btn.btn-outline-primary:disabled,label.btn.l-secondary.disabled,label.btn.l-secondary:disabled{background-color:#fff;border-color:#6441A5}button.btn.btn-outline-primary:active,button.btn.btn-outline-primary.active,.show>button.btn.btn-outline-primary.dropdown-toggle,button.btn.l-secondary:active,button.btn.l-secondary.active,.show>button.btn.l-secondary.dropdown-toggle,label.btn.btn-outline-primary:active,label.btn.btn-outline-primary.active,.show>label.btn.btn-outline-primary.dropdown-toggle,label.btn.l-secondary:active,label.btn.l-secondary.active,.show>label.btn.l-secondary.dropdown-toggle{color:#6441A5;background-color:#e6e6e6;background-image:none;border-color:#493079}button.btn.btn-outline-primary:focus,button.btn.btn-outline-primary.focus,button.btn.l-secondary:focus,button.btn.l-secondary.focus,label.btn.btn-outline-primary:focus,label.btn.btn-outline-primary.focus,label.btn.l-secondary:focus,label.btn.l-secondary.focus{box-shadow:none}button.btn.l-dropdown,label.btn.l-dropdown{color:#6441A5;background-color:#fff;border-color:rgba(0,0,0,0.15);display:inline}button.btn.l-dropdown:hover,label.btn.l-dropdown:hover{color:#6441A5;background-color:#e6e6e6;border-color:rgba(0,0,0,0.15)}button.btn.l-dropdown:focus,button.btn.l-dropdown.focus,label.btn.l-dropdown:focus,label.btn.l-dropdown.focus{box-shadow:0 0 0 2px rgba(0,0,0,0.5)}button.btn.l-dropdown.disabled,button.btn.l-dropdown:disabled,label.btn.l-dropdown.disabled,label.btn.l-dropdown:disabled{background-color:#fff;border-color:rgba(0,0,0,0.15)}button.btn.l-dropdown:active,button.btn.l-dropdown.active,.show>button.btn.l-dropdown.dropdown-toggle,label.btn.l-dropdown:active,label.btn.l-dropdown.active,.show>label.btn.l-dropdown.dropdown-toggle{color:#6441A5;background-color:#e6e6e6;background-image:none;border-color:rgba(0,0,0,0.15)}button.btn.l-dropdown:focus,button.btn.l-dropdown.focus,label.btn.l-dropdown:focus,label.btn.l-dropdown.focus{box-shadow:none}button.btn.l-dropdown::after,label.btn.l-dropdown::after{content:none}button.btn.l-dropdown i,label.btn.l-dropdown i{margin-left:5px;line-height:18px}button.btn.l-dropdown:hover,button.btn.l-dropdown:focus,button.btn.l-dropdown.focus,button.btn.l-dropdown:active,button.btn.l-dropdown.active,button.btn.l-dropdown .dropdown-toggle,label.btn.l-dropdown:hover,label.btn.l-dropdown:focus,label.btn.l-dropdown.focus,label.btn.l-dropdown:active,label.btn.l-dropdown.active,label.btn.l-dropdown .dropdown-toggle{border:1px solid #6441A5;background-color:#f2f2f2;border-color:#7e5bbe}button.btn.l-dropdown:active,button.btn.l-dropdown.active,.show>button.btn.l-dropdown.dropdown-toggle{border:1px solid #6441A5;background-color:#f2f2f2;border-color:#7e5bbe}label.radio-btn-label{margin-right:20px}input[type=\"radio\"]{display:none}input[type=\"radio\"]+span{background-color:white;border:1px solid;border-color:#6441A5;border-radius:50px;display:inline-block;margin-right:8px;padding:8px;position:relative}input[type=\"radio\"]:checked+span{color:#f00}input[type=\"radio\"]:checked+span:after{background:#6441A5;border-radius:50px;content:\" \";height:10px;width:10px;left:3px;position:absolute;top:3px}label:hover input[type=radio]+span,label:focus input[type=radio]+span{border-color:#6441A5;box-shadow:0px 1px 1px 0px #6441A5} @font-face{font-family:\"AmazonEmber-Thin\";src:url(\"https://m.media-amazon.com/images/G/01/cloudcanvas/fonts/AmazonEmber_Th._V503963883_.woff\") format(\"woff\"),url(\"https://m.media-amazon.com/images/G/01/cloudcanvas/fonts/AmazonEmber_Th._V536571949_.ttf\") format(\"truetype\")}@font-face{font-family:\"AmazonEmber-Regular\";src:url(\"https://m.media-amazon.com/images/G/01/cloudcanvas/fonts/amazon_ember_rg-webfont._V536573323_.woff2\") format(\"woff2\"),url(\"https://m.media-amazon.com/images/G/01/cloudcanvas/fonts/amazon_ember_rg-webfont._V536573323_.woff\") format(\"woff\"),url(\"https://m.media-amazon.com/images/G/01/cloudcanvas/fonts/AmazonEmber_Rg._V536573323_.ttf\") format(\"truetype\")}@font-face{font-family:\"AmazonEmber-Bold\";src:url(\"https://m.media-amazon.com/images/G/01/cloudcanvas/fonts/amazonember-bold-webfont._V529584225_.woff2\") format(\"woff2\"),url(\"https://m.media-amazon.com/images/G/01/cloudcanvas/fonts/amazonember-bold-webfont._V529584225_.woff\") format(\"woff\"),url(\"https://m.media-amazon.com/images/G/01/cloudcanvas/fonts/AmazonEmber_Bd._V536571949_.ttf\") format(\"truetype\")}a{color:#6441A5;cursor:pointer}a:hover,a:active,a.active,a:focus{text-decoration:none;color:#8A0ECC}.sub-header-area.row{border-bottom:2px solid #eee;margin-left:-10px;margin-bottom:20px}.sub-header-area.row .sub-menu-item{text-align:center;padding:0px 10px 5px;margin-right:15px;position:relative;top:2px;color:#222}.sub-header-area.row .sub-menu-item-active{border-bottom:4px solid #6441A5;color:#6441A5}table.table thead tr{border-bottom:1px solid #eee}table.table thead tr th{font-size:12px;font-weight:normal;color:#222;padding-bottom:5px;border:none;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;cursor:default}table.table thead tr th div{display:inline-block}table.table thead tr th i{margin-left:3px}table.table tbody tr{border:1px solid #eee;font-size:14px}table.table tbody tr td{color:#444;padding:20px 13px}table.table tbody tr td a{color:#6441A5 !important}table.table tbody tr div.float-right i{padding:0 3px;cursor:pointer}@keyframes newrow{from{background-color:#f8f8f8;outline:1px solid #9a7fcd}to{background-color:#fff;outline:1px solid #eee}}table.table .new-row{background-color:#fff;border:1px solid #eee;animation-name:newrow;animation-duration:1s;animation-iteration-count:1}table.table.table-hover tbody tr:hover{outline:1px solid #9a7fcd;background-color:#f8f8f8;margin-top:1px;cursor:pointer}table.table [type=\"checkbox\"]:not(:checked)+label,table.table [type=\"checkbox\"]:checked+label{position:relative;padding-left:25px;cursor:pointer;margin-top:4px;margin-bottom:15px;margin-right:5px}.graph{width:450px;display:inline-block;margin:15px 25px 10px 0;border:1px solid #ccc;padding-bottom:15px;vertical-align:top;min-height:388px}.graph>*{text-align:center}.graph .title{margin-bottom:20px;border-bottom:1px solid #ccc;text-align:left;padding-left:15px;font-family:\"AmazonEmber-Regular\";background-color:#f8f8f8;padding:10px 10px 10px 15px}.chartarea{padding:10px 25px 0px 20px}.chart-legend .legend-labels{min-width:160px}body h1,body h2,body h3{font-family:\"AmazonEmber-Light\"}body *{font-family:\"AmazonEmber-Light\";font-size:14px}body h1{font-size:24px}body h2{font-size:20px}body h3{font-size:16px}.modal-lg .modal-content{margin:60px;min-height:200px}body .modal-header{padding:15px}body .modal-header h2{margin-bottom:0;padding-bottom:0}body .modal-header button>span{font-size:24px}.modal-body{padding:20px 15px}.modal-body .row>label{padding-left:0}.modal.show .modal-dialog{top:20%}.modal-footer{border-top:0}.modal-footer button.btn.l-tertiary{padding:5px 20px}.modal-footer button.btn.l-primary{padding:5px 25px}.date-time-picker-container label{font-family:\"AmazonEmber-Regular\"}.date-time-picker-container .seperator{padding:0 10px;font-size:12px}input.datepicker{height:38px}button,input,optgroup,select,textarea{font-size:14px;font-family:\"AmazonEmber-Light\"}.form-control{border:1px solid inherit;transition:border .3s ease-out;border-radius:2px;font-size:14px;padding:8px}.form-control:focus,.form-control:hover:not(:disabled){transition:border .3s ease-in;border:1px solid #6441A5}input{height:32px}input .full-width{width:100%}.row{margin-left:0;margin-right:0}.row.center{-ms-flex-align:center;align-items:center}label{font-size:12px;color:#222;font-family:\"AmazonEmber-Regular\";padding-bottom:0;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none}label.inline{margin-top:8px}textarea{color:#222}button:focus{outline:5px auto #6441A5}[type=\"checkbox\"]+label{font-family:\"AmazonEmber-Light\";font-size:14px;cursor:pointer}[type=\"checkbox\"]:not(:checked),[type=\"checkbox\"]:checked{position:absolute;left:-9999px}[type=\"checkbox\"]:not(:checked)+label,[type=\"checkbox\"]:checked+label{position:relative;padding-left:25px;cursor:pointer}[type=\"checkbox\"]:not(:checked)+label:before,[type=\"checkbox\"]:checked+label:before{content:'';position:absolute;width:20px;height:20px;border:1px solid #6441A5;background:#fff;border-radius:2px;left:0;padding:5px 0;transition:all .2s;top:0}[type=\"checkbox\"]:checked+label:before{background-color:#6441A5}[type=\"checkbox\"]:not(:checked)+label:after,[type=\"checkbox\"]:checked+label:after{font-family:\"FontAwesome\";content:\"\u2714\";color:#fff;position:absolute;top:0;left:5px;font-size:14px;transition:all .3s;width:20px}[type=\"checkbox\"]:not(:checked)+label:after{opacity:0;transform:scale(0)}[type=\"checkbox\"]:checked+label:after{opacity:1;transform:scale(1)}[type=\"checkbox\"]:disabled:checked+label:before{box-shadow:none;border-color:#999;background-color:#ccc}[type=\"checkbox\"]:disabled:not(:checked)+label:before{border-color:#999;background-color:#fff}[type=\"checkbox\"]:disabled:checked+label:after{color:#fff}[type=\"checkbox\"]:disabled+label{color:#6441A5}label:hover:before{border:1px solid #6441A5}.form-control[type=\"search\"]:focus{border-color:#6441A5}.bg-primary{background-color:#6441A5 !important}.text-info{color:#6441A5 !important}select option:hover{background-color:#6441A5;box-shadow:inset 20px 20px #6441A5}.dropdown-item.active,.dropdown-item:active{background-color:#6441A5}.d-inline-block{position:relative}.dropdown-menu{position:absolute}.dropdown-menu{padding:0}.dropdown-item{padding:5px inherit}#deploymentDropdown::after{display:none}pagination .page-link:focus,pagination .page-link:hover{background-color:#6441A5;color:#6441A5}pagination a:not([href]):not([tabindex]):focus,pagination a:not([href]):not([tabindex]):hover{color:#fff}label.radio-btn-label{margin-right:20px}input[type=\"radio\"]{display:none}input[type=\"radio\"]+span{background-color:white;border:1px solid;border-color:#6441A5;border-radius:50px;display:inline-block;float:left;margin-right:8px;padding:8px;position:relative}input[type=\"radio\"]:checked+span{color:#f00}input[type=\"radio\"]:checked+span:after{background:#6441A5;border-radius:50px;content:\" \";height:10px;width:10px;left:3px;position:absolute;top:3px}label:hover input[type=radio]+span,label:focus input[type=radio]+span{border-color:#6441A5;box-shadow:0px 1px 1px 0px #6441A5}ul>li{list-style-type:none;margin-bottom:10px}ul{padding-left:0;margin-bottom:20px}.dropdown-item.active,.dropdown-item:active{background-color:#6441A5}.d-inline-block{width:inherit}.dropdown-menu{padding:0}.dropdown-item{font-family:\"AmazonEmber-Light\"}#deploymentDropdown::after{display:none}pagination .page-link:focus,pagination .page-link:hover{background-color:#6441A5;color:#6441A5}pagination a:not([href]):not([tabindex]):focus,pagination a:not([href]):not([tabindex]):hover{color:#fff}.warning-overlay-color{color:#E57829}.success-overlay-color{color:#58BC61}.error-overlay-color{color:#e52929}.noUi-connect{background:#6441A5 !important}.tooltip.show,.tooltip-inner{background-color:#6441A5;padding:3px 5px;opacity:1;font-size:11px;text-align:left;max-width:400px}.tooltip.bs-tooltip-bottom .tooltip-inner::before,.tooltip.bs-tether-element-attached-top .tooltip-inner::before{border-bottom-color:#6441A5;margin-top:-5px}ngb-tooltip-window.tooltip.show.bs-tooltip-bottom{margin-top:8px}.tooltip.bs-tooltip-top .tooltip-inner::before,.tooltip.bs-tether-element-attached-bottom .tooltip-inner::before{border-top-color:#6441A5;margin-bottom:-5px}ngb-tooltip-window.tooltip.show.bs-tooltip-top{margin-top:-8px}.tooltip.bs-tooltip-right .tooltip-inner::before,.tooltip.bs-tether-element-attached-left .tooltip-inner::before{border-right-color:#6441A5;margin-left:-5px}ngb-tooltip-window.tooltip.show.bs-tooltip-right{margin-left:8px}.tooltip.bs-tooltip-left .tooltip-inner::before,.tooltip.bs-tether-element-attached-right .tooltip-inner::before{border-left-color:#6441A5;margin-right:-5px}ngb-tooltip-window.tooltip.show.bs-tooltip-left{margin-left:-8px}.ngx-datatable.material{background:#FFF;box-shadow:0 5px 5px -3px rgba(0,0,0,0.2),0 8px 10px 1px rgba(0,0,0,0.14),0 3px 14px 2px rgba(0,0,0,0.12)}.ngx-datatable.material.striped .datatable-row-odd{background:#eee}.ngx-datatable.material.single-selection .datatable-body-row.active,.ngx-datatable.material.single-selection .datatable-body-row.active .datatable-row-group,.ngx-datatable.material.multi-selection .datatable-body-row.active,.ngx-datatable.material.multi-selection .datatable-body-row.active .datatable-row-group,.ngx-datatable.material.multi-click-selection .datatable-body-row.active,.ngx-datatable.material.multi-click-selection .datatable-body-row.active .datatable-row-group{background-color:#304FFE;color:#FFF}.ngx-datatable.material.single-selection .datatable-body-row.active:hover,.ngx-datatable.material.single-selection .datatable-body-row.active:hover .datatable-row-group,.ngx-datatable.material.multi-selection .datatable-body-row.active:hover,.ngx-datatable.material.multi-selection .datatable-body-row.active:hover .datatable-row-group,.ngx-datatable.material.multi-click-selection .datatable-body-row.active:hover,.ngx-datatable.material.multi-click-selection .datatable-body-row.active:hover .datatable-row-group{background-color:#193AE4;color:#FFF}.ngx-datatable.material.single-selection .datatable-body-row.active:focus,.ngx-datatable.material.single-selection .datatable-body-row.active:focus .datatable-row-group,.ngx-datatable.material.multi-selection .datatable-body-row.active:focus,.ngx-datatable.material.multi-selection .datatable-body-row.active:focus .datatable-row-group,.ngx-datatable.material.multi-click-selection .datatable-body-row.active:focus,.ngx-datatable.material.multi-click-selection .datatable-body-row.active:focus .datatable-row-group{background-color:#2041EF;color:#FFF}.ngx-datatable.material:not(.cell-selection) .datatable-body-row:hover,.ngx-datatable.material:not(.cell-selection) .datatable-body-row:hover .datatable-row-group{background-color:#eee;transition-property:background;transition-duration:.3s;transition-timing-function:linear}.ngx-datatable.material:not(.cell-selection) .datatable-body-row:focus,.ngx-datatable.material:not(.cell-selection) .datatable-body-row:focus .datatable-row-group{background-color:#ddd}.ngx-datatable.material.cell-selection .datatable-body-cell:hover,.ngx-datatable.material.cell-selection .datatable-body-cell:hover .datatable-row-group{background-color:#eee;transition-property:background;transition-duration:.3s;transition-timing-function:linear}.ngx-datatable.material.cell-selection .datatable-body-cell:focus,.ngx-datatable.material.cell-selection .datatable-body-cell:focus .datatable-row-group{background-color:#ddd}.ngx-datatable.material.cell-selection .datatable-body-cell.active,.ngx-datatable.material.cell-selection .datatable-body-cell.active .datatable-row-group{background-color:#304FFE;color:#FFF}.ngx-datatable.material.cell-selection .datatable-body-cell.active:hover,.ngx-datatable.material.cell-selection .datatable-body-cell.active:hover .datatable-row-group{background-color:#193AE4;color:#FFF}.ngx-datatable.material.cell-selection .datatable-body-cell.active:focus,.ngx-datatable.material.cell-selection .datatable-body-cell.active:focus .datatable-row-group{background-color:#2041EF;color:#FFF}.ngx-datatable.material .empty-row{height:50px;text-align:left;padding:.5rem 1.2rem;vertical-align:top;border-top:0}.ngx-datatable.material .loading-row{text-align:left;padding:.5rem 1.2rem;vertical-align:top;border-top:0}.ngx-datatable.material .datatable-header .datatable-row-left,.ngx-datatable.material .datatable-body .datatable-row-left{background-color:#FFF;background-position:100% 0;background-repeat:repeat-y;background-image:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAQAAAABCAYAAAD5PA/NAAAAFklEQVQIHWPSkNeSBmJhTQVtbiDNCgASagIIuJX8OgAAAABJRU5ErkJggg==)}.ngx-datatable.material .datatable-header .datatable-row-right,.ngx-datatable.material .datatable-body .datatable-row-right{background-position:0 0;background-color:#fff;background-repeat:repeat-y;background-image:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAQAAAABCAYAAAD5PA/NAAAAFklEQVQI12PQkNdi1VTQ5gbSwkAsDQARLAIGtOSFUAAAAABJRU5ErkJggg==)}.ngx-datatable.material .datatable-header{border-bottom:1px solid rgba(0,0,0,0.12)}.ngx-datatable.material .datatable-header .datatable-header-cell{text-align:left;padding:.9rem 1.2rem;font-weight:400;color:rgba(0,0,0,0.54);vertical-align:bottom;font-size:12px;font-weight:500}.ngx-datatable.material .datatable-header .datatable-header-cell .datatable-header-cell-wrapper{position:relative}.ngx-datatable.material .datatable-header .datatable-header-cell.longpress .draggable::after{transition:transform 400ms ease, opacity 400ms ease;opacity:.5;transform:scale(1)}.ngx-datatable.material .datatable-header .datatable-header-cell .draggable::after{content:\" \";position:absolute;top:50%;left:50%;margin:-30px 0 0 -30px;height:60px;width:60px;background:#eee;border-radius:100%;opacity:1;filter:none;transform:scale(0);z-index:9999;pointer-events:none}.ngx-datatable.material .datatable-header .datatable-header-cell.dragging .resize-handle{border-right:none}.ngx-datatable.material .datatable-header .resize-handle{border-right:solid 1px #eee}.ngx-datatable.material .datatable-body .datatable-row-detail{background:#f5f5f5;padding:10px}.ngx-datatable.material .datatable-body .datatable-group-header{background:#f5f5f5;border-bottom:solid 1px #D9D8D9;border-top:solid 1px #D9D8D9}.ngx-datatable.material .datatable-body .datatable-body-row .datatable-body-cell{text-align:left;padding:.9rem 1.2rem;vertical-align:top;border-top:0;color:rgba(0,0,0,0.87);transition:width 0.3s ease;font-size:14px;font-weight:400}.ngx-datatable.material .datatable-body .datatable-body-row .datatable-body-group-cell{text-align:left;padding:.9rem 1.2rem;vertical-align:top;border-top:0;color:rgba(0,0,0,0.87);transition:width 0.3s ease;font-size:14px;font-weight:400}.ngx-datatable.material .datatable-body .progress-linear{display:block;position:relative;width:100%;height:5px;padding:0;margin:0;position:absolute}.ngx-datatable.material .datatable-body .progress-linear .container{display:block;position:relative;overflow:hidden;width:100%;height:5px;transform:translate(0, 0) scale(1, 1);background-color:#aad1f9}.ngx-datatable.material .datatable-body .progress-linear .container .bar{transition:all .2s linear;animation:query 0.8s infinite cubic-bezier(0.39, 0.575, 0.565, 1);transition:transform .2s linear;background-color:#106cc8;position:absolute;left:0;top:0;bottom:0;width:100%;height:5px}.ngx-datatable.material .datatable-footer{border-top:1px solid rgba(0,0,0,0.12);font-size:12px;font-weight:400;color:rgba(0,0,0,0.54)}.ngx-datatable.material .datatable-footer .page-count{line-height:50px;height:50px;padding:0 1.2rem}.ngx-datatable.material .datatable-footer .datatable-pager{margin:0 10px}.ngx-datatable.material .datatable-footer .datatable-pager li{vertical-align:middle}.ngx-datatable.material .datatable-footer .datatable-pager li.disabled a{color:rgba(0,0,0,0.26) !important;background-color:transparent !important}.ngx-datatable.material .datatable-footer .datatable-pager li.active a{background-color:rgba(158,158,158,0.2);font-weight:bold}.ngx-datatable.material .datatable-footer .datatable-pager a{height:22px;min-width:24px;line-height:22px;padding:0 6px;border-radius:3px;margin:6px 3px;text-align:center;vertical-align:top;color:rgba(0,0,0,0.54);text-decoration:none;vertical-align:bottom}.ngx-datatable.material .datatable-footer .datatable-pager a:hover{color:rgba(0,0,0,0.75);background-color:rgba(158,158,158,0.2)}.ngx-datatable.material .datatable-footer .datatable-pager .datatable-icon-left,.ngx-datatable.material .datatable-footer .datatable-pager .datatable-icon-skip,.ngx-datatable.material .datatable-footer .datatable-pager .datatable-icon-right,.ngx-datatable.material .datatable-footer .datatable-pager .datatable-icon-prev{font-size:20px;line-height:20px;padding:0 3px}.datatable-checkbox{position:relative;margin:0;cursor:pointer;vertical-align:middle;display:inline-block;box-sizing:border-box;padding:10px 0}.datatable-checkbox input[type='checkbox']{position:relative;margin:0 1rem 0 0;cursor:pointer;outline:none}.datatable-checkbox input[type='checkbox']:before{transition:all 0.3s ease-in-out;content:\"\";position:absolute;left:0;z-index:1;width:1rem;height:1rem;border:2px solid #f2f2f2}.datatable-checkbox input[type='checkbox']:checked:before{transform:rotate(-45deg);height:.5rem;border-color:#009688;border-top-style:none;border-right-style:none}.datatable-checkbox input[type='checkbox']:after{content:\"\";position:absolute;top:0;left:0;width:1rem;height:1rem;background:#fff;cursor:pointer}@keyframes query{0%{opacity:1;transform:translateX(35%) scale(0.3, 1)}100%{opacity:0;transform:translateX(-50%) scale(0, 1)}}.ngx-datatable{display:block;overflow:hidden;-ms-flex-pack:center;justify-content:center;position:relative;-webkit-transform:translate3d(0, 0, 0)}.ngx-datatable [hidden]{display:none !important}.ngx-datatable *,.ngx-datatable *:before,.ngx-datatable *:after{box-sizing:border-box}.ngx-datatable.scroll-vertical .datatable-body{overflow-y:auto}.ngx-datatable.scroll-vertical .datatable-body .datatable-row-wrapper{position:absolute}.ngx-datatable.scroll-horz .datatable-body{overflow-x:auto;-webkit-overflow-scrolling:touch}.ngx-datatable.fixed-header .datatable-header .datatable-header-inner{white-space:nowrap}.ngx-datatable.fixed-header .datatable-header .datatable-header-inner .datatable-header-cell{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.ngx-datatable.fixed-row .datatable-scroll{white-space:nowrap}.ngx-datatable.fixed-row .datatable-scroll .datatable-body-row{white-space:nowrap}.ngx-datatable.fixed-row .datatable-scroll .datatable-body-row .datatable-body-cell{overflow:hidden;white-space:nowrap;text-overflow:ellipsis}.ngx-datatable.fixed-row .datatable-scroll .datatable-body-row .datatable-body-group-cell{overflow:hidden;white-space:nowrap;text-overflow:ellipsis}.ngx-datatable .datatable-body-row,.ngx-datatable .datatable-header-inner{display:-ms-flexbox;display:flex;-ms-flex-direction:row;flex-direction:row;-ms-flex-flow:row;-o-flex-flow:row;flex-flow:row}.ngx-datatable .datatable-body-cell,.ngx-datatable .datatable-header-cell{vertical-align:top;display:inline-block;line-height:1.625}.ngx-datatable .datatable-body-cell:focus,.ngx-datatable .datatable-header-cell:focus{outline:none}.ngx-datatable .datatable-row-left,.ngx-datatable .datatable-row-right{z-index:9}.ngx-datatable .datatable-row-left,.ngx-datatable .datatable-row-center,.ngx-datatable .datatable-row-grouping,.ngx-datatable .datatable-row-right{position:relative}.ngx-datatable .datatable-header{display:block;overflow:hidden}.ngx-datatable .datatable-header .datatable-header-inner{-ms-flex-align:stretch;align-items:stretch;-webkit-align-items:stretch}.ngx-datatable .datatable-header .datatable-header-cell{position:relative;display:inline-block}.ngx-datatable .datatable-header .datatable-header-cell.sortable .datatable-header-cell-wrapper{cursor:pointer}.ngx-datatable .datatable-header .datatable-header-cell.longpress .datatable-header-cell-wrapper{cursor:move}.ngx-datatable .datatable-header .datatable-header-cell .sort-btn{line-height:100%;vertical-align:middle;display:inline-block;cursor:pointer}.ngx-datatable .datatable-header .datatable-header-cell .resize-handle{display:inline-block;position:absolute;right:0;top:0;bottom:0;width:5px;padding:0 4px;visibility:hidden;cursor:ew-resize}.ngx-datatable .datatable-header .datatable-header-cell.resizeable:hover .resize-handle{visibility:visible}.ngx-datatable .datatable-body{position:relative;z-index:10;display:block}.ngx-datatable .datatable-body .datatable-scroll{display:inline-block}.ngx-datatable .datatable-body .datatable-row-detail{overflow-y:hidden}.ngx-datatable .datatable-body .datatable-row-wrapper{display:-ms-flexbox;display:flex;-ms-flex-direction:column;flex-direction:column}.ngx-datatable .datatable-body .datatable-body-row{outline:none}.ngx-datatable .datatable-body .datatable-body-row>div{display:-ms-flexbox;display:flex}.ngx-datatable .datatable-footer{display:block;width:100%}.ngx-datatable .datatable-footer .datatable-footer-inner{display:-ms-flexbox;display:flex;-ms-flex-align:center;align-items:center;width:100%}.ngx-datatable .datatable-footer .selected-count .page-count{-ms-flex:1 1 40%;flex:1 1 40%}.ngx-datatable .datatable-footer .selected-count .datatable-pager{-ms-flex:1 1 60%;flex:1 1 60%}.ngx-datatable .datatable-footer .page-count{-ms-flex:1 1 20%;flex:1 1 20%}.ngx-datatable .datatable-footer .datatable-pager{-ms-flex:1 1 80%;flex:1 1 80%;text-align:right}.ngx-datatable .datatable-footer .datatable-pager .pager,.ngx-datatable .datatable-footer .datatable-pager .pager li{padding:0;margin:0;display:inline-block;list-style:none}.ngx-datatable .datatable-footer .datatable-pager .pager li,.ngx-datatable .datatable-footer .datatable-pager .pager li a{outline:none}.ngx-datatable .datatable-footer .datatable-pager .pager li a{cursor:pointer;display:inline-block}.ngx-datatable .datatable-footer .datatable-pager .pager li.disabled a{cursor:not-allowed}#toast-container.toast-bottom-right>div{background-color:white;width:400px;color:black;cursor:default;box-shadow:none}#toast-container.toast-bottom-right>div:hover{box-shadow:none}#toast-container.toast-bottom-right>div .toast-close-button{color:black}#toast-container.toast-bottom-right>div .toast-close-button:focus,#toast-container.toast-bottom-right>div .toast-close-button:hover{opacity:1}#toast-container.toast-bottom-right>div .cgp-toastr-message{font-size:12px}#toast-container.toast-bottom-right>div .toast-close-button{font-family:\"FontAwesome\";content:\"\\f00d\";font-size:24px;position:absolute;right:6px;color:#ccc}#toast-container.toast-bottom-right>div.toast-success{border:1px solid #5CBA65;border-top:4px solid #5CBA65}#toast-container.toast-bottom-right>div.toast-success .cgp-toastr-message:before{font-family:\"FontAwesome\";content:\"\\f00c\";font-size:22px;position:absolute;left:15px;top:10px;color:#5CBA65}#toast-container.toast-bottom-right>div.toast-warning{border:1px solid orange;border-top:4px solid orange}#toast-container.toast-bottom-right>div.toast-warning .cgp-toastr-message:before{font-family:\"FontAwesome\";content:\"\\f071\";font-size:22px;position:absolute;left:15px;top:10px;color:orange}#toast-container.toast-bottom-right>div.toast-error{border:1px solid red;border-top:4px solid red}#toast-container.toast-bottom-right>div.toast-error .cgp-toastr-message:before{font-family:\"FontAwesome\";content:\"\\f071\";font-size:22px;position:absolute;left:15px;top:10px;color:red}#toast-container.toast-bottom-right>div.toast-info{border:1px solid #6441A5;border-top:4px solid #6441A5}.ignore-col-margin{margin-left:-15px}.visibility-hidden{visibility:hidden}.cursor-pointer{cursor:pointer}.no-padding{padding:0}.no-margin{margin:0}/*! nouislider - 10.1.0 - 2017-07-28 13:09:54 */.noUi-target,.noUi-target *{-webkit-touch-callout:none;-webkit-tap-highlight-color:transparent;-webkit-user-select:none;-ms-touch-action:none;touch-action:none;-ms-user-select:none;-moz-user-select:none;user-select:none;box-sizing:border-box}.noUi-target{position:relative;direction:ltr}.noUi-base{width:100%;height:100%;position:relative;z-index:1}.noUi-connect{position:absolute;right:0;top:0;left:0;bottom:0}.noUi-origin{position:absolute;height:0;width:0}.noUi-handle{position:relative;z-index:1}.noUi-state-tap .noUi-connect,.noUi-state-tap .noUi-origin{transition:top .3s,right .3s,bottom .3s,left .3s}.noUi-state-drag *{cursor:inherit !important}.noUi-base,.noUi-handle{transform:translate3d(0, 0, 0)}.noUi-horizontal{height:18px}.noUi-horizontal .noUi-handle{width:34px;height:28px;left:-17px;top:-6px}.noUi-vertical{width:18px}.noUi-vertical .noUi-handle{width:28px;height:34px;left:-6px;top:-17px}.noUi-target{background:#FAFAFA;border-radius:4px;border:1px solid #D3D3D3;box-shadow:inset 0 1px 1px #F0F0F0,0 3px 6px -5px #BBB}.noUi-connect{background:#3FB8AF;border-radius:4px;box-shadow:inset 0 0 3px rgba(51,51,51,0.45);transition:background 450ms}.noUi-draggable{cursor:ew-resize}.noUi-vertical .noUi-draggable{cursor:ns-resize}.noUi-handle{border:1px solid #D9D9D9;border-radius:3px;background:#FFF;cursor:default;box-shadow:inset 0 0 1px #FFF,inset 0 1px 7px #EBEBEB,0 3px 6px -3px #BBB}.noUi-active{box-shadow:inset 0 0 1px #FFF,inset 0 1px 7px #DDD,0 3px 6px -3px #BBB}.noUi-handle:after,.noUi-handle:before{content:\"\";display:block;position:absolute;height:14px;width:1px;background:#E8E7E6;left:14px;top:6px}.noUi-handle:after{left:17px}.noUi-vertical .noUi-handle:after,.noUi-vertical .noUi-handle:before{width:14px;height:1px;left:6px;top:14px}.noUi-vertical .noUi-handle:after{top:17px}[disabled] .noUi-connect{background:#B8B8B8}[disabled] .noUi-handle,[disabled].noUi-handle,[disabled].noUi-target{cursor:not-allowed}.noUi-pips,.noUi-pips *{box-sizing:border-box}.noUi-pips{position:absolute;color:#999}.noUi-value{position:absolute;white-space:nowrap;text-align:center}.noUi-value-sub{color:#ccc;font-size:10px}.noUi-marker{position:absolute;background:#CCC}.noUi-marker-large,.noUi-marker-sub{background:#AAA}.noUi-pips-horizontal{padding:10px 0;height:80px;top:100%;left:0;width:100%}.noUi-value-horizontal{transform:translate3d(-50%, 50%, 0)}.noUi-marker-horizontal.noUi-marker{margin-left:-1px;width:2px;height:5px}.noUi-marker-horizontal.noUi-marker-sub{height:10px}.noUi-marker-horizontal.noUi-marker-large{height:15px}.noUi-pips-vertical{padding:0 10px;height:100%;top:0;left:100%}.noUi-value-vertical{transform:translate3d(0, 50%, 0);padding-left:25px}.noUi-marker-vertical.noUi-marker{width:5px;height:2px;margin-top:-1px}.noUi-marker-vertical.noUi-marker-sub{width:10px}.noUi-marker-vertical.noUi-marker-large{width:15px}.noUi-tooltip{display:block;position:absolute;border:1px solid #D9D9D9;border-radius:3px;background:#fff;color:#000;padding:5px;text-align:center;white-space:nowrap}.noUi-horizontal .noUi-tooltip{transform:translate(-50%, 0);left:50%;bottom:120%}.noUi-vertical .noUi-tooltip{transform:translate(0, -50%);top:50%;right:120%}.leaflet-control-layers-base label div{margin-bottom:0}.leaflet-control-layers-base label div span{margin-bottom:5px}.leaflet-control-layers-base label div span:after{display:none}.leaflet-pane,.leaflet-tile,.leaflet-marker-icon,.leaflet-marker-shadow,.leaflet-tile-container,.leaflet-pane>svg,.leaflet-pane>canvas,.leaflet-zoom-box,.leaflet-image-layer,.leaflet-layer{position:absolute;left:0;top:0}.leaflet-container{overflow:hidden}.leaflet-tile,.leaflet-marker-icon,.leaflet-marker-shadow{-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;-webkit-user-drag:none}.leaflet-safari .leaflet-tile{image-rendering:-webkit-optimize-contrast}.leaflet-safari .leaflet-tile-container{width:1600px;height:1600px;-webkit-transform-origin:0 0}.leaflet-marker-icon,.leaflet-marker-shadow{display:block}.leaflet-container .leaflet-overlay-pane svg,.leaflet-container .leaflet-marker-pane img,.leaflet-container .leaflet-shadow-pane img,.leaflet-container .leaflet-tile-pane img,.leaflet-container img.leaflet-image-layer{max-width:none !important}.leaflet-container.leaflet-touch-zoom{-ms-touch-action:pan-x pan-y;touch-action:pan-x pan-y}.leaflet-container.leaflet-touch-drag{-ms-touch-action:pinch-zoom}.leaflet-container.leaflet-touch-drag.leaflet-touch-zoom{-ms-touch-action:none;touch-action:none}.leaflet-container{-webkit-tap-highlight-color:transparent}.leaflet-container a{-webkit-tap-highlight-color:rgba(51,181,229,0.4)}.leaflet-tile{filter:inherit;visibility:hidden}.leaflet-tile-loaded{visibility:inherit}.leaflet-zoom-box{width:0;height:0;box-sizing:border-box;z-index:800}.leaflet-overlay-pane svg{-moz-user-select:none}.leaflet-pane{z-index:400}.leaflet-tile-pane{z-index:200}.leaflet-overlay-pane{z-index:400}.leaflet-shadow-pane{z-index:500}.leaflet-marker-pane{z-index:600}.leaflet-tooltip-pane{z-index:650}.leaflet-popup-pane{z-index:700}.leaflet-map-pane canvas{z-index:100}.leaflet-map-pane svg{z-index:200}.leaflet-vml-shape{width:1px;height:1px}.lvml{behavior:url(#default#VML);display:inline-block;position:absolute}.leaflet-control{position:relative;z-index:800;pointer-events:visiblePainted;pointer-events:auto}.leaflet-top,.leaflet-bottom{position:absolute;z-index:1000;pointer-events:none}.leaflet-top{top:0}.leaflet-right{right:0}.leaflet-bottom{bottom:0}.leaflet-left{left:0}.leaflet-control{float:left;clear:both}.leaflet-right .leaflet-control{float:right}.leaflet-top .leaflet-control{margin-top:10px}.leaflet-bottom .leaflet-control{margin-bottom:10px}.leaflet-left .leaflet-control{margin-left:10px}.leaflet-right .leaflet-control{margin-right:10px}.leaflet-fade-anim .leaflet-tile{will-change:opacity}.leaflet-fade-anim .leaflet-popup{opacity:0;transition:opacity 0.2s linear}.leaflet-fade-anim .leaflet-map-pane .leaflet-popup{opacity:1}.leaflet-zoom-animated{transform-origin:0 0}.leaflet-zoom-anim .leaflet-zoom-animated{will-change:transform}.leaflet-zoom-anim .leaflet-zoom-animated{transition:transform 0.25s cubic-bezier(0, 0, 0.25, 1)}.leaflet-zoom-anim .leaflet-tile,.leaflet-pan-anim .leaflet-tile{transition:none}.leaflet-zoom-anim .leaflet-zoom-hide{visibility:hidden}.leaflet-interactive{cursor:pointer}.leaflet-grab{cursor:-webkit-grab;cursor:-moz-grab}.leaflet-crosshair,.leaflet-crosshair .leaflet-interactive{cursor:crosshair}.leaflet-popup-pane,.leaflet-control{cursor:auto}.leaflet-dragging .leaflet-grab,.leaflet-dragging .leaflet-grab .leaflet-interactive,.leaflet-dragging .leaflet-marker-draggable{cursor:move;cursor:-webkit-grabbing;cursor:-moz-grabbing}.leaflet-marker-icon,.leaflet-marker-shadow,.leaflet-image-layer,.leaflet-pane>svg path,.leaflet-tile-container{pointer-events:none}.leaflet-marker-icon.leaflet-interactive,.leaflet-image-layer.leaflet-interactive,.leaflet-pane>svg path.leaflet-interactive{pointer-events:visiblePainted;pointer-events:auto}.leaflet-container{background:#ddd;outline:0}.leaflet-container a{color:#0078A8}.leaflet-container a.leaflet-active{outline:2px solid orange}.leaflet-zoom-box{border:2px dotted #38f;background:rgba(255,255,255,0.5)}.leaflet-container{font:12px/1.5 \"Helvetica Neue\", Arial, Helvetica, sans-serif}.leaflet-bar{box-shadow:0 1px 5px rgba(0,0,0,0.65);border-radius:4px}.leaflet-bar a,.leaflet-bar a:hover{background-color:#fff;border-bottom:1px solid #ccc;width:26px;height:26px;line-height:26px;display:block;text-align:center;text-decoration:none;color:black}.leaflet-bar a,.leaflet-control-layers-toggle{background-position:50% 50%;background-repeat:no-repeat;display:block}.leaflet-bar a:hover{background-color:#f4f4f4}.leaflet-bar a:first-child{border-top-left-radius:4px;border-top-right-radius:4px}.leaflet-bar a:last-child{border-bottom-left-radius:4px;border-bottom-right-radius:4px;border-bottom:none}.leaflet-bar a.leaflet-disabled{cursor:default;background-color:#f4f4f4;color:#bbb}.leaflet-touch .leaflet-bar a{width:30px;height:30px;line-height:30px}.leaflet-touch .leaflet-bar a:first-child{border-top-left-radius:2px;border-top-right-radius:2px}.leaflet-touch .leaflet-bar a:last-child{border-bottom-left-radius:2px;border-bottom-right-radius:2px}.leaflet-control-zoom-in,.leaflet-control-zoom-out{font:bold 18px 'Lucida Console', Monaco, monospace;text-indent:1px}.leaflet-touch .leaflet-control-zoom-in,.leaflet-touch .leaflet-control-zoom-out{font-size:22px}.leaflet-control-layers{box-shadow:0 1px 5px rgba(0,0,0,0.4);background:#fff;border-radius:5px}.leaflet-control-layers-toggle{background-image:url(https://m.media-amazon.com/images/G/01/cloudcanvas/leaflet/layers._CB490865608_.png);width:36px;height:36px}.leaflet-retina .leaflet-control-layers-toggle{background-image:url(https://m.media-amazon.com/images/G/01/cloudcanvas/leaflet/layers-2x._CB490865608_.png);background-size:26px 26px}.leaflet-touch .leaflet-control-layers-toggle{width:44px;height:44px}.leaflet-control-layers .leaflet-control-layers-list,.leaflet-control-layers-expanded .leaflet-control-layers-toggle{display:none}.leaflet-control-layers-expanded .leaflet-control-layers-list{display:block;position:relative}.leaflet-control-layers-expanded{padding:6px 10px 6px 6px;color:#333;background:#fff}.leaflet-control-layers-scrollbar{overflow-y:scroll;overflow-x:hidden;padding-right:5px}.leaflet-control-layers-selector{margin-top:2px;position:relative;top:1px}.leaflet-control-layers label{display:block}.leaflet-control-layers-separator{height:0;border-top:1px solid #ddd;margin:5px -10px 5px -6px}.leaflet-default-icon-path{background-image:url(https://m.media-amazon.com/images/G/01/cloudcanvas/leaflet/marker-icon._CB490865608_.png)}.leaflet-container .leaflet-control-attribution{background:#fff;background:rgba(255,255,255,0.7);margin:0}.leaflet-control-attribution,.leaflet-control-scale-line{padding:0 5px;color:#333}.leaflet-control-attribution a{text-decoration:none}.leaflet-control-attribution a:hover{text-decoration:underline}.leaflet-container .leaflet-control-attribution,.leaflet-container .leaflet-control-scale{font-size:11px}.leaflet-left .leaflet-control-scale{margin-left:5px}.leaflet-bottom .leaflet-control-scale{margin-bottom:5px}.leaflet-control-scale-line{border:2px solid #777;border-top:none;line-height:1.1;padding:2px 5px 1px;font-size:11px;white-space:nowrap;overflow:hidden;box-sizing:border-box;background:#fff;background:rgba(255,255,255,0.5)}.leaflet-control-scale-line:not(:first-child){border-top:2px solid #777;border-bottom:none;margin-top:-2px}.leaflet-control-scale-line:not(:first-child):not(:last-child){border-bottom:2px solid #777}.leaflet-touch .leaflet-control-attribution,.leaflet-touch .leaflet-control-layers,.leaflet-touch .leaflet-bar{box-shadow:none}.leaflet-touch .leaflet-control-layers,.leaflet-touch .leaflet-bar{border:2px solid rgba(0,0,0,0.2);background-clip:padding-box}.leaflet-popup{position:absolute;text-align:center;margin-bottom:20px}.leaflet-popup-content-wrapper{padding:1px;text-align:left;border-radius:12px}.leaflet-popup-content{margin:13px 19px;line-height:1.4}.leaflet-popup-content p{margin:18px 0}.leaflet-popup-tip-container{width:40px;height:20px;position:absolute;left:50%;margin-left:-20px;overflow:hidden;pointer-events:none}.leaflet-popup-tip{width:17px;height:17px;padding:1px;margin:-10px auto 0;transform:rotate(45deg)}.leaflet-popup-content-wrapper,.leaflet-popup-tip{background:white;color:#333;box-shadow:0 3px 14px rgba(0,0,0,0.4)}.leaflet-container a.leaflet-popup-close-button{position:absolute;top:0;right:0;padding:4px 4px 0 0;border:none;text-align:center;width:18px;height:14px;font:16px/14px Tahoma, Verdana, sans-serif;color:#c3c3c3;text-decoration:none;font-weight:bold;background:transparent}.leaflet-container a.leaflet-popup-close-button:hover{color:#999}.leaflet-popup-scrolled{overflow:auto;border-bottom:1px solid #ddd;border-top:1px solid #ddd}.leaflet-oldie .leaflet-popup-content-wrapper{zoom:1}.leaflet-oldie .leaflet-popup-tip{width:24px;margin:0 auto;-ms-filter:\"progid:DXImageTransform.Microsoft.Matrix(M11=0.70710678, M12=0.70710678, M21=-0.70710678, M22=0.70710678)\";filter:progid:DXImageTransform.Microsoft.Matrix(M11=0.70710678, M12=0.70710678, M21=-0.70710678, M22=0.70710678)}.leaflet-oldie .leaflet-popup-tip-container{margin-top:-1px}.leaflet-oldie .leaflet-control-zoom,.leaflet-oldie .leaflet-control-layers,.leaflet-oldie .leaflet-popup-content-wrapper,.leaflet-oldie .leaflet-popup-tip{border:1px solid #999}.leaflet-div-icon{background:#fff;border:1px solid #666}.leaflet-tooltip{position:absolute;padding:6px;background-color:#fff;border:1px solid #fff;border-radius:3px;color:#222;white-space:nowrap;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;pointer-events:none;box-shadow:0 1px 3px rgba(0,0,0,0.4)}.leaflet-tooltip.leaflet-clickable{cursor:pointer;pointer-events:auto}.leaflet-tooltip-top:before,.leaflet-tooltip-bottom:before,.leaflet-tooltip-left:before,.leaflet-tooltip-right:before{position:absolute;pointer-events:none;border:6px solid transparent;background:transparent;content:\"\"}.leaflet-tooltip-bottom{margin-top:6px}.leaflet-tooltip-top{margin-top:-6px}.leaflet-tooltip-bottom:before,.leaflet-tooltip-top:before{left:50%;margin-left:-6px}.leaflet-tooltip-top:before{bottom:0;margin-bottom:-12px;border-top-color:#fff}.leaflet-tooltip-bottom:before{top:0;margin-top:-12px;margin-left:-6px;border-bottom-color:#fff}.leaflet-tooltip-left{margin-left:-6px}.leaflet-tooltip-right{margin-left:6px}.leaflet-tooltip-left:before,.leaflet-tooltip-right:before{top:50%;margin-top:-6px}.leaflet-tooltip-left:before{right:0;margin-right:-12px;border-left-color:#fff}.leaflet-tooltip-right:before{left:0;margin-left:-12px;border-right-color:#fff}html{overflow-x:hidden;margin-right:calc(-1 * (100vw - 100%))}html,body{height:100%;margin:0}.cgp-container{min-height:100%;margin-bottom:-35px}.footer-push{height:35px}.cgp-login-container>h1,.cgp-login-container>h2{text-align:center;margin-bottom:15px}.cgp-login-container>h2+loading-spinner>div{padding-top:50px}img{max-width:100%}i.refresh-icon{float:right;padding-right:20px;cursor:pointer}"],
                    encapsulation: core_1.ViewEncapsulation.None
                }), __metadata("design:paramtypes", [index_1.DefinitionService, aws_service_1.AwsService, index_1.UrlService, ng2_toastr_1.ToastsManager, core_1.ViewContainerRef])], AppComponent);
                return AppComponent;
            }();
            exports_1("AppComponent", AppComponent);
        }
    };
});
System.register("dist/app/view/error/component/page-not-found.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, PageNotFoundComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            PageNotFoundComponent = /** @class */function () {
                function PageNotFoundComponent() {}
                PageNotFoundComponent = __decorate([core_1.Component({
                    selector: 'page-not-found',
                    template: "\n       We seem to have found a dead end.  The route defined was unexpected.\n    ",
                    styles: [""]
                })], PageNotFoundComponent);
                return PageNotFoundComponent;
            }();
            exports_1("PageNotFoundComponent", PageNotFoundComponent);
        }
    };
});
System.register("dist/app/view/error/error.module.js", ["@angular/core", "app/view/error/component/page-not-found.component"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, page_not_found_component_1, ErrorModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (page_not_found_component_1_1) {
            page_not_found_component_1 = page_not_found_component_1_1;
        }],
        execute: function () {
            ErrorModule = /** @class */function () {
                function ErrorModule() {}
                ErrorModule = __decorate([core_1.NgModule({
                    imports: [],
                    exports: [page_not_found_component_1.PageNotFoundComponent],
                    declarations: [page_not_found_component_1.PageNotFoundComponent]
                })], ErrorModule);
                return ErrorModule;
            }();
            exports_1("ErrorModule", ErrorModule);
        }
    };
});
System.register("dist/app/view/authentication/auth.routes.js", ["@angular/core", "@angular/router", "./component/authentication.component"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, router_1, authentication_component_1, AuthRoutingModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }, function (authentication_component_1_1) {
            authentication_component_1 = authentication_component_1_1;
        }],
        execute: function () {
            AuthRoutingModule = /** @class */function () {
                function AuthRoutingModule() {}
                AuthRoutingModule = __decorate([core_1.NgModule({
                    imports: [router_1.RouterModule.forChild([{
                        path: 'login',
                        component: authentication_component_1.AuthenticationComponent
                    }])],
                    exports: [router_1.RouterModule]
                })], AuthRoutingModule);
                return AuthRoutingModule;
            }();
            exports_1("AuthRoutingModule", AuthRoutingModule);
        }
    };
});
System.register("dist/app/view/authentication/component/authentication.component.js", ["@angular/core", "app/aws/aws.service", "app/aws/authentication/authentication.class", "@angular/forms", "app/shared/service/index", "app/shared/component/index", "ng2-toastr"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, authentication_class_1, forms_1, index_1, index_2, ng2_toastr_1, EnumLoginState, AuthenticationComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (authentication_class_1_1) {
            authentication_class_1 = authentication_class_1_1;
        }, function (forms_1_1) {
            forms_1 = forms_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }],
        execute: function () {
            (function (EnumLoginState) {
                EnumLoginState[EnumLoginState["LOGIN"] = -1] = "LOGIN";
                EnumLoginState[EnumLoginState["FORGOT_PASSWORD"] = -2] = "FORGOT_PASSWORD";
                EnumLoginState[EnumLoginState["ENTER_CODE"] = -3] = "ENTER_CODE";
                EnumLoginState[EnumLoginState["RESEND_CODE"] = -4] = "RESEND_CODE";
            })(EnumLoginState || (EnumLoginState = {}));
            exports_1("EnumLoginState", EnumLoginState);
            AuthenticationComponent = /** @class */function () {
                function AuthenticationComponent(fb, aws, urlService, toastr, vcr) {
                    this.fb = fb;
                    this.aws = aws;
                    this.urlService = urlService;
                    this.toastr = toastr;
                    this._storageKeyHasLoggedIn = "cgp-logged-in-user";
                    this.toastr.setRootViewContainerRef(vcr);
                    this.userLoginForm = fb.group({
                        'username': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'password': [null, forms_1.Validators.compose([forms_1.Validators.required])]
                    });
                    this.userCodeForm = fb.group({
                        'code': [null, forms_1.Validators.compose([forms_1.Validators.required])]
                    });
                    this.usernameForm = fb.group({
                        'username': [null, forms_1.Validators.compose([forms_1.Validators.required])]
                    });
                    this.usernameCodeForm = fb.group({
                        'username': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'code': [null, forms_1.Validators.compose([forms_1.Validators.required])]
                    });
                    this.userInitiatedWithOutLoginPasswordResetForm = fb.group({
                        'username': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'code': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'password1': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'password2': [null, forms_1.Validators.compose([forms_1.Validators.required])]
                    }, { validator: this.areEqual });
                    this.passwordResetForm = fb.group({
                        'code': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'password1': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'password2': [null, forms_1.Validators.compose([forms_1.Validators.required])]
                    }, { validator: this.areEqual });
                    this.userChangePasswordForm = fb.group({
                        'password1': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'password2': [null, forms_1.Validators.compose([forms_1.Validators.required])]
                    }, { validator: this.areEqual });
                }
                AuthenticationComponent.prototype.ngOnInit = function () {
                    var _this = this;
                    this.isMissingBootstrap = !this.aws.isInitialized;
                    this.authstates = authentication_class_1.EnumAuthState;
                    this.loginstates = EnumLoginState;
                    this.dismissFirstTimeModal = this.dismissFirstTimeModal.bind(this);
                    this.isFirstTimeModal = this.isFirstTimeLogin = bootstrap.firstTimeUse && localStorage.getItem(this._storageKeyHasLoggedIn) === null;
                    this.user = this.aws.context.authentication.user;
                    if (this.user != null) this.validateSession(this.aws.context.authentication);
                    this._subscription = this.aws.context.authentication.change.subscribe(function (context) {
                        _this.loading = false;
                        if (!(context.state === authentication_class_1.EnumAuthState.PASSWORD_CHANGE_FAILED || context.state === authentication_class_1.EnumAuthState.LOGIN_FAILURE || context.state === authentication_class_1.EnumAuthState.LOGOUT_FAILURE || context.state === authentication_class_1.EnumAuthState.ASSUME_ROLE_FAILED || context.state === authentication_class_1.EnumAuthState.LOGIN_FAILURE_INCORRECT_USERNAME_PASSWORD || context.state === authentication_class_1.EnumAuthState.FORGOT_PASSWORD_FAILURE || context.state === authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE)) {
                            _this.change(context.state);
                        }
                        _this.output = context.output;
                        _this.error = null;
                        if (context.state === authentication_class_1.EnumAuthState.LOGIN_CONFIRMATION_NEEDED) {
                            _this.user = context.output[0];
                            _this.toastr.success("Enter the confirmation code sent by email.");
                        } else if (context.state === authentication_class_1.EnumAuthState.PASSWORD_CHANGE) {
                            _this.user = context.output[0];
                        } else if (context.state === authentication_class_1.EnumAuthState.FORGOT_PASSWORD_SUCCESS || context.state === authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_SUCCESS) {
                            _this.toastr.success("The password has been updated successfully.");
                        } else if (context.state === authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRM_CODE) {
                            _this.toastr.success("The password reset code has sent.");
                        } else if (context.state === authentication_class_1.EnumAuthState.PASSWORD_CHANGE_FAILED) {
                            _this.error = context.output[0].message;
                        } else if (context.state === authentication_class_1.EnumAuthState.LOGIN_FAILURE) {
                            _this.error = context.output[0].message;
                        } else if (context.state === authentication_class_1.EnumAuthState.FORGOT_PASSWORD_FAILURE) {
                            _this.error = context.output[0].message;
                        } else if (context.state === authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE) {
                            _this.error = context.output[0].message;
                        } else if (context.state === authentication_class_1.EnumAuthState.LOGGED_IN) {
                            localStorage.setItem(_this._storageKeyHasLoggedIn, "1");
                        } else if (context.output !== undefined && context.output.length == 1 && context.output[0].message) {
                            _this.toastr.error(context.output[0].message);
                        }
                    });
                };
                AuthenticationComponent.prototype.change = function (state) {
                    if (state == EnumLoginState.FORGOT_PASSWORD || state == EnumLoginState.RESEND_CODE) {
                        this.setUsername(this.usernameForm);
                    } else if (state == authentication_class_1.EnumAuthState.PASSWORD_CHANGE) {
                        this.setUsername(this.userChangePasswordForm);
                    } else if (state == EnumLoginState.ENTER_CODE) {
                        this.setUsername(this.userInitiatedWithOutLoginPasswordResetForm);
                    }
                    this.state = state;
                };
                AuthenticationComponent.prototype.ngOnDestroy = function () {
                    this._subscription.unsubscribe();
                };
                AuthenticationComponent.prototype.isRequiredEmpty = function (form, item) {
                    return form.controls[item].hasError('required') && form.controls[item].touched;
                };
                AuthenticationComponent.prototype.isNotValid = function (form, item) {
                    return !form.controls[item].valid && form.controls[item].touched;
                };
                AuthenticationComponent.prototype.login = function (userform) {
                    this.loading = true;
                    this.resetErrorMsg();
                    this.aws.context.authentication.login(userform.username, userform.password);
                };
                AuthenticationComponent.prototype.forgotPasswordSendCode = function (username) {
                    this.resetErrorMsg();
                    this.aws.context.authentication.forgotPassword(username);
                };
                AuthenticationComponent.prototype.confirmNewPassword = function (username, code, password) {
                    this.aws.context.authentication.forgotPasswordConfirmNewPassword(username, code, password);
                };
                AuthenticationComponent.prototype.setUsername = function (form) {
                    if (this.userLoginForm.value.username && form.controls['username']) {
                        form.controls['username'].setValue(this.userLoginForm.value.username);
                        form.controls['username'].markAsDirty();
                    }
                };
                AuthenticationComponent.prototype.confirmCode = function (code) {
                    var _this = this;
                    this.loading = true;
                    this.aws.context.userManagement.confirmCode(code, this.user.username, function (result) {
                        _this.toastr.success("Thank-you.  The code has been confirmed.");
                        _this.change(EnumLoginState.LOGIN);
                        _this.userLoginForm.value.username = _this.user.username;
                        _this.aws.context.authentication.logout();
                    }, function (err) {
                        _this.toastr.error(err.message);
                    });
                };
                AuthenticationComponent.prototype.dismissFirstTimeModal = function () {
                    this.isFirstTimeModal = false;
                    localStorage.setItem(this._storageKeyHasLoggedIn, "1");
                };
                AuthenticationComponent.prototype.dismissMissingBootstrap = function () {
                    this.isMissingBootstrap = false;
                };
                AuthenticationComponent.prototype.updatePassword = function (password) {
                    this.loading = true;
                    this.aws.context.authentication.updatePassword(this.user, password);
                };
                AuthenticationComponent.prototype.resendConfirmationCode = function (username) {
                    var _this = this;
                    this.loading = true;
                    this.aws.context.userManagement.resendConfirmationCode(username, function (result) {
                        _this.toastr.success("A new confirmation code has been sent by email.");
                    }, function (err) {
                        _this.toastr.error(err.message);
                    });
                };
                AuthenticationComponent.prototype.areEqual = function (group) {
                    var pass1 = group.controls['password1'];
                    var pass2 = group.controls['password2'];
                    pass1.setErrors(null, true);
                    if (pass1.value == pass2.value) {
                        return;
                    }
                    pass2.setErrors({ error: "invalid" }, true);
                };
                AuthenticationComponent.prototype.resetErrorMsg = function () {
                    this.error = null;
                };
                AuthenticationComponent.prototype.validateSession = function (authModule) {
                    this.user.getSession(function (err, session) {
                        if (err) {
                            return;
                        }
                        if (session.isValid()) {
                            authModule.session = session;
                            authModule.updateCredentials();
                        }
                    });
                };
                __decorate([core_1.ViewChild(index_2.ModalComponent), __metadata("design:type", index_2.ModalComponent)], AuthenticationComponent.prototype, "modalRef", void 0);
                AuthenticationComponent = __decorate([core_1.Component({
                    selector: 'login',
                    template: "<div class=\"cgp-container cgp-login-container\">     <h1> Welcome to your project {{this.aws.context.name}} Cloud Gem Portal </h1>     <h2> Adding cloud-connected features to your game using the AWS cloud </h2>      <modal *ngIf=\"isMissingBootstrap\"            [autoOpen]=\"true\"            closeButtonText=\"Got it!\"            [onClose]=\"dismissMissingBootstrap\"            [onDismiss]=\"dismissMissingBootstrap\"            title=\"Missing Cloud Gem Portal bootstrap information.\">         <div class=\"modal-body\">             <p>It seems that either the ./index.html is missing or the 'boostrap' variable in the file is empty.</p>              <h2>Development for the Cloud Gem Portal</h2>             <p>If you are developing for the Cloud Gem Portal then this is expected.  You will need to copy the project bootstrap information to the ./index.html file.</p>             <pre>&lt;LumberyardInstallPath&gt; lmbr_aws cloud-gem-framework cloud-gem-portal --show-configuration --show-url-only</pre>             <p>Copy the line of the output that looks like this</p>             <pre>{{'{'}}\"userPoolId\": XX, \"firstTimeUse\": XX, \"region\": XX, \"clientId\": XX, \"identityPoolId\": XX, \"projectConfigBucketId\": XX{{'}'}}</pre>             <p>to the</p>             <pre>&lt;LumberyardInstallPath&gt;/Gems/CloudGemFramework/v1/Website/CloudGemPortal/index.html file</pre>             <p>For more information on the topic refer to the link <a herf=\"http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-cgf-cgp-dev-gs.html\" target=\"_blank\">Cloud Gem Portal Development</a></p>              <h2>Are you just running the Cloud Gem Portal?</h2>             <p>Unfortunately this an unexpected behaviour.</p>             <p>Please head over to our <a href=\"https://gamedev.amazon.com/forums/spaces/133/cloud-canvas.html\">forums</a> and post a question regarding the issue.</p>         </div>     </modal>       <modal *ngIf=\"isFirstTimeModal\"            [autoOpen]=\"true\"            closeButtonText=\"Got it!\"            [onClose]=\"dismissFirstTimeModal\"            [onDismiss]=\"dismissFirstTimeModal\"            title=\"First time Cloud Gem Portal user\">         <div class=\"modal-body\">              <p>As this is your <b>first time</b> using the Cloud Gem Portal we will guide you through the initial login.</p>              <p>In order to log into the Cloud Gem Portal for the first time you will need to write down the administrator username and password that was generated for you in Lumberyard.</p>              <div class=\"row justify-content-center margin-top-space-sm\">                 <div class=\"col-12 text-center\">The username and password could have been displayed two different ways.</div>             </div>             <div class=\"row justify-content-center margin-top-space-sm\">                 <div class=\"col-6 text-center\">                     <p>Option 1, the <b>Lumberyard Editor</b></p>                     <a id=\"welcome-option1-img\" target=\"_blank\" href=\"https://m.media-amazon.com/images/G/01/cloudcanvas/images/first-time-login-editor.png\"><img src=\"https://m.media-amazon.com/images/G/01/cloudcanvas/images/first-time-login-editor.png\" /></a>                 </div>                 <div class=\"col-6 text-center\">                     <p>Option 2, the <b>Lumberyard CLI</b></p>                     <a id=\"welcome-option2-img\" target=\"_blank\" href=\"https://m.media-amazon.com/images/G/01/cloudcanvas/images/first-time-login-cli.png\"><img src=\"https://m.media-amazon.com/images/G/01/cloudcanvas/images/first-time-login-cli.png\" /></a>                 </div>             </div>             <div class=\"margin-top-space-lg\">                 <p>If you can't find the username and password in option 1 or option 2 then most likely the account was created previously. You can delete the administrator account using the <a href=\"https://console.aws.amazon.com/cognito/users/?region={{aws.context.region}}\">AWS console</a>.</p>                 <p>Once deleted, opening the Cloud Gem Portal <b>from Lumberyard</b> will display the account information in the Lumberyard Editor (option 1 above).</p>             </div>         </div>     </modal>     <loading-spinner size=\"lg\" *ngIf=\"loading || state == authstates.LOGGED_IN || state  == authstates.USER_CREDENTIAL_UPDATED\"></loading-spinner>      <login-container *ngIf=\"!isFirstTimeLogin && !loading && (state === undefined                 || state == loginstates.LOGIN                 || state == authstates.LOGIN_FAILURE                 || state == authstates.FORGOT_PASSWORD_SUCCESS                 || state == authstates.USER_CREDENTIAL_UPDATE_FAILED                 || state == authstates.LOGGED_OUT                 || state == authstates.FORGOT_PASSWORD_CONFIRMATION_SUCCESS)                 \"                      heading=\"Sign In to the Cloud Gem Portal\"                      submitButtonText=\"Sign In\"                      secondaryLinkText=\"Forgot password?\"                      tertiaryLinkText=\"Enter Code?\"                      [isButtonDisabled]=\"userLoginForm.controls['username'].pristine || userLoginForm.controls['password'].pristine\"                      (secondaryLinkClick)=\"error = null; change(loginstates.FORGOT_PASSWORD)\"                      (tertiaryLinkClick)=\"error = null; change(loginstates.ENTER_CODE)\"                      (formSubmitted)=\"login(userLoginForm.value)\">         <form [formGroup]=\"userLoginForm\">             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userLoginForm, 'username')}\">                 <label for=\"username\"> Username </label>                 <input class=\"form-control\" id=\"username\" type=\"text\" [formControl]=\"userLoginForm.controls['username']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userLoginForm, 'username')}\" />                 <div *ngIf=\"isRequiredEmpty(userLoginForm, 'username')\" class=\"form-control-feedback\">You must include a username.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger':isNotValid(userLoginForm, 'password')}\">                 <label for=\"password\"> Password </label>                 <input class=\"form-control\" id=\"password\" type=\"password\" [formControl]=\"userLoginForm.controls['password']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userLoginForm, 'password')}\" />                 <div *ngIf=\"isRequiredEmpty(userLoginForm, 'password')\" class=\"form-control-feedback\">You must include a password.</div>             </div>         </form>         <div class='has-danger'><p class=\"form-control-feedback\">{{error}}</p></div>     </login-container>      <login-container *ngIf=\"isFirstTimeLogin && (state === undefined                 || state == loginstates.LOGIN                 || state == authstates.LOGIN_FAILURE                 || state == authstates.FORGOT_PASSWORD_SUCCESS                 || state == authstates.USER_CREDENTIAL_UPDATE_FAILED                 || state == authstates.LOGGED_OUT                 || state == authstates.FORGOT_PASSWORD_CONFIRMATION_SUCCESS)                 \"                      heading=\"Sign into the Cloud Gem Portal for the first time\"                      submitButtonText=\"Sign In\"                      [isButtonDisabled]=\"userLoginForm.controls['username'].pristine || userLoginForm.controls['password'].pristine\"                      (formSubmitted)=\"login(userLoginForm.value)\">         <form [formGroup]=\"userLoginForm\">             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userLoginForm, 'username')}\">                 <label for=\"username\"> Username </label>                 <input class=\"form-control\" id=\"username\" type=\"text\" [formControl]=\"userLoginForm.controls['username']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userLoginForm, 'username')}\" />                 <div *ngIf=\"isRequiredEmpty(userLoginForm, 'username')\" class=\"form-control-feedback\">You must include a username.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger':isNotValid(userLoginForm, 'password')}\">                 <label for=\"password\"> Password </label>                 <input class=\"form-control\" id=\"password\" type=\"password\" [formControl]=\"userLoginForm.controls['password']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userLoginForm, 'password')}\" />                 <div *ngIf=\"isRequiredEmpty(userLoginForm, 'password')\" class=\"form-control-feedback\">You must include a password.</div>             </div>         </form>         <div class='has-danger'><p class=\"form-control-feedback\">{{error}}</p></div>     </login-container>      <login-container *ngIf=\"state == authstates.LOGIN_CONFIRMATION_NEEDED\"                      heading=\"User confirmation code\"                      submitButtonText=\"Confirm Code\"                      secondaryLinkText=\"Resend Code?\"                      [isButtonDisabled]=\"userCodeForm.controls['code'].pristine\"                      (secondaryLinkClick)=\"user ? resendConfirmationCode(user.username) : change(loginstates.RESEND_CODE)\"                      tertiaryLinkText=\"Login?\"                      (tertiaryLinkClick)=\"error = null; change(loginstates.LOGIN)\"                      (formSubmitted)=\"confirmCode(userCodeForm.value.code)\">         <form [formGroup]=\"userCodeForm\">             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userCodeForm, 'code')}\">                 <label for=\"code\"> Code </label>                 <input class=\"form-control\" id=\"code\" type=\"text\" [formControl]=\"userCodeForm.controls['code']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userCodeForm, 'code')}\" />                 <div *ngIf=\"isRequiredEmpty(userCodeForm, 'code')\" class=\"form-control-feedback\">You must include the code provided by email.</div>             </div>         </form>         <div class='has-danger'><p class=\"form-control-feedback\">{{error}}</p></div>     </login-container>      <login-container *ngIf=\"state == authstates.FORGOT_PASSWORD_CONFIRM_CODE || state == authstates.FORGOT_PASSWORD_CONFIRMATION_FAILURE\"                      heading=\"User password reset\"                      submitButtonText=\"Reset Password\"                      [isButtonDisabled]=\"passwordResetForm.controls['code'].pristine || passwordResetForm.controls['password1'].pristine || passwordResetForm.controls['password2'].pristine || isNotValid(passwordResetForm, 'password1') || isNotValid(passwordResetForm, 'password2')\"                      secondaryLinkText=\"Login?\"                      (secondaryLinkClick)=\"error = null; change(loginstates.LOGIN)\"                      (formSubmitted)=\"confirmNewPassword(usernameForm.value.username, passwordResetForm.value.code, passwordResetForm.value.password1)\">         <form [formGroup]=\"passwordResetForm\">             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(passwordResetForm, 'code')}\">                 <label for=\"code\">                     Code <i class=\"fa fa-question-circle-o\" placement=\"right\"                             ngbTooltip=\"A code has been sent to your email {{output && output[0] && output[0].CodeDeliveryDetails ? output[0].CodeDeliveryDetails.Destination : ''}}.\"></i>                 </label>                 <input class=\"form-control\" id=\"code\" type=\"text\" [formControl]=\"passwordResetForm.controls['code']\"                        [ngClass]=\"{'form-control-danger': isNotValid(passwordResetForm, 'code')}\" />                 <div *ngIf=\"isRequiredEmpty(passwordResetForm, 'code')\" class=\"form-control-feedback\">You must include the code provided by email.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(passwordResetForm, 'password1')}\">                 <label for=\"code\">                     Password <i class=\"fa fa-question-circle-o\" placement=\"right\"                                 ngbTooltip=\"Requires a minimum of 8 characters.  Must include at least 1 number, 1 special character, 1 uppercase letter, and 1 lowercase.\"></i>                 </label>                 <input class=\"form-control\" id=\"password\" type=\"password\" [formControl]=\"passwordResetForm.controls['password1']\"                        [ngClass]=\"{'form-control-danger': isNotValid(passwordResetForm, 'password1')}\" />                 <div *ngIf=\"isRequiredEmpty(passwordResetForm, 'password1')\" class=\"form-control-feedback\">You must include a password.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(passwordResetForm, 'password2')}\">                 <label for=\"code\"> Re-type password </label>                 <input class=\"form-control\" id=\"password-verify\" type=\"password\" [formControl]=\"passwordResetForm.controls['password2']\"                        [ngClass]=\"{'form-control-danger': isNotValid(passwordResetForm, 'password2')}\" />                 <div *ngIf=\"isRequiredEmpty(passwordResetForm, 'password2')\" class=\"form-control-feedback\">You must re-type the password.</div>                 <div *ngIf=\"isNotValid(passwordResetForm, 'password2')\" class=\"form-control-feedback\">The passwords entered do not match.</div>             </div>         </form>         <div class='has-danger'><p class=\"form-control-feedback\">{{error}}</p></div>     </login-container>      <login-container *ngIf=\"state == authstates.PASSWORD_RESET_BY_ADMIN_CONFIRM_CODE\"                      heading=\"The administrator has reset your password\"                      submitButtonText=\"Reset Password\"                      [isButtonDisabled]=\"passwordResetForm.controls['code'].pristine || passwordResetForm.controls['password1'].pristine || passwordResetForm.controls['password2'].pristine || isNotValid(passwordResetForm, 'password1') || isNotValid(passwordResetForm, 'password2')\"                      secondaryLinkText=\"Login?\"                      (secondaryLinkClick)=\"error = null; change(loginstates.LOGIN)\"                      (formSubmitted)=\"confirmNewPassword(userLoginForm.value.username, passwordResetForm.value.code, passwordResetForm.value.password1)\">         <form [formGroup]=\"passwordResetForm\">             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(passwordResetForm, 'code')}\">                 <label for=\"code\">                     Code <i class=\"fa fa-question-circle-o\" placement=\"right\"                             ngbTooltip=\"The administrator sent you a code to your email when they reset your password.\"></i>                 </label>                 <input class=\"form-control\" id=\"code\" type=\"text\" [formControl]=\"passwordResetForm.controls['code']\"                        [ngClass]=\"{'form-control-danger': isNotValid(passwordResetForm, 'code')}\" />                 <div *ngIf=\"isRequiredEmpty(passwordResetForm, 'code')\" class=\"form-control-feedback\">You must include the code provided by email.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(passwordResetForm, 'password1')}\">                 <label for=\"code\">                     Password <i class=\"fa fa-question-circle-o\" placement=\"right\"                                 ngbTooltip=\"Requires a minimum of 8 characters.  Must include at least 1 number, 1 special character, 1 uppercase letter, and 1 lowercase.\"></i>                 </label>                 <input class=\"form-control\" id=\"password\" type=\"password\" [formControl]=\"passwordResetForm.controls['password1']\"                        [ngClass]=\"{'form-control-danger': isNotValid(passwordResetForm, 'password1')}\" />                 <div *ngIf=\"isRequiredEmpty(passwordResetForm, 'password1')\" class=\"form-control-feedback\">You must include a password.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(passwordResetForm, 'password2')}\">                 <label for=\"code\"> Re-type password </label>                 <input class=\"form-control\" id=\"password-verify\" type=\"password\" [formControl]=\"passwordResetForm.controls['password2']\"                        [ngClass]=\"{'form-control-danger': isNotValid(passwordResetForm, 'password2')}\" />                 <div *ngIf=\"isRequiredEmpty(passwordResetForm, 'password2')\" class=\"form-control-feedback\">You must re-type the password.</div>                 <div *ngIf=\"isNotValid(passwordResetForm, 'password2')\" class=\"form-control-feedback\">The passwords entered do not match.</div>             </div>         </form>         <div class='has-danger'><p class=\"form-control-feedback\">{{error}}</p></div>     </login-container>      <login-container *ngIf=\"state == loginstates.ENTER_CODE\"                      heading=\"User password reset\"                      submitButtonText=\"Change Password\"                      [isButtonDisabled]=\"userInitiatedWithOutLoginPasswordResetForm.controls['username'].pristine                      || userInitiatedWithOutLoginPasswordResetForm.controls['code'].pristine                      || userInitiatedWithOutLoginPasswordResetForm.controls['password1'].pristine                      || userInitiatedWithOutLoginPasswordResetForm.controls['password2'].pristine                      || isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'password1')                      || isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'password2')\"                      secondaryLinkText=\"Login?\"                      (secondaryLinkClick)=\"error = null; change(loginstates.LOGIN)\"                      (formSubmitted)=\"confirmNewPassword(userInitiatedWithOutLoginPasswordResetForm.value.username, userInitiatedWithOutLoginPasswordResetForm.value.code, userInitiatedWithOutLoginPasswordResetForm.value.password1)\">         <form [formGroup]=\"userInitiatedWithOutLoginPasswordResetForm\">             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'username')}\">                 <label for=\"code\"> Username </label>                 <input class=\"form-control\" id=\"name\" type=\"username\" [formControl]=\"userInitiatedWithOutLoginPasswordResetForm.controls['username']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'username')}\" />                 <div *ngIf=\"isRequiredEmpty(userInitiatedWithOutLoginPasswordResetForm, 'username')\" class=\"form-control-feedback\">You must include a username.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'code')}\">                 <label for=\"code\"> Code </label>                 <input class=\"form-control\" id=\"code\" type=\"text\" [formControl]=\"userInitiatedWithOutLoginPasswordResetForm.controls['code']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'code')}\" />                 <div *ngIf=\"isRequiredEmpty(userInitiatedWithOutLoginPasswordResetForm, 'code')\" class=\"form-control-feedback\">You must include the code provided by email.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'password1')}\">                 <label for=\"code\">                     Password <i class=\"fa fa-question-circle-o\" placement=\"right\"                                 ngbTooltip=\"Requires a minimum of 8 characters.  Must include at least 1 number, 1 special character, 1 uppercase letter, and 1 lowercase.\"></i>                 </label>                 <input class=\"form-control\" id=\"password\" type=\"password\" [formControl]=\"userInitiatedWithOutLoginPasswordResetForm.controls['password1']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'password1')}\" />                 <div *ngIf=\"isRequiredEmpty(userInitiatedWithOutLoginPasswordResetForm, 'password1')\" class=\"form-control-feedback\">You must include a password.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'password2')}\">                 <label for=\"code\"> Re-type password </label>                 <input class=\"form-control\" id=\"password-verify\" type=\"password\" [formControl]=\"userInitiatedWithOutLoginPasswordResetForm.controls['password2']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'password2')}\" />                 <div *ngIf=\"isRequiredEmpty(userInitiatedWithOutLoginPasswordResetForm, 'password2')\" class=\"form-control-feedback\">You must re-type the password.</div>                 <div *ngIf=\"isNotValid(userInitiatedWithOutLoginPasswordResetForm, 'password2')\" class=\"form-control-feedback\">The passwords entered do not match.</div>             </div>         </form>         <div class='has-danger'><p class=\"form-control-feedback\">{{error}}</p></div>     </login-container>      <login-container *ngIf=\"state == loginstates.RESEND_CODE\"                      heading=\"Resend user confirmation code\"                      submitButtonText=\"Resend Code\"                      [isButtonDisabled]=\"usernameForm.controls['username'].pristine\"                      secondaryLinkText=\"Enter Code?\"                      (secondaryLinkClick)=\"error = null; change(loginstates.ENTER_CODE)\"                      tertiaryLinkText=\"Login?\"                      (tertiaryLinkClick)=\"error = null; change(loginstates.LOGIN)\"                      (formSubmitted)=\"resendConfirmationCode(usernameForm.value.username); change(loginstates.ENTER_CODE)\">         <form [formGroup]=\"usernameForm\">             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(usernameForm, 'username')}\">                 <label for=\"code\"> Username </label>                 <input class=\"form-control\" id=\"username\" type=\"text\" [formControl]=\"usernameForm.controls['username']\"                        [ngClass]=\"{'form-control-danger': isNotValid(usernameForm, 'username')}\" />                 <div *ngIf=\"isRequiredEmpty(usernameForm, 'username')\" class=\"form-control-feedback\">You must include a username.</div>             </div>         </form>         <div class='has-danger'><p class=\"form-control-feedback\">{{error}}</p></div>     </login-container>      <login-container *ngIf=\"state == loginstates.FORGOT_PASSWORD || state == authstates.FORGOT_PASSWORD_FAILURE\"                      heading=\"Forgotten password\"                      submitButtonText=\"Send Reset Code\"                      [isButtonDisabled]=\"usernameForm.controls['username'].pristine\"                      secondaryLinkText=\"Login?\"                      (secondaryLinkClick)=\"error = null; change(loginstates.LOGIN)\"                      (formSubmitted)=\"change(authstates.FORGOT_PASSWORD_CONFIRM_CODE); forgotPasswordSendCode(usernameForm.value.username)\">         <form [formGroup]=\"usernameForm\">             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(usernameForm, 'username')}\">                 <label for=\"code\"> Username </label>                 <input class=\"form-control\" id=\"username\" type=\"text\" [formControl]=\"usernameForm.controls['username']\"                        [ngClass]=\"{'form-control-danger': isNotValid(usernameForm, 'username')}\" />                 <div *ngIf=\"isRequiredEmpty(usernameForm, 'username')\" class=\"form-control-feedback\">You must include a username.</div>             </div>         </form>         <div class='has-danger'><p class=\"form-control-feedback\">{{error}}</p></div>     </login-container>      <login-container *ngIf=\"state == authstates.PASSWORD_CHANGE || state == authstates.PASSWORD_CHANGE_FAILED\"                      heading=\"Change password\"                      submitButtonText=\"Save Password\"                      [isButtonDisabled]=\"userChangePasswordForm.controls['password1'].pristine                      || userChangePasswordForm.controls['password2'].pristine                      || userChangePasswordForm.controls['password1'].value !== userChangePasswordForm.controls['password2'].value                      || isNotValid(userChangePasswordForm, 'password1')                      || isNotValid(userChangePasswordForm, 'password2')\"                      secondaryLinkText=\"Login?\"                      (secondaryLinkClick)=\"error = null; change(loginstates.LOGIN)\"                      (formSubmitted)=\"change(loginstates.LOGIN); updatePassword(userChangePasswordForm.value.password1)\">         <form [formGroup]=\"userChangePasswordForm\" >             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userChangePasswordForm, 'password1')}\">                 <label for=\"code\">                     Password <i class=\"fa fa-question-circle-o\" placement=\"right\"                                 ngbTooltip=\"Requires a minimum of 8 characters.  Must include at least 1 number, 1 special character, 1 uppercase letter, and 1 lowercase.\"></i>                 </label>                 <input class=\"form-control\" id=\"password\" type=\"password\" [formControl]=\"userChangePasswordForm.controls['password1']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userChangePasswordForm, 'password1')}\" />                 <div *ngIf=\"isRequiredEmpty(userChangePasswordForm, 'password1')\" class=\"form-control-feedback\">You must include a password.</div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userChangePasswordForm, 'password2')}\">                 <label for=\"code\"> Re-type password </label>                 <input class=\"form-control\" id=\"password-verify\" type=\"password\" [formControl]=\"userChangePasswordForm.controls['password2']\"                        [ngClass]=\"{'form-control-danger': isNotValid(userChangePasswordForm, 'password2')}\" />                 <div *ngIf=\"isRequiredEmpty(userChangePasswordForm, 'password2')\" class=\"form-control-feedback\">You must re-type the password.</div>                 <div *ngIf=\"isNotValid(userChangePasswordForm, 'password2')\" class=\"form-control-feedback\">The passwords entered do not match.</div>             </div>         </form>         <div class='has-danger'><p class=\"form-control-feedback\">{{error}}</p></div>     </login-container>      <pre>{{authstates[state] | devonly}}{{loginstates[state] | devonly}}</pre>     <pre>{{'FirstTimeModal: '+isFirstTimeModal | json | devonly}}</pre>     <pre>{{'FirstTimeLogin: '+isFirstTimeLogin | json | devonly}}</pre> </div> <cgp-footer></cgp-footer> <router-outlet></router-outlet>",
                    styles: ["\n        h1 {\n            padding-top: 100px;\n        }\n       .margin-top-space-lg{\n            margin-top: 75px;\n        }\n        .margin-top-space-sm{\n            margin-top: 25px;\n        }\n    "]
                }), __metadata("design:paramtypes", [forms_1.FormBuilder, aws_service_1.AwsService, index_1.UrlService, ng2_toastr_1.ToastsManager, core_1.ViewContainerRef])], AuthenticationComponent);
                return AuthenticationComponent;
            }();
            exports_1("AuthenticationComponent", AuthenticationComponent);
        }
    };
});
System.register("dist/app/view/authentication/component/login.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, LoginContainerComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            LoginContainerComponent = /** @class */function () {
                function LoginContainerComponent() {
                    this.heading = "";
                    this.submitButtonText = "Save";
                    this.secondaryLinkText = "";
                    this.tertiaryLinkText = "";
                    this.isButtonDisabled = false;
                    this.formSubmitted = new core_1.EventEmitter();
                    this.secondaryLinkClick = new core_1.EventEmitter();
                    this.tertiaryLinkClick = new core_1.EventEmitter();
                }
                LoginContainerComponent.prototype.ngOnInit = function () {};
                LoginContainerComponent.prototype.submit = function () {
                    if (!this.isButtonDisabled) {
                        this.formSubmitted.emit();
                    }
                };
                LoginContainerComponent.prototype.secondaryLink = function () {
                    this.secondaryLinkClick.emit();
                };
                LoginContainerComponent.prototype.tertiaryLink = function () {
                    this.tertiaryLinkClick.emit();
                };
                __decorate([core_1.Input(), __metadata("design:type", String)], LoginContainerComponent.prototype, "heading", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], LoginContainerComponent.prototype, "submitButtonText", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], LoginContainerComponent.prototype, "secondaryLinkText", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], LoginContainerComponent.prototype, "tertiaryLinkText", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], LoginContainerComponent.prototype, "isButtonDisabled", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], LoginContainerComponent.prototype, "formSubmitted", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], LoginContainerComponent.prototype, "secondaryLinkClick", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], LoginContainerComponent.prototype, "tertiaryLinkClick", void 0);
                LoginContainerComponent = __decorate([core_1.Component({
                    selector: 'login-container',
                    template: "\n        <div (keydown.enter)=\"submit()\" class=\"login-container\">\n            <h2> {{ heading }} </h2>\n            <ng-content></ng-content>\n            <div class=\"login-submit-container\">\n                <button type=\"submit\" (click)=\"submit()\" class=\"btn l-primary\" [disabled]=\"isButtonDisabled\">\n                    {{ submitButtonText }}\n                </button>\n            </div>\n            <div class=\"row justify-content-center text-center align-items-center\">\n                    <div [ngClass]=\"tertiaryLinkText ? 'col-5' : 'col-12' \">\n                        <p><a (click)=\"secondaryLink()\"> {{ secondaryLinkText }} </a></p>\n                    </div>\n                    <div *ngIf=\"tertiaryLinkText\" class=\"col-5\">\n                        <p><a (click)=\"tertiaryLink()\"> {{ tertiaryLinkText }} </a></p>\n                    </div>\n            </div>\n        </div>\n    ",
                    styles: [".login-container{margin:0 auto;margin-top:75px;padding:20px 20px 5px 20px;width:400px;min-height:250px;background-color:#f8f8f8;border:1px solid #999}.login-container h2{text-align:center;margin-bottom:25px}.login-container .login-submit-container{text-align:center}.login-container .login-submit-container button[type=submit]{margin:0 auto;margin-top:20px;padding:0 50px}.login-container p{margin-top:20px;text-align:center}.login-container p a{color:#6441A5}"]
                }), __metadata("design:paramtypes", [])], LoginContainerComponent);
                return LoginContainerComponent;
            }();
            exports_1("LoginContainerComponent", LoginContainerComponent);
        }
    };
});
System.register("dist/app/view/authentication/auth.module.js", ["@angular/core", "./auth.routes", "./component/authentication.component", "app/shared/shared.module", "app/view/authentication/component/login.component"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, auth_routes_1, authentication_component_1, shared_module_1, login_component_1, AuthModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (auth_routes_1_1) {
            auth_routes_1 = auth_routes_1_1;
        }, function (authentication_component_1_1) {
            authentication_component_1 = authentication_component_1_1;
        }, function (shared_module_1_1) {
            shared_module_1 = shared_module_1_1;
        }, function (login_component_1_1) {
            login_component_1 = login_component_1_1;
        }],
        execute: function () {
            AuthModule = /** @class */function () {
                function AuthModule() {}
                AuthModule = __decorate([core_1.NgModule({
                    imports: [auth_routes_1.AuthRoutingModule, shared_module_1.AppSharedModule],
                    declarations: [authentication_component_1.AuthenticationComponent, login_component_1.LoginContainerComponent]
                })], AuthModule);
                return AuthModule;
            }();
            exports_1("AuthModule", AuthModule);
        }
    };
});
System.register("dist/app/view/game/game.routes.js", ["@angular/core", "@angular/router", "app/shared/service/index", "./module/shared/service/index", "./component/game.component", "./module/admin/component/admin.component", "./module/admin/component/log.component", "./module/admin/component/user-admin/user-admin.component", "./module/analytic/component/analytic.component", "./module/analytic/component/heatmap/heatmap.component", "./module/analytic/component/dashboard/dashboard.component", "./module/support/component/support.component", "./module/cloudgems/component/gem-index.component", "./module/admin/component/jira/jira-credentials.component"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, router_1, index_1, index_2, game_component_1, admin_component_1, log_component_1, user_admin_component_1, analytic_component_1, heatmap_component_1, dashboard_component_1, support_component_1, gem_index_component_1, jira_credentials_component_1, GameRoutingModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (game_component_1_1) {
            game_component_1 = game_component_1_1;
        }, function (admin_component_1_1) {
            admin_component_1 = admin_component_1_1;
        }, function (log_component_1_1) {
            log_component_1 = log_component_1_1;
        }, function (user_admin_component_1_1) {
            user_admin_component_1 = user_admin_component_1_1;
        }, function (analytic_component_1_1) {
            analytic_component_1 = analytic_component_1_1;
        }, function (heatmap_component_1_1) {
            heatmap_component_1 = heatmap_component_1_1;
        }, function (dashboard_component_1_1) {
            dashboard_component_1 = dashboard_component_1_1;
        }, function (support_component_1_1) {
            support_component_1 = support_component_1_1;
        }, function (gem_index_component_1_1) {
            gem_index_component_1 = gem_index_component_1_1;
        }, function (jira_credentials_component_1_1) {
            jira_credentials_component_1 = jira_credentials_component_1_1;
        }],
        execute: function () {
            GameRoutingModule = /** @class */function () {
                function GameRoutingModule() {}
                GameRoutingModule = __decorate([core_1.NgModule({
                    imports: [router_1.RouterModule.forChild([{
                        path: 'game',
                        component: game_component_1.GameComponent,
                        canActivate: [index_1.AuthGuardService],
                        children: [{
                            path: 'support',
                            canActivate: [index_1.AuthGuardService],
                            component: support_component_1.SupportComponent
                        }, {
                            path: 'cloudgems',
                            canActivate: [index_1.AuthGuardService],
                            component: gem_index_component_1.GemIndexComponent
                        }, {
                            path: 'cloudgems/:id',
                            canActivate: [index_1.AuthGuardService],
                            component: gem_index_component_1.GemIndexComponent
                        }, {
                            path: 'analytics',
                            canActivate: [index_1.AuthGuardService, index_2.DependencyGuard],
                            component: analytic_component_1.AnalyticIndex
                        }, {
                            path: 'analytics/dashboard',
                            canActivate: [index_1.AuthGuardService, index_2.DependencyGuard],
                            component: dashboard_component_1.DashboardComponent
                        }, {
                            path: 'analytics/heatmap',
                            canActivate: [index_1.AuthGuardService, index_2.DependencyGuard],
                            component: heatmap_component_1.HeatmapComponent
                        }, {
                            path: 'administration',
                            canActivate: [index_1.AuthGuardService],
                            component: admin_component_1.AdminComponent
                        }, {
                            path: 'admin/users',
                            canActivate: [index_1.AuthGuardService],
                            component: user_admin_component_1.UserAdminComponent
                        }, {
                            path: 'admin/logs',
                            canActivate: [index_1.AuthGuardService],
                            component: log_component_1.ProjectLogComponent
                        }, {
                            path: 'admin/jira',
                            canActivate: [index_1.AuthGuardService, index_2.DependencyGuard],
                            component: jira_credentials_component_1.JiraCredentialsComponent
                        }]
                    }])],
                    exports: [router_1.RouterModule]
                })], GameRoutingModule);
                return GameRoutingModule;
            }();
            exports_1("GameRoutingModule", GameRoutingModule);
        }
    };
});
System.register("dist/app/view/game/module/cloudgems/gem.routes.js", ["@angular/core", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, router_1, GemRoutingModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            GemRoutingModule = /** @class */function () {
                function GemRoutingModule() {}
                GemRoutingModule = __decorate([core_1.NgModule({
                    imports: [router_1.RouterModule.forChild([])],
                    exports: [router_1.RouterModule]
                })], GemRoutingModule);
                return GemRoutingModule;
            }();
            exports_1("GemRoutingModule", GemRoutingModule);
        }
    };
});
System.register("dist/app/view/game/module/cloudgems/class/gem-interfaces.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var TackableStatus, TackableMeasure, AbstractCloudGemIndexComponent, AbstractCloudGemThumbnailComponent;
    return {
        setters: [],
        execute: function () {
            TackableStatus = /** @class */function () {
                function TackableStatus() {}
                return TackableStatus;
            }();
            exports_1("TackableStatus", TackableStatus);
            TackableMeasure = /** @class */function () {
                function TackableMeasure() {}
                return TackableMeasure;
            }();
            exports_1("TackableMeasure", TackableMeasure);
            AbstractCloudGemIndexComponent = /** @class */function () {
                function AbstractCloudGemIndexComponent() {}
                return AbstractCloudGemIndexComponent;
            }();
            exports_1("AbstractCloudGemIndexComponent", AbstractCloudGemIndexComponent);
            AbstractCloudGemThumbnailComponent = /** @class */function () {
                function AbstractCloudGemThumbnailComponent() {}
                return AbstractCloudGemThumbnailComponent;
            }();
            exports_1("AbstractCloudGemThumbnailComponent", AbstractCloudGemThumbnailComponent);
        }
    };
});
System.register("dist/app/view/game/module/cloudgems/class/gem-loader.class.js", ["reflect-metadata", "app/aws/schema.class"], function (exports_1, context_1) {
    "use strict";

    var __extends = this && this.__extends || function () {
        var extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
        };
        return function (d, b) {
            extendStatics(d, b);
            function __() {
                this.constructor = d;
            }
            d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
        };
    }();
    var __moduleName = context_1 && context_1.id;
    var schema_class_1, Loader, AbstractCloudGemLoader, AwsCloudGemLoader, LocalCloudGemLoader;
    return {
        setters: [function (_1) {}, function (schema_class_1_1) {
            schema_class_1 = schema_class_1_1;
        }],
        execute: function () {
            Loader = /** @class */function () {
                function Loader() {}
                return Loader;
            }();
            exports_1("Loader", Loader);
            AbstractCloudGemLoader = /** @class */function () {
                function AbstractCloudGemLoader(definition, http) {
                    this.definition = definition;
                    this.http = http;
                    this._cache = new Map();
                }
                AbstractCloudGemLoader.prototype.load = function (deployment, resourceGroup, callback) {
                    var _this = this;
                    var key = this.path(deployment.settings.name, resourceGroup.logicalResourceId);
                    if (this._cache[key]) {
                        callback(this._cache[key]);
                        return;
                    }
                    var meta = this.generateMeta(deployment, resourceGroup);
                    if (!meta.url) {
                        console.warn("Cloud gem path was not a valid URL: " + resourceGroup.stackId);
                        return;
                    }
                    //override the SystemJS normalize for cloud gem imports as the normalization process changes the pre-signed url signature
                    var normalizeFn = System.normalize;
                    if (this.definition.isProd) System.normalize = function (name, parentName) {
                        if ((name[0] != '.' || !!name[1] && name[1] != '/' && name[1] != '.') && name[0] != '/' && !name.match(System.absURLRegEx)) return normalizeFn(name, parentName);
                        return name;
                    };
                    if (!this.definition.isProd) {
                        this.http.get(meta.url).subscribe(function () {
                            return _this.importGem(meta, callback);
                        }, function (err) {
                            console.log(err);
                            callback(undefined);
                        });
                    } else {
                        this.importGem(meta, callback);
                    }
                    System.normalize = normalizeFn;
                };
                AbstractCloudGemLoader.prototype.path = function (deployment_name, gem_name) {
                    return schema_class_1.Schema.Folders.CGP_RESOURCES + "/deployment/" + deployment_name + "/resource-group/" + gem_name;
                };
                AbstractCloudGemLoader.prototype.importGem = function (meta, callback) {
                    var _this = this;
                    System.import(meta.url).then(function (ref) {
                        var annotations = Reflect.getMetadata('annotations', ref.definition())[0];
                        var gem = {
                            identifier: meta.name,
                            displayName: '',
                            srcIcon: '',
                            context: meta,
                            module: ref.definition(),
                            annotations: annotations,
                            thumbnail: annotations.bootstrap[0],
                            index: annotations.bootstrap[1]
                        };
                        if (ref.serviceApiType) {
                            _this.definition.addService(gem.identifier, gem.context, ref.serviceApiType());
                        }
                        _this._cache[meta.name] = gem;
                        callback(gem);
                    }, function (err) {
                        console.log(err);
                        callback(undefined);
                    }).catch(console.error.bind(console));
                };
                return AbstractCloudGemLoader;
            }();
            exports_1("AbstractCloudGemLoader", AbstractCloudGemLoader);
            AwsCloudGemLoader = /** @class */function (_super) {
                __extends(AwsCloudGemLoader, _super);
                function AwsCloudGemLoader(definition, aws, http) {
                    var _this = _super.call(this, definition, http) || this;
                    _this.definition = definition;
                    _this.aws = aws;
                    _this.http = http;
                    console.log("Using AWS gem loader!");
                    _this._pathloader = [];
                    return _this;
                }
                Object.defineProperty(AwsCloudGemLoader.prototype, "onPathLoad", {
                    get: function () {
                        return this._pathloader;
                    },
                    enumerable: true,
                    configurable: true
                });
                AwsCloudGemLoader.prototype.generateMeta = function (deployment, resourceGroup) {
                    var resourceGroupName = resourceGroup.logicalResourceId;
                    var baseUrl = this.baseUrl(deployment.settings.ConfigurationBucket, deployment.settings.name, resourceGroupName);
                    var key = this.path(deployment.settings.name, resourceGroupName);
                    var file_path = key + "/dist/" + resourceGroupName.toLocaleLowerCase() + '.js';
                    var signedurl = this.aws.context.s3.getSignedUrl('getObject', { Bucket: deployment.settings.ConfigurationBucket, Key: file_path, Expires: 600 });
                    return Object.assign({
                        identifier: resourceGroupName,
                        name: resourceGroupName,
                        basePath: baseUrl,
                        bucketId: deployment.settings.ConfigurationBucket,
                        key: key,
                        url: signedurl,
                        deploymentId: deployment.settings.name,
                        logicalResourceId: resourceGroup.logicalResourceId,
                        physicalResourceId: resourceGroup.physicalResourceId
                    }, resourceGroup.outputs);
                };
                //Format: <bucket physical id>/upload/afc8fa3f-ec8c-41c3-9c6f-42a173e2e1bb/deployment/dev/resource-group/DynamicContent/cgp-resource-code/dynamiccontent.js
                AwsCloudGemLoader.prototype.baseUrl = function (bucket, deployment_name, gem_name) {
                    return bucket + "/" + this.path(deployment_name, gem_name);
                };
                return AwsCloudGemLoader;
            }(AbstractCloudGemLoader);
            exports_1("AwsCloudGemLoader", AwsCloudGemLoader);
            LocalCloudGemLoader = /** @class */function (_super) {
                __extends(LocalCloudGemLoader, _super);
                function LocalCloudGemLoader(definition, http) {
                    var _this = _super.call(this, definition, http) || this;
                    _this.definition = definition;
                    _this.http = http;
                    return _this;
                }
                LocalCloudGemLoader.prototype.generateMeta = function (deployment, resourceGroup) {
                    var name = resourceGroup.logicalResourceId.toLocaleLowerCase();
                    var path = "node_modules/@cloud-gems/" + name + "/" + name + ".js";
                    return Object.assign({
                        identifier: name,
                        name: name,
                        bucketId: "",
                        key: "",
                        basePath: path,
                        url: path,
                        deploymentId: deployment.settings.name,
                        logicalResourceId: resourceGroup.logicalResourceId,
                        physicalResourceId: resourceGroup.physicalResourceId
                    }, resourceGroup.outputs);
                };
                return LocalCloudGemLoader;
            }(AbstractCloudGemLoader);
            exports_1("LocalCloudGemLoader", LocalCloudGemLoader);
        }
    };
});
System.register("dist/app/view/game/module/cloudgems/class/index.js", ["./gem-interfaces", "./gem-loader.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    function exportStar_1(m) {
        var exports = {};
        for (var n in m) {
            if (n !== "default") exports[n] = m[n];
        }
        exports_1(exports);
    }
    return {
        setters: [function (gem_interfaces_1_1) {
            exportStar_1(gem_interfaces_1_1);
        }, function (gem_loader_class_1_1) {
            exportStar_1(gem_loader_class_1_1);
        }],
        execute: function () {}
    };
});
System.register("dist/app/view/game/module/cloudgems/component/gem-index.component.js", ["@angular/core", "@angular/http", "../class/index", "app/aws/aws.service", "app/shared/service/index", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, http_1, index_1, aws_service_1, index_2, router_1, ROUTES, GemIndexComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            ROUTES = /** @class */function () {
                function ROUTES() {}
                ROUTES.CLOUD_GEMS = "cloudgems";
                return ROUTES;
            }();
            exports_1("ROUTES", ROUTES);
            GemIndexComponent = /** @class */function () {
                function GemIndexComponent(aws, zone, definition, http, lymetrics, gems, route, router, breadcrumb) {
                    var _this = this;
                    this.aws = aws;
                    this.zone = zone;
                    this.definition = definition;
                    this.http = http;
                    this.lymetrics = lymetrics;
                    this.gems = gems;
                    this.route = route;
                    this.router = router;
                    this.breadcrumb = breadcrumb;
                    this.subscriptions = Array();
                    this._isDirty = true;
                    this.forceRefresh = false;
                    this.gemsNum = 0;
                    this.isDeploymentLoading = function () {
                        if (!_this.aws.context.project.activeDeployment) return false;
                        return _this.aws.context.project.activeDeployment.isLoading;
                    };
                }
                GemIndexComponent.prototype.ngOnInit = function () {
                    var _this = this;
                    this.handleLoadedGem = this.handleLoadedGem.bind(this);
                    this._subscribedDeployments = [];
                    // Get route parameters and route to the current gem if there is one
                    this.subscriptions.push(this.route.params.subscribe(function (params) {
                        _this._gemIdRouteParam = params['id'];
                        _this.gems.currentGem = _this.gems.getGem(_this._gemIdRouteParam);
                        if (_this.gems.currentGem) {
                            _this.breadcrumb.removeAllBreadcrumbs();
                            // Add a breadcrumb for getting back to the Cloud Gems page
                            _this.breadcrumb.addBreadcrumb("Cloud Gems", function () {
                                _this.router.navigateByUrl('/game/cloudgems');
                            });
                            // Add a breadcrumb for the current gem
                            _this.breadcrumb.addBreadcrumb(_this.gems.currentGem.displayName, function () {
                                // toggle a force refresh for the gem to reload the main gem homepage
                                _this.forceRefresh = !_this.forceRefresh;
                                _this.breadcrumb.removeLastBreadcrumb();
                            });
                        }
                    }));
                    // every time the deployment is changed reload the gems for the current deployment
                    this.subscriptions.push(this.aws.context.project.activeDeploymentSubscription.subscribe(function (activeDeployment) {
                        _this.loadGems(activeDeployment);
                    }));
                    this.subscriptions.push(this.aws.context.project.settings.subscribe(function (settings) {
                        var deploymentCount = Object.keys(settings.deployment).length;
                        if (deploymentCount == 0) _this.gems.isLoading = false;
                    }));
                    // If there is already an active deployment use that and initialize the gems in the gem index
                    if (this.aws.context.project.activeDeployment) {
                        this.loadGems(this.aws.context.project.activeDeployment);
                    }
                };
                GemIndexComponent.prototype.loadGems = function (activeDeployment) {
                    var _this = this;
                    //you must initialize the component with isLoading = true so that we do not see the "No gems present" message
                    this.gems.isLoading = true;
                    this._loader = this.definition.isProd ? this._loader = new index_1.AwsCloudGemLoader(this.definition, this.aws, this.http) : new index_1.LocalCloudGemLoader(this.definition, this.http);
                    if (!activeDeployment) {
                        //this is used for removing the loading icon when no deployments exist.
                        this.gems.isLoading = false;
                        return;
                    }
                    this._subscribedDeployments.push(activeDeployment.settings.name);
                    this.subscriptions.push(activeDeployment.resourceGroup.subscribe(function (rgs) {
                        if (rgs === undefined || rgs.length == 0) {
                            if (!activeDeployment.isLoading) {
                                //this is used for removing the loading icon when no cloud gems exist in the project stack.
                                _this.gems.isLoading = false;
                            }
                            return;
                        }
                        _this.gemsNum = rgs.length;
                        for (var i = 0; i < rgs.length; i++) {
                            _this._loader.load(activeDeployment, rgs[i], _this.handleLoadedGem);
                        }
                        var names = rgs.map(function (gem) {
                            return gem.logicalResourceId;
                        });
                        _this.lymetrics.recordEvent('CloudGemsLoaded', {
                            'CloudGemsInDeployment': names.join(',')
                        }, {
                            'NumberOfCloudGemsInDeployment': names.length
                        });
                    }));
                    // Subscribe to gems being added to the gem service
                    this.subscriptions.push(this.gems.currentGemSubscription.subscribe(function (gem) {
                        _this.gems.isLoading = false;
                    }));
                };
                GemIndexComponent.prototype.handleLoadedGem = function (gem) {
                    var _this = this;
                    if (gem === undefined) {
                        return;
                    }
                    this.gems.isLoading = false;
                    this.zone.run(function () {
                        _this.gems.addGem(gem);
                        _this.navigateToSpecificPage();
                    });
                };
                GemIndexComponent.prototype.navigateToSpecificPage = function () {
                    var _this = this;
                    if (this.gems.currentGems.length === this.gemsNum) {
                        this.route.queryParams.subscribe(function (params) {
                            //Navigate to the deployment specified in the query parameters
                            if (params['deployment'] && params['deployment'] != _this.aws.context.project.activeDeployment.settings.name) {
                                for (var _i = 0, _a = _this.aws.context.project.deployments; _i < _a.length; _i++) {
                                    var deployment = _a[_i];
                                    if (deployment.settings.name === params['deployment']) {
                                        _this.gems.isLoading = true;
                                        _this.aws.context.project.activeDeployment = deployment;
                                        break;
                                    }
                                }
                            } else if (params['deployment'] && _this.aws.context.project.activeDeployment.settings.name === params['deployment']) {
                                _this.router.navigate(['game/cloudgems', params['target']], { queryParams: { params: params['params'] } });
                            }
                        });
                    }
                };
                GemIndexComponent.prototype.ngOnDestroy = function () {
                    for (var i = 0; i < this.subscriptions.length; i++) {
                        this.subscriptions[i].unsubscribe();
                    }
                };
                GemIndexComponent = __decorate([core_1.Component({
                    selector: 'game-gem-index',
                    template: "<!-- Header dynamically changes if you're looking at a gem or not--> <div class=\"row\">     <ng-container *ngIf=\"gems.currentGem\">         <div class=\"col-8 gems-heading\">             <breadcrumbs></breadcrumbs>         </div>         <div class=\"col-4\">             <i *ngIf=\"gems.currentGem\" (click)=\"forceRefresh = !forceRefresh\" class=\"fa fa-refresh fa-2x refresh-icon\"> </i>         </div>     </ng-container>     <ng-container *ngIf=\"!gems.currentGem\">         <h1 class=\"col-8 gems-heading\"> {{ (gems.currentGem) ? (gems.currentGem).displayName : 'Cloud Gems'}} </h1>     </ng-container> </div>  <ng-container *ngIf=\"aws.context.project.isInitialized\">     <!-- Show all the thumbnails of the gems -->     <div class=\"thumbnails-container gem-container\" *ngIf=\"!(gems.currentGem)\">         <ng-container [ngSwitch]=\"gems.currentGems.length === 0 && !isDeploymentLoading() && !gems.isLoading\">             <!-- Case where there are no gems in the current deployment  -->             <div *ngSwitchCase=\"true\">                 <p>There were no Cloud Gems found.   You can enable Cloud Gems throuh your Lumberyard Editor Project Configurator.  In the Lumberyard Editor, go to File -> Project Settings -> Configure Gems.</p>                 <p *ngIf=\"gems.currentGems.length === 0  && !gems.isLoading\">Enable Cloud Gems for your project through the Lumberyard Project Configurator. For more information, view the <a href=\"http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-cloud-gems-intro.html\">Cloud Gems documentation</a>.</p>             </div>             <!-- Case for when there are could gems in the current deployment -->             <div *ngSwitchCase=\"false\">                 <ng-container [ngSwitch]=\"gems.isLoading\">                     <div *ngSwitchCase=\"true\">                         <loading-spinner size=\"lg\"></loading-spinner>                     </div>                     <div *ngSwitchCase=\"false\">                         <span *ngFor=\"let gem of gems.currentGems\">                             <span [routerLink]=\"['/game/cloudgems/', gem.context.identifier]\">                                 <gem-factory [cloudGem]=\"gem\" [thumbnailOnly]=\"!(gems.currentGem)\" [refreshContent]=\"forceRefresh\"></gem-factory>                             </span>                         </span>                     </div>                 </ng-container>             </div>         </ng-container>     </div>     <!-- Show the specific gem the user clicked on -->     <span *ngIf=\"gems.currentGem\">         <gem-factory [cloudGem]=\"gems.currentGem\" [refreshContent]=\"forceRefresh\"></gem-factory>     </span> </ng-container> <router-outlet></router-outlet>",
                    styles: [".gems-heading-back-row>.col>*{color:#6441A5;font-size:11px}.gems-heading{font-size:24px;padding-left:0}.gems-back{font-size:14px;color:#6441A5}.gems-back a{cursor:pointer}input[type=\"range\"]::-webkit-slider-thumb{-webkit-appearance:none;width:10px;height:10px;background:#6441A5;border-radius:50%;transition:.1s}#map{height:450px}"]
                }), __metadata("design:paramtypes", [aws_service_1.AwsService, core_1.NgZone, index_2.DefinitionService, http_1.Http, index_2.LyMetricService, index_2.GemService, router_1.ActivatedRoute, router_1.Router, index_2.BreadcrumbService])], GemIndexComponent);
                return GemIndexComponent;
            }();
            exports_1("GemIndexComponent", GemIndexComponent);
        }
    };
});
System.register("dist/app/view/game/module/cloudgems/component/gem-thumbnail.component.js", ["@angular/core", "app/view/game/module/shared/component/thumbnail.component"], function (exports_1, context_1) {
    "use strict";

    var __extends = this && this.__extends || function () {
        var extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
        };
        return function (d, b) {
            extendStatics(d, b);
            function __() {
                this.constructor = d;
            }
            d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
        };
    }();
    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, thumbnail_component_1, GemThumbnailComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (thumbnail_component_1_1) {
            thumbnail_component_1 = thumbnail_component_1_1;
        }],
        execute: function () {
            GemThumbnailComponent = /** @class */function (_super) {
                __extends(GemThumbnailComponent, _super);
                /**
                  * Gem Thumbnail Component
                  * Contains overview of gem and defines required information for custom gems
                **/
                function GemThumbnailComponent() {
                    var _this = _super !== null && _super.apply(this, arguments) || this;
                    _this.title = "Unknown";
                    _this.cost = "Low";
                    _this.srcIcon = "None";
                    _this.metric = {
                        name: "Unknown",
                        value: "Loading..."
                    };
                    _this.state = {
                        label: "Loading",
                        styleType: "Loading"
                    };
                    return _this;
                }
                GemThumbnailComponent.prototype.ngOnInit = function () {};
                __decorate([core_1.Input(), __metadata("design:type", String)], GemThumbnailComponent.prototype, "title", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], GemThumbnailComponent.prototype, "cost", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], GemThumbnailComponent.prototype, "srcIcon", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], GemThumbnailComponent.prototype, "metric", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], GemThumbnailComponent.prototype, "state", void 0);
                GemThumbnailComponent = __decorate([core_1.Component({
                    selector: 'thumbnail-gem',
                    template: "<thumbnail [icon]=\"srcIcon\">     <div class=\"thumbnail-label-section row no-gutters\">         <div class=\"col-8\">              <div> {{title}} </div>         </div>         <div class=\"col-4 gem-status thumbnail-{{state.styleType}}\">              <div class=\"circle\"></div>             <span> {{state.label}} </span>         </div>     </div>     <div class=\"expanded-section row no-gutters\">         <div class=\"col-12 metrics-section\">             <span> {{metric.value}} </span>             <span> {{metric.name}} </span>         </div>     </div> </thumbnail>",
                    styles: [".gem-status{text-align:right;font-size:12px;position:relative}.gem-status>*{vertical-align:middle}.circle{width:8px;height:8px;border-radius:50px;background-color:#54BE5D;display:inline-block}.thumbnail-state span{font-family:AmazonEmber-Light}.thumbnail-Offline>.circle{background-color:gray}.thumbnail-Offline>span{color:gray}.thumbnail-Loading>.circle{background-color:#4e89f1}.thumbnail-Loading>.span{color:#4e89f1}.thumbnail-Warning>.circle{background-color:red}.thumbnail-Warning>.span{color:red}.thumbnail-Enabled>.circle{background-color:#54BE5D}.thumbnail-Enabled>.span{color:#54BE5D}"]
                })], GemThumbnailComponent);
                return GemThumbnailComponent;
            }(thumbnail_component_1.ThumbnailComponent);
            exports_1("GemThumbnailComponent", GemThumbnailComponent);
        }
    };
});
System.register("dist/app/view/game/module/cloudgems/directive/gem-factory.directive.js", ["reflect-metadata", "@angular/core", "app/shared/service/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    function createComponentFactory(compiler, cloudgem, thumbnailonly) {
        var component = thumbnailonly ? cloudgem.thumbnail : cloudgem.index;
        return compiler.compileModuleAndAllComponentsAsync(cloudgem.module).then(function (moduleWithComponentFactory) {
            return moduleWithComponentFactory.componentFactories.find(function (x) {
                return x.componentType === component;
            });
        });
    }
    exports_1("createComponentFactory", createComponentFactory);
    var core_1, index_1, GemFactory;
    return {
        setters: [function (_1) {}, function (core_1_1) {
            core_1 = core_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }],
        execute: function () {
            GemFactory = /** @class */function () {
                function GemFactory(vcRef, compiler, urlService) {
                    this.vcRef = vcRef;
                    this.compiler = compiler;
                    this.urlService = urlService;
                    this.thumbnailOnly = false;
                    this._loading = false;
                }
                GemFactory.prototype.ngOnInit = function () {
                    if (!this.cloudGem || this._loading) return;
                    if (this.cmpRef) {
                        this.cmpRef.destroy();
                    }
                    this._loading = true;
                    var component = Reflect.getMetadata('annotations', this.thumbnailOnly ? this.cloudGem.thumbnail : this.cloudGem.index)[0];
                    this.urlService.signUrls(component, this.cloudGem.context.bucketId, 60);
                    this.createComponent(this.cloudGem, this.thumbnailOnly);
                };
                GemFactory.prototype.ngOnChanges = function () {
                    this.ngOnInit();
                };
                GemFactory.prototype.ngOnDestroy = function () {
                    if (this.cmpRef) {
                        this.cmpRef.destroy();
                    }
                };
                GemFactory.prototype.createComponent = function (cloudgem, thumbnailonly) {
                    var _this = this;
                    createComponentFactory(this.compiler, cloudgem, thumbnailonly).then(function (factory) {
                        _this.addToView(cloudgem, factory, thumbnailonly);
                        // GemFactory.DynamicGemFactories[cloudgem.identifier] = factory;
                        _this._loading = false;
                    });
                };
                GemFactory.prototype.addToView = function (gem, factory, thumbnailonly) {
                    var injector = core_1.ReflectiveInjector.fromResolvedProviders([], this.vcRef.parentInjector);
                    //create the component in the vcRef DOM
                    this.cmpRef = this.vcRef.createComponent(factory, 0, injector, []);
                    this.cmpRef.instance.context = gem.context;
                    if (!thumbnailonly) return;
                    gem.displayName = this.cmpRef.instance.displayName;
                    gem.srcIcon = this.cmpRef.instance.srcIcon;
                };
                GemFactory.DynamicGemFactories = new Map();
                __decorate([core_1.Input(), __metadata("design:type", Object)], GemFactory.prototype, "cloudGem", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], GemFactory.prototype, "thumbnailOnly", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], GemFactory.prototype, "refreshContent", void 0);
                GemFactory = __decorate([core_1.Directive({ selector: 'gem-factory' }), __metadata("design:paramtypes", [core_1.ViewContainerRef, core_1.Compiler, index_1.UrlService])], GemFactory);
                return GemFactory;
            }();
            exports_1("GemFactory", GemFactory);
        }
    };
});
System.register("dist/app/view/game/module/cloudgems/service/cachehandler.service.js", ["ts-md5/dist/md5", "@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var md5_1, core_1, CacheHandlerService;
    return {
        setters: [function (md5_1_1) {
            md5_1 = md5_1_1;
        }, function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            CacheHandlerService = /** @class */function () {
                function CacheHandlerService() {
                    this.cacheMap = new Map();
                }
                CacheHandlerService.prototype.set = function (key, value, options) {
                    var encryptedKeyString = md5_1.Md5.hashStr(key).toString();
                    this.cacheMap.set(encryptedKeyString, value);
                };
                CacheHandlerService.prototype.setObject = function (key, value, options) {
                    this.set(JSON.stringify(key), value, options);
                };
                CacheHandlerService.prototype.get = function (key) {
                    var encryptedKeyString = md5_1.Md5.hashStr(key).toString();
                    var cacheResult = this.cacheMap.get(encryptedKeyString);
                    return cacheResult;
                };
                CacheHandlerService.prototype.getObject = function (key) {
                    return this.get(JSON.stringify(key));
                };
                CacheHandlerService.prototype.remove = function (key) {
                    var encryptedKeyString = md5_1.Md5.hashStr(key).toString();
                    this.cacheMap.delete(encryptedKeyString);
                };
                CacheHandlerService.prototype.removeAll = function () {
                    this.cacheMap = new Map();
                };
                CacheHandlerService.prototype.exists = function (key) {
                    var encryptedKeyString = md5_1.Md5.hashStr(key).toString();
                    var valueExists = this.cacheMap.has(encryptedKeyString);
                    if (valueExists) {
                        return true;
                    }
                    return false;
                };
                CacheHandlerService.prototype.objectExists = function (key) {
                    return this.exists(JSON.stringify(key));
                };
                CacheHandlerService = __decorate([core_1.Injectable()], CacheHandlerService);
                return CacheHandlerService;
            }();
            exports_1("CacheHandlerService", CacheHandlerService);
        }
    };
});
System.register("dist/app/view/game/module/cloudgems/gem.module.js", ["@angular/core", "./gem.routes", "./component/gem-index.component", "./component/gem-thumbnail.component", "./directive/gem-factory.directive", "../shared/shared.module", "./service/cachehandler.service"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, gem_routes_1, gem_index_component_1, gem_thumbnail_component_1, gem_factory_directive_1, shared_module_1, cachehandler_service_1, GemModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (gem_routes_1_1) {
            gem_routes_1 = gem_routes_1_1;
        }, function (gem_index_component_1_1) {
            gem_index_component_1 = gem_index_component_1_1;
        }, function (gem_thumbnail_component_1_1) {
            gem_thumbnail_component_1 = gem_thumbnail_component_1_1;
        }, function (gem_factory_directive_1_1) {
            gem_factory_directive_1 = gem_factory_directive_1_1;
        }, function (shared_module_1_1) {
            shared_module_1 = shared_module_1_1;
        }, function (cachehandler_service_1_1) {
            cachehandler_service_1 = cachehandler_service_1_1;
        }],
        execute: function () {
            GemModule = /** @class */function () {
                function GemModule() {}
                GemModule = __decorate([core_1.NgModule({
                    imports: [gem_routes_1.GemRoutingModule, shared_module_1.GameSharedModule],
                    exports: [gem_thumbnail_component_1.GemThumbnailComponent],
                    declarations: [gem_factory_directive_1.GemFactory, gem_index_component_1.GemIndexComponent, gem_thumbnail_component_1.GemThumbnailComponent],
                    providers: [cachehandler_service_1.CacheHandlerService]
                })], GemModule);
                return GemModule;
            }();
            exports_1("GemModule", GemModule);
        }
    };
});
System.register("dist/app/view/game/module/analytic/analytic.routes.js", ["@angular/core", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, router_1, AnalyticRoutingModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            AnalyticRoutingModule = /** @class */function () {
                function AnalyticRoutingModule() {}
                AnalyticRoutingModule = __decorate([core_1.NgModule({
                    imports: [router_1.RouterModule.forChild([])],
                    exports: [router_1.RouterModule]
                })], AnalyticRoutingModule);
                return AnalyticRoutingModule;
            }();
            exports_1("AnalyticRoutingModule", AnalyticRoutingModule);
        }
    };
});
System.register("dist/app/view/game/module/analytic/component/heatmap/heatmap.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var Heatmap;
    return {
        setters: [],
        execute: function () {
            /**
             * Heatmap class contains all the data related to a heatmap.  It also wraps a few Leaflet objects to keep things contained.
             */
            Heatmap = /** @class */function () {
                function Heatmap(heatmap) {
                    var _this = this;
                    this.imageUrl = "https://m.media-amazon.com/images/G/01/cloudcanvas/images/StarterGame._CB1513096809_.png";
                    this.height = 500;
                    this.width = 1500;
                    this.minZoomLevel = -1;
                    this.maxZoomLevel = 4;
                    this.xCoordinateMetric = "Select";
                    this.yCoordinateMetric = "Select";
                    this.event = "Select";
                    this.eventHasStartDateTime = false;
                    this.eventHasEndDateTime = false;
                    this.worldCoordinateLowerLeftX = 0;
                    this.worldCoordinateLowerLeftY = 0;
                    this.worldCoordinateLowerRightX = 0;
                    this.worldCoordinateLowerRightY = 0;
                    this.customFilter = undefined;
                    this.bounds = [[0, 0], [this.height, this.width]];
                    this.validationModel = {
                        id: { isValid: true }
                    };
                    /**
                     * Updates the heatmap to refect the current values.  If this is not called the heatmap will not show any changes to it's properties
                     * @param heatmap
                     */
                    this.update = function () {
                        _this.map.options.minZoom = _this.minZoomLevel;
                        _this.map.options.maxZoom = _this.maxZoomLevel;
                        if (_this.imageOverlay) {
                            _this.map.removeLayer(_this.imageOverlay);
                        }
                        _this.imageOverlay = L.imageOverlay(_this.imageUrl, _this.bounds).addTo(_this.map);
                        _this.imageOverlay.addEventListener('load', function (event) {
                            var imageElement = _this.imageOverlay.getElement();
                            _this.width = imageElement.naturalWidth;
                            _this.height = imageElement.naturalHeight;
                            _this.bounds = [[0, 0], [_this.height, _this.width]];
                            // this.map.fitBounds(this.bounds);
                        });
                    };
                    this.formatDateForQuery = function (datetime) {
                        var year = datetime.date.year;
                        var month = datetime.date.month < 10 ? "0" + datetime.date.month : datetime.date.month;
                        var day = datetime.date.day < 10 ? "0" + datetime.date.day : datetime.date.day;
                        var hour = datetime.time.hour < 10 ? "0" + datetime.time.hour : datetime.time.hour;
                        return "" + year + month + day + hour + "0000";
                    };
                    this.getFormattedStartDate = function () {
                        return _this.formatDateForQuery(_this.eventStartDateTime);
                    };
                    this.getFormattedEndDate = function () {
                        return _this.formatDateForQuery(_this.eventEndDateTime);
                    };
                    if (heatmap) {
                        if (heatmap.id) {
                            this.id = heatmap.id;
                        }
                        if (heatmap.fileName) {
                            this.fileName = heatmap.fileName;
                        }
                        if (heatmap.imageUrl) {
                            this.imageUrl = heatmap.imageUrl;
                        }
                        if (heatmap.minZoomLevel) {
                            this.minZoomLevel = heatmap.minZoomLevel;
                        }
                        if (heatmap.maxZoomLevel) {
                            this.maxZoomLevel = heatmap.maxZoomLevel;
                        }
                        if (heatmap.height) {
                            this.height = heatmap.height;
                        }
                        if (heatmap.width) {
                            this.width = heatmap.width;
                        }
                        if (heatmap.xCoordinateMetric) {
                            this.xCoordinateMetric = heatmap.xCoordinateMetric;
                        }
                        if (heatmap.yCoordinateMetric) {
                            this.yCoordinateMetric = heatmap.yCoordinateMetric;
                        }
                        if (heatmap.event) {
                            this.event = heatmap.event;
                        }
                        if (heatmap.customFilter) {
                            this.customFilter = heatmap.customFilter;
                        }
                        this.eventHasStartDateTime = heatmap.eventHasStartDateTime;
                        this.eventHasEndDateTime = heatmap.eventHasEndDateTime;
                        this.eventStartDateTime = heatmap.eventStartDateTime ? heatmap.eventStartDateTime : this.getDefaultEndDateTime;
                        this.eventEndDateTime = heatmap.eventEndDateTime ? heatmap.eventEndDateTime : this.getDefaultEndDateTime;
                        this.worldCoordinateLowerLeftX = heatmap.worldCoordinateLowerLeftX;
                        this.worldCoordinateLowerLeftY = heatmap.worldCoordinateLowerLeftY;
                        this.worldCoordinateLowerRightX = heatmap.worldCoordinateLowerRightX;
                        this.worldCoordinateLowerRightY = heatmap.worldCoordinateLowerRightY;
                        // Heatmap settings should all be loaded by this point so set the bounds, create the existing overlay and bind the image loaded handler
                        this.bounds = [[0, 0], [this.height, this.width]];
                        this.imageOverlay = L.imageOverlay(this.imageUrl, this.bounds);
                    } else {
                        this.eventStartDateTime = this.getDefaultEndDateTime();
                        this.eventEndDateTime = this.getDefaultEndDateTime();
                    }
                }
                Heatmap.prototype.getDefaultStartDateTime = function () {
                    var today = new Date();
                    return {
                        date: {
                            year: today.getFullYear(),
                            month: today.getMonth() + 1,
                            day: today.getDate() - 1
                        },
                        time: { hour: 12, minute: 0 },
                        valid: true
                    };
                };
                Heatmap.prototype.getDefaultEndDateTime = function () {
                    var today = new Date();
                    return {
                        date: {
                            year: today.getFullYear(),
                            month: today.getMonth() + 1,
                            day: today.getDate()
                        },
                        time: { hour: 12, minute: 0 },
                        valid: true
                    };
                };
                return Heatmap;
            }();
            exports_1("Heatmap", Heatmap);
        }
    };
});
System.register("dist/app/view/game/module/analytic/component/heatmap/create-heatmap.component.js", ["@angular/core", "@angular/http", "@angular/forms", "ng2-toastr/ng2-toastr", "app/shared/service/index", "app/aws/aws.service", "@angular/router", "app/view/game/module/shared/class/index", "app/shared/component/index", "app/view/game/module/analytic/component/heatmap/heatmap", "victor"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, http_1, forms_1, ng2_toastr_1, index_1, aws_service_1, router_1, index_2, index_3, heatmap_1, victor_1, Modes, CreateHeatmapComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (forms_1_1) {
            forms_1 = forms_1_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (index_3_1) {
            index_3 = index_3_1;
        }, function (heatmap_1_1) {
            heatmap_1 = heatmap_1_1;
        }, function (victor_1_1) {
            victor_1 = victor_1_1;
        }],
        execute: function () {
            (function (Modes) {
                Modes[Modes["SAVE"] = 0] = "SAVE";
                Modes[Modes["EDIT"] = 1] = "EDIT";
            })(Modes || (Modes = {}));
            CreateHeatmapComponent = /** @class */function () {
                function CreateHeatmapComponent(fb, breadcrumbs, router, definition, aws, http, toastr, cd) {
                    var _this = this;
                    this.fb = fb;
                    this.breadcrumbs = breadcrumbs;
                    this.router = router;
                    this.definition = definition;
                    this.aws = aws;
                    this.http = http;
                    this.toastr = toastr;
                    this.cd = cd;
                    this.isUpdatingExistingHeatmap = false;
                    this.limit = "1000";
                    this.getAttributes = function () {
                        if (_this.heatmap.event === undefined || _this.heatmap.event == "Select") return;
                        _this.isLoadingEventAttributes = true;
                        _this.coordinateDropdownOptions = [];
                        _this.metricQuery.tablename = _this.heatmap.event;
                        _this.metricsApiHandler.postQuery(_this.metricQuery.toString("describe ${database_unquoted}.${table_unquoted}")).subscribe(function (res) {
                            var attributes = res.Result;
                            if (res.Status.State == "FAILED") {
                                _this.isLoadingEventAttributes = false;
                                _this.toastr.error(res.Status.StateChangeReason);
                                return;
                            }
                            for (var _i = 0, attributes_1 = attributes; _i < attributes_1.length; _i++) {
                                var attribute = attributes_1[_i];
                                var parts = attribute[0].split("\t");
                                var metricName = parts[0].trim().toLowerCase();
                                var type = parts[1].trim();
                                if (!metricName) {
                                    break;
                                }
                                if (type == "bigint" || type == "double" || type == "float" || type == "int" || type == "tinyint" || type == "smallint" || type == "decimal") _this.coordinateDropdownOptions.push({ text: metricName });
                            }
                            _this.isLoadingEventAttributes = false;
                        }, function (err) {
                            _this.isLoadingEventAttributes = false;
                        });
                    };
                    this.getEvents = function () {
                        _this.isLoadingEvents = true;
                        _this.metricsApiHandler.getFilterEvents().subscribe(function (res) {
                            _this.isLoadingEvents = false;
                            var athenaTables = JSON.parse(res.body.text()).result;
                            var tableNames = [];
                            // Add the events as layers to the array
                            if (athenaTables) {
                                for (var i = 0; i < athenaTables.length; i++) {
                                    tableNames.push(athenaTables[i]);
                                }
                            }
                            // Initially store the layers into an array for easy sorting.
                            if (tableNames) {
                                for (var _i = 0, tableNames_1 = tableNames; _i < tableNames_1.length; _i++) {
                                    var layerName = tableNames_1[_i];
                                    _this.tableDropdownOptions.push({ text: layerName });
                                }
                            }
                        }, function (err) {
                            _this.isLoadingEvents = false;
                            console.log("An error occured when trying to load the metrics tables from Athena", err);
                            _this.toastr.error("An error occurred when trying to load the metrics tables from Athena", err);
                        });
                    };
                    this.updateXCoordinateDropdown = function (val) {
                        return _this.heatmap.xCoordinateMetric = val.text;
                    };
                    this.updateYCoordinateDropdown = function (val) {
                        return _this.heatmap.yCoordinateMetric = val.text;
                    };
                    this.updateEvent = function (val) {
                        _this.heatmap.event = val.text;
                        _this.getAttributes();
                    };
                    this.updateHeatmap = function (heatmap) {
                        _this.getMetricEventData();
                    };
                    this.getMetricEventData = function () {
                        if (_this.heatmap.event === undefined || _this.heatmap.event == "Select") return;
                        if (_this.heatmap.xCoordinateMetric === undefined || _this.heatmap.xCoordinateMetric == "Select") return;
                        if (_this.heatmap.yCoordinateMetric === undefined || _this.heatmap.yCoordinateMetric == "Select") return;
                        // Form query for getting heat coodrinates        
                        var from = "${database}.${table}";
                        var x = _this.heatmap.xCoordinateMetric;
                        var y = _this.heatmap.yCoordinateMetric;
                        var event = _this.heatmap.event;
                        var query = "SELECT distinct " + x + ", " + y + "\n            from " + from + " where 1 = 1";
                        query = _this.appendTimeFrame(query);
                        query = _this.appendCustomFilter(query);
                        query = _this.appendLimit(query);
                        _this.metricQuery.tablename = event;
                        // Submit the query
                        _this.isSaving = true;
                        _this.metricsApiHandler.postQuery(_this.metricQuery.toString(query)).subscribe(function (res) {
                            var heatmapArr = res.Result;
                            // Remove column header
                            if (heatmapArr && heatmapArr.length > 0) {
                                heatmapArr.splice(0, 1);
                                // Translate the position to the orientation and position of the screenshot
                                for (var i = 0; i < heatmapArr.length; i++) {
                                    var element = heatmapArr[i];
                                    // Initialize the two world coordinates as vectors - using victor.js
                                    var LL = new victor_1.default(_this.heatmap.worldCoordinateLowerLeftX, _this.heatmap.worldCoordinateLowerLeftY);
                                    var LR = new victor_1.default(_this.heatmap.worldCoordinateLowerRightX, _this.heatmap.worldCoordinateLowerRightY);
                                    var x_2 = element[0];
                                    var y_2 = element[1];
                                    var theta = Math.atan2(LR.y - LL.y, LR.x - LL.x);
                                    // Get imageResolution from Leaflet map bounds
                                    var imageResolutionX = _this.heatmap.width;
                                    var imageResolutionY = _this.heatmap.height;
                                    var referentialLength = LR.subtract(LL).length();
                                    var referentialHeight = referentialLength * (imageResolutionY / imageResolutionX);
                                    var x_1 = (x_2 - LL.x) * Math.cos(theta) + (y_2 - LL.y) * Math.sin(theta);
                                    var y_1 = -((x_2 - LL.x) * Math.sin(theta)) + (y_2 - LL.y) * Math.cos(theta);
                                    element[1] = x_1 / referentialLength * imageResolutionX;
                                    element[0] = y_1 / referentialHeight * imageResolutionY;
                                    heatmapArr[i] = element;
                                }
                                if (_this.heatmap.layer) {
                                    _this.heatmap.map.removeLayer(_this.heatmap.layer);
                                }
                                if (_this.heatmap.printControl) {
                                    _this.heatmap.map.removeControl(_this.heatmap.printControl);
                                }
                                try {
                                    var layer = L.heatLayer(heatmapArr, { radius: 25 });
                                    _this.heatmap.layer = layer.addTo(_this.heatmap.map);
                                    _this.heatmap.update();
                                    // add print option to map
                                    _this.heatmap.printControl = L.easyPrint({
                                        hidden: false,
                                        sizeModes: ['A4Landscape']
                                    }).addTo(_this.heatmap.map);
                                    _this.toastr.success("The heatmap visualization has been updated.");
                                } catch (err) {
                                    console.log("Unable to update heatmap layer: " + err);
                                    _this.toastr.error("The heatmap visualization failed to update.  Please refer to your browser console log. ");
                                }
                                _this.isSaving = false;
                            } else {
                                console.log(res);
                                var error = "";
                                if (res.Status && res.Status.StateChangeReason) error = res.Status.StateChangeReason;
                                _this.toastr.error("The heatmap failed to save. " + error);
                                _this.isSaving = false;
                            }
                        }, function (err) {
                            _this.toastr.error("Unable to update map with metric data.", err);
                        });
                    };
                    this.appendTimeFrame = function (query) {
                        var startdate = _this.heatmap.getFormattedStartDate();
                        var enddate = _this.heatmap.getFormattedEndDate();
                        // Start and end time
                        if (_this.heatmap.eventHasStartDateTime && _this.heatmap.eventHasEndDateTime) {
                            query += "\n            and p_server_timestamp_strftime >= '" + startdate + "'\n            and p_server_timestamp_strftime <= '" + enddate + "'            \n            ";
                        } else if (_this.heatmap.eventHasStartDateTime && !_this.heatmap.eventHasEndDateTime) {
                            query += "\n            and p_server_timestamp_strftime >= '" + startdate + "'            \n            ";
                        }
                        return query;
                    };
                    this.appendCustomFilter = function (query) {
                        if (_this.heatmap.customFilter && _this.heatmap.customFilter.trim() != "" && (_this.heatmap.customFilter.split('and').length > 0 || _this.heatmap.customFilter.split('or').length > 0)) {
                            return query + (" and " + _this.heatmap.customFilter);
                        }
                        return query;
                    };
                    this.appendLimit = function (query) {
                        return query + (" LIMIT " + _this.limit);
                    };
                    this.setLimit = function (limit) {
                        _this.limit = limit;
                    };
                    this.eventDateTimeUpdated = function (model) {
                        _this.heatmap.eventHasStartDateTime = model.hasStart;
                        _this.heatmap.eventHasEndDateTime = model.hasEnd;
                        _this.heatmap.eventStartDateTime = model.start;
                        _this.heatmap.eventEndDateTime = model.end;
                    };
                    this.saveHeatmap = function () {
                        if (_this.validate(_this.heatmap) == false) return;
                        _this.harmonize(_this.heatmap);
                        var heatmapData = {
                            id: _this.heatmap.id,
                            fileName: _this.heatmap.fileName,
                            imageUrl: _this.heatmap.imageUrl,
                            minZoomLevel: _this.heatmap.minZoomLevel,
                            maxZoomLevel: _this.heatmap.maxZoomLevel,
                            height: _this.heatmap.height,
                            width: _this.heatmap.width,
                            xCoordinateMetric: _this.heatmap.xCoordinateMetric,
                            yCoordinateMetric: _this.heatmap.yCoordinateMetric,
                            event: _this.heatmap.event,
                            eventHasStartDateTime: _this.heatmap.eventHasStartDateTime,
                            eventHasEndDateTime: _this.heatmap.eventHasEndDateTime,
                            eventStartDateTime: _this.heatmap.eventStartDateTime,
                            eventEndDateTime: _this.heatmap.eventEndDateTime,
                            worldCoordinateLowerLeftX: Math.round(_this.heatmap.worldCoordinateLowerLeftX),
                            worldCoordinateLowerLeftY: Math.round(_this.heatmap.worldCoordinateLowerLeftY),
                            worldCoordinateLowerRightX: Math.round(_this.heatmap.worldCoordinateLowerRightX),
                            worldCoordinateLowerRightY: Math.round(_this.heatmap.worldCoordinateLowerRightY)
                        };
                        if (heatmapData.event == 'Select') delete heatmapData.event;
                        if (_this.heatmap.customFilter) heatmapData['customFilter'] = _this.heatmap.customFilter;
                        _this.mode = _this.isUpdatingExistingHeatmap ? "Update" : "Save";
                        if (!_this.isUpdatingExistingHeatmap) {
                            _this.submitHeatmap(heatmapData);
                        } else {
                            _this.submitUpdatedHeatmap(heatmapData);
                        }
                    };
                    this.validate = function (heatmap) {
                        if (_this.heatmap.id === undefined || _this.heatmap.id === null || _this.heatmap.id.trim() === "") {
                            _this.toastr.error("The heatmap is missing a proper name.");
                            _this.heatmap.validationModel.id.isValid = false;
                            return false;
                        } else {
                            _this.heatmap.validationModel.id.isValid = true;
                        }
                        return true;
                    };
                    this.harmonize = function (heatmap) {
                        _this.heatmap.id = _this.heatmap.id.trim().replace(" ", "");
                    };
                    this.submitUpdatedHeatmap = function (heatmap) {
                        _this.isSaving = true;
                        _this.metricsApiHandler.updateHeatmap(_this.previousHeatmapId, heatmap).subscribe(function (response) {
                            _this.toastr.success("The heatmap '" + _this.heatmap.id + "' has been updated.");
                            _this.mode = "Edit";
                            _this.isSaving = false;
                            _this.isUpdatingExistingHeatmap = true;
                        }, function (err) {
                            _this.isSaving = false;
                        });
                        _this.getMetricEventData();
                    };
                    this.submitHeatmap = function (heatmap) {
                        _this.isSaving = true;
                        _this.metricsApiHandler.createHeatmap(heatmap).subscribe(function (response) {
                            _this.toastr.success("The heatmap '" + heatmap.id + "' has been saved.");
                            _this.mode = "List";
                            _this.isSaving = false;
                            _this.isUpdatingExistingHeatmap = true;
                        }, function (err) {
                            _this.isSaving = false;
                        });
                        _this.getMetricEventData();
                    };
                    // File picker logic below
                    this.selectMapImage = function () {
                        _this.importButtonRef.nativeElement.click();
                    };
                    this.importMapImage = function (event) {
                        var file = event.target.files[0];
                        // assemble bucket location for metrics
                        var uploadImageToS3Params = {
                            Body: file,
                            Bucket: _this.bucket,
                            Key: file.name
                        };
                        _this.aws.context.s3.putObject(uploadImageToS3Params, function (err, data) {
                            _this.toastr.success("Image uploaded successfully");
                            // this.heatmap.imageUrl = 'https://s3.amazonaws.com/' + this.heatmapS3Bucket + "/" + file.name
                            _this.heatmap.imageUrl = _this.aws.context.s3.getSignedUrl('getObject', { Bucket: _this.bucket, Key: file.name, Expires: 300 });
                            // Save the filename so we can recreate the signed URL for the next time the website loads
                            _this.heatmap.fileName = file.name;
                            _this.heatmap.update();
                        });
                    };
                }
                CreateHeatmapComponent.prototype.ngOnInit = function () {
                    // Get Metrics Data
                    this.metricQuery = new index_2.MetricQuery(this.aws);
                    var metricGemService = this.definition.getService("CloudGemMetric");
                    this.bucket = metricGemService.context.MetricBucketName + "/heatmaps";
                    // Initialize the metrics api handler
                    this.metricsApiHandler = new metricGemService.constructor(metricGemService.serviceUrl, this.http, this.aws);
                    if (!this.heatmap) {
                        this.heatmap = new heatmap_1.Heatmap();
                        this.isUpdatingExistingHeatmap = false;
                    } else {
                        this.isUpdatingExistingHeatmap = true;
                        // Keep track of previous ID to update in case it changes
                        this.previousHeatmapId = this.heatmap.id;
                        // Initialize the heatmap object with the existing heatmap values
                        if (this.heatmap.fileName) this.heatmap.imageUrl = this.aws.context.s3.getSignedUrl('getObject', { Bucket: this.bucket, Key: this.heatmap.fileName, Expires: 300 });
                        this.heatmap = new heatmap_1.Heatmap(this.heatmap);
                    }
                    this.coordinateDropdownOptions = new Array();
                    this.tableDropdownOptions = new Array();
                    // create the map
                    this.heatmap.map = L.map('map', {
                        crs: L.CRS.Simple,
                        minZoom: this.heatmap.minZoomLevel,
                        maxZoom: this.heatmap.maxZoomLevel,
                        attributionControl: false
                    });
                    // If the overlay exists already it should now be ready to add to the map
                    if (this.heatmap.imageOverlay) {
                        this.heatmap.imageOverlay.addTo(this.heatmap.map);
                    }
                    // Trigger an update here in case the heatmap values are different than the defaults
                    this.heatmap.update();
                    this.heatmap.map.fitBounds(this.heatmap.bounds);
                    this.getEvents();
                    this.getAttributes();
                    this.getMetricEventData();
                };
                __decorate([core_1.Input(), __metadata("design:type", heatmap_1.Heatmap)], CreateHeatmapComponent.prototype, "heatmap", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], CreateHeatmapComponent.prototype, "mode", void 0);
                __decorate([core_1.ViewChild('import'), __metadata("design:type", core_1.ElementRef)], CreateHeatmapComponent.prototype, "importButtonRef", void 0);
                __decorate([core_1.ViewChild(index_3.ModalComponent), __metadata("design:type", index_3.ModalComponent)], CreateHeatmapComponent.prototype, "modalRef", void 0);
                CreateHeatmapComponent = __decorate([core_1.Component({
                    selector: 'create-heatmap',
                    template: "<div class=\"create-heatmap-container\">     <div class=\"row\">         <div class=\"col-12\" id=\"map\"> </div>     </div>     <div class=\"heatmap-options-container\" *ngIf=\"!isSaving; else loadingHeatmap\">         <form>             <h2> Heatmap </h2>             <div class=\"row no-gutters\" [ngClass]=\"{'has-danger': !heatmap.validationModel.id.isValid}\">                 <label class=\"col-2\"> Name </label>                 <div class=\"col-6\">                     <input *ngIf=\"isUpdatingExistingHeatmap == false\" class=\"form-control\" type=\"text\" [(ngModel)]=\"heatmap.id\" name=\"id\" />                     <p *ngIf=\"isUpdatingExistingHeatmap\" > {{heatmap.id}} </p>                 </div>                 <div *ngIf=\"!heatmap.validationModel.id.isValid\" class=\"form-control-danger\">                     <div *ngIf=\"!heatmap.validationModel.id.isValid\">                         Name is required and must not have any spaces.                     </div>                 </div>             </div>             <h2> Visualization </h2>             <div class=\"row no-gutters\">                 <label class=\"col-2\"> Level Screenshot </label>                 <div class=\"col-6\">                     <input class=\"form-control\" type=\"text\" [(ngModel)]=\"heatmap.fileName\" name=\"fileName\" />                 </div>                 <div class=\"col-2\">                     <button type=\"button\" (click)=\"selectMapImage()\" class=\"btn btn-outline-primary form-control\"> Upload Image </button>                     <input #import type=\"file\" accept=\".png, .jpg, .jpeg\" (change)=\"importMapImage($event)\" class=\"btn btn-outline-primary upload-image\" />                 </div>             </div>             <div class=\"row valign-center no-gutters\">                 <label class=\"col-2\"> Max Zoom Scale </label>                 <div class=\"col-2 zoom-level\">                     <div>                         <label> Min Zoom Level </label>                     </div>                     <div class=\"row\">                         <input class=\"form-control min-max-input\" type=\"number\" [(ngModel)]=\"heatmap.minZoomLevel\" name=\"minZoomLevel\" />                         <!-- Commenting out the 'Levels' until we decide on proper messaging for zoom levels.                         I don't think 'Out' and 'In' zoom levels from the UX mock makes sense -->                         <!-- <span> Levels </span> -->                     </div>                 </div>                 <div class=\"col-2 zoom-level\">                     <div>                         <label> Max Zoom Level </label>                     </div>                     <div class=\"row\">                         <input class=\"form-control min-max-input\" type=\"number\" [(ngModel)]=\"heatmap.maxZoomLevel\" name=\"maxZoomLevel\" />                         <!-- <span> Levels </span> -->                     </div>                 </div>             </div>             <div class=\"row no-gutters\">                 <label class=\"col-2\"> Game World Coordinates </label>                 <div class=\"col-4\">                     <label> Lower Left </label><tooltip placement=\"right\" tooltip=\"Orientate the camera in Lumberyard so that it is orthogonal to the top of the level.   Place the mouse cursor at the lower left of the Lumberyard view port.  Copy the x,y mouse coordinates seen in the Lumberyard status bar to here.\"> </tooltip>                     <div class=\"row  zoom-level\">                         <div class=\"col-6\">                             <label>X:</label><input class=\"form-control min-max-input\" type=\"number\" [(ngModel)]=\"heatmap.worldCoordinateLowerLeftX\" name=\"worldCoordinateLowerLeftX\" />                         </div>                         <div class=\"col-6\">                             <label>Y:</label><input class=\"form-control min-max-input\" type=\"number\" [(ngModel)]=\"heatmap.worldCoordinateLowerLeftY\" name=\"worldCoordinateLowerLeftY\" />                         </div>                     </div>                 </div>                 <div class=\"col-4\">                     <label> Lower Right </label><tooltip placement=\"right\" tooltip=\"Orientate the camera in Lumberyard so that it is orthogonal to the top of the level.   Place the mouse cursor at the lower right of the Lumberyard view port.  Copy the x,y mouse coordinates seen in the Lumberyard status bar to here.\"> </tooltip>                     <div class=\"row  zoom-level\">                         <div class=\"col-6\">                             <label>X:</label><input class=\"form-control min-max-input\" type=\"number\" [(ngModel)]=\"heatmap.worldCoordinateLowerRightX\" name=\"worldCoordinateLowerRightX\" />                         </div>                         <div class=\"col-6\">                             <label>Y:</label><input class=\"form-control min-max-input\" type=\"number\" [(ngModel)]=\"heatmap.worldCoordinateLowerRightY\" name=\"worldCoordinateLowerRightY\" />                         </div>                     </div>                 </div>              </div>             <ng-container *ngIf=\"!isLoadingEvents; else loadingInline\">                 <h2> Data </h2>                 <div class=\"row no-gutters\">                     <div class=\"col-3\">                         <label> Event </label>                         <div>                             <dropdown placeholderText=\"{{heatmap.event}}\" [currentOption]=\"heatmap.event\" (dropdownChanged)=\"updateEvent($event)\"                                       [options]=\"tableDropdownOptions\"></dropdown>                         </div>                     </div>                     <ng-container *ngIf=\"heatmap.event && heatmap.event != 'Select'\">                         <ng-container *ngIf=\"!isLoadingEventAttributes; else loadingInline\">                             <div class=\"col-3\">                                 <label> X Axis </label>                                 <div>                                     <dropdown placeholderText=\"{{heatmap.xCoordinateMetric}}\" [currentOption]=\"heatmap.xCoordinateMetric\" (dropdownChanged)=\"updateXCoordinateDropdown($event)\"                                               [options]=\"coordinateDropdownOptions\"></dropdown>                                 </div>                             </div>                             <div class=\"col-3\">                                 <label> Y Axis </label>                                 <div>                                     <dropdown placeholderText=\"{{heatmap.yCoordinateMetric}}\" [currentOption]=\"heatmap.yCoordinateMetric\" (dropdownChanged)=\"updateYCoordinateDropdown($event)\"                                               [options]=\"coordinateDropdownOptions\"></dropdown>                                 </div>                             </div>                         </ng-container>                     </ng-container>                 </div>                 <ng-container *ngIf=\"(heatmap.xCoordinateMetric && heatmap.xCoordinateMetric != 'Select') && (heatmap.yCoordinateMetric && heatmap.yCoordinateMetric != 'Select')\">                     <div class=\"row no-gutters\">                         <div class=\"col-6\">                             <label> Date/Time range (UTC) </label>                             <datetime-range-picker (dateTimeRange)=\"eventDateTimeUpdated($event)\"                                                    [startDateModel]=\"heatmap.eventStartDateTime.date\"                                                    [endDateModel]=\"heatmap.eventEndDateTime.date\"                                                    [startTime]=\"heatmap.eventStartDateTime.time\"                                                    [endTime]=\"heatmap.eventEndDateTime.time\"                                                    [hasStart]=\"heatmap.eventHasStartDateTime\"                                                    [hasEnd]=\"heatmap.eventHasEndDateTime\"></datetime-range-picker>                          </div>                     </div>                     <div class=\"row\">                         <!-- Query input -->                         <div class=\"col-lg-11\">                             <label> Custom Filters </label>                                                         <input type=\"text\" class=\"form-control\" [(ngModel)]=\"heatmap.customFilter\" [ngModelOptions]=\"{standalone: true}\" placeholder=\"Examples: platform_identifier in ('Android', 'PC', 'iOS') or p_client_build_identifier = '1.0.1234'\" />                         </div>                         <div class=\"col-lg-1\">                             <label> Limit </label>                                                         <dropdown class=\"search-dropdown\" (dropdownChanged)=\"setLimit($event)\" [currentOption]=\"{text: limit}\" [options]=\"[{text: '100'}, {text: '500'}, {text: '1000'}, {text: 'All'}]\" placeholderText=\"Return row limit.\"> </dropdown>                         </div>                    </div>                         </ng-container>                 <div class=\"row margin-top-10 \">                     <button class=\"btn btn-primary l-primary heatmap-button\" (click)=\"saveHeatmap()\" type=\"button\"> Save </button>                 </div>             </ng-container>         </form>     </div> </div>  <ng-template #loadingInline>     <div><i class=\"fa fa-cog fa-spin fa-2x fa-fw\"></i></div> </ng-template>  <ng-template #loadingHeatmap>     <loading-spinner text=\"Refreshing Heatmap\"></loading-spinner> </ng-template>",
                    styles: ["\n        #map {\n            height: 500px;\n            margin-top:20px;\n            margin-bottom:30px;\n        }\n        .upload-image {\n            display: none;\n        }\n        .heatmap-upload .col-3 p {\n            margin-bottom: 40px;\n        }\n        .heatmap-upload {\n            margin-top: 20px;\n        }\n        .heatmap-upload > .col-3 {\n            margin-top: 15px;\n        }\n        .heatmap-options-container {\n            max-width: 1200px;\n        }\n        .heatmap-options-container > form .row, .heatmap-options-container > form {\n            margin-bottom: 15px;\n        }\n        .heatmap-upload input[type=\"file\"] {\n            display: none;\n        }\n        .add-layer {\n            line-height: 80px;\n        }\n        .heatmap-button {\n            margin-right: 5px\n        }\n        .min-max-input {\n            width: 80px\n        }\n        .margin-top-10 {\n            margin-top: 30px\n        }\n        div.zoom-level label {\n            margin-bottom: 5px;\n            font-family: \"AmazonEmber-Light\";\n        }\n        div.zoom-level .row span {\n            margin-left: 5px;\n            line-height: 31px;\n        }\n        label {\n            font-size: 11px;\n            font-family: \"AmazonEmber-Bold\";\n        }\n        label label {\n            font-family: \"AmazonEmber-Regular\";\n        }\n    "]
                }), __metadata("design:paramtypes", [forms_1.FormBuilder, index_1.BreadcrumbService, router_1.Router, index_1.DefinitionService, aws_service_1.AwsService, http_1.Http, ng2_toastr_1.ToastsManager, core_1.ChangeDetectorRef])], CreateHeatmapComponent);
                return CreateHeatmapComponent;
            }();
            exports_1("CreateHeatmapComponent", CreateHeatmapComponent);
        }
    };
});
System.register("dist/app/view/game/module/analytic/component/heatmap/list-heatmap.component.js", ["@angular/core", "@angular/http", "ng2-toastr/ng2-toastr", "app/shared/service/index", "app/aws/aws.service", "app/shared/component/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, http_1, ng2_toastr_1, index_1, aws_service_1, index_2, ListHeatmapComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }],
        execute: function () {
            ListHeatmapComponent = /** @class */function () {
                function ListHeatmapComponent(breadcrumbs, definition, aws, http, toastr) {
                    var _this = this;
                    this.breadcrumbs = breadcrumbs;
                    this.definition = definition;
                    this.aws = aws;
                    this.http = http;
                    this.toastr = toastr;
                    this.onHeatmapClick = new core_1.EventEmitter();
                    this.isLoadingHeatmaps = true;
                    this.mode = "List";
                    this.refreshList = function () {
                        // Get existing heatmaps
                        _this.isLoadingHeatmaps = true;
                        _this.heatmaps = [];
                        _this.metricsApiHandler.listHeatmaps().subscribe(function (response) {
                            var heatmapObjs = JSON.parse(response.body.text()).result[0].value;
                            // Get rid of extra array for easier use
                            heatmapObjs.map(function (heatmap) {
                                _this.heatmaps.push(heatmap.heatmap);
                            });
                            _this.isLoadingHeatmaps = false;
                        }, function (err) {
                            _this.toastr.error("Unable to retrieve existing heatmap data", err);
                        });
                    };
                    this.editHeatmap = function (heatmap) {
                        _this.onHeatmapClick.emit(heatmap);
                        _this.breadcrumbs.addBreadcrumb("Edit Heatmap", null);
                    };
                    this.dismissModal = function () {
                        _this.mode = "List";
                    };
                    this.deleteHeatmapModal = function (heatmap) {
                        _this.mode = "Delete";
                        _this.currentHeatmap = heatmap;
                    };
                    this.deleteHeatmap = function (heatmap) {
                        _this.isLoadingHeatmaps = false;
                        _this.metricsApiHandler.deleteHeatmap(heatmap.id).subscribe(function (response) {
                            _this.toastr.success("The heatmap '" + heatmap.id + "' has been deleted.");
                            _this.modalRef.close();
                            _this.refreshList();
                        }, function (err) {
                            _this.toastr.error("Unable to delete heatmap", err);
                            _this.modalRef.close();
                        });
                    };
                }
                ListHeatmapComponent.prototype.ngOnInit = function () {
                    // Get Metrics Data
                    var metricServiceUrl = this.definition.getService("CloudGemMetric");
                    this.metricsApiHandler = new metricServiceUrl.constructor(metricServiceUrl.serviceUrl, this.http, this.aws);
                    this.heatmaps = new Array();
                    this.refreshList();
                };
                __decorate([core_1.Output(), __metadata("design:type", Object)], ListHeatmapComponent.prototype, "onHeatmapClick", void 0);
                __decorate([core_1.ViewChild(index_2.ModalComponent), __metadata("design:type", index_2.ModalComponent)], ListHeatmapComponent.prototype, "modalRef", void 0);
                ListHeatmapComponent = __decorate([core_1.Component({
                    selector: 'list-heatmap',
                    template: "<h2> Existing Heatmaps </h2> <div *ngIf=\"!isLoadingHeatmaps; else loading\">     <table class=\"table table-hover\">         <thead>             <tr>                 <th> NAME </th>                 <th> IMAGE </th>                 <th> EVENT </th>             </tr>         </thead>         <tbody>             <tr *ngFor=\"let heatmap of heatmaps\" >                 <td (click)=\"editHeatmap(heatmap)\"> {{heatmap.id}} </td>                 <td (click)=\"editHeatmap(heatmap)\"> {{heatmap.fileName}} </td>                 <td (click)=\"editHeatmap(heatmap)\"> {{heatmap.event}} </td>                 <td> <div class=\"float-right\"><action-stub-items [model]=\"heatmap\" [edit]=\"editHeatmap\" [delete]=\"deleteHeatmapModal\"></action-stub-items> </div></td>             </tr>         </tbody>     </table> </div>  <modal     title=\"Delete the heatamp\"     *ngIf=\"mode == 'Delete'\"     [autoOpen]=\"true\"     [hasSubmit]=\"true\"     (modalSubmitted)=\"deleteHeatmap(currentHeatmap)\"     [onClose]=\"dismissModal\"     [onDismiss]=\"dismissModal\"     submitButtonText=\"Delete\">     <div class=\"modal-body\">         <p> Are you sure you want to delete this heatmap? </p>     </div> </modal> <ng-template #loading>     <loading-spinner></loading-spinner> </ng-template>",
                    styles: ["\n    "]
                }), __metadata("design:paramtypes", [index_1.BreadcrumbService, index_1.DefinitionService, aws_service_1.AwsService, http_1.Http, ng2_toastr_1.ToastsManager])], ListHeatmapComponent);
                return ListHeatmapComponent;
            }();
            exports_1("ListHeatmapComponent", ListHeatmapComponent);
        }
    };
});
System.register("dist/app/view/game/module/analytic/component/dashboard/dashboard-graph.component.js", ["@angular/core", "@angular/forms", "ng2-toastr/ng2-toastr", "app/view/game/module/shared/class/index", "@angular/http", "app/aws/aws.service", "app/shared/service/index", "app/shared/component/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    /**
    * metricsLineData
    * A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
    * and formats it for the ngx-charts graphing library.
    * @param series_x_label
    * @param series_y_name
    * @param dataSeriesLabel
    * @param response
    */
    function MetricLineData(series_x_label, series_y_name, dataSeriesLabel, response) {
        var data = response.Result;
        data = data.slice();
        var header = data[0];
        data.splice(0, 1);
        //At least 2 points required to create a line graph.  If this data doesn't exist generate a 0 value for
        //a single data point.
        var result = [];
        if (header.length == 2) {
            result = index_1.MapTuple(header, data);
        } else if (header.length == 3) {
            result = index_1.MapVector(header, data);
        } else if (header.length >= 4) {
            result = index_1.MapMatrix(header, data);
        }
        return result;
    }
    exports_1("MetricLineData", MetricLineData);
    var core_1, forms_1, ng2_toastr_1, index_1, http_1, aws_service_1, index_2, index_3, Modes, DashboardGraphComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (forms_1_1) {
            forms_1 = forms_1_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (index_3_1) {
            index_3 = index_3_1;
        }],
        execute: function () {
            (function (Modes) {
                Modes[Modes["READ"] = 0] = "READ";
                Modes[Modes["EDIT"] = 1] = "EDIT";
                Modes[Modes["DELETE"] = 2] = "DELETE";
            })(Modes || (Modes = {}));
            DashboardGraphComponent = /** @class */function () {
                function DashboardGraphComponent(fb, http, aws, definition, toastr) {
                    var _this = this;
                    this.fb = fb;
                    this.http = http;
                    this.aws = aws;
                    this.definition = definition;
                    this.toastr = toastr;
                    this.mode = Modes.READ;
                    this.modes = Modes;
                    this.graphModels = [];
                    this.chartTypeOptions = [{ text: "Line", parser: "lineData", ngx_chart_type: "ngx-charts-line-chart" }, { text: "Stacked Line", parser: "lineData", ngx_chart_type: "ngx-charts-area-chart-stacked" }, { text: "Bar", parser: "barData", ngx_chart_type: "ngx-charts-bar-vertical" }, { text: "Gauge", parser: "barData", ngx_chart_type: "ngx-charts-gauge" }];
                    this.load = function () {
                        _this.isLoadingGraphs = true;
                        _this.graphModels = [];
                        _this.metricGraphs = new Array();
                        _this._metricApiHandler.getDashboard(_this.facetid).subscribe(function (r) {
                            _this._metricApiHandler.getFilterEvents().subscribe(function (res) {
                                _this.isLoadingGraphs = false;
                                var distinctEvents = JSON.parse(res.body.text()).result;
                                _this._query = new index_1.MetricQuery(_this.aws, "", distinctEvents, [new index_1.AllTablesUnion(_this.aws, distinctEvents)]);
                                for (var idx in r) {
                                    var metric = r[idx];
                                    metric.index = idx;
                                    _this.metricGraphs.push(new index_1.MetricGraph(metric.title, metric.xaxislabel, metric.yaxislabel, metric.dataserieslabels, _this.sources(metric), _this.parsers(metric), metric.type, [], [], metric.colorscheme, metric));
                                    _this.graphModels.push(metric);
                                }
                            }, function (err) {
                                console.log(err);
                            });
                        }, function (err) {
                            console.log(err);
                        });
                    };
                    this.refresh = function (graph) {
                        var metric = graph.model;
                        var sources = [];
                        graph.isLoading = true;
                        for (var idy in metric.sql) {
                            var sql = metric.sql[idy];
                            sources.push(_this._metricApiHandler.postQuery(_this._query.toString(sql)));
                        }
                        _this.graphModels[graph.model.index] = graph.model;
                        _this.metricGraphs[graph.model.index] = new index_1.MetricGraph(metric.title, metric.xaxislabel, metric.yaxislabel, metric.dataserieslabels, _this.sources(metric), _this.parsers(metric), metric.type, [], [], metric.colorscheme, metric);
                    };
                    this.parsers = function (metric) {
                        var parsers = [];
                        var barData = index_1.MetricBarData;
                        var lineData = MetricLineData;
                        var choroplethData = index_1.ChoroplethData;
                        for (var idy in metric.parsers) {
                            var parser = eval(metric.parsers[idy]);
                            parsers.push(parser);
                        }
                        return parsers;
                    };
                    this.sources = function (metric) {
                        var sources = [];
                        for (var idy in metric.sql) {
                            var sql = metric.sql[idy];
                            sources.push(_this._metricApiHandler.postQuery(_this._query.toString(sql)));
                        }
                        return sources;
                    };
                    this.findChartType = function (type) {
                        for (var idx in _this.chartTypeOptions) {
                            var chart = _this.chartTypeOptions[idx];
                            if (type == chart.ngx_chart_type || type == chart.text) {
                                return { text: chart.text };
                            }
                        }
                    };
                    this.edit = function (graph) {
                        _this.currentGraph = graph;
                        _this.modalTitle = "Edit SQL";
                        _this.modalSubmitTitle = "Submit";
                        _this.setFormGroup();
                        _this.mode = Modes.EDIT;
                    };
                    this.add = function () {
                        _this.currentGraph = new index_1.MetricGraph("", "", "", [], [], []);
                        _this.modalTitle = "Add new graph";
                        _this.modalSubmitTitle = "Save";
                        _this.currentGraph.model = {
                            "dataserieslabels": ["label"],
                            "parsers": [],
                            "sql": ["SELECT platform_identifier, count(distinct universal_unique_identifier) FROM ${database}.${table_sessionstart} GROUP BY 1 ORDER BY 1"],
                            "title": "",
                            "type": "",
                            "xaxislabel": "",
                            "yaxislabel": "",
                            "id": AMA.Util.GUID()
                        };
                        _this.setFormGroup();
                        _this.mode = Modes.EDIT;
                    };
                    this.confirmDeletion = function (graph) {
                        _this.currentGraph = graph;
                        _this.mode = Modes.DELETE;
                    };
                    this.delete = function (graph) {
                        var index = graph.model.index;
                        var name = graph.title;
                        _this.graphModels.splice(index, 1);
                        _this.metricGraphs.splice(index, 1);
                        _this._metricApiHandler.postDashboard(_this.facetid, _this.graphModels).subscribe(function (r) {
                            _this.toastr.success("The graph '" + name + "' has been deleted.");
                        }, function (err) {
                            _this.toastr.success("The graph '" + name + "' failed to delete. " + err);
                            console.log(err);
                        });
                        _this.dismissModal();
                    };
                    this.control = function (id) {
                        return _this.graphForm.get(id);
                    };
                    this.setFormGroup = function () {
                        _this.graphForm = _this.fb.group({
                            title: [_this.currentGraph.model.title, forms_1.Validators.required],
                            type: [_this.currentGraph.model.type, forms_1.Validators.required],
                            xaxislabel: [_this.currentGraph.model.xaxislabel, forms_1.Validators.required],
                            yaxislabel: [_this.currentGraph.model.yaxislabel, forms_1.Validators.required],
                            sql: [_this.currentGraph.model.sql[0], forms_1.Validators.required]
                        });
                    };
                    this.updateCurrentGraph = function (val) {
                        _this.currentGraph.model.type = val.text;
                    };
                    this.valid = function (graph) {
                        for (var idx in _this.graphForm.controls) {
                            if (idx == 'type') continue;
                            var control = _this.graphForm.get(idx);
                            if (!control.valid) return false;
                        }
                        var valid = false;
                        for (var idx in _this.chartTypeOptions) {
                            var chart = _this.chartTypeOptions[idx];
                            if (graph.model.type == chart.text || graph.model.type == chart.ngx_chart_type) {
                                valid = true;
                            }
                        }
                        return valid;
                    };
                    this.submit = function (graph) {
                        _this.submitted = true;
                        if (!_this.valid(graph)) return;
                        var name = graph.title ? graph.title : graph.model.title;
                        var found = false;
                        for (var idx in _this.graphModels) {
                            var graph_entry = _this.graphModels[idx];
                            if (graph.model.id == graph_entry.id) {
                                found = true;
                                break;
                            }
                        }
                        for (var idx in _this.chartTypeOptions) {
                            var chart = _this.chartTypeOptions[idx];
                            if (graph.model.type == chart.text || graph.model.type == chart.ngx_chart_type) {
                                graph.model.parsers = [];
                                graph.model.parsers.push(chart.parser);
                                graph.model.type = chart.ngx_chart_type;
                            }
                        }
                        if (!found) {
                            _this.graphModels.push(graph.model);
                        } else {
                            graph.isLoading = true;
                        }
                        _this._metricApiHandler.postDashboard(_this.facetid, _this.graphModels).subscribe(function (r) {
                            _this.toastr.success("The graph '" + name + "' has been update successfully.");
                            if (found) _this.refresh(graph);else _this.load();
                        }, function (err) {
                            graph.isLoading = false;
                            _this.toastr.success("The graph '" + name + "' failed to update. " + err);
                            console.log(err);
                        });
                        _this.submitted = false;
                        _this.dismissModal();
                    };
                    this.closeModal = function () {
                        _this.mode = Modes.READ;
                        _this.currentGraph = undefined;
                    };
                    this.dismissModal = function () {
                        _this.modalRef.close();
                    };
                    this.getAthenaQueryUrl = function () {
                        var query_id = _this.currentGraph.response ? _this.currentGraph.response.QueryExecutionId : "";
                        return "https://console.aws.amazon.com/athena/home?force&region=" + _this.aws.context.region + "#query/history/" + query_id;
                    };
                    this.metricGraphs = new Array();
                }
                DashboardGraphComponent.prototype.ngOnInit = function () {
                    var metric_service = this.definition.getService("CloudGemMetric");
                    if (metric_service) {
                        this._metricApiHandler = new metric_service.constructor(metric_service.serviceUrl, this.http, this.aws);
                        this.facetid = this.facetid.toLocaleLowerCase();
                        this.quicksighturl = "https://" + this.aws.context.region + ".quicksight.aws.amazon.com/sn/start?#";
                        this.load();
                    }
                };
                DashboardGraphComponent.prototype.ngOnDestroy = function () {
                    // When navigating to another gem remove all the interval timers to stop making requests.
                    this.metricGraphs.forEach(function (graph) {
                        graph.clearInterval();
                    });
                };
                __decorate([core_1.Input('facetid'), __metadata("design:type", String)], DashboardGraphComponent.prototype, "facetid", void 0);
                __decorate([core_1.ViewChild(index_3.ModalComponent), __metadata("design:type", index_3.ModalComponent)], DashboardGraphComponent.prototype, "modalRef", void 0);
                DashboardGraphComponent = __decorate([core_1.Component({
                    selector: 'dashboard-graph',
                    template: "\n        <div>Charts will become available as data becomes present.  Charts will refresh automatically every 5 minutes. </div>\n        <p>Deep dive into your data with <a [href]=\"quicksighturl\" target=\"_quicksight\">AWS QuickSight</a></p>\n        <div *ngIf=\"!isLoadingGraphs; else loading\"></div>\n        <ng-container *ngFor=\"let chart of metricGraphs\">\n            <graph [ref]=\"chart\" [edit]=edit [delete]=confirmDeletion></graph>\n        </ng-container>\n        <ng-container *ngIf=\"!isLoadingGraphs\">\n            <graph [add]=add></graph>\n        </ng-container>\n        <modal *ngIf=\"mode == modes.EDIT && currentGraph \"\n            [title]=\"modalTitle\"\n            [autoOpen]=\"true\"\n            [hasSubmit]=\"true\"\n            [onDismiss]=\"dismissModal\"\n            [onClose]=\"closeModal\"\n            [submitButtonText]=\"modalSubmitTitle\"\n            (modalSubmitted)=\"submit(currentGraph)\">\n            <div class=\"modal-body\">\n                <form [formGroup]=\"graphForm\">\n                    <div class=\"form-group row\" [ngClass]=\"{'has-danger': control('title').invalid && submitted}\">\n                        <label class=\"col-lg-3 col-form-label affix\" for=\"title\"> Title </label>\n                        <div class=\"col-lg-8\"><input class=\"form-control\" type=\"string\" formControlName=\"title\" id=\"title\" [(ngModel)]=\"currentGraph.model.title\"></div>\n                        <div *ngIf=\"control('title').invalid && submitted\" class=\"form-control-danger\">\n                          <div *ngIf=\"control('title').errors.required\">\n                            Title is required.\n                          </div>\n                        </div>\n                    </div>\n                    <div class=\"form-group row\" [ngClass]=\"{'has-danger': control('type').invalid && submitted}\">\n                        <label class=\"col-lg-3 col-form-label affix\" for=\"type\"> Visualization Type </label>\n                        <div class=\"col-lg-8\">\n                            <dropdown class=\"search-dropdown\" (dropdownChanged)=\"updateCurrentGraph($event)\" [currentOption]=\"findChartType(currentGraph.model.type)\" [options]=\"chartTypeOptions\" placeholderText=\"Select type\"> </dropdown>\n                        </div>\n                        <div *ngIf=\"control('type').invalid && submitted\" class=\"form-control-danger\">\n                          <div *ngIf=\"control('type').errors.required\">\n                            Type is required.\n                          </div>\n                        </div>\n                    </div>\n                    <div class=\"form-group row\" [ngClass]=\"{'has-danger': control('xaxislabel').invalid && submitted}\">\n                        <label class=\"col-lg-3 col-form-label affix\" for=\"xaxislabel\"> X-Axis Label </label>\n                        <div class=\"col-lg-8\"><input class=\"form-control\" type=\"string\" formControlName=\"xaxislabel\" [(ngModel)]=\"currentGraph.model.xaxislabel\"></div>\n                        <div *ngIf=\"control('xaxislabel').invalid && submitted\" class=\"form-control-danger\">\n                          <div *ngIf=\"control('xaxislabel').errors.required\">\n                            X-Axis is required.\n                          </div>\n                        </div>\n                    </div>\n                    <div class=\"form-group row\" [ngClass]=\"{'has-danger': control('yaxislabel').invalid && submitted}\">\n                        <label class=\"col-lg-3 col-form-label affix\" for=\"yaxislabel\"> Y-Axis Label </label>\n                        <div class=\"col-lg-8\"><input class=\"form-control\" type=\"string\" formControlName=\"yaxislabel\" [(ngModel)]=\"currentGraph.model.yaxislabel\"></div>\n                        <div *ngIf=\"control('yaxislabel').invalid && submitted\" class=\"form-control-danger\">\n                          <div *ngIf=\"control('yaxislabel').errors.required\">\n                            Y-Axis is required.\n                          </div>\n                        </div>\n                    </div>\n                    <div class=\"form-group row\" [ngClass]=\"{'has-danger': control('sql').invalid && submitted}\">\n                        <label class=\"col-lg-3\" for=\"sql\"> SQL </label>\n                        <div *ngIf=\"control('sql').invalid && submitted\" class=\"form-control-danger\">\n                          <div *ngIf=\"control('sql').errors.required\">\n                            SQL is required.\n                          </div>\n                        </div>\n                        <textarea cols=104 rows=10 formControlName=\"sql\" [(ngModel)]=\"currentGraph.model.sql[0]\"></textarea>\n                        <p>Edit with <a [href]=\"getAthenaQueryUrl()\" target=\"_athena\">AWS Athena</a>. </p>\n                    </div>\n                </form>\n            </div>\n        </modal>\n        <modal *ngIf=\"mode == modes.DELETE\"\n               title=\"Delete Graph\"\n               [autoOpen]=\"true\"\n               [hasSubmit]=\"true\"\n               [onDismiss]=\"onDismissModal\"\n               [onClose]=\"closeModal\"\n               submitButtonText=\"Delete\"\n               (modalSubmitted)=\"delete(currentGraph)\">\n            <div class=\"modal-body\">\n                <p>\n                    Are you sure you want to delete this graph?\n                </p>\n            </div>\n        </modal>\n        <ng-template #loading>\n            <loading-spinner></loading-spinner>\n        </ng-template>\n\n    ",
                    styles: ["textarea{\n        width: 26em;\n        width: 645px;\n    }"]
                }), __metadata("design:paramtypes", [forms_1.FormBuilder, http_1.Http, aws_service_1.AwsService, index_2.DefinitionService, ng2_toastr_1.ToastsManager])], DashboardGraphComponent);
                return DashboardGraphComponent;
            }();
            exports_1("DashboardGraphComponent", DashboardGraphComponent);
        }
    };
});
System.register("dist/app/view/game/module/analytic/analytic.module.js", ["@angular/core", "./component/analytic.component", "./analytic.routes", "../shared/shared.module", "./component/dashboard/dashboard.component", "./component/heatmap/create-heatmap.component", "./component/heatmap/list-heatmap.component", "./component/heatmap/heatmap.component", "./component/dashboard/dashboard-graph.component"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, analytic_component_1, analytic_routes_1, shared_module_1, dashboard_component_1, create_heatmap_component_1, list_heatmap_component_1, heatmap_component_1, dashboard_graph_component_1, AnalyticModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (analytic_component_1_1) {
            analytic_component_1 = analytic_component_1_1;
        }, function (analytic_routes_1_1) {
            analytic_routes_1 = analytic_routes_1_1;
        }, function (shared_module_1_1) {
            shared_module_1 = shared_module_1_1;
        }, function (dashboard_component_1_1) {
            dashboard_component_1 = dashboard_component_1_1;
        }, function (create_heatmap_component_1_1) {
            create_heatmap_component_1 = create_heatmap_component_1_1;
        }, function (list_heatmap_component_1_1) {
            list_heatmap_component_1 = list_heatmap_component_1_1;
        }, function (heatmap_component_1_1) {
            heatmap_component_1 = heatmap_component_1_1;
        }, function (dashboard_graph_component_1_1) {
            dashboard_graph_component_1 = dashboard_graph_component_1_1;
        }],
        execute: function () {
            AnalyticModule = /** @class */function () {
                function AnalyticModule() {}
                AnalyticModule = __decorate([core_1.NgModule({
                    imports: [analytic_routes_1.AnalyticRoutingModule, shared_module_1.GameSharedModule],
                    declarations: [analytic_component_1.AnalyticIndex, heatmap_component_1.HeatmapComponent, create_heatmap_component_1.CreateHeatmapComponent, list_heatmap_component_1.ListHeatmapComponent, dashboard_component_1.DashboardComponent, dashboard_graph_component_1.DashboardGraphComponent]
                })], AnalyticModule);
                return AnalyticModule;
            }();
            exports_1("AnalyticModule", AnalyticModule);
        }
    };
});
System.register("dist/app/view/game/module/support/support.routes.js", ["@angular/core", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, router_1, SupportRoutingModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            SupportRoutingModule = /** @class */function () {
                function SupportRoutingModule() {}
                SupportRoutingModule = __decorate([core_1.NgModule({
                    imports: [router_1.RouterModule.forChild([])],
                    exports: [router_1.RouterModule]
                })], SupportRoutingModule);
                return SupportRoutingModule;
            }();
            exports_1("SupportRoutingModule", SupportRoutingModule);
        }
    };
});
System.register("dist/app/view/game/module/support/component/support.component.js", ["@angular/core", "app/aws/aws.service", "app/shared/service/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, index_1, SupportComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }],
        execute: function () {
            SupportComponent = /** @class */function () {
                function SupportComponent(aws, api) {
                    this.projectName = aws.context.name;
                    this.window = window;
                }
                SupportComponent = __decorate([core_1.Component({
                    selector: 'game-welcome',
                    template: "<div class=\"cgp-content-container\">     <h1> Support </h1>     <div class=\"row\">         <div class=\"col-8 no-padding\">             <div>                 <h2> What are Cloud Gems? </h2>                 <p class=\"text-justify\">                     In Lumberyard, a &quot;Gem&quot; is an individual package of code, scripts and assets that provides specific functionality.  A &quot;Cloud Gem&quot; is a Gem that uses AWS resources in your account to implement cloud-connected game features, such as daily messages, leaderboards, dynamic content, and more. <a href=\"http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-cloud-gems-intro.html\" target=\"_blank\">Learn more.</a>                 </p>             </div>             <div>                 <h2> What is this Portal? </h2>                 <p class=\"text-justify\">                     This is your Cloud Gem Portal, a serverless website that runs in your AWS account.  You can use it to configure your Cloud Gems and perform maintenance operations.  If you can get to this portal, you (or someone on your team) has set up one or more Cloud Gems for your Lumberyard project.  To add more Cloud Gems, you must set them up in the Lumberyard Project Configurator tool for your Lumberyard project. <a href=\"http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-cloud-gem-portal.html\" target=\"_blank\">Learn more.</a>                 </p>                 <p class=\"text-justify\">                     Source code for the Lumberyard Cloud Gems and the Cloud Gem Portal are included with Lumberyard, making it easy to customize your Cloud Gems and Cloud Gem Portal to fit your needs. <a href=\"http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-cgf-cgp-dev-gs.html\" target=\"_blank\">Learn more.</a>                 </p>             </div>             <div>                 <h2> Release Notes </h2>                 <p class=\"text-justify\">                     You can find all of the latest updates and improvements on our <a href=\"http://docs.aws.amazon.com/lumberyard/latest/releasenotes/lumberyard-relnotes-intro.html\" target=\"_blank\">release notes</a> page.                 </p>             </div>         </div>         <div class=\"support-containers\">             <div class=\"support-container\">                 <h2> Need help? </h2>                 Need help getting started or customizing your Cloud Gems?  There are multiple ways to learn more and get support for Cloud Gems and AWS:                 <button type=\"submit\" (click)=\"window.open('http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-intro.html')\" class=\"btn l-primary\">                     Browse documentation                      <i class=\"fa fa-external-link fa-lg\" aria-hidden=\"true\"></i>                 </button>             </div>             <div class=\"support-container\">                 <h2> Have an idea for a Cloud Gem? </h2>                 Head over to our Amazon GameDev Forums and send us your suggestions for what you think would be a great Cloud Gem to support your game!                 <button type=\"submit\" (click)=\"window.open('https://gamedev.amazon.com/forums/index.html')\" class=\"btn l-primary\">                     Visit forums                     <i class=\"fa fa-external-link fa-lg\" aria-hidden=\"true\"></i>                 </button>             </div>         </div>     </div> </div>",
                    styles: [".support-container{margin-left:30px}.support-container{padding:20px 20px 5px 20px;width:450px;margin-bottom:25px;min-height:225px;background-color:#f8f8f8;border:1px solid #999}.support-container h2{text-align:left;margin-bottom:25px}.support-container button[type=submit]{display:block;margin:0 auto;margin-top:30px;padding:5px 20px;font-size:15px}.support-container button[type=submit] i{color:#fff}.cgp-content-container>h1{margin-bottom:15px}"]
                }), __metadata("design:paramtypes", [aws_service_1.AwsService, index_1.ApiService])], SupportComponent);
                return SupportComponent;
            }();
            exports_1("SupportComponent", SupportComponent);
        }
    };
});
System.register("dist/app/view/game/module/support/support.module.js", ["@angular/core", "../shared/shared.module", "./support.routes", "./component/support.component"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, shared_module_1, support_routes_1, support_component_1, SupportModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (shared_module_1_1) {
            shared_module_1 = shared_module_1_1;
        }, function (support_routes_1_1) {
            support_routes_1 = support_routes_1_1;
        }, function (support_component_1_1) {
            support_component_1 = support_component_1_1;
        }],
        execute: function () {
            SupportModule = /** @class */function () {
                function SupportModule() {}
                SupportModule = __decorate([core_1.NgModule({
                    imports: [support_routes_1.SupportRoutingModule, shared_module_1.GameSharedModule],
                    declarations: [support_component_1.SupportComponent]
                })], SupportModule);
                return SupportModule;
            }();
            exports_1("SupportModule", SupportModule);
        }
    };
});
System.register("dist/app/view/game/module/admin/admin.routes.js", ["@angular/core", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, router_1, AdminRoutingModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            AdminRoutingModule = /** @class */function () {
                function AdminRoutingModule() {}
                AdminRoutingModule = __decorate([core_1.NgModule({
                    imports: [router_1.RouterModule.forChild([])],
                    exports: [router_1.RouterModule]
                })], AdminRoutingModule);
                return AdminRoutingModule;
            }();
            exports_1("AdminRoutingModule", AdminRoutingModule);
        }
    };
});
System.register("dist/app/view/game/module/shared/shared.module.js", ["@angular/core", "app/shared/shared.module", "./component/index", "ng2-dragula/ng2-dragula", "./service/index", "./directive/facet.directive"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, shared_module_1, index_1, ng2_dragula_1, index_2, facet_directive_1, GameSharedModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (shared_module_1_1) {
            shared_module_1 = shared_module_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (ng2_dragula_1_1) {
            ng2_dragula_1 = ng2_dragula_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (facet_directive_1_1) {
            facet_directive_1 = facet_directive_1_1;
        }],
        execute: function () {
            GameSharedModule = /** @class */function () {
                function GameSharedModule() {}
                GameSharedModule = __decorate([core_1.NgModule({
                    imports: [shared_module_1.AppSharedModule, ng2_dragula_1.DragulaModule],
                    exports: [shared_module_1.AppSharedModule, index_1.DragableComponent, index_1.ActionStubComponent, index_1.ActionStubBasicComponent, index_1.ActionStubItemsComponent, index_1.PaginationComponent, index_1.SearchComponent, index_1.FacetComponent, index_1.ThumbnailComponent, index_1.GraphComponent],
                    declarations: [index_1.DragableComponent, index_1.ActionStubComponent, index_1.ActionStubBasicComponent, index_1.ActionStubItemsComponent, index_1.PaginationComponent, index_1.SearchComponent, index_1.FacetComponent, facet_directive_1.FacetDirective, index_1.BodyTreeViewComponent, index_1.RestApiExplorerComponent, index_1.CloudWatchLogComponent, index_1.MetricComponent, index_1.ThumbnailComponent, index_1.GraphComponent],
                    providers: [ng2_dragula_1.DragulaService, index_2.InnerRouterService, index_2.ApiGatewayService, index_2.DependencyService, index_2.DependencyGuard],
                    entryComponents: [index_1.BodyTreeViewComponent, index_1.RestApiExplorerComponent, index_1.CloudWatchLogComponent, index_1.MetricComponent, index_1.GraphComponent]
                })], GameSharedModule);
                return GameSharedModule;
            }();
            exports_1("GameSharedModule", GameSharedModule);
        }
    };
});
System.register("dist/app/view/game/module/admin/admin.module.js", ["@angular/core", "./admin.routes", "../shared/shared.module", "./component/admin.component", "./component/log.component", "./component/user-admin/user-admin.component", "./component/jira/jira-credentials.component"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, admin_routes_1, shared_module_1, admin_component_1, log_component_1, user_admin_component_1, jira_credentials_component_1, AdminModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (admin_routes_1_1) {
            admin_routes_1 = admin_routes_1_1;
        }, function (shared_module_1_1) {
            shared_module_1 = shared_module_1_1;
        }, function (admin_component_1_1) {
            admin_component_1 = admin_component_1_1;
        }, function (log_component_1_1) {
            log_component_1 = log_component_1_1;
        }, function (user_admin_component_1_1) {
            user_admin_component_1 = user_admin_component_1_1;
        }, function (jira_credentials_component_1_1) {
            jira_credentials_component_1 = jira_credentials_component_1_1;
        }],
        execute: function () {
            AdminModule = /** @class */function () {
                function AdminModule() {}
                AdminModule = __decorate([core_1.NgModule({
                    imports: [admin_routes_1.AdminRoutingModule, shared_module_1.GameSharedModule],
                    declarations: [admin_component_1.AdminComponent, log_component_1.ProjectLogComponent, user_admin_component_1.UserAdminComponent, jira_credentials_component_1.JiraCredentialsComponent]
                })], AdminModule);
                return AdminModule;
            }();
            exports_1("AdminModule", AdminModule);
        }
    };
});
System.register("dist/app/view/game/component/game.component.js", ["@angular/core", "@angular/animations"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, animations_1, GameComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (animations_1_1) {
            animations_1 = animations_1_1;
        }],
        execute: function () {
            GameComponent = /** @class */function () {
                function GameComponent() {
                    this.sidebarState = "Expanded";
                }
                GameComponent.prototype.updateSidebarState = function (sidebarState) {
                    this.sidebarState = sidebarState;
                };
                GameComponent = __decorate([core_1.Component({
                    selector: 'cc-game',
                    template: "<div class=\"cgp-container\">     <cgp-nav></cgp-nav>        <cgp-sidebar [sidebarState]=\"sidebarState\" (sidebarStateChange)=\"updateSidebarState($event)\"></cgp-sidebar>     <div class=\"cgp-content-container\" [@containerMargin]=\"sidebarState\" [ngClass]=\"sidebarState === 'Collapsed' ? 'collapsed' : 'expanded'\">         <router-outlet></router-outlet>     </div>     <div class=\"footer-push\"></div> </div> <cgp-footer [sidebarState]=\"sidebarState\"></cgp-footer>",
                    styles: ["\n        .cgp-content-container {\n            padding-top: 100px;\n            margin-right: 35px;\n        }\n        .cgp-content-container > h1 {\n            margin: 25px 0 15px 0;\n        }\n\n        .cgp-content-container.expanded {\n            margin-left: 300px;\n        }\n        .cgp-content-container.collapsed {\n            margin-left: 115px;\n        }\n        "],
                    animations: [animations_1.trigger('containerMargin', [animations_1.state('Collapsed', animations_1.style({
                        'margin-left': '115px'
                    })), animations_1.state('Expanded', animations_1.style({
                        'margin-left': '300px'
                    })), animations_1.transition('Collapsed => Expanded', animations_1.animate('300ms ease-in')), animations_1.transition('Expanded => Collapsed', animations_1.animate('300ms ease-out'))])]
                })], GameComponent);
                return GameComponent;
            }();
            exports_1("GameComponent", GameComponent);
        }
    };
});
System.register("dist/app/view/game/component/nav.component.js", ["@angular/core", "app/aws/aws.service", "ts-clipboard", "app/shared/service/index", "ng2-toastr", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, ts_clipboard_1, index_1, ng2_toastr_1, router_1, NavComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (ts_clipboard_1_1) {
            ts_clipboard_1 = ts_clipboard_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            NavComponent = /** @class */function () {
                function NavComponent(aws, urlService, lymetrics, toastr, vcr, router, gems) {
                    var _this = this;
                    this.aws = aws;
                    this.urlService = urlService;
                    this.lymetrics = lymetrics;
                    this.toastr = toastr;
                    this.router = router;
                    this.gems = gems;
                    this.projectName = "Cloud Canvas Project";
                    this.nav = {
                        "deploymentName": "",
                        "projectName": this.aws.context.configBucket,
                        "deployments": [],
                        "username": ""
                    };
                    this.isCollapsed = true;
                    this.setContext = function () {
                        var username = _this.aws.context.authentication.user.username;
                        _this.nav = {
                            "deploymentName": _this.aws.context.project.activeDeployment ? _this.aws.context.project.activeDeployment.settings.name : "No Deployments",
                            "projectName": _this.aws.context.name,
                            "deployments": _this.aws.context.project.deployments,
                            "username": username.charAt(0).toUpperCase() + username.slice(1)
                        };
                    };
                    this.toastr.setRootViewContainerRef(vcr);
                    // Listen for the active deployment and the name when the nav is loaded.
                    if (this.aws.context.project.activeDeployment) {
                        this.setContext();
                    } else {
                        this.aws.context.project.activeDeploymentSubscription.subscribe(function (activeDeployment) {
                            if (activeDeployment) {
                                var url = location.search.indexOf('target') === -1 ? '/game/cloudgems' : '/game/cloudgems' + location.search;
                                _this.router.navigateByUrl(url).then(function () {
                                    _this.gems.clearCurrentGems();
                                    _this.nav.deploymentName = activeDeployment.settings.name;
                                });
                                lymetrics.recordEvent('DeploymentChanged', {}, {
                                    'NumberOfDeployments': _this.aws.context.project.deployments.length
                                });
                            } else {
                                _this.nav.deploymentName = "No Deployments";
                            }
                        });
                        this.aws.context.project.settings.subscribe(function (settings) {
                            _this.setContext();
                            if (settings) lymetrics.recordEvent('ProjectSettingsInitialized', {}, {
                                'NumberOfDeployments': Object.keys(settings.deployment).length
                            });
                        });
                        this.lymetrics.recordEvent('FeatureOpened', {
                            "Name": 'cloudgems'
                        });
                    }
                }
                NavComponent.prototype.onDeploymentClick = function (deployment) {
                    this.gems.isLoading = true;
                    this.aws.context.project.activeDeployment = deployment;
                    this.nav.deploymentName = deployment.settings.name;
                };
                NavComponent.prototype.signOut = function () {
                    this.aws.context.authentication.logout();
                };
                NavComponent.prototype.onShare = function () {
                    var params = this.urlService.querystring ? this.urlService.querystring.split("&") : [];
                    var expiration = String(new Date().getTime());
                    for (var i = 0; i < params.length; i++) {
                        var param = params[i];
                        var paramParts = param.split("=");
                        if (paramParts[0] == "Expires") {
                            expiration = paramParts[1];
                            break;
                        }
                    }
                    if (expiration != null && Number(expiration) != NaN) {
                        var expirationDate = new Date(0);
                        expirationDate.setUTCSeconds(Number(expiration));
                        var now = new Date();
                        var expirationTimeInMilli = expirationDate.getTime() - now.getTime();
                        if (expirationTimeInMilli < 0) {
                            this.lymetrics.recordEvent('ProjectUrlSharedFailed', null, { 'Expired': expirationTimeInMilli });
                            this.toastr.error("The url has expired and is no longer shareable.  Please use the Lumberyard Editor to generate a new URL. AWS -> Open Cloud Gem Portal", "Copy To Clipboard");
                        } else {
                            var minutes = Math.round(expirationTimeInMilli / 1000 / 60);
                            ts_clipboard_1.Clipboard.copy(this.urlService.baseUrl);
                            this.toastr.success("The shareable Cloud Gem Portal URL has been copied to your clipboard.", "Copy To Clipboard");
                            this.lymetrics.recordEvent('ProjectUrlSharedSuccess', null, {
                                'UrlExpirationInMinutes': minutes
                            });
                        }
                    } else {
                        this.lymetrics.recordEvent('ProjectUrlSharedFailed');
                        this.toastr.error("The URL is not properly set. Please use the Lumberyard Editor to generate a new URL. AWS -> Open Cloud Gem Portal", "Copy To Clipboard");
                    }
                };
                NavComponent = __decorate([core_1.Component({
                    selector: 'cgp-nav',
                    template: "<nav class=\"navbar navbar-toggleable-sm navbar-light bg-faded\">     <button (click)=\"isCollapsed = !isCollapsed\" class=\"navbar-toggler navbar-toggler-right\" type=\"button\" data-toggle=\"collapse\" data-target=\"#cgpNavbar\" aria-controls=\"cgpNavbar\" aria-expanded=\"false\" aria-label=\"Toggle navigation\">         <span class=\"navbar-toggler-icon\"></span>     </button>     <div class=\"navbar-brand\">         <div class=\"brand-left\">             <a href=\"{{urlService.baseUrl}}\">                 <div class=\"brand-image\">                     <img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADwAAAApCAYAAABp50paAAAAAXNSR0IArs4c6QAADMFJREFUaAXFWXt0VMUZ/2bu3d1sEhI0EqwYeVcriIFUeSbZAFIVFFRSaZPdkN1YrKK1ov3HVypaHwfRU20RzOa1WaGgBYoK9ajZJDyUFhRBBBOQozxqkbdJNrt7Z/qbDdmzhsRENqFzzt2ZO/ebb77XfI9ZRv+n5phUeYVJZ7cy4jOJZIMUbLm7Nq8O5MjeJIn1JvLOcLts3ofA1yOMsb7RMFLKDaFAaH7F5rn7oud7cnzBGXZmex7mnD8PNe6HLg+D8ZGM2A6872SM5ktJ9X6Sk72+/IM9yWgbLt42uBC9y+YZxjkrBlNbAseMUW5fXqYQgVGSyZUY3ycMcSdjcmgc0VO9Rc8F1bAzx/MgJ/6CkFQomXGESzZIEvRKLMgZaVLQCeKyGMwOlEE5cUdz0+5t2+YFe5J5vSeRdYULDmqYggFzrxJpFlKsRi1iYXtrnWE6/evaxISZ24jWR4HEPLxgDLuyKkbjzM6QjAIk5QdMynWGlHtJI4NLPgicT2OSTUN/iSTZSJItZUHaHTOH7RBckDNcMLF8BGnaahhvGpMUEAY9VFJjX1JW6wDj2mAw+lNDBl2BkJGB8/0K9B4HxgfuM9cfakdvzK/RFhUzso4QzLV50zUm14DRy8DME2D6LsmYhQz5sMGD7+hkuhfMjZB+/3z3lqLjCocrp2oZPPddQhrXlfoc/+4I7/nO9aqGC3Mqx2qM3g4zS8Lhrsl/JkTByQhFbzONnteY6UMQni5IvtnGbJgRSWmIyWcoGETY6tnWa2fYme3N4ZJWgtwEEmyOu9bxd0V6ua/wALrf5GYsTe5jjR9EnCcboVDYdF2ZFdeTrtmg3Rtxzl9yb3L1OMO9YtLO7Kqb4HFXgHAoSsxx++wbFLOdtZuG/dkyIC1lGb47WmHkoZAM/hzC+U9na37M/Fxb2VWc6SNKq+1v9riGFbOc0xvwsk2Iq3e4a+21XRF32YCLFwMGzMqVyLimMMEqymtiYzZ3/GJroiV1CkJgIZzgDOD/1j6qckOPMlxk89iAHGYsvzOEuK2s1r65K2YLMz1jEJjnwYTXlvgafuXMHp7dIsSertZ19t05yTuETGIOrMuB50rlC5C97UKCM5L3EYN6jGFlNkC6HF7YCAk5uzyKWedEd58ANydW1dmPtCeUa9yB+MyRcC0kKhalNVTdHqard+wdp5FuI8adgJ2JBMeMWL4bx+kB1uxfQfHWBTi7Y0wmfXSPMAxTSdCYVsFIphpEt5fX2FWZF2nSbBoTJ/mrRTnew0grPRT0v1m6yXXGZivG/nIaiNl1qnrtJ5EF3RzYsyoHmzWeC/ACaPNqWEkQIW+NQYb7S7n//aGB4QNEXPzVcJ6/RegjQcyCLvbmyq76I+PscUj1CXd1/pMdYGQwtcFMl05sfDfM/hTi8GOCAnUaN++DdstKqvPmdbDunKncEcXmPinDbFyjuYjrt6HEjIPZNsA9VoZkaEV5bWF9LuVqcRNuSbFYNHWkhiokgsQjMhR8LWYNz80qGw6DXABns/Xw18efO4fC1glZujFvP4aPovBfZta1P5HGvJzMO8C8iYToUruuCd6BzCxnA0c+BJQOy4DC6C1UWO6jjU3vrds2r6lt7yTbrNX4fh3eL4VQ1qDfKyi0uLzO6Y+ZYY2Z74aZJMCMHl/fcH9L26ad9ZUbHV/hW77T5n0P5x1ppJI+JXYEn5Gx1JSelJClHBCsZxaEkxQ2WymfZkJ4Eds/j14Hp1mM0mQchK+OyQGsqW0x5D3RviMmk861/SUxmSXvgbM6tk/WZ/h8xaFoAroahxMNTSuHWf4M+lpssMBCxN6Tah1q53lwQveAwFFhPJL2oPC4CFlbUogCYwAX8eTOLM+tCD/XIolZAM32AT0nUZhML/flq0zuey0mDSdQ8khIfQCToqQrZvNsVZebjUBLWZ3zaBsF7rqCrfmZnikWDZURpwd1ab4Zmn/OH6K18NxNsIAgTBIKk7Ac+QLWHYFDX6dJ030Y36uOk6aZrkDMXwTzHq7wSoNc0vBvKN/0dYdJS0waBnEuSLaEDGN6Sa3jHbVh+6acTNIlw16EFgtBlCoN66DN1X5O70Zd4zCnrep3gHkMBF0MmCOArZOSncb7XIzDigHzKzAeAraGMkNkEtdexPsvWveUL5OQ61pOyM2eTx2N7eloe+9Qw8rLUcbUxDiL1aIAhcH9h6wNTe21yKW4DGYHn8M6LeMSU4fk4AzeA09arbQGAnOYxmbEoeZ12areAvEr/M3sg1Jf/kuFmaVeTTO7YJJ3YH46NJyA/hDWIkYzXP9QJnC8Aqf1jOR8O96Vh65GOKxtJlbirbEfbGOssz6iYfuEylSTic9EeLkBm4zABik4LwmwKOzP4AHlSWx2EAv2QEPbEU8/5JosZIw/JALiqtJN9r0dbQLNPcAZexHZSEZZnX27Y3zFAJNZn4yrnHwIYlp4DTQKu31XSPlPFqLtLSbWGEeiL0JXvF/TDitLcOV44dGJWkLixjidfw6q/guGvxYi9GhpTcGWjvbuaC6sYXXoGeevAEHaWaCvILkD4BQmhcSMKAl9Ci7bxkEQU1Da4aICIpB0Br9CmLi1I+StcwwwaEz2VV3llgJlDR7E5U1kos8RP3djn+8w59A4KyAzkVVSs2S8iXTWYpXyEOrjExD0KGz4hlmjyYBNxvMUYvci9D+q6SolxNnxABksgx4NBkNrjvtbvoyOawqjqmj6X97nIsa0gUzy0RDAVMgiB5+QA4hB6D/Bc05jguqVdHBbeTU+fhAB0MV4lQICxxJ3dd4ydaOJm5AxpPGR8AvYg+IhYA74fsoSIFwkUWw2ctDZUMB+YcgOfUYEfycDnTPzryG9JCB0lNTkeTqBo3CMbSDl+dTzEZ5Xw/dUXN+Kc6xMcw2ec1pQGAfMXAuB6KuiP+L20h5+l7KoYEL5+yghG/CuHlVDR5pyesmXDHXBAv+qGIVSlgijpSLa20eAuzFQHidscjDc1G7AR0AKs0rTJNMmwQkZMI07YCmXRj5GDZr1M8dhzsdhtle0TRfaqmaAeCWkL/Ck6xb9H52tX/VZcUAQ34Pz/XvyN1+H++tF58us2l8PBcTr5jg2H6a1qMjmHS2EWAnHuzvYJE5IP/MLK9es1GyVVks//P8zBLY5GnY2CeYGk2TwooQYyVJ1qd+EcZlC+r3m8zVTzqyTEGiKmleCglm+DKtqhLO7lZnYOOAp15lJ1cR5eCCL77fSmrxqzKgn5saVE5GGMQsmvRHaykO5thaet94cr9dbUvjeuHj2hYy37sPZ3am+4XwVAw7hgX2spI6LtqmgMIC5iZ1Rg+MXAhtG0djX+qNYWAnnOAi+7g/Ks7t9+RXgcTlcyJzCLM/4znD01HzYS7trCz5GqZYzmIZM1KRyRAxhifqDSBN6eDN2Gho9BEdSj35nMCh3ns2J6ZaMpfGpSQln4FH6dUTU/oypPF2iQGC4tbTGrwezoyGAZ9y4po3Ah4xnSdd/qXFyYm5zZL4XBmGGFV6VVPiIajBUT7dbcnx8MgSikv9wDtx+4bXWxDSc4f4QYhKYhfjkJ4Hj4ulouJK6gp2Is9uQNk5TVzOrtjzYHP29J8fK7cfULIyps2sRQm6MRpQ3tirJle21cx130qrKIVJ/gW6Dr0g3X6xtcE70XBkFL1EyqfWX9+H9Is4t6nuPDSMaPh+MTlvlJKxT2vrCb7A3HNdXpJgStHSocTrmbodCB8I3+JFcvETNzc9+ExJnUhMTF6pCgZnZ+1g/BxftYUFhTmVqDMb/E/Rq3CvtvBgO3zkzeR/OJRJ3Fg8zPWrVaRWZtHRoE944bLtfSSGfN1iwLLqUAxcLirI9u+Dtl8ARroWjukVd9uFG4hhis3LRfXuF07NIz4thzuQY6OK2CGGMXQNKj4LxBslEJcbvoiDY7P0o/3QEJmqA/5XKcJ17EtnXck1jyxGDx5JBQZWRCYYbqF5s58VwoMVYYbZo48C0unJRWjHgkNafOt14/6pt8051h97SmvzVSCfnIwS+ppHpWRQTYdPmodAR3F5y56Sh1yBHOFi5teBYd/B1F0bZ3vk2hqT+MQStYjDbikfKt/DfUS5M2N9dpMCxFv8e3ozYdwxI+qNo+RvwpQHv9cC6oylIN7y+Me9Ed/F1BReLl5bhG8pwrXp2G8Zm6EIb3NWmbd/V9S5ieyoY0xWzah4Fwp34naDm8JoRp1NGG3xP9LEwHN7/dM3aJ6GV9eoFpr31VGPz4e4S5v90nR+a3Q9P/jGe5xDa7geup/FsCuOQ8suAIT7rLr7uwLWaYncgfwBGXaFKk5zWzBpXL/fN+/YHQM/5pC4CvztoBNvdeKrjMhnp5zdun2PXOYtimPgf71U9Sxn1G4AAAAAASUVORK5CYII=\">                 </div>             </a>             <a class=\"brand-text\" href=\"{{urlService.baseUrl}}\" title=\"Cloud Gem Portal\"> Cloud Gem Portal </a>         </div>         <div class=\"brand-right\">             <p class=\"small\">                 {{nav.projectName}}             </p>             <div ngbDropdown class=\"d-inline-block\">                 <div id=\"deploymentDropdown\" dropdownToggle ngbDropdownToggle>                     <a title=\"{{nav.deploymentName}}\">                         {{nav.deploymentName}}                         <i aria-hidden=\"true\" class=\"fa fa-caret-down\"></i>                     </a>                 </div>                 <div class=\"dropdown-menu\" dropdownMenu aria-labelledby=\"deploymentDropdown\">                     <a class=\"dropdown-item\" *ngFor=\"let deployment of nav.deployments\" (click)=\"onDeploymentClick(deployment)\"> {{deployment.settings.name}} </a>                 </div>             </div>         </div>     </div>     <div [ngbCollapse]=\"isCollapsed\" class=\"collapse navbar-collapse \" id=\"cgpNavbar\">                 <div class=\"navbar-nav\" [ngClass]=\"{'navbar-collapsed': isCollapsed == false}\">             <a (click)=\"$event.preventDefault()\" class=\"nav-item nav-link\" href=\"#\" title=\"Copy the Cloud Gem Portal shareable URL to the clipboard\">                 <i class=\"fa fa-share-square-o\" (click)=\"onShare()\"></i>             </a>             <div ngbDropdown class=\"d-inline-block deployment-dropdown\" [ngClass]=\"{'navbar-user-collapsed': isCollapsed == false}\">                 <div id=\"account-dropdown\" dropdownToggle ngbDropdownToggle>                     <a title=\"{{nav.deploymentName}}\">                         {{ nav.username }}                         <i aria-hidden=\"true\" class=\"fa fa-caret-down\"></i>                     </a>                 </div>                 <div id=\"signout-dropdown\" class=\"dropdown-menu\" dropdownMenu aria-labelledby=\"deploymentDropdown\">                     <a class=\"dropdown-item\" id=\"sign-out\" (click)=\"signOut()\"> Sign out </a>                 </div>             </div>         </div>     </div> </nav>",
                    styles: ["*:not(i){font-family:\"AmazonEmber-Light\"}.dropdown-toggle::after{display:none}#signout-dropdown{right:0px;text-align:center;left:auto}nav.navbar.navbar{background-image:linear-gradient(-180deg, #fff 0%, #f5f5f5 100%);box-shadow:1px 1px 3px 0 rgba(85,85,85,0.5);height:75px;padding:0;margin-bottom:1px;z-index:1001;position:fixed;top:0;width:100%}nav.navbar.navbar a{font-size:20px;cursor:pointer}nav.navbar.navbar a,nav.navbar.navbar i{color:#444}nav.navbar.navbar .dropdown-menu a{display:block}nav.navbar.navbar .dropdown-menu a.active,nav.navbar.navbar .dropdown-menu a:active{background-color:#6441A5;color:#fff}nav.navbar.navbar a.share-link{position:absolute;font-size:12px}.navbar-brand{font-size:1rem;padding-top:0;padding-bottom:0}.navbar-brand a,.navbar-brand i{color:#6441A5;display:inline-block;vertical-align:middle}.navbar-brand .brand-left{margin:0 20px;display:inline-block}.navbar-brand .brand-left>.brand-text{border-right:1px solid #eee;padding-left:5px;padding-right:20px;padding:25px 20px 20px 5px;color:#6441A5}.navbar-brand .brand-left .brand-image{display:inline-block}.navbar-brand .brand-right{display:inline-block;position:relative;top:.5em;color:#444}.navbar-brand .brand-right p.small{font-size:13px;color:#222;margin-bottom:-2px}.navbar-brand .brand-right a{padding-right:5px}.navbar-brand .brand-right a i{margin-left:3px}.navbar-nav{margin-left:auto;padding-right:20px;background-color:transparent;padding-left:10px}.navbar-nav .dropdown{padding:8px;margin:8px 0;border-left:1px solid #eee}.navbar-nav .dropdown a i{padding-left:5px}.navbar-nav a.nav-item{padding:12px 5px;margin:0 12px;color:#444}.navbar-nav a.nav-item i{padding:0;vertical-align:middle}.navbar-collapsed{border:1px solid rgba(0,0,0,0.1);padding-bottom:10px;background-color:#ffffff}.navbar-user-collapsed{padding-left:15px}.deployment-dropdown{padding-top:12px}.deployment-dropdown a>i{margin-left:5px}"]
                }), __metadata("design:paramtypes", [aws_service_1.AwsService, index_1.UrlService, index_1.LyMetricService, ng2_toastr_1.ToastsManager, core_1.ViewContainerRef, router_1.Router, index_1.GemService])], NavComponent);
                return NavComponent;
            }();
            exports_1("NavComponent", NavComponent);
        }
    };
});
System.register("dist/app/view/game/module/analytic/component/dashboard/dashboard.component.js", ["@angular/core", "@angular/forms", "@angular/router", "@angular/http", "app/aws/aws.service", "app/shared/service/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, forms_1, router_1, http_1, aws_service_1, index_1, DashboardComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (forms_1_1) {
            forms_1 = forms_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }],
        execute: function () {
            DashboardComponent = /** @class */function () {
                function DashboardComponent(fb, http, aws, definition, breadcrumbs, router) {
                    var _this = this;
                    this.fb = fb;
                    this.http = http;
                    this.aws = aws;
                    this.definition = definition;
                    this.breadcrumbs = breadcrumbs;
                    this.router = router;
                    this.tabs = ['Telemetry', 'Engagement'];
                    this.initializeBreadcrumbs = function () {
                        _this.breadcrumbs.removeAllBreadcrumbs();
                        _this.breadcrumbs.addBreadcrumb("Analytics", function () {
                            _this.router.navigateByUrl('/game/analytics');
                        });
                        _this.breadcrumbs.addBreadcrumb("Dashboard", function () {
                            _this.initializeBreadcrumbs();
                        });
                    };
                }
                DashboardComponent.prototype.ngOnInit = function () {
                    this.initializeBreadcrumbs();
                };
                DashboardComponent = __decorate([core_1.Component({
                    selector: 'analytic-dashboard',
                    template: "\n        <breadcrumbs></breadcrumbs>\n        <facet-generator [context]=\"context\"\n        [tabs]=\"tabs\"\n        (tabClicked)=\"idx=$event\"\n        ></facet-generator>\n        <div *ngIf=\"idx === 0\">            \n            <dashboard-graph [facetid]=\"tabs[idx]\"></dashboard-graph>\n        </div>      \n        <div *ngIf=\"idx === 1\">\n            <dashboard-graph [facetid]=\"tabs[idx]\"></dashboard-graph>\n        </div>      \n    ",
                    styles: ["\n        .dashboard-heading {\n            font-size: 24px;\n            padding-left: 0;\n        }\n    "]
                }), __metadata("design:paramtypes", [forms_1.FormBuilder, http_1.Http, aws_service_1.AwsService, index_1.DefinitionService, index_1.BreadcrumbService, router_1.Router])], DashboardComponent);
                return DashboardComponent;
            }();
            exports_1("DashboardComponent", DashboardComponent);
        }
    };
});
System.register("dist/app/view/game/module/analytic/component/heatmap/heatmap.component.js", ["@angular/core", "@angular/forms", "@angular/http", "ng2-toastr/ng2-toastr", "app/shared/service/index", "app/aws/aws.service", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, forms_1, http_1, ng2_toastr_1, index_1, aws_service_1, router_1, HeatmapComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (forms_1_1) {
            forms_1 = forms_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            HeatmapComponent = /** @class */function () {
                function HeatmapComponent(fb, breadcrumbs, definition, aws, http, toastr, router) {
                    var _this = this;
                    this.fb = fb;
                    this.breadcrumbs = breadcrumbs;
                    this.definition = definition;
                    this.aws = aws;
                    this.http = http;
                    this.toastr = toastr;
                    this.router = router;
                    this.mode = "List";
                    this.isLoadingHeatmaps = true;
                    this.initializeBreadcrumbs = function () {
                        _this.breadcrumbs.removeAllBreadcrumbs();
                        _this.breadcrumbs.addBreadcrumb("Analytics", function () {
                            _this.router.navigateByUrl('/game/analytics');
                        });
                        _this.breadcrumbs.addBreadcrumb("Heatmaps", function () {
                            _this.mode = "List";
                            _this.initializeBreadcrumbs();
                        });
                    };
                    this.onHeatmapClick = function (heatmap) {
                        _this.selectedHeatmap = heatmap;
                        _this.mode = "Create";
                    };
                    this.createHeatmap = function () {
                        _this.breadcrumbs.addBreadcrumb("Create Heatmap", null);
                        _this.selectedHeatmap = undefined;
                        _this.mode = "Create";
                    };
                }
                HeatmapComponent.prototype.ngOnInit = function () {
                    this.initializeBreadcrumbs();
                };
                HeatmapComponent = __decorate([core_1.Component({
                    selector: 'heatmap',
                    template: "<breadcrumbs></breadcrumbs> <div class=\"heatmap-container\">     <div [ngSwitch]=\"mode\">         <div *ngSwitchCase=\"'List'\">             <p> Heatmaps can be used to easily view your metrics data.  This can range from seeing where most of bugs are being found to where most players are racking up kills.  Get started by creating a heatmap or editing an existing one below.</p>             <button type=\"button\" (click)=\"createHeatmap()\" class=\"btn l-primary\"> Create Heatmap </button>         </div>         <div *ngSwitchCase=\"'Create'\">             <create-heatmap [heatmap]=\"selectedHeatmap\" [mode]=\"mode\"> </create-heatmap>         </div>         <div class=\"existing-heatmaps-container\" *ngSwitchCase=\"'List'\">             <list-heatmap (onHeatmapClick)=\"onHeatmapClick($event)\"></list-heatmap>         </div>     </div> </div>",
                    styles: ["\n        .existing-heatmaps-container {\n            margin-top: 25px;\n        }\n    "]
                }), __metadata("design:paramtypes", [forms_1.FormBuilder, index_1.BreadcrumbService, index_1.DefinitionService, aws_service_1.AwsService, http_1.Http, ng2_toastr_1.ToastsManager, router_1.Router])], HeatmapComponent);
                return HeatmapComponent;
            }();
            exports_1("HeatmapComponent", HeatmapComponent);
        }
    };
});
System.register("dist/app/view/game/module/admin/component/jira/jira-credentials.component.js", ["@angular/core", "@angular/forms", "@angular/http", "ng2-toastr/ng2-toastr", "app/shared/service/index", "app/aws/aws.service", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, forms_1, http_1, ng2_toastr_1, index_1, aws_service_1, router_1, JiraCredentialsComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (forms_1_1) {
            forms_1 = forms_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            JiraCredentialsComponent = /** @class */function () {
                function JiraCredentialsComponent(fb, http, aws, definition, breadcrumbs, router, toastr) {
                    var _this = this;
                    this.fb = fb;
                    this.http = http;
                    this.aws = aws;
                    this.definition = definition;
                    this.breadcrumbs = breadcrumbs;
                    this.router = router;
                    this.toastr = toastr;
                    this.initializeBreadcrumbs = function () {
                        _this.breadcrumbs.removeAllBreadcrumbs();
                        _this.breadcrumbs.addBreadcrumb("Administration", function () {
                            _this.router.navigateByUrl('/game/analytics');
                        });
                        _this.breadcrumbs.addBreadcrumb("Jira Credentials", function () {
                            _this.initializeBreadcrumbs();
                        });
                    };
                    this.updateJiraCredentials = function () {
                        if (_this.validate(_this.currentCredentials)) {
                            var body = {};
                            for (var _i = 0, _a = Object.keys(_this.currentCredentials); _i < _a.length; _i++) {
                                var property = _a[_i];
                                body[property] = _this.currentCredentials[property]['value'];
                            }
                            _this.defectReporterApiHandler.updateJiraCredentials(body).subscribe(function (response) {
                                _this.jiraCredentialsExist = true;
                                _this.toastr.success("Jira credentials were stored successfully.");
                            }, function (err) {
                                _this.toastr.error("Failed to store Jira credentials. " + err.message);
                            });
                        }
                    };
                    this.getJiraCredentials = function () {
                        _this.isLoadingJiraCredentials = true;
                        _this.defectReporterApiHandler.getJiraCredentialsStatus().subscribe(function (response) {
                            var obj = JSON.parse(response.body.text());
                            _this.jiraCredentialsExist = obj.result.exist;
                            _this.isLoadingJiraCredentials = false;
                        }, function (err) {
                            _this.toastr.error("Failed to store Jira credentials. " + err.message);
                            _this.jiraCredentialsExist = false;
                            _this.isLoadingJiraCredentials = false;
                        });
                    };
                    this.signOut = function () {
                        var body = {};
                        for (var _i = 0, _a = Object.keys(_this.currentCredentials); _i < _a.length; _i++) {
                            var property = _a[_i];
                            body[property] = "";
                        }
                        _this.defectReporterApiHandler.updateJiraCredentials(body).subscribe(function (response) {
                            _this.currentCredentials = _this.default();
                            _this.jiraCredentialsExist = false;
                            _this.toastr.success("The existing credentials were cleared successfully.");
                        }, function (err) {
                            _this.toastr.error("Failed to clear the existing credentials. " + err.message);
                        });
                    };
                    this.validate = function (model) {
                        var isValid = true;
                        for (var _i = 0, _a = Object.keys(model); _i < _a.length; _i++) {
                            var property = _a[_i];
                            model[property]['valid'] = model[property]['value'] != '' ? true : false;
                            isValid = isValid && model[property]['valid'];
                        }
                        return isValid;
                    };
                }
                JiraCredentialsComponent.prototype.ngOnInit = function () {
                    var defectReporterGemService = this.definition.getService("CloudGemDefectReporter");
                    // Initialize the defect reporter api handler
                    this.defectReporterApiHandler = new defectReporterGemService.constructor(defectReporterGemService.serviceUrl, this.http, this.aws);
                    this.jiraIntegrationEnabled = defectReporterGemService.context.JiraIntegrationEnabled === 'enabled';
                    if (this.jiraIntegrationEnabled) {
                        this.currentCredentials = this.default();
                        this.getJiraCredentials();
                    }
                    this.initializeBreadcrumbs();
                };
                JiraCredentialsComponent.prototype.getLocation = function () {
                    return 1;
                };
                JiraCredentialsComponent.prototype.default = function () {
                    return {
                        userName: {
                            valid: true,
                            value: '',
                            message: 'User name cannot be empty'
                        },
                        password: {
                            valid: true,
                            value: '',
                            message: 'Password cannot be empty'
                        },
                        server: {
                            valid: true,
                            value: '',
                            message: 'Server cannot be empty'
                        }
                    };
                };
                JiraCredentialsComponent = __decorate([core_1.Component({
                    selector: 'jira-credentials',
                    template: "<breadcrumbs></breadcrumbs> <div *ngIf=\"!jiraIntegrationEnabled\">     Jira Integration is not enabled for the gem </div> <div *ngIf=\"jiraIntegrationEnabled\">     <div [ngSwitch]=\"isLoadingJiraCredentials\">         <div *ngSwitchCase=\"true\">             <loading-spinner></loading-spinner>         </div>         <div *ngSwitchCase=\"false\">             <div *ngIf=\"jiraCredentialsExist\">                 <p>Jira Credentials exist. You can update them, but this operation will clear the existing credentials.</p>                 <button class=\"btn l-primary\" (click)=\"signOut()\">                     Update                 </button>             </div>             <div *ngIf=\"!jiraCredentialsExist\">                 <div class=\"jira-credentials-container\">                     <p>Jira credentials are required by certain Cloud Gems to operate or to enable features.</p>                     <div class=\"form-group row\" [class.has-danger]=\"!currentCredentials.userName.valid\">                         <label class=\"col-3 col-form-label affix\">User Name</label>                         <div class=\"col-6\">                             <input type=\"text\" class=\"form-control\" [(ngModel)]=\"currentCredentials.userName.value\">                             <div *ngIf=\"!currentCredentials.userName.valid\" class=\"form-control-feedback\">{{currentCredentials.userName.message}}</div>                         </div>                     </div>                     <div class=\"form-group row\" [class.has-danger]=\"!currentCredentials.password.valid\">                         <label class=\"col-3 col-form-label affix\">Password</label>                         <div class=\"col-6\">                             <input type=\"text\" class=\"form-control\" [(ngModel)]=\"currentCredentials.password.value\">                             <div *ngIf=\"!currentCredentials.password.valid\" class=\"form-control-feedback\">{{currentCredentials.password.message}}</div>                         </div>                     </div>                     <div class=\"form-group row\" [class.has-danger]=\"!currentCredentials.server.valid\">                         <label class=\"col-3 col-form-label affix\">Server</label>                         <div class=\"col-6\">                             <input type=\"text\" class=\"form-control\" [(ngModel)]=\"currentCredentials.server.value\">                             <div *ngIf=\"!currentCredentials.server.valid\" class=\"form-control-feedback\">{{currentCredentials.server.message}}</div>                         </div>                     </div>                     <button class=\"btn l-primary\" (click)=\"updateJiraCredentials()\">                         Save                     </button>                 </div>             </div>         </div>     </div> </div>",
                    styles: ["\n        .jira-credentials-container {\n            margin-top: 25px;\n        }\n    "]
                }), __metadata("design:paramtypes", [forms_1.FormBuilder, http_1.Http, aws_service_1.AwsService, index_1.DefinitionService, index_1.BreadcrumbService, router_1.Router, ng2_toastr_1.ToastsManager])], JiraCredentialsComponent);
                return JiraCredentialsComponent;
            }();
            exports_1("JiraCredentialsComponent", JiraCredentialsComponent);
        }
    };
});
System.register("dist/app/view/game/module/analytic/component/analytic.component.js", ["@angular/core", "../../shared/service/index", "app/view/game/module/shared/class/feature.class"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, index_1, feature_class_1, AnalyticIndex;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (feature_class_1_1) {
            feature_class_1 = feature_class_1_1;
        }],
        execute: function () {
            AnalyticIndex = /** @class */function () {
                function AnalyticIndex(dependencyservice) {
                    this.dependencyservice = dependencyservice;
                }
                AnalyticIndex.prototype.ngOnInit = function () {
                    this.dependencyservice.subscribeToDeploymentChanges(new feature_class_1.FeatureDefinitions());
                };
                AnalyticIndex = __decorate([core_1.Component({
                    selector: 'game-analytic-index',
                    template: "<h1>Analytics</h1> <div *ngIf=\"dependencyservice.featureMap != null && (dependencyservice.featureMap | json) != ({} | json)\">     <div *ngFor=\"let feature of dependencyservice.featureMap | objKeys\">         <ng-container *ngFor=\"let child of dependencyservice.featureMap[feature]\">             <thumbnail *ngIf=\"child.location == 0\" url=\"/game/{{child.route_path}}\" icon=\"https://m.media-amazon.com/images/G/01/cloudcanvas/images/{{child.image}}\">                 <div class=\"thumbnail-label-section row no-gutters\">                     <div> {{child.name}} </div>                 </div>                 <div class=\"expanded-section row no-gutters\">                     <div class=\"col-12\">                         {{child.description}}                     </div>                 </div>             </thumbnail>         </ng-container>     </div> </div>",
                    styles: [".coming-soon-top-heading{margin:40px 0;text-align:center}.coming-soon-top-heading h1{margin-bottom:15px}.coming-soon-content{background:#F8F8F8;border:1px solid #CCCCCC;text-align:center;margin-left:-45px;margin-right:-45px;border-left:0;border-right:0}.coming-soon-content .col-4{padding:50px}.coming-soon-content .col-4 img{margin-bottom:30px}.coming-soon-content .col-4 h2{margin-bottom:20px}"]
                }), __metadata("design:paramtypes", [index_1.DependencyService])], AnalyticIndex);
                return AnalyticIndex;
            }();
            exports_1("AnalyticIndex", AnalyticIndex);
        }
    };
});
System.register("dist/app/view/game/module/shared/service/inner-router.service.js", ["@angular/core", "rxjs/BehaviorSubject"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, BehaviorSubject_1, InnerRouterService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (BehaviorSubject_1_1) {
            BehaviorSubject_1 = BehaviorSubject_1_1;
        }],
        execute: function () {
            InnerRouterService = /** @class */function () {
                function InnerRouterService() {
                    this._bs = new BehaviorSubject_1.BehaviorSubject('');
                }
                Object.defineProperty(InnerRouterService.prototype, "onChange", {
                    get: function () {
                        return this._bs.asObservable();
                    },
                    enumerable: true,
                    configurable: true
                });
                InnerRouterService.prototype.change = function (route) {
                    this._bs.next(route);
                };
                InnerRouterService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [])], InnerRouterService);
                return InnerRouterService;
            }();
            exports_1("InnerRouterService", InnerRouterService);
        }
    };
});
System.register("dist/app/view/game/module/shared/service/action/read-swagger.class.js", ["../apigateway.service"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var apigateway_service_1, ReadSwaggerAction;
    return {
        setters: [function (apigateway_service_1_1) {
            apigateway_service_1 = apigateway_service_1_1;
        }],
        execute: function () {
            ReadSwaggerAction = /** @class */function () {
                function ReadSwaggerAction(context) {
                    this.context = context;
                }
                ReadSwaggerAction.prototype.handle = function (subject) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    var apiId = 'qyqyhyjdb0'; //args[0];
                    if (apiId === undefined || apiId === null || apiId === '') {
                        subject.next({
                            state: apigateway_service_1.EnumApiGatewayState.READ_SWAGGER_FAILURE,
                            output: ["Cognito user was undefined."]
                        });
                        return;
                    }
                    var params = {
                        exportType: 'swagger',
                        restApiId: apiId,
                        stageName: 'api',
                        accepts: 'application/json',
                        parameters: {
                            'extensions': 'integrations, authorizers, documentation, validators, gatewayresponses',
                            'Accept': 'application/json'
                        }
                    };
                    this.context.apiGateway.getExport(params, function (err, data) {
                        if (err) {
                            subject.next({
                                state: apigateway_service_1.EnumApiGatewayState.READ_SWAGGER_FAILURE,
                                output: [err]
                            });
                            return;
                        }
                        subject.next({
                            state: apigateway_service_1.EnumApiGatewayState.READ_SWAGGER_FAILURE,
                            output: [JSON.parse(data.body)]
                        });
                    });
                };
                return ReadSwaggerAction;
            }();
            exports_1("ReadSwaggerAction", ReadSwaggerAction);
        }
    };
});
System.register("dist/app/view/game/module/shared/service/apigateway.service.js", ["@angular/core", "rxjs/BehaviorSubject", "app/aws/aws.service", "./action/read-swagger.class"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, BehaviorSubject_1, aws_service_1, read_swagger_class_1, EnumApiGatewayState, EnumApiGatewayAction, ApiGatewayService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (BehaviorSubject_1_1) {
            BehaviorSubject_1 = BehaviorSubject_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (read_swagger_class_1_1) {
            read_swagger_class_1 = read_swagger_class_1_1;
        }],
        execute: function () {
            (function (EnumApiGatewayState) {
                EnumApiGatewayState[EnumApiGatewayState["INITIALIZED"] = 0] = "INITIALIZED";
                EnumApiGatewayState[EnumApiGatewayState["READ_SWAGGER_SUCCESS"] = 1] = "READ_SWAGGER_SUCCESS";
                EnumApiGatewayState[EnumApiGatewayState["READ_SWAGGER_FAILURE"] = 2] = "READ_SWAGGER_FAILURE";
            })(EnumApiGatewayState || (EnumApiGatewayState = {}));
            exports_1("EnumApiGatewayState", EnumApiGatewayState);
            (function (EnumApiGatewayAction) {
                EnumApiGatewayAction[EnumApiGatewayAction["READ_SWAGGER"] = 0] = "READ_SWAGGER";
            })(EnumApiGatewayAction || (EnumApiGatewayAction = {}));
            exports_1("EnumApiGatewayAction", EnumApiGatewayAction);
            ApiGatewayService = /** @class */function () {
                function ApiGatewayService(aws) {
                    this._actions = [];
                    this._observable = new BehaviorSubject_1.BehaviorSubject({});
                    this._actions[EnumApiGatewayAction.READ_SWAGGER] = new read_swagger_class_1.ReadSwaggerAction(aws.context);
                    this.onChange.subscribe(function (context) {
                        console.log(EnumApiGatewayState[context.state] + ":" + context.output[0]);
                    });
                }
                Object.defineProperty(ApiGatewayService.prototype, "onChange", {
                    get: function () {
                        return this._observable.asObservable();
                    },
                    enumerable: true,
                    configurable: true
                });
                ApiGatewayService.prototype.change = function (state) {
                    this._observable.next(state);
                };
                ApiGatewayService.prototype.getSwaggerAPI = function (apiid) {
                    this.execute(EnumApiGatewayAction.READ_SWAGGER, apiid);
                };
                ApiGatewayService.prototype.execute = function (transition) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    (_a = this._actions[transition]).handle.apply(_a, [this._observable].concat(args));
                    var _a;
                };
                ApiGatewayService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [aws_service_1.AwsService])], ApiGatewayService);
                return ApiGatewayService;
            }();
            exports_1("ApiGatewayService", ApiGatewayService);
        }
    };
});
System.register("dist/app/view/game/module/shared/service/dependency-check.service.js", ["@angular/core", "app/aws/aws.service", "../class/dependency-verify.class", "rxjs/Subject"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, dependency_verify_class_1, Subject_1, DependencyService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (dependency_verify_class_1_1) {
            dependency_verify_class_1 = dependency_verify_class_1_1;
        }, function (Subject_1_1) {
            Subject_1 = Subject_1_1;
        }],
        execute: function () {
            DependencyService = /** @class */function () {
                function DependencyService(aws) {
                    this.aws = aws;
                    this.featureMapSubscription = new Subject_1.Subject();
                }
                /*
                *   Used to avoid circular dependencies between Components in the definitions and those components that use this service
                */
                DependencyService.prototype.subscribeToDeploymentChanges = function (def) {
                    var _this = this;
                    if (this._deploymentSub) {
                        this._deploymentSub.unsubscribe();
                    }
                    this._deploymentSub = this.aws.context.project.activeDeploymentSubscription.subscribe(function (deployment) {
                        if (deployment) {
                            if (_this._resourceGroupSub) {
                                _this._resourceGroupSub.unsubscribe();
                            }
                            _this._resourceGroupSub = deployment.resourceGroup.subscribe(function (rgs) {
                                _this.availableFeatures = new Map();
                                for (var idx in def.defined) {
                                    var feature = def.defined[idx];
                                    if (dependency_verify_class_1.Verify.dependency(feature.component.name, _this.aws, def)) {
                                        var features = [];
                                        if (feature.parent in _this.availableFeatures) {
                                            features = _this.availableFeatures[feature.parent];
                                        } else if (feature.parent != undefined) {
                                            _this.availableFeatures[feature.parent] = features;
                                        }
                                        features.push(feature);
                                        _this.featureMapSubscription.next(_this.availableFeatures);
                                    }
                                }
                            });
                        }
                    });
                };
                DependencyService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [aws_service_1.AwsService])], DependencyService);
                return DependencyService;
            }();
            exports_1("DependencyService", DependencyService);
        }
    };
});
System.register("dist/app/view/game/module/shared/class/dependency-verify.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var Verify;
    return {
        setters: [],
        execute: function () {
            Verify = /** @class */function () {
                function Verify() {}
                Verify.dependency = function (name, aws, featureDefinitions) {
                    for (var idx in featureDefinitions.defined) {
                        var feature = featureDefinitions.defined[idx];
                        if (name == feature.component.name) {
                            var all_present = true;
                            for (var i = 0; i < feature.dependencies.length; i++) {
                                var dep = feature.dependencies[i];
                                var found = false;
                                for (var idx_1 in aws.context.project.activeDeployment.resourceGroupList) {
                                    var rg = aws.context.project.activeDeployment.resourceGroupList[idx_1];
                                    if (rg.logicalResourceId == dep) found = true;
                                }
                                all_present = all_present && found;
                            }
                            return all_present;
                        }
                    }
                    return false;
                };
                return Verify;
            }();
            exports_1("Verify", Verify);
        }
    };
});
System.register("dist/app/view/game/module/shared/service/dependency-guard.service.js", ["@angular/core", "app/aws/aws.service", "app/view/game/module/shared/class/feature.class", "../class/dependency-verify.class"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, feature_class_1, dependency_verify_class_1, DependencyGuard;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (feature_class_1_1) {
            feature_class_1 = feature_class_1_1;
        }, function (dependency_verify_class_1_1) {
            dependency_verify_class_1 = dependency_verify_class_1_1;
        }],
        execute: function () {
            DependencyGuard = /** @class */function () {
                function DependencyGuard(aws) {
                    this.aws = aws;
                    this._featureDefinitions = new feature_class_1.FeatureDefinitions();
                }
                DependencyGuard.prototype.canActivate = function (activated, state) {
                    var obj = activated.component;
                    return dependency_verify_class_1.Verify.dependency(obj.name, this.aws, this._featureDefinitions);
                };
                DependencyGuard = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [aws_service_1.AwsService])], DependencyGuard);
                return DependencyGuard;
            }();
            exports_1("DependencyGuard", DependencyGuard);
        }
    };
});
System.register("dist/app/view/game/module/shared/service/index.js", ["./inner-router.service", "./apigateway.service", "./dependency-check.service", "./dependency-guard.service"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    function exportStar_1(m) {
        var exports = {};
        for (var n in m) {
            if (n !== "default") exports[n] = m[n];
        }
        exports_1(exports);
    }
    return {
        setters: [function (inner_router_service_1_1) {
            exportStar_1(inner_router_service_1_1);
        }, function (apigateway_service_1_1) {
            exportStar_1(apigateway_service_1_1);
        }, function (dependency_check_service_1_1) {
            exportStar_1(dependency_check_service_1_1);
        }, function (dependency_guard_service_1_1) {
            exportStar_1(dependency_guard_service_1_1);
        }],
        execute: function () {}
    };
});
System.register("dist/app/view/game/module/admin/component/admin.component.js", ["@angular/core", "../../shared/service/index", "app/view/game/module/shared/class/feature.class"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, index_1, feature_class_1, AdminComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (feature_class_1_1) {
            feature_class_1 = feature_class_1_1;
        }],
        execute: function () {
            AdminComponent = /** @class */function () {
                function AdminComponent(dependencyservice) {
                    this.dependencyservice = dependencyservice;
                }
                AdminComponent.prototype.ngOnInit = function () {
                    this.dependencyservice.subscribeToDeploymentChanges(new feature_class_1.FeatureDefinitions());
                };
                AdminComponent = __decorate([core_1.Component({
                    selector: 'admin',
                    template: "<h1>Administration</h1> <div *ngIf=\"dependencyservice.featureMap != null && (dependencyservice.featureMap | json) != ({} | json)\">     <div *ngFor=\"let feature of dependencyservice.featureMap | objKeys\">         <ng-container *ngFor=\"let child of dependencyservice.featureMap[feature]\">             <thumbnail *ngIf=\"child.location == 1\" url=\"/game/{{child.route_path}}\" icon=\"https://m.media-amazon.com/images/G/01/cloudcanvas/images/{{child.image}}\">                 <div class=\"thumbnail-label-section row no-gutters\">                     <div> {{child.name}} </div>                 </div>                 <div class=\"expanded-section row no-gutters\">                     <div class=\"col-12\">                         {{child.description}}                     </div>                 </div>             </thumbnail>         </ng-container>     </div> </div>"
                }), __metadata("design:paramtypes", [index_1.DependencyService])], AdminComponent);
                return AdminComponent;
            }();
            exports_1("AdminComponent", AdminComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/class/action-stub.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var ActionItem;
    return {
        setters: [],
        execute: function () {
            ActionItem = /** @class */function () {
                function ActionItem(displayname, onclick) {
                    this.name = displayname;
                    this.onClick = onclick;
                }
                return ActionItem;
            }();
            exports_1("ActionItem", ActionItem);
        }
    };
});
System.register("dist/app/view/game/module/shared/class/search-dropdown.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var SearchDropdownOption;
    return {
        setters: [],
        execute: function () {
            SearchDropdownOption = /** @class */function () {
                function SearchDropdownOption() {}
                return SearchDropdownOption;
            }();
            exports_1("SearchDropdownOption", SearchDropdownOption);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/action-stub-basic.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, ActionStubBasicComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            ActionStubBasicComponent = /** @class */function () {
                function ActionStubBasicComponent() {
                    this.hideDelete = false;
                    this.hideEdit = false;
                    this.hideAdd = false;
                }
                ActionStubBasicComponent.prototype.ngOnInit = function () {};
                ActionStubBasicComponent.prototype.onChange = function (index, model) {
                    var num = Number(index);
                    if (num >= 0) this.custom[num].onClick(model);
                };
                ActionStubBasicComponent.prototype.onClick = function (model) {
                    if (this.click) this.click(model);
                };
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubBasicComponent.prototype, "delete", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], ActionStubBasicComponent.prototype, "hideDelete", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubBasicComponent.prototype, "edit", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], ActionStubBasicComponent.prototype, "hideEdit", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubBasicComponent.prototype, "add", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], ActionStubBasicComponent.prototype, "hideAdd", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Array)], ActionStubBasicComponent.prototype, "custom", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubBasicComponent.prototype, "click", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ActionStubBasicComponent.prototype, "id", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], ActionStubBasicComponent.prototype, "model", void 0);
                ActionStubBasicComponent = __decorate([core_1.Component({
                    selector: 'action-stub-basic',
                    template: "<div class=\"action-stub border\" (click)=\"onClick(model)\">     <action-stub-items class=\"float-right\" [edit]=\"edit\" [add]=\"add\" [delete]=\"delete\" [custom]=\"custom\" [model]=\"model\" [hideEdit]=\"hideEdit\" [hideAdd]=\"hideAdd\" [hideDelete]=\"hideDelete\"></action-stub-items>     <ng-content></ng-content> </div>",
                    styles: ["action-stub-basic{padding:10px;border:1px solid #6441A5}action-stub-basic .actions{position:absolute;right:5px;top:0}action-stub-basic .actions>*{display:inline-block}action-stub-basic .fa.fa-question-circle-o{margin-left:5px}action-stub-items .actions{right:5px;top:0;position:relative}action-stub-items .actions>*{display:inline-block}action-stub-items .fa.fa-question-circle-o{margin-left:5px}.action-stub-btn{cursor:pointer}"],
                    encapsulation: core_1.ViewEncapsulation.None
                })], ActionStubBasicComponent);
                return ActionStubBasicComponent;
            }();
            exports_1("ActionStubBasicComponent", ActionStubBasicComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/action-stub-items.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, ActionStubItemsComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            ActionStubItemsComponent = /** @class */function () {
                function ActionStubItemsComponent() {
                    this.hideDelete = false;
                    this.hideEdit = false;
                    this.hideAdd = false;
                }
                ActionStubItemsComponent.prototype.ngOnInit = function () {
                    if (!this.custom) this.custom = [];
                };
                ActionStubItemsComponent.prototype.handle = function (action, model) {
                    action.onClick(model);
                };
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubItemsComponent.prototype, "delete", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], ActionStubItemsComponent.prototype, "hideDelete", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubItemsComponent.prototype, "edit", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], ActionStubItemsComponent.prototype, "hideEdit", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubItemsComponent.prototype, "add", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], ActionStubItemsComponent.prototype, "hideAdd", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Array)], ActionStubItemsComponent.prototype, "custom", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], ActionStubItemsComponent.prototype, "model", void 0);
                ActionStubItemsComponent = __decorate([core_1.Component({
                    selector: 'action-stub-items',
                    template: "<div class=\"actions\">\n        <div class=\"action-stub-btn\">\n            <i *ngIf=\"delete && !hideDelete\" class=\"fa fa-trash-o\" (click)=\"delete(model)\" data-toggle=\"tooltip\" data-placement=\"top\" title=\"Delete\"> </i>\n            <i *ngIf=\"edit && !hideEdit\" class=\"fa fa-cog\" (click)=\"edit(model)\" data-toggle=\"tooltip\" data-placement=\"top\" title=\"Edit\"> </i>\n            <i *ngIf=\"add && !hideAdd\" class=\"fa fa-plus\" (click)=\"add(model)\" data-toggle=\"tooltip\" data-placement=\"top\" title=\"Add\"> </i>\n        </div>\n        <div *ngIf=\"custom.length > 0\" ngbDropdown>\n            <i class=\"action-stub-btn\" id=\"action-dropdown\" ngbDropdownToggle></i>\n            <div class=\"dropdown-menu dropdown-menu-right\" aria-labelledby=\"action-dropdown\">\n                <button *ngFor=\"let action of custom; let i = index\" (click)=\"handle(action, model)\" type=\"button\" class=\"dropdown-item action-stub-btn\"> {{action.name}} </button>\n            </div>\n        </div>\n    </div>",
                    styles: ["action-stub-basic{padding:10px;border:1px solid #6441A5}action-stub-basic .actions{position:absolute;right:5px;top:0}action-stub-basic .actions>*{display:inline-block}action-stub-basic .fa.fa-question-circle-o{margin-left:5px}action-stub-items .actions{right:5px;top:0;position:relative}action-stub-items .actions>*{display:inline-block}action-stub-items .fa.fa-question-circle-o{margin-left:5px}.action-stub-btn{cursor:pointer}"],
                    encapsulation: core_1.ViewEncapsulation.None
                })], ActionStubItemsComponent);
                return ActionStubItemsComponent;
            }();
            exports_1("ActionStubItemsComponent", ActionStubItemsComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/action-stub.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, ActionStubComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            ActionStubComponent = /** @class */function () {
                function ActionStubComponent() {
                    this.isExpanded = false;
                }
                ActionStubComponent.prototype.ngOnInit = function () {};
                ActionStubComponent.prototype.onChange = function (index, model) {
                    var num = Number(index);
                    if (num >= 0) this.custom[num].onClick(model);
                };
                ActionStubComponent.prototype.onClick = function (model) {
                    if (this.click) this.click(model);
                };
                ActionStubComponent.prototype.onTextTouch = function (model) {
                    if (this.textTouch) this.textTouch(model);
                };
                ActionStubComponent.prototype.onImgTouch = function (model) {
                    if (this.imgTouch) this.imgTouch(model);
                };
                ActionStubComponent.prototype.onHover = function (model) {
                    this.isHovering = true;
                };
                ActionStubComponent.prototype.onLeave = function (model) {
                    this.isHovering = false;
                };
                ActionStubComponent.prototype.onExpand = function (model) {
                    this.isExpanded = !this.isExpanded;
                };
                __decorate([core_1.Input(), __metadata("design:type", String)], ActionStubComponent.prototype, "text", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ActionStubComponent.prototype, "subtext", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ActionStubComponent.prototype, "secondaryText", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubComponent.prototype, "textTouch", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ActionStubComponent.prototype, "img", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubComponent.prototype, "imgTouch", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubComponent.prototype, "delete", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubComponent.prototype, "edit", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubComponent.prototype, "add", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Array)], ActionStubComponent.prototype, "custom", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ActionStubComponent.prototype, "click", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ActionStubComponent.prototype, "id", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], ActionStubComponent.prototype, "model", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ActionStubComponent.prototype, "iconClassNames", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ActionStubComponent.prototype, "statusClassNames", void 0);
                ActionStubComponent = __decorate([core_1.Component({
                    selector: 'action-stub',
                    template: "<div [id]=\"id\" class=\"action-stub container\"       (click)=\"onClick(model)\"      (mouseenter)=\"onHover(model)\"      (mouseleave)=\"onLeave(model)\"                >     <div class=\"row no-gutter no-gutters justify-content-start\">         <div class=\"col-1\">                         <div class=\"fa as-collapse\"                      [class.opacity-zero]=\"contentWrapper.children.length==0\"                      [class.fa-chevron-right]=\"!isExpanded\"                      [class.fa-chevron-down]=\"isExpanded\"                      (click)=\"onExpand(model)\">             </div>                     </div>         <div class=\"col-9\">             <div class=\"row no-gutters justify-content-start\">                                 <div *ngIf=\"img\" class=\"col-1\"><img (click)=\"onImgTouch(model)\" src=\"{{img}}\" /></div>                 <div *ngIf=\"iconClassNames\" class=\"col-1 {{iconClassNames}}\"></div>                 <div class=\"primary-text-container col-sm\"                      [class.no-sub-text]=\"!subtext\">                     <p (click)=\"onTextTouch(model)\">{{text}} </p>                     <small *ngIf=\"subtext\">{{subtext}}</small>                 </div>                 <div class=\"col- secondaryText\" *ngIf=\"secondaryText\"><div >{{secondaryText}}</div></div>                 <div class=\"col-1\" *ngIf=\"statusClassNames\" ><i class=\"{{statusClassNames}}\"></i></div>                             </div>         </div>         <div class=\"col-2\">             <div class=\"row justify-content-end\" *ngIf=\"isHovering\">                 <div class=\"col-6\" *ngIf=\"custom\">                     <select class=\"custom-action\" (change)=\"onChange($event.target.value, model)\">                         <option>More...</option>                         <option *ngFor=\"let action of custom; let i = index\" value=\"{{i}}\">{{action.name}}</option>                     </select>                 </div>                 <div class=\"col-2 action-stub-btn\" *ngIf=\"delete\"><i class=\"fa fa-trash\" (click)=\"delete(model)\" data-toggle=\"tooltip\" data-placement=\"top\" title=\"Delete\"> </i></div>                 <div class=\"col-2 action-stub-btn\" *ngIf=\"edit\"><i class=\"fa fa-cog\" (click)=\"edit(model)\" data-toggle=\"tooltip\" data-placement=\"top\" title=\"Edit\"> </i></div>                 <div class=\"col-2 action-stub-btn\" *ngIf=\"add\"><i class=\"fa fa-plus\" (click)=\"add(model)\" data-toggle=\"tooltip\" data-placement=\"top\" title=\"Add\"> </i></div>                             </div>         </div>     </div> </div> <div #contentWrapper       [class.hidden-xl-down]=\"!isExpanded\"           >     <ng-content></ng-content> </div>",
                    styles: ["action-stub .action-stub{border-style:solid;border-color:#ccc;border-width:1px 1px 1px 1px;padding-top:6px;min-height:40px}action-stub .action-stub .primary-text-container{line-height:15px;cursor:pointer;text-align:left}action-stub .action-stub .primary-text-container p{margin-bottom:0;text-overflow:ellipsis;overflow:hidden}action-stub .action-stub .primary-text-container img{margin:0 10px}action-stub .action-stub .primary-text-container small{font-size:9px;color:#BBBBBB;text-overflow:ellipsis;overflow:hidden;display:block}action-stub .action-stub>div{padding-right:10px;margin-left:-10px}action-stub div action-stub .action-stub{border-width:0px 1px 1px 1px;padding-left:20px}action-stub div action-stub div action-stub .action-stub{padding-left:30px}action-stub div action-stub div action-stub div action-stub .action-stub{padding-left:40px}action-stub div action-stub div action-stub div action-stub div action-stub .action-stub{padding-left:50px}action-stub div action-stub div action-stub div action-stub div action-stub div action-stub .action-stub{padding-left:60px}action-stub div action-stub div action-stub div action-stub div action-stub div action-stub div action-stub .action-stub{padding-left:70px}action-stub+action-stub>.action-stub{border-style:solid;border-color:#ccc;border-width:0px 1px 1px 1px;min-height:40px}.no-sub-text{padding-top:6px}.as-collapse{cursor:pointer;padding-top:4px}.action-stub-btn{cursor:pointer}.custom-action{float:right;margin-right:10px;margin-top:-2px;width:75px}.opacity-zero{opacity:0}"],
                    encapsulation: core_1.ViewEncapsulation.None
                }), __metadata("design:paramtypes", [])], ActionStubComponent);
                return ActionStubComponent;
            }();
            exports_1("ActionStubComponent", ActionStubComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/dragable.component.js", ["@angular/core", "ng2-dragula/ng2-dragula"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, ng2_dragula_1, DragableComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (ng2_dragula_1_1) {
            ng2_dragula_1 = ng2_dragula_1_1;
        }],
        execute: function () {
            DragableComponent = /** @class */function () {
                function DragableComponent(dragulaService) {
                    this.dragulaService = dragulaService;
                    this.widthPc = "100";
                }
                DragableComponent.prototype.ngOnInit = function () {
                    var _this = this;
                    this.onDragSubscription = this.dragulaService.drag.subscribe(function (value) {
                        _this.onDrag(value.slice(1));
                    });
                    this.onDropSubscription = this.dragulaService.drop.subscribe(function (value) {
                        _this.onDrop(value.slice(1));
                    });
                    this.onOverSubscription = this.dragulaService.over.subscribe(function (value) {
                        _this.onOver(value.slice(1));
                    });
                    this.onOutSubscription = this.dragulaService.out.subscribe(function (value) {
                        _this.onOut(value.slice(1));
                    });
                };
                DragableComponent.prototype.ngOnDestroy = function () {
                    this.onDragSubscription.unsubscribe();
                    this.onDropSubscription.unsubscribe();
                    this.onOverSubscription.unsubscribe();
                    this.onOutSubscription.unsubscribe();
                };
                DragableComponent.prototype.hasClass = function (el, name) {
                    return new RegExp('(?:^|\\s+)' + name + '(?:\\s+|$)').test(el.className);
                };
                DragableComponent.prototype.addClass = function (el, name) {
                    if (!this.hasClass(el, name)) {
                        el.className = el.className ? [el.className, name].join(' ') : name;
                    }
                };
                DragableComponent.prototype.removeClass = function (el, name) {
                    if (this.hasClass(el, name)) {
                        el.className = el.className.replace(new RegExp('(?:^|\\s+)' + name + '(?:\\s+|$)', 'g'), '');
                    }
                };
                DragableComponent.prototype.onDrag = function (args) {
                    var element = args[0];
                    var targetContainer = args[1];
                    var sourceContainer = args[2];
                    this.removeClass((element && element.children.length) > 0 ? element.children[0] : element, 'ex-moved');
                    if (this.drag) this.drag(element, targetContainer, sourceContainer);
                };
                DragableComponent.prototype.onDrop = function (args) {
                    var element = args[0];
                    var targetContainer = args[1];
                    var sourceContainer = args[2];
                    this.addClass((element && element.children.length) > 0 ? element.children[0] : element, 'ex-moved');
                    if (this.drop) this.drop(element, targetContainer, sourceContainer);
                };
                DragableComponent.prototype.onOver = function (args) {
                    var element = args[0];
                    var targetContainer = args[1];
                    var sourceContainer = args[2];
                    this.addClass((element && element.children.length) > 0 ? element.children[0] : element, 'ex-over');
                    if (this.over) this.over(element, targetContainer, sourceContainer);
                };
                DragableComponent.prototype.onOut = function (args) {
                    var element = args[0];
                    var targetContainer = args[1];
                    var sourceContainer = args[2];
                    this.removeClass(targetContainer, 'ex-over');
                    if (this.out) this.out(element, targetContainer, sourceContainer);
                };
                __decorate([core_1.Input(), __metadata("design:type", String)], DragableComponent.prototype, "bagId", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], DragableComponent.prototype, "widthPc", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], DragableComponent.prototype, "heightPx", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], DragableComponent.prototype, "drop", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], DragableComponent.prototype, "over", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], DragableComponent.prototype, "drag", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], DragableComponent.prototype, "out", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], DragableComponent.prototype, "id", void 0);
                DragableComponent = __decorate([core_1.Component({
                    selector: 'dragable',
                    template: "\n        <div [id]=\"id\" class=\"\" [style.width.%]=\"widthPc\" [style.height.px]=\"heightPx\" style=\"overflow: auto; cursor: move;\" [dragula]=\"bagId\">\n            <ng-content></ng-content>\n        </div>\n    ",
                    styles: [".gu-mirror {   position: fixed !important;   margin: 0 !important;   z-index: 9999 !important;   opacity: 0.8;   -ms-filter: \"progid:DXImageTransform.Microsoft.Alpha(Opacity=80)\";   filter: alpha(opacity=80); } .gu-hide {   display: none !important; } .gu-unselectable {   -webkit-user-select: none !important;   -moz-user-select: none !important;   -ms-user-select: none !important;   user-select: none !important; } .gu-transit {   opacity: 0.2;   -ms-filter: \"progid:DXImageTransform.Microsoft.Alpha(Opacity=20)\";   filter: alpha(opacity=20); } .container{width:100%}.ex-moved{background-color:rgba(78,0,255,0.05);transition:background-color 500ms ease-out}.container.ex-over{background-color:rgba(78,0,255,0.05)}.container.ex-over>.action-stub{background-color:rgba(78,0,255,0.05)}"],
                    encapsulation: core_1.ViewEncapsulation.None
                }), __metadata("design:paramtypes", [ng2_dragula_1.DragulaService])], DragableComponent);
                return DragableComponent;
            }();
            exports_1("DragableComponent", DragableComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/pagination.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, PaginationComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            PaginationComponent = /** @class */function () {
                function PaginationComponent() {
                    this.startingPage = 1;
                    this.pageChanged = new core_1.EventEmitter();
                    this.pageFn = function (pageNum) {
                        this.currentPage = pageNum;
                        this.pageChanged.emit(this.currentPage);
                    };
                    this.range = function (value) {
                        var a = [];
                        for (var i = 0; i < value; ++i) {
                            a.push(i + 1);
                        }
                        return a;
                    };
                }
                PaginationComponent.prototype.ngOnInit = function () {
                    this.currentPage = this.startingPage;
                };
                PaginationComponent.prototype.reset = function () {
                    this.pageFn(1);
                };
                PaginationComponent.prototype.nextPage = function () {
                    this.pageChanged.emit(1);
                };
                PaginationComponent.prototype.prevPage = function () {
                    this.pageChanged.emit(-1);
                };
                __decorate([core_1.Input(), __metadata("design:type", String)], PaginationComponent.prototype, "type", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Number)], PaginationComponent.prototype, "pages", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], PaginationComponent.prototype, "showNext", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], PaginationComponent.prototype, "showPrevious", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Number)], PaginationComponent.prototype, "startingPage", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], PaginationComponent.prototype, "pageChanged", void 0);
                PaginationComponent = __decorate([core_1.Component({
                    selector: 'pagination',
                    template: "\n    <ng-container [ngSwitch]=\"type\">\n        <ng-container *ngSwitchCase=\"'Token'\">\n            <nav aria-label=\"Page navigation\">\n                <ul class=\"pagination\">\n                    <li class=\"page-item\">\n                        <a class=\"page-link\" (click)=\"prevPage(currentPageIndex)\" *ngIf=\"showPrevious\">\n                            Previous Page\n                        </a> \n                    </li>\n                    <li class=\"page-item\">\n                        <a class=\"page-link\" (click)=\"nextPage(currentPageIndex)\" *ngIf=\"showNext\">\n                            Next Page\n                        </a>\n                    </li>\n                </ul>\n            </nav>\n        </ng-container>\n        <ng-container *ngSwitchDefault>\n            <nav aria-label=\"Page navigation\">\n                <ul class=\"pagination\" *ngIf=\"pages > 1\">\n                    <li *ngFor=\"let index of range(pages)\" class=\"page-item\">\n                            <a class=\"page-link\" \n                                [class.page-active]=\"index == currentPage\"\n                                (click)=\"pageFn(index)\"> {{index}} </a>\n                    </li>\n                </ul>\n                <pre>{{'Total pages: ' + pages | devonly}}</pre>\n                <pre>{{'Current page index: ' + currentPage | devonly}}</pre>\n            </nav>\n        </ng-container>\n    </ng-container>\n    ",
                    styles: [".page-link{color:#6441A5}.page-link:focus,.page-link:hover{color:#6441A5}.page-active{color:#fff !important;background-color:#6441A5}.pagination{-ms-flex-wrap:wrap;flex-wrap:wrap}"],
                    encapsulation: core_1.ViewEncapsulation.None
                }), __metadata("design:paramtypes", [])], PaginationComponent);
                return PaginationComponent;
            }();
            exports_1("PaginationComponent", PaginationComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/search.component.js", ["@angular/core", "@angular/forms", "rxjs/add/operator/debounceTime"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, forms_1, SearchResult, SearchComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (forms_1_1) {
            forms_1 = forms_1_1;
        }, function (_1) {}],
        execute: function () {
            SearchResult = /** @class */function () {
                function SearchResult(id, value) {
                    this.id = id;
                    this.value = value;
                }
                return SearchResult;
            }();
            exports_1("SearchResult", SearchResult);
            SearchComponent = /** @class */function () {
                function SearchComponent() {
                    // Placeholder for the text input.  This placeholder should be relevant for all dropdown cases.
                    // If none is specified the input placeholder will be blank
                    this.searchInputPlaceholder = "";
                    // Output for when the search input is updated.
                    this.searchUpdated = new core_1.EventEmitter();
                    this.searchTextControl = new forms_1.FormControl();
                    this.changeDropdown = function (dropdownOption) {
                        var triggerSearch = false;
                        if (this.dropdownOption !== this.currentDropdownOption && this.searchTextControl.value === "") {
                            triggerSearch = true;
                        }
                        if (dropdownOption !== this.currentDropdownOption && this.searchTextControl.value) {
                            this.searchTextControl.setValue("");
                        }
                        this.currentDropdownOption = dropdownOption;
                        if (triggerSearch) this.search();
                    };
                }
                SearchComponent.prototype.ngOnInit = function () {
                    var _this = this;
                    this.searchTextControl.setValue("");
                    if (this.dropdownOptions) {
                        this.currentDropdownOption = this.dropdownOptions[0];
                    }
                    // search form control - debounced and emits after a delay
                    this._searchTextSubscription = this.searchTextControl.valueChanges.debounceTime(500).subscribe(function (newSearchText) {
                        _this.search();
                    });
                };
                SearchComponent.prototype.search = function () {
                    if (this.currentDropdownOption && this.currentDropdownOption.text) {
                        this.searchUpdated.emit(new SearchResult(this.currentDropdownOption.text, this.searchTextControl.value));
                    } else {
                        this.searchUpdated.emit(new SearchResult("", this.searchTextControl.value));
                    }
                    if (this.currentDropdownOption && this.currentDropdownOption.functionCb) {
                        if (this.currentDropdownOption.argsCb) {
                            this.currentDropdownOption.functionCb(this.currentDropdownOption.argsCb);
                        } else this.currentDropdownOption.functionCb(this.currentDropdownOption.text);
                    }
                };
                SearchComponent.prototype.ngOnDestroy = function () {
                    if (this._searchTextSubscription) {
                        this._searchTextSubscription.unsubscribe();
                    }
                };
                __decorate([core_1.Input(), __metadata("design:type", Array)], SearchComponent.prototype, "dropdownOptions", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], SearchComponent.prototype, "dropdownPlaceholderText", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], SearchComponent.prototype, "searchInputPlaceholder", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], SearchComponent.prototype, "searchUpdated", void 0);
                SearchComponent = __decorate([core_1.Component({
                    selector: 'search',
                    template: "\n        <form class=\"form-inline\">\n            <div class=\"form-group\">\n                <dropdown class=\"search-dropdown\" (dropdownChanged)=\"changeDropdown($event)\" [options]=\"dropdownOptions\" [placeholderText]=\"dropdownPlaceholderText\"> </dropdown>\n                <input (keyUp)=\"emitSearch($event)\" [formControl]=\"searchTextControl\" class=\"form-control search-text\" type=\"text\" [placeholder]=\"searchInputPlaceholder\" />\n            </div>\n        </form>\n\n    ",
                    styles: ["\n       .search-dropdown {\n           margin-right: 15px;\n       }\n       input.search-text {\n            width: 300px;\n       }\n    "]
                }), __metadata("design:paramtypes", [])], SearchComponent);
                return SearchComponent;
            }();
            exports_1("SearchComponent", SearchComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/directive/facet.directive.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, FacetDirective;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            FacetDirective = /** @class */function () {
                function FacetDirective(viewContainerRef) {
                    this.viewContainerRef = viewContainerRef;
                }
                FacetDirective = __decorate([core_1.Directive({
                    selector: '[facet-host]'
                }), __metadata("design:paramtypes", [core_1.ViewContainerRef])], FacetDirective);
                return FacetDirective;
            }();
            exports_1("FacetDirective", FacetDirective);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/facet/facet.component.js", ["@angular/core", "../../class/index", "../../directive/facet.directive", "app/shared/service/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, index_1, facet_directive_1, index_2, FacetComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (facet_directive_1_1) {
            facet_directive_1 = facet_directive_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }],
        execute: function () {
            FacetComponent = /** @class */function () {
                function FacetComponent(componentFactoryResolver, definitions, lymetrics) {
                    this.componentFactoryResolver = componentFactoryResolver;
                    this.definitions = definitions;
                    this.lymetrics = lymetrics;
                    this.tabClicked = new core_1.EventEmitter();
                    this.model = {
                        facets: {
                            qualified: new Array()
                        },
                        cloudGemFacetCount: 0,
                        activeFacetIndex: 0
                    };
                }
                FacetComponent.prototype.ngOnInit = function () {
                    if (!this.context) this.context = {};
                    this.context = Object.assign(this.context, this.definitions.defines);
                    // Default setting for subnav component
                    if (!this.facets) this.facets = [];
                    // Define all qualified facets for this Cloud Gem
                    this.defineQualifiedFacets();
                    this.model.cloudGemFacetCount = this.facets.length;
                    this.facets = this.facets.concat(this.model.facets.qualified.map(function (facet) {
                        return facet.title;
                    }));
                    this.emitActiveTab(0);
                };
                FacetComponent.prototype.emitActiveTab = function (index) {
                    this.sendEvent(index);
                    this.model.activeFacetIndex = index;
                    this.tabClicked.emit(index);
                    this.loadComponent(index - this.model.cloudGemFacetCount);
                };
                FacetComponent.prototype.defineQualifiedFacets = function () {
                    //iterate all defined facets
                    if (!this.disableInheritedFacets) {
                        for (var i = 0; i < index_1.FacetDefinitions.defined.length; i++) {
                            var facet = index_1.FacetDefinitions.defined[i];
                            var isApplicable = true;
                            for (var x = 0; x < facet.constraints.length; x++) {
                                var constraint = facet.constraints[x];
                                //determine if the defined facet qualifies for this cloud gem
                                if (!(constraint in this.context)) {
                                    isApplicable = false;
                                } else {
                                    isApplicable = true;
                                    facet.data[constraint] = this.context[constraint];
                                    facet.data["Identifier"] = this.metricIdentifier;
                                    break;
                                }
                            }
                            if (isApplicable) {
                                this.model.facets.qualified.push(facet);
                            }
                        }
                    }
                    //order the qualified facets by their priority index
                    this.model.facets.qualified.sort(function (facet1, facet2) {
                        if (facet1.order > facet2.order) return 1;
                        if (facet1.order < facet2.order) return -1;
                        return 0;
                    });
                };
                FacetComponent.prototype.loadComponent = function (index) {
                    var viewContainerRef = this.facetHost.viewContainerRef;
                    viewContainerRef.clear();
                    if (index >= this.model.facets.qualified.length || index < 0) {
                        return;
                    }
                    var facet = this.model.facets.qualified[index];
                    if (!facet.component) {
                        console.warn("Facet generator failed to find a component for this qualified facet.");
                        return;
                    }
                    var componentFactory = this.componentFactoryResolver.resolveComponentFactory(facet.component);
                    var componentRef = viewContainerRef.createComponent(componentFactory);
                    componentRef.instance.data = facet.data;
                };
                FacetComponent.prototype.sendEvent = function (index) {
                    this.lymetrics.recordEvent('FacetOpened', {
                        "Name": this.facets[index].toString(),
                        "Identifier": this.metricIdentifier
                    });
                };
                __decorate([core_1.Input(), __metadata("design:type", Object)], FacetComponent.prototype, "context", void 0);
                __decorate([core_1.Input('tabs'), __metadata("design:type", Array)], FacetComponent.prototype, "facets", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], FacetComponent.prototype, "metricIdentifier", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], FacetComponent.prototype, "disableInheritedFacets", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], FacetComponent.prototype, "tabClicked", void 0);
                __decorate([core_1.ViewChild(facet_directive_1.FacetDirective), __metadata("design:type", facet_directive_1.FacetDirective)], FacetComponent.prototype, "facetHost", void 0);
                FacetComponent = __decorate([core_1.Component({
                    selector: 'facet-generator',
                    template: "\n    <div class=\"row subNav\">\n        <div class=\"tab\"\n            [class.tab-active]=\"model.activeFacetIndex == i\"\n            *ngFor=\"let facet of facets; let i = index\">\n                <a (click)=\"emitActiveTab(i)\"> {{facets[i]}} </a>\n        </div>\n    </div>\n    <ng-template facet-host></ng-template>\n\n    ",
                    styles: [".subNav.row{border-bottom:2px solid #eee;margin-bottom:25px}.subNav.row .tab{cursor:pointer;text-align:center;padding:0 0 5px;margin-right:30px;position:relative;top:2px;color:#222}.subNav.row .tab-active{border-bottom:4px solid #6441A5;color:#6441A5}"]
                }), __metadata("design:paramtypes", [core_1.ComponentFactoryResolver, index_2.DefinitionService, index_2.LyMetricService])], FacetComponent);
                return FacetComponent;
            }();
            exports_1("FacetComponent", FacetComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/facet/rest-explorer.component.js", ["@angular/core", "app/aws/aws.service", "app/shared/class/index", "app/shared/service/index", "@angular/http"], function (exports_1, context_1) {
    "use strict";

    var __extends = this && this.__extends || function () {
        var extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
        };
        return function (d, b) {
            extendStatics(d, b);
            function __() {
                this.constructor = d;
            }
            d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
        };
    }();
    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, index_1, index_2, http_1, BodyTreeViewComponent, RestApiExplorerComponent, RestAPIExplorer;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }],
        execute: function () {
            BodyTreeViewComponent = /** @class */function () {
                function BodyTreeViewComponent() {}
                BodyTreeViewComponent.prototype.ngOnInit = function () {};
                BodyTreeViewComponent.prototype.placeholder = function (obj) {
                    if (obj.type.indexOf('array') >= 0 && obj.items) {
                        if (obj.items.properties) {
                            var keys = Object.keys(obj.items.properties);
                            var result = "[{";
                            for (var key in obj.items.properties) {
                                result += key + ":" + obj.items.properties[key].type + ",";
                            }
                            result = result.split(",").slice(0, keys.length).join(",");
                            result += "}]";
                            return keys ? result : null;
                        } else {
                            return obj.type + "[" + obj.items.type + "]";
                        }
                    } else {
                        return obj.type;
                    }
                };
                BodyTreeViewComponent.prototype.isObjectOrArray = function (type) {
                    if (!type) return false;
                    return type.match(/(array|object)/g) ? true : false;
                };
                BodyTreeViewComponent.prototype.isObjectToDisplay = function (obj) {
                    if (!obj) return false;
                    var result = obj.properties && Object.keys(obj.properties).length > 0 ? false : true;
                    return result;
                };
                __decorate([core_1.Input(), __metadata("design:type", Object)], BodyTreeViewComponent.prototype, "data", void 0);
                BodyTreeViewComponent = __decorate([core_1.Component({
                    selector: 'body-tree-view',
                    template: "\n                <div *ngIf=\"data['properties'] !== undefined\">\n                    <div *ngFor=\"let prop of data['properties'] | objKeys\">\n                        <div *ngIf=\"isObjectToDisplay(data['properties'][prop])\" class=\"row mb-3 no-gutters rest-prop\" [ngClass]=\"{'has-danger': data['properties'][prop].valid === false}\">\n                            <span class=\"col-lg-2\" affix> {{prop}} </span>                        \n                            <input *ngIf=\"!isObjectOrArray(data['properties'][prop].type)\" class=\"form-control\" type=\"{{data['properties'][prop].type}}\" [ngClass]=\"{'input-dotted' : data['required'] === undefined || data['required'].indexOf(prop) < 0, 'form-control-danger': data['properties'][prop].valid === false}\" \n                                [(ngModel)]=\"data['properties'][prop].value\" \n                                placeholder=\"{{placeholder(data['properties'][prop])}}\" />                            \n                            <textarea rows=\"10\" *ngIf=\"isObjectOrArray(data['properties'][prop].type)\" class=\"col-lg-8 form-control\" [ngClass]=\"{'input-dotted' : data['required'] === undefined || data['required'].indexOf(prop) < 0, 'form-control-danger': data['properties'][prop].valid === false}\" \n                                [(ngModel)]=\"data['properties'][prop].value\" \n                                placeholder=\"{{placeholder(data['properties'][prop])}}\">\n                            </textarea>\n                            <span class=\"ml-3\"><i *ngIf=\"data['properties'][prop].description\" class=\"fa fa-question-circle-o\" placement=\"bottom\" ngbTooltip=\"{{data['properties'][prop].description}}\"></i></span>\n                            <div *ngIf=\"data['properties'][prop].message\" class=\"form-control-feedback\">{{data['properties'][prop].message}}</div>\n                        </div>\n                        <body-tree-view [data]=\"data['properties'][prop]\"></body-tree-view>          \n                    </div>\n                </div>\n  "
                })], BodyTreeViewComponent);
                return BodyTreeViewComponent;
            }();
            exports_1("BodyTreeViewComponent", BodyTreeViewComponent);
            RestApiExplorerComponent = /** @class */function () {
                function RestApiExplorerComponent(aws, http, metric) {
                    this.aws = aws;
                    this.http = http;
                    this.metric = metric;
                    this.model = {
                        path: {
                            selected: undefined,
                            verbs: undefined,
                            verb: undefined,
                            verbValueValid: true,
                            value: undefined,
                            parameters: undefined
                        },
                        harmonized: {
                            paths: []
                        },
                        showSwagger: false,
                        swagger: undefined,
                        template: undefined,
                        response: [],
                        multiplier: 1
                    };
                    this.hasPathParams = false;
                    this.hasQueryParams = false;
                    this.functions = ["{rand}"];
                    this.FUNC_RAND_IDX = 0;
                    this.isRunningCommand = false;
                }
                RestApiExplorerComponent.prototype.ngOnInit = function () {
                    this.serviceUrl = this.data["ServiceUrl"];
                    var apiid = this.aws.parseAPIId(this.serviceUrl);
                    this.getSwagger(apiid, this.aws.context.apiGateway, this.model);
                    this.api = new RestAPIExplorer(this.serviceUrl, this.http, this.aws, this.metric, this.data["Identifier"]);
                };
                RestApiExplorerComponent.prototype.getSwagger = function (apiid, apiGateway, model) {
                    var params = {
                        exportType: 'swagger',
                        restApiId: apiid,
                        stageName: 'api',
                        accepts: 'application/json',
                        parameters: {
                            'extensions': 'integrations, authorizers, documentation, validators, gatewayresponses',
                            'Accept': 'application/json'
                        }
                    };
                    this.aws.context.apiGateway.getExport(params, function (err, data) {
                        if (err) {
                            console.log(err);
                            return;
                        }
                        model.swagger = JSON.parse(data.body);
                        var paths = Object.keys(model.swagger.paths);
                        for (var i = 0; i < paths.length; i++) {
                            var parts = paths[i].split("{");
                            var obj = {
                                original: paths[i],
                                harmonized: parts[0]
                            };
                            model.harmonized.paths.push(obj);
                        }
                    });
                };
                RestApiExplorerComponent.prototype.getBodyParameters = function (parameter) {
                    if (this.model.template) return this.model.template;
                    this.model.template = {};
                    this.defineBodyTemplate(parameter, this.model.template);
                    return this.model.template;
                };
                //recursively iterate all nested swaggger schema definitions
                RestApiExplorerComponent.prototype.defineBodyTemplate = function (parameter, master) {
                    if (typeof parameter != 'object') {
                        return undefined;
                    }
                    if (parameter['$ref']) {
                        var schema = parameter['$ref'];
                        var parts = schema.split('/');
                        var template = this.model.swagger;
                        for (var i = 1; i < parts.length; i++) {
                            var part = parts[i];
                            template = template[part];
                        }
                        if (template.type == 'array') {
                            parameter.type = 'array[' + template.items.type + ']';
                            return undefined;
                        }
                        return template;
                    }
                    for (var key in parameter) {
                        var value = parameter[key];
                        var result = this.defineBodyTemplate(value, master);
                        if (result !== undefined) {
                            if (key == 'schema') {
                                master = Object.assign(master, result);
                            } else {
                                parameter[key] = result;
                            }
                            this.defineBodyTemplate(result, master);
                        }
                    }
                    return undefined;
                };
                RestApiExplorerComponent.prototype.initializeVerbContext = function (verb) {
                    this.model.path.verbValueValid = true;
                    this.model.path.verb = verb;
                    this.model.path.parameters = this.model.path.verbs[verb].parameters ? this.model.path.verbs[verb].parameters : [];
                    this.model.template = undefined;
                    this.hasPathParams = false;
                    this.hasQueryParams = false;
                    for (var _i = 0, _a = this.model.path.parameters; _i < _a.length; _i++) {
                        var params = _a[_i];
                        if (params.in == "path") {
                            this.hasPathParams = true;
                        }
                        if (params.in == "query") {
                            this.hasQueryParams = true;
                        }
                    }
                };
                RestApiExplorerComponent.prototype.isValidRequest = function () {
                    var isValid = true;
                    if (this.model.path.verb === undefined) {
                        this.model.response.push("Select a VERB for this request");
                        this.model.path.verbValueValid = false;
                        return false;
                    }
                    for (var i = 0; i < this.model.path.parameters.length; i++) {
                        var param = this.model.path.parameters[i];
                        param.valid = true;
                        //validate GET parameters
                        if (param.in == 'query' && param.required && (param.value === null || param.value === undefined)) {
                            param.message = "The GET querystring parameter '" + param.name + "' is a required field.";
                            isValid = param.valid = false;
                            //validate PATH parameters
                        } else if (param.in == 'path' && param.required && (param.value === null || param.value === undefined)) {
                            param.message = "The PATH parameter '" + param.name + "' is a required field.";
                            isValid = param.valid = false;
                            //Validate POST parameters
                        } else if (param.in == 'body') {
                            if (param.required && this.model.template['required'] && (param.value === null || param.value === undefined)) {
                                isValid = param.valid = false;
                                param.message = "The BODY parameter '" + param.name + "' is a required field";
                            }
                            if (this.model.template && this.model.template['properties']) {
                                for (var key in this.model.template['properties']) {
                                    var requiredproperty = this.model.template['required'] ? this.model.template['required'].filter(function (e) {
                                        return e == key;
                                    }).length > 0 : false;
                                    var prop = this.model.template['properties'][key];
                                    prop.message = undefined;
                                    if (prop.type == 'integer') {
                                        isValid = prop.valid = requiredproperty ? prop.value !== undefined && prop.value.length > 0 && !isNaN(Number.parseInt(prop.value)) : prop.value !== undefined ? prop.value.length > 0 && !isNaN(Number.parseInt(prop.value)) : true;
                                        if (!isValid) prop.message = "The POST body parameter '" + key + "' is a " + (requiredproperty ? "required " : "") + "integer field.";
                                    } else if (prop.type && (prop.type == 'object' || prop.type.indexOf('array') >= 0)) {
                                        var matches = prop.value ? prop.value.match(/[^\"\']{rand}[^\"\']/g) : [];
                                        if (matches) {
                                            for (var i_1 = 0; i_1 < matches.length; i_1++) {
                                                var match = matches[i_1];
                                                var first_char = match.substring(0, 1);
                                                var last_char = match.substring(match.length - 1, match.length);
                                                var new_str = first_char + '"' + this.functions[this.FUNC_RAND_IDX] + '"' + last_char;
                                                prop.value = prop.value.split(match).join(new_str);
                                            }
                                        }
                                        try {
                                            JSON.parse(prop.value);
                                            isValid = prop.valid = true;
                                        } catch (e) {
                                            isValid = prop.valid = false;
                                            prop.message = "The POST body parameter '" + key + "' is a " + (requiredproperty ? "required " : "") + "object field.  " + e.message;
                                        }
                                    } else {
                                        isValid = prop.valid = requiredproperty ? prop.value !== undefined && prop.value.length > 0 : true;
                                        if (!isValid) prop.message = "The POST body parameter '" + key + "' is a  " + (requiredproperty ? "required " : "") + "string field.";
                                    }
                                }
                            }
                        }
                        if (param.message) console.log(param.message);
                    }
                    return isValid;
                };
                RestApiExplorerComponent.prototype.initializePathContext = function (path) {
                    this.model.path.value = path.original;
                    this.model.path.selected = path.original;
                    this.model.path.verbs = this.deleteOptions(this.model.swagger.paths[path.original]);
                    this.model.path.parameters = [];
                    this.model.path.verb = undefined;
                };
                RestApiExplorerComponent.prototype.hasBody = function (parameters) {
                    if (parameters === undefined) return false;
                    for (var i = 0; i < parameters.length; i++) {
                        if (parameters[i].in == 'body') {
                            return true;
                        }
                    }
                    return false;
                };
                RestApiExplorerComponent.prototype.deleteOptions = function (object) {
                    delete object.options;
                    return object;
                };
                RestApiExplorerComponent.prototype.send = function () {
                    this.model.response = [];
                    if (!this.isValidRequest()) return;
                    var parameters = this.getParameters();
                    for (var i = 0; i < this.model.multiplier; i++) {
                        this.execute(this.addRandomCharacters(parameters.path, 5), this.addRandomCharacters(parameters.querystring, 5), JSON.parse(this.addRandomCharacters(JSON.stringify(parameters.body), 5)));
                    }
                };
                RestApiExplorerComponent.prototype.getParameters = function () {
                    var path = this.model.path.selected.substring(1);
                    var querystring = "";
                    var body = {};
                    for (var i = this.model.path.parameters.length - 1; i >= 0; i--) {
                        var param = this.model.path.parameters[i];
                        if (param.in == 'query' && this.isValidValue(param.value)) {
                            var separator = querystring.indexOf('?') == -1 ? '?' : '&';
                            querystring += separator + param.name + "=" + param.value.trim();
                        } else if (param.in == 'body') {
                            body = this.defineBodyParameters(this.model.template);
                        } else if (param.in == 'path' && this.isValidValue(param.value)) {
                            path = path.replace("{" + param.name + "}", param.value.trim());
                            path += '/';
                        }
                    }
                    return {
                        'path': path.charAt(path.length - 1) == '/' ? path.substr(0, path.length - 1) : path,
                        'querystring': querystring,
                        'body': body
                    };
                };
                RestApiExplorerComponent.prototype.isValidValue = function (value) {
                    return value !== undefined && value !== null && value.trim() !== "";
                };
                RestApiExplorerComponent.prototype.defineBodyParameters = function (obj) {
                    var nested = {};
                    if (obj) {
                        for (var key in obj['properties']) {
                            var entry = obj['properties'][key];
                            if (entry.value) {
                                if (entry.type == 'integer' || entry.type == 'number') {
                                    nested[key] = Number.parseInt(entry.value);
                                } else if (entry.type && (entry.type == 'object' || entry.type.indexOf('array') >= 0)) {
                                    nested[key] = JSON.parse(entry.value);
                                } else {
                                    nested[key] = entry.value.trim();
                                }
                            } else if (entry.properties) {
                                nested[key] = this.defineBodyParameters(entry);
                            } else if (entry.items) {
                                nested[key] = this.defineBodyParameters(entry.items);
                            }
                        }
                    }
                    return nested;
                };
                RestApiExplorerComponent.prototype.execute = function (path, querystring, body) {
                    var _this = this;
                    if (this.model.path.verb == 'get') {
                        this.api.get(path + querystring).subscribe(function (response) {
                            _this.addToResponse(_this.runView(_this.requestParameters(path, querystring, body), JSON.parse(response.body.text())));
                        }, function (err) {
                            _this.addToResponse(err.message);
                        });
                    } else if (this.model.path.verb == 'post') {
                        this.api.post(path + "/" + querystring, body).subscribe(function (response) {
                            _this.addToResponse(_this.runView(_this.requestParameters(path, querystring, body), JSON.parse(response.body.text())));
                        }, function (err) {
                            _this.addToResponse(err.message);
                        });
                    } else if (this.model.path.verb == 'put') {
                        this.api.put(path + "/" + querystring, body).subscribe(function (response) {
                            _this.addToResponse(_this.runView(_this.requestParameters(path, querystring, body), JSON.parse(response.body.text())));
                        }, function (err) {
                            _this.addToResponse(err.message);
                        });
                    } else if (this.model.path.verb == 'delete') {
                        this.api.delete(path + "/" + querystring).subscribe(function (response) {
                            _this.addToResponse(_this.runView(_this.requestParameters(path, querystring, body), JSON.parse(response.body.text())));
                        }, function (err) {
                            _this.addToResponse(err.message);
                        });
                    }
                    this.isRunningCommand = true;
                };
                RestApiExplorerComponent.prototype.addToResponse = function (param) {
                    this.model.response.push(param);
                    this.isRunningCommand = false;
                };
                RestApiExplorerComponent.prototype.runView = function (request, response) {
                    return {
                        request: request,
                        response: response
                    };
                };
                RestApiExplorerComponent.prototype.requestParameters = function (path, querystring, body) {
                    return {
                        path: path,
                        querystring: querystring,
                        body: body
                    };
                };
                RestApiExplorerComponent.prototype.openAWSConsole = function (host) {
                    var parts = host.split('.');
                    window.open(encodeURI("https://console.aws.amazon.com/apigateway/home?region=" + this.aws.context.region + "#/apis/" + parts[0] + "/resources/"), 'awsConsoleAPIGatewayViewer');
                };
                /**
                 * Add random characters by string replacing {rand} with random alphanumerical characters.  An example of this is to quickly create multiple random accounts.
                 * @param queryString entire query string
                 * @param numberOfCharsToAdd number of characters to add to the string
                 */
                RestApiExplorerComponent.prototype.addRandomCharacters = function (value, numberOfCharsToAdd) {
                    if (value) {
                        var matches = value.match(/\{rand\}/g);
                        if (matches) {
                            for (var i = 0; i < matches.length; i++) {
                                var randChars = Math.random().toString(36).substr(2, numberOfCharsToAdd);
                                var match = matches[i];
                                value = value.replace(/\{rand\}/, randChars);
                            }
                            return value;
                        }
                    }
                    return value;
                };
                __decorate([core_1.Input(), __metadata("design:type", Object)], RestApiExplorerComponent.prototype, "data", void 0);
                RestApiExplorerComponent = __decorate([core_1.Component({
                    selector: 'facet-rest-api-explorer',
                    template: "<div class=\"container rest-container\" *ngIf=\"model !== undefined && model.swagger\">     <div class=\"row mb-3\">         <label class=\"col-md-2\"> Host </label>         <span class=\"col-md-10\"> {{model.swagger.host}} <a><i class=\"fa fa-external-link\" (click)=\"openAWSConsole(model.swagger.host)\"></i></a> </span>     </div>         <div class=\"row mb-3\">         <label class=\"col-md-2\"> Title </label>         <span class=\"col-md-10\"> {{model.swagger.info.title}} </span>     </div>     <div class=\"row mb-3\">         <label class=\"col-md-2\"> Version </label>         <span class=\"col-md-10\"> {{model.swagger.info.version}} </span>     </div>     <div class=\"row mb-3\">         <label class=\"col-md-2\"> Schemes </label>         <span class=\"col-md-10\"> {{model.swagger.schemes.join(',')}} </span>     </div>       <div class=\"row mb-3\">         <label class=\"col-md-2\"> Path </label>         <div class=\"d-inline-block\"  ngbDropdown>             <button type=\"button\" class=\"btn l-dropdown\" id=\"path-dropdown\" ngbDropdownToggle>                 <span> {{model.path.selected ? model.path.selected : 'Select'}} </span>                 <i class=\"fa fa-caret-down\" aria-hidden=\"true\"></i>             </button>             <div class=\"dropdown-menu dropdown-menu-center\" aria-labelledby=\"path-dropdown\">                 <button *ngFor=\"let path of model.harmonized.paths\" (click)=\"initializePathContext(path)\" type=\"button\" class=\"dropdown-item\"> {{path.original}} </button>             </div>                     </div>             </div>     <div class=\"row mb-3\" *ngIf=\"model.path.verbs\">         <label class=\"col-md-2\"> Verb </label>         <div class=\"row no-gutters\">             <div class=\"row col-md-12\">                 <div class=\"d-inline-block\" *ngIf=\"model.path.verbs\" [ngClass]=\"{'form-control-danger': !model.path.verbValueValid}\" ngbDropdown>                     <button type=\"button\" class=\"btn l-dropdown\" id=\"verb-dropdown\" ngbDropdownToggle>                         <span> {{model.path.verb ? model.path.verb : 'Select'}} </span>                         <i class=\"fa fa-caret-down\" aria-hidden=\"true\"></i>                     </button>                     <div class=\"dropdown-menu dropdown-menu-right\" aria-labelledby=\"verb-dropdown\">                         <button *ngFor=\"let verb of model.path.verbs | objKeys\" (click)=\"initializeVerbContext(verb)\" type=\"button\" class=\"dropdown-item\"> {{verb}} </button>                     </div>                 </div>             </div>             <div class=\"row col-md-12\">                 <p class=\"ml-3 mt-2\" *ngIf=\"model.path.verb && model.path.verbs && model.path.verbs[model.path.verb]\"><i>{{model.path.verbs[model.path.verb].description}}</i></p>             </div>         </div>     </div>     <div class=\"show-swagger\" *ngIf=\"model.path.verb\">         <button type=\"button\" (click)=\"model.showRawVerb= !model.showRawVerb\" class=\"btn btn-outline-primary\">{{ !model.showRawVerb ? \"Show Path Swagger\" : \"Hide Path Swagger\"}}</button>     </div>     <div *ngIf=\"model.showRawVerb\" class=\"response-area\">         <h2>Path Swagger</h2>         <pre>{{ model.path.verbs[model.path.verb] | json}}</pre>     </div>      <div class=\"row mb-3\" *ngIf=\"model.path.parameters && model.path.parameters.length > 0\">         <label class=\"col-lg-2\"> Parameters </label>         <div class=\"col-lg-10\">                         <label *ngIf=\"hasPathParams\"> Path </label>             <div *ngFor=\"let parameter of model.path.parameters\" [ngClass]=\"{'has-danger': parameter.valid === false}\">                                                 <div *ngIf=\"parameter.in == 'path'\">                                         <div class=\"row mb-3 no-gutters rest-prop\">                         <span class=\"col-lg-2\"> {{parameter.name}}</span>                         <input class=\"form-control\" type=\"{{parameter.type}}\" [(ngModel)]=\"parameter.value\" [ngClass]=\"{'input-dotted' : parameter.required == false, 'form-control-danger': parameter.valid === false}\" placeholder=\"{{parameter.type}}\" />                         <span class=\"ml-3\"><i *ngIf=\"parameter.description\" class=\"fa fa-question-circle-o\" placement=\"bottom\" ngbTooltip=\"{{parameter.description}}\"></i></span>                         <div *ngIf=\"parameter.message\" class=\"form-control-feedback\">{{parameter.message}}</div>                     </div>                 </div>             </div>             <label *ngIf=\"hasQueryParams\"> Query </label>             <div *ngFor=\"let parameter of model.path.parameters\" [ngClass]=\"{'has-danger': parameter.valid === false}\">                                                 <div *ngIf=\"parameter.in == 'query'\">                                         <div class=\"row mb-3 no-gutters rest-prop\">                         <span class=\"col-lg-2\"> {{parameter.name}}</span>                         <input class=\"form-control\" type=\"{{parameter.type}}\" [(ngModel)]=\"parameter.value\" [ngClass]=\"{'input-dotted' : parameter.required == false, 'form-control-danger': parameter.valid === false}\" placeholder=\"{{parameter.type}}\" />                         <span class=\"ml-3\"><i *ngIf=\"parameter.description\" class=\"fa fa-question-circle-o\" placement=\"bottom\" ngbTooltip=\"{{parameter.description}}\"></i></span>                         <div *ngIf=\"parameter.message\" class=\"form-control-feedback\">{{parameter.message}}</div>                     </div>                 </div>             </div>             <div *ngFor=\"let parameter of model.path.parameters\">                                 <div *ngIf=\"parameter.in == 'body'\">                                         <label> Body </label>                     <body-tree-view [data]=\"getBodyParameters(parameter, model.swagger.definitions)\"></body-tree-view>                 </div>             </div>         </div>     </div>     <div *ngIf=\"model.path.selected && model.path.verb && model.path.verb.toUpperCase() != 'GET'\" class=\"row mb-3 rest-multiplier\">         <span> Run {{model.path.selected}} </span>         <input class=\"col-md-2 form-control\" [(ngModel)]=\"model.multiplier\" type=\"number\" />          <span> times </span>     </div>     <div class=\"show-swagger\">         <button type=\"button\" (click)=\"model.showSwagger= !model.showSwagger\" class=\"btn btn-outline-primary\">{{ !model.showSwagger ? \"Show Swagger\" : \"Hide Swagger\"}}</button>     </div>     <div class=\"show-swagger\">         <button type=\"button\" (click)=\"send()\" class=\"btn l-primary\">Send</button>     </div>     <div [ngSwitch]=\"isRunningCommand\">         <div *ngSwitchCase=\"true\">             <loading-spinner> </loading-spinner>         </div>         <div *ngSwitchCase=\"false\">             <div class=\"row mb-3\" *ngIf=\"model.response.length > 0\">                 <div class=\"col-12 response-area\" *ngFor=\"let result of model.response; let i = index\" [attr.data-index]=\"i\">                     <h2>Run {{i + 1}} </h2>                     <pre>{{result | json }}</pre>                 </div>             </div>         </div>         </div>     <div *ngIf=\"model.showSwagger\" class=\"row mb-3 response-area\">                 <h2>Swagger</h2>         <pre>{{model.swagger | json }}</pre>     </div>         </div>",
                    styles: [".vertical-input-margin{margin-top:5px;margin-bottom:5px}.input-dotted{border-style:dashed;border-color:#ccc}.response-area{background-color:#f8f8f8;margin:15px;padding:15px;max-height:400px;overflow:auto}textarea{padding-left:8px !important}.rest-container{margin-left:0}.rest-container label{margin-top:5px}.rest-container .d-inline-block{margin-left:15px;position:relative}.rest-container .row.rest-prop span{max-width:200px;margin-right:15px;display:-ms-flexbox;display:flex;-ms-flex-align:center;align-items:center}.rest-container .row.rest-prop input{max-width:600px}.rest-container .show-swagger button{margin-left:15px;margin-bottom:15px}.rest-container .query-param>label:not(:first-child){display:none}.rest-container .rest-multiplier>input{margin:0 15px}.rest-container .rest-multiplier>span{margin-top:5px;margin-left:15px}"],
                    encapsulation: core_1.ViewEncapsulation.None
                }), __metadata("design:paramtypes", [aws_service_1.AwsService, http_1.Http, index_2.LyMetricService])], RestApiExplorerComponent);
                return RestApiExplorerComponent;
            }();
            exports_1("RestApiExplorerComponent", RestApiExplorerComponent);
            RestAPIExplorer = /** @class */function (_super) {
                __extends(RestAPIExplorer, _super);
                function RestAPIExplorer(serviceBaseURL, http, aws, metrics, metricIdentifier) {
                    return _super.call(this, serviceBaseURL, http, aws, metrics, metricIdentifier) || this;
                }
                return RestAPIExplorer;
            }(index_1.ApiHandler);
            exports_1("RestAPIExplorer", RestAPIExplorer);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/facet/cloud-watch-logs.component.js", ["@angular/core", "app/aws/aws.service", "@angular/http", "app/aws/schema.class"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, http_1, schema_class_1, CloudWatchLogComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (schema_class_1_1) {
            schema_class_1 = schema_class_1_1;
        }],
        execute: function () {
            CloudWatchLogComponent = /** @class */function () {
                function CloudWatchLogComponent(aws, http) {
                    this.aws = aws;
                    this.http = http;
                    this.model = {
                        groups: [],
                        groupNames: [],
                        err: undefined,
                        loading: false
                    };
                }
                CloudWatchLogComponent.prototype.ngOnInit = function () {
                    this.getCloudWatchLogByStackId = this.getCloudWatchLogByStackId.bind(this);
                    this.getCloudWatchLogByLogId = this.getCloudWatchLogByLogId.bind(this);
                    if (this.data["physicalResourceId"]) this.getCloudWatchLogByStackId(this.data["physicalResourceId"], this.aws.context, this.model);else this.getCloudWatchLogByLogId(this.data["cloudwatchPhysicalId"], this.aws.context, this.model);
                };
                CloudWatchLogComponent.prototype.openAwsConsoleLog = function (groupName, stream) {
                    var url = "https://console.aws.amazon.com/cloudwatch/home?region=" + this.aws.context.region + "#logEventViewer:group=" + groupName;
                    if (stream) url += ";stream=" + stream;
                    window.open(encodeURI(url), 'awsConsoleLogViewer');
                };
                CloudWatchLogComponent.prototype.getCloudWatchLogByLogId = function (logId, context, model) {
                    this.model.loading = true;
                    var now = new Date();
                    now.setMinutes(-20);
                    var startingTime = now.getTime();
                    var params = {
                        logGroupName: '/aws/lambda/' + logId,
                        interleaved: true,
                        limit: 500,
                        startTime: startingTime
                    };
                    context.cloudWatchLogs.filterLogEvents(params, function (err, data) {
                        model.loading = false;
                        if (err) {
                            model.err = "No CloudWatch logs found.";
                        } else {
                            model.groupNames.push(params.logGroupName);
                            model.groups.push({
                                name: params.logGroupName,
                                events: data.events.reverse()
                            });
                        }
                    });
                };
                CloudWatchLogComponent.prototype.getCloudWatchLogByStackId = function (stackId, context, model) {
                    var _this = this;
                    var params = {
                        StackName: stackId
                    };
                    this.model.loading = true;
                    context.cloudFormation.describeStackResources(params, function (err, data) {
                        if (err) {
                            model.loading = false;
                            model.err = err.message;
                        } else {
                            for (var i = 0; i < data.StackResources.length; i++) {
                                var spec = data.StackResources[i];
                                if (spec[schema_class_1.Schema.CloudFormation.RESOURCE_TYPE.NAME] == schema_class_1.Schema.CloudFormation.RESOURCE_TYPE.TYPES.LAMBDA) {
                                    _this.getCloudWatchLogByLogId(spec.PhysicalResourceId, context, model);
                                }
                            }
                        }
                    });
                };
                CloudWatchLogComponent.prototype.goToLog = function (fragment) {
                    window.location.hash = 'blank';
                    window.location.hash = fragment;
                };
                __decorate([core_1.Input(), __metadata("design:type", Object)], CloudWatchLogComponent.prototype, "data", void 0);
                CloudWatchLogComponent = __decorate([core_1.Component({
                    selector: 'facet-cloud-watch-logs',
                    template: "\n            <div *ngIf=\"model.groups.length == 0 && model.loading\">\n                <loading-spinner></loading-spinner>\n            </div>\n            <div *ngIf=\"model.err && model.groups.length == 0\">\n                {{model.err}}\n            </div>\n            <div *ngIf=\"model.groups.length > 0\">\n                <div id=\"top\" col=\"row\">\n                    <h4>**Logs can be delayed a few minutes.  Displaying the last 20 minutes of CloudWatch Logs.</h4>\n                </div>\n                <div class=\"row table-of-contents\" *ngIf=\"model.groupNames.length>1\">\n                    <div class=\"col-12\">\n                        <div class=\"d-inline-block dropdown-outer\"  ngbDropdown>\n                            <button type=\"button\" class=\"btn l-dropdown\" id=\"path-dropdown\" ngbDropdownToggle>\n                                <span class=\"dropdown-inner\"> Cloud Watch Groups </span>\n                                <i class=\"fa fa-caret-down\" aria-hidden=\"true\"></i>\n                            </button>\n                            <div class=\"dropdown-menu dropdown-menu-center\" aria-labelledby=\"path-dropdown\">\n                                <button *ngFor=\"let group of model.groupNames; let i = index\" (click)=\"goToLog(group)\" type=\"button\" class=\"dropdown-item\"> {{group}} </button>\n                            </div>\n                        </div>\n                    </div>\n                </div>\n                <div col=\"row\">\n                    <div *ngFor=\"let group of model.groups\">\n                        <div id=\"{{group.name}}\" class=\"row\">\n                            <div class=\"col-1\"><h2>Log Group</h2></div>\n                            <div class=\"col-11\"><p class=\"text-info\">{{group.name}} (<a (click)=\"goToLog('top')\"> Top </a>) <a><i class=\"fa fa-external-link\" (click)=\"openAwsConsoleLog(group.name)\"></i></a></p></div>\n                        </div>\n                        <table class=\"table table-hover\">\n                            <thead>\n                                <tr>\n                                    <th width=\"10%\"> START </th>\n                                    <th width=\"60%\"> MESSAGE </th>\n                                    <th width=\"20%\"> LOG STREAM </th>\n                                    <th width=\"10%\"> TIMESTAMP </th>\n                                </tr>\n                            </thead>\n                            <tbody>\n                                <tr *ngFor=\"let entry of group.events\" (click)=\"openAwsConsoleLog(group.name, entry.logStreamName)\">\n                                    <td >  {{entry.ingestionTime | toDateFromEpochInMilli | date:'medium'}} </td>\n                                    <td >  {{entry.message}} </td>\n                                    <td >  {{entry.logStreamName}} </td>\n                                    <td >  {{entry.timestamp | toDateFromEpochInMilli | date:'medium'}} </td>\n                                </tr>\n                            </tbody>\n                        </table>\n                    </div>\n                </div>\n            </div>\n    ",
                    styles: ["\n       .table-of-contents {\n            margin-bottom: 20px;\n        }\n        .dropdown-inner {\n            width: 360px;\n            display: inline-flex;\n        }\n        .dropdown-outer {\n            width: 400px;\n        }\n        "]
                }), __metadata("design:paramtypes", [aws_service_1.AwsService, http_1.Http])], CloudWatchLogComponent);
                return CloudWatchLogComponent;
            }();
            exports_1("CloudWatchLogComponent", CloudWatchLogComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/facet/metric.component.js", ["@angular/core", "app/aws/aws.service", "@angular/http"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, http_1, MetricComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }],
        execute: function () {
            MetricComponent = /** @class */function () {
                function MetricComponent(aws, http) {
                    this.aws = aws;
                    this.http = http;
                    this.noYAxisLabels = true;
                    this.bar1 = [{ label: "<5", value: 1203 }, { label: "5-13", value: 50 }, { label: "14-17", value: 123 }, { label: "18-24", value: 119 }, { label: "25-44", value: 5 }, { label: "45-64", value: 785 }, { label: "≥65", value: 964 }];
                    this.bar2 = [{ label: "<5", value: 1233, color: "#98abc5" }, { label: "5-13", value: 50, color: "#8a89a6" }, { label: "14-17", value: 123, color: "#7b6888" }, { label: "18-24", value: 119, color: "#6b486b" }, { label: "25-44", value: 5, color: "#a05d56" }, { label: "45-64", value: 785, color: "#d0743c" }, { label: "≥65", value: 964, color: "#ff8c00" }];
                    this.pie1 = [{ label: "<5", value: 2704659 }, { label: "5-13", value: 4499890 }, { label: "14-17", value: 2159981 }, { label: "18-24", value: 3853788 }, { label: "25-44", value: 14106543 }, { label: "45-64", value: 8819342 }, { label: "≥65", value: 612463 }];
                    this.pie2 = [{ label: "<5", value: 2704659, color: "#98abc5" }, { label: "5-13", value: 4499890, color: "#8a89a6" }, { label: "14-17", value: 2159981, color: "#7b6888" }, { label: "18-24", value: 3853788, color: "#6b486b" }, { label: "25-44", value: 14106543, color: "#a05d56" }, { label: "45-64", value: 8819342, color: "#d0743c" }, { label: "≥65", value: 612463, color: "#ff8c00" }];
                    this.model = {};
                }
                __decorate([core_1.Input(), __metadata("design:type", Object)], MetricComponent.prototype, "data", void 0);
                MetricComponent = __decorate([core_1.Component({
                    selector: 'facet-metric',
                    template: "                   \n           <div class=\"row\"><barchart [data]=\"bar1\" [title]=\"'My Test Bar Chart'\" [x-axis-label]=\"'ages'\" [y-axis-label]=\"'Number of People'\" [width]=\"400\" [height]=\"300\"></barchart></div>\n           <div class=\"row\"><piechart [data]=\"bar1\" [title]=\"'My Test Bar Chart'\" [width]=\"400\" [height]=\"300\"></piechart></div>\n\n           <div class=\"row\"><barchart [data]=\"bar2\" [title]=\"'My Test Bar Chart'\" [x-axis-label]=\"'ages'\" [y-axis-label]=\"'Number of People'\" [width]=\"900\" [height]=\"600\"></barchart></div>\n           <div class=\"row\"><piechart [data]=\"pie2\" [title]=\"'My Test Bar Chart'\" [width]=\"600\" [height]=\"600\"></piechart></div>\n\n           <div class=\"row\"><barchart [data]=\"bar2\" [no-yaxis-labels]=\"noYAxisLabels\" [width]=\"150\" [height]=\"100\"></barchart></div>\n           <div class=\"row\"><piechart [data]=\"pie2\" [width]=\"150\" [height]=\"150\"></piechart></div>\n    ",
                    styles: [""]
                }), __metadata("design:paramtypes", [aws_service_1.AwsService, http_1.Http])], MetricComponent);
                return MetricComponent;
            }();
            exports_1("MetricComponent", MetricComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/thumbnail.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, ThumbnailComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            ThumbnailComponent = /** @class */function () {
                function ThumbnailComponent() {}
                ThumbnailComponent.prototype.ngOnInit = function () {};
                __decorate([core_1.Input(), __metadata("design:type", String)], ThumbnailComponent.prototype, "icon", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ThumbnailComponent.prototype, "url", void 0);
                ThumbnailComponent = __decorate([core_1.Component({
                    selector: 'thumbnail',
                    template: "\n    <a [routerLink]=\"url\" class=\"card gem-thumbnail\">\n        <div class=\"thumbnail-image\">\n            <img [src]=\"icon\" />\n        </div>\n        <div class=\"card-block\">\n            <ng-content select=\".thumbnail-label-section\"> </ng-content>\n            <div class=\"label-expanded\">\n                <ng-content select=\".expanded-section\"></ng-content>\n            </div>\n        </div>\n    </a>\n    ",
                    styles: [".card.gem-thumbnail{cursor:pointer;width:330px;display:inline-block;margin-right:15px;margin-bottom:15px;vertical-align:top;height:220px;position:relative;background-color:#f8f8f8;border:1px solid #ccc}.card.gem-thumbnail .thumbnail-image{height:180px;width:220px;margin:0 auto;text-align:center}.card.gem-thumbnail .card-block{position:absolute;bottom:0;background-color:black;opacity:0.85;color:#fff;width:100%;padding:0.5rem}.card.gem-thumbnail .card-block *{color:#fff}.card.gem-thumbnail .label-expanded{height:0;overflow:hidden;transition:height .250s ease-out;overflow:hidden}.card.gem-thumbnail .expanded-section *{font-size:12px}.card.gem-thumbnail:hover .label-expanded{height:20px;transition:height .250s ease-in}"],
                    encapsulation: core_1.ViewEncapsulation.None
                }), __metadata("design:paramtypes", [])], ThumbnailComponent);
                return ThumbnailComponent;
            }();
            exports_1("ThumbnailComponent", ThumbnailComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/graph.component.js", ["@angular/core", "app/view/game/module/shared/class/index", "ajv"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, index_1, ajv_1, GraphComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (ajv_1_1) {
            ajv_1 = ajv_1_1;
        }],
        execute: function () {
            GraphComponent = /** @class */function () {
                function GraphComponent(componentFactoryResolver) {
                    var _this = this;
                    this.componentFactoryResolver = componentFactoryResolver;
                    this._ajv = new ajv_1.default({ allErrors: true });
                    this._series = {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "name": { "type": 'string' },
                                "series": {
                                    "type": "array",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "name": { "type": "string", "default": "NA" },
                                            "value": { "type": "number", "default": 0 }
                                        },
                                        "additionalProperties": false,
                                        "required": ["name", "value"]
                                    }
                                }
                            },
                            "required": ["name", "series"]
                        }
                    };
                    this._pair = {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "name": { "type": 'string', "default": "NA" },
                                "value": { "type": 'number', "default": 0 }
                            },
                            "additionalProperties": false,
                            "required": ["name", "value"]
                        }
                    };
                    this.chartSchemas = new Map([["ngx-charts-line-chart", new ajv_1.default().compile(this._series)], ["ngx-charts-area-chart-stacked", new ajv_1.default().compile(this._series)], ["ngx-charts-bar-vertical", new ajv_1.default().compile(this._pair)], ["ngx-charts-gauge", new ajv_1.default().compile(this._pair)]]);
                    this.errorMessage = undefined;
                    this.isvalid = false;
                    this.validated = false;
                    this.validateDataSeries = function (type, dataSeries) {
                        if (_this.validated) return _this.isvalid;
                        if (!_this.chartSchemas.has(type)) _this.errorMessage = "The graph type '" + type + "' does not have an associated schema to validate against.";
                        if (dataSeries == null || dataSeries == undefined) return false;
                        _this.validated = true;
                        var test = _this.chartSchemas.get(type);
                        var isvalid = test(dataSeries);
                        if (isvalid) {
                            _this.errorMessage = undefined;
                            _this.isvalid = true;
                            return true;
                        } else {
                            console.log("Validation errors:");
                            console.log(test.errors);
                            console.log("Data source:");
                            console.log(dataSeries);
                            _this.errorMessage = "The datasource schema was malformed for the graph plotter.  For more details view your browser JavaScript console. Message: " + test.errors[0].message + ".";
                        }
                        return false;
                    };
                    this.xAxisFormatting = function (label) {
                        var dataseries = _this.ref.dataSeries;
                        if (dataseries != null && dataseries != undefined && (dataseries.length == 0 || dataseries.length == 2)) return label;
                        if (dataseries[0].series == undefined || dataseries[0].series == null) return label;
                        var idx = -1;
                        var length = dataseries[0].series.length;
                        for (var i = 0; i < length; i++) {
                            var entry = dataseries[0].series[i];
                            if (label == entry.name) {
                                idx = i;
                                break;
                            }
                        }
                        if (length <= 6) {
                            return label;
                        } else {
                            if (idx % 4 == 0) {
                                return label;
                            } else {
                                return "";
                            }
                        }
                    };
                }
                GraphComponent.prototype.ngOnInit = function () {};
                GraphComponent.prototype.ngAfterViewInit = function () {};
                GraphComponent.prototype.ngOnDestroy = function () {};
                __decorate([core_1.Input('ref'), __metadata("design:type", index_1.MetricGraph)], GraphComponent.prototype, "ref", void 0);
                __decorate([core_1.Input('edit'), __metadata("design:type", Function)], GraphComponent.prototype, "onEdit", void 0);
                __decorate([core_1.Input('add'), __metadata("design:type", Function)], GraphComponent.prototype, "onAdd", void 0);
                __decorate([core_1.Input('delete'), __metadata("design:type", Function)], GraphComponent.prototype, "onDelete", void 0);
                GraphComponent = __decorate([core_1.Component({
                    selector: 'graph',
                    template: "        \n                    <div class=\"graph\">                                                                \n                        <div class=\"title row no-gutter no-gutters justify-content-end\"> \n                            <div class=\"col-9\"> {{ ref ? ref.title : \"Add New Graph\" }} </div>                      \n                            <div class=\"col-3\">                                    \n                                <ng-container *ngIf=\"ref && !ref.isLoading\">\n                                    <span class=\"float-right\" >\n                                        <action-stub-items  [model]=\"ref\" [edit]=\"onEdit\" [delete]=\"onDelete\"></action-stub-items>\n                                    </span>\n                                </ng-container>\n                            </div>\n                        </div>                     \n                        <div [ngClass]=\"{'chartarea': ref == undefined || ref.type != 'ngx-charts-gauge'}\">                        \n                            <div style=\"padding-top: 80px\" *ngIf=\"ref && ref.isLoading == true\">\n                                <loading-spinner></loading-spinner>\n                            </div>  \n                            <div style=\"padding-top: 50px\" *ngIf=\"ref && !ref.isLoading && ref.response && ref.response.Status && (ref.response.Status.State == 'FAILED' || ref.response.Status.State == 'CANCELLED') \">\n                                <p>{{ref.response.Status.StateChangeReason}}</p>\n                            </div>  \n                            <div style=\"padding-top: 50px\" *ngIf=\"errorMessage || (ref && ref.errorMessage)\">\n                                <p>{{errorMessage}}</p>\n                                <p *ngIf=\"ref\">{{ref.errorMessage}}</p>\n                            </div>                              \n                            <ng-container *ngIf=\"ref && ref.isLoading == false && validateDataSeries(ref.type, ref.dataSeries)\">                             \n                                <ngx-charts-line-chart *ngIf=\"ref.type == 'ngx-charts-line-chart'\"\n                                    [view]=\"[400, 300]\"\n                                    [scheme]=\"ref.colorScheme\"\n                                    [results]=\"ref.dataSeries\"                                \n                                    xAxis=\"true\"\n                                    yAxis=\"true\"\n                                    showXAxisLabel=\"true\"\n                                    showYAxisLabel=\"true\"\n                                    [xAxisLabel]=\"ref.xAxisLabel\"\n                                    [yAxisLabel]=\"ref.yAxisLabel\"\n                                    [xAxisTickFormatting]=\"xAxisFormatting\"     \n                                    autoScale=\"true\"\n                                    yScaleMin=0\n                                    >                        \n                                </ngx-charts-line-chart>\n                                 <ngx-charts-area-chart-stacked *ngIf=\"ref.type == 'ngx-charts-area-chart-stacked'\"\n                                    [view]=\"[400, 300]\"\n                                    [scheme]=\"ref.colorScheme\"\n                                    [results]=\"ref.dataSeries\"                                \n                                    xAxis=\"true\"\n                                    yAxis=\"true\"\n                                    showXAxisLabel=\"true\"\n                                    showYAxisLabel=\"true\"\n                                    [xAxisLabel]=\"ref.xAxisLabel\"\n                                    [yAxisLabel]=\"ref.yAxisLabel\"\n                                    [xAxisTickFormatting]=\"xAxisFormatting\"     \n                                    autoScale=\"true\"\n                                    yScaleMin=0\n                                    >                        \n                                </ngx-charts-area-chart-stacked>\n                                <ngx-charts-bar-vertical *ngIf=\"ref.type == 'ngx-charts-bar-vertical'\"\n                                    [view]=\"[400, 300]\"\n                                    [scheme]=\"ref.colorScheme\"\n                                    [results]=\"ref.dataSeries\"\n                                    [gradient]=\"gradient\"                            \n                                    xAxis=\"true\"\n                                    yAxis=\"true\"\n                                    showXAxisLabel=\"true\"\n                                    showYAxisLabel=\"true\"\n                                    [xAxisLabel]=\"ref.xAxisLabel\"\n                                    [yAxisLabel]=\"ref.yAxisLabel\"               \n                                    [xAxisTickFormatting]=\"xAxisFormatting\"                          \n                                    >\n                                </ngx-charts-bar-vertical>\n                                <ngx-charts-gauge *ngIf=\"ref.type == 'ngx-charts-gauge' \"\n                                  [view]=\"[450, 300]\"\n                                  [scheme]=\"ref.colorScheme\"\n                                  [results]=\"ref.dataSeries\"\n                                  [min]=\"0\"\n                                  [max]=\"100\"\n                                  [angleSpan]=\"240\"\n                                  [startAngle]=\"-120\"\n                                  [units]=\"ref.yAxisLabel\"\n                                  [bigSegments]=\"10\"\n                                  [smallSegments]=\"5\"\n                                  >\n                                </ngx-charts-gauge>\n                            </ng-container>\n                            <ng-container *ngIf=\"!ref\">                             \n                                <div (click)=\"onAdd()\" class=\"clickable\" ><i class=\"fa fa-plus fa-5x new-graph\"></i></div>\n                            </ng-container>\n                        </div>                        \n                    </div>\n    ",
                    styles: [".new-graph { margin-top: 94px; color: gainsboro; } .clickable { cursor: pointer; } "]
                }), __metadata("design:paramtypes", [core_1.ComponentFactoryResolver])], GraphComponent);
                return GraphComponent;
            }();
            exports_1("GraphComponent", GraphComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/component/index.js", ["./action-stub-basic.component", "./action-stub-items.component", "./action-stub.component", "./dragable.component", "./pagination.component", "./search.component", "./facet/facet.component", "./facet/rest-explorer.component", "./facet/cloud-watch-logs.component", "./facet/metric.component", "./thumbnail.component", "./graph.component"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    function exportStar_1(m) {
        var exports = {};
        for (var n in m) {
            if (n !== "default") exports[n] = m[n];
        }
        exports_1(exports);
    }
    return {
        setters: [function (action_stub_basic_component_1_1) {
            exportStar_1(action_stub_basic_component_1_1);
        }, function (action_stub_items_component_1_1) {
            exportStar_1(action_stub_items_component_1_1);
        }, function (action_stub_component_1_1) {
            exportStar_1(action_stub_component_1_1);
        }, function (dragable_component_1_1) {
            exportStar_1(dragable_component_1_1);
        }, function (pagination_component_1_1) {
            exportStar_1(pagination_component_1_1);
        }, function (search_component_1_1) {
            exportStar_1(search_component_1_1);
        }, function (facet_component_1_1) {
            exportStar_1(facet_component_1_1);
        }, function (rest_explorer_component_1_1) {
            exportStar_1(rest_explorer_component_1_1);
        }, function (cloud_watch_logs_component_1_1) {
            exportStar_1(cloud_watch_logs_component_1_1);
        }, function (metric_component_1_1) {
            exportStar_1(metric_component_1_1);
        }, function (thumbnail_component_1_1) {
            exportStar_1(thumbnail_component_1_1);
        }, function (graph_component_1_1) {
            exportStar_1(graph_component_1_1);
        }],
        execute: function () {}
    };
});
System.register("dist/app/view/game/module/shared/class/facet.class.js", ["../component/index"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var index_1, Facet, FacetDefinitions;
    return {
        setters: [function (index_1_1) {
            index_1 = index_1_1;
        }],
        execute: function () {
            Facet = /** @class */function () {
                //highest priority is 0, the higher the number the lower priority    
                function Facet(title, component, order, constraints, data) {
                    if (data === void 0) {
                        data = {};
                    }
                    this.title = title;
                    this.component = component;
                    this.order = order;
                    this.constraints = constraints;
                    this.data = data;
                }
                return Facet;
            }();
            exports_1("Facet", Facet);
            FacetDefinitions = /** @class */function () {
                function FacetDefinitions() {}
                FacetDefinitions.defined = [new Facet("REST Explorer", index_1.RestApiExplorerComponent, 0, ["ServiceUrl"]), new Facet("Log", index_1.CloudWatchLogComponent, 1, ["physicalResourceId", "cloudwatchPhysicalId"])];
                return FacetDefinitions;
            }();
            exports_1("FacetDefinitions", FacetDefinitions);
        }
    };
});
System.register("dist/app/view/game/module/shared/class/metric-graph.class.js", ["rxjs/Observable", "rxjs/add/observable/combineLatest"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    /**
    * parseMetricsLineData
    * A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
    * and formats it for the ngx-charts graphing library.
    * @param series_x_label
    * @param series_y_name
    * @param dataSeriesLabel
    * @param response
    */
    function MetricLineData(series_x_label, series_y_name, dataSeriesLabel, response) {
        var data = response.Result;
        data = data.slice();
        var header = data[0];
        data.splice(0, 1);
        //At least 2 points required to create a line graph.  If this data doesn't exist generate a 0 value for
        //a single data point.
        var result = [];
        if (header.length == 2) {
            result = MapTuple(header, data);
        } else if (header.length == 3) {
            result = MapVector(header, data);
        }
        return result;
    }
    exports_1("MetricLineData", MetricLineData);
    function MapTuple(header, data) {
        if (data.length <= 1 || !(data instanceof Array)) {
            return {
                "name": header[0],
                "series": [{
                    "name": "Insufficient Data Found",
                    "value": 0
                }]
            };
        }
        return {
            "name": header[0],
            "series": data.map(function (e) {
                var value = e[1];
                if (value == undefined) value = 0;
                return {
                    "name": e[0],
                    "value": Number.parseFloat(value)
                };
            })
        };
    }
    exports_1("MapTuple", MapTuple);
    function MapVector(header, data) {
        if (data.length <= 1 || !(data instanceof Array)) {
            return {
                "name": header[0],
                "series": [{
                    "name": "Insufficient Data Found",
                    "value": 0
                }]
            };
        }
        var map = new Map();
        for (var idx in data) {
            var row = data[idx];
            var x = row[0];
            if (!x) continue;
            var y = row[1];
            var z = row[2];
            if (!map[x]) map[x] = { "name": x, "series": [] };
            var value = z;
            if (value == undefined) value = 0;
            map[x].series.push({
                "name": y, "value": Number.parseFloat(value)
            });
        }
        var arr = [];
        for (var idx in map) {
            var series = map[idx];
            arr.push(series);
        }
        return arr;
    }
    exports_1("MapVector", MapVector);
    function MapMatrix(header, data) {
        if (data.length <= 1 || !(data instanceof Array)) {
            return {
                "name": header[0],
                "series": [{
                    "name": "Insufficient Data Found",
                    "value": 0
                }]
            };
        }
        var map = new Map();
        for (var idx in data) {
            var row = data[idx];
            var x = row[0];
            if (!x) continue;
            if (!map[x]) map[x] = { "name": x, "series": [] };
            for (var idz = 1; idz < header.length; idz++) {
                var name_1 = header[idz];
                var value = row[idz];
                if (value == undefined) value = 0;
                map[x].series.push({
                    "name": name_1, "value": Number.parseFloat(value)
                });
            }
        }
        var arr = [];
        for (var idx in map) {
            var series = map[idx];
            arr.push(series);
        }
        return arr;
    }
    exports_1("MapMatrix", MapMatrix);
    /**
      * metricsBarData
      * A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
      * and formats it for the ngx-charts graphing library.
      * @param series_x_label
      * @param series_y_name
      * @param dataSeriesLabel
      * @param response
      */
    function MetricBarData(series_x_label, series_y_name, dataSeriesLabel, response) {
        var data = response.Result;
        data = data.slice();
        var header = data[0];
        data.splice(0, 1);
        //At least 2 points required to create a line graph.  If this data doesn't exist generate a 0 value for
        //a single data point.
        if (data.length <= 0) {
            return [{
                "name": "Insufficient Data Found",
                "value": 0
            }];
        }
        return data.map(function (e) {
            var value = e[1];
            if (value == undefined) value = 0;
            return {
                "name": e[0],
                "value": Number.parseFloat(value)
            };
        });
    }
    exports_1("MetricBarData", MetricBarData);
    /**
      * ChoroplethData
      * A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
      * and formats it for the ngx-charts graphing library.
      * @param series_x_label
      * @param series_y_name
      * @param dataSeriesLabel
      * @param response
      */
    function ChoroplethData(series_x_label, series_y_name, dataSeriesLabel, response) {
        console.log(response);
    }
    exports_1("ChoroplethData", ChoroplethData);
    var Observable_1, MetricGraph;
    return {
        setters: [function (Observable_1_1) {
            Observable_1 = Observable_1_1;
        }, function (_1) {}],
        execute: function () {
            /**
             * MetricGraph
             * Model for encapsulating all info associated with a single metrics graph.
             */
            MetricGraph = /** @class */function () {
                function MetricGraph(title, xAxisLabel, yAxisLabel, dataSeriesLabels, dataSeriesObservable, dataMappingFunction, chart_type, series_name_x, series_name_y, color_scheme, model, refreshFrequency) {
                    if (xAxisLabel === void 0) {
                        xAxisLabel = "Date";
                    }
                    var _this = this;
                    // Text label for x Axis of graph
                    this.xAxisLabel = "Date";
                    this.errorMessage = undefined;
                    /**
                     * PopulateGraph
                     * Adds the data to the graph so it can be rendered.  Will be rendered once the observable returns.
                     */
                    this.populateGraph = function () {
                        _this._is_loading = true;
                        _this.response = undefined;
                        // Store all observables in a variable so we can resolve them all together.
                        var obs = Observable_1.Observable.combineLatest(_this._graphObservables);
                        _this._subscription = obs.subscribe(function (res) {
                            if (!res || res instanceof Array && res[0] === undefined) return;
                            if (res[0] instanceof Array) _this.dataSeries = res[0];else _this.dataSeries = res;
                            _this._subscription.unsubscribe();
                            _this._subscription.remove(_this._subscription);
                            _this._subscription.closed;
                        }, function (err) {
                            //retrieve the datasource soon than later as the backend timed out
                            _this._is_loading = false;
                            setTimeout(_this.refresh, 10000);
                            _this.errorMessage = "Please check that the SQL is valid.  Click the edit icon in the right top of this box to edit.";
                            console.log(err);
                        });
                    };
                    this.clearInterval = function () {
                        clearInterval(_this._intervalTimer);
                    };
                    this.refresh = function () {
                        _this.errorMessage = undefined;
                        _this.populateGraph();
                    };
                    this.title = title;
                    this.xAxisLabel = xAxisLabel;
                    this.yAxisLabel = yAxisLabel;
                    this.dataSeriesLabels = dataSeriesLabels;
                    this._dataSeriesObservable = dataSeriesObservable;
                    this._refreshFrequency = refreshFrequency ? refreshFrequency : 1800000;
                    this._dataMappingFunction = dataMappingFunction;
                    this._series_name_x = series_name_x;
                    this._series_name_y = series_name_y;
                    this.minDate = new Date();
                    this.maxDate = new Date();
                    this.minDate.setHours(this.minDate.getHours() - 12);
                    this.maxDate.setHours(this.maxDate.getHours() + 12);
                    this.dataSeries = undefined;
                    this.chart_type = chart_type ? chart_type : "ngx-charts-line-chart";
                    this.colorScheme = color_scheme ? color_scheme : {
                        domain: ['#401E5F', '#49226D', '#551a8b', '#642E95', '#7343A0', '#8358AA', '#8054A8', '#8E67B1', '#9C7ABB', '#AA8DC5', '#B8A0CE', '#C6B3D8', '#D4C6E2', '#E2D9EB', '#f8ecd3', '#f4e2be', '#f0d8a8', '#edce92', '#e9c57c', '#e5bb66', '#e1b151', '#dea73b', '#da9e25', '#c48e21', '#ae7e1e', '#996e1a', '#835f16', '#6d4f12', '#573f0f', '#412f0b', '#2c2007']
                    };
                    this.model = model;
                    if (this.dataSeriesLabels && this.dataSeriesLabels.length > 1) this.showLegend = true;
                    this._graphObservables = this._dataSeriesObservable.map(function (graphObservable, index) {
                        return graphObservable.map(function (response) {
                            if (response === undefined) return;
                            _this._is_loading = false;
                            _this.response = 'body' in response && response instanceof Object ? JSON.parse(response.body.text()).result : response;
                            if (_this.response.Result && _this.response.Result.length == 0 || _this.response.Status && (_this.response.Status.State == "FAILED" || _this.response.Status.State == "CANCELLED")) return;
                            return _this._dataMappingFunction[index](_this._series_name_x != undefined ? _this._series_name_x[index] : undefined, _this._series_name_y != undefined ? _this._series_name_y[index] : undefined, _this.dataSeriesLabels[index], _this.response);
                        });
                    });
                    if (dataSeriesObservable) {
                        this.populateGraph();
                        this._intervalTimer = setInterval(function () {
                            _this.refresh();
                        }, this._refreshFrequency);
                    }
                }
                Object.defineProperty(MetricGraph.prototype, "type", {
                    get: function () {
                        return this.chart_type;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(MetricGraph.prototype, "isLoading", {
                    get: function () {
                        return this._is_loading;
                    },
                    set: function (value) {
                        this._is_loading = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(MetricGraph.prototype, "dataMapFunction", {
                    get: function () {
                        return this._dataMappingFunction;
                    },
                    enumerable: true,
                    configurable: true
                });
                return MetricGraph;
            }();
            exports_1("MetricGraph", MetricGraph);
        }
    };
});
System.register("dist/app/view/game/module/shared/class/query.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __extends = this && this.__extends || function () {
        var extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
        };
        return function (d, b) {
            extendStatics(d, b);
            function __() {
                this.constructor = d;
            }
            d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
        };
    }();
    var __moduleName = context_1 && context_1.id;
    var MetricQuery, Directive, AllTablesUnion;
    return {
        setters: [],
        execute: function () {
            MetricQuery = /** @class */function () {
                function MetricQuery(aws, table, tables, supported_directives) {
                    if (table === void 0) {
                        table = "";
                    }
                    if (tables === void 0) {
                        tables = [];
                    }
                    if (supported_directives === void 0) {
                        supported_directives = [];
                    }
                    var _this = this;
                    this.aws = aws;
                    this.table = table;
                    this.tables = tables;
                    this.supported_directives = supported_directives;
                    this.template = function (sql) {
                        for (var idx in _this.directives) {
                            var directive = _this.directives[idx];
                            sql = directive.construct(sql);
                        }
                        var date = new Date();
                        var projectName = _this.aws.context.name.replace('-', '_').toLowerCase();
                        var deploymentName = _this.aws.context.project.activeDeployment.settings.name.replace('-', '_').toLowerCase();
                        var table_prefix = projectName + "_" + deploymentName + "_table_";
                        var table = "" + table_prefix + _this._table;
                        var database = projectName + "_" + deploymentName;
                        var table_clientinitcomplete = table_prefix + "clientinitcomplete";
                        var table_sessionstart = table_prefix + "sessionstart";
                        var str = "let database = '\"" + database + "\"'; " + " let database_unquoted = '" + database + "'; " + " let table = '\"" + table + "\"';" + " let table_unquoted = '" + table + "'; " + " let year = " + date.getUTCFullYear() + ";" + " let month = " + (date.getUTCMonth() + 1) + ";" + " let day = " + date.getDate() + ";" + " let hour = " + date.getHours() + ";" + " let table_clientinitcomplete = '\"" + table_clientinitcomplete + "\"';" + " let table_sessionstart = '\"" + table_sessionstart + "\"';" + " return `" + sql.replace(/[\r\n]/g, ' ') + "`;";
                        return new Function(str)();
                    };
                    this.toString = function (sql) {
                        return _this.template(sql);
                    };
                    this._directives = supported_directives;
                    this._table = table;
                }
                Object.defineProperty(MetricQuery.prototype, "tablename", {
                    set: function (value) {
                        this._table = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(MetricQuery.prototype, "directives", {
                    get: function () {
                        return this._directives;
                    },
                    enumerable: true,
                    configurable: true
                });
                return MetricQuery;
            }();
            exports_1("MetricQuery", MetricQuery);
            Directive = /** @class */function () {
                function Directive(aws) {
                    this.aws = aws;
                    this.parser = new MetricQuery(this.aws);
                }
                return Directive;
            }();
            exports_1("Directive", Directive);
            AllTablesUnion = /** @class */function (_super) {
                __extends(AllTablesUnion, _super);
                function AllTablesUnion(aws, tables) {
                    var _this = _super.call(this, aws) || this;
                    _this.aws = aws;
                    _this.tables = tables;
                    return _this;
                }
                AllTablesUnion.prototype.construct = function (template) {
                    var pattern = /<ALL_TABLES_UNION>[\s\S]*<\/ALL_TABLES_UNION>/g;
                    var match = pattern.exec(template);
                    if (match == null || match.length == 0) return template;
                    var content = "";
                    for (var idz = 0; idz < match.length; idz++) {
                        var content_match = match[idz];
                        var inner_template = content_match.replace("<ALL_TABLES_UNION>", "").replace("</ALL_TABLES_UNION>", "");
                        var inner_content = "";
                        for (var idx = 0; idx < this.tables.length; idx++) {
                            var table = this.tables[idx];
                            this.parser.tablename = table;
                            inner_content = inner_content + " " + this.parser.toString(inner_template);
                            if (idx < this.tables.length - 1) inner_content = inner_content + " UNION ";
                        }
                        content = template.replace(content_match, inner_content);
                    }
                    return content;
                };
                return AllTablesUnion;
            }(Directive);
            exports_1("AllTablesUnion", AllTablesUnion);
        }
    };
});
System.register("dist/app/view/game/module/shared/class/index.js", ["./action-stub.class", "./search-dropdown.class", "./facet.class", "./metric-graph.class", "./query.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    function exportStar_1(m) {
        var exports = {};
        for (var n in m) {
            if (n !== "default") exports[n] = m[n];
        }
        exports_1(exports);
    }
    return {
        setters: [function (action_stub_class_1_1) {
            exportStar_1(action_stub_class_1_1);
        }, function (search_dropdown_class_1_1) {
            exportStar_1(search_dropdown_class_1_1);
        }, function (facet_class_1_1) {
            exportStar_1(facet_class_1_1);
        }, function (metric_graph_class_1_1) {
            exportStar_1(metric_graph_class_1_1);
        }, function (query_class_1_1) {
            exportStar_1(query_class_1_1);
        }],
        execute: function () {}
    };
});
System.register("dist/app/view/game/module/admin/component/user-admin/user-admin.component.js", ["@angular/core", "@angular/forms", "app/aws/user/user-management.class", "app/aws/aws.service", "app/shared/component/index", "app/view/game/module/shared/class/index", "ng2-toastr", "app/shared/service/index", "app/shared/class/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, forms_1, user_management_class_1, aws_service_1, index_1, index_2, ng2_toastr_1, index_3, environment, EnumUserAdminModes, EnumAccountStatus, UserAdminComponent, Role;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (forms_1_1) {
            forms_1 = forms_1_1;
        }, function (user_management_class_1_1) {
            user_management_class_1 = user_management_class_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }, function (index_3_1) {
            index_3 = index_3_1;
        }, function (environment_1) {
            environment = environment_1;
        }],
        execute: function () {
            (function (EnumUserAdminModes) {
                EnumUserAdminModes[EnumUserAdminModes["Default"] = 0] = "Default";
                EnumUserAdminModes[EnumUserAdminModes["AddUser"] = 1] = "AddUser";
                EnumUserAdminModes[EnumUserAdminModes["DeleteUser"] = 2] = "DeleteUser";
                EnumUserAdminModes[EnumUserAdminModes["EditRole"] = 3] = "EditRole";
                EnumUserAdminModes[EnumUserAdminModes["ResetPassword"] = 4] = "ResetPassword";
            })(EnumUserAdminModes || (EnumUserAdminModes = {}));
            exports_1("EnumUserAdminModes", EnumUserAdminModes);
            (function (EnumAccountStatus) {
                EnumAccountStatus[EnumAccountStatus["ACTIVE"] = 0] = "ACTIVE";
                EnumAccountStatus[EnumAccountStatus["PENDING"] = 1] = "PENDING";
                EnumAccountStatus[EnumAccountStatus["UNCONFIRMED"] = 2] = "UNCONFIRMED";
                EnumAccountStatus[EnumAccountStatus["ARCHIVED"] = 3] = "ARCHIVED";
                EnumAccountStatus[EnumAccountStatus["CONFIRMED"] = 4] = "CONFIRMED";
                EnumAccountStatus[EnumAccountStatus["COMPROMISED"] = 5] = "COMPROMISED";
                EnumAccountStatus[EnumAccountStatus["RESET_REQUIRED"] = 6] = "RESET_REQUIRED";
                EnumAccountStatus[EnumAccountStatus["FORCE_CHANGE_PASSWORD"] = 7] = "FORCE_CHANGE_PASSWORD";
                EnumAccountStatus[EnumAccountStatus["DISABLED"] = 8] = "DISABLED";
            })(EnumAccountStatus || (EnumAccountStatus = {}));
            exports_1("EnumAccountStatus", EnumAccountStatus);
            UserAdminComponent = /** @class */function () {
                function UserAdminComponent(fb, aws, cdr, toastr, metric) {
                    this.aws = aws;
                    this.cdr = cdr;
                    this.toastr = toastr;
                    this.metric = metric;
                    this.awsCognitoLink = "https://console.aws.amazon.com/cognito/home";
                    this.missingEmailString = "n/a";
                    this.currentTabIndex = 0;
                    this.userSortDescending = true;
                    this.roleSortDescending = true;
                    this.rolesList = [];
                    this.currentMode = 0;
                    this.userAdminModesEnum = EnumUserAdminModes;
                    this.userState = EnumAccountStatus;
                    this.allUsers = [];
                    this.userList = [];
                    this.original_allUsers = [];
                    this.loadingUsers = false;
                    this.action = new index_2.ActionItem("Reset Password", this.resetPasswordView);
                    this.stubActions = [this.action];
                    this.userValidationError = "";
                    this.numUserPages = 0;
                    this.currentStartIndex = 0;
                    this.pageSize = 10;
                    this.enumUserGroup = user_management_class_1.EnumGroupName;
                    this.userForm = fb.group({
                        'username': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'email': [null, forms_1.Validators.compose([forms_1.Validators.required])],
                        'password': [this.passwordGenerator(), forms_1.Validators.compose([forms_1.Validators.required])],
                        'role': user_management_class_1.EnumGroupName.USER
                    });
                }
                Object.defineProperty(UserAdminComponent.prototype, "metricIdentifier", {
                    get: function () {
                        return environment.metricWhiteListedFeature[environment.metricAdminIndex];
                    },
                    enumerable: true,
                    configurable: true
                });
                UserAdminComponent.prototype.ngOnInit = function () {
                    var _this = this;
                    // bind function scopes
                    this.editRoleModal = this.editRoleModal.bind(this);
                    this.dismissUserModal = this.dismissUserModal.bind(this);
                    this.dismissRoleModal = this.dismissRoleModal.bind(this);
                    this.addUser = this.addUser.bind(this);
                    this.deleteUser = this.deleteUser.bind(this);
                    this.updateUserList = this.updateUserList.bind(this);
                    this.updateUserPage = this.updateUserPage.bind(this);
                    this.resetPasswordView = this.resetPasswordView.bind(this);
                    this.action.onClick = this.action.onClick.bind(this);
                    this.resetPassword = this.resetPassword.bind(this);
                    this.deleteUserView = this.deleteUserView.bind(this);
                    // end of binding function scopes
                    this.searchDropdownOptions = [{ text: "Username" }, { text: "Email" }];
                    // alert change detection tree when form is updated
                    this.userForm.valueChanges.subscribe(function () {
                        _this.cdr.detectChanges();
                    });
                    // Roles list isn't modifiable for now
                    this.rolesList.push(new Role("Portal Admin", 1));
                    this.rolesList.push(new Role("User", 1));
                    this.updateUserList();
                };
                UserAdminComponent.prototype.isRequiredEmpty = function (form, item) {
                    return form.controls[item].hasError('required') && form.controls[item].touched;
                };
                UserAdminComponent.prototype.isNotValid = function (form, item) {
                    return !form.controls[item].valid && form.controls[item].touched;
                };
                UserAdminComponent.prototype.updateUserList = function () {
                    var _this = this;
                    this.cdr.detectChanges();
                    this.currentMode = EnumUserAdminModes.Default;
                    this.loadingUsers = true;
                    var users = [];
                    this.aws.context.userManagement.getAdministrators(function (result) {
                        for (var i = 0; i < result.Users.length; i++) {
                            _this.appendUser(result.Users[i], _this.rolesList[0].name, users);
                        }
                        _this.rolesList[0].numUsers = result.Users.length;
                        _this.aws.context.userManagement.getUsers(function (result) {
                            for (var i = 0; i < result.Users.length; i++) {
                                _this.appendUser(result.Users[i], _this.rolesList[1].name, users);
                            }
                            _this.rolesList[1].numUsers = result.Users.length;
                            users.sort(function (user1, user2) {
                                if (user1.Username > user2.Username) return 1;
                                if (user1.Username < user2.Username) return -1;
                                return 0;
                            });
                            _this.setAllUsers(users);
                            _this.loadingUsers = false;
                        }, function (err) {
                            _this.toastr.error(err.message);
                        });
                    }, function (err) {
                        _this.toastr.error(err.message);
                    });
                };
                UserAdminComponent.prototype.updateUserPage = function (pageNum) {
                    // return a subset of all users for the current page
                    if (!pageNum) pageNum = 1;
                    this.userList = this.allUsers.slice((pageNum - 1) * this.pageSize, (pageNum - 1) * this.pageSize + this.pageSize);
                };
                UserAdminComponent.prototype.updateSubNav = function (tabIndex) {
                    this.currentTabIndex = tabIndex;
                };
                UserAdminComponent.prototype.editRoleModal = function (model) {
                    this.currentMode = EnumUserAdminModes.EditRole;
                    this.currentRole = model;
                };
                UserAdminComponent.prototype.changeUserSortDirection = function () {
                    this.userSortDescending = !this.userSortDescending;
                    this.setAllUsers(this.allUsers.reverse());
                };
                UserAdminComponent.prototype.changeRoleSortDirection = function () {
                    this.roleSortDescending = !this.roleSortDescending;
                    if (this.roleSortDescending) {
                        this.rolesList = this.rolesList.sort(Role.sortByNameDescending);
                    } else {
                        this.rolesList = this.rolesList.sort(Role.sortByNameDescending).reverse();
                    }
                };
                UserAdminComponent.prototype.deleteUserView = function (user) {
                    this.currentUser = user;
                    this.currentMode = EnumUserAdminModes.DeleteUser;
                };
                UserAdminComponent.prototype.resetPasswordView = function (user) {
                    this.currentUser = user;
                    this.currentMode = EnumUserAdminModes.ResetPassword;
                };
                UserAdminComponent.prototype.resetPassword = function (user) {
                    var _this = this;
                    this.aws.context.userManagement.resetPassword(user.Username, function (result) {
                        _this.modalRef.close();
                        _this.toastr.success("The user '" + user.Username + "' password has been reset");
                        _this.updateUserList();
                    }, function (err) {
                        _this.modalRef.close();
                        _this.toastr.error(err.message);
                    });
                };
                UserAdminComponent.prototype.setDefaultUserFormValues = function () {
                    this.userForm.reset();
                    // initialize form group values if needed        
                    this.userForm.setValue({
                        username: "",
                        email: "",
                        password: this.passwordGenerator(),
                        role: user_management_class_1.EnumGroupName.USER
                    });
                };
                UserAdminComponent.prototype.searchUsers = function (filter) {
                    if (!filter.value) {
                        this.setAllUsers(this.original_allUsers.length > this.allUsers.length ? this.original_allUsers : this.allUsers);
                        this.original_allUsers = [];
                        return;
                    }
                    if (this.original_allUsers.length == 0) this.original_allUsers = Array.from(this.allUsers);else this.setAllUsers(this.original_allUsers);
                    this.setAllUsers(this.allUsers.filter(function (user) {
                        var value = user[filter.id];
                        return user[filter.id].indexOf(filter.value) >= 0;
                    }));
                };
                UserAdminComponent.prototype.addUser = function (user) {
                    var _this = this;
                    // submit is done in the validation in this case so we don't have to do anything here.
                    this.aws.context.userManagement.register(user.username, user.password, user.email, user.role, function (result) {
                        _this.modalRef.close();
                        _this.toastr.success("The user '" + user.username + "' has been added");
                        _this.updateUserList();
                    }, function (err) {
                        // Display the error and remove the beginning chunk since it's not formatted well
                        _this.userValidationError = err.message;
                    });
                };
                UserAdminComponent.prototype.deleteUser = function (user) {
                    var _this = this;
                    this.aws.context.userManagement.deleteUser(user.Username, function (result) {
                        _this.modalRef.close();
                        _this.toastr.success("The user '" + user.Username + "' has been deleted");
                        _this.updateUserList();
                    }, function (err) {
                        // Display the error and remove the beginning chunk since it's not formatted well
                        _this.toastr.error(err.message);
                    });
                };
                UserAdminComponent.prototype.dismissUserModal = function () {
                    this.currentMode = EnumUserAdminModes.Default;
                    this.userValidationError = "";
                    this.setDefaultUserFormValues();
                };
                UserAdminComponent.prototype.dismissRoleModal = function () {
                    this.currentMode = EnumUserAdminModes.Default;
                };
                UserAdminComponent.prototype.containsAccountStatus = function (user_status, target_status) {
                    if (!user_status) return false;
                    var name = EnumAccountStatus[target_status];
                    var index = user_status.toLowerCase().indexOf(name.toLowerCase());
                    return user_status.toLowerCase().indexOf(name.toLowerCase()) >= 0;
                };
                UserAdminComponent.prototype.setAllUsers = function (array) {
                    this.allUsers = array;
                    this.numUserPages = Math.ceil(this.allUsers.length / this.pageSize);
                    this.updateUserPage();
                };
                UserAdminComponent.prototype.appendUser = function (user, rolename, array) {
                    var attributes = user.Attributes.filter(function (user) {
                        return user.Name == 'email';
                    });
                    user['Role'] = rolename;
                    user['Email'] = attributes.length > 0 ? attributes[0].Value : this.missingEmailString;
                    array.push(user);
                };
                UserAdminComponent.prototype.passwordGenerator = function () {
                    var upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
                    var lower = "abcdefghijklmnopqrstuvwxyz";
                    var number = "0123456789";
                    var symbol = "!@#$%^&*()";
                    var password = lower.charAt(Math.floor(Math.random() * lower.length)) + upper.charAt(Math.floor(Math.random() * upper.length)) + number.charAt(Math.floor(Math.random() * number.length)) + symbol.charAt(Math.floor(Math.random() * symbol.length)) + (Math.random() + 1).toString(36).substr(2, 8);
                    var passwordArray = password.split('');
                    for (var x = passwordArray.length - 1; x >= 0; x--) {
                        var idx = Math.floor(Math.random() * (x + 1));
                        var item = passwordArray[idx];
                        passwordArray[idx] = passwordArray[x];
                        passwordArray[x] = item;
                    }
                    return passwordArray.join('');
                };
                __decorate([core_1.ViewChild(index_1.ModalComponent), __metadata("design:type", index_1.ModalComponent)], UserAdminComponent.prototype, "modalRef", void 0);
                UserAdminComponent = __decorate([core_1.Component({
                    selector: 'admin',
                    template: "<div class=\"row no-gutters\">     <h1 class=\"col-8 gems-heading\"> User Administration  </h1>     <div class=\"col-4\">         <i (click)=\"updateUserList()\" class=\"fa fa-refresh fa-2x refresh-icon\"> </i>     </div> </div>     <facet-generator [tabs]=\"['Users', 'Roles']\"         (tabClicked)=\"updateSubNav($event)\"         [metricIdentifier]=\"metricIdentifier\"></facet-generator> <div *ngIf=\"currentTabIndex === 0\">     <div class=\"row add-row\">         <div class=\"col-6\">             <button id=\"add-new-user\" (click)=\"currentMode = userAdminModesEnum.AddUser\" class=\"btn l-primary ignore-col-margin\">                 Add New User             </button>         </div>         <div class=\"col-6\">             <search class=\"float-right\"                 [dropdownOptions]=\"searchDropdownOptions\"                 searchInputPlaceholder=\"Search for Users\"                 (searchUpdated)=\"searchUsers($event)\">             </search>         </div>     </div>     <div [ngSwitch]=\"loadingUsers\">         <div *ngSwitchCase=\"true\">             <loading-spinner></loading-spinner>         </div>         <div *ngSwitchCase=\"false\">             <h2 class=\"user-count\"> {{allUsers?.length}} Users </h2>             <table class=\"table\">                 <thead>                     <tr>                         <th>                             <div (click)=\"changeUserSortDirection()\" class=\"cursor-pointer\">                                 USERNAME                                 <i *ngIf=\"userSortDescending\" class=\"fa fa-caret-down\" aria-hidden=\"true\"></i>                                 <i *ngIf=\"!userSortDescending\" class=\"fa fa-caret-up\" aria-hidden=\"true\"></i>                             </div>                         </th>                         <th> EMAIL </th>                         <th> ROLE </th>                         <th> STATE </th>                         <th> ADDED </th>                         <th> MODIFIED </th>                     </tr>                 </thead>                 <tbody>                     <tr *ngFor=\"let user of userList\" [id]=\"user.Username\">                         <td> {{user.Username}}</td>                         <td> {{user.Email}} </td>                         <td> {{user.Role}} </td>                         <td>                             <i class=\"fa\"                                 [class.success-overlay-color]=\"containsAccountStatus(user.UserStatus, userState.CONFIRMED) || containsAccountStatus(user.UserStatus, userState.UNCONFIRMED)\"                                 [class.fa-check-circle]=\"containsAccountStatus(user.UserStatus, userState.CONFIRMED) || containsAccountStatus(user.UserStatus, userState.UNCONFIRMED)\"                                 [class.warning-overlay-color]=\"containsAccountStatus(user.UserStatus, userState.PENDING) || containsAccountStatus(user.UserStatus, userState.UNCONFIRMED)\"                                 [class.fa-clock-o]=\"containsAccountStatus(user.UserStatus, userState.PENDING) || containsAccountStatus(user.UserStatus, userState.UNCONFIRMED)\"></i>                             {{user.UserStatus.split('_').join(' ')}}                             <i class=\"fa fa-question-circle-o\" *ngIf=\"containsAccountStatus(user.UserStatus, userState.ARCHIVED)\" placement=\"bottom\"                                 ngbTooltip=\"The account has been archived due to inactivity.  This can only be corrected at {{awsCognitoLink}}.\"></i>                             <i class=\"fa fa-question-circle-o\" *ngIf=\"containsAccountStatus(user.UserStatus, userState.UNCONFIRMED)\" placement=\"bottom\"                                 ngbTooltip=\"After the player creates an account, it's not usable until they confirm their email address.\"></i>                             <i class=\"fa fa-question-circle-o\" *ngIf=\"containsAccountStatus(user.UserStatus, userState.COMPROMISED)\" placement=\"bottom\"                                 ngbTooltip=\"Please investigate this account further at {{awsCognitoLink}}.\"></i>                             <i class=\"fa fa-question-circle-o\" *ngIf=\"containsAccountStatus(user.UserStatus, userState.RESET_REQUIRED)\" placement=\"bottom\"                                 ngbTooltip=\"The user was imported but has not logged in.\"></i>                             <i class=\"fa fa-question-circle-o\" *ngIf=\"containsAccountStatus(user.UserStatus, userState.FORCE_CHANGE_PASSWORD)\"                                 ngbTooltip=\"After a AWS admin creates an account at {{awsCognitoLink}}, the user is forced to change their password before they can successfully sign in.\" placement=\"bottom\"></i>                             <i class=\"fa fa-question-circle-o\" *ngIf=\"containsAccountStatus(user.UserStatus, userState.DISABLED)\" placement=\"bottom\"                                 ngbTooltip=\"The user has been disabled on the Amazon Cognito site ({{awsCognitoLink}}).\"></i>                         </td>                         <td> {{user.UserCreateDate | date: 'short' }}</td>                         <td> {{user.UserLastModifiedDate | date: 'short' }}</td>                         <td>                         <td>                             <div class=\"float-right\">                                 <action-stub-items *ngIf=\"user.Email != missingEmailString\" class=\"\" [model]=\"user\" [delete]=\"deleteUserView\" [custom]=\"stubActions\"></action-stub-items>                             </div>                         </td>                     </tr>                 </tbody>             </table>             <pagination [pages]=\"numUserPages\"                         (pageChanged)=\"updateUserPage($event)\"></pagination>         </div>     </div> </div> <div *ngIf=\"currentTabIndex === 1\">         <h2 class=\"user-count\"> 2 Roles </h2>     <table class=\"table\">         <thead>             <tr>                 <th>                     <div (click)=\"changeRoleSortDirection()\" class=\"cursor-pointer\">                         ROLE                         <i *ngIf=\"roleSortDescending\" class=\"fa fa-caret-down\" aria-hidden=\"true\"></i>                         <i *ngIf=\"!roleSortDescending\" class=\"fa fa-caret-up\" aria-hidden=\"true\"></i>                     </div>                 </th>                 <th> USERS </th>             </tr>         </thead>         <tbody>             <tr *ngFor=\"let role of rolesList\" (click)=\"editRoleModal(role)\" >                 <td style=\"width: 15%\"> {{ role.name }} </td>                 <td> {{ role.numUsers }} </td>             </tr>         </tbody>     </table> </div>   <modal *ngIf=\"currentMode == userAdminModesEnum.AddUser\"     title=\"Add User\"     [hasSubmit]=\"true\"     [canSubmit]=\"userForm.valid\"     [autoOpen]=\"true\"     [metricIdentifier]=\"metricIdentifier\"     (modalSubmitted)=\"addUser(userForm.value)\"     [onDismiss]=\"dismissUserModal\"     [onClose]=\"dismissUserModal\"     submitButtonText=\"Add User\">     <div class=\"modal-body\">         <p *ngIf=\"userValidationError\" class=\"text-danger\">             {{userValidationError}}         </p>         <form [formGroup]=\"userForm\" (ngSubmit)=\"createUser(userForm.value)\" autocomplete=\"off\">             <div class=\"form-group row\" [ngClass]=\"{'has-danger':!userForm.controls['username'].valid && userForm.controls['username'].touched}\">                 <label class=\"col-4 col-form-label affix\" for=\"userName\"> Username </label>                 <div class=\"col-6 no-padding\">                     <input class=\"form-control\" id=\"username\" type=\"text\" [formControl]=\"userForm.controls['username']\"                     [ngClass]=\"{'form-control-danger': !userForm.controls['username'].valid && userForm.controls['username'].touched}\" />                     <div *ngIf=\"userForm.controls['username'].hasError('required') && userForm.controls['username'].touched\" class=\"form-control-feedback\">You must include a username.</div>                     <div *ngIf=\"!userForm.controls['username'].valid && !userForm.controls['username'].hasError('required') && userForm.controls['username'].touched\" class=\"form-control-feedback\"> Invalid username </div>                 </div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger':!userForm.controls['email'].valid && userForm.controls['email'].touched}\">                 <label class=\"col-4 col-form-label affix\" for=\"email\"> Email </label>                 <div class=\"col-6 no-padding\">                     <input class=\"form-control\" autocomplete=\"off\" id=\"email\" type=\"email\" name=\"email\" [formControl]=\"userForm.controls['email']\"                     [ngClass]=\"{'form-control-danger': !userForm.controls['email'].valid && userForm.controls['email'].touched}\" />                     <div *ngIf=\"userForm.controls['email'].hasError('required') && userForm.controls['email'].touched\" class=\"form-control-feedback\">You must include an email.</div>                 </div>             </div>             <div class=\"form-group row\" [ngClass]=\"{'has-danger': isNotValid(userForm, 'password')}\">                 <label class=\"col-4 col-form-label affix\" for=\"code\"> Temporary Password <i class=\"fa fa-question-circle-o\" placement=\"bottom\"                     ngbTooltip=\"Requires a minimum of 8 characters.  Must include at least 1 number, 1 special character, 1 uppercase letter, and 1 lowercase.\"></i></label>                 <div class=\"col-6 no-padding\">                     <input class=\"form-control\" id=\"password\" type=\"text\" [formControl]=\"userForm.controls['password']\"                            [ngClass]=\"{'form-control-danger': isNotValid(userForm, 'password')}\" />                     <div *ngIf=\"isRequiredEmpty(userForm, 'password')\" class=\"form-control-feedback\">You must include a password.</div>                 </div>             </div>             <div class=\"form-group row\">                 <label class=\"col-4 col-form-label affix\"> Roles </label>                 <div class=\"radio-btn-container\">                     <label class=\"radio-btn-label\">                         <input class=\"form-control\" type=\"radio\" [value]=\"enumUserGroup.ADMINISTRATOR\"                         [formControl]=\"userForm.controls['role']\" formControlName=\"role\"/>                         <span></span> Admin                     </label>                     <label class=\"radio-btn-label\">                         <input class=\"form-control\" type=\"radio\" [value]=\"enumUserGroup.USER\"                         [formControl]=\"userForm.controls['role']\" formControlName=\"role\" checked/>                         <span></span> User                     </label>                 </div>             </div>         </form>     </div>  </modal>  <modal *ngIf=\"currentMode == userAdminModesEnum.DeleteUser\"     title=\"Delete the user ''{{currentUser.Username}}''\"     [autoOpen]=\"true\"     [hasSubmit]=\"true\"     (modalSubmitted)=\"deleteUser(currentUser)\"     [onClose]=\"dismissUserModal\"     [onDismiss]=\"dismissUserModal\"     submitButtonText=\"Delete\">     <div class=\"modal-body\">         <p> Are you sure you want to delete this user? </p>     </div> </modal>  <modal *ngIf=\"currentMode == userAdminModesEnum.ResetPassword\"        title=\"Reset password for the user '{{currentUser.Username}}'\"        [autoOpen]=\"true\"        [hasSubmit]=\"true\"        (modalSubmitted)=\"resetPassword(currentUser)\"        [onClose]=\"dismissUserModal\"        [onDismiss]=\"dismissUserModal\"        submitButtonText=\"Reset\">     <div class=\"modal-body\">         <p> Are you sure you want to reset the password for this user? </p>     </div> </modal>",
                    styles: ["\n        .add-row {\n            margin-bottom: 25px;\n        }\n\n        h2.user-count {\n            margin-left: 10px;\n        }\n        "]
                }), __metadata("design:paramtypes", [forms_1.FormBuilder, aws_service_1.AwsService, core_1.ChangeDetectorRef, ng2_toastr_1.ToastsManager, index_3.LyMetricService])], UserAdminComponent);
                return UserAdminComponent;
            }();
            exports_1("UserAdminComponent", UserAdminComponent);
            Role = /** @class */function () {
                function Role(name, numUsers) {
                    this._name = name;
                    this._numUsers = numUsers;
                }
                Object.defineProperty(Role.prototype, "name", {
                    get: function () {
                        return this._name;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Role.prototype, "numUsers", {
                    get: function () {
                        return this._numUsers;
                    },
                    set: function (numUsers) {
                        this._numUsers = numUsers;
                    },
                    enumerable: true,
                    configurable: true
                });
                Role.sortByNameDescending = function (a, b) {
                    return a.name.localeCompare(b.name);
                };
                return Role;
            }();
            exports_1("Role", Role);
        }
    };
});
System.register("dist/app/view/game/module/admin/component/log.component.js", ["@angular/core", "app/aws/aws.service"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, ProjectLogComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }],
        execute: function () {
            ProjectLogComponent = /** @class */function () {
                function ProjectLogComponent(aws) {
                    this.aws = aws;
                    this.context = {};
                    this.refresh = false;
                }
                ProjectLogComponent.prototype.ngOnInit = function () {
                    this.context = {
                        cloudwatchPhysicalId: bootstrap.projectPhysicalId
                    };
                };
                ProjectLogComponent = __decorate([core_1.Component({
                    selector: 'project-logs',
                    template: "<div class=\"row no-gutters\">     <h1 class=\"col-8 gems-heading\"> Project Logs  </h1>     <div class=\"col-4\">              </div> </div>     <facet-generator *ngIf=\"!refresh\"         (tabClicked)=\"currentTabIndex = $event\"         [context]=\"context\"         metricIdentifier=\"projectlogs\"></facet-generator>"
                }), __metadata("design:paramtypes", [aws_service_1.AwsService])], ProjectLogComponent);
                return ProjectLogComponent;
            }();
            exports_1("ProjectLogComponent", ProjectLogComponent);
        }
    };
});
System.register("dist/app/view/game/module/shared/class/feature.class.js", ["../../analytic/component/dashboard/dashboard.component", "../../analytic/component/heatmap/heatmap.component", "../../admin/component/jira/jira-credentials.component", "../../analytic/component/analytic.component", "../../admin/component/admin.component", "../../admin/component/user-admin/user-admin.component", "../../admin/component/log.component"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var dashboard_component_1, heatmap_component_1, jira_credentials_component_1, analytic_component_1, admin_component_1, user_admin_component_1, log_component_1, Feature, Location, FeatureDefinitions;
    return {
        setters: [function (dashboard_component_1_1) {
            dashboard_component_1 = dashboard_component_1_1;
        }, function (heatmap_component_1_1) {
            heatmap_component_1 = heatmap_component_1_1;
        }, function (jira_credentials_component_1_1) {
            jira_credentials_component_1 = jira_credentials_component_1_1;
        }, function (analytic_component_1_1) {
            analytic_component_1 = analytic_component_1_1;
        }, function (admin_component_1_1) {
            admin_component_1 = admin_component_1_1;
        }, function (user_admin_component_1_1) {
            user_admin_component_1 = user_admin_component_1_1;
        }, function (log_component_1_1) {
            log_component_1 = log_component_1_1;
        }],
        execute: function () {
            Feature = /** @class */function () {
                //highest priority is 0, the higher the number the lower priority
                function Feature(parent, name, component, order, dependencies, route_path, location, description, image, data) {
                    if (description === void 0) {
                        description = "";
                    }
                    if (image === void 0) {
                        image = "";
                    }
                    if (data === void 0) {
                        data = {};
                    }
                    this.parent = parent;
                    this.name = name;
                    this.component = component;
                    this.order = order;
                    this.dependencies = dependencies;
                    this.route_path = route_path;
                    this.location = location;
                    this.description = description;
                    this.image = image;
                    this.data = data;
                }
                return Feature;
            }();
            exports_1("Feature", Feature);
            (function (Location) {
                Location[Location["Top"] = 0] = "Top";
                Location[Location["Bottom"] = 1] = "Bottom";
            })(Location || (Location = {}));
            exports_1("Location", Location);
            FeatureDefinitions = /** @class */function () {
                function FeatureDefinitions() {
                    this._defined = [new Feature(undefined, "Analytics", analytic_component_1.AnalyticIndex, 0, [], "analytics", Location.Top), new Feature("Analytics", "Dashboard", dashboard_component_1.DashboardComponent, 0, ["CloudGemMetric"], "analytics/dashboard", Location.Top, "View live game telemetry", "CloudGemMetric_optimized._CB492341798_.png"), new Feature("Analytics", "Heatmap", heatmap_component_1.HeatmapComponent, 1, ["CloudGemMetric"], "analytics/heatmap", Location.Top, "View heatmaps using your game data", "Heat_Map_Gem_Portal_Optinized._CB1517862571_.png"), new Feature(undefined, "Administration", admin_component_1.AdminComponent, 0, [], "administration", Location.Bottom), new Feature("Administration", "User Administration", user_admin_component_1.UserAdminComponent, 0, [], "admin/users", Location.Bottom, "Manage who has access to the Cloud Gem Portal", "user._CB536715123_.png"), new Feature("Administration", "Project Logs", log_component_1.ProjectLogComponent, 1, [], "admin/logs", Location.Bottom, "Project Stack Logs", "setting._CB536715120_.png"), new Feature("Administration", "JIRA Credentials", jira_credentials_component_1.JiraCredentialsComponent, 2, ["CloudGemDefectReporter"], "admin/jira", Location.Bottom, "View heatmaps using your game data", "Heat_Map_Gem_Portal_Optinized._CB1517862571_.png")];
                }
                Object.defineProperty(FeatureDefinitions.prototype, "defined", {
                    get: function () {
                        return this._defined;
                    },
                    enumerable: true,
                    configurable: true
                });
                return FeatureDefinitions;
            }();
            exports_1("FeatureDefinitions", FeatureDefinitions);
        }
    };
});
System.register("dist/app/view/game/component/sidebar.component.js", ["@angular/core", "@angular/router", "@angular/animations", "app/aws/aws.service", "app/shared/service/index", "app/view/game/module/shared/service/index", "app/view/game/module/shared/class/feature.class"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, router_1, animations_1, aws_service_1, index_1, index_2, feature_class_1, SidebarComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }, function (animations_1_1) {
            animations_1 = animations_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (feature_class_1_1) {
            feature_class_1 = feature_class_1_1;
        }],
        execute: function () {
            SidebarComponent = /** @class */function () {
                function SidebarComponent(aws, lymetrics, gemService, router, dependencyservice) {
                    this.aws = aws;
                    this.lymetrics = lymetrics;
                    this.gemService = gemService;
                    this.router = router;
                    this.dependencyservice = dependencyservice;
                    this.sidebarStateVal = "Expanded";
                    this.sidebarStateChange = new core_1.EventEmitter();
                }
                Object.defineProperty(SidebarComponent.prototype, "sidebarState", {
                    get: function () {
                        return this.sidebarStateVal;
                    },
                    set: function (val) {
                        this.sidebarStateVal = val;
                        this.sidebarStateChange.emit(this.sidebarStateVal);
                    },
                    enumerable: true,
                    configurable: true
                });
                SidebarComponent.prototype.ngOnInit = function () {
                    var _this = this;
                    var featureDefinitions = new feature_class_1.FeatureDefinitions();
                    this.dependencyservice.subscribeToDeploymentChanges(featureDefinitions);
                    this.dependencyservice.featureMapSubscription.subscribe(function (featureMap) {
                        _this.featureMap = featureMap;
                    });
                };
                // TODO: Implement a routing solution for metrics that covers the whole website.
                SidebarComponent.prototype.raiseRouteEvent = function (route) {
                    this.lymetrics.recordEvent('FeatureOpened', {
                        "Name": route
                    });
                    this.router.navigate([route]);
                };
                __decorate([core_1.Input(), __metadata("design:type", Object), __metadata("design:paramtypes", [Object])], SidebarComponent.prototype, "sidebarState", null);
                __decorate([core_1.Output(), __metadata("design:type", Object)], SidebarComponent.prototype, "sidebarStateChange", void 0);
                SidebarComponent = __decorate([core_1.Component({
                    selector: 'cgp-sidebar',
                    template: "<div class=\"sidebar-container\" [@sidebarState]=\"sidebarState\" [class.sidebar-collapse]=\"sidebarState === 'Collapsed'\">     <div class=\"sidebar-main\" [ngClass]=\"(sidebarState === 'Collapsed') ? 'collapsed' : 'expanded'\">         <div class=\"sidebar-links\">             <a routerLink=\"/game/cloudgems\" routerLinkActive=\"active\" [routerLinkActiveOptions]=\"{ exact: true }\">                 <i class=\"fa fa-home fa-lg\" aria-hidden=\"true\"></i>                 <span *ngIf=\"sidebarState === 'Expanded'\"> Cloud Gems </span>             </a>             <div *ngIf=\"sidebarState === 'Expanded'\">                 <ng-container [ngSwitch]=\"gemService.currentGems.length === 0 && !gemService.isLoading\">                     <!-- Case where there aren't any gems and we're done loading -->                     <div *ngSwitchCase=\"true\"> </div>                     <!-- Case where we might still be loading gems -->                     <div *ngSwitchCase=\"false\">                         <div class=\"child-links\" [ngSwitch]=\"gemService.isLoading\">                             <div *ngSwitchCase=\"true\">                                 <loading-spinner></loading-spinner>                             </div>                             <div *ngSwitchCase=\"false\">                                 <div *ngFor=\"let gem of gemService.currentGems\" class=\"sidebar-links\">                                     <a class=\"child-link\" (click)=\"raiseRouteEvent('/game/cloudgems/'+gem.identifier)\" routerLinkActive=\"active\">                                         {{gem.displayName}}                                     </a>                                 </div>                             </div>                         </div>                     </div>                 </ng-container>             </div>             <div *ngIf=\"featureMap != null && (featureMap | json) != ({} | json)\">                 <div *ngFor=\"let feature of featureMap | objKeys\">                     <div *ngIf=\"featureMap[feature].length > 0 && featureMap[feature][0].location == 0\">                         <a (click)=\"raiseRouteEvent('/game/'+feature.toLowerCase())\" routerLinkActive=\"active\">                             <i class=\"fa fa-area-chart fa-lg\" aria-hidden=\"true\"></i>                             <span *ngIf=\"sidebarState === 'Expanded'\"> {{ feature }} </span>                         </a>                         <div class=\"child-links\" *ngIf=\"sidebarState === 'Expanded'\">                             <div *ngFor=\"let child of featureMap[feature]\">                                 <a *ngIf=\"child.location == 0\" class=\"child-link\" (click)=\"raiseRouteEvent('/game/'+child.route_path)\" routerLinkActive=\"active\">                                     {{child.name}}                                 </a>                             </div>                         </div>                     </div>                 </div>             </div>             <a (click)=\"raiseRouteEvent('/game/support')\" routerLinkActive=\"active\">                 <i class=\"fa fa-life-ring fa-lg\" aria-hidden=\"true\"></i>                 <span *ngIf=\"sidebarState === 'Expanded'\"> Support </span>             </a>         </div>         <!-- Used to ensure the side bar footer doesn't overlay the top nav bar items when the windows size is small -->         <div class=\"push\" [ngClass]=\"(sidebarState === 'Collapsed') ? 'collapsed' : 'expanded'\"></div>     </div>     <div class=\"sidebar-footer\" [ngClass]=\"(sidebarState === 'Collapsed') ? 'collapsed' : 'expanded'\">         <div class=\"sidebar-links\">             <div *ngIf=\"featureMap != null && (featureMap | json) != ({} | json)\">                 <div *ngFor=\"let feature of featureMap | objKeys\">                     <div *ngIf=\"featureMap[feature].length > 0 && featureMap[feature][0].location == 1\">                         <a (click)=\"raiseRouteEvent('/game/'+feature.toLowerCase())\" routerLinkActive=\"active\">                             <i class=\"fa fa-cog fa-lg\" aria-hidden=\"true\"></i>                             <span *ngIf=\"sidebarState === 'Expanded'\"> {{ feature }} </span>                         </a>                         <div class=\"child-links\" *ngIf=\"sidebarState === 'Expanded'\">                             <div *ngFor=\"let child of featureMap[feature]\">                                 <a *ngIf=\"child.location == 1\" class=\"child-link\" (click)=\"raiseRouteEvent('/game/'+child.route_path)\" routerLinkActive=\"active\">                                     {{child.name}}                                 </a>                             </div>                         </div>                     </div>                 </div>             </div>         </div>         <div class=\"sidebar-close\" (click)=\"sidebarState = (sidebarState === 'Expanded') ? 'Collapsed' : 'Expanded'\">             <i [ngClass]=\"sidebarState === 'Expanded' ? 'fa-angle-double-left' : 'fa-angle-double-right'\" class=\"fa fa-angle-double-left fa-lg\" aria-hidden=\"true\"></i>         </div>     </div> </div>",
                    styles: [".push.expanded{height:300px}.push.collapsed{height:150px}.sidebar-container{position:fixed;top:0;bottom:0;height:100%;background-color:#f8f8f8;z-index:1;padding-top:75px;overflow:hidden}.sidebar-container .sidebar-main{min-height:100%}.sidebar-container .sidebar-main.expanded{margin-bottom:-190px}.sidebar-container .sidebar-main.collapsed{margin-bottom:-145px}.sidebar-container .sidebar-links a{position:relative;padding:20px 25px;margin:10px 0 5px 0;cursor:pointer;color:inherit;text-decoration:none;border-left:3px solid transparent;display:block}.sidebar-container .sidebar-links a:hover,.sidebar-container .sidebar-links a:active,.sidebar-container .sidebar-links a.hover,.sidebar-container .sidebar-links a.active,.sidebar-container .sidebar-links a .active{color:#6441A5}.sidebar-container .sidebar-links a.active{border-left:3px solid #6441A5}.sidebar-container .sidebar-links a i{color:inherit;top:14px}.sidebar-container .sidebar-links a span{color:inherit;font-family:\"AmazonEmber-Light\";font-size:16px;white-space:nowrap;overflow:hidden;top:8px;left:60px}.sidebar-container .sidebar-links a *{position:absolute}.sidebar-container .sidebar-links .child-links .child-link{padding:0;text-align:left;padding-left:60px;font-size:12px;margin:5px 0;font-family:\"AmazonEmber-Regular\";white-space:nowrap;overflow:hidden}.sidebar-container .sidebar-footer.expanded{height:190px}.sidebar-container .sidebar-footer.collapsed{height:145px}.sidebar-container .sidebar-footer .sidebar-links{border-top:1px solid #eee;border-bottom:1px solid #eee;margin-bottom:20px;padding-bottom:15px}.sidebar-container .sidebar-footer .sidebar-close{float:right;margin-right:25px}.sidebar-container.sidebar-collapse .sidebar-links a i{margin-right:0}.sidebar-container.sidebar-collapse .sidebar-footer .sidebar-close{float:none;margin-right:0;text-align:center}"],
                    animations: [animations_1.trigger('sidebarState', [animations_1.state('Collapsed', animations_1.style({
                        'width': '75px'
                    })), animations_1.state('Expanded', animations_1.style({
                        'width': '265px'
                    })), animations_1.transition('Collapsed => Expanded', animations_1.animate('300ms ease-in')), animations_1.transition('Expanded => Collapsed', animations_1.animate('300ms ease-out'))])]
                }), __metadata("design:paramtypes", [aws_service_1.AwsService, index_1.LyMetricService, index_1.GemService, router_1.Router, index_2.DependencyService])], SidebarComponent);
                return SidebarComponent;
            }();
            exports_1("SidebarComponent", SidebarComponent);
        }
    };
});
System.register("dist/app/view/game/game.module.js", ["@angular/core", "./game.routes", "app/shared/shared.module", "./module/cloudgems/gem.module", "./module/analytic/analytic.module", "./module/support/support.module", "./module/admin/admin.module", "./component/game.component", "./component/nav.component", "./component/sidebar.component"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, game_routes_1, shared_module_1, gem_module_1, analytic_module_1, support_module_1, admin_module_1, game_component_1, nav_component_1, sidebar_component_1, GameModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (game_routes_1_1) {
            game_routes_1 = game_routes_1_1;
        }, function (shared_module_1_1) {
            shared_module_1 = shared_module_1_1;
        }, function (gem_module_1_1) {
            gem_module_1 = gem_module_1_1;
        }, function (analytic_module_1_1) {
            analytic_module_1 = analytic_module_1_1;
        }, function (support_module_1_1) {
            support_module_1 = support_module_1_1;
        }, function (admin_module_1_1) {
            admin_module_1 = admin_module_1_1;
        }, function (game_component_1_1) {
            game_component_1 = game_component_1_1;
        }, function (nav_component_1_1) {
            nav_component_1 = nav_component_1_1;
        }, function (sidebar_component_1_1) {
            sidebar_component_1 = sidebar_component_1_1;
        }],
        execute: function () {
            GameModule = /** @class */function () {
                function GameModule() {}
                GameModule = __decorate([core_1.NgModule({
                    imports: [game_routes_1.GameRoutingModule, shared_module_1.AppSharedModule, support_module_1.SupportModule, admin_module_1.AdminModule, analytic_module_1.AnalyticModule, gem_module_1.GemModule],
                    declarations: [nav_component_1.NavComponent, sidebar_component_1.SidebarComponent, game_component_1.GameComponent]
                })], GameModule);
                return GameModule;
            }();
            exports_1("GameModule", GameModule);
        }
    };
});
System.register("dist/app/shared/pipe/model-debug.pipe.js", ["@angular/core", "../service/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, index_1, ModelDebugPipe;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }],
        execute: function () {
            ModelDebugPipe = /** @class */function () {
                function ModelDebugPipe(definition) {
                    this.definition = definition;
                }
                ModelDebugPipe.prototype.transform = function (value, args) {
                    if (this.definition.isProd) return;
                    if (this.definition.defines.debugModel == true) return value;
                    return;
                };
                ModelDebugPipe = __decorate([core_1.Pipe({ name: 'devonly' }), __metadata("design:paramtypes", [index_1.DefinitionService])], ModelDebugPipe);
                return ModelDebugPipe;
            }();
            exports_1("ModelDebugPipe", ModelDebugPipe);
        }
    };
});
System.register("dist/app/shared/pipe/object-keys.pipe.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, ObjectKeysPipe;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            ObjectKeysPipe = /** @class */function () {
                function ObjectKeysPipe() {}
                ObjectKeysPipe.prototype.transform = function (value, args) {
                    if (args === void 0) {
                        args = null;
                    }
                    return Object.keys(value);
                };
                ObjectKeysPipe = __decorate([core_1.Pipe({ name: 'objKeys' })], ObjectKeysPipe);
                return ObjectKeysPipe;
            }();
            exports_1("ObjectKeysPipe", ObjectKeysPipe);
        }
    };
});
System.register("dist/app/shared/pipe/from-epoch.pipe.js", ["@angular/core", "../class/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, index_1, FromEpochPipe;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }],
        execute: function () {
            FromEpochPipe = /** @class */function () {
                function FromEpochPipe() {}
                FromEpochPipe.prototype.transform = function (value, args) {
                    return index_1.DateTimeUtil.fromEpoch(Number.parseInt(value) / 1000);
                };
                FromEpochPipe = __decorate([core_1.Pipe({ name: 'toDateFromEpochInMilli' })], FromEpochPipe);
                return FromEpochPipe;
            }();
            exports_1("FromEpochPipe", FromEpochPipe);
        }
    };
});
System.register("dist/app/shared/pipe/array-filter.pipe.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, FilterArrayPipe;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            FilterArrayPipe = /** @class */function () {
                function FilterArrayPipe() {}
                FilterArrayPipe.prototype.transform = function (value, args) {
                    return value.filter(function (e) {
                        return e !== args[0];
                    });
                };
                FilterArrayPipe = __decorate([core_1.Pipe({ name: 'arrayFilter' })], FilterArrayPipe);
                return FilterArrayPipe;
            }();
            exports_1("FilterArrayPipe", FilterArrayPipe);
        }
    };
});
System.register("dist/app/shared/pipe/index.js", ["./model-debug.pipe", "./object-keys.pipe", "./from-epoch.pipe", "./array-filter.pipe"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    function exportStar_1(m) {
        var exports = {};
        for (var n in m) {
            if (n !== "default") exports[n] = m[n];
        }
        exports_1(exports);
    }
    return {
        setters: [function (model_debug_pipe_1_1) {
            exportStar_1(model_debug_pipe_1_1);
        }, function (object_keys_pipe_1_1) {
            exportStar_1(object_keys_pipe_1_1);
        }, function (from_epoch_pipe_1_1) {
            exportStar_1(from_epoch_pipe_1_1);
        }, function (array_filter_pipe_1_1) {
            exportStar_1(array_filter_pipe_1_1);
        }],
        execute: function () {}
    };
});
System.register("dist/app/shared/directive/anchor.directive.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, AnchorDirective;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            AnchorDirective = /** @class */function () {
                function AnchorDirective(viewContainerRef) {
                    this.viewContainerRef = viewContainerRef;
                }
                AnchorDirective = __decorate([core_1.Directive({
                    selector: '[anchor-host]'
                }), __metadata("design:paramtypes", [core_1.ViewContainerRef])], AnchorDirective);
                return AnchorDirective;
            }();
            exports_1("AnchorDirective", AnchorDirective);
        }
    };
});
System.register("dist/app/shared/component/auto-focus.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, AutoFocusComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            AutoFocusComponent = /** @class */function () {
                function AutoFocusComponent(elementRef) {
                    this.elementRef = elementRef;
                }
                ;
                AutoFocusComponent.prototype.ngOnInit = function () {
                    this.elementRef.nativeElement.focus();
                };
                AutoFocusComponent = __decorate([core_1.Directive({
                    selector: '[ngautofocus]'
                }), __metadata("design:paramtypes", [core_1.ElementRef])], AutoFocusComponent);
                return AutoFocusComponent;
            }();
            exports_1("AutoFocusComponent", AutoFocusComponent);
        }
    };
});
System.register("dist/app/shared/component/datetime-range-picker.component.js", ["@angular/core", "app/shared/class/datetime.class"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, datetime_class_1, DateTimeRangePickerComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (datetime_class_1_1) {
            datetime_class_1 = datetime_class_1_1;
        }],
        execute: function () {
            DateTimeRangePickerComponent = /** @class */function () {
                function DateTimeRangePickerComponent() {
                    this.dateTimeRange = new core_1.EventEmitter();
                    this.isStartRequired = false;
                    this.isEndRequired = false;
                    this.model = {
                        isNoEnd: true,
                        isNoStart: true,
                        hasRequiredStartDateTime: false,
                        hasRequiredEndDateTime: false
                    };
                }
                DateTimeRangePickerComponent.prototype.ngOnChanges = function (changes) {
                    this.init();
                };
                DateTimeRangePickerComponent.prototype.change = function () {
                    var result = {
                        start: {
                            date: this.model.dpStart,
                            time: this.model.stime
                        },
                        end: {
                            date: this.model.dpEnd,
                            time: this.model.etime
                        },
                        hasStart: !this.model.isNoStart,
                        hasEnd: !this.model.isNoEnd,
                        hasRequiredStartDateTime: this.model.isStartRequired,
                        hasRequiredEndDateTime: this.model.isEndRequired
                    };
                    this.validateDate(result);
                    this.dateTimeRange.emit(result);
                };
                DateTimeRangePickerComponent.prototype.ngOnInit = function () {
                    this.init();
                };
                DateTimeRangePickerComponent.prototype.init = function () {
                    if (this.hasStart == null) this.model.hasStart = false;
                    if (this.hasEnd == null) this.model.hasEnd = false;
                    if (this.startDateModel) {
                        this.model.dpStart = this.startDateModel;
                    }
                    if (this.endDateModel) {
                        this.model.dpEnd = this.endDateModel;
                    }
                    if (this.startTime) this.model.stime = this.startTime;
                    if (this.endTime) this.model.etime = this.endTime;
                    if (this.isStartRequired == null) {
                        this.model.isStartRequired = false;
                    } else {
                        this.model.isStartRequired = this.isStartRequired;
                    }
                    if (this.isEndRequired == null) {
                        this.model.isEndRequired = false;
                    } else {
                        this.model.isEndRequired = this.isEndRequired;
                    }
                    if (this.model.isStartRequired) {
                        this.model.isNoStart = false;
                    } else {
                        this.model.isNoStart = !this.hasStart;
                    }
                    if (this.model.isEndRequired) {
                        this.model.isNoEnd = false;
                    } else {
                        this.model.isNoEnd = !this.hasEnd;
                    }
                    this.change();
                };
                DateTimeRangePickerComponent.default = function (initializingdate) {
                    var today = new Date();
                    return {
                        date: {
                            year: datetime_class_1.DateTimeUtil.isValid(initializingdate) ? initializingdate.getFullYear() : today.getFullYear(),
                            month: datetime_class_1.DateTimeUtil.isValid(initializingdate) ? initializingdate.getMonth() + 1 : today.getMonth() + 1,
                            day: datetime_class_1.DateTimeUtil.isValid(initializingdate) ? initializingdate.getDate() : today.getDate()
                        },
                        time: datetime_class_1.DateTimeUtil.isValid(initializingdate) ? { hour: initializingdate.getHours(), minute: initializingdate.getMinutes() } : { hour: 12, minute: 0 },
                        valid: true
                    };
                };
                DateTimeRangePickerComponent.prototype.validateDate = function (model) {
                    var isValid = true;
                    var start = datetime_class_1.DateTimeUtil.toObjDate(model.start);
                    var end = datetime_class_1.DateTimeUtil.toObjDate(model.end);
                    model.start.valid = model.end.valid = true;
                    model.date = {};
                    if (this.hasEnd) {
                        model.end.valid = end ? true : false;
                        if (!model.end.valid) model.date = { message: "The end date is not a valid date.  A date must have a date, hour and minute." };
                        isValid = isValid && model.end.valid;
                    }
                    if (this.hasStart) {
                        model.start.valid = start ? true : false;
                        if (!model.start.valid) model.date = { message: "The start date is not a valid date.  A date must have a date, hour and minute." };
                        isValid = isValid && model.start.valid;
                    }
                    if (this.hasEnd && this.hasStart) {
                        var isValidDateRange = start < end;
                        isValid = isValid && isValidDateRange;
                        if (!isValidDateRange) {
                            model.date = { message: "The start date must be less than the end date." };
                            model.start.valid = false;
                        }
                    }
                    model.valid = isValid;
                };
                __decorate([core_1.Output(), __metadata("design:type", Object)], DateTimeRangePickerComponent.prototype, "dateTimeRange", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], DateTimeRangePickerComponent.prototype, "hasStart", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], DateTimeRangePickerComponent.prototype, "hasEnd", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], DateTimeRangePickerComponent.prototype, "startDateModel", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], DateTimeRangePickerComponent.prototype, "endDateModel", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], DateTimeRangePickerComponent.prototype, "startTime", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], DateTimeRangePickerComponent.prototype, "endTime", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], DateTimeRangePickerComponent.prototype, "isStartRequired", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], DateTimeRangePickerComponent.prototype, "isEndRequired", void 0);
                DateTimeRangePickerComponent = __decorate([core_1.Component({
                    selector: 'datetime-range-picker',
                    template: "<div class=\"date-time-picker-container\">      <label class=\"form-control-label\">Start:</label>     <div class=\"form-group form-inline row\">         <input class=\"form-control form-control-danger col-3 datepicker\" placeholder=\"yyyy-mm-dd\"             name=\"dps\" [(ngModel)]=\"model.dpStart\" ngbDatepicker #ds=\"ngbDatepicker\" (ngModelChange)=\"change()\"             [disabled]=\"model.isNoStart\" (click)=\"ds.toggle()\">         <ngb-timepicker [(ngModel)]=\"model.stime\"                          (ngModelChange)=\"change()\"                          [meridian]=\"true\"                          [disabled]=\"model.isNoStart\"                          [spinners]=\"false\"></ngb-timepicker>         <div class=\"form-check\" *ngIf=\"!model.isStartRequired\">             <span class=\"seperator\"> or </span>             <input id=\"isStart\" type=\"checkbox\" [(ngModel)]=\"model.isNoStart\" name=\"isNoStart\" (ngModelChange)=\"change()\" />             <label class=\"l-checkbox\" for=\"isStart\">                 No Start             </label>         </div>     </div> <label class=\"form-control-label\">End:</label>     <div class=\"form-group form-inline row\">         <input class=\"form-control form-control-danger col-3 datepicker\" placeholder=\"yyyy-mm-dd\"             name=\"dpe\" [(ngModel)]=\"model.dpEnd\" ngbDatepicker #de=\"ngbDatepicker\" (ngModelChange)=\"change()\"             [disabled]=\"model.isNoEnd\" (click)=\"de.toggle()\">         <ngb-timepicker [(ngModel)]=\"model.etime\"                          (ngModelChange)=\"change()\"                          [meridian]=\"true\"                          [disabled]=\"model.isNoEnd\"                          [spinners]=\"false\"></ngb-timepicker>         <div class=\"form-check\" *ngIf=\"!model.isEndRequired\">             <span class=\"seperator\"> or </span>             <input id=\"isEnd\" type=\"checkbox\" [(ngModel)]=\"model.isNoEnd\" name=\"isNoEnd\" (ngModelChange)=\"change()\" />             <label class=\"l-checkbox\" for=\"isEnd\">                 No End             </label>         </div>     </div>     </div> <!-- <pre>{{model | json | devonly}}</pre> -->",
                    styles: [".date-time-picker-container label{font-family:\"AmazonEmber-Light\"}.date-time-picker-container input.datepicker{height:32px;margin-right:10px}"]
                }), __metadata("design:paramtypes", [])], DateTimeRangePickerComponent);
                return DateTimeRangePickerComponent;
            }();
            exports_1("DateTimeRangePickerComponent", DateTimeRangePickerComponent);
        }
    };
});
System.register("dist/app/shared/component/footer.component.js", ["@angular/core", "@angular/animations"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, animations_1, FooterComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (animations_1_1) {
            animations_1 = animations_1_1;
        }],
        execute: function () {
            FooterComponent = /** @class */function () {
                function FooterComponent() {}
                __decorate([core_1.Input(), __metadata("design:type", String)], FooterComponent.prototype, "sidebarState", void 0);
                FooterComponent = __decorate([core_1.Component({
                    selector: 'cgp-footer',
                    template: "<div [@footerMargin]=\"sidebarState\" class=\"footer-container\"> \t<div class=\"footer-image\"> \t\t<img src=\"https://m.media-amazon.com/images/G/01/cloudcanvas/images/poweredbyly._V510110372_.png\"/> \t</div> </div>",
                    styles: ["\n        .footer-container {\n            height: 35px;\t\n            width: 100%;\n            text-align: center;\n        }\n    "],
                    animations: [animations_1.trigger('footerMargin', [animations_1.state('Collapsed', animations_1.style({
                        'margin-left': '20px'
                    })), animations_1.state('Expanded', animations_1.style({
                        'margin-left': '75px'
                    })), animations_1.transition('Collapsed => Expanded', animations_1.animate('300ms ease-in')), animations_1.transition('Expanded => Collapsed', animations_1.animate('300ms ease-out'))])]
                })], FooterComponent);
                return FooterComponent;
            }();
            exports_1("FooterComponent", FooterComponent);
        }
    };
});
System.register("dist/app/shared/component/tooltip.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, TooltipComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            TooltipComponent = /** @class */function () {
                function TooltipComponent() {
                    this.placement = "bottom";
                }
                __decorate([core_1.Input(), __metadata("design:type", String)], TooltipComponent.prototype, "tooltip", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], TooltipComponent.prototype, "placement", void 0);
                TooltipComponent = __decorate([core_1.Component({
                    selector: 'tooltip',
                    template: "<i class=\"fa fa-question-circle-o\" [placement]=\"placement\"\n    [ngbTooltip]=\"tooltip\"></i>",
                    styles: ["\n        i {\n            margin-left: 10px;\n            margin-top: 10px;\n        }\n    "]
                })], TooltipComponent);
                return TooltipComponent;
            }();
            exports_1("TooltipComponent", TooltipComponent);
        }
    };
});
System.register("dist/app/shared/component/loading-spinner.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, LoadingSpinnerComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            LoadingSpinnerComponent = /** @class */function () {
                function LoadingSpinnerComponent() {
                    this.size = 'sm';
                }
                __decorate([core_1.Input(), __metadata("design:type", String)], LoadingSpinnerComponent.prototype, "size", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], LoadingSpinnerComponent.prototype, "text", void 0);
                LoadingSpinnerComponent = __decorate([core_1.Component({
                    selector: 'loading-spinner',
                    template: "\n        <div class=\"text-center loading-spinner-container\" [ngSwitch]=\"size\">\n            <div *ngSwitchCase=\"'lg'\">\n                <img src=\"https://m.media-amazon.com/images/G/01/cloudcanvas/images/beaver_flat_animation_optimized_32._V518427760_.gif\" />\n                <p class=\"text-center\">\n                    Loading...\n                </p>\n            </div>\n            <div *ngIf=\"text\" class=\"loading-text\">\n                {{ text }}\n            </div>\n            <div *ngSwitchDefault class=\"small-loading-icon\">\n                <i class=\"fa fa-cog fa-spin fa-2x fa-fw\"></i>\n                <span class=\"sr-only\">Loading...</span>\n            </div>\n        </div>\n    ",
                    styles: ["\n        /* Loading spinner */\n\n        loading-spinner {\n            width: 100%;\n            text-align: center;\n        }\n        .small-loading-icon{\n            width: 125px !important;\n            height: 97.22px !important;\n            background-size: 125px 97.22px !important;\n            margin: 25px auto;\n        }\n    "],
                    encapsulation: core_1.ViewEncapsulation.None
                }), __metadata("design:paramtypes", [])], LoadingSpinnerComponent);
                return LoadingSpinnerComponent;
            }();
            exports_1("LoadingSpinnerComponent", LoadingSpinnerComponent);
        }
    };
});
System.register("dist/app/shared/service/api.service.js", ["@angular/core", "app/shared/class/index", "@angular/http", "app/aws/aws.service", "@angular/router"], function (exports_1, context_1) {
    "use strict";

    var __extends = this && this.__extends || function () {
        var extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
        };
        return function (d, b) {
            extendStatics(d, b);
            function __() {
                this.constructor = d;
            }
            d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
        };
    }();
    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, index_1, http_1, aws_service_1, router_1, ApiService, ProjectServiceAPI;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }],
        execute: function () {
            ApiService = /** @class */function () {
                function ApiService(aws, http, router) {}
                ApiService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [aws_service_1.AwsService, http_1.Http, router_1.Router])], ApiService);
                return ApiService;
            }();
            exports_1("ApiService", ApiService);
            ProjectServiceAPI = /** @class */function (_super) {
                __extends(ProjectServiceAPI, _super);
                function ProjectServiceAPI(serviceBaseURL, http, aws) {
                    return _super.call(this, serviceBaseURL, http, aws, null, "Project") || this;
                }
                ProjectServiceAPI.prototype.delete = function (id) {
                    return _super.prototype.delete.call(this, "portal/info/" + id);
                };
                ProjectServiceAPI.prototype.post = function (body) {
                    return _super.prototype.post.call(this, "portal/content", body);
                };
                ProjectServiceAPI.prototype.get = function () {
                    return _super.prototype.get.call(this, "service/status");
                };
                return ProjectServiceAPI;
            }(index_1.ApiHandler);
            exports_1("ProjectServiceAPI", ProjectServiceAPI);
        }
    };
});
System.register("dist/app/shared/service/auth-guard.service.js", ["@angular/core", "@angular/router", "app/aws/aws.service"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, router_1, aws_service_1, AuthGuardService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }],
        execute: function () {
            AuthGuardService = /** @class */function () {
                function AuthGuardService(router, aws) {
                    this.router = router;
                    this.aws = aws;
                }
                AuthGuardService.prototype.canActivate = function (route, state) {
                    return this.aws.context.authentication.isLoggedIn;
                };
                AuthGuardService.prototype.canActivateChild = function (route, state) {
                    return this.canActivate(route, state);
                };
                AuthGuardService.prototype.canLoad = function (route) {
                    var url = "/" + route.path;
                    return this.checkLogin(url);
                };
                AuthGuardService.prototype.checkLogin = function (url) {
                    return this.aws.context.authentication.isLoggedIn;
                };
                AuthGuardService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [router_1.Router, aws_service_1.AwsService])], AuthGuardService);
                return AuthGuardService;
            }();
            exports_1("AuthGuardService", AuthGuardService);
        }
    };
});
System.register("dist/app/shared/service/definition.service.js", ["@angular/core", "app/shared/class/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, environment, DirectoryService, DefinitionService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (environment_1) {
            environment = environment_1;
        }],
        execute: function () {
            DirectoryService = /** @class */function () {
                function DirectoryService(context, service_api_constructor) {
                    this._serviceApiConstructor = service_api_constructor;
                    this._context = context;
                }
                Object.defineProperty(DirectoryService.prototype, "constructor", {
                    get: function () {
                        return this._serviceApiConstructor;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(DirectoryService.prototype, "serviceUrl", {
                    get: function () {
                        return this._context['ServiceUrl'];
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(DirectoryService.prototype, "context", {
                    get: function () {
                        return this._context;
                    },
                    enumerable: true,
                    configurable: true
                });
                return DirectoryService;
            }();
            exports_1("DirectoryService", DirectoryService);
            DefinitionService = /** @class */function () {
                function DefinitionService() {
                    this.isProd = environment.isProd;
                    this._dev_defines = {};
                    this._prod_defines = {};
                    if (this.isProd) {
                        this._current_defines = this._prod_defines;
                    } else {
                        this._current_defines = this._dev_defines;
                        this._current_defines.isDev = true;
                    }
                    this._directoryService = new Map();
                    this._current_defines.debugModel = true;
                }
                Object.defineProperty(DefinitionService.prototype, "defines", {
                    get: function () {
                        return this._current_defines;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(DefinitionService.prototype, "isTest", {
                    get: function () {
                        return environment.isTest;
                    },
                    enumerable: true,
                    configurable: true
                });
                /*
                *  Add a new service to the service listing for other gems to utilize.
                */
                DefinitionService.prototype.addService = function (id, context, service_api_constructor) {
                    this._directoryService[id.toLowerCase()] = new DirectoryService(context, service_api_constructor);
                };
                /*
                *  Get a specific gem service to utilize from your gem
                */
                DefinitionService.prototype.getService = function (name) {
                    return this._directoryService[name.toLowerCase()];
                };
                DefinitionService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [])], DefinitionService);
                return DefinitionService;
            }();
            exports_1("DefinitionService", DefinitionService);
        }
    };
});
System.register("dist/app/shared/service/selective-preload-strategy.service.js", ["rxjs/add/observable/of", "rxjs/Observable", "@angular/core", "./definition.service"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var Observable_1, core_1, definition_service_1, PreloadingService;
    return {
        setters: [function (_1) {}, function (Observable_1_1) {
            Observable_1 = Observable_1_1;
        }, function (core_1_1) {
            core_1 = core_1_1;
        }, function (definition_service_1_1) {
            definition_service_1 = definition_service_1_1;
        }],
        execute: function () {
            PreloadingService = /** @class */function () {
                function PreloadingService(defintion) {
                    this.defintion = defintion;
                    this.preloadedModules = [];
                }
                PreloadingService.prototype.preload = function (route, load) {
                    if (route.data && route.data['preload']) {
                        // add the route path to our preloaded module array
                        this.preloadedModules.push(route.path);
                        // log the route path to the console
                        if (!this.defintion.isProd) console.log('Preloaded: ' + route.path);
                        return load();
                    } else {
                        return Observable_1.Observable.of(null);
                    }
                };
                PreloadingService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [definition_service_1.DefinitionService])], PreloadingService);
                return PreloadingService;
            }();
            exports_1("PreloadingService", PreloadingService);
        }
    };
});
System.register("dist/app/aws/context.class.js", ["rxjs/BehaviorSubject"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var BehaviorSubject_1, EnumContextState, AwsContext;
    return {
        setters: [function (BehaviorSubject_1_1) {
            BehaviorSubject_1 = BehaviorSubject_1_1;
        }],
        execute: function () {
            (function (EnumContextState) {
                EnumContextState[EnumContextState["INITIALIZED"] = 0] = "INITIALIZED";
                EnumContextState[EnumContextState["CLIENT_UPDATED"] = 1] = "CLIENT_UPDATED";
                EnumContextState[EnumContextState["PROJECT_UPDATED"] = 2] = "PROJECT_UPDATED";
                EnumContextState[EnumContextState["DEPLOYMENT_UPDATED"] = 4] = "DEPLOYMENT_UPDATED";
                EnumContextState[EnumContextState["CREDENTIAL_UPDATED"] = 8] = "CREDENTIAL_UPDATED";
            })(EnumContextState || (EnumContextState = {}));
            exports_1("EnumContextState", EnumContextState);
            AwsContext = /** @class */function () {
                function AwsContext() {
                    this._projectName = '';
                    this._change = new BehaviorSubject_1.BehaviorSubject({});
                }
                Object.defineProperty(AwsContext.prototype, "name", {
                    get: function () {
                        return this._projectName;
                    },
                    set: function (value) {
                        if (value === undefined || value === null) return;
                        var configBucketParts = value.split('-configuration-');
                        this._projectName = configBucketParts[0];
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "cognitoUserPool", {
                    get: function () {
                        return this._cognitoUserPool;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "clientId", {
                    get: function () {
                        return this._cognitoClientId;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "apiGateway", {
                    get: function () {
                        return this._apigateway;
                    },
                    set: function (value) {
                        this._apigateway = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "cognitoIdentity", {
                    get: function () {
                        return this._cognitoIdentity;
                    },
                    set: function (value) {
                        this._cognitoIdentity = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "cloudWatchLogs", {
                    get: function () {
                        return this._cloudWatchLogs;
                    },
                    set: function (value) {
                        this._cloudWatchLogs = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "cognitoIdentityService", {
                    get: function () {
                        return this._cognitoIdentityService;
                    },
                    set: function (value) {
                        this._cognitoIdentityService = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "awsClient", {
                    get: function () {
                        return this._awsClient;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "configBucket", {
                    get: function () {
                        return this._configBucket;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "region", {
                    get: function () {
                        return this._region;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "config", {
                    get: function () {
                        return AWS.config;
                    },
                    set: function (value) {
                        AWS.config.update(value);
                        this.transition(EnumContextState.CLIENT_UPDATED);
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "s3", {
                    get: function () {
                        return this._s3;
                    },
                    set: function (value) {
                        this._s3 = value;
                        this.transition(EnumContextState.CLIENT_UPDATED);
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "authentication", {
                    get: function () {
                        return this._authentication;
                    },
                    set: function (value) {
                        this._authentication = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "userManagement", {
                    get: function () {
                        return this._usermanagement;
                    },
                    set: function (value) {
                        this._usermanagement = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "cloudFormation", {
                    get: function () {
                        return this._cloudFormation;
                    },
                    set: function (value) {
                        this._cloudFormation = value;
                        this.transition(EnumContextState.CLIENT_UPDATED);
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "deployment", {
                    get: function () {
                        return this._deployment;
                    },
                    set: function (value) {
                        this._deployment = value;
                        this.transition(EnumContextState.DEPLOYMENT_UPDATED);
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "project", {
                    get: function () {
                        return this._project;
                    },
                    set: function (value) {
                        this._project = value;
                        this.transition(EnumContextState.PROJECT_UPDATED);
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "change", {
                    get: function () {
                        return this._change.asObservable();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "userPoolId", {
                    get: function () {
                        return this._userPoolId;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsContext.prototype, "identityPoolId", {
                    get: function () {
                        return this._identityPoolId;
                    },
                    enumerable: true,
                    configurable: true
                });
                AwsContext.prototype.init = function (userPoolId, clientId, identityPoolId, configBucket, region) {
                    this.name = configBucket;
                    this._configBucket = configBucket;
                    this._region = region;
                    this._userPoolId = userPoolId;
                    this._cognitoClientId = clientId;
                    this._identityPoolId = identityPoolId;
                    this._cognitoUserPool = new AWSCognito.CognitoIdentityServiceProvider.CognitoUserPool({
                        UserPoolId: this._userPoolId,
                        ClientId: this._cognitoClientId
                    });
                    var config = {
                        region: region,
                        credentials: new AWS.CognitoIdentityCredentials({
                            IdentityPoolId: identityPoolId
                        }, {
                            region: region
                        }),
                        sslEnabled: true,
                        signatureVersion: 'v4'
                    };
                    this.config = config;
                    this._awsClient = function (serviceName, options) {
                        return new AWS[serviceName](options);
                    };
                };
                AwsContext.prototype.initializeServices = function () {
                    this.s3 = this.awsClient("S3", { apiVersion: "2006-03-01", region: this.region });
                    this.cloudFormation = this.awsClient("CloudFormation", { apiVersion: "2010-05-15", region: this.region });
                    this.cognitoIdentityService = this.awsClient("CognitoIdentityServiceProvider", { apiVersion: "2016-04-18", region: this.region });
                    this.cognitoIdentity = this.awsClient("CognitoIdentity", { apiVersion: "2014-06-30", region: this.region });
                    this.apiGateway = this.awsClient("APIGateway", { apiVersion: "2015-07-09", region: this.region });
                    this.cloudWatchLogs = this.awsClient("CloudWatchLogs", { apiVersion: "2014-03-28", region: this.region });
                };
                AwsContext.prototype.transition = function (to) {
                    this._change.next({
                        state: to,
                        context: this
                    });
                };
                return AwsContext;
            }();
            exports_1("AwsContext", AwsContext);
        }
    };
});
System.register("dist/app/aws/authentication/action/login.class.js", ["../authentication.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var authentication_class_1, LoginAction;
    return {
        setters: [function (authentication_class_1_1) {
            authentication_class_1 = authentication_class_1_1;
        }],
        execute: function () {
            LoginAction = /** @class */function () {
                function LoginAction(context) {
                    this.context = context;
                }
                LoginAction.prototype.handle = function (subject) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    var username = args[0];
                    if (username === undefined || username === null || username === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.LOGIN_FAILURE,
                            output: ["Cognito username was undefined."]
                        });
                        return;
                    }
                    var password = args[1];
                    if (password === undefined || password === null || password === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.LOGIN_FAILURE,
                            output: ["Cognito password was undefined."]
                        });
                        return;
                    }
                    var authenticationData = {
                        Username: username.trim(),
                        Password: password.trim()
                    };
                    var authenticationDetails = new AWSCognito.CognitoIdentityServiceProvider.AuthenticationDetails(authenticationData);
                    var userdata = {
                        Username: authenticationData.Username,
                        Pool: this.context.cognitoUserPool
                    };
                    var cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata);
                    cognitouser.authenticateUser(authenticationDetails, {
                        onSuccess: function (result) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.LOGGED_IN,
                                output: [result]
                            });
                        },
                        onFailure: function (err) {
                            if (err.code == "UserNotConfirmedException" && err.name == "NotAuthorizedException") {
                                subject.next({
                                    state: authentication_class_1.EnumAuthState.LOGIN_FAILURE_INCORRECT_USERNAME_PASSWORD,
                                    output: [cognitouser]
                                });
                                return;
                            }
                            if (err.code == "PasswordResetRequiredException") {
                                subject.next({
                                    state: authentication_class_1.EnumAuthState.PASSWORD_RESET_BY_ADMIN_CONFIRM_CODE,
                                    output: [cognitouser]
                                });
                                return;
                            }
                            if (err.code == "UserNotConfirmedException") {
                                subject.next({
                                    state: authentication_class_1.EnumAuthState.LOGIN_CONFIRMATION_NEEDED,
                                    output: [cognitouser]
                                });
                                return;
                            }
                            subject.next({
                                state: authentication_class_1.EnumAuthState.LOGIN_FAILURE,
                                output: [err]
                            });
                        },
                        newPasswordRequired: function (userattributes, requiredattributes) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.PASSWORD_CHANGE,
                                output: [cognitouser, userattributes, requiredattributes]
                            });
                        }
                    });
                };
                return LoginAction;
            }();
            exports_1("LoginAction", LoginAction);
        }
    };
});
System.register("dist/app/aws/authentication/action/logout.class.js", ["../authentication.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var authentication_class_1, LogoutAction;
    return {
        setters: [function (authentication_class_1_1) {
            authentication_class_1 = authentication_class_1_1;
        }],
        execute: function () {
            LogoutAction = /** @class */function () {
                function LogoutAction(context) {
                    this.context = context;
                }
                LogoutAction.prototype.handle = function (subject) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    var user = args[0];
                    if (user === undefined || user === null || user === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.LOGOUT_FAILURE,
                            output: ["Cognito user was undefined."]
                        });
                        return;
                    }
                    user.signOut();
                    if (AWS.config.credentials != null) {
                        AWS.config.credentials.clearCachedId();
                        AWS.config.credentials.expired = true;
                        AWS.config.credentials = null;
                    }
                    subject.next({
                        state: authentication_class_1.EnumAuthState.LOGGED_OUT,
                        output: [args[0]]
                    });
                };
                return LogoutAction;
            }();
            exports_1("LogoutAction", LogoutAction);
        }
    };
});
System.register("dist/app/aws/authentication/action/assume-role.class.js", ["../authentication.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var authentication_class_1, AssumeRoleAction;
    return {
        setters: [function (authentication_class_1_1) {
            authentication_class_1 = authentication_class_1_1;
        }],
        execute: function () {
            AssumeRoleAction = /** @class */function () {
                function AssumeRoleAction(context) {
                    this.context = context;
                }
                AssumeRoleAction.prototype.handle = function (subject) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    var rolearn = args[0];
                    if (rolearn === undefined || rolearn === null || rolearn === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.ASSUME_ROLE_FAILED,
                            output: ["Cognito role ARN was undefined."]
                        });
                        return;
                    }
                    var identityId = args[1];
                    if (identityId === undefined || identityId === null || identityId === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.ASSUME_ROLE_FAILED,
                            output: ["Cognito identity Id was undefined."]
                        });
                        return;
                    }
                    var idToken = args[2];
                    if (idToken === undefined || idToken === null || idToken === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.ASSUME_ROLE_FAILED,
                            output: ["Cognito identity Id was undefined."]
                        });
                        return;
                    }
                    var logins = {};
                    logins['cognito-idp.' + this.context.region + '.amazonaws.com/' + this.context.userPoolId] = idToken;
                    var params = {
                        IdentityId: identityId,
                        CustomRoleArn: rolearn,
                        Logins: (_a = {}, _a['cognito-idp.' + this.context.region + '.amazonaws.com/' + this.context.userPoolId] = idToken, _a)
                    };
                    console.log(params);
                    var cognitoidentity = new AWS.CognitoIdentity();
                    cognitoidentity.getCredentialsForIdentity(params, function (err, data) {
                        if (err) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.ASSUME_ROLE_FAILED,
                                output: [err]
                            });
                            return;
                        }
                        console.log(data);
                        subject.next({
                            state: authentication_class_1.EnumAuthState.ASSUME_ROLE_SUCCESS,
                            output: [data]
                        });
                    });
                    var _a;
                };
                return AssumeRoleAction;
            }();
            exports_1("AssumeRoleAction", AssumeRoleAction);
        }
    };
});
System.register("dist/app/aws/authentication/action/update-credential.class.js", ["../authentication.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var authentication_class_1, UpdateCredentialAction;
    return {
        setters: [function (authentication_class_1_1) {
            authentication_class_1 = authentication_class_1_1;
        }],
        execute: function () {
            UpdateCredentialAction = /** @class */function () {
                function UpdateCredentialAction(context) {
                    this.context = context;
                }
                UpdateCredentialAction.prototype.handle = function (subject) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    var idToken = args[0];
                    if (idToken === undefined || idToken === null || idToken === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED,
                            output: ["Cognito session token was undefined."]
                        });
                        return;
                    }
                    var logins = {};
                    logins['cognito-idp.' + this.context.region + '.amazonaws.com/' + this.context.userPoolId] = idToken;
                    var creds = new AWS.CognitoIdentityCredentials({
                        IdentityPoolId: this.context.identityPoolId,
                        Logins: logins
                    });
                    AWS.config.credentials = creds;
                    var callback = function (err) {
                        if (err || AWS.config.credentials == null) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED,
                                output: [err.code + " : " + err.message]
                            });
                            return;
                        }
                        subject.next({
                            state: authentication_class_1.EnumAuthState.USER_CREDENTIAL_UPDATED,
                            output: [idToken, AWS.config.credentials.identityId, AWS.config.credentials]
                        });
                    };
                    if (AWS.config.credentials == null) {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED,
                            output: ["Credentials no longer exist."]
                        });
                    } else {
                        AWS.config.credentials.refresh(function (err) {
                            if (AWS.config.credentials.accessKeyId === undefined) {
                                AWS.config.credentials.refresh(function (err) {
                                    callback(err);
                                });
                                return;
                            }
                            callback(err);
                        });
                    }
                };
                return UpdateCredentialAction;
            }();
            exports_1("UpdateCredentialAction", UpdateCredentialAction);
        }
    };
});
System.register("dist/app/aws/authentication/action/update-password.class.js", ["../authentication.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var authentication_class_1, UpdatePasswordAction;
    return {
        setters: [function (authentication_class_1_1) {
            authentication_class_1 = authentication_class_1_1;
        }],
        execute: function () {
            UpdatePasswordAction = /** @class */function () {
                function UpdatePasswordAction(context) {
                    this.context = context;
                }
                UpdatePasswordAction.prototype.handle = function (subject) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    var user = args[0];
                    if (user === undefined || user === null || user === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.PASSWORD_CHANGE_FAILED,
                            output: ["Cognito user was undefined."]
                        });
                        return;
                    }
                    var password = args[1];
                    if (password === undefined || password === null || password === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.PASSWORD_CHANGE_FAILED,
                            output: ["Cognito password was undefined."]
                        });
                        return;
                    }
                    // get these details and call
                    user.completeNewPasswordChallenge(password.trim(), [], {
                        onSuccess: function (result) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.PASSWORD_CHANGED,
                                output: [result]
                            });
                        },
                        onFailure: function (err) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.PASSWORD_CHANGE_FAILED,
                                output: [err, user]
                            });
                        }
                    });
                };
                return UpdatePasswordAction;
            }();
            exports_1("UpdatePasswordAction", UpdatePasswordAction);
        }
    };
});
System.register("dist/app/aws/authentication/action/forgot-password.class.js", ["../authentication.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var authentication_class_1, ForgotPasswordAction;
    return {
        setters: [function (authentication_class_1_1) {
            authentication_class_1 = authentication_class_1_1;
        }],
        execute: function () {
            ForgotPasswordAction = /** @class */function () {
                function ForgotPasswordAction(context) {
                    this.context = context;
                }
                ForgotPasswordAction.prototype.handle = function (subject) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    var username = args[0];
                    if (username === undefined || username === null || username === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_FAILURE,
                            output: ["Cognito username was undefined."]
                        });
                        return;
                    }
                    var userdata = {
                        Username: username.trim(),
                        Pool: this.context.cognitoUserPool
                    };
                    var cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata);
                    cognitouser.forgotPassword({
                        onSuccess: function (result) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_SUCCESS,
                                output: [result]
                            });
                        },
                        onFailure: function (err) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_FAILURE,
                                output: [err]
                            });
                        },
                        inputVerificationCode: function (result) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRM_CODE,
                                output: [result]
                            });
                        }
                    });
                };
                return ForgotPasswordAction;
            }();
            exports_1("ForgotPasswordAction", ForgotPasswordAction);
        }
    };
});
System.register("dist/app/aws/authentication/action/forgot-password-confirm-password.class.js", ["../authentication.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var authentication_class_1, ForgotPasswordConfirmNewPasswordAction;
    return {
        setters: [function (authentication_class_1_1) {
            authentication_class_1 = authentication_class_1_1;
        }],
        execute: function () {
            ForgotPasswordConfirmNewPasswordAction = /** @class */function () {
                function ForgotPasswordConfirmNewPasswordAction(context) {
                    this.context = context;
                }
                ForgotPasswordConfirmNewPasswordAction.prototype.handle = function (subject) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    var username = args[0];
                    if (username === undefined || username === null || username === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE,
                            output: ["Cognito username was undefined."]
                        });
                        return;
                    }
                    var password = args[1];
                    if (password === undefined || password === null || password === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE,
                            output: ["Cognito password was undefined."]
                        });
                        return;
                    }
                    var code = args[2];
                    if (code === undefined || code === null || code === '') {
                        subject.next({
                            state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE,
                            output: ["Cognito verification code was undefined."]
                        });
                        return;
                    }
                    var userdata = {
                        Username: username.trim(),
                        Pool: this.context.cognitoUserPool
                    };
                    var cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata);
                    cognitouser.confirmPassword(code, password.trim(), {
                        onSuccess: function () {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_SUCCESS,
                                output: []
                            });
                        },
                        onFailure: function (err) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRMATION_FAILURE,
                                output: [err]
                            });
                        },
                        inputVerificationCode: function (result) {
                            subject.next({
                                state: authentication_class_1.EnumAuthState.FORGOT_PASSWORD_CONFIRM_CODE,
                                output: [result]
                            });
                        }
                    });
                };
                return ForgotPasswordConfirmNewPasswordAction;
            }();
            exports_1("ForgotPasswordConfirmNewPasswordAction", ForgotPasswordConfirmNewPasswordAction);
        }
    };
});
System.register("dist/app/aws/authentication/authentication.class.js", ["rxjs/Observable", "rxjs/Subject", "./action/login.class", "./action/logout.class", "./action/assume-role.class", "./action/update-credential.class", "./action/update-password.class", "./action/forgot-password.class", "./action/forgot-password-confirm-password.class", "app/aws/user/user-management.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var Observable_1, Subject_1, login_class_1, logout_class_1, assume_role_class_1, update_credential_class_1, update_password_class_1, forgot_password_class_1, forgot_password_confirm_password_class_1, user_management_class_1, EnumAuthState, EnumAuthStateAction, Authentication;
    return {
        setters: [function (Observable_1_1) {
            Observable_1 = Observable_1_1;
        }, function (Subject_1_1) {
            Subject_1 = Subject_1_1;
        }, function (login_class_1_1) {
            login_class_1 = login_class_1_1;
        }, function (logout_class_1_1) {
            logout_class_1 = logout_class_1_1;
        }, function (assume_role_class_1_1) {
            assume_role_class_1 = assume_role_class_1_1;
        }, function (update_credential_class_1_1) {
            update_credential_class_1 = update_credential_class_1_1;
        }, function (update_password_class_1_1) {
            update_password_class_1 = update_password_class_1_1;
        }, function (forgot_password_class_1_1) {
            forgot_password_class_1 = forgot_password_class_1_1;
        }, function (forgot_password_confirm_password_class_1_1) {
            forgot_password_confirm_password_class_1 = forgot_password_confirm_password_class_1_1;
        }, function (user_management_class_1_1) {
            user_management_class_1 = user_management_class_1_1;
        }],
        execute: function () {
            (function (EnumAuthState) {
                EnumAuthState[EnumAuthState["INITIALIZED"] = 0] = "INITIALIZED";
                EnumAuthState[EnumAuthState["LOGGED_IN"] = 1] = "LOGGED_IN";
                EnumAuthState[EnumAuthState["LOGGED_OUT"] = 2] = "LOGGED_OUT";
                EnumAuthState[EnumAuthState["FORGOT_PASSWORD"] = 3] = "FORGOT_PASSWORD";
                EnumAuthState[EnumAuthState["PASSWORD_CHANGE"] = 4] = "PASSWORD_CHANGE";
                EnumAuthState[EnumAuthState["PASSWORD_CHANGE_FAILED"] = 5] = "PASSWORD_CHANGE_FAILED";
                EnumAuthState[EnumAuthState["PASSWORD_CHANGED"] = 6] = "PASSWORD_CHANGED";
                EnumAuthState[EnumAuthState["LOGIN_FAILURE"] = 7] = "LOGIN_FAILURE";
                EnumAuthState[EnumAuthState["USER_CREDENTIAL_UPDATED"] = 8] = "USER_CREDENTIAL_UPDATED";
                EnumAuthState[EnumAuthState["USER_CREDENTIAL_UPDATE_FAILED"] = 9] = "USER_CREDENTIAL_UPDATE_FAILED";
                EnumAuthState[EnumAuthState["LOGIN_CONFIRMATION_NEEDED"] = 10] = "LOGIN_CONFIRMATION_NEEDED";
                EnumAuthState[EnumAuthState["LOGOUT_FAILURE"] = 11] = "LOGOUT_FAILURE";
                EnumAuthState[EnumAuthState["ASSUME_ROLE_SUCCESS"] = 12] = "ASSUME_ROLE_SUCCESS";
                EnumAuthState[EnumAuthState["ASSUME_ROLE_FAILED"] = 13] = "ASSUME_ROLE_FAILED";
                EnumAuthState[EnumAuthState["LOGIN_FAILURE_INCORRECT_USERNAME_PASSWORD"] = 14] = "LOGIN_FAILURE_INCORRECT_USERNAME_PASSWORD";
                EnumAuthState[EnumAuthState["FORGOT_PASSWORD_CONFIRM_CODE"] = 15] = "FORGOT_PASSWORD_CONFIRM_CODE";
                EnumAuthState[EnumAuthState["FORGOT_PASSWORD_FAILURE"] = 16] = "FORGOT_PASSWORD_FAILURE";
                EnumAuthState[EnumAuthState["FORGOT_PASSWORD_SUCCESS"] = 17] = "FORGOT_PASSWORD_SUCCESS";
                EnumAuthState[EnumAuthState["FORGOT_PASSWORD_CONFIRMATION_FAILURE"] = 18] = "FORGOT_PASSWORD_CONFIRMATION_FAILURE";
                EnumAuthState[EnumAuthState["FORGOT_PASSWORD_CONFIRMATION_SUCCESS"] = 19] = "FORGOT_PASSWORD_CONFIRMATION_SUCCESS";
                EnumAuthState[EnumAuthState["PASSWORD_RESET_BY_ADMIN_CONFIRM_CODE"] = 20] = "PASSWORD_RESET_BY_ADMIN_CONFIRM_CODE";
            })(EnumAuthState || (EnumAuthState = {}));
            exports_1("EnumAuthState", EnumAuthState);
            (function (EnumAuthStateAction) {
                EnumAuthStateAction[EnumAuthStateAction["LOGIN"] = 0] = "LOGIN";
                EnumAuthStateAction[EnumAuthStateAction["LOGOUT"] = 1] = "LOGOUT";
                EnumAuthStateAction[EnumAuthStateAction["UPDATE_PASSWORD"] = 2] = "UPDATE_PASSWORD";
                EnumAuthStateAction[EnumAuthStateAction["UPDATE_CREDENTIAL"] = 3] = "UPDATE_CREDENTIAL";
                EnumAuthStateAction[EnumAuthStateAction["ASSUME_ROLE"] = 4] = "ASSUME_ROLE";
                EnumAuthStateAction[EnumAuthStateAction["FORGOT_PASSWORD"] = 5] = "FORGOT_PASSWORD";
                EnumAuthStateAction[EnumAuthStateAction["FORGOT_PASSWORD_CONFIRM_NEW_PASSWORD"] = 6] = "FORGOT_PASSWORD_CONFIRM_NEW_PASSWORD";
            })(EnumAuthStateAction || (EnumAuthStateAction = {}));
            exports_1("EnumAuthStateAction", EnumAuthStateAction);
            Authentication = /** @class */function () {
                function Authentication(context, router, metric) {
                    var _this = this;
                    this.context = context;
                    this.router = router;
                    this.metric = metric;
                    this._actions = [];
                    this._actions[EnumAuthStateAction.LOGIN] = new login_class_1.LoginAction(this.context);
                    this._actions[EnumAuthStateAction.LOGOUT] = new logout_class_1.LogoutAction(this.context);
                    this._actions[EnumAuthStateAction.UPDATE_CREDENTIAL] = new update_credential_class_1.UpdateCredentialAction(this.context);
                    this._actions[EnumAuthStateAction.ASSUME_ROLE] = new assume_role_class_1.AssumeRoleAction(this.context);
                    this._actions[EnumAuthStateAction.UPDATE_PASSWORD] = new update_password_class_1.UpdatePasswordAction(this.context);
                    this._actions[EnumAuthStateAction.FORGOT_PASSWORD] = new forgot_password_class_1.ForgotPasswordAction(this.context);
                    this._actions[EnumAuthStateAction.FORGOT_PASSWORD_CONFIRM_NEW_PASSWORD] = new forgot_password_confirm_password_class_1.ForgotPasswordConfirmNewPasswordAction(this.context);
                    this.refreshSessionOrLogout = this.refreshSessionOrLogout.bind(this);
                    this.hookUpdateCredentials = this.hookUpdateCredentials.bind(this);
                    this.updateCredentials = this.updateCredentials.bind(this);
                    this._actionObservable = new Subject_1.Subject();
                    this.change.subscribe(function (context) {
                        if (context.state === EnumAuthState.LOGGED_IN || context.state === EnumAuthState.PASSWORD_CHANGED) {
                            _this._session = context.output[0];
                            _this.updateCredentials();
                        } else if (context.state === EnumAuthState.LOGGED_OUT) {
                            _this._session = undefined;
                            _this.router.navigate(["/login"]);
                        } else if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATED) {
                            _this._identityId = context.output[1];
                            _this._user = _this.context.cognitoUserPool.getCurrentUser();
                            _this.user.refreshSession = _this.user.refreshSession.bind(_this);
                        }
                    });
                }
                Object.defineProperty(Authentication.prototype, "change", {
                    get: function () {
                        return this._actionObservable.asObservable();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "user", {
                    get: function () {
                        return this._user;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "accessToken", {
                    get: function () {
                        if (this._session === undefined) return;
                        return this._session.getAccessToken().getJwtToken();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "idToken", {
                    get: function () {
                        if (this._session === undefined) return;
                        return this._session.getIdToken().getJwtToken();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "refreshToken", {
                    get: function () {
                        if (this._session === undefined) return;
                        return this._session.getRefreshToken().getJwtToken();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "refreshCognitoToken", {
                    get: function () {
                        if (this._session === undefined) return;
                        return this._session.getRefreshToken();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "isLoggedIn", {
                    get: function () {
                        return this.idToken ? true : false;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "session", {
                    get: function () {
                        return this._session;
                    },
                    set: function (value) {
                        this._session = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "isSessionValid", {
                    get: function () {
                        return this._session != null && this._session.isValid();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "identityId", {
                    get: function () {
                        return this._identityId;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Authentication.prototype, "isAdministrator", {
                    get: function () {
                        var obj = this.parseJwt(this.idToken);
                        var admin_group = obj['cognito:groups'].filter(function (group) {
                            return group == user_management_class_1.EnumGroupName[user_management_class_1.EnumGroupName.ADMINISTRATOR].toLowerCase();
                        });
                        return admin_group.length > 0;
                    },
                    enumerable: true,
                    configurable: true
                });
                Authentication.prototype.updateCredentials = function () {
                    this.execute(EnumAuthStateAction.UPDATE_CREDENTIAL, this._session.getIdToken().getJwtToken());
                };
                Authentication.prototype.login = function (username, password) {
                    this.execute(EnumAuthStateAction.LOGIN, username, password);
                };
                Authentication.prototype.forgotPassword = function (username) {
                    this.execute(EnumAuthStateAction.FORGOT_PASSWORD, username);
                };
                Authentication.prototype.forgotPasswordConfirmNewPassword = function (username, code, password) {
                    this.execute(EnumAuthStateAction.FORGOT_PASSWORD_CONFIRM_NEW_PASSWORD, username, password, code);
                };
                Authentication.prototype.logout = function (isforced) {
                    if (isforced === void 0) {
                        isforced = false;
                    }
                    this.metric.recordEvent("LoggedOut", {
                        "ForcedLogout": isforced.toString(),
                        "Class": this.constructor["name"]
                    }, null);
                    this.execute(EnumAuthStateAction.LOGOUT, this.user);
                };
                Authentication.prototype.assumeRole = function (rolearn) {
                    this.execute(EnumAuthStateAction.ASSUME_ROLE, rolearn, this.identityId, this.idToken);
                };
                Authentication.prototype.updatePassword = function (user, password) {
                    this.execute(EnumAuthStateAction.UPDATE_PASSWORD, user, password);
                };
                Authentication.prototype.refreshSessionOrLogout = function (observable) {
                    if (observable === void 0) {
                        observable = null;
                    }
                    var auth = this;
                    return new Observable_1.Observable(function (observer) {
                        //no current user is present
                        //BUG: currently the local cookies for the user are deleted somewhere after 30s when logged in.
                        if (!auth.user) {
                            auth.logout(true);
                        } else {
                            // validate the session                           
                            if (auth.isSessionValid) {
                                auth.executeObservableOrLogout(auth, observer, observable);
                            } else {
                                //this will cause the session to generate new tokens for expired ones
                                var context = auth.context;
                                var auth2_1 = auth;
                                try {
                                    auth.user.refreshSession(auth.refreshCognitoToken, function (err, session) {
                                        if (err) {
                                            console.error(err);
                                            auth2_1.logout(true);
                                            return;
                                        }
                                        auth2_1.session = session;
                                        auth2_1.hookUpdateCredentials(auth2_1, function (response) {
                                            auth2_1.context.initializeServices();
                                            auth2_1.executeObservableOrLogout(auth2_1, observer, observable);
                                        }, function (error) {
                                            console.error(error);
                                            auth2_1.logout(true);
                                        });
                                        auth2_1.updateCredentials();
                                    });
                                } catch (e) {
                                    auth2_1.logout(true);
                                }
                            }
                        }
                    });
                };
                Authentication.prototype.hookUpdateCredentials = function (auth, success, failure) {
                    var subscription = auth.change.subscribe(function (context) {
                        if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATED || context.state === EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED) subscription.unsubscribe();
                        if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATED) {
                            success(context.output);
                        } else if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATE_FAILED) {
                            failure(context.output);
                        }
                    });
                };
                Authentication.prototype.executeObservableOrLogout = function (auth, observer, observable) {
                    if (auth.isSessionValid && observer && observable) {
                        observer.next(observable);
                    } else {
                        auth.logout(true);
                    }
                };
                Authentication.prototype.execute = function (transition) {
                    var args = [];
                    for (var _i = 1; _i < arguments.length; _i++) {
                        args[_i - 1] = arguments[_i];
                    }
                    (_a = this._actions[transition]).handle.apply(_a, [this._actionObservable].concat(args));
                    var _a;
                };
                Authentication.prototype.parseJwt = function (token) {
                    var base64Url = token.split('.')[1];
                    var base64 = base64Url.replace('-', '+').replace('_', '/');
                    return JSON.parse(atob(base64));
                };
                ;
                return Authentication;
            }();
            exports_1("Authentication", Authentication);
        }
    };
});
System.register("dist/app/aws/user/action/resend-confirmation-code.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var ResendConfirmationCodeAction;
    return {
        setters: [],
        execute: function () {
            ResendConfirmationCodeAction = /** @class */function () {
                function ResendConfirmationCodeAction(context) {
                    this.context = context;
                }
                ResendConfirmationCodeAction.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var username = args[0];
                    if (username === undefined || username === null || username === '') {
                        error("Cognito username was undefined.");
                        return;
                    }
                    var userdata = {
                        Username: username.trim(),
                        Pool: this.context.cognitoUserPool
                    };
                    var cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata);
                    cognitouser.resendConfirmationCode(function (err, result) {
                        if (err) {
                            error(err);
                            subject.next(err);
                            return;
                        }
                        success(result);
                    });
                };
                return ResendConfirmationCodeAction;
            }();
            exports_1("ResendConfirmationCodeAction", ResendConfirmationCodeAction);
        }
    };
});
System.register("dist/app/aws/user/action/get-identity-roles.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var GetUserRolesAction;
    return {
        setters: [],
        execute: function () {
            GetUserRolesAction = /** @class */function () {
                function GetUserRolesAction(context) {
                    this.context = context;
                }
                GetUserRolesAction.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var identityId = args[0];
                    if (identityId === undefined || identityId === null || identityId === '') {
                        error("Cognito identity Id was undefined.");
                        return;
                    }
                    var params = {
                        IdentityId: identityId
                    };
                    this.context.cognitoIdentity.getIdentityPoolRoles(params, function (err, data) {
                        if (err) {
                            error(err);
                            subject.next(err);
                            return;
                        }
                        success(data);
                    });
                };
                return GetUserRolesAction;
            }();
            exports_1("GetUserRolesAction", GetUserRolesAction);
        }
    };
});
System.register("dist/app/aws/user/action/get-user-attributes.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var GetUserAttributesTransition;
    return {
        setters: [],
        execute: function () {
            GetUserAttributesTransition = /** @class */function () {
                function GetUserAttributesTransition(context) {
                    this.context = context;
                }
                GetUserAttributesTransition.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var cognitouser = args[0];
                    if (cognitouser === undefined || cognitouser === null || cognitouser === '') {
                        error("Cognito user was undefined.");
                        return;
                    }
                    cognitouser.getSession(function (err, session) {
                        if (err) {
                            error(err);
                            subject.next(err);
                            return;
                        }
                        cognitouser.getUserAttributes(function (err, attributes) {
                            if (err) {
                                error(err);
                                subject.next(err);
                                return;
                            }
                            success(attributes);
                        });
                    });
                };
                return GetUserAttributesTransition;
            }();
            exports_1("GetUserAttributesTransition", GetUserAttributesTransition);
        }
    };
});
System.register("dist/app/aws/user/action/registering.class.js", ["../user-management.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var user_management_class_1, RegisteringAction;
    return {
        setters: [function (user_management_class_1_1) {
            user_management_class_1 = user_management_class_1_1;
        }],
        execute: function () {
            RegisteringAction = /** @class */function () {
                function RegisteringAction(context) {
                    this.context = context;
                }
                RegisteringAction.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var username = args[0];
                    if (username === undefined || username === null || username === '') {
                        error("Cognito username was undefined.");
                        return;
                    }
                    var password = args[1];
                    if (password === undefined || password === null || password === '') {
                        error("Cognito password was undefined.");
                        return;
                    }
                    var email = args[2];
                    if (email === undefined || email === null || email === '') {
                        error("Cognito email was undefined.");
                        return;
                    }
                    var group = args[3];
                    if (group === undefined || group === null || group === '') {
                        error("The user group was undefined.");
                        return;
                    }
                    var paramsForGroup = {
                        'GroupName': user_management_class_1.EnumGroupName[group].toLowerCase(),
                        'UserPoolId': this.context.userPoolId,
                        'Username': username.trim()
                    };
                    var paramsForCreate = {
                        'UserPoolId': this.context.userPoolId,
                        'Username': username.trim(),
                        'UserAttributes': [{
                            'Name': 'email',
                            'Value': email.trim()
                        }, {
                            'Name': 'email_verified',
                            'Value': 'true'
                        }],
                        'TemporaryPassword': password.trim(),
                        'DesiredDeliveryMediums': ["EMAIL"]
                    };
                    var cognitoIdentityService = this.context.cognitoIdentityService;
                    cognitoIdentityService.adminCreateUser(paramsForCreate, function (err, result) {
                        if (err) {
                            error(err);
                            subject.next(err);
                            return;
                        }
                        cognitoIdentityService.adminAddUserToGroup(paramsForGroup, function (err, result) {
                            if (err) {
                                error(err);
                                subject.next(err);
                                return;
                            }
                            success(result);
                        });
                    });
                };
                return RegisteringAction;
            }();
            exports_1("RegisteringAction", RegisteringAction);
        }
    };
});
System.register("dist/app/aws/user/action/list-users.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var GetUsersAction;
    return {
        setters: [],
        execute: function () {
            GetUsersAction = /** @class */function () {
                function GetUsersAction(context) {
                    this.context = context;
                }
                GetUsersAction.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var userPoolId = args[0];
                    if (userPoolId === undefined || userPoolId === null || userPoolId === '') {
                        error("Cognito user pool Id was undefined.");
                        return;
                    }
                    var attributes = args[1];
                    var filter = args[2];
                    var limit = args[3];
                    var paginationToken = args[4];
                    var params = {
                        UserPoolId: userPoolId
                    };
                    if (attributes) params['AttributesToGet'] = attributes;
                    if (filter) params['Filter'] = filter;
                    if (limit) params['Limit'] = limit;
                    if (paginationToken) params['PaginationToken'] = paginationToken;
                    this.context.cognitoIdentityService.listUsers(params, function (err, data) {
                        if (err) {
                            error(err);
                            subject.next(err);
                        } else {
                            success(data);
                        }
                    });
                };
                return GetUsersAction;
            }();
            exports_1("GetUsersAction", GetUsersAction);
        }
    };
});
System.register("dist/app/aws/user/action/confirming-code.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var ConfirmingCodeAction;
    return {
        setters: [],
        execute: function () {
            ConfirmingCodeAction = /** @class */function () {
                function ConfirmingCodeAction(context) {
                    this.context = context;
                }
                ConfirmingCodeAction.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var code = args[0];
                    if (code === undefined || code === null || code === '') {
                        error(["Cognito user code was undefined."]);
                        return;
                    }
                    var username = args[1];
                    if (username === undefined || username === null || username === '') {
                        error("Cognito username was undefined.");
                        return;
                    }
                    var userdata = {
                        Username: username,
                        Pool: this.context.cognitoUserPool
                    };
                    var cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata);
                    cognitouser.confirmRegistration(code, true, function (err, result) {
                        if (err) {
                            error(err);
                            subject.next(err);
                            return;
                        }
                        success(result);
                    });
                };
                return ConfirmingCodeAction;
            }();
            exports_1("ConfirmingCodeAction", ConfirmingCodeAction);
        }
    };
});
System.register("dist/app/aws/user/action/update-password.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var UpdatePasswordAction;
    return {
        setters: [],
        execute: function () {
            UpdatePasswordAction = /** @class */function () {
                function UpdatePasswordAction(context) {
                    this.context = context;
                }
                UpdatePasswordAction.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var username = args[0];
                    if (username === undefined || username === null || username === '') {
                        error("Cognito username was undefined.");
                        return;
                    }
                    var password = args[1];
                    if (password === undefined || password === null || password === '') {
                        error("Cognito password was undefined.");
                        return;
                    }
                    var authenticationData = {
                        Username: username.trim(),
                        Password: password.trim()
                    };
                    var authenticationDetails = new AWSCognito.CognitoIdentityServiceProvider.AuthenticationDetails(authenticationData);
                    var userdata = {
                        Username: authenticationData.Username,
                        Pool: this.context.cognitoUserPool
                    };
                    var cognitouser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userdata);
                    var userattributes = args[2];
                    delete userattributes.email_verified;
                    // get these details and call
                    cognitouser.completeNewPasswordChallenge(password, userattributes, {
                        onSuccess: function (result) {
                            success(cognitouser);
                        },
                        onFailure: function (err) {
                            error(err, cognitouser);
                            subject.next(err);
                        }
                    });
                };
                return UpdatePasswordAction;
            }();
            exports_1("UpdatePasswordAction", UpdatePasswordAction);
        }
    };
});
System.register("dist/app/aws/user/action/list-users-in-group.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var GetUsersByGroupAction;
    return {
        setters: [],
        execute: function () {
            GetUsersByGroupAction = /** @class */function () {
                function GetUsersByGroupAction(context) {
                    this.context = context;
                }
                GetUsersByGroupAction.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var userPoolId = args[0];
                    if (userPoolId === undefined || userPoolId === null || userPoolId === '') {
                        error("Cognito user pool Id was undefined.");
                        return;
                    }
                    var groupName = args[1];
                    if (groupName === undefined || groupName === null || groupName === '') {
                        error("Cognito group name was undefined.");
                        return;
                    }
                    var limit = args[2];
                    var paginationToken = args[3];
                    var params = {
                        UserPoolId: userPoolId,
                        GroupName: groupName
                    };
                    if (limit) params['Limit'] = limit;
                    if (paginationToken) params['NextToken'] = paginationToken;
                    this.context.cognitoIdentityService.listUsersInGroup(params, function (err, data) {
                        if (err) {
                            error(err);
                            subject.next(err);
                        } else {
                            success(data);
                        }
                    });
                };
                return GetUsersByGroupAction;
            }();
            exports_1("GetUsersByGroupAction", GetUsersByGroupAction);
        }
    };
});
System.register("dist/app/aws/user/action/delete-user.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var DeleteUserAction;
    return {
        setters: [],
        execute: function () {
            DeleteUserAction = /** @class */function () {
                function DeleteUserAction(context) {
                    this.context = context;
                }
                DeleteUserAction.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var username = args[0];
                    if (username === undefined || username === null || username === '') {
                        error("Cognito username was undefined.");
                        return;
                    }
                    var params = {
                        UserPoolId: this.context.userPoolId,
                        Username: username.trim() /* required */
                    };
                    this.context.cognitoIdentityService.adminDeleteUser(params, function (err, data) {
                        if (err) {
                            error(err);
                            subject.next(err);
                        } else {
                            success(data);
                        }
                    });
                };
                return DeleteUserAction;
            }();
            exports_1("DeleteUserAction", DeleteUserAction);
        }
    };
});
System.register("dist/app/aws/user/action/reset-password.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var ResetPasswordAction;
    return {
        setters: [],
        execute: function () {
            ResetPasswordAction = /** @class */function () {
                function ResetPasswordAction(context) {
                    this.context = context;
                }
                ResetPasswordAction.prototype.handle = function (success, error, subject) {
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    var username = args[0];
                    if (username === undefined || username === null || username === '') {
                        error("Cognito username was undefined.");
                        return;
                    }
                    var params = {
                        UserPoolId: this.context.userPoolId,
                        Username: username.trim()
                    };
                    // get these details and call
                    this.context.cognitoIdentityService.adminResetUserPassword(params, function (err, result) {
                        if (err) {
                            error(err);
                            subject.next(err);
                            return;
                        }
                        success(result);
                    });
                };
                return ResetPasswordAction;
            }();
            exports_1("ResetPasswordAction", ResetPasswordAction);
        }
    };
});
System.register("dist/app/aws/user/user-management.class.js", ["./action/resend-confirmation-code.class", "./action/get-identity-roles.class", "./action/get-user-attributes.class", "./action/registering.class", "./action/list-users.class", "./action/confirming-code.class", "./action/update-password.class", "./action/list-users-in-group.class", "./action/delete-user.class", "./action/reset-password.class", "rxjs/BehaviorSubject"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var resend_confirmation_code_class_1, get_identity_roles_class_1, get_user_attributes_class_1, registering_class_1, list_users_class_1, confirming_code_class_1, update_password_class_1, list_users_in_group_class_1, delete_user_class_1, reset_password_class_1, BehaviorSubject_1, EnumUserManagementAction, EnumGroupName, UserManagement;
    return {
        setters: [function (resend_confirmation_code_class_1_1) {
            resend_confirmation_code_class_1 = resend_confirmation_code_class_1_1;
        }, function (get_identity_roles_class_1_1) {
            get_identity_roles_class_1 = get_identity_roles_class_1_1;
        }, function (get_user_attributes_class_1_1) {
            get_user_attributes_class_1 = get_user_attributes_class_1_1;
        }, function (registering_class_1_1) {
            registering_class_1 = registering_class_1_1;
        }, function (list_users_class_1_1) {
            list_users_class_1 = list_users_class_1_1;
        }, function (confirming_code_class_1_1) {
            confirming_code_class_1 = confirming_code_class_1_1;
        }, function (update_password_class_1_1) {
            update_password_class_1 = update_password_class_1_1;
        }, function (list_users_in_group_class_1_1) {
            list_users_in_group_class_1 = list_users_in_group_class_1_1;
        }, function (delete_user_class_1_1) {
            delete_user_class_1 = delete_user_class_1_1;
        }, function (reset_password_class_1_1) {
            reset_password_class_1 = reset_password_class_1_1;
        }, function (BehaviorSubject_1_1) {
            BehaviorSubject_1 = BehaviorSubject_1_1;
        }],
        execute: function () {
            (function (EnumUserManagementAction) {
                EnumUserManagementAction[EnumUserManagementAction["REGISTERING"] = 0] = "REGISTERING";
                EnumUserManagementAction[EnumUserManagementAction["GET_USER_ATTRIBUTE"] = 1] = "GET_USER_ATTRIBUTE";
                EnumUserManagementAction[EnumUserManagementAction["RESEND_CONFIRMATION_CODE"] = 2] = "RESEND_CONFIRMATION_CODE";
                EnumUserManagementAction[EnumUserManagementAction["GET_ROLES"] = 3] = "GET_ROLES";
                EnumUserManagementAction[EnumUserManagementAction["GET_USERS"] = 4] = "GET_USERS";
                EnumUserManagementAction[EnumUserManagementAction["CONFIRM_CODE"] = 5] = "CONFIRM_CODE";
                EnumUserManagementAction[EnumUserManagementAction["UPDATE_PASSWORD"] = 6] = "UPDATE_PASSWORD";
                EnumUserManagementAction[EnumUserManagementAction["GET_USERS_BY_GROUP"] = 7] = "GET_USERS_BY_GROUP";
                EnumUserManagementAction[EnumUserManagementAction["DELETE_USER"] = 8] = "DELETE_USER";
                EnumUserManagementAction[EnumUserManagementAction["RESET_PASSWORD"] = 9] = "RESET_PASSWORD";
            })(EnumUserManagementAction || (EnumUserManagementAction = {}));
            exports_1("EnumUserManagementAction", EnumUserManagementAction);
            (function (EnumGroupName) {
                EnumGroupName[EnumGroupName["ADMINISTRATOR"] = 0] = "ADMINISTRATOR";
                EnumGroupName[EnumGroupName["USER"] = 1] = "USER";
            })(EnumGroupName || (EnumGroupName = {}));
            exports_1("EnumGroupName", EnumGroupName);
            UserManagement = /** @class */function () {
                function UserManagement(context, router, metric) {
                    if (metric === void 0) {
                        metric = null;
                    }
                    var _this = this;
                    this.context = context;
                    this.metric = metric;
                    this._actions = [];
                    this._identifier = "User Administration";
                    this._error = new BehaviorSubject_1.BehaviorSubject({});
                    this._error.subscribe(function (err) {
                        if (err.code == "CredentialsError") {
                            _this.context.authentication.refreshSessionOrLogout();
                        }
                    });
                    this._actions[EnumUserManagementAction.REGISTERING] = new registering_class_1.RegisteringAction(this.context);
                    this._actions[EnumUserManagementAction.GET_USER_ATTRIBUTE] = new get_user_attributes_class_1.GetUserAttributesTransition(this.context);
                    this._actions[EnumUserManagementAction.RESEND_CONFIRMATION_CODE] = new resend_confirmation_code_class_1.ResendConfirmationCodeAction(this.context);
                    this._actions[EnumUserManagementAction.GET_ROLES] = new get_identity_roles_class_1.GetUserRolesAction(this.context);
                    this._actions[EnumUserManagementAction.GET_USERS] = new list_users_class_1.GetUsersAction(this.context);
                    this._actions[EnumUserManagementAction.CONFIRM_CODE] = new confirming_code_class_1.ConfirmingCodeAction(this.context);
                    this._actions[EnumUserManagementAction.UPDATE_PASSWORD] = new update_password_class_1.UpdatePasswordAction(this.context);
                    this._actions[EnumUserManagementAction.GET_USERS_BY_GROUP] = new list_users_in_group_class_1.GetUsersByGroupAction(this.context);
                    this._actions[EnumUserManagementAction.DELETE_USER] = new delete_user_class_1.DeleteUserAction(this.context);
                    this._actions[EnumUserManagementAction.RESET_PASSWORD] = new reset_password_class_1.ResetPasswordAction(this.context);
                }
                UserManagement.prototype.getUserRoles = function (success, error) {
                    this.execute(EnumUserManagementAction.GET_ROLES, success, error, this.context.authentication.identityId);
                };
                UserManagement.prototype.getUserAttributes = function (user, success, error) {
                    this.execute(EnumUserManagementAction.GET_USER_ATTRIBUTE, success, error, user);
                };
                UserManagement.prototype.resendConfirmationCode = function (username, success, error) {
                    this.execute(EnumUserManagementAction.RESEND_CONFIRMATION_CODE, success, error, username);
                };
                UserManagement.prototype.register = function (username, password, email, group, success, error) {
                    this.execute(EnumUserManagementAction.REGISTERING, success, error, username, password, email, group);
                };
                UserManagement.prototype.getAllUsers = function (success, error, attributestoget, filter, limit, paginationtoken) {
                    if (attributestoget === void 0) {
                        attributestoget = [];
                    }
                    this.execute(EnumUserManagementAction.GET_USERS, success, error, this.context.userPoolId, attributestoget, filter, limit, paginationtoken);
                };
                UserManagement.prototype.updatePassword = function (username, password, userattributes, success, error) {
                    this.execute(EnumUserManagementAction.UPDATE_PASSWORD, success, error, username, password, userattributes);
                };
                UserManagement.prototype.confirmCode = function (code, username, success, error) {
                    this.execute(EnumUserManagementAction.CONFIRM_CODE, success, error, code, username);
                };
                UserManagement.prototype.getAdministrators = function (success, error, limit, paginationtoken) {
                    this.execute(EnumUserManagementAction.GET_USERS_BY_GROUP, success, error, this.context.userPoolId, EnumGroupName[EnumGroupName.ADMINISTRATOR].toLowerCase(), limit, paginationtoken);
                };
                UserManagement.prototype.getUsers = function (success, error, limit, paginationtoken) {
                    this.execute(EnumUserManagementAction.GET_USERS_BY_GROUP, success, error, this.context.userPoolId, EnumGroupName[EnumGroupName.USER].toLowerCase(), limit, paginationtoken);
                };
                UserManagement.prototype.getUserByGroupName = function (groupName, success, error, limit, paginationtoken) {
                    this.execute(EnumUserManagementAction.GET_USERS_BY_GROUP, success, error, this.context.userPoolId, groupName, limit, paginationtoken);
                };
                UserManagement.prototype.deleteUser = function (username, success, error) {
                    this.execute(EnumUserManagementAction.DELETE_USER, success, error, username);
                };
                UserManagement.prototype.resetPassword = function (username, success, error) {
                    this.execute(EnumUserManagementAction.RESET_PASSWORD, success, error, username);
                };
                UserManagement.prototype.execute = function (transition, success_callback, error_callback) {
                    var _this = this;
                    var args = [];
                    for (var _i = 3; _i < arguments.length; _i++) {
                        args[_i - 3] = arguments[_i];
                    }
                    this.recordEvent("ApiServiceRequested", {
                        "Identifier": this._identifier,
                        "Verb": "GET",
                        "Path": EnumUserManagementAction[transition].toString()
                    }, null);
                    (_a = this._actions[transition]).handle.apply(_a, [function (result) {
                        _this.recordEvent("ApiServiceSuccess", {
                            "Identifier": _this._identifier,
                            "Path": EnumUserManagementAction[transition].toString()
                        }, null);
                        success_callback(result);
                    }, function (err) {
                        _this.recordEvent("ApiServiceError", {
                            "Message": err.message,
                            "Identifier": _this._identifier,
                            "Path": EnumUserManagementAction[transition].toString()
                        }, null);
                        error_callback(err);
                    }, this._error].concat(args));
                    var _a;
                };
                UserManagement.prototype.recordEvent = function (eventName, attribute, metric) {
                    if (attribute === void 0) {
                        attribute = {};
                    }
                    if (metric === void 0) {
                        metric = null;
                    }
                    if (this.metric !== null) this.metric.recordEvent(eventName, attribute, metric);
                };
                return UserManagement;
            }();
            exports_1("UserManagement", UserManagement);
        }
    };
});
System.register("dist/app/aws/resource-group.class.js", ["rxjs/BehaviorSubject", "./schema.class"], function (exports_1, context_1) {
    "use strict";

    var __extends = this && this.__extends || function () {
        var extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
        };
        return function (d, b) {
            extendStatics(d, b);
            function __() {
                this.constructor = d;
            }
            d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
        };
    }();
    var __moduleName = context_1 && context_1.id;
    var BehaviorSubject_1, schema_class_1, ResourceOutputEnum, ResourceOutput, ResourceGroup, AwsResourceGroup;
    return {
        setters: [function (BehaviorSubject_1_1) {
            BehaviorSubject_1 = BehaviorSubject_1_1;
        }, function (schema_class_1_1) {
            schema_class_1 = schema_class_1_1;
        }],
        execute: function () {
            (function (ResourceOutputEnum) {
                ResourceOutputEnum[ResourceOutputEnum["ServiceUrl"] = 0] = "ServiceUrl";
            })(ResourceOutputEnum || (ResourceOutputEnum = {}));
            exports_1("ResourceOutputEnum", ResourceOutputEnum);
            ResourceOutput = /** @class */function () {
                function ResourceOutput(key, val, desc) {
                    this.outputKey = key;
                    this.outputValue = val;
                    this.description = desc;
                }
                return ResourceOutput;
            }();
            exports_1("ResourceOutput", ResourceOutput);
            ResourceGroup = /** @class */function () {
                function ResourceGroup() {}
                return ResourceGroup;
            }();
            exports_1("ResourceGroup", ResourceGroup);
            AwsResourceGroup = /** @class */function (_super) {
                __extends(AwsResourceGroup, _super);
                function AwsResourceGroup(context, spec) {
                    var _this = _super.call(this) || this;
                    _this.context = context;
                    _this.setResource(spec);
                    _this._outputs = new Map();
                    _this._updating = new BehaviorSubject_1.BehaviorSubject(true);
                    return _this;
                }
                Object.defineProperty(AwsResourceGroup, "RESOURCE_TEMPLATE_FILE", {
                    get: function () {
                        return "resource-template.json";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsResourceGroup.prototype, "logicalResourceId", {
                    get: function () {
                        return this._logicalResourceId;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsResourceGroup.prototype, "physicalResourceId", {
                    get: function () {
                        return this._physicalResourceId;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsResourceGroup.prototype, "resourceType", {
                    get: function () {
                        return this._resourceType;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsResourceGroup.prototype, "resourceStatus", {
                    get: function () {
                        return this._resourceStatus;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsResourceGroup.prototype, "stackId", {
                    get: function () {
                        return this._stackId;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsResourceGroup.prototype, "stackName", {
                    get: function () {
                        return this._stackName;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsResourceGroup.prototype, "outputs", {
                    get: function () {
                        return this._outputs;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsResourceGroup.prototype, "updating", {
                    get: function () {
                        return this._updating.asObservable();
                    },
                    enumerable: true,
                    configurable: true
                });
                AwsResourceGroup.prototype.setOutputs = function () {
                    var _this = this;
                    //desribe the resource group stack resources
                    this.context.cloudFormation.describeStacks({
                        StackName: this.physicalResourceId
                    }, function (err, data) {
                        if (err) {
                            var message = err.message;
                            if (err.statusCode == 403) {
                                _this.context.authentication.refreshSessionOrLogout();
                            }
                        }
                        var stackOutputs = data.Stacks[0].Outputs;
                        for (var i = 0; i < stackOutputs.length; i++) {
                            var o = stackOutputs[i];
                            _this.outputs[o.OutputKey] = o.OutputValue;
                        }
                        _this._updating.next(false);
                    });
                };
                AwsResourceGroup.prototype.setResource = function (specification) {
                    this._logicalResourceId = specification[schema_class_1.Schema.CloudFormation.LOGICAL_RESOURCE_ID];
                    this._physicalResourceId = specification[schema_class_1.Schema.CloudFormation.PHYSICAL_RESOURCE_ID];
                    this._resourceStatus = specification[schema_class_1.Schema.CloudFormation.RESOURCE_STATUS];
                    this._resourceType = specification[schema_class_1.Schema.CloudFormation.RESOURCE_TYPE.NAME];
                    this._stackId = specification[schema_class_1.Schema.CloudFormation.STACK_ID];
                    this._stackName = specification[schema_class_1.Schema.CloudFormation.STACK_NAME];
                };
                return AwsResourceGroup;
            }(ResourceGroup);
            exports_1("AwsResourceGroup", AwsResourceGroup);
        }
    };
});
System.register("dist/app/aws/schema.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var Schema;
    return {
        setters: [],
        execute: function () {
            Schema = /** @class */function () {
                function Schema() {}
                Schema.CloudFormation = {
                    LOGICAL_RESOURCE_ID: "LogicalResourceId",
                    PHYSICAL_RESOURCE_ID: "PhysicalResourceId",
                    RESOURCE_STATUS: "ResourceStatus",
                    RESOURCE_TYPE: {
                        NAME: "ResourceType",
                        TYPES: {
                            S3: "AWS::S3::Bucket",
                            CLOUD_FORMATION: "AWS::CloudFormation::Stack",
                            LAMBDA: "AWS::Lambda::Function"
                        }
                    },
                    STACK_ID: "StackId",
                    STACK_NAME: "StackName"
                };
                Schema.Files = {
                    INDEX_HTML: "index.html"
                };
                Schema.Folders = {
                    CGP_RESOURCES: "cgp-resource-code"
                };
                return Schema;
            }();
            exports_1("Schema", Schema);
        }
    };
});
System.register("dist/app/aws/deployment.class.js", ["rxjs/BehaviorSubject", "./aws.service", "./resource-group.class", "./schema.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var BehaviorSubject_1, aws_service_1, resource_group_class_1, schema_class_1, AwsDeployment;
    return {
        setters: [function (BehaviorSubject_1_1) {
            BehaviorSubject_1 = BehaviorSubject_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (resource_group_class_1_1) {
            resource_group_class_1 = resource_group_class_1_1;
        }, function (schema_class_1_1) {
            schema_class_1 = schema_class_1_1;
        }],
        execute: function () {
            AwsDeployment = /** @class */function () {
                function AwsDeployment(context, deploymentsettings) {
                    this.context = context;
                    this.deploymentsettings = deploymentsettings;
                    this._isLoading = false;
                    this._deploymentSettings = deploymentsettings;
                    this._resourceGroups = new BehaviorSubject_1.BehaviorSubject(undefined);
                    this._resourceGroupList = [];
                }
                Object.defineProperty(AwsDeployment, "DEPLOYMENT_TEMPLATE_FILE", {
                    get: function () {
                        return "deployment-template.json";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsDeployment.prototype, "resourceGroup", {
                    get: function () {
                        return this._resourceGroups.asObservable();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsDeployment.prototype, "resourceGroupList", {
                    get: function () {
                        return this._resourceGroupList;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsDeployment.prototype, "settings", {
                    get: function () {
                        return this._deploymentSettings;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsDeployment.prototype, "configEndpointURL", {
                    get: function () {
                        return this._deploymentSettings.ConfigurationBucket + '/' + this._deploymentSettings.ConfigurationKey;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsDeployment.prototype, "resourceGroupConfigEndpointURL", {
                    get: function () {
                        return this.configEndpointURL + "/resource-group";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsDeployment.prototype, "isLoading", {
                    get: function () {
                        return this._isLoading;
                    },
                    enumerable: true,
                    configurable: true
                });
                AwsDeployment.prototype.update = function () {
                    var _this = this;
                    if (!this.settings || !this.settings.DeploymentStackId) {
                        this._resourceGroups.next([]);
                        return;
                    }
                    var region = aws_service_1.AwsService.getRegionFromArn(this.settings.DeploymentStackId);
                    this._isLoading = true;
                    this.context.cloudFormation.describeStacks({
                        StackName: this.settings.DeploymentStackId
                    }, function (err, data) {
                        if (err) {
                            console.log(err);
                            _this._isLoading = false;
                            return;
                        }
                        //finish the deployment settings configuration
                        _this.updateSettings(data);
                        //we have the endpoint to get the resourcegroup configuration
                        //get the configuration from the S3 bucket
                        _this.getResourceGroups();
                    });
                };
                AwsDeployment.prototype.updateSettings = function (data) {
                    for (var i = 0; i < data.Stacks.length; i++) {
                        for (var x = 0; x < data.Stacks[i].Parameters.length; x++) {
                            var parameter = data.Stacks[i].Parameters[x];
                            var key = parameter['ParameterKey'];
                            var value = parameter['ParameterValue'];
                            if (key == "ConfigurationBucket") {
                                this.settings.ConfigurationBucket = value;
                            } else if (key == "ConfigurationKey") {
                                this.settings.ConfigurationKey = value;
                            }
                        }
                    }
                };
                AwsDeployment.prototype.getResourceGroups = function () {
                    var _this = this;
                    this.context.cloudFormation.describeStackResources({
                        StackName: this.settings.DeploymentStackId
                    }, function (err, data) {
                        if (err) {
                            console.log(err);
                            _this._resourceGroupList = [];
                            _this._resourceGroups.next(_this._resourceGroupList);
                            _this._isLoading = false;
                            return;
                        }
                        var rgs = [];
                        for (var i = 0; i < data.StackResources.length; i++) {
                            var spec = data.StackResources[i];
                            var logicalResourceId = spec[schema_class_1.Schema.CloudFormation.LOGICAL_RESOURCE_ID];
                            //Are we a resource group or resource
                            if (spec[schema_class_1.Schema.CloudFormation.RESOURCE_TYPE.NAME] == schema_class_1.Schema.CloudFormation.RESOURCE_TYPE.TYPES.CLOUD_FORMATION) {
                                //we are a resource group
                                var rg = new resource_group_class_1.AwsResourceGroup(_this.context, spec);
                                rgs.push(rg);
                            }
                        }
                        var updatedCount = 0;
                        for (var i = 0; i < rgs.length; i++) {
                            var rg = rgs[i];
                            rg.updating.subscribe(function (isUpdating) {
                                _this._isLoading = false;
                                if (isUpdating) {
                                    return;
                                }
                                updatedCount++;
                                if (updatedCount == rgs.length) {
                                    _this._resourceGroupList = rgs;
                                    _this._resourceGroups.next(_this._resourceGroupList);
                                }
                            });
                            rg.setOutputs();
                        }
                        if (rgs.length == 0) {
                            _this._isLoading = false;
                            _this._resourceGroupList = [];
                            _this._resourceGroups.next(_this._resourceGroupList);
                        }
                    });
                };
                return AwsDeployment;
            }();
            exports_1("AwsDeployment", AwsDeployment);
        }
    };
});
System.register("dist/app/aws/project.class.js", ["rxjs/Subject", "./deployment.class", "rxjs/add/operator/map", "rxjs/add/operator/catch"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var Subject_1, deployment_class_1, AwsProject;
    return {
        setters: [function (Subject_1_1) {
            Subject_1 = Subject_1_1;
        }, function (deployment_class_1_1) {
            deployment_class_1 = deployment_class_1_1;
        }, function (_1) {}, function (_2) {}],
        execute: function () {
            AwsProject = /** @class */function () {
                function AwsProject(context, http) {
                    var _this = this;
                    this.context = context;
                    this.http = http;
                    this._settings = new Subject_1.Subject();
                    this._activeDeploymentSubscription = new Subject_1.Subject();
                    this._activeDeployment = null;
                    this._deployments = [];
                    this._outputs = new Map();
                    this._activeDeploymentSubscription.subscribe(function (deployment) {
                        if (deployment) {
                            _this._activeDeployment = deployment;
                            deployment.update();
                        }
                    });
                    //initialize the deployments array
                    this.settings.subscribe(function (d) {
                        if (!d) {
                            _this.isInitialized = true;
                            _this._activeDeploymentSubscription.next(undefined);
                            return;
                        }
                        if (d.DefaultDeployment || Object.keys(d.deployment).length == 0) _this.isInitialized = true;
                        //clear the array but don't generate a new reference
                        _this._deployments.length = 0;
                        var default_deployment = d.DefaultDeployment;
                        var default_found = false;
                        for (var entry in d.deployment) {
                            var obj = d.deployment[entry];
                            var deploymentSetting = {
                                name: entry,
                                DeploymentAccessStackId: obj['DeploymentAccessStackId'],
                                DeploymentStackId: obj['DeploymentStackId'],
                                ConfigurationBucket: obj['ConfigurationBucket'],
                                ConfigurationKey: obj['ConfigurationKey']
                            };
                            var awsdeployment = new deployment_class_1.AwsDeployment(_this.context, deploymentSetting);
                            _this._deployments.push(awsdeployment);
                            //set the default deployment
                            if (entry == d.DefaultDeployment) {
                                _this._activeDeploymentSubscription.next(awsdeployment);
                                default_found = true;
                            }
                        }
                        if (!default_found) _this._activeDeploymentSubscription.next(awsdeployment);
                    });
                    var proj = this;
                    var signedRequest = this.context.s3.getSignedUrl('getObject', { Bucket: this.context.configBucket, Key: AwsProject.PROJECT_SETTING_FILE, Expires: 120 });
                    this.http.request(signedRequest).subscribe(function (response) {
                        var settings = response.json();
                        var deployments = settings.deployment;
                        //Filter out our 'wildcard' settings and emit our deployment list
                        delete deployments["*"];
                        proj._settingsCache = settings;
                        proj._settings.next(settings);
                    }, function (err) {
                        console.error(err);
                    });
                }
                Object.defineProperty(AwsProject, "BUCKET_CONFIG_NAME", {
                    get: function () {
                        return "config_bucket";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsProject, "PROJECT_SETTING_FILE", {
                    get: function () {
                        return "project-settings.json";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsProject.prototype, "isInitialized", {
                    get: function () {
                        return this._isInitialized;
                    },
                    set: function (value) {
                        this._isInitialized = value;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsProject.prototype, "deployments", {
                    get: function () {
                        return this._deployments;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsProject.prototype, "activeDeployment", {
                    get: function () {
                        return this._activeDeployment;
                    },
                    set: function (activeDeployment) {
                        this._activeDeployment = activeDeployment;
                        this._activeDeploymentSubscription.next(activeDeployment);
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsProject.prototype, "activeDeploymentSubscription", {
                    get: function () {
                        return this._activeDeploymentSubscription.asObservable();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsProject.prototype, "settings", {
                    get: function () {
                        return this._settings.asObservable();
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsProject.prototype, "outputs", {
                    get: function () {
                        return this._outputs;
                    },
                    enumerable: true,
                    configurable: true
                });
                return AwsProject;
            }();
            exports_1("AwsProject", AwsProject);
        }
    };
});
System.register("dist/app/aws/aws.service.js", ["@angular/core", "./context.class", "./authentication/authentication.class", "./user/user-management.class", "./project.class", "@angular/router", "@angular/http", "app/shared/service/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, context_class_1, authentication_class_1, user_management_class_1, project_class_1, router_1, http_1, index_1, AwsService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (context_class_1_1) {
            context_class_1 = context_class_1_1;
        }, function (authentication_class_1_1) {
            authentication_class_1 = authentication_class_1_1;
        }, function (user_management_class_1_1) {
            user_management_class_1 = user_management_class_1_1;
        }, function (project_class_1_1) {
            project_class_1 = project_class_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }],
        execute: function () {
            AwsService = /** @class */function () {
                function AwsService(router, http, metric) {
                    this.router = router;
                    this.http = http;
                    this.metric = metric;
                    this._isInitialized = false;
                    this._context = new context_class_1.AwsContext();
                    this._context.authentication = new authentication_class_1.Authentication(this.context, this.router, this.metric);
                    this._context.userManagement = new user_management_class_1.UserManagement(this.context, this.router, this.metric);
                }
                AwsService.getStackNameFromArn = function (arn) {
                    if (arn) return arn.split('/')[1];
                    return null;
                };
                AwsService.getRegionFromArn = function (arn) {
                    if (arn) return arn.split(':')[3];
                    return null;
                };
                AwsService.getAccountIdFromArn = function (arn) {
                    if (arn) return arn.split(':')[4];
                    return null;
                };
                Object.defineProperty(AwsService.prototype, "isInitialized", {
                    get: function () {
                        return this._isInitialized;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(AwsService.prototype, "context", {
                    get: function () {
                        return this._context;
                    },
                    enumerable: true,
                    configurable: true
                });
                AwsService.prototype.init = function (userPoolId, clientId, identityPoolId, bucketid, region, definitions) {
                    var _this = this;
                    //init the aws context
                    this._isInitialized = this.isValidBootstrap(userPoolId, clientId, identityPoolId, bucketid, region);
                    if (!this._isInitialized) return;
                    this._context.init(userPoolId, clientId, identityPoolId, bucketid, region);
                    this.initializeSubscriptions();
                    this.context.authentication.change.subscribe(function (context) {
                        if (context.state === authentication_class_1.EnumAuthState.LOGGED_OUT) {
                            _this._initializationSubscription.unsubscribe();
                            _this.initializeSubscriptions();
                            _this.router.navigate(['/login']);
                        }
                    });
                };
                AwsService.prototype.parseAPIId = function (serviceurl) {
                    if (!serviceurl) return undefined;
                    var schema_parts = serviceurl.split('//');
                    var url_parts = schema_parts[1].split('.');
                    return url_parts[0];
                };
                AwsService.prototype.initializeSubscriptions = function () {
                    var _this = this;
                    this._initializationSubscription = this.subscribeUserUpdated();
                    this.context.authentication.change.subscribe(function (context) {
                        if (context.state === authentication_class_1.EnumAuthState.LOGGED_OUT) {
                            _this._initializationSubscription = _this.subscribeUserUpdated();
                        }
                    });
                };
                AwsService.prototype.subscribeUserUpdated = function () {
                    var _this = this;
                    return this.context.authentication.change.subscribe(function (context) {
                        if (context.state === authentication_class_1.EnumAuthState.USER_CREDENTIAL_UPDATED) {
                            _this._initializationSubscription.unsubscribe();
                            _this.context.initializeServices();
                            if (!_this.context.project) {
                                _this.context.project = new project_class_1.AwsProject(_this.context, _this.http);
                            }
                            // Parse the URL to get the query parameters
                            var baseUrl = location.href;
                            var queryParam = {};
                            if (baseUrl.indexOf("deployment=") !== -1) {
                                var queryParams = baseUrl.slice(baseUrl.indexOf("deployment="));
                                var queryParamSections = queryParams.split("&");
                                for (var _i = 0, queryParamSections_1 = queryParamSections; _i < queryParamSections_1.length; _i++) {
                                    var section = queryParamSections_1[_i];
                                    var keyValuePair = section.split('=');
                                    queryParam[keyValuePair[0]] = decodeURIComponent(decodeURIComponent(keyValuePair[1]));
                                }
                            }
                            _this.router.navigate(['game/cloudgems'], { 'queryParams': queryParam });
                        }
                    });
                };
                AwsService.prototype.isValidBootstrap = function (userPoolId, clientId, identityPoolId, bucketid, region) {
                    return userPoolId && clientId && identityPoolId && bucketid && region ? true : false;
                };
                AwsService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [router_1.Router, http_1.Http, index_1.LyMetricService])], AwsService);
                return AwsService;
            }();
            exports_1("AwsService", AwsService);
        }
    };
});
System.register("dist/app/shared/service/url.service.js", ["@angular/core", "app/aws/aws.service"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, aws_service_1, UrlService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }],
        execute: function () {
            UrlService = /** @class */function () {
                function UrlService(aws) {
                    this.aws = aws;
                }
                Object.defineProperty(UrlService.prototype, "baseUrl", {
                    get: function () {
                        return this._baseUrl;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(UrlService.prototype, "querystring", {
                    get: function () {
                        return this._querystring;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(UrlService.prototype, "fragment", {
                    get: function () {
                        return this._fragment;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(UrlService.prototype, "encryptedPayload", {
                    get: function () {
                        return this._encryptionPayload;
                    },
                    enumerable: true,
                    configurable: true
                });
                UrlService.prototype.parseLocationHref = function (base_url) {
                    this._baseUrl = base_url.replace('/login', ''); // this.router.url;
                    this._querystring = this.baseUrl.split('?')[1];
                    this._fragment = this.baseUrl.split('#')[1];
                };
                UrlService.prototype.signUrls = function (component, bucket, expirationinseconds) {
                    if (!bucket) return;
                    if (component.templateUrl) {
                        component.templateUrl = this.aws.context.s3.getSignedUrl('getObject', { Bucket: bucket, Key: component.templateUrl, Expires: expirationinseconds });
                    }
                    if (!component.styleUrls) return;
                    for (var i = 0; i < component.styleUrls.length; i++) {
                        component.styleUrls[i] = this.aws.context.s3.getSignedUrl('getObject', { Bucket: bucket, Key: component.styleUrls[i], Expires: expirationinseconds });
                    }
                };
                UrlService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [aws_service_1.AwsService])], UrlService);
                return UrlService;
            }();
            exports_1("UrlService", UrlService);
        }
    };
});
System.register("dist/app/shared/class/datetime.class.js", ["@angular/common"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var common_1, DateTimeUtil;
    return {
        setters: [function (common_1_1) {
            common_1 = common_1_1;
        }],
        execute: function () {
            DateTimeUtil = /** @class */function () {
                function DateTimeUtil() {}
                DateTimeUtil.toObjDate = function (obj) {
                    var time = null;
                    if (this.isNotNullOrUndefined(obj.time) && this.isNotNullOrUndefined(obj.time.hour) && this.isNotNullOrUndefined(obj.time.minute) && this.isNotNullOrUndefined(obj.date) && this.isNotNullOrUndefined(obj.date.month) && this.isNotNullOrUndefined(obj.date.year) && this.isNotNullOrUndefined(obj.date.day)) {
                        time = {
                            hour: obj.time.hour,
                            minute: obj.time.minute
                        };
                    } else {
                        return undefined;
                    }
                    return new Date(obj.date.year, obj.date.month - 1, obj.date.day, time.hour, time.minute, null);
                };
                // toDate: use moment js to convert a date object to a date string for serialization.
                DateTimeUtil.toDate = function (obj, format) {
                    if (format === void 0) {
                        format = 'MMM DD YYYY HH:mm';
                    }
                    var newDate = moment(DateTimeUtil.toObjDate(obj)).format(format);
                    return newDate;
                };
                DateTimeUtil.toString = function (date, format) {
                    return new common_1.DatePipe("en-US").transform(date, format);
                };
                DateTimeUtil.isValid = function (date) {
                    return !(date == null || isNaN(date.valueOf()));
                };
                DateTimeUtil.getUTCDate = function () {
                    var date = new Date();
                    return new Date(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate(), date.getUTCHours(), date.getUTCMinutes(), date.getUTCSeconds());
                };
                DateTimeUtil.fromEpoch = function (timestamp) {
                    var d = new Date(timestamp * 1000);
                    return d;
                };
                DateTimeUtil.isNotNullOrUndefined = function (obj) {
                    return !(obj === undefined || obj === null);
                };
                return DateTimeUtil;
            }();
            exports_1("DateTimeUtil", DateTimeUtil);
        }
    };
});
System.register("dist/app/shared/class/log.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var Log;
    return {
        setters: [],
        execute: function () {
            Log = /** @class */function () {
                function Log() {}
                Log.error = function (message) {
                    console.error(message);
                };
                Log.success = function (message) {
                    console.log(message);
                };
                return Log;
            }();
            exports_1("Log", Log);
        }
    };
});
System.register("dist/app/shared/class/string.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var StringUtil;
    return {
        setters: [],
        execute: function () {
            StringUtil = /** @class */function () {
                function StringUtil() {}
                StringUtil.toEtcetera = function (input, maxlength, suffix) {
                    if (suffix === void 0) {
                        suffix = "...";
                    }
                    var output = input;
                    if (output && output.length > maxlength) {
                        output = output.substr(0, maxlength) + suffix;
                    }
                    return output;
                };
                return StringUtil;
            }();
            exports_1("StringUtil", StringUtil);
        }
    };
});
System.register("dist/app/shared/class/apihandler.class.js", ["@angular/http", "rxjs/Observable", "rxjs/add/operator/concatAll", "rxjs/add/observable/throw"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var http_1, Observable_1, DEFAULT_ACCEPT_TYPE, DEFAULT_CONTENT_TYPE, HEADER_CONTENT_TYPE, SIGNING_API, ApiHandler;
    return {
        setters: [function (http_1_1) {
            http_1 = http_1_1;
        }, function (Observable_1_1) {
            Observable_1 = Observable_1_1;
        }, function (_1) {}, function (_2) {}],
        execute: function () {
            DEFAULT_ACCEPT_TYPE = "application/json;charset=utf-8";
            DEFAULT_CONTENT_TYPE = "application/json;charset=utf-8";
            HEADER_CONTENT_TYPE = "Content-Type";
            SIGNING_API = "execute-api";
            ApiHandler = /** @class */function () {
                function ApiHandler(serviceAPI, http, aws, metric, metricIdentifier) {
                    if (metric === void 0) {
                        metric = null;
                    }
                    if (metricIdentifier === void 0) {
                        metricIdentifier = "";
                    }
                    this._serviceAPI = serviceAPI;
                    this._http = http;
                    this._aws = aws;
                    this._identifier = metricIdentifier;
                    this._metric = metric;
                    this.get = this.get.bind(this);
                    this.post = this.post.bind(this);
                    this.put = this.put.bind(this);
                    this.delete = this.delete.bind(this);
                    this.sortQueryParametersByCodePoint = this.sortQueryParametersByCodePoint.bind(this);
                    this.execute = this.execute.bind(this);
                    this.mapResponseCodes = this.mapResponseCodes.bind(this);
                    this.recordEvent = this.recordEvent.bind(this);
                    this.AwsSigv4Signer = this.AwsSigv4Signer.bind(this);
                }
                ApiHandler.prototype.get = function (restPath, headers) {
                    if (headers === void 0) {
                        headers = [];
                    }
                    var options = {
                        method: http_1.RequestMethod.Get,
                        headers: headers,
                        url: this._serviceAPI + "/" + restPath
                    };
                    return this.execute(options);
                };
                ApiHandler.prototype.sortQueryParametersByCodePoint = function (url) {
                    var parts = url.split("?");
                    if (parts.length == 1) {
                        return url;
                    }
                    var questring = parts[1];
                    console.log(questring);
                    return url;
                };
                ApiHandler.prototype.post = function (restPath, body, headers) {
                    if (headers === void 0) {
                        headers = [];
                    }
                    var options = {
                        method: http_1.RequestMethod.Post,
                        url: this._serviceAPI + "/" + restPath,
                        headers: headers,
                        body: JSON.stringify(body)
                    };
                    options.headers[HEADER_CONTENT_TYPE] = DEFAULT_CONTENT_TYPE;
                    return this.execute(options);
                };
                ApiHandler.prototype.put = function (restPath, body, headers) {
                    if (headers === void 0) {
                        headers = [];
                    }
                    var options = {
                        method: http_1.RequestMethod.Put,
                        headers: headers,
                        url: this._serviceAPI + "/" + restPath,
                        body: JSON.stringify(body)
                    };
                    options.headers[HEADER_CONTENT_TYPE] = DEFAULT_CONTENT_TYPE;
                    return this.execute(options);
                };
                ApiHandler.prototype.delete = function (restPath, body, headers) {
                    if (headers === void 0) {
                        headers = [];
                    }
                    var options = {
                        method: http_1.RequestMethod.Delete,
                        headers: headers,
                        url: this._serviceAPI + "/" + restPath,
                        body: JSON.stringify(body)
                    };
                    options.headers[HEADER_CONTENT_TYPE] = DEFAULT_CONTENT_TYPE;
                    return this.execute(options);
                };
                ApiHandler.prototype.execute = function (options) {
                    var _this = this;
                    var request_observable = new Observable_1.Observable(function (observer) {
                        var signedRequest = _this.AwsSigv4Signer(options);
                        if (signedRequest == null) {
                            observer.error(new Error("Status: CreateRequestFailed, Message: Unable to create request"));
                        } else {
                            var http_observable = _this._http.request(signedRequest);
                            _this.recordEvent("ApiServiceRequested", {
                                "Identifier": _this._identifier,
                                "Verb": http_1.RequestMethod[options.method],
                                "Path": options.url.replace(_this._serviceAPI, "")
                            }, null);
                            observer.next(_this.mapResponseCodes(http_observable));
                        }
                    });
                    return this._aws.context.authentication.refreshSessionOrLogout(request_observable.concatAll()).concatAll();
                };
                ApiHandler.prototype.mapResponseCodes = function (obs) {
                    var _this = this;
                    return obs.map(function (res) {
                        if (res) {
                            _this.recordEvent("ApiServiceSuccess", {
                                "Identifier": _this._identifier,
                                "Path": res.url.replace(_this._serviceAPI, "")
                            }, null);
                            if (res.status === 201) {
                                return { status: res.status, body: res };
                            } else if (res.status === 200) {
                                return { status: res.status, body: res };
                            }
                        }
                    }).catch(function (error) {
                        var error_body = error._body;
                        if (error_body != undefined) {
                            var obj = undefined;
                            try {
                                obj = JSON.parse(error_body);
                            } catch (err) {}
                            if (obj != undefined) {
                                if (obj.Message != null) {
                                    error.statusText += ", " + obj.Message;
                                } else if (obj.errorMessage != null) {
                                    error.statusText = obj.errorMessage;
                                } else if (obj.message != null) {
                                    error.statusText += ", " + obj.message;
                                }
                            }
                        }
                        _this.recordEvent("ApiServiceError", {
                            "Code": error.status,
                            "Message": error.statusText,
                            "Identifier": _this._identifier
                        }, null);
                        if (error.status == 'Missing credentials in config') {
                            _this._aws.context.authentication.logout(true);
                        }
                        return Observable_1.Observable.throw(new Error("Status: " + error.status + ", Message:" + error.statusText));
                    });
                };
                ApiHandler.prototype.recordEvent = function (eventName, attribute, metric) {
                    if (attribute === void 0) {
                        attribute = {};
                    }
                    if (metric === void 0) {
                        metric = null;
                    }
                    if (this._metric !== null) this._metric.recordEvent(eventName, attribute, metric);
                };
                ApiHandler.prototype.AwsSigv4Signer = function (options) {
                    if (options == null || options == undefined) return null;
                    var parts = options.url.split('?');
                    var host = parts[0].substr(8, parts[0].indexOf("/", 8) - 8);
                    var path = parts[0].substr(parts[0].indexOf("/", 8));
                    var querystring = this.codePointSort(parts[1]);
                    var requestmethod = http_1.RequestMethod[options.method];
                    if (requestmethod == undefined) return null;
                    var now = new Date();
                    if (!options.headers) options.headers = [];
                    options.headers.host = host;
                    options.headers.accept = DEFAULT_ACCEPT_TYPE;
                    options.pathname = function () {
                        return path;
                    };
                    options.methodIndex = options.method;
                    options.method = http_1.RequestMethod[options.method].toLocaleUpperCase();
                    options.search = function () {
                        return querystring ? querystring : "";
                    };
                    options.region = AWS.config.region;
                    var signer = new AWS.Signers.V4(options, SIGNING_API);
                    if (!AWS.config.credentials) this._aws.context.authentication.logout(true);
                    signer.addAuthorization(AWS.config.credentials, now);
                    options.method = options.methodIndex;
                    delete options.search;
                    delete options.pathname;
                    delete options.headers.host;
                    return new http_1.Request(options);
                };
                ;
                ApiHandler.prototype.codePointSort = function (querystring) {
                    if (!querystring) return undefined;
                    var parts = querystring.split("&");
                    var array = [];
                    parts.forEach(function (param) {
                        var key_value = param.split("=");
                        var obj = {
                            key: key_value[0],
                            val: key_value.length > 1 ? key_value[1] : undefined
                        };
                        array.push(obj);
                    });
                    array = array.sort(function (p1, p2) {
                        return p1.key > p2.key ? 1 : p1.key < p2.key ? -1 : 0;
                    });
                    var sorted_querystring = "";
                    var i = 0;
                    array.forEach(function (obj) {
                        i++;
                        sorted_querystring += obj.key + "=" + obj.val;
                        if (i < array.length) sorted_querystring += "&";
                    });
                    return sorted_querystring;
                };
                return ApiHandler;
            }();
            exports_1("ApiHandler", ApiHandler);
        }
    };
});
System.register("dist/app/shared/class/project-api-handler.class.js", ["app/shared/class/index", "rxjs/add/operator/concatAll", "rxjs/add/observable/throw"], function (exports_1, context_1) {
    "use strict";

    var __extends = this && this.__extends || function () {
        var extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
        };
        return function (d, b) {
            extendStatics(d, b);
            function __() {
                this.constructor = d;
            }
            d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
        };
    }();
    var __moduleName = context_1 && context_1.id;
    var index_1, ProjectApi;
    return {
        setters: [function (index_1_1) {
            index_1 = index_1_1;
        }, function (_1) {}, function (_2) {}],
        execute: function () {
            /**
             * API Handler for the RESTful services defined in the cloud gem swagger.json
            */
            ProjectApi = /** @class */function (_super) {
                __extends(ProjectApi, _super);
                function ProjectApi(serviceBaseURL, http, aws) {
                    var _this = _super.call(this, serviceBaseURL, http, aws) || this;
                    _this.serviceBaseURL = serviceBaseURL;
                    return _this;
                }
                ProjectApi.prototype.getDashboard = function (facetid) {
                    return _super.prototype.get.call(this, 'dashboard/' + facetid);
                };
                ProjectApi.prototype.postDashboard = function (facetid, body) {
                    return _super.prototype.post.call(this, 'dashboard/' + facetid, body);
                };
                return ProjectApi;
            }(index_1.ApiHandler);
            exports_1("ProjectApi", ProjectApi);
        }
    };
});
System.register("dist/app/shared/class/index.js", ["./datetime.class", "./log.class", "./string.class", "./apihandler.class", "./environment.class", "./project-api-handler.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    function exportStar_1(m) {
        var exports = {};
        for (var n in m) {
            if (n !== "default") exports[n] = m[n];
        }
        exports_1(exports);
    }
    return {
        setters: [function (datetime_class_1_1) {
            exportStar_1(datetime_class_1_1);
        }, function (log_class_1_1) {
            exportStar_1(log_class_1_1);
        }, function (string_class_1_1) {
            exportStar_1(string_class_1_1);
        }, function (apihandler_class_1_1) {
            exportStar_1(apihandler_class_1_1);
        }, function (environment_class_1_1) {
            exportStar_1(environment_class_1_1);
        }, function (project_api_handler_class_1_1) {
            exportStar_1(project_api_handler_class_1_1);
        }],
        execute: function () {}
    };
});
System.register("dist/app/shared/service/lymetric.service.js", ["@angular/core", "app/shared/class/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, environment, LyMetricService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (environment_1) {
            environment = environment_1;
        }],
        execute: function () {
            LyMetricService = /** @class */function () {
                function LyMetricService() {
                    this._service = undefined;
                    var creds = new AWS.CognitoIdentityCredentials({
                        IdentityPoolId: environment.isProd ? "us-east-1:966bee25-4446-4681-84e5-891fe7b3fbd0" : "us-east-1:5e212aee-a4c4-4670-ae69-18c3d7044199"
                    }, {
                        region: 'us-east-1'
                    });
                    this._service = new AMA.Manager({
                        appTitle: "Cloud Gem Portal",
                        appId: environment.isProd ? "1f52bbba6ceb434daca8aa30f6d1b8f2" : "e1e41196e1c74f88ae970266d68a8ecc",
                        appVersionCode: "v1.0",
                        platform: navigator.appName,
                        platformVersion: navigator.appVersion,
                        model: navigator.userAgent,
                        autoSubmitEvents: true,
                        autoSubmitInterval: 5000,
                        provider: creds,
                        globalAttributes: {
                            "Region": bootstrap.region,
                            "UserAgent": navigator.userAgent ? navigator.userAgent.toLowerCase() : navigator.userAgent,
                            "CognitoDev": bootstrap.cognitoDev,
                            "CognitoTest": bootstrap.cognitoTest,
                            "CognitoProd": bootstrap.cognitoProd
                        },
                        globalMetrics: {},
                        clientOptions: {
                            region: 'us-east-1',
                            credentials: creds
                        }
                    });
                }
                LyMetricService.prototype.recordEvent = function (eventName, attribute, metric) {
                    if (attribute === void 0) {
                        attribute = {};
                    }
                    if (metric === void 0) {
                        metric = null;
                    }
                    if (this.isAuthorized(eventName, attribute)) this._service.recordEvent(eventName, attribute, metric);
                };
                LyMetricService.prototype.isAuthorized = function (eventName, attribute) {
                    if (eventName == "FeatureOpened") {
                        var featureRoute = attribute.Name.toLowerCase();
                        for (var feature in environment.metricWhiteListedFeature) {
                            var normalizedName = environment.metricWhiteListedFeature[feature].trim().replace(' ', '').toLowerCase();
                            if (featureRoute.indexOf(normalizedName) >= 0) {
                                return true;
                            }
                        }
                        return false;
                    }
                    if (eventName == "ModalOpened" || eventName == "FacetOpened" || eventName == "ApiServiceError" || eventName == "ApiServiceSuccess" || eventName == "ApiServiceRequested") {
                        if (!attribute.Identifier) return false;
                        var identifier = attribute.Identifier.trim().replace(' ', '').toLowerCase();
                        for (var index in environment.metricWhiteListedCloudGem) {
                            var normalizedName = environment.metricWhiteListedCloudGem[index].trim().replace(' ', '').toLowerCase();
                            if (identifier.indexOf(normalizedName) >= 0) {
                                return true;
                            }
                        }
                        for (var index in environment.metricWhiteListedFeature) {
                            var normalizedName = environment.metricWhiteListedFeature[index].trim().replace(' ', '').toLowerCase();
                            if (identifier.indexOf(normalizedName) >= 0) {
                                return true;
                            }
                        }
                        return false;
                    }
                    if (eventName == "CloudGemsLoaded") {
                        var cloudGemsInDeployment_1 = attribute.CloudGemsInDeployment.toLowerCase();
                        attribute.CloudGemsInDeployment = environment.metricWhiteListedCloudGem.filter(function (gem) {
                            return cloudGemsInDeployment_1.indexOf(gem.trim().replace(' ', '').toLowerCase()) >= 0;
                        }).join(',');
                        return true;
                    }
                    return true;
                };
                LyMetricService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [])], LyMetricService);
                return LyMetricService;
            }();
            exports_1("LyMetricService", LyMetricService);
        }
    };
});
System.register("dist/app/shared/service/gem.service.js", ["@angular/core", "rxjs/Subject"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, Subject_1, GemService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (Subject_1_1) {
            Subject_1 = Subject_1_1;
        }],
        execute: function () {
            GemService = /** @class */function () {
                function GemService() {
                    this._currentGems = new Array();
                    this._currentGem = null;
                    this._currentGemSubscription = new Subject_1.Subject();
                    this._isLoading = true;
                }
                /**
                 * Add a gem for the gem service to track
                 * @param gem gem to add
                 */
                GemService.prototype.addGem = function (gem) {
                    // Only add a new gem if that identifier isn't present in the c
                    var gemExists = false;
                    this._currentGems.forEach(function (currentGem) {
                        if (currentGem.identifier === gem.identifier) {
                            gemExists = true;
                        }
                    });
                    if (!gemExists) {
                        this._currentGems.push(gem);
                        this._currentGemSubscription.next(gem);
                    }
                };
                Object.defineProperty(GemService.prototype, "currentGems", {
                    /**
                     * Returns an array of the current deployment's gems
                     */
                    get: function () {
                        return this._currentGems;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(GemService.prototype, "isLoading", {
                    get: function () {
                        return this._isLoading;
                    },
                    set: function (isLoading) {
                        this._isLoading = isLoading;
                    },
                    enumerable: true,
                    configurable: true
                });
                /**
                 *  Returns the gem object for the passed in unique gem ID.  If the gem doesn't exist return null
                 * @param gemId
                 */
                GemService.prototype.getGem = function (gemId) {
                    var gemToReturn = null;
                    this._currentGems.forEach(function (gem) {
                        if (gemId === gem.identifier) {
                            gemToReturn = gem;
                        }
                    });
                    return gemToReturn;
                };
                Object.defineProperty(GemService.prototype, "currentGem", {
                    /**
                     * Return the current gem, null if no gem is active
                     */
                    get: function () {
                        return this._currentGem;
                    },
                    // Methods to keep track of the current gem being used
                    set: function (gem) {
                        this._currentGem = gem;
                    },
                    enumerable: true,
                    configurable: true
                });
                GemService.prototype.clearCurrentGems = function () {
                    this._currentGems = [];
                };
                Object.defineProperty(GemService.prototype, "currentGemSubscription", {
                    get: function () {
                        return this._currentGemSubscription.asObservable();
                    },
                    enumerable: true,
                    configurable: true
                });
                GemService = __decorate([core_1.Injectable(), __metadata("design:paramtypes", [])], GemService);
                return GemService;
            }();
            exports_1("GemService", GemService);
        }
    };
});
System.register("dist/app/shared/service/pagination.service.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, PaginationService;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            PaginationService = /** @class */function () {
                function PaginationService() {}
                Object.defineProperty(PaginationService.prototype, "iconClasses", {
                    get: function () {
                        return {
                            sortAscending: 'fa fa-sort-asc',
                            sortDescending: 'fa fa-sort-desc',
                            pagerLeftArrow: 'fa fa-chevron-left',
                            pagerRightArrow: 'fa fa-chevron-right',
                            pagerPrevious: 'fa fa-step-backward',
                            pagerNext: 'fa fa-step-forward'
                        };
                    },
                    enumerable: true,
                    configurable: true
                });
                ;
                PaginationService = __decorate([core_1.Injectable()], PaginationService);
                return PaginationService;
            }();
            exports_1("PaginationService", PaginationService);
        }
    };
});
System.register("dist/app/shared/service/index.js", ["./api.service", "./auth-guard.service", "./definition.service", "./selective-preload-strategy.service", "./url.service", "./lymetric.service", "./gem.service", "./breadcrumb.service", "./pagination.service"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    function exportStar_1(m) {
        var exports = {};
        for (var n in m) {
            if (n !== "default") exports[n] = m[n];
        }
        exports_1(exports);
    }
    return {
        setters: [function (api_service_1_1) {
            exportStar_1(api_service_1_1);
        }, function (auth_guard_service_1_1) {
            exportStar_1(auth_guard_service_1_1);
        }, function (definition_service_1_1) {
            exportStar_1(definition_service_1_1);
        }, function (selective_preload_strategy_service_1_1) {
            exportStar_1(selective_preload_strategy_service_1_1);
        }, function (url_service_1_1) {
            exportStar_1(url_service_1_1);
        }, function (lymetric_service_1_1) {
            exportStar_1(lymetric_service_1_1);
        }, function (gem_service_1_1) {
            exportStar_1(gem_service_1_1);
        }, function (breadcrumb_service_1_1) {
            exportStar_1(breadcrumb_service_1_1);
        }, function (pagination_service_1_1) {
            exportStar_1(pagination_service_1_1);
        }],
        execute: function () {}
    };
});
System.register("dist/app/shared/component/modal.component.js", ["@angular/core", "@ng-bootstrap/ng-bootstrap", "app/shared/service/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, ng_bootstrap_1, index_1, ModalComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (ng_bootstrap_1_1) {
            ng_bootstrap_1 = ng_bootstrap_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }],
        execute: function () {
            ModalComponent = /** @class */function () {
                function ModalComponent(_modalService, lymetrics) {
                    this._modalService = _modalService;
                    this.lymetrics = lymetrics;
                    this.size = 'lg';
                    this.submitButtonText = "Save";
                    this.closeButtonText = "Close";
                    // Boolean for whether or not the modal should have a submit button
                    this.hasSubmit = false;
                    this.width = 500;
                    // Boolean toggle for the submit button
                    this.canSubmit = true;
                    this.modalSubmitted = new core_1.EventEmitter();
                    this._modalOpen = false;
                    this._formDisabled = false;
                }
                ModalComponent.prototype.ngOnInit = function () {
                    this.close = this.close.bind(this);
                    this.dismiss = this.dismiss.bind(this);
                    this.ngAfterViewChecked = this.ngAfterViewChecked.bind(this);
                    this.onSubmit = this.onSubmit.bind(this);
                    this.open = this.open.bind(this);
                    this.lymetrics.recordEvent('ModalOpened', {
                        "Title": this.title,
                        "Identifier": this.metricIdentifier
                    });
                };
                ModalComponent.prototype.close = function (preventFunction) {
                    this._modalRef.close();
                    this._modalOpen = false;
                    if (!preventFunction) preventFunction = false;
                    if (this.onClose && !preventFunction) this.onClose();
                };
                ModalComponent.prototype.dismiss = function (preventFunction) {
                    this._modalRef.dismiss();
                    this._modalOpen = false;
                    if (!preventFunction) preventFunction = false;
                    if (this.onDismiss && !preventFunction) this.onDismiss();
                };
                ModalComponent.prototype.ngAfterViewChecked = function () {
                    var _this = this;
                    if (this.autoOpen && !this._modalOpen) {
                        this._modalOpen = true;
                        window.setTimeout(function () {
                            _this.open(_this.modalTemplate);
                        });
                    }
                };
                ModalComponent.prototype.onSubmit = function () {
                    this.modalSubmitted.emit();
                };
                ModalComponent.prototype.open = function (content) {
                    var _this = this;
                    this._modalRef = this._modalService.open(content, { size: this.size });
                    this._modalRef.result.then(function (reason) {
                        if (reason === "Cancel") _this.close();
                    }, function (reason) {
                        _this.dismiss();
                    });
                };
                __decorate([core_1.Input(), __metadata("design:type", String)], ModalComponent.prototype, "size", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ModalComponent.prototype, "title", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ModalComponent.prototype, "submitButtonText", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ModalComponent.prototype, "closeButtonText", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], ModalComponent.prototype, "hasSubmit", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], ModalComponent.prototype, "autoOpen", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Number)], ModalComponent.prototype, "width", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], ModalComponent.prototype, "canSubmit", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ModalComponent.prototype, "onClose", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Function)], ModalComponent.prototype, "onDismiss", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], ModalComponent.prototype, "metricIdentifier", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], ModalComponent.prototype, "modalSubmitted", void 0);
                __decorate([core_1.ViewChild("content"), __metadata("design:type", Object)], ModalComponent.prototype, "modalTemplate", void 0);
                ModalComponent = __decorate([core_1.Component({
                    selector: 'modal',
                    template: "\n    <ng-template #content let-c=\"close\" let-d=\"dismiss\">\n    <div class=\"modal-header\">\n        <h2>{{title}}</h2>\n        <button type=\"button\" class=\"close\" aria-label=\"Close\" (click)=\"d('Cross click')\">\n        <span aria-hidden=\"true\">&times;</span>\n        </button>\n    </div>\n    <ng-content select=\".modal-body\"></ng-content>\n    <div class=\"modal-footer\">\n        <!-- inline 5px margin here stops button from moving a bit on modal popup -->\n        <button id=\"modal-dismiss\" type=\"button\" class=\"btn btn-outline-primary\" style=\"margin-right: 5px\" (click)=\"c('Cancel')\">{{closeButtonText}}</button>\n        <button *ngIf=\"hasSubmit\" id=\"modal-submit\" type=\"submit\" [disabled]=\"!canSubmit\" (click)=\"onSubmit()\" class=\"btn l-primary\">{{submitButtonText}}</button>\n    </div>\n    </ng-template>\n    <span class=\"modal-trigger-span\" (click)=\"open(content)\"><ng-content select=\".modal-trigger\"></ng-content></span>\n    "
                }), __metadata("design:paramtypes", [ng_bootstrap_1.NgbModal, index_1.LyMetricService])], ModalComponent);
                return ModalComponent;
            }();
            exports_1("ModalComponent", ModalComponent);
        }
    };
});
System.register("dist/app/shared/component/chart-pie.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, PieChartComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            PieChartComponent = /** @class */function () {
                function PieChartComponent(viewContainerRef) {
                    this.viewContainerRef = viewContainerRef;
                    this.width = 900;
                    this.height = 500;
                    this.data = new Array();
                    this.title = "";
                }
                PieChartComponent.prototype.ngOnInit = function () {
                    var _this = this;
                    this.loading = true;
                    System.import('https://m.media-amazon.com/images/G/01/cloudcanvas/d3/d3.min._V506499212_.js').then(function (lib) {
                        _this._d3 = lib;
                        _this.loading = false;
                        _this.setSize(_this.data);
                        var graph = _this.draw(_this.data);
                        _this.addLabels(graph);
                    });
                };
                PieChartComponent.prototype.svg = function () {
                    var elem = this.viewContainerRef.element.nativeElement;
                    return this._d3.select(elem).select("svg");
                };
                PieChartComponent.prototype.chartLayer = function () {
                    return this.svg().append("g").classed("chartLayer", true);
                };
                PieChartComponent.prototype.setSize = function (data) {
                    this._margin = { top: 40, left: 0, bottom: 40, right: 0 };
                    this._chartWidth = this.width - (this._margin.left + this._margin.right);
                    this._chartHeight = this.height - (this._margin.top + this._margin.bottom);
                    this.svg().attr("width", this.width).attr("height", this.height);
                    this.chartLayer().attr("width", this._chartWidth).attr("height", this._chartHeight).attr("transform", "translate(" + [this._margin.left, this._margin.top] + ")");
                    this.svg().attr("align", "center").attr("vertical-align", "middle");
                };
                PieChartComponent.prototype.draw = function (data) {
                    var radius = Math.min(this._chartWidth, this._chartHeight) / 2;
                    var color = this._d3.scaleOrdinal(["#6441A5", "#F29C33", "#67538c", "#FFFFFF", "#415fa5", " #f8f8f8", "#6441A5", "#BBBBBB", "#6441a5", "#9641a5"]);
                    var arcs = this._d3.pie().sort(null).value(function (d) {
                        return d.value;
                    })(data);
                    var arc = this._d3.arc().outerRadius(radius).innerRadius(5).padAngle(0.03).cornerRadius(8);
                    var pieG = this.chartLayer().selectAll("g").data([data]).enter().append("g").attr("transform", "translate(" + [this._chartWidth / 2, this._chartHeight / 2] + ")");
                    var block = pieG.selectAll(".arc").data(arcs);
                    var graph = block.enter().append("g").classed("arc", true);
                    graph.append("path").attr("d", arc).attr("id", function (d, i) {
                        return "arc-" + i;
                    }).attr("stroke", "#999999").attr("fill", function (d, i) {
                        return d.data.color ? d.data.color : color(i);
                    });
                    return graph;
                };
                PieChartComponent.prototype.addLabels = function (graph) {
                    var radius = Math.min(this._chartWidth, this._chartHeight) / 2;
                    var labelArc = this._d3.arc().outerRadius(radius).innerRadius(radius);
                    // text label for the title
                    graph.append("text").attr("class", "label").attr("x", 0).attr("y", 0 - (radius + 10)).attr("text-anchor", "middle").style("font-size", "12px").style("font-family", "AmazonEmber-Regular").text(this.title);
                    // text labels
                    graph.append("text").attr("transform", function (d) {
                        return "translate(" + labelArc.centroid(d) + ")";
                    }).attr("dy", "10px").attr("dx", "-20px").attr("xlink:href", function (d, i) {
                        return "#arc-" + i;
                    }).style("font-size", "12px").text(function (d) {
                        return d.data.label;
                    });
                };
                __decorate([core_1.Input(), __metadata("design:type", Number)], PieChartComponent.prototype, "width", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Number)], PieChartComponent.prototype, "height", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], PieChartComponent.prototype, "data", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], PieChartComponent.prototype, "title", void 0);
                PieChartComponent = __decorate([core_1.Component({
                    selector: 'piechart',
                    template: "        \n        <div class=\"row\">    \n            <div class=\"col-12\">                \n                <loading-spinner *ngIf=\"loading\" [size]=\"'sm'\"></loading-spinner>\n                <svg id=\"\" class=\"pad\"></svg>\n            </div>           \n        </div>        \n    ",
                    styles: ["\n        .pad{\n            padding-top: 30px;\n            padding-left: 40px;\n        }\n    "]
                }), __metadata("design:paramtypes", [core_1.ViewContainerRef])], PieChartComponent);
                return PieChartComponent;
            }();
            exports_1("PieChartComponent", PieChartComponent);
        }
    };
});
System.register("dist/app/shared/component/chart-bar.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, BarChartComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            BarChartComponent = /** @class */function () {
                function BarChartComponent(viewContainerRef) {
                    this.viewContainerRef = viewContainerRef;
                    this.width = 900;
                    this.height = 500;
                    this.data = new Array();
                    this.xaxisLabel = "";
                    this.yaxisLabel = "";
                }
                BarChartComponent.prototype.ngOnInit = function () {
                    var _this = this;
                    this.loading = true;
                    System.import('https://m.media-amazon.com/images/G/01/cloudcanvas/d3/d3.min._V506499212_.js').then(function (lib) {
                        _this.loading = false;
                        _this._d3 = lib;
                        _this.harmonize(_this.data);
                        _this.setSize(_this.data);
                        var graph = _this.draw(_this.data);
                        _this.addLabels(graph);
                    });
                };
                BarChartComponent.prototype.svg = function () {
                    var elem = this.viewContainerRef.element.nativeElement;
                    return this._d3.select(elem).select("svg");
                };
                BarChartComponent.prototype.chartLayer = function () {
                    return this.svg().append("g").classed("chartLayer", true);
                };
                BarChartComponent.prototype.harmonize = function (data) {
                    var maxDigits = this._d3.max(data, function (d) {
                        return d.value;
                    }).toString().length;
                    if (maxDigits > 6) {
                        this._perThousands = true;
                        var numberOfThousands = maxDigits / 3;
                        data.forEach(function (d) {
                            d.value = d.value / (1000 * (maxDigits - 1));
                        });
                    }
                };
                BarChartComponent.prototype.setSize = function (data) {
                    this._margin = { top: 30, left: 40, bottom: 40, right: 40 };
                    this._chartWidth = this.width - (this._margin.left + this._margin.right);
                    this._chartHeight = this.height - (this._margin.top + this._margin.bottom);
                    this.svg().attr("width", this.width).attr("height", this.height);
                    this.chartLayer().attr("width", this._chartWidth).attr("height", this._chartHeight).append("g").attr("transform", "translate(" + [this._margin.left, this._margin.top] + ")");
                    this.svg().attr("align", "center").attr("vertical-align", "middle");
                };
                BarChartComponent.prototype.draw = function (data) {
                    var w = this._chartWidth;
                    var h = this._chartHeight;
                    // set the ranges
                    var x = this._d3.scaleBand().domain(data.map(function (d) {
                        return d.label;
                    })).range([0, w]).padding(0.2);
                    var y = this._d3.scaleLinear().domain([0, this._d3.max(data, function (d) {
                        return d.value;
                    })]).range([h, 0]);
                    // append the rectangles for the bar chart
                    this.chartLayer().selectAll("g").data(data).enter().append("rect").attr("class", "bar").attr("x", function (d) {
                        return x(d.label);
                    }).attr("y", function (d) {
                        return y(d.value);
                    }).attr("width", x.bandwidth()).attr("height", function (d) {
                        return h - y(d.value);
                    }).attr("fill", function (d, i) {
                        return d.color ? d.color : "#6441A5";
                    });
                    // text label for the bars
                    this.chartLayer().selectAll(".text").data(data).enter().append("text").attr("class", "label").attr("x", function (d) {
                        return x(d.label) + x.bandwidth() / 2;
                    }).attr("y", function (d) {
                        return y(d.value) - 10;
                    }).attr("text-anchor", "middle").style("font-size", "10px").text(function (d) {
                        return d.value;
                    });
                    // x-Axis
                    this.chartLayer().append("g").attr("transform", "translate(0," + h + ")").call(this._d3.axisBottom(x));
                    // y-Axis
                    if (!this.noYLabels) {
                        this.chartLayer().append("g").call(this._d3.axisLeft(y));
                    }
                    return this.chartLayer();
                };
                BarChartComponent.prototype.addLabels = function (graph) {
                    // text label for the x-axis
                    graph.append("text").attr("x", this._chartWidth / 2).attr("y", this._chartHeight + this._margin.bottom - 5).attr("text-anchor", "middle").style("font-size", "12px").text(this.xaxisLabel);
                    // text label for the y axis
                    if (!this.noYLabels) {
                        graph.append("text").attr("transform", "rotate(-90)").attr("y", 0 - 70).attr("x", 0 - this._chartHeight / 2).attr("dy", "1em").style("text-anchor", "middle").style("font-size", "12px").text(this._perThousands ? this.yaxisLabel + " (Thousands)" : this.yaxisLabel);
                    }
                    // text label for the title
                    graph.append("text").attr("class", "label").attr("x", this._chartWidth / 2).attr("y", 0 - this._margin.top / 2).attr("text-anchor", "middle").style("font-size", "12px").style("font-weight", "bold").style("font-family", "AmazonEmber-Regular").text(this.title);
                };
                __decorate([core_1.Input(), __metadata("design:type", Number)], BarChartComponent.prototype, "width", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Number)], BarChartComponent.prototype, "height", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], BarChartComponent.prototype, "data", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], BarChartComponent.prototype, "title", void 0);
                __decorate([core_1.Input('x-axis-label'), __metadata("design:type", String)], BarChartComponent.prototype, "xaxisLabel", void 0);
                __decorate([core_1.Input('y-axis-label'), __metadata("design:type", String)], BarChartComponent.prototype, "yaxisLabel", void 0);
                __decorate([core_1.Input('no-yaxis-labels'), __metadata("design:type", Boolean)], BarChartComponent.prototype, "noYLabels", void 0);
                BarChartComponent = __decorate([core_1.Component({
                    selector: 'barchart',
                    template: "        \n        <div class=\"row\">    \n            <div class=\"col-12\">\n                <loading-spinner *ngIf=\"loading\" [size]=\"'sm'\"></loading-spinner>\n                <svg class=\"pad\"></svg>\n            </div>           \n        </div>       \n    ",
                    styles: ["\n        .pad{\n           padding-left: 75px;\n           padding-top: 30px;\n        }        \n    "]
                }), __metadata("design:paramtypes", [core_1.ViewContainerRef])], BarChartComponent);
                return BarChartComponent;
            }();
            exports_1("BarChartComponent", BarChartComponent);
        }
    };
});
System.register("dist/app/shared/component/chart-line.component.js", ["@angular/core", "d3", "d3-svg-legend"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, d3, d3_svg_legend_1, LineChartComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (d3_1) {
            d3 = d3_1;
        }, function (d3_svg_legend_1_1) {
            d3_svg_legend_1 = d3_svg_legend_1_1;
        }],
        execute: function () {
            LineChartComponent = /** @class */function () {
                function LineChartComponent(viewContainerRef) {
                    this.viewContainerRef = viewContainerRef;
                    this.width = 900;
                    this.height = 500;
                    this.dataArray = new Array();
                    this.labelArray = new Array();
                    this.isLeftYaxises = new Array();
                    this.hasRightYAxis = false;
                    this.isXaxisUnitTime = false;
                    this.xaxisLabel = "";
                    this.leftYaxisLabel = "";
                    this.rightYaxisLabel = "";
                }
                LineChartComponent.prototype.ngOnInit = function () {
                    this.loading = false;
                    this._d3 = d3;
                    this._legend = d3_svg_legend_1.default;
                    this.setSize();
                    var graph = this.draw();
                    this.addLabels(graph);
                };
                LineChartComponent.prototype.svg = function () {
                    var elem = this.viewContainerRef.element.nativeElement;
                    return this._d3.select(elem).select("svg");
                };
                LineChartComponent.prototype.chartLayer = function () {
                    return this.svg().append("g").classed("chartLayer", true);
                };
                LineChartComponent.prototype.setSize = function () {
                    this._margin = { top: 30, left: 40, bottom: 40, right: 260 };
                    this._chartWidth = this.width - (this._margin.left + this._margin.right);
                    this._chartHeight = this.height - (this._margin.top + this._margin.bottom);
                    this.svg().attr("width", this.width).attr("height", this.height);
                    this.chartLayer().attr("width", this._chartWidth).attr("height", this._chartHeight).append("g").attr("transform", "translate(" + [this._margin.left, this._margin.top] + ")");
                    this.svg().attr("align", "center").attr("vertical-align", "middle");
                };
                LineChartComponent.prototype.addLegend = function () {
                    var ordinal = this._d3.scaleOrdinal(this._d3.schemeCategory20).domain(this.labelArray);
                    this.svg().append("g").attr("class", "legendOrdinal").attr("transform", "translate(" + (this._chartWidth + 60) + ",0)");
                    var legendOrdinal = this._legend.legendColor().shapeWidth(5).scale(ordinal);
                    this.svg().select(".legendOrdinal").call(legendOrdinal);
                };
                LineChartComponent.prototype.draw = function () {
                    var w = this._chartWidth;
                    var h = this._chartHeight;
                    var x;
                    if (!this.isXaxisUnitTime) {
                        x = this._d3.scaleLinear().range([0, w]);
                    } else {
                        x = this._d3.scaleTime().range([0, w]);
                    }
                    var left_y = this._d3.scaleLinear().range([h, 0]);
                    var right_y;
                    if (this.hasRightYAxis) {
                        right_y = this._d3.scaleLinear().range([h, 0]);
                    }
                    var line_left_yaxis = this._d3.line().x(function (d) {
                        return x(d.label);
                    }).y(function (d) {
                        return left_y(d.value);
                    });
                    var line_right_yaxis;
                    if (this.hasRightYAxis) {
                        line_right_yaxis = this._d3.line().x(function (d) {
                            return x(d.label);
                        }).y(function (d) {
                            return right_y(d.value);
                        });
                    }
                    // scale the range of the data
                    var minMaxX = this.findMinMaxX(this.dataArray);
                    var x_min = minMaxX[0];
                    var x_max = minMaxX[1];
                    x.domain([x_min, x_max]);
                    if (this.hasRightYAxis) {
                        var leftRightYMax = this.findMaxLeftRightY(this.dataArray);
                        var left_y_max = leftRightYMax[0];
                        var right_y_max = leftRightYMax[1];
                        left_y.domain([0, left_y_max + left_y_max / 10]);
                        right_y.domain([0, right_y_max + right_y_max / 10]);
                    } else {
                        var left_y_max = this.findMaxLeftY(this.dataArray);
                        left_y.domain([0, left_y_max + left_y_max / 10]);
                    }
                    // add x-Axis
                    this.chartLayer().append("g").attr("transform", "translate(0," + h + ")").call(this._d3.axisBottom(x));
                    // add lines
                    var color = this._d3.scaleOrdinal(this._d3.schemeCategory20);
                    if (this.hasRightYAxis) {
                        for (var i = 0; i < this.dataArray.length; i++) {
                            var data = this.dataArray[i];
                            if (this.isLeftYaxises[i]) {
                                this.addLine(data, line_left_yaxis, left_y, color(i));
                            } else {
                                this.addLine(data, line_right_yaxis, right_y, color(i));
                            }
                        }
                    } else {
                        for (var i = 0; i < this.dataArray.length; i++) {
                            var data = this.dataArray[i];
                            this.addLine(data, line_left_yaxis, left_y, color(i));
                        }
                    }
                    // add y-Axis
                    this.chartLayer().append("g").call(this._d3.axisLeft(left_y));
                    if (this.hasRightYAxis) {
                        this.chartLayer().append("g").attr("transform", "translate(" + w + ",0)").call(this._d3.axisRight(right_y));
                    }
                    // add legend
                    this.addLegend();
                    return this.chartLayer();
                };
                LineChartComponent.prototype.findMinMaxX = function (dataArray) {
                    var x_min = null;
                    var x_max = null;
                    for (var _i = 0, dataArray_1 = dataArray; _i < dataArray_1.length; _i++) {
                        var data = dataArray_1[_i];
                        for (var _a = 0, data_1 = data; _a < data_1.length; _a++) {
                            var point = data_1[_a];
                            if (x_min == null) {
                                x_min = point.label;
                            } else {
                                if (point.label < x_min) {
                                    x_min = point.label;
                                }
                            }
                            if (x_max == null) {
                                x_max = point.label;
                            } else {
                                if (point.label > x_max) {
                                    x_max = point.label;
                                }
                            }
                        }
                    }
                    return [x_min, x_max];
                };
                LineChartComponent.prototype.findMaxLeftRightY = function (dataArray) {
                    var _this = this;
                    var left_y_max = null;
                    dataArray.forEach(function (data, i) {
                        if (_this.isLeftYaxises[i]) {
                            data.forEach(function (point) {
                                if (left_y_max == null) {
                                    left_y_max = point.value;
                                } else {
                                    if (point.value > left_y_max) {
                                        left_y_max = point.value;
                                    }
                                }
                            });
                        }
                    });
                    var right_y_max = null;
                    dataArray.forEach(function (data, i) {
                        if (!_this.isLeftYaxises[i]) {
                            data.forEach(function (point) {
                                if (right_y_max == null) {
                                    right_y_max = point.value;
                                } else {
                                    if (point.value > right_y_max) {
                                        right_y_max = point.value;
                                    }
                                }
                            });
                        }
                    });
                    return [left_y_max, right_y_max];
                };
                LineChartComponent.prototype.findMaxLeftY = function (dataArray) {
                    var left_y_max = null;
                    dataArray.forEach(function (data) {
                        data.forEach(function (point) {
                            if (left_y_max == null) {
                                left_y_max = point.value;
                            } else {
                                if (point.value > left_y_max) {
                                    left_y_max = point.value;
                                }
                            }
                        });
                    });
                    return left_y_max;
                };
                LineChartComponent.prototype.addLine = function (data, line, y, color) {
                    if (data.length == 0) {
                        return;
                    }
                    this.svg().append("path").datum(data).attr("fill", "none").style("stroke", color).attr("d", line);
                };
                LineChartComponent.prototype.addLabels = function (graph) {
                    // text label for the x-axis
                    graph.append("text").attr("x", this._chartWidth / 2).attr("y", this._chartHeight + this._margin.bottom - 5).attr("text-anchor", "middle").style("font-size", "12px").text(this.xaxisLabel);
                    // text label for the y axis
                    if (this.leftYaxisLabel) {
                        graph.append("text").attr("transform", "rotate(-90)").attr("y", 0 - 70).attr("x", 0 - this._chartHeight / 2).attr("dy", "1em").style("text-anchor", "middle").style("font-size", "12px").text(this.leftYaxisLabel);
                    }
                    if (this.rightYaxisLabel) {
                        graph.append("text").attr("transform", "rotate(-90)").attr("y", this._chartWidth + 55).attr("x", 0 - this._chartHeight / 2).attr("dy", "1em").style("text-anchor", "middle").style("font-size", "12px").text(this.rightYaxisLabel);
                    }
                    // text label for the title
                    graph.append("text").attr("class", "label").attr("x", this._chartWidth / 2).attr("y", 0 - this._margin.top / 2).attr("text-anchor", "middle").style("font-size", "12px").style("font-weight", "bold").style("font-family", "AmazonEmber-Regular").text(this.title);
                };
                __decorate([core_1.Input(), __metadata("design:type", Number)], LineChartComponent.prototype, "width", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Number)], LineChartComponent.prototype, "height", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], LineChartComponent.prototype, "dataArray", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], LineChartComponent.prototype, "labelArray", void 0);
                __decorate([core_1.Input('is-left-yaxises'), __metadata("design:type", Object)], LineChartComponent.prototype, "isLeftYaxises", void 0);
                __decorate([core_1.Input('has-right-yaxis'), __metadata("design:type", Boolean)], LineChartComponent.prototype, "hasRightYAxis", void 0);
                __decorate([core_1.Input('is-xaxis-unit-time'), __metadata("design:type", Boolean)], LineChartComponent.prototype, "isXaxisUnitTime", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], LineChartComponent.prototype, "title", void 0);
                __decorate([core_1.Input('x-axis-label'), __metadata("design:type", String)], LineChartComponent.prototype, "xaxisLabel", void 0);
                __decorate([core_1.Input('left-y-axis-label'), __metadata("design:type", String)], LineChartComponent.prototype, "leftYaxisLabel", void 0);
                __decorate([core_1.Input('right-y-axis-label'), __metadata("design:type", String)], LineChartComponent.prototype, "rightYaxisLabel", void 0);
                LineChartComponent = __decorate([core_1.Component({
                    selector: 'linechart',
                    template: "\n        <div class=\"row\">\n            <div class=\"col-12\">\n                <loading-spinner *ngIf=\"loading\" [size]=\"'sm'\"></loading-spinner>\n                <svg class=\"pad\"></svg>\n            </div>\n        </div>\n    ",
                    styles: ["\n        .pad{\n           padding-left: 75px;\n           padding-top: 30px;\n        }\n        /* Allow negative svg content to be visible */\n        svg {\n            overflow: visible;\n        }\n    "]
                }), __metadata("design:paramtypes", [core_1.ViewContainerRef])], LineChartComponent);
                return LineChartComponent;
            }();
            exports_1("LineChartComponent", LineChartComponent);
        }
    };
});
System.register("dist/app/shared/component/tag.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, TagComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            TagComponent = /** @class */function () {
                function TagComponent() {
                    this.delete = new core_1.EventEmitter();
                }
                TagComponent.prototype.ngOnInit = function () {
                    var s = this.typeName;
                };
                TagComponent.prototype.deleteTag = function (tag) {
                    this.delete.emit();
                };
                __decorate([core_1.Input(), __metadata("design:type", String)], TagComponent.prototype, "tagName", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], TagComponent.prototype, "isEditing", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], TagComponent.prototype, "typeName", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], TagComponent.prototype, "delete", void 0);
                TagComponent = __decorate([core_1.Component({
                    selector: 'tag',
                    template: "\n        <label [ngClass]=\"tagName == 'Show all tags' || tagName == 'Hide' ? 'badge badge-info' : 'badge badge-primary'\" isEditing=\"isEditing\"> \n            {{tagName}}\n            <span *ngIf=\"isEditing == 'true'\">\n                <i class=\"fa fa-times\" data-toggle=\"tooltip\" data-placement=\"top\" title=\"Remove\" (click)=\"deleteTag(tag)\"></i>\n            </span>\n        </label>\n    ",
                    styles: [".badge{font-family:\"AmazonEmber-Light\";font-size:12px}.badge:hover{cursor:pointer}.badge-primary{background-color:#6441A5;color:#fff}.badge-info{background-color:#fff;color:#6441A5;border:1px solid #6441A5}.fa-times{color:#fff}"]
                })], TagComponent);
                return TagComponent;
            }();
            exports_1("TagComponent", TagComponent);
        }
    };
});
System.register("dist/app/shared/component/inline-editing.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, core_2, core_3, InlineEditingComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
            core_2 = core_1_1;
            core_3 = core_1_1;
        }],
        execute: function () {
            InlineEditingComponent = /** @class */function () {
                function InlineEditingComponent() {
                    this.contentChange = new core_1.EventEmitter();
                    this.startEditing = new core_1.EventEmitter();
                    this.newTagContent = "";
                }
                InlineEditingComponent.prototype.ngOnChanges = function (change) {
                    if (this.type == "tags") {
                        if (change.isEditing && !change.isEditing.currentValue) {
                            this.generateTagsInDisplayMode();
                        } else {
                            this.newTagContent = "";
                        }
                    } else if (this.type == "text" && this.inputFieldRef) {
                        var nativeElement = this.inputFieldRef.nativeElement;
                        nativeElement.style.height = this.isEditing ? nativeElement.scrollHeight + "px" : "30px";
                    }
                };
                InlineEditingComponent.prototype.change = function () {
                    this.contentChange.emit(this.content);
                };
                InlineEditingComponent.prototype.changeDisplayOption = function (tag) {
                    if (tag == "Show all tags") {
                        this.displayMode = true;
                        this.tagsInDisplayMode = JSON.parse(JSON.stringify(this.content));
                        this.tagsInDisplayMode.push("Hide");
                    } else if (tag == "Hide") {
                        this.displayMode = true;
                        this.addShowAllTag();
                    }
                };
                InlineEditingComponent.prototype.editContent = function () {
                    if (this.type == "text") {
                        this.inputFieldRef.nativeElement.focus();
                    } else if (this.type == "tags") {
                        if (this.displayMode) {
                            this.displayMode = false;
                            return;
                        }
                        this.newTagInputRef.nativeElement.className = "new-tag";
                        this.newTagInputRef.nativeElement.focus();
                    }
                    this.startEditing.emit();
                };
                InlineEditingComponent.prototype.editNewTag = function ($event) {
                    if (this.newTagContent == "" && $event != "") {
                        this.content.push($event);
                    } else {
                        this.content[this.content.length - 1] = $event;
                    }
                    this.newTagContent = $event;
                };
                InlineEditingComponent.prototype.addNewTag = function () {
                    this.newTagContent = "";
                };
                InlineEditingComponent.prototype.deleteLastTag = function () {
                    if (this.newTagContent == "") {
                        this.content.pop();
                    }
                };
                InlineEditingComponent.prototype.deleteTag = function (tag) {
                    var index = this.content.indexOf(tag, 0);
                    if (index > -1) {
                        this.content.splice(index, 1);
                    }
                };
                InlineEditingComponent.prototype.generateTagsInDisplayMode = function () {
                    this.tagsInDisplayMode = [];
                    if (this.content.length <= 4) {
                        this.tagsInDisplayMode = JSON.parse(JSON.stringify(this.content));
                    } else {
                        this.addShowAllTag();
                    }
                };
                InlineEditingComponent.prototype.addShowAllTag = function () {
                    this.tagsInDisplayMode = this.content.slice(0, 4);
                    this.tagsInDisplayMode.push("Show all tags");
                };
                __decorate([core_1.Input(), __metadata("design:type", Object)], InlineEditingComponent.prototype, "content", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], InlineEditingComponent.prototype, "type", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], InlineEditingComponent.prototype, "isEditing", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], InlineEditingComponent.prototype, "contentChange", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], InlineEditingComponent.prototype, "startEditing", void 0);
                __decorate([core_2.ViewChild('newTag'), __metadata("design:type", core_3.ElementRef)], InlineEditingComponent.prototype, "newTagInputRef", void 0);
                __decorate([core_2.ViewChild('inputField'), __metadata("design:type", core_3.ElementRef)], InlineEditingComponent.prototype, "inputFieldRef", void 0);
                InlineEditingComponent = __decorate([core_1.Component({
                    selector: 'inline-editing',
                    template: "\n        <div [ngSwitch]=\"type\">\n            <div *ngSwitchCase=\"'text'\">\n                <textarea #inputField [ngClass]=\"isEditing ? 'editable-content edit-mode' : 'editable-content view-mode'\" [(ngModel)]=\"content\" (ngModelChange)=\"change()\" (mousedown)=\"editContent()\"></textarea>\n            </div>\n            <div *ngSwitchCase=\"'tags'\" [ngClass]=\"isEditing ? 'editable-content edit-mode' : 'editable-content view-mode'\" (click)=\"editContent()\">\n                <span [ngSwitch]=\"isEditing\">\n                    <span *ngSwitchCase=\"false\">\n                        <tag *ngFor=\"let tag of this.tagsInDisplayMode\" [tagName]=\"tag\" (click)=\"changeDisplayOption(tag)\"></tag>\n                    </span>\n                    <span *ngSwitchCase=\"true\">\n                        <span *ngFor=\"let tag of content let i = index\">\n                            <tag *ngIf=\"i < content.length - 1 || (tag != '' && newTagContent == '')\" [tagName]=\"tag\" [isEditing]=\"isEditing ? 'true' : 'false'\" (delete)=\"deleteTag(tag)\"></tag>\n                        </span>                 \n                    </span>\n                </span>\n                <input #newTag type=\"text\" [ngClass]=\"content.length == 0 || isEditing ? 'new-tag' : 'new-tag-hide'\" [ngModel]=\"newTagContent\" (ngModelChange)=\"editNewTag($event)\" (keyup.enter)=\"addNewTag()\" (keyup.backspace)=\"deleteLastTag()\"> \n            </div>\n        </div>\n    ",
                    styles: [".editable-content{width:100%;line-height:26px;padding-left:2px}.editable-content.view-mode{background:transparent;border:transparent;cursor:pointer;resize:none}.editable-content.view-mode:hover{background:#fcfaed;border:1px solid #444}.editable-content.edit-mode{background:white;border:1px solid #444}.new-tag{background:transparent;border:transparent;margin-left:2px}.new-tag:hover{cursor:pointer}.edit-mode:focus,.new-tag:focus{outline:none}.new-tag-hide{display:none}"]
                })], InlineEditingComponent);
                return InlineEditingComponent;
            }();
            exports_1("InlineEditingComponent", InlineEditingComponent);
        }
    };
});
System.register("dist/app/shared/component/switch-button.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, SwitchButtonComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            SwitchButtonComponent = /** @class */function () {
                function SwitchButtonComponent() {
                    this.enabledChange = new core_1.EventEmitter();
                    this.switch = new core_1.EventEmitter();
                }
                SwitchButtonComponent.prototype.switchStatusChange = function () {
                    this.enabledChange.emit(this.enabled);
                    this.switch.emit();
                };
                __decorate([core_1.Input(), __metadata("design:type", Boolean)], SwitchButtonComponent.prototype, "enabled", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], SwitchButtonComponent.prototype, "enabledChange", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], SwitchButtonComponent.prototype, "switch", void 0);
                SwitchButtonComponent = __decorate([core_1.Component({
                    selector: 'switch-button',
                    template: "\n        <label class=\"switch\">\n            <input type=\"checkbox\" [(ngModel)]=\"enabled\" (ngModelChange)=\"switchStatusChange()\">\n            <span class=\"slider round\"></span>\n        </label>\n    ",
                    styles: [".switch{position:relative;display:inline-block;width:44px;height:24px;margin-bottom:-5px;margin-left:5px}.switch input{display:none}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;transition:.4s;border-radius:24px}.slider:before{position:absolute;content:\"\";height:20px;width:20px;left:2px;bottom:2px;background-color:white;transition:.4s;border-radius:24px}input:checked+.slider{background-color:#6441A5}input:checked+.slider:before{transform:translateX(20px)}"]
                })], SwitchButtonComponent);
                return SwitchButtonComponent;
            }();
            exports_1("SwitchButtonComponent", SwitchButtonComponent);
        }
    };
});
System.register("dist/app/shared/service/breadcrumb.service.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, BreadcrumbService, Breadcrumb;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            BreadcrumbService = /** @class */function () {
                function BreadcrumbService() {
                    this._breadcrumbs = new Array();
                }
                /** Adds a breadcrumb to the existing list of breadcrumbs
                 * @param breadcrumb the breadcrumb to add
                **/
                BreadcrumbService.prototype.addBreadcrumb = function (label, functionCb) {
                    this._breadcrumbs.push(new Breadcrumb(label, functionCb));
                };
                /**
                 * Remove the last breadcrumb
                 * @return the breadcrumb that was removed
                **/
                BreadcrumbService.prototype.removeLastBreadcrumb = function () {
                    return this._breadcrumbs.pop();
                };
                /**
                 * Removes all the current breadcrumbs from the stack.
                **/
                BreadcrumbService.prototype.removeAllBreadcrumbs = function () {
                    for (var i = this._breadcrumbs.length - 1; i >= 0; i--) {
                        this.removeLastBreadcrumb();
                    }
                };
                Object.defineProperty(BreadcrumbService.prototype, "breadcrumbs", {
                    get: function () {
                        return this._breadcrumbs;
                    },
                    enumerable: true,
                    configurable: true
                });
                BreadcrumbService = __decorate([core_1.Injectable()
                /**
                 * Breadcrumb Service
                 * Keeps track of the current state of the breadcrumbs for gems.  This service allows
                 * gems to modify the current breadcrumbs to suit their implementation.  The underlying
                 * data model for this is just a stack.
                */

                , __metadata("design:paramtypes", [])], BreadcrumbService);
                return BreadcrumbService;
            }();
            exports_1("BreadcrumbService", BreadcrumbService);
            Breadcrumb = /** @class */function () {
                function Breadcrumb(label, functionCb) {
                    this._label = label;
                    this._functionCb = functionCb;
                }
                Object.defineProperty(Breadcrumb.prototype, "label", {
                    get: function () {
                        return this._label;
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Breadcrumb.prototype, "functionCb", {
                    get: function () {
                        return this._functionCb;
                    },
                    enumerable: true,
                    configurable: true
                });
                return Breadcrumb;
            }();
        }
    };
});
System.register("dist/app/shared/component/breadcrumb.component.js", ["@angular/core", "../service/breadcrumb.service"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, breadcrumb_service_1, BreadcrumbComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (breadcrumb_service_1_1) {
            breadcrumb_service_1 = breadcrumb_service_1_1;
        }],
        execute: function () {
            BreadcrumbComponent = /** @class */function () {
                function BreadcrumbComponent(breadcrumbService) {
                    this.breadcrumbService = breadcrumbService;
                }
                BreadcrumbComponent.prototype.ngOnInit = function () {};
                BreadcrumbComponent = __decorate([core_1.Component({
                    selector: 'breadcrumbs',
                    template: "\n\t<span *ngFor=\"let breadcrumb of breadcrumbService?.breadcrumbs; let i = index\" [attr.data-index]=\"i\">\n\t\t<ng-container [ngSwitch]=\"i + 1 === breadcrumbService?.breadcrumbs.length\">\n\t\t\t<!-- If this is the last breadcrumb, make it a header and not a usable link -->\n\t\t\t<div *ngSwitchCase=\"true\">\n\t\t\t\t<h1> {{breadcrumb.label}} </h1>\n\t\t\t</div>\n\t\t\t<span *ngSwitchCase=\"false\" class=\"breadcrumb-link\">\n\t\t\t\t<a (click)=\"breadcrumb.functionCb()\"> {{breadcrumb.label}} </a>\n\t\t\t\t<i class=\"fa fa-angle-right\" aria-hidden=\"true\"></i>\n\t\t\t</span>\n\t\t</ng-container>\n\t</span>\n\t",
                    styles: [".breadcrumb-link>*{font-size:13px}.breadcrumb-link>a{font-family:\"AmazonEmber-Light\"}.breadcrumb-link>a:hover,.breadcrumb-link>a:focus{text-decoration:none;color:#8A0ECC}"]
                }), __metadata("design:paramtypes", [breadcrumb_service_1.BreadcrumbService])], BreadcrumbComponent);
                return BreadcrumbComponent;
            }();
            exports_1("BreadcrumbComponent", BreadcrumbComponent);
        }
    };
});
System.register("dist/app/shared/component/dropdown.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, DropdownComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            DropdownComponent = /** @class */function () {
                function DropdownComponent() {
                    // Optional event to listen to that returns the current dropdown option whenever it's changed.
                    this.dropdownChanged = new core_1.EventEmitter();
                }
                DropdownComponent.prototype.ngOnInit = function () {
                    // If the first option doesn't exist at all then this isn't a functional dropdown so just return.
                    if (!this.options || !this.options[0]) {
                        return;
                    }
                    if (this.currentOption && this.currentOption.text) {
                        this.dropdownText = this.currentOption.text;
                    } else if (this.placeholderText) {
                        this.dropdownText = this.placeholderText;
                        this.currentOption = null;
                    } else {
                        this.dropdownText = this.options[0].text;
                    }
                    if (this.options) {
                        this.currentOption = this.options[0];
                    }
                };
                /**
                 * Runs whenever the dropdown option is changed.  Bound to a click event in the tempalte.
                 * @param option
                 */
                DropdownComponent.prototype.changedDropdown = function (option) {
                    this.dropdownChanged.emit(option);
                    this.currentOption = option;
                    this.dropdownText = option.text;
                    if (option.functionCb) {
                        option.functionCb(option.args);
                    }
                };
                __decorate([core_1.Input(), __metadata("design:type", Array)], DropdownComponent.prototype, "options", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], DropdownComponent.prototype, "currentOption", void 0);
                __decorate([core_1.Input(), __metadata("design:type", String)], DropdownComponent.prototype, "placeholderText", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Number)], DropdownComponent.prototype, "width", void 0);
                __decorate([core_1.Output(), __metadata("design:type", Object)], DropdownComponent.prototype, "dropdownChanged", void 0);
                DropdownComponent = __decorate([core_1.Component({
                    selector: 'dropdown',
                    template: "\n        <div *ngIf=\"options\" class=\"d-inline-block\" ngbDropdown>\n            <button [ngStyle]=\"{'width.px': width }\" type=\"button\" class=\"btn l-dropdown\" ngbDropdownToggle>\n                <span> {{ dropdownText }} </span>\n                <span class=\"fa fa-caret-down\" style=\"position:absolute;right:5px\" aria-hidden=\"true\"></span>\n            </button>\n            <div class=\"dropdown-menu\" aria-labelledby=\"priority-dropdown\">\n                <button (click)=\"changedDropdown(option)\" type=\"button\" *ngFor=\"let option of options\" class=\"dropdown-item\">\n                    {{ option.text }}\n                </button>\n            </div>\n        </div>\n    "
                }), __metadata("design:paramtypes", [])], DropdownComponent);
                return DropdownComponent;
            }();
            exports_1("DropdownComponent", DropdownComponent);
        }
    };
});
System.register("dist/app/shared/component/progress-bar.component.js", ["@angular/core"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, ProgressBarComponent;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }],
        execute: function () {
            ProgressBarComponent = /** @class */function () {
                function ProgressBarComponent() {
                    this.hasError = false;
                }
                __decorate([core_1.Input(), __metadata("design:type", Number)], ProgressBarComponent.prototype, "width", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Number)], ProgressBarComponent.prototype, "height", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Number)], ProgressBarComponent.prototype, "percent", void 0);
                __decorate([core_1.Input(), __metadata("design:type", Object)], ProgressBarComponent.prototype, "hasError", void 0);
                ProgressBarComponent = __decorate([core_1.Component({
                    selector: 'progress-bar',
                    template: "\n        <span class=\"progress-border\" [style.width.px]=\"width\" [style.height.px]=\"height\">\n            <span class=\"cgp-progress-bar\" [class.error-background]=\"hasError\" [class.progress-background]=\"!hasError\" [style.width.%]=\"percent\"></span>\n            <span class=\"progress-percent\" [style.line-height.px]=\"height\"> {{percent}}% </span>\n        </span>\n    ",
                    styles: [".progress-border{border:1px solid;color:#959595;display:inline-block;position:relative}.error-background{background-color:#d63a32}.progress-background{background-color:#a26bff}.cgp-progress-bar{height:100%;display:inline-block}.progress-percent{position:absolute;left:0;top:0;text-align:center;width:100%;color:black}"]
                })], ProgressBarComponent);
                return ProgressBarComponent;
            }();
            exports_1("ProgressBarComponent", ProgressBarComponent);
        }
    };
});
System.register("dist/app/shared/component/index.js", ["./auto-focus.component", "./datetime-range-picker.component", "./footer.component", "./tooltip.component", "./loading-spinner.component", "./modal.component", "./chart-pie.component", "./chart-bar.component", "./chart-line.component", "./tag.component", "./inline-editing.component", "./switch-button.component", "./breadcrumb.component", "./dropdown.component", "./progress-bar.component"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    function exportStar_1(m) {
        var exports = {};
        for (var n in m) {
            if (n !== "default") exports[n] = m[n];
        }
        exports_1(exports);
    }
    return {
        setters: [function (auto_focus_component_1_1) {
            exportStar_1(auto_focus_component_1_1);
        }, function (datetime_range_picker_component_1_1) {
            exportStar_1(datetime_range_picker_component_1_1);
        }, function (footer_component_1_1) {
            exportStar_1(footer_component_1_1);
        }, function (tooltip_component_1_1) {
            exportStar_1(tooltip_component_1_1);
        }, function (loading_spinner_component_1_1) {
            exportStar_1(loading_spinner_component_1_1);
        }, function (modal_component_1_1) {
            exportStar_1(modal_component_1_1);
        }, function (chart_pie_component_1_1) {
            exportStar_1(chart_pie_component_1_1);
        }, function (chart_bar_component_1_1) {
            exportStar_1(chart_bar_component_1_1);
        }, function (chart_line_component_1_1) {
            exportStar_1(chart_line_component_1_1);
        }, function (tag_component_1_1) {
            exportStar_1(tag_component_1_1);
        }, function (inline_editing_component_1_1) {
            exportStar_1(inline_editing_component_1_1);
        }, function (switch_button_component_1_1) {
            exportStar_1(switch_button_component_1_1);
        }, function (breadcrumb_component_1_1) {
            exportStar_1(breadcrumb_component_1_1);
        }, function (dropdown_component_1_1) {
            exportStar_1(dropdown_component_1_1);
        }, function (progress_bar_component_1_1) {
            exportStar_1(progress_bar_component_1_1);
        }],
        execute: function () {}
    };
});
System.register("dist/app/shared/shared.module.js", ["@angular/core", "@angular/common", "@angular/router", "@angular/http", "@ng-bootstrap/ng-bootstrap", "@angular/forms", "./pipe/index", "app/aws/aws.service", "./service/index", "@swimlane/ngx-charts", "ng2-nouislider", "./directive/anchor.directive", "@swimlane/ngx-datatable", "./component/index"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, common_1, router_1, http_1, ng_bootstrap_1, forms_1, index_1, aws_service_1, index_2, ngx_charts_1, ng2_nouislider_1, anchor_directive_1, ngx_datatable_1, index_3, AppSharedModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (common_1_1) {
            common_1 = common_1_1;
        }, function (router_1_1) {
            router_1 = router_1_1;
        }, function (http_1_1) {
            http_1 = http_1_1;
        }, function (ng_bootstrap_1_1) {
            ng_bootstrap_1 = ng_bootstrap_1_1;
        }, function (forms_1_1) {
            forms_1 = forms_1_1;
        }, function (index_1_1) {
            index_1 = index_1_1;
        }, function (aws_service_1_1) {
            aws_service_1 = aws_service_1_1;
        }, function (index_2_1) {
            index_2 = index_2_1;
        }, function (ngx_charts_1_1) {
            ngx_charts_1 = ngx_charts_1_1;
        }, function (ng2_nouislider_1_1) {
            ng2_nouislider_1 = ng2_nouislider_1_1;
        }, function (anchor_directive_1_1) {
            anchor_directive_1 = anchor_directive_1_1;
        }, function (ngx_datatable_1_1) {
            ngx_datatable_1 = ngx_datatable_1_1;
        }, function (index_3_1) {
            index_3 = index_3_1;
        }],
        execute: function () {
            AppSharedModule = /** @class */function () {
                function AppSharedModule() {}
                AppSharedModule_1 = AppSharedModule;
                AppSharedModule.forRoot = function () {
                    return {
                        ngModule: AppSharedModule_1,
                        providers: [index_2.DefinitionService, index_2.BreadcrumbService, index_2.GemService, index_2.PaginationService, index_2.LyMetricService, {
                            provide: aws_service_1.AwsService,
                            useClass: aws_service_1.AwsService,
                            deps: [router_1.Router, http_1.Http, index_2.LyMetricService]
                        }, {
                            provide: index_2.UrlService,
                            useClass: index_2.UrlService,
                            deps: [aws_service_1.AwsService]
                        }, {
                            provide: index_2.ApiService,
                            useClass: index_2.ApiService,
                            deps: [aws_service_1.AwsService, http_1.Http, router_1.Router]
                        }, {
                            provide: index_2.AuthGuardService,
                            useClass: index_2.AuthGuardService,
                            deps: [router_1.Router, aws_service_1.AwsService]
                        }, {
                            provide: index_2.PreloadingService,
                            useClass: index_2.PreloadingService,
                            deps: [index_2.DefinitionService]
                        }]
                    };
                };
                AppSharedModule = AppSharedModule_1 = __decorate([core_1.NgModule({
                    imports: [common_1.CommonModule, http_1.JsonpModule, http_1.HttpModule, forms_1.FormsModule, ng_bootstrap_1.NgbModule, forms_1.ReactiveFormsModule, router_1.RouterModule, ngx_charts_1.NgxChartsModule, ng2_nouislider_1.NouisliderModule, ngx_datatable_1.NgxDatatableModule],
                    exports: [common_1.CommonModule, http_1.JsonpModule, http_1.HttpModule, forms_1.FormsModule, ng_bootstrap_1.NgbModule, forms_1.ReactiveFormsModule, router_1.RouterModule, ngx_charts_1.NgxChartsModule, ngx_datatable_1.NgxDatatableModule, index_3.ModalComponent, index_3.DateTimeRangePickerComponent, index_3.LoadingSpinnerComponent, index_1.ModelDebugPipe, index_1.ObjectKeysPipe, index_1.FromEpochPipe, index_1.FilterArrayPipe, index_3.AutoFocusComponent, index_3.FooterComponent, index_3.TooltipComponent, index_3.PieChartComponent, index_3.BarChartComponent, index_3.TagComponent, index_3.InlineEditingComponent, index_3.SwitchButtonComponent, index_3.BreadcrumbComponent, index_3.DropdownComponent, index_3.ProgressBarComponent, index_3.LineChartComponent, ng2_nouislider_1.NouisliderModule, anchor_directive_1.AnchorDirective],
                    declarations: [index_3.ModalComponent, index_3.DateTimeRangePickerComponent, index_3.LoadingSpinnerComponent, index_1.ModelDebugPipe, index_1.ObjectKeysPipe, index_1.FromEpochPipe, index_1.FilterArrayPipe, index_3.AutoFocusComponent, index_3.FooterComponent, index_3.TooltipComponent, index_3.PieChartComponent, index_3.BarChartComponent, index_3.TagComponent, index_3.InlineEditingComponent, index_3.SwitchButtonComponent, index_3.BreadcrumbComponent, index_3.DropdownComponent, index_3.ProgressBarComponent, index_3.LineChartComponent, anchor_directive_1.AnchorDirective]
                })], AppSharedModule);
                return AppSharedModule;
                var AppSharedModule_1;
            }();
            exports_1("AppSharedModule", AppSharedModule);
        }
    };
});
System.register("dist/app/app.module.js", ["@angular/core", "@angular/platform-browser", "@ng-bootstrap/ng-bootstrap", "@angular/common", "app/app.routes", "app/app.component", "app/view/error/error.module", "app/view/authentication/auth.module", "app/view/game/game.module", "app/shared/shared.module", "@angular/platform-browser/animations", "ng2-toastr/ng2-toastr"], function (exports_1, context_1) {
    "use strict";

    var __extends = this && this.__extends || function () {
        var extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
        };
        return function (d, b) {
            extendStatics(d, b);
            function __() {
                this.constructor = d;
            }
            d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
        };
    }();
    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, platform_browser_1, ng_bootstrap_1, common_1, core_2, app_routes_1, app_component_1, error_module_1, auth_module_1, game_module_1, shared_module_1, animations_1, ng2_toastr_1, CustomToastrOptions, AppModule;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
            core_2 = core_1_1;
        }, function (platform_browser_1_1) {
            platform_browser_1 = platform_browser_1_1;
        }, function (ng_bootstrap_1_1) {
            ng_bootstrap_1 = ng_bootstrap_1_1;
        }, function (common_1_1) {
            common_1 = common_1_1;
        }, function (app_routes_1_1) {
            app_routes_1 = app_routes_1_1;
        }, function (app_component_1_1) {
            app_component_1 = app_component_1_1;
        }, function (error_module_1_1) {
            error_module_1 = error_module_1_1;
        }, function (auth_module_1_1) {
            auth_module_1 = auth_module_1_1;
        }, function (game_module_1_1) {
            game_module_1 = game_module_1_1;
        }, function (shared_module_1_1) {
            shared_module_1 = shared_module_1_1;
        }, function (animations_1_1) {
            animations_1 = animations_1_1;
        }, function (ng2_toastr_1_1) {
            ng2_toastr_1 = ng2_toastr_1_1;
        }],
        execute: function () {
            CustomToastrOptions = /** @class */function (_super) {
                __extends(CustomToastrOptions, _super);
                function CustomToastrOptions() {
                    var _this = _super !== null && _super.apply(this, arguments) || this;
                    _this.animate = "flyRight";
                    _this.showCloseButton = true;
                    _this.maxShown = 3;
                    _this.positionClass = 'toast-bottom-right';
                    _this.messageClass = 'cgp-toastr-message';
                    _this.titleClass = 'cgp-toastr-title';
                    return _this;
                }
                return CustomToastrOptions;
            }(ng2_toastr_1.ToastOptions);
            exports_1("CustomToastrOptions", CustomToastrOptions);
            AppModule = /** @class */function () {
                function AppModule() {}
                AppModule = __decorate([core_1.NgModule({
                    imports: [platform_browser_1.BrowserModule, animations_1.BrowserAnimationsModule, ng_bootstrap_1.NgbModule.forRoot(), shared_module_1.AppSharedModule.forRoot(), ng2_toastr_1.ToastModule.forRoot(), error_module_1.ErrorModule, auth_module_1.AuthModule, game_module_1.GameModule, app_routes_1.AppRoutingModule],
                    declarations: [app_component_1.AppComponent],
                    providers: [{ provide: common_1.APP_BASE_HREF, useValue: '/' }, { provide: core_2.LOCALE_ID, useValue: "en-US" }, { provide: ng2_toastr_1.ToastOptions, useClass: CustomToastrOptions }],
                    bootstrap: [app_component_1.AppComponent]
                })], AppModule);
                return AppModule;
            }();
            exports_1("AppModule", AppModule);
        }
    };
});
System.register("dist/app/shared/class/environment.class.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var metricWhiteListedCloudGem, metricWhiteListedFeature, metricAdminIndex, isTest, isProd;
    return {
        setters: [],
        execute: function () {
            exports_1("metricWhiteListedCloudGem", metricWhiteListedCloudGem = []);
            exports_1("metricWhiteListedFeature", metricWhiteListedFeature = ['Cloud Gems', 'Support', 'Analytics', 'Admin', 'User Administration']);
            exports_1("metricAdminIndex", metricAdminIndex = 3);
            exports_1("isTest", isTest = false);
            exports_1("isProd", isProd = true);
        }
    };
});
System.register("dist/app/main.js", ["reflect-metadata", "zone.js", "@angular/platform-browser-dynamic", "./app.module", "@angular/core", "app/shared/class/environment.class"], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    var platform_browser_dynamic_1, app_module_1, core_1, environment, platform;
    return {
        setters: [function (_1) {}, function (_2) {}, function (platform_browser_dynamic_1_1) {
            platform_browser_dynamic_1 = platform_browser_dynamic_1_1;
        }, function (app_module_1_1) {
            app_module_1 = app_module_1_1;
        }, function (core_1_1) {
            core_1 = core_1_1;
        }, function (environment_1) {
            environment = environment_1;
        }],
        execute: function () {
            if (environment.isProd) {
                core_1.enableProdMode();
            }
            platform = platform_browser_dynamic_1.platformBrowserDynamic();
            platform.bootstrapModule(app_module_1.AppModule);
        }
    };
});
System.register("dist/app/shared/pipe/safe-html.pipe.js", ["@angular/core", "@angular/platform-browser"], function (exports_1, context_1) {
    "use strict";

    var __decorate = this && this.__decorate || function (decorators, target, key, desc) {
        var c = arguments.length,
            r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc,
            d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = this && this.__metadata || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var __moduleName = context_1 && context_1.id;
    var core_1, platform_browser_1, SafeHtmlPipe;
    return {
        setters: [function (core_1_1) {
            core_1 = core_1_1;
        }, function (platform_browser_1_1) {
            platform_browser_1 = platform_browser_1_1;
        }],
        execute: function () {
            SafeHtmlPipe = /** @class */function () {
                function SafeHtmlPipe(sanitized) {
                    this.sanitized = sanitized;
                }
                SafeHtmlPipe.prototype.transform = function (value) {
                    return this.sanitized.bypassSecurityTrustHtml(value);
                };
                SafeHtmlPipe = __decorate([core_1.Pipe({ name: 'safeHtml' }), __metadata("design:paramtypes", [platform_browser_1.DomSanitizer])], SafeHtmlPipe);
                return SafeHtmlPipe;
            }();
            exports_1("SafeHtmlPipe", SafeHtmlPipe);
        }
    };
});
System.register("dist/app/view/game/module/shared/class/facetable.interface.js", [], function (exports_1, context_1) {
    "use strict";

    var __moduleName = context_1 && context_1.id;
    return {
        setters: [],
        execute: function () {}
    };
});