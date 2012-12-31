/* Requires json-table.builder.js */
$("head").append("<script src=\"/js/json-table-builder.js\"></script>") ;

JsonTableBuilder.derive(Instances) ;
function Instances(prefix)
{
	if ( !prefix )
		prefix = "instances" ;
	JsonTableBuilder.call(this, "/instances.json?players=1", prefix) ;
	this.columns = [ 'Inst', 'Type', 'Age(s)', 'T0', 'T1', 'T2', 'Cap', 'Pass', 'State', 'Child' ] ;
}

Instances.prototype.row = function(values, numPlayers)
{
	var $tr = $('<tr>') ;
    if ( numPlayers < 1 ) numPlayers = 1 ;
	$tr.append($('<td>').attr('rowSpan', numPlayers + 1).text(String(values[0]))) ;
	for ( var i = 1 ; i < values.length ; ++i )
	{
		$tr.append($('<td>').text(String(values[i]))) ;
	}
	return $tr ;
}

Instances.prototype.populate = function($hdrEl, $bodyEl, $footEl, data)
{
 data.sort(function (a,b) { return a.id - b.id ; }) ;
 for ( var i = 0 ; i < data.length ; ++i )
 {
  var rowClass = 'l' + (i%2) ;
  var row = data[i] ;
  var info = [row.id, row.cat+"."+row.type, row.age, row.teams[0], row.teams[1], row.teams[2], row.cap, row.passwd, row.state, row.child] ;
  var $row = this.row(info, row.players.length) ;
  $row.addClass(rowClass) ;
  $bodyEl.append($row) ;
  if ( row.players && row.players.length > 0 )
  {
   row.players.sort(function (a,b) { if ( a.call < b.call ) return -1 ; if ( b.call > a.call ) return 1 ; return 0 ; }) ;
   var $subEl = $('<table width="100%">') ;
   for ( var p = 0 ; p < row.players.length ; ++p )
   {
    var player = row.players[p] ;
    $row = this.row([player.call, player.id, player.team, player.weap[0]+","+player.weap[1]+","+player.weap[2]+","+player.weap[3], player.spawned ? "yes" : "no"]).addClass(rowClass) ;
    $subEl.append($row) ;
   }
   $bodyEl.append($('<tr>').append($('<td>').attr('colSpan', info.length - 1).addClass(rowClass).append($subEl))) ;
  }
  else
  {
   $bodyEl.append($('<tr><td colSpan="'+(info.length-1)+'">Empty</td></tr>').addClass(rowClass)) ;
  }
 }

 $bodyEl.show() ;
}

instances = new Instances() ;

$(document).ready(function () { instances.attachToRefresh() ; instances.fetchData() ; }) ;

