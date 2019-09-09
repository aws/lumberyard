import { Input, Component, Output, EventEmitter } from '@angular/core';
import { Observable } from 'rxjs/Observable';

@Component({
    selector: 'field-mapping',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/jira-integration.component.css'],
    template: `
    <div placement="right"
        [ngbTooltip]="schema"
        triggers="click:focusout"
        placement="right">
            <input class="form-control"
                [type]="type"
                placeholder="Type forward search..."
                [ngbTypeahead]="searchReportFields"
                [(ngModel)]="model"
                (ngModelChange)="emit(model)"
                />
    </div>
  `
})

export class CloudGemDefectReporterFieldMappingComponent {
    @Input() model: any;
    @Input() type: string = "text";
    @Input() possibleFields: string[];
    @Output() modelChange = new EventEmitter<any>();

    private _list: string[] = [];

    constructor() { }

    ngOnInit(): void {
        for (let entry of this.possibleFields) {
            if (!entry.startsWith("p_")) {
                this._list.push(`[${entry}]`);
            }
        }
    }

    private emit(model: any) {
        this.modelChange.emit(model)
    }

    /**
  * Typeahead function for searching through report fields
  **/
    searchReportFields = (text$: Observable<string>) =>
        text$.debounceTime(200)
            .distinctUntilChanged()
            .map(term =>
                this._list.filter(v => v.toLocaleLowerCase().indexOf(term.toLocaleLowerCase()) > -1).slice(0, 50));
}