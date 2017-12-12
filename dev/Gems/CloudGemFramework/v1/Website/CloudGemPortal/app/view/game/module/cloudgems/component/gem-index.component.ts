import { Component, ChangeDetectorRef, OnInit, ComponentRef, ReflectiveInjector, Input, OnChanges, OnDestroy, NgZone } from '@angular/core';
import { Http } from '@angular/http';
import { Gemifiable, AbstractCloudGemLoader, AwsCloudGemLoader, LocalCloudGemLoader } from '../class/index';
import { AwsService } from 'app/aws/aws.service';
import { InnerRouterService } from "../../shared/service/index";
import { Observable } from 'rxjs/Observable'
import { DefinitionService, LyMetricService, GemService } from 'app/shared/service/index';
export class ROUTES {
    public static CLOUD_GEMS = "cloudgems";
}

@Component({
    selector: 'game-gem-index',
    templateUrl: 'app/view/game/module/cloudgems/component/gem-index.component.html',
    styleUrls: ['app/view/game/module/cloudgems/component/gem-index.component.css']
})
export class GemIndexComponent implements OnInit, OnDestroy{
    private model: any = {               
        currentGem: undefined,
        forceRefresh: false,
        hasInitialized: false,
        isLoading: false,
        cloudgems: [],
        numberToLoad: 0,
        numberLoaded: 0
    }        
    private subscriptions = Array<any>();
    private _changeDetectionTimer: any;
    private _isDirty = true;
    private _subscribedDeployments: string[];
    private _loader: AbstractCloudGemLoader;    
            
    constructor(              
        private aws: AwsService,
        private router: InnerRouterService,
        private zone: NgZone, 
        private definition: DefinitionService,
        private http: Http,
        private lymetrics: LyMetricService,
        private gems: GemService  
    ) {   }

    ngOnInit() {        
        this._loader = this.definition.isProd ?
                this._loader = new AwsCloudGemLoader(this.definition, this.aws, this.http)
                : new LocalCloudGemLoader(this.definition, this.http);
        
        this.handleLoadedGem = this.handleLoadedGem.bind(this);
        this._subscribedDeployments = [];
        this.model.isLoading = true
        this.gems.observable.subscribe(gem => {
            this.model.numberLoaded++            
            this.setDirty() 
        })
        this.subscriptions.push(this.aws.context.project.activeDeployment.subscribe(activeDeployment => {            
            if (!activeDeployment || this._subscribedDeployments.indexOf(activeDeployment.settings.name) > -1) {            
                return;
            }

            this._subscribedDeployments.push(activeDeployment.settings.name);                        
            activeDeployment.resourceGroup.subscribe(rgs => {
                if (rgs === undefined)
                    return;

                this.model.numberLoaded = 0
                this.model.numberToLoad = rgs.length                
                this.zone.run(() => {
                    this.model.isLoading = rgs.length > 0 ? true : false                    
                    this.model.cloudgems = [];  
                })           
                
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
                
                this.zone.run(() => {                    
                    this.model.hasInitialized = true;
                })                                
            })
            
        }));
        
        this.subscriptions.push(this.router.onChange.subscribe(route => {
            if (route == ROUTES.CLOUD_GEMS) {
                this.back();
            }
        }));
        this.subscriptions.push(this.aws.context.project.activeDeployment.subscribe(d => {
            this.back();
        }));
    }

    private handleLoadedGem(gem: Gemifiable): void {
        if (gem === undefined) {
            this.model.numberLoaded++
            this.setDirty()
            return 
        }
        this.zone.run(() => {
            this.model.cloudgems.push(gem)
        })
    }

    load(gem: Gemifiable) {                  
        this.gems.currentGem = gem;
    }

    ngOnDestroy() {                
        for (var i = 0; i < this.subscriptions.length; i++) {
            this.subscriptions[i].unsubscribe();
        }
    }

    back() {
        this.gems.currentGem = null;
    }

    setDirty(): void {
        this.zone.run(() => {            
            if (this.model.numberLoaded >= this.model.numberToLoad)
                this.model.isLoading = false;
        })
    }
   
}
