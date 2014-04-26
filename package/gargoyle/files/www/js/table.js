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
	var tableBody = controlDocument.createElement('tbody');
	newTable.appendChild(tableBody);
	newTable.id=tableId;

	var row = controlDocument.createElement('tr');
	row.className='header_row';
	tableBody.appendChild(row);
	
	var columnIndex;
	for(columnIndex=0; columnIndex < columnNames.length; columnIndex++)
	{
		var header = controlDocument.createElement('th');
		if( typeof(columnNames[columnIndex]) == 'string' )
		{
			var splitText = (columnNames[columnIndex]).replace(/\&amp;/g, '&').split(/\n/)
			while(splitText.length > 0)
			{
				var next = splitText.shift()
				header.appendChild(  controlDocument.createTextNode(next)  )
				if(splitText.length >0)
				{
					header.appendChild( controlDocument.createElement('br')  )
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
		for(i=1; i<2; i++)
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
	tableBody=table.firstChild;
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
		cellContent.value = UI.Remove;
		cellContent.className="default_button";
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
		cellContent.value = String.fromCharCode(8593);
		cellContent.className="default_button";
		cellContent.onclick= function() { moveThisCellsRowUp(this); rowMoveCallback(this, "up"); };
		cell = controlDocument.createElement('td');
		cell.className=table.id + '_column_' + cellIndex;
		cell.appendChild(cellContent);
		row.appendChild(cell);
		cellIndex++;

		cellContent = createInput("button", controlDocument);
		cellContent.value = String.fromCharCode(8595);
		cellContent.className="default_button";
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
	rowIndex = 1;
	while(rowIndex < allRows.length && allRows[rowIndex] != row ) { rowIndex++; }
	if(rowIndex > 1)
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
	rowIndex = 1;
	while(rowIndex < allRows.length && allRows[rowIndex] != row ) { rowIndex++; }
	if(rowIndex < allRows.length-1 && allRows.length > 2 )
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
	rows[0].className = enabled==true ? 'header_row' : 'disabled_header_row';

	rowIndex = 1;
	while(rowIndex < rows.length)
	{
		if(enabled==true)
		{
			rows[rowIndex].className = rowIndex % 2 == 0 ? 'even' : 'odd';
		}
		else
		{
			rows[rowIndex].className = rowIndex % 2 == 0 ? 'disabled_even' : 'disabled_odd';
		}
		
		cells = rows[rowIndex].childNodes;
		for(cellIndex = 0; cellIndex < cells.length; cellIndex++)
		{
			cellContent=cells[cellIndex].firstChild;
			if(cellContent.type == "button" )
			{
				cellContent.disabled = (enabled == false);
				cellContent.className = (enabled == false) ? "default_button_disabled" : "default_button";
			}
		}		
		rowIndex++;
	}
	tableSanityCheck(table);
}



function tableSanityCheck(table)
{
	// If there are no rows of data (just a header)
	// then hide the whole thing until data is added
	table.style.display = (table.rows.length < 2) ? "none" : "block";
}
