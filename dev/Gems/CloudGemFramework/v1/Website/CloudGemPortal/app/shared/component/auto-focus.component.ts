import { Directive, OnInit, ElementRef } from '@angular/core';

@Directive({
    selector: '[ngautofocus]'
})
export class AutoFocusComponent implements OnInit {

    constructor(private elementRef: ElementRef) {        
    };

    ngOnInit(): void {        
        this.elementRef.nativeElement.focus();
    }

}