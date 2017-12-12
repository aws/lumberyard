import { Injectable } from '@angular/core';
import {
    CanActivate, Router,
    ActivatedRouteSnapshot,
    RouterStateSnapshot,
    CanActivateChild,    
    CanLoad, Route    
} from '@angular/router';
import { AwsService } from 'app/aws/aws.service'
import { AwsProject } from 'app/aws/project.class'

@Injectable()
export class AuthGuardService implements CanActivate, CanActivateChild, CanLoad {

    constructor(private router: Router, private aws: AwsService) {
    }

    canActivate(route: ActivatedRouteSnapshot, state: RouterStateSnapshot): boolean {                
        return this.aws.context.authentication.isLoggedIn;
    }

    canActivateChild(route: ActivatedRouteSnapshot, state: RouterStateSnapshot): boolean {        
        return this.canActivate(route, state);
    }

    canLoad(route: Route): boolean {
        let url = `/${route.path}`;        
        return this.checkLogin(url);
    }

    checkLogin(url: string): boolean {        
        return this.aws.context.authentication.isLoggedIn;
    }
}
