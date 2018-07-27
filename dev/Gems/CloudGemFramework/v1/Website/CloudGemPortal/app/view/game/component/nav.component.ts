import { Component, OnInit, ViewContainerRef } from '@angular/core';
import { AwsService, Deployment } from 'app/aws/aws.service';
import { AwsDeployment } from 'app/aws/deployment.class'
import { Clipboard } from 'ts-clipboard';
import { UrlService, LyMetricService, GemService } from "app/shared/service/index"
import { Authentication } from "app/aws/authentication/authentication.class";
import { ToastsManager } from 'ng2-toastr';
import { Router } from '@angular/router';
import * as environment from 'app/shared/class/index';

@Component({
    selector: 'cgp-nav',
    templateUrl: "app/view/game/component/nav.component.html",
    styleUrls: ["app/view/game/component/nav.component.css"]
})
export class NavComponent {
    projectName = "Cloud Canvas Project";
    nav = {
        "deploymentName": "",
        "projectName": this.aws.context.configBucket,
        "deployments": [],
        "username": ""
    }
    public isCollapsed: boolean = true;

    public constructor(private aws: AwsService, private urlService: UrlService, private lymetrics: LyMetricService,
        private toastr: ToastsManager, vcr: ViewContainerRef, private router: Router, private gems: GemService) {
        this.toastr.setRootViewContainerRef(vcr);        
        // Listen for the active deployment and the name when the nav is loaded.
        if (this.aws.context.project.activeDeployment) {
            this.setContext();
        } else {
            this.aws.context.project.activeDeploymentSubscription.subscribe(activeDeployment => {
                if (activeDeployment) {
                    let url = location.search.indexOf('target') === -1 ? '/game/cloudgems' : '/game/cloudgems' + location.search;
                    this.router.navigateByUrl(url).then(() => {
                        this.gems.clearCurrentGems();
                        this.nav.deploymentName = activeDeployment.settings.name;
                    });
                    lymetrics.recordEvent('DeploymentChanged', {}, {
                        'NumberOfDeployments': this.aws.context.project.deployments.length
                    })
                } else {
                    this.nav.deploymentName = "No Deployments";
                }
            });

            this.aws.context.project.settings.subscribe(settings => {
                this.setContext();
                if (settings)
                    lymetrics.recordEvent('ProjectSettingsInitialized', {}, {
                        'NumberOfDeployments': Object.keys(settings.deployment).length
                    })
            });
            this.lymetrics.recordEvent('FeatureOpened', {
                "Name": 'cloudgems'
            })
        }
    }
        
    public onDeploymentClick(deployment: Deployment) {
        this.gems.isLoading = true;
        this.aws.context.project.activeDeployment = <AwsDeployment>deployment;
        this.nav.deploymentName = deployment.settings.name;
    }

    public signOut(): void {
        this.aws.context.authentication.logout();
    }

    public onShare() {
        let params = this.urlService.querystring ? this.urlService.querystring.split("&") : [];
        let expiration: string = String(new Date().getTime());
        for (var i = 0; i < params.length; i++) {
            var param = params[i];
            var paramParts = param.split("=");
            if (paramParts[0] == "Expires") {
                expiration = paramParts[1]
                break;
            }
        }

        if (expiration != null && Number(expiration) != NaN) {
            let expirationDate = new Date(0);
            expirationDate.setUTCSeconds(Number(expiration))
            let now = new Date()
            let expirationTimeInMilli = expirationDate.getTime() - now.getTime();

            if (expirationTimeInMilli < 0) {
                this.lymetrics.recordEvent('ProjectUrlSharedFailed', null, { 'Expired': expirationTimeInMilli })
                this.toastr.error("The url has expired and is no longer shareable.  Please use the Lumberyard Editor to generate a new URL. AWS -> Open Cloud Gem Portal", "Copy To Clipboard");
            } else {
                let minutes = Math.round(((expirationTimeInMilli / 1000) / 60));
                Clipboard.copy(this.urlService.baseUrl)
                this.toastr.success("The shareable Cloud Gem Portal URL has been copied to your clipboard.", "Copy To Clipboard");
                this.lymetrics.recordEvent('ProjectUrlSharedSuccess', null, {
                    'UrlExpirationInMinutes': minutes
                })
            }
        } else {
            this.lymetrics.recordEvent('ProjectUrlSharedFailed')
            this.toastr.error("The URL is not properly set. Please use the Lumberyard Editor to generate a new URL. AWS -> Open Cloud Gem Portal", "Copy To Clipboard");
        }

    }

    private setContext = () => {
        var username = this.aws.context.authentication.user.username;
        this.nav = {
            "deploymentName": this.aws.context.project.activeDeployment ? this.aws.context.project.activeDeployment.settings.name : "No Deployments",
            "projectName": this.aws.context.name,
            "deployments": this.aws.context.project.deployments,
            "username": username.charAt(0).toUpperCase() + username.slice(1)
        }
    }

}
