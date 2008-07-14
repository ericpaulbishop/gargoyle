/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


/**
* 
* split into otherLines, portForwards, and remoteAccepts
* return an array with
* 	index0=otherLines
*	index1=portForwards
*	index2=remoteAccepts
*
*
* otherLines is just an array of lines that doesn't belong
* to the forwards or accepts
*
*
* structure of portForwards is an array of arrays, each one 
* representing a forward:
* 	index0=name 
*	index1=protocol
* 	index2=multiport forward (true/false)
* 	index3=from port (start)
* 	index4=to port / end source port range (depending on whether this is a multi-port forward)
* 	index5=destination ip
* 	index6=enabled (true/false)
*
*
*
* remoteAccepts is an array of arrays, each one representing an accept:
*	index1=local port
*	index2=remote port
*
* 

*
* The structure of the file when it is saved will always be
* the other lines, the forwards, then the accepts
*
*
*/

function parseFirewallLines(firewallLines, routerIp)
{
	otherLines=[];
	portForwards=[];
	remoteAccepts=[];



	nonGargPortfLine1Hash = [];  // portf lines type 1, dnat in prerouting wan  (also may be part of remote accept)
	nonGargPortfLine2Hash = [];  // portf lines type 2, accept in forwarding wan
	
	parseLine1 = function(line1)
	{
		proto = line1.match(/[\t ]+\-p[\t ]+([^\t ]+)($|[\t ]+)/)[1]
		proto = proto.toUpperCase();

		fromPort=line1.match(/[\t ]+\-\-dport[\t ]+([^\t ]+)($|[\t ]+)/)[1];
		toData=line1.match(/[\t ]+\-\-to[\t ]+([^\t ]+)($|[\t ]+)/)[1];
		toIp=toData;
		toPort=fromPort;
		multi=false;
		if(toSplitMatch = toData.match(/(.*):(.*)$/))
		{
			toIp=toSplitMatch[1];
			toPort=toSplitMatch[2];
		}
		if(fromSplitMatch = fromPort.match(/(.*):(.*)$/))
		{
			multi=true;
			fromPort=fromSplitMatch[1];
			toPort=fromSplitMatch[2];
		}	
		return ["-",proto, multi, fromPort, toPort, toIp, true];
	}
	

	portfIndices = [];
	for(lineIndex = 0; lineIndex < firewallLines.length; lineIndex++)
	{
		line = firewallLines[lineIndex];
		if(line.match(/^###GARG-PORTF#/))
		{
			splitLine = line.split(/^###GARG-PORTF#/);
			info = splitLine.join("");
			numRules = info.substr(0,1);
			name = info.substr(2);
			portfIndices[lineIndex] = 1;
			portfIndices[lineIndex+1] = 1;
			portfIndices[lineIndex+2] = 1;

			lineIndex++;
			line1= firewallLines[lineIndex];
			line1Data=parseLine1(line1);			
			
			lineIndex++;
			line2=firewallLines[lineIndex];
			
			ruleEnabled = (!line1.match(/^[\t ]*#/)) && (!line2.match(/^[\t ]*#/));	
	
			line1Data[0] = name;
			line1Data[6] = ruleEnabled;
			if(numRules == 2)
			{
				portfIndices[lineIndex+1] = 1;
				portfIndices[lineIndex+2] = 1;
				line1Data[1] = 'Both';
				lineIndex++;
				lineIndex++;
			}

			portForwards.push(line1Data);
		
		}
		else if(line.match(/^[\t ]*iptables[\t ]+/) && 
			line.match(/[\t ]+\-t[\t ]+nat/) && 
			line.match(/[\t ]+\-p[\t ]+/) && 
			line.match(/[\t ]+\-A[\t ]+prerouting_wan/) && 
			line.match(/[\t ]+\-\-dport[\t ]+/) && 
			line.match(/[\t ]+\-j[\t ]+DNAT/) && 
			line.match(/[\t ]+\-\-to[\t ]+/) 
			)
		{
			line1Data=parseLine1(line);
			if(line1Data[2] == true)  //is it a port range?
			{
				//protocol - toIP - portRange
				nonGargPortfLine1Hash[ line1Data[1] + "-" + line1Data[5] + "-" + line1Data[3] + ":" + line1Data[4] ] = lineIndex;
			}
			else
			{
				//protocol - toIP - toPort
				nonGargPortfLine1Hash[ line1Data[1] + "-" + line1Data[5] + "-" + line1Data[4] ] = [lineIndex,line1Data];
			}
		}
		else if(line.match(/^[\t ]*iptables[\t ]+/) && 
			line.match(/[\t ]+\-A[\t ]+forwarding_wan/)  && 
			line.match(/[\t ]+\-p[\t ]+/) && 
			line.match(/[\t ]+\-\-dport[\t ]+/) && 
			line.match(/[\t ]+-d[\t ]+/) && 
			line.match(/[\t ]+\-j[\t ]+ACCEPT/)
			)
		{
			proto = line.match(/[\t ]+\-p[\t ]+([^\t ]+)($|[\t ]+)/)[1]
			proto = proto.toUpperCase();

			toPort=line.match(/[\t ]+\-\-dport[\t ]+([^\t ]+)($|[\t ]+)/)[1];
			toIp=line.match(/[\t ]+\-d[\t ]+([^\t ]+)($|[\t ]+)/)[1];
			
			nonGargPortfLine2Hash[ proto + "-" + toIp + "-" + toPort ] = lineIndex;
		}
	}
	

	for (rule in nonGargPortfLine2Hash)
	{
		line2Index=nonGargPortfLine2Hash[rule];
		if( (line1Info = nonGargPortfLine1Hash[rule]) != null && portfIndices[rule] == null)
		{

			line1Index=line1Info[0];
			line1Data=line1Info[1];			

			portfIndices[line1Index] = 1;
			portfIndices[line2Index] = 1;		

			proto = line1Data[1];
			toIp  = line1Data[5];
			toPort= line1Data[2] ?  line1Data[3] + ":" + line1Data[4] : line1Data[4];
			
			otherProto = proto == 'TCP' ? 'UDP' : 'TCP';
			otherRule = otherProto + "-" + toIp + "-" + toPort ;
			if(nonGargPortfLine1Hash[otherRule] != null && nonGargPortfLine2Hash[otherRule] != null)
			{
				portfIndices[ nonGargPortfLine1Hash[otherRule][0] ] = 1;
				portfIndices[ nonGargPortfLine2Hash[otherRule] ] = 1;
				line1Data[1] = 'Both';
				
			}
			portForwards.push(line1Data);
		}
	}





	//local port -> [ aray of line types, can be: pre_accept-index, pre_reject-index, pre_forward-index-remoteport, input_accept-index ]
	//for now we are going to assume that connection protocol of all ports we are accepting on the router is tcp
	//the only ones that really matter (e.g. are configured in gargoyle) are ssh, http and https all of which are tcp
	acceptHash=[];
	
	routerRegex = new RegExp(routerIp + ":([0-9]+)[\t ]*$");
	for(lineIndex = 0; lineIndex < firewallLines.length; lineIndex++)
	{
		if(portfIndices[lineIndex] == null)
		{
			line = firewallLines[lineIndex];
			if(	line.match(/^[\t ]*iptables[\t ]+/) && 
				line.match(/[\t ]+\-t[\t ]+nat/) && 
				line.match(/[\t ]+\-p[\t ]+tcp/) && 
				line.match(/[\t ]+\-A[\t ]+prerouting_wan/) && 
				line.match(/[\t ]+\-\-dport[\t ]*/) &&
				line.match(/[\t ]+\-j[\t ]+ACCEPT/)
			)
			{
				localPort=line.match(/[\t ]+\-\-dport[\t ]+([^\t ]+)($|[\t ]+)/)[1];
				if(acceptHash[localPort] == null)
				{
					acceptHash[localPort] = [];
				}
				acceptHash[localPort].push( "pre_accept-" + lineIndex);
			}
			else if(	line.match(/^[\t ]*iptables[\t ]+/) && 
					line.match(/[\t ]+\-t[\t ]+nat/) && 
					line.match(/[\t ]+\-p[\t ]+tcp/) && 
					line.match(/[\t ]+\-A[\t ]+prerouting_wan/) && 
					line.match(/[\t ]+\-\-dport[\t ]*/) &&
					(line.match(/[\t ]+\-j[\t ]+REJECT/) ||  line.match(/[\t ]+\-j[\t ]+DROP/))
			)
			{
				localPort=line.match(/[\t ]+\-\-dport[\t ]+([^\t ]+)($|[\t ]+)/)[1];
				if(acceptHash[localPort] == null)
				{
					acceptHash[localPort] = [];
				}
				acceptHash[localPort].push( "pre_reject-" + lineIndex);

			}
			else if(	line.match(/^[\t ]*iptables[\t ]+/) && 
					line.match(/[\t ]+\-p[\t ]+tcp/) && 
					line.match(/[\t ]+\-A[\t ]+input_wan/) && 
					line.match(/[\t ]+\-\-dport[\t ]*/) &&
					line.match(/[\t ]+\-j[\t ]+ACCEPT/)
			)
			{	
				localPort=line.match(/[\t ]+\-\-dport[\t ]+([^\t ]+)($|[\t ]+)/)[1];
				if(acceptHash[localPort] == null)
				{
					acceptHash[localPort] = [];
				}
				acceptHash[localPort].push( "input_accept-" + lineIndex);

			}
			else if(	line.match(/^[\t ]*iptables[\t ]+/) && 
					line.match(/[\t ]+\-t[\t ]+nat/) && 
					line.match(/[\t ]+\-p[\t ]+/) && 
					line.match(/[\t ]+\-A[\t ]+prerouting_wan/) && 
					line.match(/[\t ]+\-\-dport[\t ]+/) && 
					line.match(/[\t ]+\-j[\t ]+DNAT/) && 
					line.match(/[\t ]+\-\-to[\t ]+/) &&
					line.match(routerRegex)
			)
			{
				remotePort=line.match(/[\t ]+\-\-dport[\t ]+([^\t ]+)($|[\t ]+)/)[1];
				localPort=line.match(routerRegex)[1];
				if(acceptHash[localPort] == null)
				{
					acceptHash[localPort] = [];
				}
				acceptHash[localPort].push( "pre_forward-" + lineIndex + "-" + remotePort);
			}
		}
	}

	acceptIndices=[]
	for( localPort in acceptHash )
	{
		remotePort = -1;

		rejectPreIndex = -1;
		acceptPreIndex = -1;
		forwardPreIndex = -1;
		acceptInputIndex = -1;
		localPortAcceptIndices=[];
		
		portLines=acceptHash[localPort];

		for(lineIndex = 0; lineIndex < portLines.length; lineIndex++)
		{
			line =  portLines[lineIndex];
			if( rejectMatch = line.match(/^pre_reject\-(.*)$/) )
			{
				rejectPreIndex = lineIndex;
				localPortAcceptIndices.push(rejectMatch[1]);
			}
			else if(acceptMatch =  line.match(/^pre_accept\-(.*)$/))
			{
				acceptPreIndex = lineIndex;
				localPortAcceptIndices.push(acceptMatch[1]);

			}
			else if( inputMatch = line.match(/^input_accept\-(.*)$/))
			{
				acceptInputIndex = lineIndex;
				localPortAcceptIndices.push(inputMatch[1]);
			}
			else if( forwardMatch= line.match(/pre_forward\-([^\-]+)\-(.*)$/))
			{
				forwardPreIndex = lineIndex;
				localPortAcceptIndices.push(forwardMatch[1]);
				remotePort = forwardMatch[2];
			}
		}
		if(acceptInputIndex >= 0)
		{
			if(acceptPreIndex >= 0)
			{
				remotePort = localPort;
			}
			//else remote port is defined in forward
			
			if(remotePort > 0)
			{
				remoteAccepts.push([localPort, remotePort]);
				for(lpaiIndex=0; lpaiIndex < localPortAcceptIndices.length; lpaiIndex++)
				{
					acceptIndices[ localPortAcceptIndices[lpaiIndex] ] = 1;
				}
			}	
		}
	}

	for(lineIndex = 0; lineIndex < firewallLines.length; lineIndex++)
	{
		if(portfIndices[lineIndex] == null && acceptIndices[lineIndex] == null)
		{
			line = firewallLines[lineIndex];
			if(line != '')
			{
				otherLines.push(line);
			}
		}
	}

	return [otherLines, portForwards, remoteAccepts];
}


function getFirewallWriteCommands(firewallData, routerIp)
{
	otherLines=firewallData[0];
	portForwards=firewallData[1];
	remoteAccepts=firewallData[2];


	firewallLines = [];
	for(otherIndex = 0; otherIndex < otherLines.length; otherIndex++)
	{
		firewallLines.push(otherLines[otherIndex]);
	}
	firewallLines.push("");
	firewallLines.push("");

	/*
	* 	index0=name 
	*	index1=protocol
	* 	index2=multiport forward (true/false)
	* 	index3=from port (start)
	* 	index4=to port / end source port range (depending on whether this is a multi-port forward)
	* 	index5=destination ip
	* 	index6=enabled (true/false)
	*/
	for(portfIndex = 0; portfIndex < portForwards.length; portfIndex++)
	{
		portf = portForwards[portfIndex];
		protocol = portf[1].toLowerCase();
		numRules = protocol == "both" ? 2 : 1;
		dnatTo = portf[2] ? portf[5] : portf[5] + ":" + portf[4];
		dport  = portf[2] ? portf[3] + ":" + portf[4] : portf[3];
		newDPort = portf[2] ? portf[3] + ":" + portf[4] : portf[4];
		
		comment = portf[6] ? "" : "#";
		firewallLines.push( "###GARG-PORTF#" + numRules + "#" + portf[0] );

		if( protocol == "tcp" || protocol == "both")
		{
			firewallLines.push( comment + "iptables -t nat -A prerouting_wan -p tcp --dport " + dport + " -j DNAT --to " + dnatTo);
			firewallLines.push( comment + "iptables        -A forwarding_wan -p tcp --dport " + newDPort + " -j ACCEPT" );
		}
		if( protocol == "udp" || protocol == "both")
		{
			firewallLines.push( comment + "iptables -t nat -A prerouting_wan -p udp --dport " + dport + " -j DNAT --to " + dnatTo);
			firewallLines.push( comment + "iptables        -A forwarding_wan -p udp --dport " + newDPort + "  -j ACCEPT" );
		}
		firewallLines.push("");
		firewallLines.push("");
	}


	/*
	*	index1=local port
	*	index2=remote port	
	*/
	for(acceptIndex = 0; acceptIndex < remoteAccepts.length; acceptIndex++)
	{
		accept = remoteAccepts[acceptIndex];
		if(accept[0] == accept[1])
		{
			firewallLines.push("iptables -t nat -A prerouting_wan -p tcp --dport " + accept[0] + " -j ACCEPT");
			firewallLines.push("iptables        -A input_wan      -p tcp --dport " + accept[0] + " -j ACCEPT");	
		}
		else
		{
			firewallLines.push("iptables -t nat -A prerouting_wan -p tcp --dport " + accept[0] + " -j DROP");
			firewallLines.push("iptables -t nat -A prerouting_wan -p tcp --dport " + accept[1] + " -j DNAT --to " + routerIp + ":" + accept[0]);
			firewallLines.push("iptables        -A input_wan      -p tcp --dport " + accept[0] + " -j ACCEPT");
		}
		firewallLines.push("");
		firewallLines.push("");
	}
	
	
	commands = ["touch /etc/firewall.user", "rm /etc/firewall.user"];
	for(firewallIndex = 0; firewallIndex < firewallLines.length; firewallIndex++)
	{
		commands.push("echo \'" + firewallLines[firewallIndex] + "\' >> /etc/firewall.user");
	}

	return commands;
}







