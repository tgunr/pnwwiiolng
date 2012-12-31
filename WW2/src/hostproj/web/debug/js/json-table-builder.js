/* Implements base class for implementing
   jQuery based JSON-data fetching code which
   stuffs the data into tables.

   Derive by using JsonTableBuilder.derive(yourClass);

   Member functions:

		.fetchData()
			Fetch data and, if a refresh timer is available,
			continue fetching.

		.cancelUpdates()
			Stop fetching data.

		.reschedule()
			Cancel currently pending update and, if a refresh
			timer is available, schedule another fetch for later.

		.display(text)
			Replace the body element with the text, hide the
			footer if present.

		.row([value array])
			Returns a jQuery <tr> populated with one <td> for
			each value in the the array.

		.populate($headerEl, $bodyEl, $footerEl, data)
			Your code to translate the json data result (data) into
			html. $headerEl, $bodyEl, $footerEl reflect the
			jQuery elements for the table's header, body and footer
			at the time the data was received.

		.derive(className)
			Makes className a subclass of JsonTableBuilder.
*/

//! Constructor.
function JsonTableBuilder(jsonUrl, prefix)
{
	if ( !prefix )
		prefix = "results" ;
	this.headerDivName = prefix + "-hdr" ;
	this.outputDivName = prefix + "-body" ;
	this.footerDivName = prefix + "-foot" ;
	this.refreshTimer = null ;
	this.jsonUrl = jsonUrl ;
    this.lastUrl = "" ;
	this.populatedHeader = false ;
	this.clearOnRefresh = true ;
}

//! Cancel any pending refresh.
JsonTableBuilder.prototype.cancelUpdates = function()
{
	clearTimeout(this.refreshTimer) ;
	this.refreshTimer = null ;
}

//! [optional] [overload] How to schedule data be refreshed later.
JsonTableBuilder.prototype.reschedule = function()
{
	this.cancelUpdates() ;
	// Do nothing - you need to implement this yourself.
}

JsonTableBuilder.prototype.display = function(text)
{
	var $footEl = $('#' + this.footerDivName) ;
	if ( $footEl )
		$footEl.hide() ;

	var $bodyEl = $('#' + this.outputDivName) ;
	if ( !$bodyEl )
		return ;

	$bodyEl.html("<tr><th colspan='"+this.columns.length+"'>"+text+"</th></tr>") ;
}

JsonTableBuilder.prototype.row = function(values)
{
	var $tr = $('<tr>') ;
	for ( var i = 0 ; i < values.length ; ++i )
	{
		$tr.append($('<td>').text(String(values[i]))) ;
	}
	return $tr ;
}

//! [overload] Populate your table from the data.
JsonTableBuilder.prototype.populate = function($hdrEl, $bodyEl, $footEl, data)
{
	this.display("'populate' function not implemented.") ;
}

//! [optional] [overload] Display an error.
JsonTableBuilder.prototype._rxError = function()
{
	this.cancelUpdates() ;
	$('#' + this.outputDivName).css('color', this.oldColor) ;

    if ( this.lastUrl )
		this.display("Last refresh failed [<a href=\""+this.lastUrl+"\">query</a>]") ;
	else
		this.display("Last refresh failed. Server may be offline/unreachable.") ;

	this.reschedule() ;
}

//! Receive data and forward it to populate.
JsonTableBuilder.prototype._rxData = function(data)
{
	this.cancelUpdates() ;

	var $bodyEl = $('#' + this.outputDivName) ;
	if ( !$bodyEl || $bodyEl.length == 0 )
		return ;

	var $hdrEl =  $('#' + this.headerDivName) ;
	if ( !this.populatedHeader && this.columns.length > 0 && $hdrEl && $hdrEl.length != 0 )
	{
		this.populatedHeader = true ;
		var $tr = $('<tr class="head">') ;
		for ( var i = 0 ; i < this.columns.length ; ++i )
		{
			$tr.append($('<th>').html(String(this.columns[i]))) ;
		}
		$hdrEl.replaceWith($tr) ;
	}

	var $footEl = $('#' + this.footerDivName) ;

	// Clear out what's currently there.
	if ( this.clearOnRefresh )
		$bodyEl.html("") ;
	$bodyEl.css('color', this.oldColor) ;

	// Invoke the populate function now.
	this.populate($hdrEl, $bodyEl, $footEl, data) ;

	// Schedule a refresh.
	this.reschedule() ;
}

JsonTableBuilder.prototype.args = function()
{
	return "" ;
}

//! [optional] Request a new data.
JsonTableBuilder.prototype.fetchData = function(args)
{
	this.cancelUpdates() ;

	var $bodyEl = $('#' + this.outputDivName) ;
	if ( !$bodyEl || $bodyEl.length == 0 )
		return ;

	this.oldColor = $bodyEl.css('color') ;
	$bodyEl.css('color', 'gray') ;

	this.lastUrl = this.jsonUrl ;
    var extraArgs = this.args() ;
    if ( extraArgs )
	{
		if ( args ) args += "&" ; else args = "" ;
		args += extraArgs ;
	}
	if ( args ) {
		if ( this.lastUrl.match(/\?/) ) this.lastUrl += "&" ;
		else this.lastUrl += "?" ;
		this.lastUrl += args ;
	}

	$.getJSON(this.lastUrl).success(jQuery.proxy(this._rxData, this)).error(jQuery.proxy(this._rxError, this)) ;
}

JsonTableBuilder.derive = function(child)
{
	child.prototype = new JsonTableBuilder() ;
	child.prototype.constructor = child ;
}

JsonTableBuilder.prototype.derive = JsonTableBuilder.derive ;

JsonTableBuilder.prototype.attachToRefresh = function()
{
	this.refresh = jQuery.proxy(this.fetchData, this) ;
	refreshListeners.push(this.refresh) ;
	this.reschedule = function ()
	{
		this.cancelUpdates() ;
		this.refreshTimer = setTimeout(this.refresh, refreshRate * 1000) ;
	}
}

