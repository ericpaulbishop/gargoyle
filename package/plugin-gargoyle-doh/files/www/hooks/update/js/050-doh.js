function setDoHVisibility()
{
	byId('dohupservwarning').style = dohenabled == '1' ? 'display:block' : 'display:none';
	updateServiceWarningCount += dohenabled == '1' ? 1 : 0;
	setUpgradeServiceWarningVisibility();
}

addLoadFunction(setDoHVisibility);
