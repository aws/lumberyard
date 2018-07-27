import { Component, OnInit, Input, Output, EventEmitter, ViewChild, ComponentFactoryResolver } from '@angular/core';
import { Facet, Facetable, FacetDefinitions } from '../../class/index'
import { FacetDirective } from '../../directive/facet.directive'
import { RestApiExplorerComponent, CloudWatchLogComponent } from '../../component/index'
import { DefinitionService, LyMetricService } from 'app/shared/service/index'

@Component({
    selector: 'facet-generator',
    template: `
    <div class="row subNav">
        <div class="tab"
            [class.tab-active]="model.activeFacetIndex == i"
            *ngFor="let facet of facets; let i = index">
                <a (click)="emitActiveTab(i)"> {{facets[i]}} </a>
        </div>
    </div>
    <ng-template facet-host></ng-template>

    `,
    styleUrls: ['app/view/game/module/shared/component/facet/facet.component.css']
})
export class FacetComponent implements OnInit {
    @Input() context?: any;
    @Input('tabs') facets?: String[];
    // Optional string that will be used as the registered cloud gem id for metrics
    @Input() metricIdentifier: string;
    /** Disable inherited facets such as REST explorer and Logs */
    @Input() disableInheritedFacets: boolean;
    @Output() tabClicked = new EventEmitter<number>();
    @ViewChild(FacetDirective) facetHost: FacetDirective;

    private model = {
        facets: {
            qualified: new Array<Facet>()
        },
        cloudGemFacetCount: 0,
        activeFacetIndex: 0
    }

    constructor(private componentFactoryResolver: ComponentFactoryResolver, private definitions: DefinitionService, private lymetrics: LyMetricService) { }

    ngOnInit(): void {
        if (!this.context)
            this.context = {}

        this.context = Object.assign(this.context, this.definitions.defines)

        // Default setting for subnav component
        if (!this.facets)
            this.facets = []

        // Define all qualified facets for this Cloud Gem
        this.defineQualifiedFacets()

        this.model.cloudGemFacetCount = this.facets.length;
        this.facets = this.facets.concat(this.model.facets.qualified.map(facet => {
            return facet.title
        }))

        this.emitActiveTab(0);
    }

    emitActiveTab(index: number): void {
        this.sendEvent(index);
        this.model.activeFacetIndex = index;
        this.tabClicked.emit(index);
        this.loadComponent(index - this.model.cloudGemFacetCount)
    }

    defineQualifiedFacets(): void {
        //iterate all defined facets
        if (!this.disableInheritedFacets) {
            for (let i = 0; i < FacetDefinitions.defined.length; i++) {
                let facet = FacetDefinitions.defined[i]
                let isApplicable = true;
                for (let x = 0; x < facet.constraints.length; x++) {
                    let constraint = facet.constraints[x];
                    //determine if the defined facet qualifies for this cloud gem
                    if (!(constraint in this.context)) {
                        isApplicable = false;
                    } else {
                        isApplicable = true;
                        facet.data[constraint] = this.context[constraint]
                        facet.data["Identifier"] = this.metricIdentifier
                        break;
                    }
                }

                if (isApplicable) {
                    this.model.facets.qualified.push(facet)
                }
            }
        }

        //order the qualified facets by their priority index
        this.model.facets.qualified.sort((facet1, facet2) => {
            if (facet1.order > facet2.order)
                return 1;
            if (facet1.order < facet2.order)
                return -1;
            return 0;
        })
    }

    loadComponent(index: number) {
        let viewContainerRef = this.facetHost.viewContainerRef;
        viewContainerRef.clear();

        if (index >= this.model.facets.qualified.length ||
            index < 0) {
            return;
        }

        let facet = this.model.facets.qualified[index];
        if (!facet.component) {
            console.warn("Facet generator failed to find a component for this qualified facet.")
            return;
        }
        let componentFactory = this.componentFactoryResolver.resolveComponentFactory(facet.component);

        let componentRef = viewContainerRef.createComponent(componentFactory);
        (<Facetable>componentRef.instance).data = facet.data;
    }

    sendEvent(index:number) {
        this.lymetrics.recordEvent('FacetOpened', {
            "Name": this.facets[index].toString(),
            "Identifier": this.metricIdentifier
        })
    }
}