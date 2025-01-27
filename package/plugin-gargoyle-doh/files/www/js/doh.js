/*
 * This program is copyright © 2024 Michael Gray and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var DoH=new Object();
var uci;

function setProviderSelect()
{
	var vals = [];
	var names = [];

	var tabData = getTableDataArray(byId('doh_configured_instances_table'), true, false);
	var hideNames = tabData.map(function(tabRow){return tabRow[0]});

	providerData.forEach(function(provider,idx) {
		if(hideNames.indexOf(provider.title) < 0 && (provider.http2_only == undefined || curlhttp2))
		{
			vals.push(idx);
			names.push(provider.title);
		}
	});

	setAllowableSelections('select_provider',vals,names);
	setProviderOptionSelect();
}

function setProviderOptionSelect()
{
	var currentSel = byId('select_provider').value;
	var vals = [];
	var names = [];
	var defaultSel = '';
	byId('text_provider_option').value="";
	var params = providerData[currentSel].params;
	if(params != undefined)
	{
		byId('provider_option_label').innerText = translateJsonDescription(params.option.description) + ':';
		if(params.option.type == 'select')
		{
			byId('select_provider_option').style="display:block";
			byId('text_provider_option').style="display:none";
			params.option.options.forEach(function(providerOption) {
				vals.push(providerOption.value);
				names.push(providerOption.description);
			});
			defaultSel = params.option.default;
		}
		else if(params.option.type == 'text')
		{
			byId('select_provider_option').style="display:none";
			byId('text_provider_option').style="display:block";
		}
	}
	else
	{
		byId('provider_option_label').innerText = '';
		vals.push('');
		names.push('-');
		byId('select_provider_option').style="display:block";
		byId('text_provider_option').style="display:none";
	}

	setAllowableSelections('select_provider_option',vals,names);
	byId('select_provider_option').value = defaultSel;
}

function setStatusText()
{
	var statusText = DoH.NRun;
	var statusColour = '#880000';

	if(serviceRunning)
	{
		statusText = DoH.Run + '. ' + DoH.Ports + ': ';
		statusText = statusText + serviceRunningPorts.replace(' ',',');
		statusColour = '#008800';
	}

	setChildText('hdp_status', statusText, statusColour, true, null, document);
}

function resetData()
{
	uci = uciOriginal.clone();

	var enabled = uciOriginal.get('https-dns-proxy','config','enabled');
	byId('doh_enable').checked = enabled == '1';

	var canaryicloud = uciOriginal.get('https-dns-proxy','config','canary_domains_icloud');
	byId('icloud_canary_domain').value = canaryicloud;

	var canarymozilla = uciOriginal.get('https-dns-proxy','config','canary_domains_mozilla');
	byId('mozilla_canary_domain').value = canarymozilla;

	var confInstData = [];
	var uciConfInsts = uciOriginal.getAllSectionsOfType('https-dns-proxy','https-dns-proxy');
	uciConfInsts.forEach(function(uciConfInst) {
		var resolver = 'Custom'; // FIXME I18N
		var resolver_option = '-';
		var resolver_url = uciOriginal.get('https-dns-proxy',uciConfInst,'resolver_url');
		var port = uciOriginal.get('https-dns-proxy',uciConfInst,'listen_port');
		var bootstrap_dns = '8.8.8.8,8.8.4.4';

		// Match predefined resolvers
		providerData.some(function(provider) {
			var pattern = templateToRegex(provider);
			var patmatch = resolver_url.match(pattern);
			//if(pattern.test(resolver_url))
			if(patmatch != null)
			{
				resolver = provider.title;
				if(patmatch.length > 1 && patmatch[1] != '')
				{
					resolver_option = resolverOptionDescription(provider, patmatch[1]);
				}
				bootstrap_dns = provider.bootstrap_dns;
				return true;
			}
		});

		confInstData.push([resolver, resolver_option, resolver_url, port, bootstrap_dns]);
	});
	var tableContainer = byId('doh_configured_instances_table_container');
	// FIXME I18N
	var confInstTable = createTable(['Resolver','Resolver Option','Resolver URL','Port','Bootstrap DNS'], confInstData, 'doh_configured_instances_table', true, false, setProviderSelect);
	while(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(confInstTable);

	setProviderSelect();

	setStatusText();
}

function saveChanges()
{
	setControlsEnabled(false, true);

	uci.set('https-dns-proxy','config','enabled', byId('doh_enable').checked ? '1' : '0');
	uci.set('https-dns-proxy','config','canary_domains_icloud',byId('icloud_canary_domain').value);
	uci.set('https-dns-proxy','config','canary_domains_mozilla',byId('mozilla_canary_domain').value);

	sections = uciOriginal.getAllSectionsOfType('https-dns-proxy','https-dns-proxy');
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		uci.removeSection('https-dns-proxy',sections[sectionIndex]);
	}

	var tabData = getTableDataArray(byId('doh_configured_instances_table'), true, false);
	tabData.forEach(function(tabRow, idx) {
		var uciId = idx + 1;
		uci.set('https-dns-proxy','doh_resolver_' + uciId, '', 'https-dns-proxy');
		uci.set('https-dns-proxy','doh_resolver_' + uciId, 'bootstrap_dns', tabRow[4]);
		uci.set('https-dns-proxy','doh_resolver_' + uciId, 'resolver_url', tabRow[2]);
		uci.set('https-dns-proxy', 'doh_resolver_' + uciId, 'listen_addr', '127.0.0.1');
		uci.set('https-dns-proxy', 'doh_resolver_' + uciId, 'listen_port', tabRow[3]);
		uci.set('https-dns-proxy', 'doh_resolver_' + uciId, 'user', 'nobody');
		uci.set('https-dns-proxy', 'doh_resolver_' + uciId, 'group', 'nogroup');
	});

	commands = uci.getScriptCommands(uciOriginal);

	commands = commands + '\n' + '/etc/init.d/https-dns-proxy restart';

	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			uciOriginal = uci.clone();
			resetData();
			setControlsEnabled(true);
			window.location.href = window.location.href;
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function templateToRegex(provider)
{
	var retVal = provider.template;
	if(provider.params != undefined)
	{
		Object.keys(provider.params).forEach(function(paramName) {
			retVal = retVal.replace('{' + paramName + '}', provider.params[paramName].regex);
		});
	}
	retVal = retVal.replaceAll('/','\\/');
	retVal = retVal.replaceAll('.','\\.');
	retVal = new RegExp(retVal);

	return retVal;
}

function resolverOptionDescription(provider, optionval)
{
	var retVal = optionval;
	if(provider.params != undefined)
	{
		Object.keys(provider.params).forEach(function(paramName) {
			provider.params[paramName].options.some(function(paramOption) {
				if(paramOption.value == optionval)
				{
					retVal = paramOption.description;
					return true;
				}
			});
		});
	}

	return retVal;
}

function addDoHResolver()
{
	var providerId = getSelectedValue('select_provider');
	var providerOption = '';
	var providerOptionName = '-';

	var provider = providerData[providerId];
	if(provider.params != undefined)
	{
		if(provider.params.option.type == 'select')
		{
			providerOption = getSelectedValue('select_provider_option');
			providerOptionName = getSelectedText('select_provider_option');
		}
		else if(provider.params.option.type == 'text')
		{
			providerOption = byId('text_provider_option').value;
			providerOptionName = providerOption;
		}
	}

	var portNum = '5053';
	var tabData = getTableDataArray(byId('doh_configured_instances_table'), true, false);
	var ports = tabData.map(function(tabRow) {return tabRow[3]});
	while(ports.indexOf(portNum) >= 0)
	{
		portNum = (portNum - -1).toString();
	}

	//['Resolver','Resolver Option','Resolver URL','Port','Bootstrap DNS']
	var rowData = [provider.title, providerOptionName, provider.template.replace('{option}',providerOption),portNum.toString(),provider.bootstrap_dns];
	addTableRow(byId('doh_configured_instances_table'),rowData,true,false,setProviderSelect);

	setProviderSelect();
}

function translateJsonDescription(str)
{
	var i18nMap = [];
	i18nMap['Filter'] = DoH.Filter;
	i18nMap['Filters'] = DoH.Filters;
	i18nMap['Variant'] = DoH.Variant;
	i18nMap['Username'] = DoH.Username;
	i18nMap['Location'] = DoH.Location;

	return i18nMap[str] == undefined ? 'Undefined' : i18nMap[str];
}
