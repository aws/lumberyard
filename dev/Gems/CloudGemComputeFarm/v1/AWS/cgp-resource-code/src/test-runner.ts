import { Http } from '@angular/http';
import { CloudGemComputeFarmInterface } from "./index.component"
import { testEvents } from "./test_events"

export class TestRunner {
    private _testEvents;
    private _testIndex = 0;
    private _testStartIndex = 100;
    private _testSpeedFactor = 1.0;
    private _testStartTime = null;
    private _timer;
    public loaded = false;

    constructor(private _computeFarmInterface: CloudGemComputeFarmInterface, private http: Http) {
        this._testEvents = testEvents;
        this.loaded = true;
    }

    private animate(): void {
        var events = [];
        while (this._testIndex < this._testEvents.length) {
            var td = new Date(this._testEvents[this._testIndex]['timestamp'] + "Z");
            if (td <= this._testStartTime) {
                events.push(this._testEvents[this._testIndex++]);
            }
            else {
                break;
            }
        }

        this._computeFarmInterface.frameUpdate(this._testStartTime, events);

        var nextDateIdx = this._testIndex < this._testEvents.length ? this._testIndex : (this._testEvents.length - 1);
        var nextDate = new Date(this._testEvents[nextDateIdx]['timestamp'] + "Z");
        this._testStartTime.setSeconds(this._testStartTime.getSeconds() + 1);
        
        var self = this;
        this._timer = setTimeout(function() { self.animate(); }, 1000 / this._testSpeedFactor);
    }

    public run() {
        this._testIndex = this._testStartIndex;
        this._testStartTime = new Date(this._testEvents[this._testIndex]['timestamp'] + "Z");

        var events = this._testEvents.slice(0, this._testIndex);
        this._computeFarmInterface.frameUpdate(this._testStartTime, events);
        this.animate();
    }
}
