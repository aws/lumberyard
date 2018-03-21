import { Component, OnInit, Input, Output, EventEmitter } from '@angular/core';
import { FormControl } from '@angular/forms';
import { SearchDropdownOption } from '../class/index';
import { Subscription } from 'rxjs';

import 'rxjs/add/operator/debounceTime';

export class SearchResult {
    public id: string;
    public value: string;
    constructor(id: string, value: string) {
        this.id = id;
        this.value = value;
    }
}

@Component({
    selector: 'search',
    template: `
        <form class="form-inline">
            <div class="form-group">
                <dropdown class="search-dropdown" (dropdownChanged)="changeDropdown($event)" [options]="dropdownOptions" [placeholderText]="dropdownPlaceholderText"> </dropdown>
                <input (keyUp)="emitSearch($event)" [formControl]="searchTextControl" class="form-control search-text" type="text" [placeholder]="searchInputPlaceholder" />
            </div>
        </form>

    `,
    styles: [`
       .search-dropdown {
           margin-right: 15px;
       }
       input.search-text {
            width: 300px;
       }
    `]
})
export class SearchComponent implements OnInit {
    // Dropdown option hash.  Contains the text and the callback function to be used when the text input is used.
    // Optional args can be passed in to the callback function.  Otherwise the text of the dropdown will be returned
    @Input() dropdownOptions: [{ text: string, functionCb: Function, argsCb?: any }]
    // Placeholder text for the search dropdown.  If not specified the searchDropdownOption[0] dropdown name will be used instead.
    @Input() dropdownPlaceholderText: string;
    // Placeholder for the text input.  This placeholder should be relevant for all dropdown cases.
    // If none is specified the input placeholder will be blank
    @Input() searchInputPlaceholder: string = "";
    // Output for when the search input is updated.
    @Output() searchUpdated = new EventEmitter<SearchResult>();

    currentDropdownOption: { text: string, functionCb: Function, argsCb?: any };
    searchTextControl = new FormControl();

    private _searchTextSubscription: Subscription;
    constructor() { }

    ngOnInit() {
        this.searchTextControl.setValue("");
        if (this.dropdownOptions) {
            this.currentDropdownOption = this.dropdownOptions[0];
        }
        // search form control - debounced and emits after a delay
        this._searchTextSubscription = this.searchTextControl.valueChanges.debounceTime(500).subscribe(newSearchText => {
            this.search();
        });
    }

    search() {
        if (this.currentDropdownOption && this.currentDropdownOption.text) {
            this.searchUpdated.emit(new SearchResult(this.currentDropdownOption.text, this.searchTextControl.value));
        } else {
            this.searchUpdated.emit(new SearchResult("", this.searchTextControl.value))
        }
        if (this.currentDropdownOption && this.currentDropdownOption.functionCb) {
            if (this.currentDropdownOption.argsCb) {
                this.currentDropdownOption.functionCb(this.currentDropdownOption.argsCb);
            } else this.currentDropdownOption.functionCb(this.currentDropdownOption.text);
        }
    }

    changeDropdown = function (dropdownOption) {

        let triggerSearch = false;
        if (this.dropdownOption !== this.currentDropdownOption && this.searchTextControl.value === "") {
            triggerSearch = true;
        }
        if (dropdownOption !== this.currentDropdownOption && this.searchTextControl.value) {
            this.searchTextControl.setValue("");
        }

        this.currentDropdownOption = dropdownOption;

        if (triggerSearch) this.search();
    }
    ngOnDestroy() {
        if (this._searchTextSubscription) {
            this._searchTextSubscription.unsubscribe();
        }
    }
}
