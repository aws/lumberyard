import { Component, ChangeDetectorRef, OnInit, ComponentRef, ReflectiveInjector, Input, OnChanges, OnDestroy, NgZone } from '@angular/core';
import { Http } from '@angular/http';
import { Gemifiable, AbstractCloudGemLoader, AwsCloudGemLoader, LocalCloudGemLoader } from '../class/index';
import { AwsService } from 'app/aws/aws.service';
import { InnerRouterService } from "../../shared/service/index";
import { Observable } from 'rxjs/Observable'
import { DefinitionService, LyMetricService, GemService, BreadcrumbService, PaginationService } from 'app/shared/service/index';
import {Router, ActivatedRoute, ParamMap } from '@angular/router';
export class ROUTES {
    public static CLOUD_GEMS = "cloudgems";
}

@Component({
    selector: 'game-gem-index',
    templateUrl: 'app/view/game/module/cloudgems/component/gem-index.component.html',
    styleUrls: ['app/view/game/module/cloudgems/component/gem-index.component.css']
})
export class GemIndexComponent implements OnInit, OnDestroy{
    private subscriptions = Array<any>();
    private _changeDetectionTimer: any;
    private _isDirty = true;
    private _subscribedDeployments: string[];
    private _loader: AbstractCloudGemLoader;
    private _gemIdRouteParam: string;
    private forceRefresh: boolean = false;

    constructor(
        private aws: AwsService,
        private zone: NgZone,
        private definition: DefinitionService,
        private http: Http,
        private lymetrics: LyMetricService,
        private gems: GemService ,
        private route: ActivatedRoute,
        private router: Router,
        private breadcrumb: BreadcrumbService
    ) { }

    ngOnInit() {
        this.handleLoadedGem = this.handleLoadedGem.bind(this);
        this._subscribedDeployments = [];

        // Get route parameters and route to the current gem if there is one
        this.subscriptions.push(this.route.params.subscribe(params => {
            this._gemIdRouteParam = params['id'];
            this.gems.currentGem = this.gems.getGem(this._gemIdRouteParam);
            if (this.gems.currentGem) {
                this.breadcrumb.removeAllBreadcrumbs();

                // Add a breadcrumb for getting back to the Cloud Gems page
                this.breadcrumb.addBreadcrumb("Cloud Gems", () => {
                    this.router.navigateByUrl('/game/cloudgems');
                })

                // Add a breadcrumb for the current gem
                this.breadcrumb.addBreadcrumb(this.gems.currentGem.displayName, () => {
                    // toggle a force refresh for the gem to reload the main gem homepage
                    this.forceRefresh = !this.forceRefresh;
                    this.breadcrumb.removeLastBreadcrumb();
                });
            }
        }));

        // every time the deployment is changed reload the gems for the current deployment
        this.subscriptions.push(this.aws.context.project.activeDeploymentSubscription.subscribe(activeDeployment => {
            this.loadGems(activeDeployment);
        }));

        // If there is already an active deployment use that and initialize the gems in the gem index
        if (this.aws.context.project.activeDeployment) {
            this.loadGems(this.aws.context.project.activeDeployment);
        }
    }

    private loadGems(activeDeployment) {
        this._loader = this.definition.isProd ? this._loader = new AwsCloudGemLoader(this.definition, this.aws, this.http) : new LocalCloudGemLoader(this.definition, this.http);

        if (!activeDeployment || !activeDeployment.settings || this._subscribedDeployments.indexOf(activeDeployment.settings.name) > -1) {
            return;
        }

        this._subscribedDeployments.push(activeDeployment.settings.name);
        this.subscriptions.push(activeDeployment.resourceGroup.subscribe(rgs => {
            this.gems.isLoading = false;
            if (rgs === undefined)
                return;

            for (var i = 0; i < rgs.length; i++) {
                this._loader.load(activeDeployment, rgs[i], this.handleLoadedGem);
            }
            if (rgs.length > 0) {
                let names = rgs.map(gem => gem.logicalResourceId)
                this.lymetrics.recordEvent('CloudGemsLoaded', {
                    'CloudGemsInDeployment': names.join(','),
                }, {
                        'NumberOfCloudGemsInDeployment': names.length
                    })
            }
        }));

        // Subscribe to gems being added to the gem service
        this.subscriptions.push(this.gems.currentGemSubscription.subscribe((gem) => {
            this.gems.isLoading = false;
        }))
    }


    private handleLoadedGem(gem: Gemifiable): void {
        this.gems.isLoading = false;
        if (gem === undefined) {
            return;

        }
        this.zone.run(() => {
            this.gems.addGem(gem);
        })
    }

    ngOnDestroy() {
        for (var i = 0; i < this.subscriptions.length; i++) {
            this.subscriptions[i].unsubscribe();
        }
    }
}
