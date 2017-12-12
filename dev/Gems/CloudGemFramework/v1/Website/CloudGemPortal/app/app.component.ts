import { Component, ViewEncapsulation, ViewContainerRef } from '@angular/core';
import { DefinitionService, UrlService } from 'app/shared/service/index';
import { AwsService } from 'app/aws/aws.service';
import { ToastsManager } from 'ng2-toastr';

declare const bootstrap: any;

@Component({
    selector: 'cc-app',
    template: '<router-outlet></router-outlet>',
    styleUrls: ['./styles/bootstrap/bootstrap.css', './styles/main.css'],
    encapsulation: ViewEncapsulation.None
})
export class AppComponent {
    constructor(ds: DefinitionService, aws: AwsService, urlService: UrlService, private toastr: ToastsManager, vcr: ViewContainerRef ) {                
        toastr.setRootViewContainerRef(vcr);
        urlService.parseLocationHref(location.href)
        if (ds.isTest) {
            localStorage.clear()
        }

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
                
        aws.init(bootstrap.userPoolId, bootstrap.clientId, bootstrap.identityPoolId, bootstrap.projectConfigBucketId, bootstrap.region, ds)

    }
}
