import { RequestOptions, Request, RequestMethod, Response, Http, Headers } from '@angular/http';
import { Observable } from 'rxjs/rx';
import { Router } from "@angular/router";
import { AwsService } from "app/aws/aws.service";

//Use AWS from JS
declare var AWS: any;
declare var toastr: any;

declare module String {
    export var format: any;
}

const DEFAULT_ACCEPT_TYPE = "application/json;charset=utf-8";
const DEFAULT_CONTENT_TYPE = "application/json;charset=utf-8";
const HEADER_CONTENT_TYPE = "Content-Type";
const SIGNING_API = "execute-api"

export interface Restifiable{    
    get(restPath: string): Observable<any>;
    post(restPath: string, body?: any): Observable<any>;
    put(restPath: string, body: any): Observable<any>;
    delete(restPath: string): Observable<any>
}

export abstract class ApiHandler implements Restifiable {

    private _serviceAPI: string;
    private _http: Http;
    private _aws: AwsService;

    constructor(serviceAPI: string, http: Http, aws: AwsService) {
        this._serviceAPI = serviceAPI;
        this._http = http;
        this._aws = aws;
    }

    public get(restPath: string): Observable<any> {
        let url = this._serviceAPI + "/" + restPath;

        var options = {
            method: RequestMethod.Get,
            headers: [],
            url: url
        };

        var signedRequest = this.AwsSigv4Signer(options);
        let observable = this._http.request(signedRequest);

        return this.mapResponseCodes(observable);
    }

    public post(restPath: string, body?: any): Observable<any> {
        let url = this._serviceAPI + "/" + restPath;

        var options = {
            method: RequestMethod.Post,
            url: url,
            headers: [],
            body: JSON.stringify(body)
        };
        options.headers[HEADER_CONTENT_TYPE] = DEFAULT_CONTENT_TYPE

        var signedRequest = this.AwsSigv4Signer(options);
        let observable = this._http.request(signedRequest);

        return this.mapResponseCodes(observable);
    }

    public put(restPath: string, body: any): Observable<any> {
        let url = this._serviceAPI + "/" + restPath;

        var options = {
            method: RequestMethod.Put,
            headers: [],
            url: url,
            body: JSON.stringify(body)
        };
        options.headers[HEADER_CONTENT_TYPE] = DEFAULT_CONTENT_TYPE

        var signedRequest = this.AwsSigv4Signer(options);
        let observable = this._http.request(signedRequest);

        return this.mapResponseCodes(observable);
    }

    public delete(restPath: string): Observable<any> {
        let url = this._serviceAPI + "/" + restPath;

        var options = {
            method: RequestMethod.Delete,
            url: url
        };

        var signedRequest = this.AwsSigv4Signer(options);
        let observable = this._http.request(signedRequest);

        return this.mapResponseCodes(observable);
    }

    private mapResponseCodes(obs: Observable<any>): Observable<any> {
        return obs.map((res: Response) => {
            if (res) {
                if (res.status === 201) {
                    return { status: res.status, body: res }
                }
                else if (res.status === 200) {
                    return { status: res.status, body: res }
                }
            }
        }).catch((error: any) => {
            let error_body = error._body
            if (error_body != undefined) {
                var obj = undefined;
                try {
                    obj = JSON.parse(error_body);
                } catch (err) {                                        
                }
                if (obj != undefined) {                    
                    if (obj.Message != null) {
                        error.statusText += ", " + obj.Message
                    } else if (obj.errorMessage != null) {
                        error.statusText = obj.errorMessage
                    } else if (obj.message != null) {
                        error.statusText += ", " + obj.message;
                    }
                    if (error.code == "NotAuthorizedException" || error.code == "CredentialsError")
                        this._aws.context.authentication.logout();
                } else {                
                    this._aws.context.authentication.logout();
                }
            }

            if (error.status === 500) {
                return Observable.throw(new Error("Status: "+ error.status+", Message:"+ error.statusText));
            }
            else if (error.status === 400) {
                return Observable.throw(new Error("Status: " + error.status + ", Message:" + error.statusText));
            }
            else if (error.status === 403) {
                return Observable.throw(new Error("Status: " + error.status + ", Message:" + error.statusText));
            }
            else if (error.status === 409) {
                return Observable.throw(new Error("Status: " + error.status + ", Message:" + error.statusText));
            }
            else if (error.status === 406) {
                return Observable.throw(new Error("Status: " + error.status + ", Message:" + error.statusText));
            } else {
                return Observable.throw(new Error("Status: " + error.status + ", Message:" + error.statusText));
            }
        });
    }

    public AwsSigv4Signer(options: any) {  
        var parts = options.url.split('?');    
        var host = parts[0].substr(8, parts[0].indexOf("/", 8) - 8);
        var path = parts[0].substr(parts[0].indexOf("/", 8));
        var querystring = parts[1];

        var now = new Date();
        if (!options.headers)
            options.headers = [];
        options.headers.host = host;
        options.headers.accept = DEFAULT_ACCEPT_TYPE

        options.pathname = function () {
            return path;
        };
        options.methodIndex = options.method;
        options.method = RequestMethod[options.method].toLocaleUpperCase();
        options.search = function () {
            return querystring ? querystring : "";
        };
        options.region = AWS.config.region;

        var signer = new AWS.Signers.V4(options, SIGNING_API);

        signer.addAuthorization(AWS.config.credentials, now);
        options.method = options.methodIndex;

        delete options.search;
        delete options.pathname;
        delete options.headers.host;
        return new Request(options);
    };
}