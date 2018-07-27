export class EventLog {
    public events: Array<any> = [];
    public frameStartIndex: number = 0;

    private _eventIdHash = {};

    updateEvents(inEvents: Array<any>): void {
        this.frameStartIndex = this.events.length;

        for (var idx = 0; idx < inEvents.length; idx++) {
            let evt = inEvents[idx];
            let eventId = evt['event-id'];

            if (eventId in this._eventIdHash) {
                continue;
            }

            this._eventIdHash[eventId] = evt;
            this.events.push(evt);
            evt['date'] = new Date(evt['timestamp'] + "Z");
        }
    }

    clear(): void {
        this.events = [];
        this.frameStartIndex = 0;
        this._eventIdHash = {};
    }
}
