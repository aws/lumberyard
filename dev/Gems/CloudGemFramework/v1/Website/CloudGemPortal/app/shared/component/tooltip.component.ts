import { Component, Input, ViewEncapsulation } from '@angular/core';

@Component({
    selector: 'tooltip',
    template: `<i class="fa fa-question-circle-o" [placement]="placement"
    [ngbTooltip]="tooltip"></i>`,
    styles: [`
        i {
            margin-left: 10px;
            margin-top: 10px;
        }
    `]
})
export class TooltipComponent {
    @Input() tooltip: string;
    @Input() placement: Placement = "bottom";
}

export type Placement = "top" | "bottom" | "left" | "right";
