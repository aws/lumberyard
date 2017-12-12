import { Component, ViewContainerRef, OnInit, Input } from '@angular/core';

declare var System: any

@Component({
    selector: 'piechart',
    template: `        
        <div class="row">    
            <div class="col-12">                
                <loading-spinner *ngIf="loading" [size]="'sm'"></loading-spinner>
                <svg id="" class="pad"></svg>
            </div>           
        </div>        
    `,
    styles: [`
        .pad{
            padding-top: 30px;
            padding-left: 40px;
        }
    `]
})
export class PieChartComponent {
    @Input() width: number = 900;
    @Input() height: number = 500;
    @Input() data: any = new Array<any>();
    @Input() title: string = ""
    
    private _chartWidth: number
    private _chartHeight: number
    private _margin: any
    private _d3: any;

    private loading: any;

    constructor(private viewContainerRef: ViewContainerRef) {

    }

    ngOnInit() {
        this.loading = true;
        System.import('https://m.media-amazon.com/images/G/01/cloudcanvas/d3/d3.min._V506499212_.js').then((lib) => {
            this._d3 = lib
            this.loading = false;
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
    
    setSize(data: Array<any>) {
        this._margin = { top: 40, left: 0, bottom: 40, right: 0 }

        this._chartWidth = this.width - (this._margin.left + this._margin.right)
        this._chartHeight = this.height - (this._margin.top + this._margin.bottom)

        this.svg().attr("width", this.width).attr("height", this.height)
        
        this.chartLayer()
            .attr("width", this._chartWidth)
            .attr("height", this._chartHeight)
            .attr("transform", "translate(" + [this._margin.left, this._margin.top] + ")")

        this.svg().attr("align", "center").attr("vertical-align", "middle"); 
    }

    draw(data: Array<any>): any {        
        let radius = Math.min(this._chartWidth, this._chartHeight) / 2;
        let color = this._d3.scaleOrdinal(["#6441A5", "#F29C33", "#67538c", "#FFFFFF", "#415fa5", " #f8f8f8", "#6441A5", "#BBBBBB", "#6441a5", "#9641a5"]);

        let arcs = this._d3.pie()
            .sort(null)
            .value(function (d) { return d.value; })
            (data)
                
        let arc = this._d3.arc()
            .outerRadius(radius)
            .innerRadius(5)
            .padAngle(0.03)
            .cornerRadius(8)



        var pieG = this.chartLayer().selectAll("g")
            .data([data])
            .enter()
            .append("g")
            .attr("transform", "translate(" + [this._chartWidth / 2, this._chartHeight / 2] + ")")

        let block = pieG.selectAll(".arc")
            .data(arcs)

        var graph = block.enter().append("g").classed("arc", true)
                
        graph.append("path")
            .attr("d", arc)
            .attr("id", function (d, i) { return "arc-" + i })
            .attr("stroke", "#999999")
            .attr("fill", function (d, i) { return d.data.color ? d.data.color : color(i) })       

        return graph         

    }

    addLabels(graph:any) {
        let radius = Math.min(this._chartWidth, this._chartHeight) / 2;
        let labelArc = this._d3.arc()
            .outerRadius(radius)
            .innerRadius(radius);

        // text label for the title
        graph.append("text")
            .attr("class", "label")
            .attr("x", 0)
            .attr("y", 0 - (radius + 10))
            .attr("text-anchor", "middle")
            .style("font-size", "12px")              
            .style("font-family", "AmazonEmber-Regular")             
            .text(this.title);

        // text labels
        graph.append("text").attr("transform", (d: any) => "translate(" + labelArc.centroid(d) + ")")
            .attr("dy", "10px")
            .attr("dx", "-20px")
            .attr("xlink:href", function (d, i) { return "#arc-" + i; })            
            .style("font-size", "12px")
            .text((d: any) => d.data.label);

    }

}