import { Injectable } from '@angular/core';

@Injectable()
/**
 * Breadcrumb Service
 * Keeps track of the current state of the breadcrumbs for gems.  This service allows
 * gems to modify the current breadcrumbs to suit their implementation.  The underlying
 * data model for this is just a stack.
*/
export class BreadcrumbService {
	private _breadcrumbs: Array<Breadcrumb>;
	constructor() {
		this._breadcrumbs = new Array<Breadcrumb>();
	}

	/** Adds a breadcrumb to the existing list of breadcrumbs
	 * @param breadcrumb the breadcrumb to add
	**/
	addBreadcrumb(label, functionCb) {
		this._breadcrumbs.push(new Breadcrumb(label, functionCb));
	}	

	/**
	 * Remove the last breadcrumb
	 * @return the breadcrumb that was removed
	**/
	removeLastBreadcrumb(): Breadcrumb {
		return this._breadcrumbs.pop();
	}

	/**
	 * Removes all the current breadcrumbs from the stack.
	**/
	removeAllBreadcrumbs() {
		for (var i = this._breadcrumbs.length - 1; i >= 0; i--) {
			this.removeLastBreadcrumb();
		}
	}

	public get breadcrumbs(): Array<Breadcrumb> {
		return this._breadcrumbs
	}
}



class Breadcrumb {
	private _label: string;
	private _functionCb: Function;

	constructor(label: string, functionCb: Function) {
		this._label = label;
		this._functionCb = functionCb;
	}

	get label(): string {
		return this._label;
	}

	get functionCb(): Function {
		return this._functionCb;
	}
}
