import { Component, Input, Output, ViewEncapsulation, EventEmitter } from '@angular/core';

@Component({
    selector: 'tag',
    template: `
        <label [ngClass]="tagName == 'Show all tags' || tagName == 'Hide' ? 'badge badge-info' : 'badge badge-primary'" isEditing="isEditing"> 
            {{tagName}}
            <span *ngIf="isEditing == 'true'">
                <i class="fa fa-times" data-toggle="tooltip" data-placement="top" title="Remove" (click)="deleteTag(tag)"></i>
            </span>
        </label>
    `,
    styleUrls: ['./app/shared/component/tag.component.css']
})
export class TagComponent {
    @Input() tagName: string;
    @Input() isEditing: boolean;
    @Input() typeName: string;
    @Output() delete = new EventEmitter<any>();

    public ngOnInit() {
        let s = this.typeName;
    }

    public deleteTag(tag): void {
        this.delete.emit();
    }
}
