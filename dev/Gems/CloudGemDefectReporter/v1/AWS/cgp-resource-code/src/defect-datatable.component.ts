import { Component, Input } from '@angular/core';
import { MetricQuery } from 'app/view/game/module/shared/class/index';
import { Observable } from 'rxjs/Observable';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';


@Component({
    selector: 'defect-datatable',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/defect-datatable.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-datatable.component.scss']
})

export class CloudGemDefectReproterDefectDatatableComponent {
    @Input() aws: any;
    @Input() metricApiHandler: any;
    @Input() defectReporterApiHandler: any;
    @Input() partialQuery: string;
    @Input() limit: string;
    @Input() toDefectDetailsPageCallback: Function;
    @Input() bookmark: boolean;

    public loading: boolean;
    public loadingSort: boolean = false;
    private metricQueryBuilder: MetricQuery;
    readonly numberOfRowsPerPage: number = 10;
    private tmpRows = [];
    private partialInputQuery: string = "";
    private manualUpdate = false;
    private rows: any[];
    private columns: any[];
    private allColumns: any[];
    private filteredRows = [];
    private readStatusOption: string = "";


    constructor(private toastr: ToastsManager) {
    }

    ngOnInit() {
        this.metricQueryBuilder = new MetricQuery(this.aws, "defect");
        let s = this.partialQuery;

        this.fetchQueryFromInput(this.partialQuery, false);
    }

    /**
    * Takes input from html textfield, converts partial input into a query and fetches results.
    * @param input  String input from user.
    * @param manualUpdate  Whether the operation is triggered by users
    **/
    public fetchQueryFromInput(input: string, manualUpdate: boolean): void {
        this.partialInputQuery = input;
        let query = this.constructQuery(input);
        this.manualUpdate = manualUpdate;

        this.fetch(query);
    }


    /**
    * Checks if columns and rows are populated with data.
    * @param rows  The rows to check.
    * @returns Boolean that indicates if data is available.
    **/
    public isDataAvailable(rows): boolean {
        this.filteredRows = [...this.filteredRows];

        if (this.allColumns.length > 0 && rows.length > 0) {
            return true;
        } else {
            return false;
        }
    }


    /**
    * Getter for the limit of results per query to Athena.
    * @returns The maximum number of results that can be returned.
    **/
    public getMaximumLimit(): string {
    
        return this.limit;
    }

    /**
    * Update the rows on display according to the bookmark and read status.
    **/
    public updateFilteredRows(): void {
        this.filteredRows = this.bookmark ? this.rows.filter(row => row.bookmark === 1) : this.rows;
        
        if (this.readStatusOption !== "") {
            this.filteredRows = this.filteredRows.filter(
                row => row.report_status === this.readStatusOption);
        }
    }

    /**
    * Takes input string from portal and adds necessary fields to query the Metrics gem.
    * @param input  String input from user.
    * @param orderBY String of SQL column name to use for sorting.
    * @param direction String input defining direction of sort (ascending or decending).
    * @returns String query ready to be sent to AWS Athena.
    **/
    private constructQuery(input:string, orderBy?:string, direction?:string): string {
    
        let isInputEmpty = !input;

        let partialWhereClause = isInputEmpty ? `WHERE p_event_name='defect'` : `WHERE p_event_name='defect' and`;
        let inputWhereClause = isInputEmpty ? "" : input;

        let orderByClause = !orderBy ? "" : `ORDER BY ` + orderBy;
        let directionClause = !direction ? "" : direction;
        let limit = this.getMaximumLimit();
    
        let query = this.metricQueryBuilder.toString("SELECT * FROM ${database}.${table} " + partialWhereClause + ` ` +  inputWhereClause + ` ` + orderByClause + ` ` +  directionClause + ` LIMIT ` + limit);
    
        return query;
    }


    /**
    * Observable wrapper for Metric Api Handler call.
    * @returns Observable of query to subscribe to.
    **/
    private postQuery(query): Observable<any> {
        return this.metricApiHandler.postQuery(query);
    }


    /**
    * Handles retrieval of data from Athena.
    * @param query  String ANSI SQL query.
    **/
    private fetch(query): void {
        this.loading = true;
        this.clearData();

        this.postQuery(query).subscribe(
            response => {
                this.populateData(response, false);
            },
            err => {
                this.toastr.error("Unable to fetch query. Error: ", err);

                this.clearData();
                this.loading = false;
            }
        );
    }

    /**
    * Listener to sort event on table.
    * @param event  Event emitted by ngx-datatable
    **/
    private onSort(event) {
        const columnName = event.column.prop;

        const sort = event.sorts[0];
        let direction = sort.dir === 'desc' ? 'DESC':'ASC';

        this.loadingSort = true;

        if (columnName === 'report_status') {
            this.filteredRows.sort((r1, r2) => {
                let r1Status = r1[columnName] === 'read' ? 1 : 0;
                let r2Status = r2[columnName] === 'read' ? 1 : 0;

                if (r1Status > r2Status) {
                    return direction === 'ASC' ? 1 : -1;
                }

                if (r1Status < r2Status) {
                    return direction === 'ASC' ? -1 : 1;
                }

                return 0;
            });

            this.loadingSort = false;
            return;
        }

        let query = this.constructQuery(this.partialQuery, columnName, direction);
        this.postQuery(query).subscribe(
            response => {
                this.populateData(response, true);

                this.loadingSort = false;
            },
            err=>{
                this.toastr.error("Unable to sort data. Error: ", err);
                
                this.clearData();
                
                this.loadingSort = false;
            }
        )
    }


    /**
    * Toggles a column's visibility in the table.
    * @param col    Column object in column selector.
    **/
    private toggle(col) {
        const isSelected = this.isSelected(col);

        if (isSelected) {
            this.columns = this.columns.filter(c => {
                return c.name !== col.name;
            });
        } else {
            let index = this.allColumns.findIndex(c => {
                return c.prop === col.prop;
            });
            this.columns.splice(index, 0, col);
            this.columns = [...this.columns];
        }
    }


    /**
    * Logic to check when a column is selected in the column selector.
    * @param col    Column object in column selector.
    * @returns  Boolean indicating if the column is selected.
    **/
    private isSelected(col): boolean {
        return this.columns.find(c => {
            return c.name === col.name;
        });
    }


    /**
    * Logic for when to show an entry in the column selector. Takes filtering into account.
    * @param input  String from filter text input field
    * @param columnName String of the column name.
    * @returns Boolean the indicates if the column entry should be shown.
    **/
    private isInColumnSelectorView(input:string, columnName:string): boolean {

        if (!input){
            return true;
        }
        else {
            let isInputInColumnName = columnName.toLowerCase().includes(input.toLowerCase());
            
            return isInputInColumnName ? true : false;
        }
    }


    /**
    * Sets all checkboxes in column selector to selected
    **/
    private selectAllColumns(): void {

        this.columns = this.allColumns.slice();
        this.columns = [...this.columns];
    }

        
    /**
    * Sets all checkboxes in column selector to unselected
    **/
    private clearSelections(): void {

        this.columns = [];
        this.columns = [...this.columns];
    }

    
    /**
    * Function that is called when any activation(ex. mouseOver) ocurrs on a row in the data table
    * @param event  Event emmited on row activation by ngx-datatable.
    **/
    private onRowActivation(event:any): void {
        if (event.type === 'click') {
            let defect = event.row;
            if (event.column.prop !== 'bookmark') {
                this.onShowDefectDetailsPage(defect);
            }
            else {
                defect['bookmark'] = defect['bookmark'] === 0 ? 1 : 0;
                this.saveReportHeader(defect);

                this.updateFilteredRows();
            }
        }
    }    


    /**
    * Use function to call out when handling the onShow click
    * @param defect Defect row data.
    **/
    private onShowDefectDetailsPage = (defect: any) => {   
        if (this.toDefectDetailsPageCallback) {
            this.toDefectDetailsPageCallback(defect);
        }
    }  
    
    /**
    * Checks response from query to confirm the query was succesful since failed queries also return 200
    * @param response   Response object;
    **/
    private isSuccessfulQuery(response): boolean {
        if (!!response.Status && !!response.Status.State)  {
            if (response.Status.State === "SUCCEEDED") {
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }


    /**
    * Populate columns and rows from query response
    * @param response   Response object from query.
    * @param isSort Boolean that indicates if this data came from a sort operation.
    **/
    private populateData(response, isSort?:boolean): void {

        if (!this.isSuccessfulQuery(response)) {
            if (response.Status.StateChangeReason.indexOf("not found") !== -1) {
                this.toastr.error("No data is found in the database.");
            }
            else {
                this.toastr.error("Unable to populate data. Check query format.");
            }
            this.clearData();
            this.loading = false;
            return;
        }

        if (this.manualUpdate) {
            this.onSuccessfulQuery();
        }

        let headers = response.Result.slice(0, 1);

        if (!isSort){        
            this.columns = [];
        }

        this.allColumns = [];

        this.addColumnObject('report_status', isSort);

        for (let header of headers) {
            for (let cell of header) {
                this.addColumnObject(cell, isSort);
            }
        }
        
        this.tmpRows = [];
        let responseRows = response.Result.slice(1, response.Result.length);
        for (let row of responseRows) {
            let rowObject = {};
            row.forEach((element, index) => {
                let headerName = headers[0][index];
                rowObject[headerName] = element;
            });
            rowObject['bookmark'] = 0;
            rowObject['prop'] = row;
            this.tmpRows.push(rowObject);
        }

        this.updateReportHeaders();
    }

    /**
    * Store the search information on success
    **/
    private onSuccessfulQuery(): void {
        let body = { user_id: this.aws.context.authentication.user.username, query_params: this.partialInputQuery };
        this.defectReporterApiHandler.addNewSearch(body).subscribe(
            response => {
            },
            err => {
                this.toastr.error("Failed to save the search. ", err);
            }
        );
    }

    /**
    * Update the report header information
    * @param row The row to check.
    **/
    private updateReportHeaders(): void {
        this.defectReporterApiHandler.getReportHeaders().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let reportHeaders = obj.result;

            let reportUuids = [];
            for (let reportHeader of reportHeaders) {
                reportUuids.push(reportHeader['universal_unique_identifier'])
            }

            this.rows = [];
            for (let row of this.tmpRows) {
                let index = reportUuids.indexOf(row['universal_unique_identifier']);
                let header = index === -1 ? { bookmark: 0, report_status: 'unread' } : reportHeaders[index];
                this.mergeReportProperties(row, header);

                if (index=== -1) {
                    this.saveReportHeader(row);
                }
            }

            this.updateFilteredRows();
            this.loading = false;
        }, err => {
            this.toastr.error("Failed to update the addtional report information. " + err.message);
        });
    }

    /**
    * Save header information of the current report to the DynamoDB table.
    * @param row The row to update.
    **/
    private saveReportHeader(row): void {
        let body = {
            universal_unique_identifier: row["universal_unique_identifier"],
            bookmark: row["bookmark"],
            report_status: row["report_status"]
        }
        this.defectReporterApiHandler.updateReportHeader(body).subscribe(response => {
        }, err => {
            this.toastr.error("Failed to update the report header. " + err.message);
        });
    }

    /**
    * Merge the header information with the current row
    * @param row The row to update.
    * @param header Addtional header information including bookmark and read status
    **/
    private mergeReportProperties(row, header): void {
        for (let key of Object.keys(header)) {
            row[key] = JSON.parse(JSON.stringify(header[key]));
        }

        this.rows.push(row);
    }
    

    /**
    * Clears stored data for columns and rows
    **/
    private clearData(): void {
        this.columns = [];
        this.allColumns = [];
        this.rows = [];
        this.filteredRows = [];
    }

    /**
    * Add a new cell to the columns
    * @param cell The cell to add.
    * @param isSort Boolean that indicates if this data came from a sort operation.
    **/
    private addColumnObject(cell: string, isSort: boolean): void {
        let cellName = cell.toUpperCase();
        let columnObject = {};
        columnObject['name'] = cellName;
        columnObject['prop'] = cell;
        columnObject['sortable'] = true;

        this.allColumns.push(columnObject);

        if (!isSort) {
            this.columns.push(columnObject);
        }
    }
}