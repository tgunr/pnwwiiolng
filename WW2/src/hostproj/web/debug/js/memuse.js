/* Requires json-table.builder.js */
$("head").append("<script src=\"/js/json-table-builder.js\"></script>") ;
$("head").append("<script src=\"/js/page.js\"></script>") ;

var prevUse = {} ;
var memUse ;

JsonTableBuilder.derive(MemUse) ;
function MemUse(prefix)
{
	if ( !prefix )
		prefix = "memuse" ;
	JsonTableBuilder.call(this, "/mem.json", prefix) ;
	this.columns = [ 'Location', 'Allocs', 'Frees', 'Unfreed', 'Max', 'Cur', 'Last Used' ] ;
}

function sortMemUse(a, b)
{
 return memUse[b]['cur'] - memUse[a]['cur'] ;
}

function equalUsage(field, row)
{
 if ( field != "allocs" && field != "frees" )
 {
  return false ;
 }
 return ( parseInt(row["allocs"]) == parseInt(row["frees"]) ) ;
}

MemUse.prototype.populate = function ($hdrEl, $bodyEl, $footEl, data)
{
 memUse = data ;

 var memUseIndex = [];
 for ( u in memUse ) { memUseIndex.push(u) ; }
 memUseIndex.sort(sortMemUse) ;

 if ( memUseIndex.length <= 0 )
 {
  this.display("Memory Usage is not currently enabled") ;
  return ;
 }

 var line = 0 ;
 for ( i in memUseIndex )
 {
  var u = memUseIndex[i] ;
  var mu = memUse[u] ;
  var prev = prevUse[u] ;

  // Location.
  var $tr = $('<tr>') . append( $('<th>').text(u) ) . addClass('l'+line) ;
  line = 1 - line ;

  // Remaining fields.
  for ( f in mu )
  {
   var value = parseInt(mu[f]) ;
   var color = 'black' ;
   var bold = 'normal' ;
   // Change?
   if ( f != "la" && prev && prev[f] && prev[f] != value )
   {
     var old = parseInt(prev[f]) ;
     bold = 'bold' ;
     if ( equalUsage(f, mu) == true )
       color = 'green' ;
     else if ( old > value )
       color = 'blue' ;
     else if ( old < value )
       color = 'red' ;
   }
   else if ( equalUsage(f, mu) == true )
     color = 'green' ;
   if ( f == "unfreed" && value > 0 )
     bold = 'bolder' ;
   $tr.append($('<td>').text(String(value)).css('color', color).css('fontWeight', bold)) ;
  }

  $bodyEl.append($tr) ;
 }
}

memUseScript = new MemUse() ;

$(document).ready(function () { memUseScript.attachToRefresh() ; memUseScript.fetchData() ; }) ;

