import { Injectable } from '@angular/core';
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Observable } from 'rxjs/Observable'
import { AwsContext } from './context.class'
import { Authentication, AuthStateActionContext, EnumAuthState } from './authentication/authentication.class'
import { UserManagement } from './user/user-management.class'
import { AwsProject } from './project.class'
import { ResourceGroup, ResourceOutput } from './resource-group.class'
import { DefinitionService } from 'app/shared/service/index'
import { Router } from '@angular/router';
import { Http } from '@angular/http';
import { LyMetricService } from 'app/shared/service/index'

declare var AWS

export interface ProjectDeploymentSettings {
    DeploymentAccessStackId?: string;
    DeploymentStackId?: string;
    ConfigurationBucket?: string;
    ConfigurationKey?: string;
}

export interface DeploymentSettings extends ProjectDeploymentSettings {
    name: string;
}


export interface ProjectSettings {
    DefaultDeployment?: string;
    ReleaseDeployment?: string;
    deployment: ProjectDeploymentSettings;
}

export interface Deployment {
    settings: DeploymentSettings;
    resourceGroup: Observable<ResourceGroup[]>;
    update(): void;
}

export interface Targetable {
    logicalResourceId: string;
    physicalResourceId: string;
    resourceType: string;
    resourceStatus: string;
    stackId: string;
    stackName: string;
}

export interface Outputable {
    outputs: Map<string, ResourceOutput>;
}


@Injectable()
export class AwsService {
    private _context: AwsContext;
    private _isInitialized: boolean;

    public static getStackNameFromArn(arn: string) {
        if (arn)
            return arn.split('/')[1];
        return null;

    }
    public static getRegionFromArn(arn: string) {
        if (arn)
            return arn.split(':')[3];
        return null;
    }

    public static getAccountIdFromArn(arn: string) {
        if (arn)
            return arn.split(':')[4];
        return null;
    }

    get isInitialized(): boolean {
        return this._isInitialized;
    }

    get context(): AwsContext {
        return this._context;
    }

    constructor(
        private router: Router,
        private http: Http, 
        private metric: LyMetricService
    ) {
        this._isInitialized = false;
        this._context = new AwsContext();
        this._context.authentication = new Authentication(this.context, this.metric);
        this._context.userManagement = new UserManagement(this.context, this.router, this.metric);
    }

    public init(userPoolId: string, clientId: string, identityPoolId: string, bucketid: string, region: string, definitions: DefinitionService): void {
        //init the aws context                
        this._isInitialized = this.isValidBootstrap(userPoolId, clientId, identityPoolId, bucketid, region)
        if (!this._isInitialized)
            return;       

        this._context.init(userPoolId, clientId, identityPoolId, bucketid, region);
        
        this.initializeSubscriptions();

        this.context.authentication.change.subscribe(context => {
            if (context.state === EnumAuthState.LOGGED_OUT) {
                this.initializeSubscriptions();
                this.router.navigate(['/login']);
            }
        });

    }

    public parseAPIId(serviceurl: string) {
        if (!serviceurl)
            return undefined
        let schema_parts = serviceurl.split('//');
        let url_parts = schema_parts[1].split('.');
        return url_parts[0];

    }

    private initializeSubscriptions(): void {
        let initializationSubscription = this.context.authentication.change.subscribe(context => {
            if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATED) {
                initializationSubscription.unsubscribe();
                this.context.initializeServices();
                this.context.project = new AwsProject(this.context, this.http);             
                this.router.navigate(['game/cloudgems']);
            }
        });
    }

    private isValidBootstrap(userPoolId: string, clientId: string, identityPoolId: string, bucketid: string, region: string): boolean {
        return userPoolId && clientId && identityPoolId && bucketid && region ? true : false
    }
}

