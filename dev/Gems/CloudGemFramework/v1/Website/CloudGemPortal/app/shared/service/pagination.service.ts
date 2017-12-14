import { Injectable } from '@angular/core';

@Injectable()
export class PaginationService {
    get iconClasses() {
        return {
            sortAscending: 'fa fa-sort-asc',
            sortDescending: 'fa fa-sort-desc',
            pagerLeftArrow: 'fa fa-chevron-left',
            pagerRightArrow: 'fa fa-chevron-right',
            pagerPrevious: 'fa fa-step-backward',
            pagerNext: 'fa fa-step-forward'
        }
    };
}

