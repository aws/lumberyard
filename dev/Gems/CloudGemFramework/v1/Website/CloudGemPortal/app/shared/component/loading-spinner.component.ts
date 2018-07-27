import { Component, Input, ViewEncapsulation } from '@angular/core';

@Component({
    selector: 'loading-spinner',
    template: `
        <div class="text-center loading-spinner-container" [ngSwitch]="size">
            <div *ngSwitchCase="'lg'">
                <img src="https://m.media-amazon.com/images/G/01/cloudcanvas/images/beaver_flat_animation_optimized_32._V518427760_.gif" />
                <p class="text-center">
                    Loading...
                </p>
            </div>
            <div *ngIf="text" class="loading-text">
                {{ text }}
            </div>
            <div *ngSwitchDefault class="small-loading-icon">
                <i class="fa fa-cog fa-spin fa-2x fa-fw"></i>
                <span class="sr-only">Loading...</span>
            </div>
        </div>
    `,
    styles: [`
        /* Loading spinner */

        loading-spinner {
            width: 100%;
            text-align: center;
        }
        .small-loading-icon{
            width: 125px !important;
            height: 97.22px !important;
            background-size: 125px 97.22px !important;
            margin: 25px auto;
        }
    `],
    encapsulation: ViewEncapsulation.None
})
export class LoadingSpinnerComponent {
    @Input() size: 'sm' | 'lg' = 'sm';
    // Optional text to display before the loading icon.
    @Input() text?: "";
    constructor() { }
}
