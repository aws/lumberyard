import { Component, Input, ViewEncapsulation } from '@angular/core';

@Component({
    selector: 'info',
    template: `
       <i class="fa fa-question-circle-o" data-toggle="tooltip" data-html="true" data-title="{{tooltip}}"></i>
    `,
    styles: [``]
})
export class InfoComponent {
    @Input() tooltip: string;
}
