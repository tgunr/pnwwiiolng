$("head").append("<script src=\"/js/page.js\"></script>") ;

var dbKey = "pnPlayerBalanceStatsPoints" + document.domain ;
var timer ;

var statsPoints = [ ] ;
var statsDraw = 0 ;
var maxPoints = 8000 ;
var maxLines = 26 ;
var refreshRate = 30 ;
var jqPlot = 0 ;
var storage ;
var lineNo = 0 ;

function addRow($row, players) {
 $('#output').prepend($row.addClass('l'+lineNo)) ;
 lineNo = 1 - lineNo ;
 var table = document.getElementById("output") ;
 while ( table.rows.length > maxLines ) {
  table.deleteRow(table.rows.length - 1) ;
 }
 statsPoints.push([new Date(), parseInt(players)]) ;
 while ( statsPoints.length > maxPoints ) {
  statsPoints.splice(0,1) ;
 }
 if ( storage ) { storage.setItem(dbKey, JSON.stringify(statsPoints)) ; }
 if ( statsDraw ) { statsDraw() ; }
 var freqEl = document.getElementById("freq") ;
 if ( freqEl ) {
  refreshRate = freqEl.value ;
 }
 timer = setTimeout("sendRequest()", refreshRate * 1000) ;
}

function parseJSON(o) {
 var $row = $('<tr>') ;
 var now = makeDate(new Date()) ;

 var values = [ now, o.players, o.total.lobby, o.total.allied, o.total.axis, o.spawned.allied, o.spawned.axis ] ;
 for (i in values) {
  $row.append($('<td>').text(String(values[i])).css('text-align', 'center')) ;
 }
 addRow($row, o.players) ;
}

function errorHandler(o) {
 var $row = $('<tr>') ;
 var now = new Date() ;
 $row.append($('<th>').text(makeDate(new Date()))) ;
 $row.append($('<td>').html("<i>Unable to retrieve stats: Code " + o.status + "</i>").css('text-align','center').attr('colSpan', 7)) ;
 addRow($row, 0) ;
}

function sendRequest() {
 $.getJSON("/player-balance.json", parseJSON).error(errorHandler);
}

$(function() {
	storage = window.localStorage ;
	if ( !storage ) { storage = localStorage ; }
	if ( !storage ) { storage = window.sessionStorage ; }
	if ( !storage ) { storage = sessionStorage ; }

	if ( storage ) {
	 try {
	  var text = storage.getItem(dbKey) ;
	  var data = JSON.parse(text) ;
	  for ( i in data ) {
	   statsPoints.push([new Date(data[i][0]), parseInt(data[i][1])]);
	  }
	 } catch ( e ) {
	 }
	}

	// Executed after page loads.
	statsDraw = function () {
		var dMin = statsPoints[0][0] ;
		var dMax = statsPoints[statsPoints.length-1][0] ;
		if ( jqPlot ) {
			jqPlot.series[0].data = statsPoints ;
			jqPlot.replot({
				resetAxes: true, clear: true, defaultWidth: 400, defaultHeight:160, axes:{yaxis:{min:0}, xaxis:{min:dMin, max:dMax}},
			});
		}
		else {
    		var options = {
				series:[{lineWidth:1, showMarker:false}],
				axes:{
					yaxis:{
						label:'Players',
						labelRenderer: $.jqplot.CanvasAxisLabelRenderer,
						tickOptions: { formatString: "%d" },
						min: 0, pad: 0
					},
					xaxis:{
						renderer:$.jqplot.DateAxisRenderer,
						labelRenderer: $.jqplot.CanvasAxisLabelRenderer,
						tickOptions: { formatString: "%m/%d %H:%M:%S" },
						min: dMin, max: dMax
					}
				},
				resetAxes: true,
				clear: true,
				defaultWidth: 400,
				defaultHeight: 160,
			} ;
   			jqPlot = $.jqplot("jqGraph", [statsPoints], options) ;
		}
	};
	sendRequest() ;
}) ;

function changeFreq(e) {
 if ( timer ) { clearTimeout(timer) ; }
 refreshRate = e.value ;
 timer = setTimeout("sendRequest()", refreshRate * 1000) ;
}

function clearStats() {
 if ( timer ) { clearTimeout(timer) ; }
 statsPoints = [] ;
 if ( storage ) { storage.setItem(dbKey, "") ; }
 timer = setTimeout("sendRequest()", 1000) ;
 return false ;
}
