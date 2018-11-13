import { Input, Component } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';
import { AwsService } from "app/aws/aws.service";
import { Queries } from './index';
import { Observable } from 'rxjs/Observable';
import { MetricQuery } from 'app/view/game/module/shared/class/index';
import { MetricGraph, MetricBarData } from 'app/view/game/module/shared/class/metric-graph.class';


@Component({
    selector: 'dashboard-tab',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/dashboard-tab.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-list-page.component.css']
})

export class CloudGemDefectReporterDashboardTabComponent {
    @Input() metricApiHandler: any;

    public queryGraph: MetricGraph;
    private metricQueryBuilder: MetricQuery;

    constructor(private aws: AwsService, private metric: LyMetricService) {
    }

    ngOnInit() {
        this.metricQueryBuilder = new MetricQuery(this.aws, "defect");
        this.queryGraph = this.queryLastXHours(24);
    }

    /**
    * Updates the visible graph.
    * @param graph MetricGraph object
    **/
    public updateGraph(graph: MetricGraph): void {
        this.queryGraph = graph;
    }

    /**
    * Creates a graph with data from the last x hours.
    * @param hour Number representing the number of hours to count back from.
    * @returns MetricGraph object with updated labels and query.
    **/
    private queryLastXHours(hour: number): MetricGraph {
        let title = "Number of Defects per Hour";
        let xAxis = "Hours";
        let yAxis = "Number of Defects";

        return new MetricGraph(
            title,
            xAxis,
            yAxis,
            [],
            [this.postQuery(Queries.lastXHours(hour))],
            [MetricBarData],
            "ngx-charts-bar-vertical",
            [],
            []
        );
    }


    /**
    * Creates a graph with data from the last x days.
    * @param day Number that represents the number of days to count back from.
    * @returns MetricGraph object with updated labels and query.
    **/
    private queryLastXDays(day: number): MetricGraph {
        let title = "Number of Defects per Day";
        let xAxis = "Days";
        let yAxis = "Number of Defects";

        return new MetricGraph(
            title,
            xAxis,
            yAxis,
            [],
            [this.postQuery(Queries.lastXDays(day))],
            [MetricBarData],
            "ngx-charts-bar-vertical",
            [],
            []
        );
    }


    /**
    * Creates graph with data from the last x months.
    * @param month Number that represents the number of months to count back from.
    * @returns MetricGraph object with updated labels and query.
    **/
    private queryLastXMonths(month: number): MetricGraph {
        let title = "Number of Defects per Month";
        let xAxis = "Months";
        let yAxis = "Number of Defects";

        return new MetricGraph(
            title,
            xAxis,
            yAxis,
            [],
            [this.postQuery(Queries.lastXMonths(month))],
            [MetricBarData],
            "ngx-charts-bar-vertical",
            [],
            []
        );
    }


    /**
    * Observable wrapper for metric api handler call to post query.
    * @param query ANSI SQL string without table programmatically populated.
    **/
    private postQuery(query: string): Observable<any> {
        return this.metricApiHandler.postQuery(this.metricQueryBuilder.toString(query));
    }
}