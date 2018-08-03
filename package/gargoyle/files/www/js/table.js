/*
 * This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


function createTable(columnNames, rowData, tableId, rowsAreRemovable, rowsAreMovable, rowRemoveCallback, rowMoveCallback, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;

	var newTable = controlDocument.createElement('table');
	var thead = controlDocument.createElement('thead');
	newTable.appendChild(thead);
	var tableBody = controlDocument.createElement('tbody');
	newTable.appendChild(tableBody);
	newTable.id = tableId;
	newTable.className = "table table-striped table-bordered";

	var row = controlDocument.createElement('tr');
	row.className = 'header_row';
	thead.appendChild(row);

	var columnIndex;
	for ( columnIndex=0; columnIndex < columnNames.length; columnIndex++ )
	{
		var header = controlDocument.createElement('th');
		if(typeof(columnNames[columnIndex]) == 'string')
		{
			var splitText = (columnNames[columnIndex]).replace(/\&amp;/g, '&').split(/\n/);
			while(splitText.length > 0)
			{
				var next = splitText.shift();
				header.appendChild(  controlDocument.createTextNode(next)  );
				if(splitText.length >0)
				{
					header.appendChild( controlDocument.createElement('br')  );
				}
			}
		}
		else
		{
			header.appendChild( columnNames[columnIndex] )
		}
		headerContent = typeof(columnNames[columnIndex]) == 'string' ? controlDocument.createTextNode(columnNames[columnIndex]) : columnNames[columnIndex];
		row.appendChild(header);
	}
	if(rowsAreRemovable)
	{
		var header = controlDocument.createElement('th');
		headerContent = controlDocument.createTextNode('');
		header.appendChild(headerContent);
		row.appendChild(header);
	}
	if(rowsAreMovable)
	{
		for(i=0; i<2; i++)
		{
			var header = controlDocument.createElement('th');
			headerContent = controlDocument.createTextNode('');
			header.appendChild(headerContent);
			row.appendChild(header);
		}
	}

	if(rowData != null)
	{
		for (rowIndex in rowData)
		{
			addTableRow(newTable, rowData[rowIndex], rowsAreRemovable, rowsAreMovable, rowRemoveCallback, rowMoveCallback);
		}
	}

	tableSanityCheck(newTable);
	return newTable;
}


function addTableRow(table, rowData, rowsAreRemovable, rowsAreMovable, rowRemoveCallback, rowMoveCallback, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;

	rowRemoveCallback = rowRemoveCallback == null ? function(){} : rowRemoveCallback;
	rowMoveCallback = rowMoveCallback == null ? function(){} : rowMoveCallback;


	row = controlDocument.createElement('tr');
	tableBody=table.tBodies[0];
	numRows= tableBody.rows.length;
	tableBody.appendChild(row);

	cellIndex = 1;
	while(cellIndex <= rowData.length)
	{
		cell = controlDocument.createElement('td');
		if(typeof(rowData[cellIndex-1]) == 'string')
		{
			var splitText = (rowData[cellIndex-1]).split(/\n/)
			while(splitText.length > 0)
			{
				var next = splitText.shift()
				cell.appendChild(  controlDocument.createTextNode(next)  )
				if(splitText.length >0)
				{
					cell.appendChild( controlDocument.createElement('br')  )
				}
			}
		}
		else
		{
			cell.appendChild(rowData[cellIndex-1]);
		}
		cell.className=table.id + '_column_' + cellIndex;
		row.appendChild(cell);
		cellIndex++;
	}
	if(rowsAreRemovable)
	{
		cellContent = createInput("button", controlDocument);
		cellContent.textContent = UI.Remove;
		cellContent.className = "btn btn-default btn-remove";
		cellContent.onclick= function() { row = this.parentNode.parentNode; table=row.parentNode.parentNode; removeThisCellsRow(this); rowRemoveCallback(table,row); };
		cell = controlDocument.createElement('td');
		cell.className=table.id + '_column_' + cellIndex;
		cell.appendChild(cellContent);
		row.appendChild(cell);
		cellIndex++;
	}
	if(rowsAreMovable)
	{
		cellContent = createInput("button", controlDocument);
		cellContent.textContent = String.fromCharCode(8593);
		cellContent.className = "btn btn-default btn-move-up";
		cellContent.onclick= function() { moveThisCellsRowUp(this); rowMoveCallback(this, "up"); };
		cell = controlDocument.createElement('td');
		cell.className=table.id + '_column_' + cellIndex;
		cell.appendChild(cellContent);
		row.appendChild(cell);
		cellIndex++;

		cellContent = createInput("button", controlDocument);
		cellContent.textContent = String.fromCharCode(8595);
		cellContent.className = "btn btn-default btn-move-down";
		cellContent.onclick= function() { moveThisCellsRowDown(this); rowMoveCallback(this, "down"); };
		cell = controlDocument.createElement('td');
		cell.className=table.id + '_column_' + cellIndex;
		cell.appendChild(cellContent);
		row.appendChild(cell);
		cellIndex++;

	}
	row.className = numRows % 2 == 0 ? 'even' : 'odd';

	tableSanityCheck(table);
}


function removeThisCellsRow(button)
{
	row=button.parentNode.parentNode;
	tableBody= row.parentNode;
	tableBody.removeChild(row);

	setRowClasses(tableBody.parentNode, true);
}
function moveThisCellsRowUp(button)
{
	row=button.parentNode.parentNode;
	tableBody=row.parentNode;

	allRows =tableBody.childNodes;
	rowIndex = 0;
	while(rowIndex < allRows.length && allRows[rowIndex] != row ) { rowIndex++; }
	if(rowIndex > 0)
	{
		tmp = allRows[rowIndex];
		tableBody.removeChild(allRows[rowIndex]);
		tableBody.insertBefore(tmp, allRows[rowIndex-1]);
	}
	setRowClasses(tableBody.parentNode, true);

}
function moveThisCellsRowDown(button)
{
	row=button.parentNode.parentNode;
	tableBody=row.parentNode;

	allRows =tableBody.childNodes;
	rowIndex = 0;
	while(rowIndex < allRows.length && allRows[rowIndex] != row ) { rowIndex++; }
	if(rowIndex < allRows.length-1 && allRows.length > 1 )
	{
		tmp = allRows[rowIndex+1];
		tableBody.removeChild(allRows[rowIndex+1]);
		tableBody.insertBefore(tmp, allRows[rowIndex]);
	}
	setRowClasses(tableBody.parentNode, true);


}



function getTableDataArray(table, rowsAreRemovable, rowsAreMovable)
{

	numEmptyCells = (rowsAreRemovable ? 1 : 0) + (rowsAreMovable ? 2 : 0);

	data = new Array();
	rows = table.rows;
	rowIndex = 0;
	while (rowIndex < rows.length)
	{
		row= rows[rowIndex];
		if(row.firstChild.tagName.toLowerCase() == 'td')
		{
			rowData = new Array();
			cells = row.childNodes;

			numCells= row.childNodes.length- numEmptyCells;
			cellNum = 0;
			while(cellNum < numCells)
			{
				cellContent = cells[cellNum].firstChild;
				if(typeof(cellContent.data) == 'string')
				{
					rowData.push( cells[cellNum].firstChild.data);
				}
				else
				{
					rowData.push(cellContent);
				}
				cellNum++;
			}
			data.push(rowData);
		}
		rowIndex++;
	}
	return data;
}


function setRowClasses(table, enabled)
{
	rows = table.rows;
	rows[0].className = enabled==true ? 'header_row' : 'disabled header_row';

	rowIndex = 1;
	while(rowIndex < rows.length)
	{
		if(enabled==true)
		{
			rows[rowIndex].className = rowIndex % 2 == 0 ? 'even' : 'odd';
		}
		else
		{
			rows[rowIndex].className = rowIndex % 2 == 0 ? 'disabled even' : 'disabled odd';
		}

		cells = rows[rowIndex].childNodes;
		for(cellIndex = 0; cellIndex < cells.length; cellIndex++)
		{
			cellContent=cells[cellIndex].firstChild;
			if(cellContent.type == "button" )
			{
				setElementEnabled(cellContent, enabled);
			}
		}
		rowIndex++;
	}
	tableSanityCheck(table);
}


function createTableFilter()
{
	var args = arguments[0];
	var TFilter_Table = args[0];
	var TFilter_Cols = args[1];
	var TFilter_Store = args[2];
	var TFilter_Focus = args[3];
	
	if((TFilter_Table == "") || (TFilter_Table == null))
	{
		return;
	}

	var table = document.getElementById(TFilter_Table);
	var tableHeader = table.firstChild.nodeName == "THEAD" ? table.firstChild : null;
	var headerRow = tableHeader.firstChild;
	var numColsTab = headerRow.childElementCount;

	var filterRow = document.createElement('tr');
	filterRow.className = 'filter_row';
	for(var x = 0;x < numColsTab;x++)
	{
		var filterCol = document.createElement('th');
		var filterTxt = document.createElement('input');
		if(TFilter_Cols.length == 0)
		{
			filterTxt.disabled = true;
		}
		else if(x <= TFilter_Cols.length)
		{
			filterTxt.disabled = !TFilter_Cols[x];
		}
		if(TFilter_Store.length == 0)
		{
			filterTxt.value = "";
		}
		else if(x <= TFilter_Store.length)
		{
			filterTxt.value = TFilter_Store[x];
		}

		filterTxt.onkeyup = function(){applyTableFilter(this.parentNode.parentNode.parentNode.parentNode.id);};

		filterCol.appendChild(filterTxt);
		filterRow.appendChild(filterCol);
	}
	
	tableHeader.insertBefore(filterRow,headerRow);
	applyTableFilter(TFilter_Table);
	for(var x = 0; x < TFilter_Focus.length; x++)
	{
		if(TFilter_Focus[x] == true)
		{
			filterRow.children[x].firstChild.focus();
		}
	}
}

function storeTableFilter(tableID)
{
	if(tableID == null)
	{
		return;
	}

	var tableEl = document.getElementById(tableID);
	var filterRow = tableEl.firstChild.firstChild;
	if(filterRow.className == "filter_row")
	{
		for(var x = 0; x < filterRow.children.length; x++)
		{
			TFilter_Data[getTFilterIDX(tableID)][2][x] = filterRow.children[x].firstChild.value;
			TFilter_Data[getTFilterIDX(tableID)][3][x] = document.activeElement === filterRow.children[x].firstChild ? true : false;
		}
	}

	return;
}

function applyTableFilter(tableID)
{
	if(tableID == null)
	{
		return;
	}

	var TFilter_Data = [];

	var tableEl = document.getElementById(tableID);
	var filterRow = tableEl.firstChild.firstChild;
	if(filterRow.className == "filter_row")
	{
		for(var x = 0; x < filterRow.children.length; x++)
		{
			TFilter_Data[x] = filterRow.children[x].firstChild.value.toUpperCase();
		}
	}

	var tableBody = tableEl.lastChild;
	for(var x = 0; x < tableBody.children.length; x++)
	{
		var rowEl = tableBody.children[x];
		rowEl.style.display = "";
		for(var y = 0; y < rowEl.children.length; y++)
		{
			if(TFilter_Data[y] != "")
			{
				testVar = rowEl.children[y].firstChild.innerHTML == undefined ? rowEl.children[y].firstChild.data : rowEl.children[y].firstChild.innerHTML;
				if(testVar.toUpperCase().indexOf(TFilter_Data[y]) > -1)
				{
					rowEl.style.display = "";
				}
				else
				{
					rowEl.style.display = "none";
					break;
				}
			}
		}
	}

	return;
}


function getTFilterIDX(tableID)
{
	if(tableID == null)
	{
		return 0;
	}

	for(var x = 0; x < TFilter_Data.length; x++)
	{
		if(TFilter_Data[x][0] == tableID)
		{
			return x;
		}
	}

	return 0;
}



function tableSanityCheck(table)
{
	// If there are no rows of data (just a header)
	// then hide the whole thing until data is added
	table.style.display = (table.rows.length < 2) ? "none" : "";
}
