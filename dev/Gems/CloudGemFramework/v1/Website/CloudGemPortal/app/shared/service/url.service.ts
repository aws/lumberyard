import { Injectable, Component } from '@angular/core';
import { AwsService } from "app/aws/aws.service";

@Injectable()
export class UrlService {

    private _fragment: string
    private _querystring: string
    private _baseUrl: string 
    private _encryptionPayload: string[]

    get baseUrl(): string {
        return this._baseUrl;
    }

    get querystring(): string {
        return this._querystring;
    }

    get fragment(): string {
        return this._fragment;
    }

    get encryptedPayload(): string[] {
        return this._encryptionPayload;
    }

    constructor(private aws: AwsService) {       
    }

    public parseLocationHref(base_url:string) {
        this._baseUrl = base_url.replace('/login',''); // this.router.url;
        this._querystring = this.baseUrl.split('?')[1];
        this._fragment = this.baseUrl.split('#')[1];        
    }

    public signUrls(component: Component, bucket: string, expirationinseconds: number): void {
        if (!bucket)
            return;

        if (component.templateUrl) {
            component.templateUrl = this.aws.context.s3.getSignedUrl('getObject', { Bucket: bucket, Key: component.templateUrl, Expires: expirationinseconds })        
        }

        if (!component.styleUrls)
            return;

        for (let i = 0; i < component.styleUrls.length; i++) {
            component.styleUrls[i] = this.aws.context.s3.getSignedUrl('getObject', { Bucket: bucket, Key: component.styleUrls[i], Expires: expirationinseconds })        
        }       
    }
}