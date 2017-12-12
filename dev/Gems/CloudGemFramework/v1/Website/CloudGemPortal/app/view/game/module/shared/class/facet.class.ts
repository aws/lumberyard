import { Type } from '@angular/core';
import { RestApiExplorerComponent, CloudWatchLogComponent, MetricComponent } from '../component/index'

export class Facet {
    //highest priority is 0, the higher the number the lower priority    
    constructor(public title: string, public component: Type<any>, public order: number, public constraints : Array<string>, public data: any = {}) { }
}

export class FacetDefinitions {
    public static defined = [
        new Facet("REST Explorer", RestApiExplorerComponent, 0, ["ServiceUrl"]),
        new Facet("Log", CloudWatchLogComponent, 1, ["physicalResourceId"])     
    ]
}
