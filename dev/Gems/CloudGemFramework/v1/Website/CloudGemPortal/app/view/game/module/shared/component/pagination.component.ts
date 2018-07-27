import {Component, Input, ViewEncapsulation, Output, EventEmitter} from '@angular/core';

export type paginationType = 'Numbers' | 'Token';

@Component({
    selector: 'pagination',
    template: `
    <ng-container [ngSwitch]="type">
        <ng-container *ngSwitchCase="'Token'">
            <nav aria-label="Page navigation">
                <ul class="pagination">
                    <li class="page-item">
                        <a class="page-link" (click)="prevPage(currentPageIndex)" *ngIf="showPrevious">
                            Previous Page
                        </a> 
                    </li>
                    <li class="page-item">
                        <a class="page-link" (click)="nextPage(currentPageIndex)" *ngIf="showNext">
                            Next Page
                        </a>
                    </li>
                </ul>
            </nav>
        </ng-container>
        <ng-container *ngSwitchDefault>
            <nav aria-label="Page navigation">
                <ul class="pagination" *ngIf="pages > 1">
                    <li *ngFor="let index of range(pages)" class="page-item">
                            <a class="page-link" 
                                [class.page-active]="index == currentPage"
                                (click)="pageFn(index)"> {{index}} </a>
                    </li>
                </ul>
                <pre>{{'Total pages: ' + pages | devonly}}</pre>
                <pre>{{'Current page index: ' + currentPage | devonly}}</pre>
            </nav>
        </ng-container>
    </ng-container>
    `,
    styleUrls: ['app/view/game/module/shared/component/pagination.component.css'],
    encapsulation: ViewEncapsulation.None   
})
export class PaginationComponent {
    @Input() type?: paginationType
    @Input() pages?: number;
    @Input() showNext?: string;
    @Input() showPrevious?: string;
    @Input() startingPage?: number = 1;
    @Output() pageChanged = new EventEmitter<number>();

    // The current page of the pagination component.  
    // Should be the number of the page and not the index so 1 is the first page, not 0
    currentPage: number;

    constructor() {        
    }

    ngOnInit() {
        this.currentPage = this.startingPage;
    }

    pageFn = function (pageNum) {
        this.currentPage = pageNum;
        this.pageChanged.emit(this.currentPage);
    }

    reset() {
        this.pageFn(1);
    }

    range = (value) => {
        let a = [];
        for (let i = 0; i < value; ++i) {
            a.push(i + 1)
        }
        return a;
    }

    nextPage() {
        this.pageChanged.emit(1);
    }

    prevPage() {
        this.pageChanged.emit(-1);
    }

}
