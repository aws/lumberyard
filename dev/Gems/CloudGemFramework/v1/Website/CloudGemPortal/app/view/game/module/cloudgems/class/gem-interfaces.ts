import { Observable } from 'rxjs/Observable'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Response } from '@angular/http';

export class TackableStatus implements Stateable {
    label: string;
    styleType: StyleType;
}

export class TackableMeasure implements Measurable {
    name: string;
    value: string;
}


//Used to populate the cloud gem stat on the thumbnail of the gems thumbnail
export interface Measurable {
    name: string,
    value: string    
}

/**
  * Tackable State
  * State: a function which returns the state of the Gem - most likely via a Lambda call
  * GemStatus: Uses an enum of possibe Gem Statuses to determine what String and CSS styling is applied in the Gem MediumTackable
**/
export interface Stateable {
    label: string,
    styleType: StyleType    
}
export type StyleType = "Offline" | "Error" | "Warning" | "Loading" | "Enabled";
export type Cost = "None" | "Low" | "Medium" | "High";

//Used to populate the cloud gems thumbnail on the gems main landing page
export interface Tackable {
    displayName: string,
    iconSrc: string,
    cost: Cost,
    state: Stateable,
    metric: Measurable
}

//Main interface for any cloud gem
export interface Gemifiable {
    identifier: string;    
    template: Observable<string>;             
    initialize(any): void;
    context: any;
    tackable: Tackable;
    classType(): any;    
    styles: Observable<string>;
    hasStyle: boolean;
}
