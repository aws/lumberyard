import { Component, Input, Output, ViewEncapsulation, EventEmitter } from '@angular/core';
import { ViewChild } from '@angular/core';
import {ElementRef} from '@angular/core';

@Component({
    selector: 'inline-editing',
    template: `
        <div [ngSwitch]="type">
            <div *ngSwitchCase="'text'">
                <textarea #inputField [ngClass]="isEditing ? 'editable-content edit-mode' : 'editable-content view-mode'" [(ngModel)]="content" (ngModelChange)="change()" (mousedown)="editContent()"></textarea>
            </div>
            <div *ngSwitchCase="'tags'" [ngClass]="isEditing ? 'edit-mode' : 'editable-content'" (click)="editContent()">
                <span [ngSwitch]="isEditing">
                    <span *ngSwitchCase="false">
                        <tag *ngFor="let tag of this.tagsInDisplayMode" [tagName]="tag" (click)="changeDisplayOption(tag)"></tag>
                    </span>
                    <span *ngSwitchCase="true">
                        <tag *ngFor="let tag of this.content" [tagName]="tag" [isEditing]="isEditing ? 'true' : 'false'" (delete)="deleteTag(tag)"></tag>                   
                    </span>
                </span>
                <input #newTag type="text" [ngClass]="content.length == 0 || isEditing ? 'new-tag' : 'new-tag-hide'" (keyup.enter)="addNewTag()" (keyup.backspace)="deleteLastTag()"> 
            </div>
        </div>
    `,
    styleUrls: ['./app/shared/component/inline-editing.component.css']
})
export class InlineEditingComponent {
    @Input() content: any;
    @Input() type: any;
    @Input() isEditing: boolean;
    @Output() contentChange = new EventEmitter<any>();
    @Output() startEditing = new EventEmitter<any>();

    private firstTimeClick: boolean;
    private originalValue: any;
    private displayMode: boolean;
    private tagsInDisplayMode: string[];

    @ViewChild('newTag') newTagInputRef: ElementRef;
    @ViewChild('inputField') inputFieldRef: ElementRef;

    public ngOnChanges(change: any) {
        if (this.type == "tags") {
            if (change.isEditing && !change.isEditing.currentValue) {
                this.generateTagsInDisplayMode();
            }
        }
        else if (this.type == "text" && this.inputFieldRef) {
            let nativeElement = this.inputFieldRef.nativeElement;
            nativeElement.style.height = this.isEditing ? nativeElement.scrollHeight + "px" : "30px";
        }
    }

    public change(): void {
        this.contentChange.emit(this.content);
    }

    public changeDisplayOption(tag): void {
        if (tag == "Show all tags") {
            this.displayMode = true;
            this.tagsInDisplayMode = JSON.parse(JSON.stringify(this.content));
            this.tagsInDisplayMode.push("Hide");
        }
        else if (tag == "Hide") {
            this.displayMode = true;
            this.addShowAllTag();
        }
    }

    public editContent(): void {
        if (this.type == "text") {
            this.inputFieldRef.nativeElement.focus();
        }
        else if (this.type == "tags") {
            if (this.displayMode) {
                this.displayMode = false;
                return;
            }
            this.newTagInputRef.nativeElement.className = "new-tag";
            this.newTagInputRef.nativeElement.focus();
        }
        this.startEditing.emit();
    }

    public addNewTag(): void {
        if (this.newTagInputRef.nativeElement.value != "") {
            this.content.push(this.newTagInputRef.nativeElement.value);
            this.newTagInputRef.nativeElement.value = "";
        }
    }

    public deleteLastTag(): void {
        if (this.newTagInputRef.nativeElement.value == "") {
            this.content.pop();
        }
    }

    public deleteTag(tag): void {
        var index = this.content.indexOf(tag, 0);
        if (index > -1) {
            this.content.splice(index, 1);
        }
    }

    private generateTagsInDisplayMode(): void {
        this.tagsInDisplayMode = [];
        if (this.content.length <= 4) {
            this.tagsInDisplayMode = JSON.parse(JSON.stringify(this.content));
        }
        else {
            this.addShowAllTag();
        }
    }

    private addShowAllTag(): void {
        this.tagsInDisplayMode = this.content.slice(0, 4);
        this.tagsInDisplayMode.push("Show all tags");
    }
}
