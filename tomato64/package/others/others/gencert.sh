#!/bin/sh
export PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/root


. nvram_ops

PID=$$
PIDFILE="/var/run/gencert.pid"
WAITTIMER=0

while [ -f "$PIDFILE" -a $WAITTIMER -lt 14 ]; do
	WAITTIMER=$((WAITTIMER+2))
	sleep $WAITTIMER
done
touch $PIDFILE

OPENSSL=/usr/sbin/openssl

LANCN=$(NG https_crt_cn)
LANIP=$(NG lan_ipaddr)
LANHOSTNAME=$(NG lan_hostname)
ROUTERNAME=$(NG router_name)
KEYNAME="key.pem"
CERTNAME="cert.pem"
OPENSSLCNF="/etc/openssl.config.$PID"

cd /etc

cp -L /etc/ssl/openssl.cnf $OPENSSLCNF

echo "0.commonName=CN" >> $OPENSSLCNF
echo "0.commonName_value=$LANIP" >> $OPENSSLCNF
echo "0.organizationName=O" >> $OPENSSLCNF
echo "0.organizationName_value=Tomato64" >> $OPENSSLCNF
echo "0.organizationalUnitName=OU" >> $OPENSSLCNF
echo "0.organizationalUnitName_value=Tomato64 Team" >> $OPENSSLCNF
echo "0.emailAddress=E" >> $OPENSSLCNF
echo "0.emailAddress_value=root@localhost" >> $OPENSSLCNF

# Required extension
sed -i "/\[ v3_ca \]/aextendedKeyUsage=serverAuth" $OPENSSLCNF

# Start of SAN extensions
sed -i "/\[ CA_default \]/acopy_extensions=copy" $OPENSSLCNF
sed -i "/\[ v3_ca \]/asubjectAltName=@alt_names" $OPENSSLCNF
sed -i "/\[ v3_ca \]/akeyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment" $OPENSSLCNF
sed -i "/\[ v3_req \]/asubjectAltName=@alt_names" $OPENSSLCNF
echo "[alt_names]" >> $OPENSSLCNF

I=1
# IP
echo "IP.$I = $LANIP" >> $OPENSSLCNF
echo "DNS.$I = $LANIP" >> $OPENSSLCNF # For broken clients like IE
I=$(($I + 1))

# User-defined SANs (if we have any)
[ "$LANCN" != "" ] && {
	for CN in $LANCN; do
		echo "DNS.$I = $CN" >> $OPENSSLCNF
		I=$(($I + 1))
	done
}

# hostnames
[ "$ROUTERNAME" != "" ] && {
	echo "DNS.$I = $ROUTERNAME" >> $OPENSSLCNF
	I=$(($I + 1))
}

[ "$LANHOSTNAME" != "" -a "$ROUTERNAME" != "$LANHOSTNAME" ] && {
	echo "DNS.$I = $LANHOSTNAME" >> $OPENSSLCNF
	I=$(($I + 1))
}

[ "$(date +%s)" -gt 946684800 ] && {
	NS https_crt_timeset=1
} || {
	NS https_crt_timeset=0
}

# create the key and certificate request
$OPENSSL req -new -out /tmp/cert.csr.$PID -keyout /tmp/privkey.pem.$PID -newkey rsa:2048 -passout pass:password -config $OPENSSLCNF
$OPENSSL ecparam -out $KEYNAME.$PID -name prime256v1 -genkey
$OPENSSL req -new -key $KEYNAME.$PID -out /tmp/cert.csr.$PID -config $OPENSSLCNF

# import the self-certificate
RANDFILE=/dev/urandom $OPENSSL req -x509 -new -nodes -in /tmp/cert.csr.$PID -key $KEYNAME.$PID -days 3653 -sha256 -out $CERTNAME.$PID -set_serial $1 -config $OPENSSLCNF

# server.pem for WebDav SSL
cat $KEYNAME.$PID $CERTNAME.$PID > server.pem

mv $KEYNAME.$PID $KEYNAME
mv $CERTNAME.$PID $CERTNAME

chmod 640 $KEYNAME

rm -f /tmp/cert.csr.$PID /tmp/privkey.pem.$PID $OPENSSLCNF $PIDFILE
