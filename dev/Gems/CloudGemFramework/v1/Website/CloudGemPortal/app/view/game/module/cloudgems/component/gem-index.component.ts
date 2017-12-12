import { Component, OnInit, ComponentRef, ReflectiveInjector, Input } from '@angular/core';
import { Http, Response, RequestOptionsArgs } from '@angular/http';
import { CloudGemService } from '../service/cloud-gem.service';
import { Gemifiable } from '../class/index';
import { AwsService } from 'app/aws/aws.service';
import { InnerRouterService } from "../../shared/service/index";
import { Observable } from 'rxjs/Observable'

const defaultGemHeading = "Cloud Gems";

export class ROUTES {
    public static CLOUD_GEMS = "cloudgems";
}

@Component({
    selector: 'game-gem-index',
    templateUrl: 'app/view/game/module/cloudgems/component/gem-index.component.html',
    styleUrls: ['app/view/game/module/cloudgems/component/gem-index.component.css']
})
export class GemIndexComponent {
    private _isInitialized = false;
    public isTackableView = true;
    public gemsHeading = defaultGemHeading;
    public currentGem: Gemifiable
    public currentGemHeadingImageUrl = "";
    public forceRefresh: boolean = false;

    public model: {
        isLoading: boolean,
        hasGemServiceInitialized: boolean
    }

    private subActiveDeployment: any = null;
    private subRouter: any = null;
    private subscriptions = Array<any>();

    constructor(
        private cloudGemService: CloudGemService,
        private http: Http,
        private aws: AwsService,
        private router: InnerRouterService
    ) {

    }

    ngOnInit() {
        this.subscriptions.push(this.router.onChange.subscribe(route => {
            if (route == ROUTES.CLOUD_GEMS) {
                this.back();
            }
        }));
        this.subscriptions.push(this.aws.context.project.activeDeployment.subscribe(d => {
            this.back();
        }));
        this.subscriptions.push(this.cloudGemService.hasInitialized.subscribe(b => {
            this.model.hasGemServiceInitialized = b
        }));
        this.subscriptions.push(this.cloudGemService.isLoading.subscribe(b => {
            this.model.isLoading = b
        }));
    }

    ngOnDestroy() {
        for (var i = 0; i < this.subscriptions.length; i++) {
            this.subscriptions[i].unsubscribe();
        }
    }


    public load(gem: Gemifiable) {
        this.isTackableView = false;
        this.currentGem = gem;
        this.gemsHeading = gem.tackable.displayName;
        this.currentGemHeadingImageUrl = gem.tackable.iconSrc;
    }

    public back() {
        this.isTackableView = true;
        this.gemsHeading = defaultGemHeading;
        this.currentGemHeadingImageUrl = "";
        this.currentGem = null;
    }
}
