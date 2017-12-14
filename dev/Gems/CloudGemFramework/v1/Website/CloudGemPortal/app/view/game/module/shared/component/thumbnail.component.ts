import { Component, OnInit, Input, ViewEncapsulation } from '@angular/core';

@Component({
    selector: 'thumbnail',
    template: `
    <a [routerLink]="url" class="card gem-thumbnail">
        <div class="thumbnail-image">
            <img [src]="icon" />
        </div>
        <div class="card-block">
            <ng-content select=".thumbnail-label-section"> </ng-content>
            <div class="label-expanded">
                <ng-content select=".expanded-section"></ng-content>
            </div>
        </div>
    </a>
    `,
    styleUrls: ['app/view/game/module/shared/component/thumbnail.component.css'],
    encapsulation: ViewEncapsulation.None

})
export class ThumbnailComponent implements OnInit {
    @Input() icon?: string;
    @Input() url?: string;
    constructor() { }
    ngOnInit() {
    }
}