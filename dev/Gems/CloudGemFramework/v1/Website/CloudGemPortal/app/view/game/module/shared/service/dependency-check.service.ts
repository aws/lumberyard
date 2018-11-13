import { Injectable } from '@angular/core';
import {
    CanActivate, Router,
    ActivatedRouteSnapshot,
    RouterStateSnapshot,
    CanActivateChild,
    CanLoad, Route
} from '@angular/router';
import { AwsService } from 'app/aws/aws.service'
import { FeatureDefinitions, Feature } from 'app/view/game/module/shared/class/feature.class'
import { Subscription } from 'rxjs/Subscription';
import { Verify } from '../class/dependency-verify.class'
import { Observable } from 'rxjs/Observable';
import { Subject } from 'rxjs/Subject';

@Injectable()
export class DependencyService {
    private _deploymentSub: Subscription
    private _resourceGroupSub: Subscription
    private _availableFeatures: Map<string, Feature[]>;
    featureMapSubscription = new Subject<Map<string, Feature[]>>();

    constructor(private aws: AwsService) {
        this._availableFeatures = new Map<string, Feature[]>()
    }

    get availableFeatures() {
        return this._availableFeatures;
    }
 

    /*
    *   Used to avoid circular dependencies between Components in the definitions and those components that use this service
    */
    subscribeToDeploymentChanges(def: FeatureDefinitions): void {
        if (this._deploymentSub) {
            this._deploymentSub.unsubscribe();
        }
        this._deploymentSub = this.aws.context.project.activeDeploymentSubscription.subscribe(deployment => {
            if (deployment) {
                this.loadResourceGroups(def, deployment)
            }
        })
        if (this.aws.context.project.activeDeployment != null) {
            this.loadResourceGroups(def, this.aws.context.project.activeDeployment)
        }
    }

    loadResourceGroups = (definitions, deployment) => {
        if (this._resourceGroupSub) {
            this._resourceGroupSub.unsubscribe();
        }
        this._resourceGroupSub = deployment.resourceGroup.subscribe(rgs => {
            this._availableFeatures = new Map<string, Feature[]>()
            for (let idx in definitions.defined) {
                let feature = definitions.defined[idx]
                if (Verify.dependency(feature.component.name, this.aws, definitions)) {
                    let features = []
                    if (feature.parent in this._availableFeatures) {
                        features = this._availableFeatures[feature.parent]
                    }
                    else if (feature.parent != undefined) {
                        this._availableFeatures[feature.parent] = features
                    }
                    features.push(feature)                    
                }
            }
            this.featureMapSubscription.next(this.availableFeatures);
        })
    }

    ngOnDestroy() {
        if (this._deploymentSub) {
            this._deploymentSub.unsubscribe();
        }
        if (this._resourceGroupSub) {
            this._resourceGroupSub.unsubscribe();
        }
    }


}


