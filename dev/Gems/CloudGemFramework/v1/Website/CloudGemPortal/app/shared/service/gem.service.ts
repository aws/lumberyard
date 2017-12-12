import { Injectable, OnInit } from '@angular/core';
import { Subject } from 'rxjs/Subject';
import { Observable } from 'rxjs/Observable';
import { Gemifiable } from 'app/view/game/module/cloudgems/class/gem-interfaces';

@Injectable()
export class GemService {

    private _currentGems: Gemifiable[];
    private _currentGem: Gemifiable;
    private _behaviour: Subject<Gemifiable>;

    constructor() {
        this._currentGems = new Array<Gemifiable>();
        this._currentGem = null;
        this._behaviour = new Subject<Gemifiable>();        
    }

    addGem(gem: Gemifiable) {        
        this._currentGems.push(gem);
        this._behaviour.next(gem);
    }

    getGems(): Array<Gemifiable> {
        return this._currentGems;
    }

    get currentGem(): Gemifiable {
        return this._currentGem;
    }

    // Methods to keep track of the current gem being used
    set currentGem(gem) {
        this._currentGem = gem;
    }

    get observable(): Observable<Gemifiable> {
        return this._behaviour.asObservable();
    }
}