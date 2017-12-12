import { Injectable } from '@angular/core';
import * as environment from 'app/shared/class/index'

@Injectable()
export class DefinitionService {
    public isProd: boolean = environment.isProd;   

    private _dev_defines = {}
    private _prod_defines = {}    

    private _current_defines: any;

    constructor() {
        if (this.isProd) {
            this._current_defines = this._prod_defines
        } else {
            this._current_defines = this._dev_defines
            this._current_defines.isDev = true;
        }        

        this._current_defines.debugModel = true;                
    }   

    public get defines(): any {
        return this._current_defines;
    }

    public get isTest(): any {
        return environment.isTest;
    }
    
}