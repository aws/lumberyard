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
        private router: Router
    ) {
        this._isInitialized = false;
        this._context = new AwsContext();
        this._context.authentication = new Authentication(this.context);
        this._context.userManagement = new UserManagement(this.context, this.router);
        this.context.authentication.change.subscribe(context => {
            if (context.state === EnumAuthState.USER_CREDENTIAL_UPDATED) {
                //TODO:  wrap the client to check the access token prior to calling the client.
                this.context.s3 = this.context.awsClient("S3", "2006-03-01");
                this.context.cloudFormation = this.context.awsClient("CloudFormation", "2010-05-15");
                this.context.cognitoIdentityService = this.context.awsClient("CognitoIdentityServiceProvider", "2016-04-18");
                this.context.cognitoIdentity = this.context.awsClient("CognitoIdentity", "2014-06-30");
                
                this.router.navigate(['/game/cloudgems']);
            } else if (context.state === EnumAuthState.LOGGED_OUT) {
                this.router.navigate(['/login']);
            }
        });
        this._context.project = new AwsProject(this.context);
    }

    public init(userPoolId: string, clientId: string, identityPoolId: string, bucketid: string, region: string): void {
        //init the aws context                
        this._isInitialized = true;
        this._context.init(userPoolId, clientId, identityPoolId, bucketid, region);
        this.context.project.init(bucketid);
    }



}

