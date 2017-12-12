import { Injectable } from '@angular/core';
import { Http } from '@angular/http';
import { Router } from "@angular/router";
import { AwsService } from 'app/aws/aws.service';
import { Gemifiable, AbstractCloudGemLoader, AwsCloudGemLoader, LocalCloudGemLoader } from '../class/index';
import { DefinitionService } from 'app/shared/service/index';
import { CacheHandlerService } from 'app/view/game/module/cloudgems/service/cachehandler.service';
import { Subscription } from 'rxjs/Subscription';
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Observable } from 'rxjs/Observable';
import { DomSanitizer } from '@angular/platform-browser';

@Injectable()
export class CloudGemService {
    public dynamicGems: Gemifiable[];
    private _isLoadingGems: BehaviorSubject<boolean>;
    private _hasInitialized: BehaviorSubject<boolean>;
    private _gemDict: any;
    private _onLoadSub: any;
    private _isTemplateLoadedSub: any;
    private _loader: AbstractCloudGemLoader;
    protected subscribedDeployments: string[];

    get isLoading(): Observable<boolean>{
        return this._isLoadingGems.asObservable();
    }

    get hasInitialized(): Observable<boolean> {
        return this._hasInitialized.asObservable();
    }

    constructor(
        private http: Http,
        private router: Router,
        private aws: AwsService,
        private definition: DefinitionService,
        private cache: CacheHandlerService,
        private sanitizer: DomSanitizer) {
        this.dynamicGems = [];
        this._gemDict = {};
        this._isLoadingGems = new BehaviorSubject<boolean>(false);
        this._hasInitialized = new BehaviorSubject<boolean>(false);

        this.handleLoadedGem = this.handleLoadedGem.bind(this);

        if (definition.isProd)
            this._loader = new AwsCloudGemLoader(http, aws);
        else
            this._loader = new LocalCloudGemLoader(http);

        //Add your providers here
        let assignedProviders = {
            http: this.http,
            router: this.router,
            aws: this.aws,
            definition: this.definition,
            cache: this.cache,
            sanitizer: this.sanitizer
        }

        this.subscribedDeployments = [];

        aws.context.project.activeDeployment.subscribe(activeDeployment => {
            //have we already subscribed to this deployment stack event
            if (!activeDeployment || this.subscribedDeployments.indexOf(activeDeployment.settings.name) > -1) {                
                return;
            }

            this.unregisterAllGems();
            this._isLoadingGems.next(true);            
            this._hasInitialized.next(false);
            this.subscribedDeployments.push(activeDeployment.settings.name);

            activeDeployment.resourceGroup.subscribe(rgs => {
                for (var i = 0; i < rgs.length; i++) {
                    this._loader.load(assignedProviders, activeDeployment, rgs[i], this.handleLoadedGem);
                }
                if (rgs.length == 0)
                    this._isLoadingGems.next(false);
                this._hasInitialized.next(true);
            })
        });

    }

    public destroy(): void {
        console.log("destroy")
    }

    private handleLoadedGem(gem: Gemifiable): void {
        if (gem === undefined) {
            this._isLoadingGems.next(false);            
            return
        }
        if (!gem || this._gemDict[gem.identifier] !== undefined) {
            return;
        }
        this.register(gem);
        this._isLoadingGems.next(false);
    }

    private register(dynamicGem: Gemifiable): void {
        if (!dynamicGem)
            return;
        this._gemDict[dynamicGem.identifier] = dynamicGem;
        this.dynamicGems.push(dynamicGem);
    }

    /* Unregisters all gems by resetting the gem dictionary and the behavior subject subscription */
    private unregisterAllGems() {
        this._gemDict = {};
        this.dynamicGems = [];
    }
}