import { Component, OnInit, ViewContainerRef, Input } from '@angular/core';
import * as d3 from "d3";
import legend from "d3-svg-legend";

declare var System: any

@Component({
    selector: 'linechart',
    template: `
        <div class="row">
            <div class="col-12">
                <loading-spinner *ngIf="loading" [size]="'sm'"></loading-spinner>
                <svg class="pad"></svg>
            </div>
        </div>
    `,
    styles: [`
        .pad{
           padding-left: 75px;
           padding-top: 30px;
        }
        /* Allow negative svg content to be visible */
        svg {
            overflow: visible;
        }
    `]
})
export class LineChartComponent {
    @Input() width: number = 900;
    @Input() height: number = 500;
    @Input() dataArray: any = new Array<Array<any>>();
    @Input() labelArray: any = new Array<string>();
    @Input('is-left-yaxises') isLeftYaxises: any = new Array<boolean>();
    @Input('has-right-yaxis') hasRightYAxis: boolean = false;
    @Input('is-xaxis-unit-time') isXaxisUnitTime: boolean = false;
    @Input() title: string;
    @Input('x-axis-label') xaxisLabel: string = "";
    @Input('left-y-axis-label') leftYaxisLabel: string = "";
    @Input('right-y-axis-label') rightYaxisLabel: string = "";

    private _chartWidth: number
    private _chartHeight: number
    private _margin: any
    private _d3: any;
    private _legend: any;

    private loading: any;

    constructor(private viewContainerRef: ViewContainerRef) {

    }

    ngOnInit() {
        this.loading = false;
        this._d3 = d3;
        this._legend = legend;
        this.setSize();
        let graph = this.draw();
        this.addLabels(graph);
    }

    svg() {
        let elem = this.viewContainerRef.element.nativeElement;
        return this._d3.select(elem).select("svg")
    }

    chartLayer() {
        return this.svg().append("g").classed("chartLayer", true)
    }

    setSize() {
        this._margin = { top: 30, left: 40, bottom: 40, right: 260 }

        this._chartWidth = this.width - (this._margin.left + this._margin.right)
        this._chartHeight = this.height - (this._margin.top + this._margin.bottom)

        this.svg().attr("width", this.width).attr("height", this.height)

        this.chartLayer()
            .attr("width", this._chartWidth)
            .attr("height", this._chartHeight)
            .append("g")
            .attr("transform", "translate(" + [this._margin.left, this._margin.top] + ")")

        this.svg().attr("align", "center").attr("vertical-align", "middle");
    }

    addLegend() {
        let ordinal = this._d3.scaleOrdinal(this._d3.schemeCategory20)
            .domain(this.labelArray);

        this.svg().append("g")
            .attr("class", "legendOrdinal")
            .attr("transform", "translate(" + (this._chartWidth+60) + ",0)");

        var legendOrdinal = this._legend.legendColor()
            .shapeWidth(5)
            .scale(ordinal);

        this.svg().select(".legendOrdinal")
            .call(legendOrdinal);
    }

    draw() {
        let w = this._chartWidth;
        let h = this._chartHeight;

        let x;
        if (!this.isXaxisUnitTime) {
            x = this._d3.scaleLinear().range([0, w]);
        } else {
            x = this._d3.scaleTime().range([0, w]);
        }

        let left_y = this._d3.scaleLinear().range([h, 0]);
        let right_y;
        if (this.hasRightYAxis) {
            right_y = this._d3.scaleLinear().range([h, 0]);
        }

        let line_left_yaxis = this._d3.line()
            .x(function (d) {
                return x(d.label);
            })
            .y(function (d) {
                return left_y(d.value);
            });

        let line_right_yaxis;
        if (this.hasRightYAxis) {
            line_right_yaxis = this._d3.line()
                .x(function (d) {
                    return x(d.label);
                })
                .y(function (d) {
                    return right_y(d.value);
                });
        }

        // scale the range of the data
        let minMaxX = this.findMinMaxX(this.dataArray);
        let x_min = minMaxX[0];
        let x_max = minMaxX[1];

        x.domain([x_min, x_max]);

        if (this.hasRightYAxis) {
            let leftRightYMax = this.findMaxLeftRightY(this.dataArray);
            let left_y_max = leftRightYMax[0];
            let right_y_max = leftRightYMax[1];

            left_y.domain([0, left_y_max + left_y_max / 10]);
            right_y.domain([0, right_y_max + right_y_max / 10]);
        } else {
            let left_y_max = this.findMaxLeftY(this.dataArray);
            left_y.domain([0, left_y_max + left_y_max / 10]);
        }

        // add x-Axis
        this.chartLayer().append("g")
            .attr("transform", "translate(0," + h + ")")
            .call(this._d3.axisBottom(x));

        // add lines
        let color = this._d3.scaleOrdinal(this._d3.schemeCategory20);
        if (this.hasRightYAxis) {
            for (let i = 0; i < this.dataArray.length; i++) {
                let data = this.dataArray[i];
                if (this.isLeftYaxises[i]) {
                    this.addLine(data, line_left_yaxis, left_y, color(i));
                } else {
                    this.addLine(data, line_right_yaxis, right_y, color(i));
                }
            }
        } else {
            for (let i = 0; i < this.dataArray.length; i++) {
                let data = this.dataArray[i];
                this.addLine(data, line_left_yaxis, left_y, color(i));
            }
        }

        // add y-Axis
        this.chartLayer().append("g")
            .call(this._d3.axisLeft(left_y));

        if (this.hasRightYAxis) {
            this.chartLayer().append("g")
                .attr("transform", "translate(" + (w) + ",0)")
                .call(this._d3.axisRight(right_y));
        }

        // add legend
        this.addLegend();

        return this.chartLayer();
    }

    private findMinMaxX(dataArray: Array<Array<any>>): Array<number> {
        let x_min = null;
        let x_max = null;
        for (let data of dataArray) {
            for (let point of data) {
                if (x_min == null) {
                    x_min = point.label;
                } else {
                    if (point.label < x_min) {
                        x_min = point.label;
                    }
                }

                if (x_max == null) {
                    x_max = point.label;
                } else {
                    if (point.label > x_max) {
                        x_max = point.label;
                    }
                }
            }
        }

        return [x_min, x_max];
    }

    private findMaxLeftRightY(dataArray: Array<Array<any>>): Array<number> {
        let left_y_max = null;
        dataArray.forEach((data, i) => {
            if (this.isLeftYaxises[i]) {
                data.forEach(point => {
                    if (left_y_max == null) {
                        left_y_max = point.value;
                    } else {
                        if (point.value > left_y_max) {
                            left_y_max = point.value;
                        }
                    }
                });
            }
        });

        let right_y_max = null;
        dataArray.forEach((data, i) => {
            if (!this.isLeftYaxises[i]) {
                data.forEach((point) => {
                    if (right_y_max == null) {
                        right_y_max = point.value;
                    } else {
                        if (point.value > right_y_max) {
                            right_y_max = point.value;
                        }
                    }
                });
            }
        });

        return [left_y_max, right_y_max];
    }

    private findMaxLeftY(dataArray: Array<Array<any>>): number {
        let left_y_max = null;
        dataArray.forEach(data => {
            data.forEach(point => {
                if (left_y_max == null) {
                    left_y_max = point.value;
                } else {
                    if (point.value > left_y_max) {
                        left_y_max = point.value;
                    }
                }
            });
        });

        return left_y_max;
    }

    private addLine(data: any, line: any, y: any, color: any): void {
        if (data.length == 0) {
            return;
        }

        this.svg().append("path")
            .datum(data)
            .attr("fill", "none")
            .style("stroke", color)
            .attr("d", line);
    }

    addLabels(graph: any) {
        // text label for the x-axis
        graph.append("text")
            .attr("x", (this._chartWidth / 2))
            .attr("y", this._chartHeight + this._margin.bottom - 5)
            .attr("text-anchor", "middle")
            .style("font-size", "12px")
            .text(this.xaxisLabel);

        // text label for the y axis
        if (this.leftYaxisLabel) {
            graph.append("text")
                .attr("transform", "rotate(-90)")
                .attr("y", 0 - 70)
                .attr("x", 0 - (this._chartHeight / 2))
                .attr("dy", "1em")
                .style("text-anchor", "middle")
                .style("font-size", "12px")
                .text(this.leftYaxisLabel);
        }

        if (this.rightYaxisLabel) {
            graph.append("text")
                .attr("transform", "rotate(-90)")
                .attr("y", this._chartWidth + 55)
                .attr("x", 0 - (this._chartHeight / 2))
                .attr("dy", "1em")
                .style("text-anchor", "middle")
                .style("font-size", "12px")
                .text(this.rightYaxisLabel);
        }

        // text label for the title
        graph.append("text")
            .attr("class", "label")
            .attr("x", (this._chartWidth / 2))
            .attr("y", 0 - (this._margin.top / 2))
            .attr("text-anchor", "middle")
            .style("font-size", "12px")
            .style("font-weight", "bold")
            .style("font-family", "AmazonEmber-Regular")
            .text(this.title);
    }

}