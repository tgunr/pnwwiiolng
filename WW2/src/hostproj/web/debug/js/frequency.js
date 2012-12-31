var refreshRate = 0 ;
var refreshTimer = null ;

var refreshListeners = [] ;

function populateRefreshRate() {
 var $rr = $('#refresh-rate') ;
 if ( !$rr ) { return ; }

 var rates = [
	[ '1s', 1 ], [ '5s', 5 ], [ '10s', 10 ], [ '15s', 15 ], [ '20s', 20 ], [ '30s', 30 ], [ '60s', 60 ], [ '90s', 90 ],
	[ '2m', 120 ], [ '2.5m', 150 ], [ '5m', 300 ], [ '10m', 600 ], [ '15m', 900 ], [ '30m', 1800 ], [ '1hr', 3600 ]
 ] ;

 $rr.css('text-align', 'center') ;

 var defaultRate = 0 ;
 if ( typeof defaultRefreshRate === 'undefined' ) defaultRate = rates[5][1] ;
 else defaultRate = parseInt(defaultRefreshRate) ;

 var html = "<span class='control'><label>Update Rate:</label>" ;
 for ( var i = 0 ; i < rates.length ; ++i ) {
  var checked = '' ;
  if ( defaultRate == rates[i][1] ) {
   checked = " checked='checked'" ;
   refreshRate = rates[i][1] ;
  }
  html += "&nbsp;<input class='radio' type='radio' name='freq' value='"+rates[i][1]+"' onChange='changeFreq(this)'"+checked+">"+rates[i][0]+"</input>" ;
 }
 if ( !(typeof refreshRateControls === 'undefined') ) 
  html += refreshRateControls ;
 html += "</span>" ;
 $rr.html(html) ;
}

var $rrEl = $('#refresh-rate') ;
if ( $rrEl && $rrEl.length != 0 ) {
 populateRefreshRate() ;
} else {
 $(document).ready(populateRefreshRate) ;
}

function changeFreq(e) {
 clearTimeout(refreshTimer) ;
 refreshRate = e.value ;
 for ( var i in refreshListeners ) {
  refreshListeners[i]() ;
 }
}

