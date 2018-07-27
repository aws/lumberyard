import { Injectable } from '@angular/core';
import * as environment from 'app/shared/class/index'

export class DirectoryService {
    private _serviceApiConstructor: Function
    private _context: string

    constructor(context: string, service_api_constructor: Function) {
        this._serviceApiConstructor = service_api_constructor
        this._context = context
    }

    public get constructor(): Function {
        return this._serviceApiConstructor
    }

    public get serviceUrl(): string {
        return this._context['ServiceUrl']
    }
    public get context(): any {
        return this._context;
    }
}

@Injectable()
export class DefinitionService {
    public isProd: boolean = environment.isProd;

    private _dev_defines = {}
    private _prod_defines = {}

    private _current_defines: any;
    private _directoryService: Map<string, DirectoryService>;

    constructor() {
        if (this.isProd) {
            this._current_defines = this._prod_defines
        } else {
            this._current_defines = this._dev_defines
            this._current_defines.isDev = true;
        }
        this._directoryService = new Map<string, DirectoryService>();
        this._current_defines.debugModel = true;
    }

    public get defines(): any {
        return this._current_defines;
    }

    public get isTest(): any {
        return environment.isTest;
    }

    /*
    *  Add a new service to the service listing for other gems to utilize.
    */
    public addService(id: string, context: any, service_api_constructor: Function) {
        this._directoryService[id.toLowerCase()] = new DirectoryService(context, service_api_constructor)
    }

    /*
    *  Get a specific gem service to utilize from your gem
    */
    public getService(name: string) {
        return this._directoryService[name.toLowerCase()]
    }

}