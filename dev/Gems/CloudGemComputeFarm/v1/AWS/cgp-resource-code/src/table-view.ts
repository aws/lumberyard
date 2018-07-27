import { EventLog } from "./event-log"

class DisplayDetails {
    // Class for managing detail text we want to display when hovering.
    private beginTime: string = "--";
    private completeTime: string = "--";
    private totalTime: string = "--";
    private activity: string = "--"

    constructor(private slice: any) {
        // Record our start time and activity
        let evt = slice.startEvent;
        this.beginTime = evt.date.toLocaleString();
        this.activity = (evt.desc && evt.desc.length) ? evt.desc : this.activity;
    }

    close(evt: any): void {
        // Record our finish time
        this.completeTime = evt.date.toLocaleString();
        this.totalTime = this.formatTimeDiff(evt.date - this.slice.startEvent.date);
    }

    formatTimeDiff(value): string {
        let dt = new Date(value);
        let result = dt.getUTCHours() + "h " + dt.getUTCMinutes() + "m " +
            dt.getUTCSeconds() + "s";

        return result;
    }
}

export class TableView {
    private _timelineStart: Date = null;
    
    tableRows = [];
    hostIdsToTableRowIndex = {};

    constructor(private _eventLog: EventLog) {
    }

    clearTable(): void {
        this.tableRows = [];
        this.hostIdsToTableRowIndex = {};
        this._timelineStart = null;
    }

    updateTable(currentTime: Date): void {
        let updateRows = {};
        let now = currentTime;
        let events = this._eventLog.events;

        for (var eventIdx = this._eventLog.frameStartIndex; eventIdx < events.length; eventIdx++) {
            var evt = events[eventIdx];
            var hostId: string = evt['host-id'];
            var existingRowIndex = this.hostIdsToTableRowIndex[hostId];
            var existingRow = null;
            
            if (existingRowIndex === undefined) {
                existingRowIndex = this.tableRows.length;
                this.tableRows.push({
                    'events': {}, 
                    'sorted': [], 
                    'slices': [],
                    'status': "",
                    'statusPos': 0,
                    'idx': existingRowIndex, 
                    'hostId': hostId
                });
                existingRow = this.tableRows[existingRowIndex];
                this.hostIdsToTableRowIndex[hostId] = existingRowIndex;
            }
            else {
                existingRow = this.tableRows[existingRowIndex];
            }
            
            var rowInfo = existingRow;
            var eventId = evt['event-id'];
            
            var timestampDate = evt['date'];
            if (!this._timelineStart || timestampDate < this._timelineStart)
            {
                this._timelineStart = timestampDate;
            }

            rowInfo.events[eventId] = evt;
            rowInfo.sorted.push(evt);
            updateRows[hostId] = true;
        }
        
        for (var hostId in updateRows) {
            var rowInfo = this.tableRows[this.hostIdsToTableRowIndex[hostId]];
            var currentSlice = null;

            for (var eventIdx = 0; eventIdx < rowInfo.sorted.length; eventIdx++) {
                var evt = rowInfo.sorted[eventIdx];
                var status = evt['desc'];

                if (!status) {
                    if (currentSlice) {
                        currentSlice.width = this.dateToPx(evt.date, currentSlice.startEvent.date);
                        currentSlice.details.close(evt);
                        currentSlice.open = false;
                        currentSlice = null;
                    }
                }
                else {
                    if (!currentSlice) {
                        currentSlice = evt['slice'];

                        if (currentSlice === undefined) {
                            rowInfo.slices.push({
                                'width': 0,
                                'left': this.dateToPx(evt.date),
                                'open': true,
                                'startEvent': evt,
                                'details': null
                            });
                            currentSlice = rowInfo.slices[rowInfo.slices.length - 1];
                            currentSlice.details = new DisplayDetails(currentSlice);
                        }
                    }
                }
            }

            if (currentSlice) {
                rowInfo.statusPos = currentSlice.left;
                rowInfo.status = rowInfo.sorted[rowInfo.sorted.length - 1]['desc'];
                rowInfo.openSlice = currentSlice;
            }
            else {
                rowInfo.status = "";
                rowInfo.openSlice = null;
            }
        }

        for (var rowIdx in this.tableRows) {
            var rowInfo = this.tableRows[rowIdx];

            if (rowInfo.openSlice) {
                rowInfo.openSlice.width = this.dateToPx(now, rowInfo.openSlice.startEvent.date);
            }
        }
    }
    
    dateToPx(date: any, relativeTo: any = null): string {
        if (!relativeTo) {
            relativeTo = this._timelineStart;
        }
        
        return Math.floor((date - relativeTo) / 1000).toString();
    }
}
