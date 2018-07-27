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
import { Verify } from '../class/dependency-verify.class'


@Injectable()
export class DependencyGuard implements CanActivate {
    private _featureDefinitions: FeatureDefinitions

    constructor(private aws: AwsService) {
        this._featureDefinitions = new FeatureDefinitions()
    }

    canActivate(activated: ActivatedRouteSnapshot, state: RouterStateSnapshot): boolean {
        let obj = <any>activated.component
        return Verify.dependency(obj.name, this.aws, this._featureDefinitions)
    }
}