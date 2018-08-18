import { Component, OnInit, Input } from '@angular/core';
import { CloudGemMetricApi } from './api-handler.class';
import { AwsService } from 'app/aws/aws.service';
import { Http } from '@angular/http';
import { Observable } from 'rxjs/Observable';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import 'rxjs/add/operator/distinctUntilChanged'

@Component({
    selector: 'metric-filtering',
    template: `
        <ng-template #loading>
            <loading-spinner></loading-spinner>
        </ng-template>
        <div class="filter-section-container">
            <p>
                What events would you like to filter out?  These metrics will not be recorded and cannot be recovered.
            </p>
            <div class="filter-events-container container" *ngIf="!isLoadingFilters; else loading">
                <div class="row labels">
                    <div class="col-2">
                        <label> EVENT NAME </label>
                    </div>
                    <div class="col-2">
                        <label> PLATFORM </label>
                    </div>
                    <div class="col-2">
                        <label> TYPE </label>
                    </div>
                    <div class="col-2" *ngIf="hasAttributes">
                        <label> ATTRIBUTES </label>
                    </div>
                </div>
                <div class="row filters" *ngFor="let filter of filters">
                    <div class="col-2">
                        <input type="text" class="form-control" [ngbTypeahead]="searchEvents" [(ngModel)]="filter.event" />
                    </div>
                    <div class="col-2">
                        <dropdown [options]="platformOptions" (dropdownChanged)="updateFilterPlatform($event, filter)" placeholderText="Platform" [currentOption]="filter.platformDropdownValue" name="dropdown" ></dropdown>
                    </div>
                    <div class="col-2">
                        <dropdown [options]="typeOptions" (dropdownChanged)="updateFilterType($event, filter)" placeholderText="Type" [currentOption]="filter.typeDropdownValue" name="dropdown" ></dropdown>
                    </div>
                    <div class="col-4" class="event-attributes" *ngIf="filter.type === 'Attribute'">
                        <div class="row" *ngFor="let attribute of filter.attributes; let i = index;">
                            <div class="col-6">
                                <input type="text" class="form-control" [(ngModel)]="filter.attributes[i]" />
                            </div>
                            <div *ngIf="i === filter.attributes.length - 1" class="col-6">
                                <button type="button" class="btn btn-outline-primary" (click)="addAttribute(filter)"> + Add Attribute </button>
                            </div>
                            <div *ngIf="filter.attributes.length > 1 && i < filter.attributes.length - 1" class="col-6">
                                <button type="button" class="btn btn-outline-primary" (click)="removeAttribute(filter, attribute)"> - Remove Attribute </button>
                            </div>
                        </div>
                    </div>
                    <i (click)="removeFilter(filter)" class="fa fa-trash-o float-right"> </i>
                </div>
                <div class="row add-filter">
                    <div class="col-lg-2">
                        <button type="button" class="btn btn-outline-primary" (click)="addFilter()"> + Add Filter </button>
                    </div>
                </div>
                <h2> Priority </h2>
                <p> Event priority is used to define what events should take priority when the local game disk space is low.  The threshold for when prioritization takes place can be changed in the Cloud Gem Metric settings facet.  It is labelled as "Prioritization Threshold".  Priority for events goes from highest priority at the top to lowest priority at the bottom.  Events without priortization defined are dropped when local disk space is limited.
                <div class="priority-dragable-container" *ngIf="!isLoadingFilters; else loading">
                    <div class="priority-label-high">
                        <label> Highest Priority </label>
                    </div>
                    <dragable [drop]="onFilterPriorityChanged">
                        <div *ngFor="let filter of filters; let i = index" [id]="filter.event">
                            <action-stub [model]="filter" [text]="filter.event"> </action-stub>
                        </div>
                    </dragable>
                    <div class="priority-label-low">
                        <label class="priority-label-low"> Lowest Priority </label>
                    </div>
                </div>
                <div class="row update-event-filters">
                    <div class="col-lg-2">
                        <button type="submit" (click)="updateFilters(filters)" class="btn l-primary"> Update Filters </button>
                    </div>
                </div>
            </div>
        </div>
    `,
    styles: [`
        .row {
            margin-bottom: 15px;
        }
        .filter-events-container dropdown {
            margin-right: 15px;
        }
        .filter-section-container {
            margin-bottom: 20px;
            max-width: 1000px;
        }
        .filter-priority-container {
            max-width: 1000px;
        }
        .add-filter {
            margin-top: 20px;
            margin-bottom: 30px;
        }
        .update-event-filters {
            margin-bottom: 15px;
        }
        .labels {
            margin-bottom: 10px;
        }
        .row.event-attributes {
            padding: 0;
        }
        .row i {
            line-height: 30px;
        }
        .filters.row button {
            width: 135px;
            text-align: center;
        }
        .priority-dragable-container {
            width: 250px;
            margin-left: 15px;
        }
        .priority-label-high {
            margin-bottom: 10px;
        }
        .priority-label-low {
            margin-top: 10px;
            margin-bottom: 20px;
        }
        .priority-update {
            margin-top: 15px;
        }
    `]

})

export class MetricFilteringComponent implements OnInit {
    @Input() context: any;
    private _apiHandler: CloudGemMetricApi;
    // Dropdown & Typeahead lists
    events = [];
    platformOptions = [{ text: "All" }]
    typeOptions = [
        { text: "All" },
        { text: "Attribute" }
    ]

    isLoadingFilters: boolean = true;

    // Filter lists
    filters: Array<Filter>;
    hasAttributes: boolean = false;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager) { }
    ngOnInit() {
        this._apiHandler = new CloudGemMetricApi(this.context.ServiceUrl, this.http, this.aws);
        // Get any events and platforms from current metrics
        this._apiHandler.getFilterEvents().subscribe((res) => {
            let filterEvents = JSON.parse(res.body.text()).result;
            filterEvents.map( filterEvent => this.events.push(filterEvent) );
        });

        this._apiHandler.getFilterPlatforms().subscribe((res) => {
            let filterPlatforms = JSON.parse(res.body.text()).result;
            filterPlatforms.map((platform) => {
                this.platformOptions.push({ text: platform })
            })
        });

        this.getExistingFilters();
    }

    getExistingFilters = () => {
        // Get any existing filters
        this.filters = new Array<Filter>();
        this._apiHandler.getFilters().subscribe((res) => {
            var filters = JSON.parse(res.body.text()).result.filters;
            filters.map((filter: Filter, index: number) => {
                // If the attributes array is empty add a string so it will register in the template and provide an input to add an attribute
                if (filter.attributes.length > 0) {
                    this.hasAttributes = true;
                }

                // If the filter doesn't have a priority assign one here
                if (!filter.precedence) filter.precedence = index;
                this.filters.push( new Filter(filter.event, filter.platform, filter.type, filter.attributes, filter.precedence));
            });
            this.isLoadingFilters = false;
        })
    }

    /** Add a new filter row locally.  Not saved to the server until #UpdateFilters is called */
    addFilter = () => this.filters.push(Filter.default());

    /** Remove a filter from the filter list locally */
    removeFilter = (filter) => this.filters.splice(this.filters.indexOf(filter), 1);

    /** Add a row to the filter attribute list */
    addAttribute = (filter: Filter) => filter.attributes.push("");

    /** Remove a row from the attributes of a filter */
    removeAttribute = (filter: Filter, attribute: string) => filter.attributes.splice(filter.attributes.indexOf(attribute), 1);

    /** Update the type for a filter when the dropdown input is changed.*/
    updateFilterType = (typeDropdownValue, filter) => {
        filter.type = typeDropdownValue.text
        if (filter.type === 'Attribute') {
            this.hasAttributes = true;
        }
    }

    /* Update the platform value when the dropdown input is changed */
    updateFilterPlatform = (platformDropdownValue, filter) => filter.platform = platformDropdownValue.text

    /** Update the server with the current list of filters */
    updateFilters = (filters: Filter[]) => {
        this.isLoadingFilters = true;
        filters.map((filter) => {
            delete filter.platformDropdownValue;
            delete filter.typeDropdownValue;
            // Comment this block to not allow submission of attribute types with no actual attributes values specified
            filter.attributes.map((attribute) => {
                if (!attribute) delete filter.attributes[filter.attributes.indexOf(attribute)];
            });
        })
        // parse out dropdown field values and add a parent filter property
        let filtersObj = {
            filters: filters
        }
        this._apiHandler.updateFilters(filtersObj).subscribe(() => {
            this.toastr.success("Updated filters");
            this.getExistingFilters();
        }, (err) => {
            this.toastr.error("Unable to update Filters", err);
            this.isLoadingFilters = false;
            console.error(err);
        });
    }

    /** Update the priorities given the ordering of the events in the dragable section */
    onFilterPriorityChanged = (element, targetContainer, subContainer): void => {
        let prioritizedEvents = [];
        // Get filter event name from target container and add it to an ordered list
        for (let filterEle of targetContainer.children) {
            prioritizedEvents.push(filterEle['id']);
        }
        this.filters = this.changeFilterOrdering(prioritizedEvents, this.filters);


    }

    /** Typeahead function for searching through filter events */
    searchEvents = (text$: Observable<string>) =>
        text$.debounceTime(200)
        .distinctUntilChanged()
        .map(term =>
        this.events.filter(v => v.toLocaleLowerCase().indexOf(term.toLocaleLowerCase()) > -1).slice(0, 10));

    changeFilterOrdering(filterEvents: string[], filters: Filter[]): Filter[] {
        let prioritizedFilters = [];
        filterEvents.map((filterEvent) => {
            filters.map(filter => {
                if (filter.event === filterEvent) {
                    prioritizedFilters.push(filter);
                }
            });
        });
        return prioritizedFilters;
    }

}

/** Filter model */
export class Filter {
    event: string;
    platform: string
    platformDropdownValue: { text: string };
    type: string;
    typeDropdownValue: { text: string };
    precedence: number;
    attributes: string[];

    constructor(event: string, platform: string, type: string, attributes: string[], precedence?: number) {
        this.event = event;
        this.platform = platform  ;
        this.attributes = attributes;
        this.precedence = precedence;

        // Type can be lowercased in the DB so check that case here
        if (type === "all") this.type = 'All' ;
        else if (type === "attribute") this.type = "Attribute";
        else this.type = type;

        // assign dropdown object values
        this.platformDropdownValue = { text: this.platform };
        this.typeDropdownValue = { text: this.type };
    }

    static default = () => new Filter( "", "All", "All", [""]);
}