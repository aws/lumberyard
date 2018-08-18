import 'reflect-metadata';
import { AwsService, Deployment } from 'app/aws/aws.service';
import { ResourceGroup, ResourceOutputEnum, ResourceOutput } from 'app/aws/resource-group.class'
import { Gemifiable } from './gem-interfaces';
import { Schema } from 'app/aws/schema.class'
import { DefinitionService } from 'app/shared/service/index';
import { Http } from '@angular/http';


export abstract class Loader {
    abstract load(activeDeployment: Deployment, resourceGroups: ResourceGroup, callback: Function): void;
}

declare var System: any

export abstract class AbstractCloudGemLoader implements Loader {
    protected abstract generateMeta(activeDeployment: Deployment, resourceGroup: ResourceGroup): any;
    private _cache: Map<string, Gemifiable>;

    constructor(protected definition: DefinitionService, protected http : Http) {
        this._cache = new Map<string, Gemifiable>();
    }

    public load(deployment: Deployment, resourceGroup: ResourceGroup, callback: Function): void {
        let key = this.path(deployment.settings.name, resourceGroup.logicalResourceId)

        if (this._cache[key]) {
            callback(this._cache[key]);
            return;
        }

        let meta = this.generateMeta(deployment, resourceGroup);

        if (!meta.url) {
            console.warn("Cloud gem path was not a valid URL: " + resourceGroup.stackId);
            return;
        }

        //override the SystemJS normalize for cloud gem imports as the normalization process changes the pre-signed url signature
        let normalizeFn = System.normalize
        if (this.definition.isProd)
            System.normalize = function (name, parentName) {
                if ((name[0] != '.' || (!!name[1] && name[1] != '/' && name[1] != '.')) && name[0] != '/' && !name.match(System.absURLRegEx))
                    return normalizeFn(name, parentName)
                return name
            }

        if (!this.definition.isProd) {
            this.http.get(meta.url).subscribe(() =>
                this.importGem(meta, callback),
                (err) => {
                    console.log(err);
                    callback(undefined)
                });
        } else {
            this.importGem(meta, callback);
        }
        System.normalize = normalizeFn;
    }

    protected path(deployment_name: string, gem_name: string): string {
        return Schema.Folders.CGP_RESOURCES + "/deployment/" + deployment_name + "/resource-group/" + gem_name;
    }

    private importGem(meta, callback) {
        System.import(meta.url).then((ref: any) => {
            let annotations = Reflect.getMetadata('annotations', ref.definition())[0];
            let gem: Gemifiable = {
                identifier: meta.name,
                displayName: '',
                srcIcon: '',
                context: meta,
                module: ref.definition(),
                annotations: annotations,
                thumbnail: annotations.bootstrap[0],
                index: annotations.bootstrap[1]
            }
            if (ref.serviceApiType) {
                this.definition.addService(gem.identifier, gem.context, ref.serviceApiType())
            }
            this._cache[meta.name] = gem;
            callback(gem);
        },
        (err: any) => {
            console.log(err);
            callback(undefined);
        })
        .catch(console.error.bind(console));
    }
}

export class AwsCloudGemLoader extends AbstractCloudGemLoader {
    private _pathloader: any[];

    public get onPathLoad(): any[] {
        return this._pathloader;
    }

    constructor(
        protected definition: DefinitionService,
        private aws: AwsService,
        protected http: Http
    ) {
        super(definition, http);
        console.log("Using AWS gem loader!");
        this._pathloader = [];
    }

    public generateMeta(deployment: Deployment, resourceGroup: ResourceGroup): any {
        let resourceGroupName = resourceGroup.logicalResourceId;
        let baseUrl = this.baseUrl(deployment.settings.ConfigurationBucket, deployment.settings.name, resourceGroupName);
        let key = this.path(deployment.settings.name, resourceGroupName)
        let file_path = key + "/dist/" + resourceGroupName.toLocaleLowerCase() + '.js'
        let signedurl = this.aws.context.s3.getSignedUrl('getObject', { Bucket: deployment.settings.ConfigurationBucket, Key: file_path, Expires: 600 })
        return Object.assign(<any>{
            identifier: resourceGroupName,
            name: resourceGroupName,
            basePath: baseUrl,
            bucketId: deployment.settings.ConfigurationBucket,
            key: key,
            url: signedurl,
            deploymentId: deployment.settings.name,
            logicalResourceId: resourceGroup.logicalResourceId,
            physicalResourceId: resourceGroup.physicalResourceId
        }, resourceGroup.outputs);
    }

    //Format: <bucket physical id>/upload/afc8fa3f-ec8c-41c3-9c6f-42a173e2e1bb/deployment/dev/resource-group/DynamicContent/cgp-resource-code/dynamiccontent.js
    private baseUrl(bucket: string, deployment_name: string, gem_name: string): string {
        return bucket + "/" + this.path(deployment_name, gem_name);
    }

}

export class LocalCloudGemLoader extends AbstractCloudGemLoader {
    private _map: Map<string, string>;

    constructor(protected definition: DefinitionService, protected http: Http) {
        super(definition, http);
    }

    public generateMeta(deployment: Deployment, resourceGroup: ResourceGroup): any {
        let name = resourceGroup.logicalResourceId.toLocaleLowerCase()
        let path = "node_modules/@cloud-gems/" + name + "/" + name + ".js"

        return Object.assign(<any>{
            identifier: name,
            name: name,
            bucketId: "",
            key: "",
            basePath: path,
            url: path,
            deploymentId: deployment.settings.name,
            logicalResourceId: resourceGroup.logicalResourceId,
            physicalResourceId: resourceGroup.physicalResourceId
        }, resourceGroup.outputs);
    }



}
