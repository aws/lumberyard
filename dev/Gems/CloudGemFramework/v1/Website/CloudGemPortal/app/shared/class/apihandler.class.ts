import { RequestOptions, Request, RequestMethod, Response, Http, Headers } from '@angular/http';
import { Observable } from 'rxjs/Observable';
import { Router } from "@angular/router";
import { AwsService } from "app/aws/aws.service";
import { LyMetricService, Event } from 'app/shared/service/index';
import 'rxjs/add/operator/concatAll'
import 'rxjs/add/observable/throw'

//Use AWS from JS
declare var AWS: any;

declare module String {
    export var format: any;
}

const DEFAULT_ACCEPT_TYPE = "application/json;charset=utf-8";
const DEFAULT_CONTENT_TYPE = "application/json;charset=utf-8";
const HEADER_CONTENT_TYPE = "Content-Type";
const SIGNING_API = "execute-api"

export interface Restifiable {
    get(restPath: string): Observable<any>;
    post(restPath: string, body?: any): Observable<any>;
    put(restPath: string, body: any): Observable<any>;
    delete(restPath: string, body?: any): Observable<any>
}

export abstract class ApiHandler implements Restifiable {

    private _serviceAPI: string;
    private _http: Http;
    private _aws: AwsService;
    private _identifier: string;
    private _metric: LyMetricService;

    constructor(serviceAPI: string, http: Http, aws: AwsService, metric: LyMetricService = null, metricIdentifier: string = "") {
        this._serviceAPI = serviceAPI;
        this._http = http;
        this._aws = aws;
        this._identifier = metricIdentifier;
        this._metric = metric
        this.get = this.get.bind(this)
        this.post = this.post.bind(this)
        this.put = this.put.bind(this)
        this.delete = this.delete.bind(this)
        this.sortQueryParametersByCodePoint = this.sortQueryParametersByCodePoint.bind(this)
        this.execute = this.execute.bind(this)
        this.mapResponseCodes = this.mapResponseCodes.bind(this)
        this.recordEvent = this.recordEvent.bind(this)
        this.AwsSigv4Signer = this.AwsSigv4Signer.bind(this)        
    }

    public get(restPath: string, headers: any = []): Observable<any> {        
        let options = {
            method: RequestMethod.Get,
            headers: headers,
            url: this._serviceAPI + "/" + restPath
        };        

        return this.execute(options);
    }

    private sortQueryParametersByCodePoint(url: string): string {
        let parts = url.split("?")
        if (parts.length == 1) {
            return url;
        }

        let questring = parts[1]
        console.log(questring)


        return url;
        
    }

    public post(restPath: string, body?: any, headers: any = []): Observable<any> {
        var options = {
            method: RequestMethod.Post,
            url: this._serviceAPI + "/" + restPath,
            headers: headers,
            body: JSON.stringify(body)
        };
        options.headers[HEADER_CONTENT_TYPE] = DEFAULT_CONTENT_TYPE

        return this.execute(options)
    }

    public put(restPath: string, body?: any, headers: any = []): Observable<any> {        
        var options = {
            method: RequestMethod.Put,
            headers: headers,
            url: this._serviceAPI + "/" + restPath,
            body: JSON.stringify(body)
        };
        options.headers[HEADER_CONTENT_TYPE] = DEFAULT_CONTENT_TYPE

        return this.execute(options)
    }

    public delete(restPath: string, body?: any, headers: any = []): Observable<any> {
        var options = {
            method: RequestMethod.Delete,
            headers: headers,
            url: this._serviceAPI + "/" + restPath,
            body: JSON.stringify(body)
        };
        options.headers[HEADER_CONTENT_TYPE] = DEFAULT_CONTENT_TYPE
        
        return this.execute(options)
    }

    private execute(options: any): Observable<any> {
        let request_observable = new Observable<any>((observer) => {
            var signedRequest = this.AwsSigv4Signer(options);
            if (signedRequest == null) {
                observer.error(new Error("Status: CreateRequestFailed, Message: Unable to create request"));
            } else {
                let http_observable = this._http.request(signedRequest);

                this.recordEvent("ApiServiceRequested", {
                    "Identifier": this._identifier,
                    "Verb": RequestMethod[options.method],
                    "Path": options.url.replace(this._serviceAPI, "")
                }, null);
                observer.next(this.mapResponseCodes(http_observable))
            }
        });

        return this._aws.context.authentication.refreshSessionOrLogout(request_observable.concatAll()).concatAll();
    }

    private mapResponseCodes(obs: Observable<any>): Observable<any> {
        return obs.map((res: Response) => {
            if (res) {
                this.recordEvent("ApiServiceSuccess", {
                    "Identifier": this._identifier,
                    "Path": res.url.replace(this._serviceAPI, "")
                }, null);
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
                }
            }
            this.recordEvent("ApiServiceError", {
                "Code": error.status,
                "Message": error.statusText,
                "Identifier": this._identifier                
            }, null);
            if (error.status == 'Missing credentials in config') {
                this._aws.context.authentication.logout(true)
            }
            return Observable.throw(new Error("Status: " + error.status + ", Message:" + error.statusText));
            })
    }

    public recordEvent(eventName: Event, attribute: { [name: string]: string; } = {}, metric: { [name: string]: number; } = null) {
        if (this._metric !== null)
            this._metric.recordEvent(eventName, attribute, metric);
    }
      
    public AwsSigv4Signer(options: any) {
        if (options == null || options == undefined)
            return null

        var parts = options.url.split('?');        
        var host = parts[0].substr(8, parts[0].indexOf("/", 8) - 8);
        var path = parts[0].substr(parts[0].indexOf("/", 8));
        var querystring = this.codePointSort(parts[1]);
        var requestmethod = RequestMethod[options.method]
        if (requestmethod == undefined)
            return null

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

        if (!AWS.config.credentials)
            this._aws.context.authentication.logout(true)

        signer.addAuthorization(AWS.config.credentials, now);
        options.method = options.methodIndex;

        delete options.search;
        delete options.pathname;
        delete options.headers.host;
        return new Request(options);
    };

    private codePointSort(querystring: string): string {
        if (!querystring)
            return undefined
        var parts = querystring.split("&")
        var array = []
        parts.forEach(function (param) {
            var key_value = param.split("=")
            var obj = {
                key: key_value[0],
                val: key_value.length > 1 ? key_value[1] : undefined
            }
            array.push(obj)
        });

        array = array.sort((p1, p2) => {
            return p1.key > p2.key ? 1 : p1.key < p2.key ? -1 : 0
        })

        var sorted_querystring = ""
        var i = 0
        array.forEach(function (obj) {
            i++;
            sorted_querystring += obj.key + "=" + obj.val
            if (i < array.length)
                sorted_querystring += "&"
        });
        return sorted_querystring
    }
}