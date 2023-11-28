function h_countbitsfromleftv6(num) {
	if (num == 0xffff)
		return(16);

	var i = 0;
	var bitpat = 0xffff0000; 
	while (i < 16) {
		if (num == (bitpat & 0xffff))
			return(i);

		bitpat = bitpat >> 1;
		i++;
	}
	return(Number.NaN);
}

function numberOfBitsOnNetMaskV6(netmask) {
	var total = 0;
	var t = netmask.split(':');
	for (var i = 0; i <= 7 ; i++)
		total += h_countbitsfromleftv6(t[i]);

	return total;
}

function getNetworkAddressV6(ipaddress, netmask) {
	return fixIPv6(ntoa(atonv6(ipaddress) & atonv6(netmask)));
}

function getBroadcastAddressV6(network, netmask) {
	return fixIPv6(ntoa(atonv6(network) ^ (~ atonv6(netmask))));
}

function getAddressV6(ipaddress, network) {
	return fixIPv6(ntoa((atonv6(network)) + (atonv6(ipaddress))));
}

function fixIPv6(ip, x) {
	var a, n, i;
	a = ip;
	i = a.indexOf("<br>");
	if (i > 0)
		a = a.slice(0,i);

	a = a.split(':');
	if (a.length != 8) return null;
	for (i = 0; i < 8; ++i) {
		n = a[i] * 1;
		if ((isNaN(n)) || (n < 0) || (n > 0xffff)) return null;
		a[i] = n;
	}
	if ((x) && ((a[7] == 0) || (a[7] == 0xffff))) return null;
	return a.join(':');
}