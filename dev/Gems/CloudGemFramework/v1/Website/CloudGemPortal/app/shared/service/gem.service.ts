import { Injectable, OnInit } from '@angular/core';
import { Subject } from 'rxjs/Subject';
import { Observable } from 'rxjs/Observable';
import { Gemifiable } from 'app/view/game/module/cloudgems/class/gem-interfaces';

@Injectable()
export class GemService {

    private _currentGems: Gemifiable[];
    private _currentGem: Gemifiable;
    private _currentGemSubscription: Subject<Gemifiable>;
    private _isLoading: boolean;

    constructor() {
        this._currentGems = new Array<Gemifiable>();
        this._currentGem = null;
        this._currentGemSubscription = new Subject<Gemifiable>();
        this._isLoading = true;
    }

    /**
     * Add a gem for the gem service to track
     * @param gem gem to add
     */
    addGem(gem: Gemifiable) {

        // Only add a new gem if that identifier isn't present in the c
        var gemExists = false;
        this._currentGems.forEach(currentGem => {
            if (currentGem.identifier === gem.identifier) {
                gemExists = true;
            }
        });
        if (!gemExists) {
                this._currentGems.push(gem);
                this._currentGemSubscription.next(gem);
        }
    }

    /**
     * Returns an array of the current deployment's gems
     */
    get currentGems(): Array<Gemifiable> {
        return this._currentGems;
    }

    set isLoading(isLoading: boolean) {
        this._isLoading = isLoading;
    }

    get isLoading(): boolean {
        return this._isLoading;
    }

    /**
     *  Returns the gem object for the passed in unique gem ID.  If the gem doesn't exist return null
     * @param gemId
     */
    getGem(gemId: string): Gemifiable {
        var gemToReturn = null;
        this._currentGems.forEach(gem => {
            if(gemId === gem.identifier) {
                gemToReturn = gem;
            }
        });
        return gemToReturn;

    }

    /**
     * Return the current gem, null if no gem is active
     */
    get currentGem(): Gemifiable {
        return this._currentGem;
    }

    // Methods to keep track of the current gem being used
    set currentGem(gem) {
        this._currentGem = gem;
    }

    clearCurrentGems(): void {
        this._currentGems = [];
    }

    get currentGemSubscription(): Observable<Gemifiable> {
        return this._currentGemSubscription.asObservable();
    }

}