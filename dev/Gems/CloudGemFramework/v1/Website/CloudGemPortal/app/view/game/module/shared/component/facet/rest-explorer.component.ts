import { Component, Input, ViewEncapsulation, OnInit } from '@angular/core';
import { AwsService } from "app/aws/aws.service";
import { ApiHandler, StringUtil } from 'app/shared/class/index';
import { LyMetricService } from 'app/shared/service/index';
import { Http } from '@angular/http';
import { Facetable } from '../../class/index';

@Component({
    selector: 'body-tree-view',
    template: `
                <div *ngIf="data['properties'] !== undefined">
                    <div *ngFor="let prop of data['properties'] | objKeys">
                        <div *ngIf="isObjectToDisplay(data['properties'][prop])" class="row mb-3 no-gutters rest-prop" [ngClass]="{'has-danger': data['properties'][prop].valid === false}">
                            <span class="col-lg-2" affix> {{prop}} </span>                        
                            <input *ngIf="!isObjectOrArray(data['properties'][prop].type)" class="form-control" type="{{data['properties'][prop].type}}" [ngClass]="{'input-dotted' : data['required'] === undefined || data['required'].indexOf(prop) < 0, 'form-control-danger': data['properties'][prop].valid === false}" 
                                [(ngModel)]="data['properties'][prop].value" 
                                placeholder="{{placeholder(data['properties'][prop])}}" />                            
                            <textarea rows="10" *ngIf="isObjectOrArray(data['properties'][prop].type)" class="col-lg-8 form-control" [ngClass]="{'input-dotted' : data['required'] === undefined || data['required'].indexOf(prop) < 0, 'form-control-danger': data['properties'][prop].valid === false}" 
                                [(ngModel)]="data['properties'][prop].value" 
                                placeholder="{{placeholder(data['properties'][prop])}}">
                            </textarea>
                            <span class="ml-3"><i *ngIf="data['properties'][prop].description" class="fa fa-question-circle-o" placement="bottom" ngbTooltip="{{data['properties'][prop].description}}"></i></span>
                            <div *ngIf="data['properties'][prop].message" class="form-control-feedback">{{data['properties'][prop].message}}</div>
                        </div>
                        <body-tree-view [data]="data['properties'][prop]"></body-tree-view>          
                    </div>
                </div>
  `
})
export class BodyTreeViewComponent {
    @Input() data: any;

    ngOnInit(): void {             
    }

    placeholder(obj: any) {        
        if (obj.type.indexOf('array') >= 0 && obj.items) {
            if (obj.items.properties) {
                let keys = Object.keys(obj.items.properties)
                let result = "[{"
                for (var key in obj.items.properties) {
                    result += key + ":" + obj.items.properties[key].type + ","
                }
                result = result.split(",").slice(0, keys.length).join(",")
                result += "}]"
                return (keys ? result : null)
            } else {
                return obj.type + "[" + obj.items.type + "]"
            }
        } else {
            return obj.type
        }
    }

    isObjectOrArray(type:string): boolean {
        if (!type)
            return false        
        return type.match(/(array|object)/g) ? true : false                   
    }

    isObjectToDisplay(obj: any): boolean {
        if (!obj)
            return false;
        let result = obj.properties && Object.keys(obj.properties).length > 0 ? false : true        
        return result

    }
}

@Component({
    selector: 'facet-rest-api-explorer',
    templateUrl: 'app/view/game/module/shared/component/facet/rest-explorer.component.html',
    styleUrls: ['app/view/game/module/shared/component/facet/rest-explorer.component.css'],
    encapsulation: ViewEncapsulation.None
})
export class RestApiExplorerComponent implements Facetable {
    @Input() data: any;

    private serviceUrl;
    private api: RestAPIExplorer
    private model = {
        path: {
            selected: undefined,
            verbs: undefined,
            verb: undefined,
            verbValueValid: true,
            value: undefined,
            parameters: undefined,
        },
        harmonized: {
            paths: []
        },
        showSwagger: false,
        swagger: undefined,
        template: undefined,
        response: [],
        multiplier: 1
    }

    private hasPathParams: boolean = false;
    private hasQueryParams: boolean = false;
    private functions: Array<string> =  ["{rand}"]
    private FUNC_RAND_IDX = 0
    private isRunningCommand = false;

    constructor(private aws: AwsService, private http: Http, private metric: LyMetricService) {
    }

    ngOnInit() {
        this.serviceUrl = this.data["ServiceUrl"];
        let apiid = this.aws.parseAPIId(this.serviceUrl);
        this.getSwagger(apiid, this.aws.context.apiGateway, this.model)
        this.api = new RestAPIExplorer(this.serviceUrl, this.http, this.aws, this.metric, this.data["Identifier"])
    }

    getSwagger(apiid: string, apiGateway: any, model: any) {
        var params = {
            exportType: 'swagger',
            restApiId: apiid,
            stageName: 'api',
            accepts: 'application/json',
            parameters: {
                'extensions': 'integrations, authorizers, documentation, validators, gatewayresponses',
                'Accept': 'application/json'
            }
        };

        this.aws.context.apiGateway.getExport(params, function (err, data) {
            if (err) {
                console.log(err)
                return
            }
            model.swagger = JSON.parse(data.body)
            let paths = Object.keys(model.swagger.paths)
            for (var i = 0; i < paths.length; i++) {
                let parts = paths[i].split("{")
                let obj = {
                    original: paths[i],
                    harmonized: parts[0]
                }
                model.harmonized.paths.push(obj)
            }
        });
    }

    getBodyParameters(parameter: any): any {
        if (this.model.template)
            return this.model.template
        this.model.template = {}
        this.defineBodyTemplate(parameter, this.model.template)
        return this.model.template
    }

    //recursively iterate all nested swaggger schema definitions
    defineBodyTemplate(parameter: any, master: any): any {

        if (typeof parameter != 'object') {
            return undefined;
        }
        if (parameter['$ref']) {
            let schema = parameter['$ref'];            
            let parts = schema.split('/');
            let template = this.model.swagger
            for (let i = 1; i < parts.length; i++) {
                let part = parts[i]
                template = template[part]
            }
            if (template.type == 'array') {
                parameter.type = 'array[' + template.items.type + ']'
                return undefined
            }
            return template
        }

        for (var key in parameter) {
            var value = parameter[key];
            let result = this.defineBodyTemplate(value, master)
            if (result !== undefined) {
                if (key == 'schema') {
                    master = Object.assign(master, result)
                } else {
                    parameter[key] = result
                }
                this.defineBodyTemplate(result, master)
            }
        }
        return undefined;
    }

    initializeVerbContext(verb: string): void {
        this.model.path.verbValueValid = true;
        this.model.path.verb = verb;
        this.model.path.parameters = this.model.path.verbs[verb].parameters ? this.model.path.verbs[verb].parameters : []
        this.model.template = undefined
        this.hasPathParams = false;
        this.hasQueryParams = false;
        for (let params of this.model.path.parameters) {
            if (params.in == "path") {
                this.hasPathParams = true;
            }
            if (params.in == "query") {
                this.hasQueryParams = true;
            }
        }
    }

    isValidRequest(): boolean {
        let isValid = true;

        if (this.model.path.verb === undefined) {
            this.model.response.push("Select a VERB for this request")
            this.model.path.verbValueValid = false;
            return false
        }

        for (var i = 0; i < this.model.path.parameters.length; i++) {
            let param = this.model.path.parameters[i];
            param.valid = true
            //validate GET parameters
            if (param.in == 'query' && param.required && (param.value === null || param.value === undefined)) {
                param.message = "The GET querystring parameter '" + param.name + "' is a required field."
                isValid = param.valid = false
            //validate PATH parameters
            } else if (param.in == 'path' && param.required && (param.value === null || param.value === undefined)) {
                param.message = "The PATH parameter '" + param.name + "' is a required field."
                isValid = param.valid = false
            //Validate POST parameters
            } else if (param.in == 'body') { 
                if (param.required && this.model.template['required'] && (param.value === null || param.value === undefined)) {
                    isValid = param.valid = false
                    param.message = "The BODY parameter '" + param.name + "' is a required field"
                }
                if (this.model.template && this.model.template['properties']) {
                    for (var key in this.model.template['properties']) {
                        var requiredproperty = this.model.template['required'] ? this.model.template['required'].filter(e => e == key).length > 0 : false                        
                        var prop = this.model.template['properties'][key]
                        prop.message = undefined
                        if (prop.type == 'integer') {
                            isValid = prop.valid = requiredproperty ?
                                prop.value !== undefined && prop.value.length > 0 && !isNaN(Number.parseInt(prop.value))
                                : prop.value !== undefined ? prop.value.length > 0 && !isNaN(Number.parseInt(prop.value)) : true
                            if (!isValid)
                                prop.message = "The POST body parameter '" + key + "' is a " + (requiredproperty ? "required " : "") + "integer field."
                        } else if (prop.type && (prop.type == 'object' || prop.type.indexOf('array') >= 0)) {  
                            let matches = prop.value ? prop.value.match(/[^\"\']{rand}[^\"\']/g) : []
                            if (matches) {
                                for (let i = 0; i < matches.length; i++) {
                                    let match = matches[i]
                                    let first_char = match.substring(0, 1)
                                    let last_char = match.substring(match.length - 1, match.length)
                                    let new_str = first_char + '"' + this.functions[this.FUNC_RAND_IDX] + '"' + last_char                                    
                                    prop.value = prop.value.split(match).join(new_str);
                                }                                
                            }                            
                            try {
                                JSON.parse(prop.value)
                                isValid = prop.valid = true
                            } catch (e) {
                                isValid = prop.valid = false
                                prop.message = "The POST body parameter '" + key + "' is a " + (requiredproperty ? "required " : "") + "object field.  " + (<Error>e).message
                            }
                        } else {
                            isValid = prop.valid = requiredproperty ? prop.value !== undefined && prop.value.length > 0 : true
                            if (!isValid)
                                prop.message = "The POST body parameter '" + key + "' is a  " + (requiredproperty ? "required " : "") + "string field."
                        }                        
                    }
                }
            }
            if (param.message)
                console.log(param.message)
        }

        return isValid;
    }

    initializePathContext(path: any) {
        this.model.path.value = path.original;
        this.model.path.selected = path.original;
        this.model.path.verbs = this.deleteOptions(this.model.swagger.paths[path.original]);
        this.model.path.parameters = [];
        this.model.path.verb = undefined;
    }

    hasBody(parameters: any) {
        if (parameters === undefined)
            return false

        for (var i = 0; i < parameters.length; i++) {
            if (parameters[i].in == 'body') {
                return true
            }
        }
        return false
    }

    deleteOptions(object: any): any {
        delete object.options;
        return object;
    }

    send(): void {
        this.model.response = [];

        if (!this.isValidRequest())
            return;

        let parameters = this.getParameters();   
        for (let i = 0; i < this.model.multiplier; i++ ) {
            this.execute(
                this.addRandomCharacters(parameters.path, 5),
                this.addRandomCharacters(parameters.querystring, 5),
                JSON.parse(this.addRandomCharacters(JSON.stringify(parameters.body), 5)));
        }
    }

    getParameters(): any {
        let path : string = this.model.path.selected.substring(1)
        let querystring = "";
        let body = {};
        for (var i = this.model.path.parameters.length - 1; i >= 0; i--) {
            let param = this.model.path.parameters[i]
            if (param.in == 'query' && this.isValidValue(param.value)) {
                let separator = querystring.indexOf('?') == -1 ? '?' : '&'
                querystring += separator + param.name + "=" + param.value.trim()
            } else if (param.in == 'body') {                
                body = this.defineBodyParameters(this.model.template)
            } else if (param.in == 'path' && this.isValidValue(param.value)) {
                path = path.replace("{" + param.name + "}", param.value.trim()) 
                path += '/'
            }
        }
        return {
            'path': path.charAt(path.length - 1) == '/' ? path.substr(0, path.length - 1) : path,
            'querystring': querystring,
            'body': body
        }
    }

    isValidValue(value): boolean {
        return value !== undefined && value !== null && value.trim() !== ""
    }

    defineBodyParameters(obj: any): any {
        let nested = {};
        if (obj) {
            for (var key in obj['properties']) {
                let entry = obj['properties'][key]
                if (entry.value) {
                    if (entry.type == 'integer' || entry.type == 'number') {
                        nested[key] = Number.parseInt(entry.value)
                    } else if (entry.type && (entry.type == 'object' || entry.type.indexOf('array') >= 0)) {
                        nested[key] = JSON.parse(entry.value)
                    } else {
                        nested[key] = entry.value.trim()
                    }
                } else if (entry.properties) {
                    nested[key] = this.defineBodyParameters(entry)                    
                } else if (entry.items) {
                    nested[key] = this.defineBodyParameters(entry.items)                    
                }
            }
        }
        return nested;

    }

    execute(path: string, querystring: string, body: string): void {
        if (this.model.path.verb == 'get') {
            this.api.get(path + querystring).subscribe(response => {                                
                this.addToResponse(this.runView(this.requestParameters(path, querystring, body), JSON.parse(response.body.text())))
            }, (err) => {
                this.addToResponse(err.message)   
            })
        } else if (this.model.path.verb == 'post') {
            this.api.post(path + "/" + querystring, body).subscribe(response => {
                this.addToResponse(this.runView(this.requestParameters(path, querystring, body), JSON.parse(response.body.text())))
            }, (err) => {
                this.addToResponse(err.message)   
            })
        } else if (this.model.path.verb == 'put') {
            this.api.put(path + "/" + querystring, body).subscribe(response => {
                this.addToResponse(this.runView(this.requestParameters(path, querystring, body), JSON.parse(response.body.text())))
            }, (err) => {
                this.addToResponse(err.message)   
            })
        } else if (this.model.path.verb == 'delete') {
            this.api.delete(path + "/" + querystring).subscribe(response => {                
                this.addToResponse(this.runView(this.requestParameters(path, querystring, body), JSON.parse(response.body.text())))
            }, (err) => {
                this.addToResponse(err.message)                
            })
        }
        this.isRunningCommand = true
    }

    addToResponse(param: string): void {
        this.model.response.push(param)
        this.isRunningCommand = false
    }

    runView(request: any, response: any): any {
        return {
            request: request,
            response: response
        }
    }

    requestParameters(path, querystring, body): any {
        return {            
                path: path,
                querystring: querystring,
                body: body            
        }        
    }

    openAWSConsole(host: string): void {
        let parts = host.split('.')
        window.open(encodeURI("https://console.aws.amazon.com/apigateway/home?region=" + this.aws.context.region + "#/apis/" + parts[0] + "/resources/"), 'awsConsoleAPIGatewayViewer')
    }

    
    /**
     * Add random characters by string replacing {rand} with random alphanumerical characters.  An example of this is to quickly create multiple random accounts.
     * @param queryString entire query string
     * @param numberOfCharsToAdd number of characters to add to the string
     */
    public addRandomCharacters(value: string, numberOfCharsToAdd: number): any {
        if (value) {            
            let matches = value.match(/\{rand\}/g)
            if (matches) {                
                for (let i = 0; i < matches.length; i++) {
                    var randChars = Math.random().toString(36).substr(2, numberOfCharsToAdd)
                    let match = matches[i]
                    value = value.replace(/\{rand\}/, randChars);
                }
                return value;
            }
        }
        return value;
    }
}


export class RestAPIExplorer extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metrics: LyMetricService, metricIdentifier: string) {
        super(serviceBaseURL, http, aws, metrics, metricIdentifier);
    }

}

