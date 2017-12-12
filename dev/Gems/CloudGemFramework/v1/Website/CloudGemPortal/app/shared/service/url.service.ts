import { Injectable } from '@angular/core';
import {
    Router    
} from '@angular/router';

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

    constructor(private router: Router) {       
    }

    public parseLocationHref(base_url:string) {
        this._baseUrl = base_url.replace('/login',''); // this.router.url;
        this._querystring = this.baseUrl.split('?')[1];
        this._fragment = this.baseUrl.split('#')[1];        
    }
    
}