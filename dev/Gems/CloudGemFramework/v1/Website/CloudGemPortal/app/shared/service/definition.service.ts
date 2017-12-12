import { Injectable} from '@angular/core';

@Injectable()
export class DefinitionService {
    public isProd: boolean = false;   

    private _dev_defines = {}
    private _prod_defines = {}    

    private _current_defines: any;

    constructor() {
        if (this.isProd) {
            this._current_defines = this._prod_defines
        } else {
            this._current_defines = this._dev_defines
        }        

        this._current_defines.debugModel = true;
    }   

    public get defines(): any {
        return this._current_defines;
    }
}