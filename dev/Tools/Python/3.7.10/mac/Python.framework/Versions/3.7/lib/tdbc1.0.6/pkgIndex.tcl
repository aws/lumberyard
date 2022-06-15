# Index file to load the TDBC package.

# Make sure that TDBC is running in a compatible version of Tcl, and
# that TclOO is available.

if {[catch {package present Tcl 8.5}]} {
    return
}
package ifneeded tdbc 1.0.6 \
    "package require TclOO 0.6-;\
    [list load [file join $dir libtdbc1.0.6.dylib] tdbc]\;\
    [list source [file join $dir tdbc.tcl]]"
