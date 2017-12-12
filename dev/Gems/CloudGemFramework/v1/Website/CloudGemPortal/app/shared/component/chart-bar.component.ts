import { Component, OnInit, ViewContainerRef, Input } from '@angular/core';

declare var System: any

@Component({
    selector: 'barchart',
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
    `]
})
export class BarChartComponent {
    @Input() width: number = 900;
    @Input() height: number = 500;
    @Input() data: any = new Array<any>();
    @Input() title: string;
    @Input('x-axis-label') xaxisLabel: string = "";
    @Input('y-axis-label') yaxisLabel: string = "";
    @Input('no-yaxis-labels') noYLabels: boolean;

    private _chartWidth: number
    private _chartHeight: number
    private _perThousands: boolean
    private _margin: any
    private _d3: any;

    private loading: any;

    constructor(private viewContainerRef: ViewContainerRef) {

    }

    ngOnInit() {
        this.loading = true;
        System.import('https://m.media-amazon.com/images/G/01/cloudcanvas/d3/d3.min._V506499212_.js').then((lib) => {
            this.loading = false;
            this._d3 = lib            
            this.harmonize(this.data)
            this.setSize(this.data)
            let graph = this.draw(this.data)
            this.addLabels(graph)
        })
    }

    svg() {
        let elem = this.viewContainerRef.element.nativeElement;
        return this._d3.select(elem).select("svg")
    }

    chartLayer() {
        return this.svg().append("g").classed("chartLayer", true)
    }

    harmonize(data: any) {
        let maxDigits = this._d3.max(data, function (d) { return d.value; }).toString().length

        if (maxDigits > 6) {
            this._perThousands = true;
            let numberOfThousands = maxDigits / 3
            data.forEach(function (d) {
                d.value = d.value / (1000 * (maxDigits - 1));
            });
        }
    }

    setSize(data: Array<any>) {
        this._margin = { top: 30, left: 40, bottom: 40, right: 40 }

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

    draw(data: Array<any>) {
        let w = this._chartWidth
        let h = this._chartHeight

        // set the ranges
        var x = this._d3.scaleBand()
            .domain(data.map(function (d) { return d.label; }))
            .range([0, w])
            .padding(0.2);
        var y = this._d3.scaleLinear()
            .domain([0, this._d3.max(data, function (d) { return d.value; })])
            .range([h, 0]);

        // append the rectangles for the bar chart
        this.chartLayer().selectAll("g")
            .data(data)
            .enter().append("rect")
            .attr("class", "bar")
            .attr("x", function (d) { return x(d.label); })
            .attr("y", function (d) { return y(d.value); })
            .attr("width", x.bandwidth())
            .attr("height", function (d) { return h - y(d.value); })
            .attr("fill", function (d, i) { return d.color ? d.color : "#6441A5" })

        // text label for the bars
        this.chartLayer().selectAll(".text")
            .data(data)
            .enter()
            .append("text")
            .attr("class", "label")
            .attr("x", (function (d) { return x(d.label) + (x.bandwidth() / 2) }))
            .attr("y", function (d) { return y(d.value) - 10; })
            .attr("text-anchor", "middle")            
            .style("font-size", "10px")
            .text(function (d) { return d.value; });  

        // x-Axis
        this.chartLayer().append("g")
            .attr("transform", "translate(0," + h + ")")
            .call(this._d3.axisBottom(x));

        // y-Axis
        if (!this.noYLabels) {
            this.chartLayer().append("g")
                .call(this._d3.axisLeft(y));
        }


        return this.chartLayer();
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
        if (!this.noYLabels) {
            graph.append("text")
                .attr("transform", "rotate(-90)")
                .attr("y", 0 - 70)
                .attr("x", 0 - (this._chartHeight / 2))
                .attr("dy", "1em")
                .style("text-anchor", "middle")
                .style("font-size", "12px")
                .text(this._perThousands ? this.yaxisLabel + " (Thousands)" : this.yaxisLabel);
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