import { Component } from '@angular/core';
import { ImageOverlay } from 'leaflet';

declare var L: any;

/**
 * Heatmap class contains all the data related to a heatmap.  It also wraps a few Leaflet objects to keep things contained.
 */

export class Heatmap {
    id: string;
    map: L.Map;
    imageUrl: string = "https://m.media-amazon.com/images/G/01/cloudcanvas/images/StarterGame._CB1513096809_.png";
    fileName: string;
    height: number = 500;
    width: number = 1500;
    minZoomLevel: number = -1;
    maxZoomLevel: number = 4;
    imageOverlay: L.ImageOverlay;
    xCoordinateMetric: string = "Select";
    yCoordinateMetric: string = "Select";
    layer: L.Layer;
    event: string = "Select";
    eventHasStartDateTime: boolean = false;
    eventHasEndDateTime: boolean = false;
    eventStartDateTime: any;
    eventEndDateTime: any;
    worldCoordinateLowerLeftX: number = 0;
    worldCoordinateLowerLeftY: number = 0 ;
    worldCoordinateLowerRightX: number = 0;
    worldCoordinateLowerRightY: number = 0;
    printControl: any;
    customFilter: string = undefined

    bounds: [number, number][] = [[0, 0], [this.height, this.width]];
    validationModel = {
        id: { isValid: true }
    }

    constructor(heatmap?: Heatmap){

        if (heatmap) {
            if (heatmap.id) {
                this.id = heatmap.id;
            }
            if (heatmap.fileName) {
                this.fileName = heatmap.fileName;
            }
            if (heatmap.imageUrl) {
                this.imageUrl = heatmap.imageUrl;
            }
            if (heatmap.minZoomLevel) {
                this.minZoomLevel = heatmap.minZoomLevel;
            }
            if (heatmap.maxZoomLevel) {
                this.maxZoomLevel = heatmap.maxZoomLevel;
            }
            if (heatmap.height) {
                this.height = heatmap.height;
            }
            if (heatmap.width) {
                this.width = heatmap.width;
            }

            if (heatmap.xCoordinateMetric) {
                this.xCoordinateMetric = heatmap.xCoordinateMetric;
            }
            if (heatmap.yCoordinateMetric) {
                this.yCoordinateMetric = heatmap.yCoordinateMetric;
            }
            if (heatmap.event) {
                this.event = heatmap.event;
            }
            if (heatmap.customFilter) {
                this.customFilter = heatmap.customFilter;
            }
            this.eventHasStartDateTime = heatmap.eventHasStartDateTime;
            this.eventHasEndDateTime = heatmap.eventHasEndDateTime;

            this.eventStartDateTime = heatmap.eventStartDateTime ? heatmap.eventStartDateTime : this.getDefaultEndDateTime;
            this.eventEndDateTime = heatmap.eventEndDateTime ? heatmap.eventEndDateTime : this.getDefaultEndDateTime;

            this.worldCoordinateLowerLeftX = heatmap.worldCoordinateLowerLeftX;
            this.worldCoordinateLowerLeftY = heatmap.worldCoordinateLowerLeftY;
            this.worldCoordinateLowerRightX = heatmap.worldCoordinateLowerRightX;
            this.worldCoordinateLowerRightY = heatmap.worldCoordinateLowerRightY;

            // Heatmap settings should all be loaded by this point so set the bounds, create the existing overlay and bind the image loaded handler
            this.bounds = [[0, 0], [this.height, this.width]];
            this.imageOverlay = L.imageOverlay(this.imageUrl, this.bounds);
        } else {
            this.eventStartDateTime = this.getDefaultEndDateTime();
            this.eventEndDateTime = this.getDefaultEndDateTime();
        }


    }

    /**
     * Updates the heatmap to refect the current values.  If this is not called the heatmap will not show any changes to it's properties
     * @param heatmap
     */
    update = () => {
        this.map.options.minZoom = this.minZoomLevel;
        this.map.options.maxZoom = this.maxZoomLevel;
        if (this.imageOverlay) {
            this.map.removeLayer(this.imageOverlay)
        }
        this.imageOverlay = L.imageOverlay(this.imageUrl, this.bounds).addTo(this.map);
        this.imageOverlay.addEventListener('load', (event) => {

            let imageElement = this.imageOverlay.getElement();
            this.width = imageElement.naturalWidth;
            this.height = imageElement.naturalHeight;

            this.bounds = [[0, 0], [this.height, this.width]];
            // this.map.fitBounds(this.bounds);
        });

    }

    private getDefaultStartDateTime() {
        var today = new Date();
        return {
            date: {
                year: today.getFullYear(),
                month: today.getMonth() + 1,
                day: today.getDate() - 1
            },
            time: { hour: 12, minute: 0 },
            valid: true
        }
    }

    private getDefaultEndDateTime() {
        var today = new Date();
        return {
            date: {
                year: today.getFullYear(),
                month: today.getMonth() + 1,
                day: today.getDate()
            },
            time: { hour: 12, minute: 0 },
            valid: true
        }
    }

    private formatDateForQuery = (datetime) => {
        let year = datetime.date.year;
        let month = datetime.date.month < 10 ? "0" + datetime.date.month : datetime.date.month;
        let day = datetime.date.day < 10 ? "0" + datetime.date.day : datetime.date.day;
        let hour = datetime.time.hour < 10 ? "0" + datetime.time.hour : datetime.time.hour;
        return `${year}${month}${day}${hour}0000`
    }

    public getFormattedStartDate = () => {
        return this.formatDateForQuery(this.eventStartDateTime)
    }

    public getFormattedEndDate = () => {
        return this.formatDateForQuery(this.eventEndDateTime)
    }
}
