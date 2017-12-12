import { Component, ViewEncapsulation } from '@angular/core';
import { DefinitionService, UrlService } from 'app/shared/service/index';
import { AwsService } from 'app/aws/aws.service';

declare const toastr: any;
declare const cgpBootstrap: any;

@Component({
    selector: 'cc-app',
    template: '<template ngbModalContainer></template><router-outlet></router-outlet>',
    styleUrls: ['./styles/bootstrap/bootstrap.css', './styles/main.css'],
    encapsulation: ViewEncapsulation.None
})
export class AppComponent {
    constructor(aws: AwsService, ds: DefinitionService, urlService: UrlService) {                
        urlService.parseLocationHref(location.href)
        if (ds.isProd)
            window.onbeforeunload = function (e) {
                var e = e || window.event;
                var msg = "Do you really want to leave this page?"

                // For IE and Firefox
                if (e) {
                    e.returnValue = msg;
                }

                // For Safari / chrome
                return msg;
            };
                
        aws.init(cgpBootstrap.userPoolId, cgpBootstrap.clientId, cgpBootstrap.identityPoolId, cgpBootstrap.projectConfigBucketId, cgpBootstrap.region)

        toastr.options = {
            "closeButton": true,
            "debug": false,
            "newestOnTop": true,
            "progressBar": true,
            "positionClass": "toast-top-center",
            "preventDuplicates": false,
            "onclick": null,
            "showDuration": "300",
            "hideDuration": "1000",
            "timeOut": "6000",
            "extendedTimeOut": "1000",
            "showEasing": "swing",
            "hideEasing": "linear",
            "showMethod": "fadeIn",
            "hideMethod": "fadeOut"
        }
    }
}
