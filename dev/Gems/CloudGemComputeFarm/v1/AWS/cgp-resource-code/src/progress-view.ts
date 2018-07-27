import { EventLog } from "./event-log"

class DisplayDetails {
    // Class for managing detail text we want to display when hovering.
    private createTime: string;
    private beginTime: string = "--";
    private completeTime: string = "--";
    private buildTime: string = "--";
    private totalTime: string = "--";
    private machineId: string = "--";

    private _activated = false;

    constructor(private owner: any) {
        this.createTime = owner.startTime.toLocaleString();
    }

    update(evt: any): void {
        this.machineId = evt['host-id'];

        if (!this._activated && this.owner.active) {
            // Record our start time
            this._activated = true;
            this.beginTime = evt['date'].toLocaleString();
        }

        if (this.owner.closeTime) {
            // Record our finish time
            this.completeTime = this.owner.closeTime.toLocaleString();
            this.buildTime = this.formatTimeDiff(this.owner.closeTime - this.owner.activeTime);
            this.totalTime = this.formatTimeDiff(this.owner.closeTime - this.owner.startTime);
        }
    }

    formatTimeDiff(value): string {
        let dt = new Date(value);
        let result = dt.getUTCHours() + "h " + dt.getUTCMinutes() + "m " +
            dt.getUTCSeconds() + "s";

        return result;
    }
}

export class ProgressView {
    progressRoot = null;
    private _progressPathLookup = {};
    private _progressHostIdLookup = {};
    private _currentTime = null;
    private _progressStartTime = null;
    private _updateMarker = 0;
    private _maxDepth = 0;
    private _columnAdvance = 40;

    constructor(private _eventLog: EventLog) {
    }

    clearProgress(): void {
        this.progressRoot = null;
        this._progressPathLookup = {};
        this._progressHostIdLookup = {};
        this._currentTime = null;
        this._progressStartTime = null;
        this._updateMarker = 0;
        this._maxDepth = 0;
    }

    updateProgress(currentTime: Date): void {
        this._currentTime = currentTime;
        let updateNodes = [];
        this._updateMarker++;

        let events = this._eventLog.events;

        for (var eventIdx = this._eventLog.frameStartIndex; eventIdx < events.length; eventIdx++) {
            var evt = events[eventIdx];
            var pathStr = evt['path'];
            var timestamp = evt['date'];
            var existingNode = undefined;

            if (pathStr === undefined || pathStr === null) {
                continue;
            }

            if (this._progressStartTime == null) {
                this._progressStartTime = new Date(timestamp.getTime() - 48000);  // Start 48 seconds earlier to add a 48 pixel buffer to get everything onscreen correctly
            }

            var path = JSON.parse(pathStr);
            var searchPath = path.slice(0);  // deep copy

            // Find the deepest node in the hierarchy with a path that prefixes the node we're looking for
            while (true) {
                existingNode = this._progressPathLookup[searchPath];

                if (existingNode !== undefined) {
                    break;
                }

                if (searchPath.length) {
                    searchPath.pop();
                }
                else {
                    var progressParent = {
                        'children': [],
                        'path': [],
                        'childLeadPos': 0,
                        'childTailPos': 0,
                        'closeTimePos': 0,
                        'childrenClosed': false
                    };

                    this.progressRoot = this.createNode(progressParent, [], evt, timestamp);
                    this.progressRoot.parent = null;
                    existingNode = this.progressRoot;
                    break;
                }
            }

            // Now extend the node we found with new children up to the sought-after path
            while (searchPath.length < path.length) {
                searchPath.push(path[searchPath.length]);
                var newChild = this.createNode(existingNode, searchPath, evt, timestamp);

                if (existingNode.children.length) {
                    newChild.prevSibling = existingNode.children[existingNode.children.length - 1];
                    newChild.prevSibling.nextSibling = newChild;
                }
                else {
                    existingNode.startTime = timestamp;
                    existingNode.startTimePos = this.timeToPosition(timestamp);
                }

                this._maxDepth = Math.max(this._maxDepth, newChild.path.length);
                existingNode.children.push(newChild);
                existingNode.childrenClosed = false;
                existingNode = newChild;
            }

            // Record information about the event in the node we found/created
            this._progressHostIdLookup[evt['host-id']] = existingNode;

            var description = evt['desc'];
            var status = evt['status']
            existingNode['nextLabel'] = description ? (status ? (description + ": " + status) : description) : "";

            if (!existingNode.active && evt['phase']) {
                // Record the moment this node became "active", i.e. done with division phase
                existingNode.active = true;
                existingNode.activeTime = timestamp;
            }

            if (!evt['activity']) {
                // An activity has ended, so record whatever closing information is applicable
                var desc = existingNode.lastEvent['desc'];
                var phase = existingNode.lastEvent['phase'];

                if (phase != null) {
                    if (phase == 0) {
                        // Once the division phase ends is where the node's start time begins
                        existingNode['startTime'] = timestamp;
                        existingNode['startTimePos'] = this.timeToPosition(timestamp);
                    }
                    else {
                        // Any other phase ending is where the node's end time is recorded
                        existingNode['closeTime'] = timestamp;
                        existingNode['closeTimePos'] = this.timeToPosition(timestamp);

                        if (!path.length) {
                            this.progressRoot.fixedParent.childTailPos = existingNode.closeTimePos;
                        }
                    }
                }
            }

            existingNode.lastEvent = evt;
            existingNode.details.update(evt);

            // Make sure all parent nodes to this node get their geometries updated
            while (existingNode) {
                if (existingNode.updateMarker != this._updateMarker) {
                    existingNode.updateMarker = this._updateMarker;
                    updateNodes.push(existingNode);
                }

                existingNode = existingNode.parent;
            }
        }

        updateNodes.sort(function(a, b) { return b.path.length - a.path.length; });
        let now = this._currentTime;
        let nowPos = this.timeToPosition(now);

        let self = this;
        setTimeout(function() { 
            for (let idx = 0; idx < updateNodes.length; idx++) {
                self.updateProgressNode(updateNodes[idx], nowPos);
            }
            self.progressFrameUpdate(nowPos);
        }, 0);
    }

    private updateProgressNode(node: any, nowPos: number): void {
        // Update the geomoetry for this node
        let indent = node.path.length * this._columnAdvance;
        node.label = node.nextLabel;
        node.initialized = true;
        
        node.childrenClosed = node.children.reduce(function(incoming, current) { return incoming && current.closeTimePos; }, true);
        node.childLeadPos = node.children.reduce(function(best, current) { return Math.min(best, current.startTimePos); }, node.startTimePos) + indent;
        node.childTailPos = node.children.reduce(function(best, current) { return Math.max(best, current.closeTimePos); }, 0) + indent + this._columnAdvance * (this._maxDepth - node.path.length);

        if (!node.childTailPos) {
            node.childTailPos = nowPos + indent;
        }

        node.childSpace = node.children.length ? (node.childTailPos - node.childLeadPos) : 0;
        node.leadEndPos = node.startTimePos + indent;
        node.mainPos = node.startTimePos + node.childSpace + indent + (node.children.length ? this._columnAdvance * (this._maxDepth - node.path.length - 1) : 0);

        if (node.closeTimePos) {
            // This node is closed, so position its tail as well as the tail for its children
            node.mainEndPos = node.closeTimePos + indent + this._columnAdvance * 2 * (this._maxDepth - node.path.length);
            
            if (node.childrenClosed) {
                node.childTailPos = node.mainPos;
            }

        }
        else {
            // This node is open, so its tail is zero-length
            node.mainEndPos = node.mainPos;
        }

        node.childrenHeight = node.children.reduce(function(sum, current) { return sum + current.containerHeight; }, 0);
        node.shaftHeight = node.children.length ? (node.childrenHeight - node.children[node.children.length - 1].containerHeight + 24) : 0;
        node.containerHeight = node.childrenHeight + 24;
    }

    private progressFrameUpdate(nowPos: number): void {
        if (!this.progressRoot) {
            return;
        }

        for (var pathKey in this._progressPathLookup) {
            var node = this._progressPathLookup[pathKey];

            if (node.fixedParent.childrenClosed) {
                continue;
            }

            var indent = node.path.length * this._columnAdvance;

            if (!node.closeTimePos) {
                if (node.childrenClosed) {
                    node.mainEndPos = nowPos + indent;

                    if (node.children.length) {
                        node.mainEndPos += this._columnAdvance * 2 * (this._maxDepth - node.path.length);
                    }

                    node.childTailPos = node.mainPos;
                }
                else {
                    node.childTailPos = nowPos + indent + this._columnAdvance * (this._maxDepth - node.path.length);
                }
            }
        }

        if (!this.progressRoot.closeTimePos) {
            this.progressRoot.fixedParent.childTailPos = nowPos + this._columnAdvance * this._maxDepth;
        }
    }
    
    private timeToPosition(dt: any): number {
        return (dt - this._progressStartTime) / 1000;
    }

    private createNode(parent: any, path: Array<any>, evt: any, timestamp: Date): any {
        var result = {
            'index': parent.children.length,
            'updateMarker': 0,
            'path': path.slice(0),
            'pathStr': path.toString(),
            'parent': parent,
            'fixedParent': parent,
            'children': [],
            'childrenClosed': true,
            'label': "",
            'nextLabel': "",
            'containerHeight': 0,
            'childrenHeight': 0,
            'startTime': timestamp,
            'startTimePos': this.timeToPosition(timestamp),
            'activeTime': null,
            'closeTimePos': 0,
            'closeTime': null,
            'nextSibling': null,
            'prevSibling': null,
            'lastEvent': evt,
            'active': false,
            'initialized': false,
            'entered': false,
            'details': null,

            'mainPos': 0,
            'childSpace': 0,
            'childLeadPos': 0,
            'childTailPos': 0,
            'leadEndPos': 0
        };

        result.leadEndPos = result.startTimePos + path.length * this._columnAdvance;
        result.details = new DisplayDetails(result);

        this._progressPathLookup[path.toString()] = result;

        return result;
    }
}
