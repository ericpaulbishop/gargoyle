/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var connLS=new Object(); //part of i18n

function saveChanges()
{
	var errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);
		
		maxConnections = document.getElementById("max_connections").value;
		tcpTimeout = document.getElementById("tcp_timeout").value;
		udpTimeout = document.getElementById("udp_timeout").value;

		var commands = [];
		commands.push("cp /etc/sysctl.conf /tmp/sysctl.conf.tmp.1");
		var addParameterCommands = function(parameterFile, parameterId, parameterValue)
		{
			var parameterName = parameterId.replace(/^.*\./g, "");
			commands.push("echo " + parameterValue + " > " + parameterFile);
			commands.push("cat /tmp/sysctl.conf.tmp.1 | grep -v \"^" + parameterId + "=\" > /tmp/sysctl.conf.tmp.2");
			commands.push("echo \"" + parameterId + "=" + parameterValue + "\" >> /tmp/sysctl.conf.tmp.2");
			commands.push("mv /tmp/sysctl.conf.tmp.2 /tmp/sysctl.conf.tmp.1");
		}
		addParameterCommands("/proc/sys/net/netfilter/nf_conntrack_max", "net.netfilter.nf_conntrack_max", maxConnections);
		addParameterCommands("/proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established", "net.netfilter.nf_conntrack_tcp_timeout_established", tcpTimeout);
		addParameterCommands("/proc/sys/net/netfilter/nf_conntrack_udp_timeout_stream", "net.netfilter.nf_conntrack_udp_timeout_stream", udpTimeout);
		commands.push("mv /tmp/sysctl.conf.tmp.1 /etc/sysctl.conf");	

		var param = getParameterDefinition("commands", commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				resetData();
				setControlsEnabled(true);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function proofreadAll()
{
	var testConnMax = function(text){ return validateNumericRange(text,1,16384); }
	var testTimeout = function(text){ return validateNumericRange(text,1,3600); };
	var fields = ["max_connections", "tcp_timeout", "udp_timeout"];
	var labels = ["max_connections_label", "tcp_timeout_label", "udp_timeout_label"];
	return proofreadFields( fields, labels, [testConnMax,testTimeout,testTimeout], [0,0,0], fields); 
}

function resetData()
{
	document.getElementById("max_connections").value = maxConnections;
	document.getElementById("tcp_timeout").value = tcpTimeout;
	document.getElementById("udp_timeout").value = udpTimeout;
}
