import { Component, Input, Output, ViewEncapsulation, EventEmitter } from '@angular/core';

@Component({
    selector: 'progress-bar',
    template: `
        <span class="progress-border" [style.width.px]="width" [style.height.px]="height">
            <span class="cgp-progress-bar" [class.error-background]="hasError" [class.progress-background]="!hasError" [style.width.%]="percent"></span>
            <span class="progress-percent" [style.line-height.px]="height"> {{percent}}% </span>
        </span>
    `,
    styleUrls: ['./app/shared/component/progress-bar.component.css']
})
export class ProgressBarComponent {
    @Input() width: number;
    @Input() height: number;
    @Input() percent: number;
    @Input() hasError = false;       
}
