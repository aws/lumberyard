import { Injectable } from '@angular/core';
import { Http, Response } from '@angular/http';
import { Observable } from 'rxjs/Observable'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { AwsService, Deployment } from 'app/aws/aws.service';
import { ResourceGroup, ResourceOutputEnum, ResourceOutput } from 'app/aws/resource-group.class'
import { Gemifiable } from './gem-interfaces';
import { Schema } from 'app/aws/schema.class'


export interface CloudGemMeta {
    name: string;
    bucketId: string;
    key: string;
    basePath: string;
    url: string;
    deploymentId: string;
    cloudFormationOutputMap: Map<string, ResourceOutput>;
}

export abstract class Loader {
    abstract load(providers: any, activeDeployment: Deployment, resourceGroups: ResourceGroup, callback: Function): void;    
}

declare var System: any

//TODO:  only loaded gem javascript hashes that are registered on the cloud gem portal
export abstract class AbstractCloudGemLoader implements Loader {
    protected abstract generateMeta(activeDeployment: Deployment, resourceGroup: ResourceGroup): CloudGemMeta;
    private _cache: Map<string, Gemifiable>;    

    constructor(protected http: Http) {        
        this._cache = new Map<string, Gemifiable>();
    }

    public load(assignableProviders: any, deployment: Deployment, resourceGroup: ResourceGroup, callback: Function): void {
        let key = this.path(deployment.settings.name, resourceGroup.logicalResourceId)

        if (this._cache[key]) {
            callback(this._cache[key]);
            return;
        }

        let cloudGem = this.generateMeta(deployment, resourceGroup);

        if (!cloudGem.url) {
            console.warn("Cloud gem path was not a valid URL: " + resourceGroup.stackId);
            return;
        }

        //override the SystemJS normalize for cloud gem imports as the normalization process changes the pre-signed url signature
        let normalizeFn = System.normalize
        System.normalize = function (name, parentName) {            
            if ((name[0] != '.' || (!!name[1] && name[1] != '/' && name[1] != '.')) && name[0] != '/' && !name.match(System.absURLRegEx))
                return normalizeFn(name, parentName)
            return name
        }               
        System.import(cloudGem.url)
            .then((ref: any) => {                
                let context = {
                    meta: cloudGem,
                    provider: assignableProviders
                }
                let gem: Gemifiable = ref.getInstance(context);
                this._cache[cloudGem.key] = gem;
                callback(gem);
            },
            (err: any) => {
                console.log(err);
                callback(undefined);
            })
            .catch(console.error.bind(console));         
        System.normalize = normalizeFn;
    }

    protected path(deployment_name: string, gem_name: string): string {
        return Schema.Folders.CGP_RESOURCES + "/deployment/" + deployment_name + "/resource-group/" + gem_name;
    }
}

@Injectable()
export class AwsCloudGemLoader extends AbstractCloudGemLoader {
    private _pathloader: CloudGemMeta[];

    public get onPathLoad(): CloudGemMeta[] {
        return this._pathloader;
    }

    constructor(protected http: Http,
        private aws: AwsService
    ) {
        super(http);
        console.log("Using AWS gem loader!");
        this._pathloader = [];
    }

    public generateMeta(deployment: Deployment, resourceGroup: ResourceGroup): CloudGemMeta {
        let resourceGroupName = resourceGroup.logicalResourceId;
        let baseUrl = this.baseUrl(deployment.settings.ConfigurationBucket, deployment.settings.name, resourceGroupName);
        let key = this.path(deployment.settings.name, resourceGroupName)        
        let signedurl = this.aws.context.s3.getSignedUrl('getObject', { Bucket: deployment.settings.ConfigurationBucket, Key: key + "/" + resourceGroupName.toLocaleLowerCase() + '.js', Expires: 600 })        
        return <CloudGemMeta>{
            name: resourceGroupName,
            basePath: baseUrl,
            bucketId: deployment.settings.ConfigurationBucket,
            key: key,
            url: signedurl,
            deploymentId: deployment.settings.name,
            cloudFormationOutputMap: resourceGroup.outputs
        };
    }

    //Format: <bucket physical id>/upload/afc8fa3f-ec8c-41c3-9c6f-42a173e2e1bb/deployment/dev/resource-group/DynamicContent/cgp-resource-code/dynamiccontent.js
    private baseUrl(bucket: string, deployment_name: string, gem_name: string): string {
        return bucket + "/" + this.path(deployment_name, gem_name);
    }

}

@Injectable()
export class LocalCloudGemLoader extends AbstractCloudGemLoader {
    private _map: Map<string, string>;

    constructor(protected http: Http) {
        super(http);
        this._map = new Map<string, string>();
        this._map["cloudgemmessageoftheday"] = 'external/cloudgemmessageoftheday/cloudgemmessageoftheday.js';
        this._map["cloudgemdynamiccontent"] = 'external/cloudgemdynamiccontent/cloudgemdynamiccontent.js';
        this._map["cloudgemplayeraccount"] = 'external/cloudgemplayeraccount/cloudgemplayeraccount.js';
        this._map["cloudgemleaderboard"] = 'external/cloudgemleaderboard/cloudgemleaderboard.js';                    
    }
    

    public generateMeta(deployment: Deployment, resourceGroup: ResourceGroup): CloudGemMeta {
        let name = resourceGroup.logicalResourceId.toLocaleLowerCase()
        let path = this._map[name]

        return <CloudGemMeta>{
            name: name,
            bucketId: "",
            key: "",
            basePath: path,
            url: path,
            deploymentId: deployment.settings.name,
            cloudFormationOutputMap: resourceGroup.outputs
        };
    }

}
