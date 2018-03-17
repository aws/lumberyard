import { Component, OnInit, Input, Output, EventEmitter } from '@angular/core';

/**
 * Dropdown component
 * Wrapper for the ng-bootstrap dropdown component
 */

@Component({
    selector: 'dropdown',
    template: `
        <div *ngIf="options" class="d-inline-block" ngbDropdown>
            <button type="button" class="btn search-dropdown l-dropdown" ngbDropdownToggle>
                <span> {{ dropdownText }} </span>
                <i class="fa fa-caret-down" aria-hidden="true"></i>
            </button>
            <div class="dropdown-menu" aria-labelledby="priority-dropdown">
                <button (click)="changedDropdown(option)" type="button" *ngFor="let option of options" class="dropdown-item">
                    {{ option.text }}
                </button>
            </div>
        </div>
    `
})

export class DropdownComponent implements OnInit {
    // Dropdown option hash.  Contains the text and the callback function to be used when the text input is used.
    // Optionally args can be passed in to the callback function.  Otherwise the text of the dropdown will be returned
    @Input() options: [{ text: string, functionCb?: Function, args?: any }]
    // Placeholder text for the search dropdown.  If not specified the searchDropdownOption[0] dropdown name will be used instead.
    @Input() placeholderText: string;

    // Optional event to listen to that returns the current dropdown option whenever it's changed.
    @Output() dropdownChanged = new EventEmitter<string>();

    private currentOption: { text: string, functionCb?: Function, args?: any };
    private dropdownText;
    constructor() { }

    ngOnInit() {
        // If the first option doesn't exist at all then this isn't a functional dropdown so just return.
        if (this.options === undefined || this.options.length == 0 || this.options[0] === null || this.options[0] === undefined) {
            return;
        }

        if (this.placeholderText) {
            this.dropdownText = this.placeholderText;
            this.currentOption = null;
        } else {
           this.dropdownText = this.options[0].text;

        }
        if (this.options) {
            this.currentOption = this.options[0];
        }
    }

    /**
     * Runs whenever the dropdown option is changed.  Bound to a click event in the tempalte.
     * @param option
     */
    changedDropdown(option) {
        this.dropdownChanged.emit(option);
        this.currentOption = option;
        this.dropdownText = option.text;
        if (option.functionCb) {
            option.functionCb(option.args);
        }
    }
}