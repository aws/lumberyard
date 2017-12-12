import {Component, Input, ViewEncapsulation, Output, EventEmitter} from '@angular/core';

@Component({
    selector: 'pagination',
    template: `
    <nav aria-label="Page navigation">
        <ul class="pagination" *ngIf="pages > 1">
            <li *ngFor="let index of range(pages)" class="page-item">
                    <a class="page-link" 
                        [class.page-active]="index == currentPage"
                        (click)="pageFn(index)"> {{index}} </a>
            </li>
        </ul>
        <pre>{{'Total pages: ' + pages | devonly}}</pre>
        <pre>{{'Current page index: ' + currentIndex | devonly}}</pre>
    </nav>
    `,
    styleUrls: ['app/view/game/module/shared/component/pagination.component.css'],
    encapsulation: ViewEncapsulation.None   
})
export class PaginationComponent {
    @Input() pages: number;
    @Output() pageChanged = new EventEmitter<number>();

    // The current page of the pagination component.  
    // Should be the number of the page and not the index so 1 is the first page, not 0
    currentPage: number;

    constructor() {        
    }

    ngOnInit() {
        this.currentPage = 1
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

}
