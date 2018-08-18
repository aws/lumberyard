import { Component, OnInit, ViewChild, ElementRef, Input, ChangeDetectorRef, HostListener } from '@angular/core';
import { Http } from '@angular/http';
import { FormBuilder, FormGroup, Validators, FormArray } from '@angular/forms';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { DefinitionService, BreadcrumbService } from 'app/shared/service/index';
import { AwsService } from 'app/aws/aws.service';
import { Router } from '@angular/router';
import { MetricGraph, MetricQuery } from 'app/view/game/module/shared/class/index';
import { ImageOverlay, LeafletEvent, imageOverlay } from 'leaflet';
import { ModalComponent } from 'app/shared/component/index';
import { Heatmap } from 'app/view/game/module/analytic/component/heatmap/heatmap';
import { HeatmapMode } from 'app/view/game/module/analytic/component/heatmap/heatmap.component';

import Victor from 'victor';

declare var L: any;

enum Modes {
    SAVE,
    EDIT
}


@Component({
    selector: 'create-heatmap',
    templateUrl: 'app/view/game/module/analytic/component/heatmap/create-heatmap.component.html',
    styles: [`
        #map {
            height: 500px;
            margin-top:20px;
            margin-bottom:30px;
        }
        .upload-image {
            display: none;
        }
        .heatmap-upload .col-3 p {
            margin-bottom: 40px;
        }
        .heatmap-upload {
            margin-top: 20px;
        }
        .heatmap-upload > .col-3 {
            margin-top: 15px;
        }
        .heatmap-options-container {
            max-width: 1200px;
        }
        .heatmap-options-container > form .row, .heatmap-options-container > form {
            margin-bottom: 15px;
        }
        .heatmap-upload input[type="file"] {
            display: none;
        }
        .add-layer {
            line-height: 80px;
        }
        .heatmap-button {
            margin-right: 5px
        }
        .min-max-input {
            width: 80px
        }
        .margin-top-10 {
            margin-top: 30px
        }
        div.zoom-level label {
            margin-bottom: 5px;
            font-family: "AmazonEmber-Light";
        }
        div.zoom-level .row span {
            margin-left: 5px;
            line-height: 31px;
        }
        label {
            font-size: 11px;
            font-family: "AmazonEmber-Bold";
        }
        label label {
            font-family: "AmazonEmber-Regular";
        }
    `]
})

export class CreateHeatmapComponent implements OnInit {

    @Input() heatmap?: Heatmap;
    @Input() mode: HeatmapMode;    
    // TODO: Can remove unless we're going to use a file selector
    @ViewChild('import') importButtonRef: ElementRef;

    private bucket: string;
    private metricsApiHandler: any;
    private coordinateDropdownOptions: any[];
    private tableDropdownOptions: any[];

    private metricQuery: MetricQuery;

    private isUpdatingExistingHeatmap: boolean = false;
    private previousHeatmapId: string;    
    private isLoadingEventAttributes: boolean;
    private isLoadingEvents: boolean;
    private isSaving: boolean;
    private limit: string = "1000";    

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private fb: FormBuilder, private breadcrumbs: BreadcrumbService,
        private router: Router,
        private definition: DefinitionService,
        private aws: AwsService,
        private http: Http,
        private toastr: ToastsManager,
        private cd: ChangeDetectorRef) { }

    ngOnInit() {
        // Get Metrics Data
        this.metricQuery = new MetricQuery(this.aws);
        let metricGemService = this.definition.getService("CloudGemMetric");
        this.bucket = metricGemService.context.MetricBucketName + "/heatmaps";

        // Initialize the metrics api handler
        this.metricsApiHandler = new metricGemService.constructor(metricGemService.serviceUrl, this.http, this.aws)

        if (!this.heatmap) {
            this.heatmap = new Heatmap();
            this.isUpdatingExistingHeatmap = false;
        } else {
            this.isUpdatingExistingHeatmap = true;
            // Keep track of previous ID to update in case it changes
            this.previousHeatmapId = this.heatmap.id;
            // Initialize the heatmap object with the existing heatmap values
            if (this.heatmap.fileName)
                this.heatmap.imageUrl = this.aws.context.s3.getSignedUrl('getObject', { Bucket: this.bucket, Key: this.heatmap.fileName, Expires: 300 })
            this.heatmap = new Heatmap(this.heatmap);
        }
        this.coordinateDropdownOptions = new Array<any>();
        this.tableDropdownOptions = new Array<any>();

        // create the map
        this.heatmap.map = L.map('map', {
            crs: L.CRS.Simple,
            minZoom: this.heatmap.minZoomLevel,
            maxZoom: this.heatmap.maxZoomLevel,
            attributionControl: false,
        });

        // If the overlay exists already it should now be ready to add to the map
        if (this.heatmap.imageOverlay) {
            this.heatmap.imageOverlay.addTo(this.heatmap.map);
        }

        // Trigger an update here in case the heatmap values are different than the defaults
        this.heatmap.update();
        this.heatmap.map.fitBounds(this.heatmap.bounds);

        this.getEvents()
        this.getAttributes()
        this.getMetricEventData()
    }

    getAttributes = () => {
        if (this.heatmap.event === undefined || this.heatmap.event == "Select")
            return

        this.isLoadingEventAttributes = true
        this.coordinateDropdownOptions = []
        this.metricQuery.tablename = this.heatmap.event;
        this.metricsApiHandler.postQuery(this.metricQuery.toString("describe ${database_unquoted}.${table_unquoted}")).subscribe((res) => {
            let attributes = res.Result;
            if (res.Status.State == "FAILED") {
                this.isLoadingEventAttributes = false;
                this.toastr.error(res.Status.StateChangeReason)
                return
            }
            for (let attribute of attributes) {
                let parts = attribute[0].split("\t");
                let metricName = parts[0].trim().toLowerCase();
                let type = parts[1].trim();
                if (!metricName) {
                    break;
                }
                if (type == "bigint" ||
                    type == "double" ||
                    type == "float" ||
                    type == "int" ||
                    type == "tinyint" ||
                    type == "smallint" ||
                    type == "decimal")
                    this.coordinateDropdownOptions.push({ text: metricName });
            }
            this.isLoadingEventAttributes = false
        }, err => {
            this.isLoadingEventAttributes = false
        });
    }

    getEvents = () => {
        this.isLoadingEvents = true
        this.metricsApiHandler.getFilterEvents().subscribe((res) => {
            this.isLoadingEvents = false
            let athenaTables = JSON.parse(res.body.text()).result
            let tableNames = [];

            // Add the events as layers to the array
            if (athenaTables) {
                for (let i = 0; i < athenaTables.length; i++) {
                    tableNames.push(athenaTables[i]);
                }
            }

            // Initially store the layers into an array for easy sorting.
            if (tableNames) {
                for (let layerName of tableNames) {
                    this.tableDropdownOptions.push({ text: layerName })
                }
            }
        }, err => {
            this.isLoadingEvents = false
            console.log("An error occured when trying to load the metrics tables from Athena", err)
            this.toastr.error("An error occurred when trying to load the metrics tables from Athena", err)
        });
    }

    updateXCoordinateDropdown = (val)  => this.heatmap.xCoordinateMetric = val.text

    updateYCoordinateDropdown = (val)  => this.heatmap.yCoordinateMetric = val.text;

    updateEvent = (val) => {
        this.heatmap.event = val.text;
        this.getAttributes()
    }


    updateHeatmap = (heatmap: Heatmap) => {
        this.getMetricEventData();
    }

    getMetricEventData = () => {
        if (this.heatmap.event === undefined || this.heatmap.event == "Select")
            return

        if (this.heatmap.xCoordinateMetric === undefined || this.heatmap.xCoordinateMetric == "Select")
            return

        if (this.heatmap.yCoordinateMetric === undefined || this.heatmap.yCoordinateMetric == "Select")
            return

        // Form query for getting heat coodrinates        
        let from =  "${database}.${table}"
        let x = this.heatmap.xCoordinateMetric;
        let y =  this.heatmap.yCoordinateMetric;
        let event = this.heatmap.event;
        let query = `SELECT distinct ${x}, ${y}
            from ${from} where 1 = 1`;

        query = this.appendTimeFrame(query)

        query = this.appendCustomFilter(query)

        query = this.appendLimit(query)

        this.metricQuery.tablename = event

        // Submit the query
        this.isSaving = true;
        this.metricsApiHandler.postQuery(this.metricQuery.toString(query))
        .subscribe(res => {
            let heatmapArr = res.Result;
            // Remove column header
            if (heatmapArr && heatmapArr.length > 0) {
                heatmapArr.splice(0, 1);

                // Translate the position to the orientation and position of the screenshot
                for (let i = 0; i < heatmapArr.length; i++) {
                    let element = heatmapArr[i]

                    // Initialize the two world coordinates as vectors - using victor.js
                    let LL = new Victor(this.heatmap.worldCoordinateLowerLeftX, this.heatmap.worldCoordinateLowerLeftY);
                    let LR = new Victor(this.heatmap.worldCoordinateLowerRightX, this.heatmap.worldCoordinateLowerRightY);

                    let x = element[0];
                    let y = element[1];

                    let theta = Math.atan2((LR.y - LL.y), (LR.x - LL.x))

                    // Get imageResolution from Leaflet map bounds
                    let imageResolutionX = this.heatmap.width;
                    let imageResolutionY = this.heatmap.height;

                    let referentialLength = LR.subtract(LL).length();
                    let referentialHeight = referentialLength * (imageResolutionY / imageResolutionX)

                    let x_1 = ((x - LL.x) * Math.cos(theta)) + ((y - LL.y) * Math.sin(theta))
                    let y_1 = -((x - LL.x) * Math.sin(theta)) + ((y - LL.y) * Math.cos(theta))

                    element[1] = (x_1 / referentialLength) * imageResolutionX;
                    element[0] = (y_1 / referentialHeight) * imageResolutionY;

                    heatmapArr[i] = element;
                }

                if (this.heatmap.layer) {
                    this.heatmap.map.removeLayer(this.heatmap.layer);
                }
                if (this.heatmap.printControl) {
                    this.heatmap.map.removeControl(this.heatmap.printControl);
                }


                try {
                    let layer = L.heatLayer(heatmapArr, { radius: 25 })

                    this.heatmap.layer = layer.addTo(this.heatmap.map);
                    this.heatmap.update();
                    // add print option to map
                    this.heatmap.printControl = L.easyPrint({
                        hidden: false,
                        sizeModes: ['A4Landscape']
                    }).addTo(this.heatmap.map);
                    this.toastr.success("The heatmap visualization has been updated.");
                } catch (err) {
                    console.log("Unable to update heatmap layer: " + err);
                    this.toastr.error("The heatmap visualization failed to update.  Please refer to your browser console log. ");
                }
                this.isSaving = false                
            } else {
                console.log(res)
                let error = ""
                if (res.Status && res.Status.StateChangeReason)
                    error = res.Status.StateChangeReason
                this.toastr.error(`The heatmap failed to save. ${error}`)                
                this.isSaving = false
            }
        }, err => {
            this.toastr.error("Unable to update map with metric data.", err);
        });

    }

    appendTimeFrame = (query) => {

        let startdate = this.heatmap.getFormattedStartDate()
        let enddate = this.heatmap.getFormattedEndDate()
        // Start and end time
        if (this.heatmap.eventHasStartDateTime && this.heatmap.eventHasEndDateTime) {
            query += `
            and p_server_timestamp_strftime >= '${startdate}'
            and p_server_timestamp_strftime <= '${enddate}'            
            `
        }
        // Just a start time
        else if (this.heatmap.eventHasStartDateTime && !this.heatmap.eventHasEndDateTime) {
            query += `
            and p_server_timestamp_strftime >= '${startdate}'            
            `
        }        
        return query
    }

    appendCustomFilter = (query: string): string => {                    
        if (this.heatmap.customFilter && this.heatmap.customFilter.trim() != "" && (this.heatmap.customFilter.split('and').length > 0 || this.heatmap.customFilter.split('or').length > 0)) {
            return query + ` and ${this.heatmap.customFilter}`;
        }
        return query
    }

    appendLimit = (query) => {
        return query + ` LIMIT ${this.limit}`
    }

    setLimit = (limit) => {
        this.limit = limit
    }

    eventDateTimeUpdated = (model) => {
        this.heatmap.eventHasStartDateTime = model.hasStart;
        this.heatmap.eventHasEndDateTime = model.hasEnd;

        this.heatmap.eventStartDateTime = model.start;
        this.heatmap.eventEndDateTime = model.end;
    }

    saveHeatmap = () => {
        if (this.validate(this.heatmap) == false)            
            return 

        this.harmonize(this.heatmap)
        
        let heatmapData = {
            id: this.heatmap.id,
            fileName: this.heatmap.fileName,
            imageUrl: this.heatmap.imageUrl,
            minZoomLevel: this.heatmap.minZoomLevel,
            maxZoomLevel: this.heatmap.maxZoomLevel,
            height: this.heatmap.height,
            width: this.heatmap.width,
            xCoordinateMetric: this.heatmap.xCoordinateMetric,
            yCoordinateMetric: this.heatmap.yCoordinateMetric,
            event: this.heatmap.event,            
            eventHasStartDateTime: this.heatmap.eventHasStartDateTime,
            eventHasEndDateTime: this.heatmap.eventHasEndDateTime,
            eventStartDateTime: this.heatmap.eventStartDateTime,
            eventEndDateTime: this.heatmap.eventEndDateTime,
            worldCoordinateLowerLeftX: Math.round(this.heatmap.worldCoordinateLowerLeftX),
            worldCoordinateLowerLeftY: Math.round(this.heatmap.worldCoordinateLowerLeftY),
            worldCoordinateLowerRightX: Math.round(this.heatmap.worldCoordinateLowerRightX),
            worldCoordinateLowerRightY: Math.round(this.heatmap.worldCoordinateLowerRightY),
        }
       
        if (heatmapData.event=='Select')
            delete heatmapData.event

        if (this.heatmap.customFilter)
            heatmapData['customFilter'] = this.heatmap.customFilter

        this.mode = this.isUpdatingExistingHeatmap ? "Update" : "Save"
        if (!this.isUpdatingExistingHeatmap) {
            this.submitHeatmap(heatmapData);
        } else {
            this.submitUpdatedHeatmap(heatmapData);
        }
    }

    validate = (heatmap): boolean => {
        if (this.heatmap.id === undefined
            || this.heatmap.id === null
            || this.heatmap.id.trim() === ""
        ) {
            this.toastr.error("The heatmap is missing a proper name.")
            this.heatmap.validationModel.id.isValid = false
            return false
        } else {
            this.heatmap.validationModel.id.isValid = true
        }
        
        return true
    }

    harmonize = (heatmap) => {
        this.heatmap.id = this.heatmap.id.trim().replace(" ", "")        
    }

    submitUpdatedHeatmap = (heatmap) => {
        this.isSaving = true        
        this.metricsApiHandler.updateHeatmap(this.previousHeatmapId, heatmap).subscribe(response => {
            this.toastr.success(`The heatmap '${this.heatmap.id}' has been updated.`);
            this.mode = "Edit"
            this.isSaving = false
            this.isUpdatingExistingHeatmap = true
        }, err => {
            this.isSaving = false
            })       
        this.getMetricEventData()
    }

    submitHeatmap = (heatmap) => {
        this.isSaving = true        
        this.metricsApiHandler.createHeatmap(heatmap).subscribe(response => {
            this.toastr.success(`The heatmap '${heatmap.id}' has been saved.`);
            this.mode = "List";
            this.isSaving = false
            this.isUpdatingExistingHeatmap = true
        }, err => {
            this.isSaving = false
            })
        this.getMetricEventData()
    }

    // File picker logic below
    selectMapImage = () => {
        this.importButtonRef.nativeElement.click();
    }
    importMapImage = (event) => {
        let file = event.target.files[0];
        // assemble bucket location for metrics
        var uploadImageToS3Params = {
            Body: file,
            Bucket: this.bucket,
            Key: file.name
        }
        this.aws.context.s3.putObject(uploadImageToS3Params, (err, data) => {
            this.toastr.success("Image uploaded successfully");
            // this.heatmap.imageUrl = 'https://s3.amazonaws.com/' + this.heatmapS3Bucket + "/" + file.name
            this.heatmap.imageUrl = this.aws.context.s3.getSignedUrl('getObject', { Bucket: this.bucket, Key: file.name, Expires: 300 })
            // Save the filename so we can recreate the signed URL for the next time the website loads
            this.heatmap.fileName = file.name;
            this.heatmap.update();
        })
    }
}


