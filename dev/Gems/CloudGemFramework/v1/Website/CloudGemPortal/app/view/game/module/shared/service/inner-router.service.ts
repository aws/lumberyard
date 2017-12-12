import { Injectable } from '@angular/core';
import { Observable } from 'rxjs/Observable'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';

@Injectable()
export class InnerRouterService {
    get onChange(): Observable<string> {
        return this._bs.asObservable();
    }

    private _bs: BehaviorSubject<string>;

    constructor() {
        this._bs = new BehaviorSubject<string>('');
    }   

    public change(route: string) {
        this._bs.next(route);
    }
}