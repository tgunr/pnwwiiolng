/* Requires json-table.builder.js */
$("head").append("<script src=\"/js/json-table-builder.js\"></script>") ;

JsonTableBuilder.derive(InstLoads) ;
function InstLoads(prefix)
{
	if ( !prefix )
		prefix = "inst-loads" ;
	JsonTableBuilder.call(this, "/inst-cell-load.json", prefix) ;
	this.columns = [ 'Cell#', 'Capacity', 'Reserved', 'Instances' ] ;
}

InstLoads.prototype.row = function(host, cap, res, inst)
{
 var $tr = JsonTableBuilder.prototype.row.call(this, [host, cap, res, inst]) ;
 // Color code this line based on the status.
 var color = 'green' ;
 if ( cap == 0 || res >= cap ) {
  color = 'red' ;	// red = full or unavailable.
 } else if ( res >= (cap * 0.75) ) {
  color = 'orange' ;	// orange = 3/4 full or more
 } else if ( res == 0 ) {
  color = 'blue' ;		// empty
 }

 $tr.css('color', color) ;

 return $tr ;
}

InstLoads.prototype.populate = function($hdrEl, $bodyEl, $footEl, data)
{
 var hosts = [] ;
 var totalCap = 0 ;
 var totalInst = 0 ;
 var totalRes = 0 ;

 for ( var ip in data.cells ) { hosts.push(ip) ; }
 hosts.sort() ;
 for ( var i = 0 ; i < hosts.length ; ++i )
 {
  var host = hosts[i] ;
  var info = data.cells[host] ;
  var cap = parseInt(info['cap']) ;
  var res = parseInt(info['res']) ;
  var inst = parseInt(info['inst']) ;

  totalCap += cap ;
  totalRes += res ;
  totalInst += inst ;

  $bodyEl.append(this.row(host, cap, res, inst).addClass('l'+(i%2))) ;
 }

 if ( $footEl && $footEl.length != 0 )
 {
  var now = String(new Date()) ;
  now = now.replace(/\s*\(.*\)\s*/g, '') ;
  $footEl.html("") ;
  $footEl.append(this.row("Totals " + now, totalCap, totalRes, totalInst).addClass('foot')) ;
  $footEl.show() ;
 }

 $bodyEl.show() ;
}

instLoads = new InstLoads() ;

$(document).ready(function () { instLoads.attachToRefresh() ; instLoads.fetchData() ; }) ;

