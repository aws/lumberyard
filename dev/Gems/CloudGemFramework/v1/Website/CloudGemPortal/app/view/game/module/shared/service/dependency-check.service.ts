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
    private availableFeatures: Map<string, Feature[]>;
    featureMapSubscription = new Subject<Map<string, Feature[]>>();

    constructor(private aws: AwsService) {

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
                if (this._resourceGroupSub) {
                    this._resourceGroupSub.unsubscribe();
                }
                this._resourceGroupSub = deployment.resourceGroup.subscribe(rgs => {
                    this.availableFeatures = new Map<string, Feature[]>()
                    for (let idx in def.defined) {
                        let feature = def.defined[idx]
                        if (Verify.dependency(feature.component.name, this.aws, def)) {
                            let features = []
                            if (feature.parent in this.availableFeatures) {
                                features = this.availableFeatures[feature.parent]
                            }
                            else if (feature.parent != undefined) {
                                this.availableFeatures[feature.parent] = features
                            }
                            features.push(feature)
                            this.featureMapSubscription.next(this.availableFeatures);
                        }
                    }
                })
            }
        })
    }
}


