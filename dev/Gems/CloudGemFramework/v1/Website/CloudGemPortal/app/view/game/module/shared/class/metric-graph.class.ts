import { Observable } from 'rxjs/Observable';
import { Subscription } from 'rxjs/Subscription';
import { OnInit } from '@angular/core';
import 'rxjs/add/observable/combineLatest'

/**
* parseMetricsLineData
* A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
* and formats it for the ngx-charts graphing library.
* @param series_x_label
* @param series_y_name
* @param dataSeriesLabel
* @param response
*/
export function MetricLineData(series_x_label: string, series_y_name: string, dataSeriesLabel: string, response: any): any {
    let data = response.Result;
    data = data.slice();

    let header = data[0]
    data.splice(0, 1)
    //At least 2 points required to create a line graph.  If this data doesn't exist generate a 0 value for
    //a single data point.
    let result = <any>[]
    if (header.length == 2) {
        result = MapTuple(header, data)
    } else if (header.length == 3) {
        result = MapVector(header, data)
    }
    return result
}

export function MapTuple(header, data): any {
    if (data.length <= 1 || !(data instanceof Array)) {
        return {
            "name": header[0],
            "series": [
                {
                    "name": "Insufficient Data Found",
                    "value": 0
                }
            ]
        }

    }
    return {
        "name": header[0],
        "series": data.map((e) => {
            let value = e[1]
            if (value == undefined)
                value = 0
            return {                
                "name": e[0],
                "value": Number.parseFloat(value)
            }
        })
    }
}

export function MapVector(header, data): any {
    if (data.length <= 1 || !(data instanceof Array)) {
        return {
            "name": header[0],
            "series": [
                {
                    "name": "Insufficient Data Found",
                    "value": 0
                }
            ]
        }

    }
    let map = new Map<string, any>();
    for (let idx in data) {
        let row = data[idx]
        let x = row[0]
        if (!x)
            continue
        let y = row[1]
        let z = row[2]

        if (!map[x])
            map[x] = { "name": x, "series": [] }

        let value = z
        if (value == undefined)
            value = 0

        map[x].series.push({
            "name": y, "value": Number.parseFloat(value)
        })

    }
    let arr = []
    for (let idx in map) {
        let series = map[idx]
        arr.push(series)
    }

    return arr
}

export function MapMatrix(header, data): any {
    if (data.length <= 1 || !(data instanceof Array)) {
        return {
            "name": header[0],
            "series": [
                {
                    "name": "Insufficient Data Found",
                    "value": 0
                }
            ]
        }

    }
    let map = new Map<string, any>();
    for (let idx in data) {
        let row = data[idx]
        let x = row[0]
        if (!x)
            continue

        if (!map[x])
            map[x] = { "name": x, "series": [] }

        for (let idz = 1; idz < header.length; idz++) {
            let name = header[idz]
            let value = row[idz]            
            if (value == undefined)
                value = 0

            map[x].series.push({
                "name": name, "value": Number.parseFloat(value)
            })
        }

    }
    let arr = []
    for (let idx in map) {
        let series = map[idx]
        arr.push(series)
    }

    return arr
}

/**
  * metricsBarData
  * A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
  * and formats it for the ngx-charts graphing library.
  * @param series_x_label
  * @param series_y_name
  * @param dataSeriesLabel
  * @param response
  */
export function MetricBarData(series_x_label: string, series_y_name: string, dataSeriesLabel: string, response: any): any {
    let data = response.Result;
    data = data.slice();

    let header = data[0]
    data.splice(0, 1)
    //At least 2 points required to create a line graph.  If this data doesn't exist generate a 0 value for
    //a single data point.
    if (data.length <= 0) {
        return [{
            "name": "Insufficient Data Found",
            "value": 0
        }]
    }
    return data.map((e) => {
        let value = e[1]
        if (value == undefined)
            value = 0
        return {
            "name": e[0],
            "value": Number.parseFloat(value)
        }
    })

} 

/**
  * ChoroplethData
  * A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
  * and formats it for the ngx-charts graphing library.
  * @param series_x_label
  * @param series_y_name
  * @param dataSeriesLabel
  * @param response
  */
export function ChoroplethData(series_x_label: string, series_y_name: string, dataSeriesLabel: string, response: any): any {
    console.log(response)
} 

/**
 * MetricGraph
 * Model for encapsulating all info associated with a single metrics graph.
 */
export class MetricGraph {
    // Title of the graph
    public readonly title: string;
    // Text label for x Axis of graph
    public readonly xAxisLabel: string = "Date";
    // Text label for y Axis of graph
    public readonly yAxisLabel: string;
    // The label for the data series.  Shows up when hovering on a graph in the popover and in the legend
    public readonly dataSeriesLabels: string[];    
    // Data series variables. Holds the parsed data that's ready to be rendered by ngx-charts
    public dataSeries: Array<Object>;

    public readonly colorScheme: any

    public model: any

    public response: any
    
    // Timer for the interval that keeps repopulating the graph with new data.  Can be used to cancel the interval.
    private readonly _intervalTimer: any;

    private _series_name_x?: string[]

    private _series_name_y?: string[]

    // Mapping function required to map the response data to the graph data
    private readonly _dataMappingFunction: Array<(series_name_x: string, series_name_y: string, series_label: string, response: any)=> any>;

    // Observable for returning graph series data.  Should be JSON data to be processed by #ParseMetricsData
    private _dataSeriesObservable: Array<Observable<any>>;

    private readonly _refreshFrequency?: Number;

    private readonly showLegend: boolean

    private readonly chart_type: string

    private errorMessage = undefined

    private _subscription: Subscription

    private _is_loading: boolean

    private _graphObservables: Observable<any>[];

    get type(): any {
        return this.chart_type
    } 

    get isLoading(): boolean {
        return this._is_loading
    } 

    set isLoading(value:boolean) {
        this._is_loading = value
    } 

    get dataMapFunction(): Array<(series_name_x: string, series_name_y: string, series_label: string, response: any) => any> {
        return this._dataMappingFunction
    }
    
    minDate: Date;
    maxDate: Date;

    constructor(title: string,
        xAxisLabel: string = "Date",
        yAxisLabel: string,
        dataSeriesLabels: string[],        
        dataSeriesObservable: Array<Observable<any>>,
        dataMappingFunction: Array<(series_name_x: string, series_name_y: string, series_label: string, response: any) => any>,                
        chart_type?: string, 
        series_name_x?: string[],
        series_name_y?: string[],         
        color_scheme?: any,
        model?: any,
        refreshFrequency?: Number      
    ) {
        this.title = title;
        this.xAxisLabel = xAxisLabel;
        this.yAxisLabel = yAxisLabel;
        this.dataSeriesLabels = dataSeriesLabels;
        this._dataSeriesObservable = dataSeriesObservable;        
        this._refreshFrequency = (refreshFrequency) ? refreshFrequency : 1800000;
        this._dataMappingFunction = dataMappingFunction;        
        this._series_name_x = series_name_x
        this._series_name_y = series_name_y
        this.minDate = new Date();
        this.maxDate = new Date();
        this.minDate.setHours(this.minDate.getHours() - 12);
        this.maxDate.setHours(this.maxDate.getHours() + 12);
        this.dataSeries = undefined
        this.chart_type = (chart_type) ? chart_type : "ngx-charts-line-chart"
        this.colorScheme = (color_scheme) ? color_scheme : {
            domain: ['#401E5F', '#49226D', '#551a8b', '#642E95', '#7343A0', '#8358AA', '#8054A8', '#8E67B1', '#9C7ABB', '#AA8DC5', '#B8A0CE', '#C6B3D8', '#D4C6E2', '#E2D9EB', '#f8ecd3',
                '#f4e2be', '#f0d8a8', '#edce92', '#e9c57c', '#e5bb66', '#e1b151', '#dea73b', '#da9e25', '#c48e21', '#ae7e1e', '#996e1a', '#835f16', '#6d4f12', '#573f0f', '#412f0b', '#2c2007']
        };
        this.model = model     
        
        if (this.dataSeriesLabels && this.dataSeriesLabels.length > 1)
            this.showLegend = true

        this._graphObservables = this._dataSeriesObservable.map((graphObservable, index) => {
            return graphObservable.map((response) => {
                if (response === undefined)
                    return
                this._is_loading = false
                this.response = 'body' in response && response instanceof Object ? JSON.parse(response.body.text()).result : response

                if ((this.response.Result && this.response.Result.length == 0)
                    || (this.response.Status && (this.response.Status.State == "FAILED" || this.response.Status.State == "CANCELLED")))
                    return
                return this._dataMappingFunction[index](this._series_name_x != undefined ? this._series_name_x[index] : undefined, this._series_name_y != undefined ? this._series_name_y[index] : undefined, this.dataSeriesLabels[index], this.response);
            });
        });

        if (dataSeriesObservable) {
            this.populateGraph();

            this._intervalTimer = setInterval(() => {
                this.refresh();
            }, this._refreshFrequency);
        }
    }

    /**
     * PopulateGraph
     * Adds the data to the graph so it can be rendered.  Will be rendered once the observable returns.
     */
    populateGraph = () => {
        this._is_loading = true        
        this.response = undefined
        // Store all observables in a variable so we can resolve them all together.
        
        let obs = Observable.combineLatest(this._graphObservables)
        this._subscription = obs.subscribe((res) => {
            if (!res || (res instanceof Array && res[0] === undefined))
                return
            if (res[0] instanceof Array)
                this.dataSeries = res[0];
            else
                this.dataSeries = res;
            this._subscription.unsubscribe();
            this._subscription.remove(this._subscription);
            this._subscription.closed;
        }, err => {    
            //retrieve the datasource soon than later as the backend timed out
            this._is_loading = false        
            setTimeout(this.refresh, 10000);
            this.errorMessage = "Please check that the SQL is valid.  Click the edit icon in the right top of this box to edit."
            console.log(err)            
        });
    }

    public clearInterval = () => {
        clearInterval(this._intervalTimer)
    }
    
    public refresh = () => {        
        this.errorMessage = undefined                       
        this.populateGraph();        
    }

}