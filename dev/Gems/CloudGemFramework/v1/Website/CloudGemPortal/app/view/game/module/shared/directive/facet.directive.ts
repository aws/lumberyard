import { Directive, ViewContainerRef } from '@angular/core';

@Directive({
  selector: '[facet-host]',
})
export class FacetDirective {
  constructor(public viewContainerRef: ViewContainerRef) { }
}
