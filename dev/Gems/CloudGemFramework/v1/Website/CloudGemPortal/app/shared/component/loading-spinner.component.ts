import { Component, Input, ViewEncapsulation } from '@angular/core';

@Component({
    selector: 'loading-spinner',
    template: `
        <div class="text-center">
            <div *ngIf="size == 'lg'">
                <img src="https://m.media-amazon.com/images/G/01/cloudcanvas/beaver_animation._V532414134_.gif" />
            </div>
            <p class="text-center">                    
                Loading...
            </p>
        </div>
    `,
    styles: [`
        /* Loading spinner */
        .loading-spinner-container {
            text-align: center;
        }
        .small-loading-icon{
            width: 125px !important; 
            height: 97.22px !important; 
            background-size: 125px 97.22px !important;
        }
    `]
})
export class LoadingSpinnerComponent {    
    @Input() size: 'sm' | 'lg' = 'sm';
    constructor() { }
}
