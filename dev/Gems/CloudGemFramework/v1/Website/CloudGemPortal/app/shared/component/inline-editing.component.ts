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
            <div *ngSwitchCase="'tags'" [ngClass]="isEditing ? 'editable-content edit-mode' : 'editable-content view-mode'" (click)="editContent()">
                <span [ngSwitch]="isEditing">
                    <span *ngSwitchCase="false">
                        <tag *ngFor="let tag of this.tagsInDisplayMode" [tagName]="tag" (click)="changeDisplayOption(tag)"></tag>
                    </span>
                    <span *ngSwitchCase="true">
                        <span *ngFor="let tag of content let i = index">
                            <tag *ngIf="i < content.length - 1 || (tag != '' && newTagContent == '')" [tagName]="tag" [isEditing]="isEditing ? 'true' : 'false'" (delete)="deleteTag(tag)"></tag>
                        </span>                 
                    </span>
                </span>
                <input #newTag type="text" [ngClass]="content.length == 0 || isEditing ? 'new-tag' : 'new-tag-hide'" [ngModel]="newTagContent" (ngModelChange)="editNewTag($event)" (keyup.enter)="addNewTag()" (keyup.backspace)="deleteLastTag()"> 
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
    private newTagContent: string = "";

    @ViewChild('newTag') newTagInputRef: ElementRef;
    @ViewChild('inputField') inputFieldRef: ElementRef;

    public ngOnChanges(change: any) {
        if (this.type == "tags") {
            if (change.isEditing && !change.isEditing.currentValue) {
                this.generateTagsInDisplayMode();
            }
            else {
                this.newTagContent = "";
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

    public editNewTag($event): void {
        if (this.newTagContent == "" && $event != "") {
            this.content.push($event);
        }
        else {           
            this.content[this.content.length - 1] = $event;
        }
        this.newTagContent = $event;
    }

    public addNewTag(): void {
        this.newTagContent = "";
    }

    public deleteLastTag(): void {
        if (this.newTagContent == "") {
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
