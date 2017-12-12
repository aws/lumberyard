import { Observable } from 'rxjs/Observable'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Response } from '@angular/http';
import { NgModule, Component } from '@angular/core';

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
    srcIcon: string,    
    state: Stateable,
    metric: Measurable
}

//Main interface for any cloud gem
export interface Gemifiable {
    identifier: string,
    displayName: string,
    srcIcon: string,
    context: any
    module: typeof NgModule,    
    annotations: any,
    thumbnail: typeof Component,
    index: typeof Component,
    
}

export abstract class AbstractCloudGemIndexComponent {
    abstract context: any;
}

export abstract class AbstractCloudGemThumbnailComponent implements Tackable {
    abstract context: any;
    abstract displayName: string;
    abstract srcIcon: string;
    abstract state: Stateable;
    abstract metric: Measurable;
}

