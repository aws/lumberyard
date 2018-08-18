import { Directive, ViewContainerRef } from '@angular/core';

@Directive({
    selector: '[anchor-host]',
})
export class AnchorDirective {
    constructor(public viewContainerRef: ViewContainerRef) { }
}