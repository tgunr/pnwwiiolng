/* Requires json-table.builder.js */
$("head").append("<script src=\"/js/json-table-builder.js\"></script>") ;

JsonTableBuilder.derive(InstCells) ;
function InstCells(prefix)
{
	if ( !prefix )
		prefix = "inst-cells" ;
	JsonTableBuilder.call(this, "/inst-cell-instances.json", prefix) ;
	this.columns = [ 'InstID', 'Cell#', 'Capacity' ] ;
}

InstCells.prototype.populate = function($hdrEl, $bodyEl, $footEl, data)
{
	if ( $footEl && $footEl.length != 0 )
		$footEl.hide() ;

	var insts = data.instances ;

	if ( insts.length > 0 )
	{
		// Sort the instance list by instanceID.
		insts.sort(function (l, r) { return parseInt(l.id) - parseInt(r.id) ; }) ;

		for ( var i = 0 ; i < insts.length ; ++i )
		{
			var inst = insts[i] ;
			$bodyEl.append(this.row([inst.id, inst.cell, inst.cap])).addClass('l'+(i%2)) ;
		}
	}
	else
	{
		this.display("Server reporting there are currently no instances.") ;
	}
}

instCells = new InstCells() ;

$(document).ready(function () { instCells.attachToRefresh() ; instCells.fetchData() ; }) ;

