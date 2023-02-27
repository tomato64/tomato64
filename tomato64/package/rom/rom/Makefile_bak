SHELL := /bin/bash
include ../common.mak

all:
	@echo "" >rom/etc/motd
	@echo "" >>rom/etc/motd
	@echo "FreshTomato `cat ../shared/tomato_version`" >>rom/etc/motd

clean:
	rm -f rom/etc/motd

certs: dummy
	others/mk-ca-bundle.pl -fu rom/etc/ssl/certs/ca-certificates.crt || true

certs-install: certs
	install -d $(INSTALLDIR)/rom/etc/ssl/certs
	install -m 0644 rom/etc/ssl/certs/ca-certificates.crt $(INSTALLDIR)/rom/etc/ssl/certs/ca-certificates.crt
	sed -r '/^\s*(#|$$)/d;/\-BEGIN/{h;d};H;/\-END/{x;/\-BEGIN/p};d' -i $(INSTALLDIR)/rom/etc/ssl/certs/ca-certificates.crt
	ln -s certs/ca-certificates.crt $(INSTALLDIR)/rom/etc/ssl/cert.pem

install:
	mkdir -p $(INSTALLDIR)
	( tar --exclude=.svn --exclude=*.bak -cpf - rom ) | ( cd $(INSTALLDIR) && tar -xpf - )

	# remove comments, compress whitespace
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/services > $(INSTALLDIR)/rom/etc/services
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/protocols > $(INSTALLDIR)/rom/etc/protocols

ifneq ($(TCONFIG_STUBBY),y)
	rm -f $(INSTALLDIR)/rom/etc/ssl/certs/ca-certificates.crt
else
ifneq ($(TCONFIG_IPV6),y)
	install -m 0644 others/dot-servers-ipv4.dat $(INSTALLDIR)/rom/dot-servers.dat
else
	install -m 0644 others/dot-servers.dat $(INSTALLDIR)/rom/dot-servers.dat
endif
	$(MAKE) certs-install
endif

ifeq ($(TCONFIG_HTTPS),y)
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/ssl/openssl.cnf > $(INSTALLDIR)/rom/etc/ssl/openssl.cnf
else
	rm -f $(INSTALLDIR)/rom/etc/ssl/openssl.cnf
endif

ifneq ($(TCONFIG_DNSSEC),y)
	rm -f $(INSTALLDIR)/rom/etc/trust-anchors.conf
endif

ifneq ($(TCONFIG_DNSCRYPT),y)
	rm -f $(INSTALLDIR)/rom/etc/dnscrypt-resolvers.csv
else
	@resolvers_path=$(INSTALLDIR)"/rom/etc"; \
	resolvers_md="$${resolvers_path}/public-resolvers.md"; \
	resolvers_file="$${resolvers_path}/dnscrypt-resolvers.csv"; \
	resolvers_file_tmp="$${resolvers_file}.tmp"; \
	resolvers_url="https://download.dnscrypt.info/resolvers-list/v2/public-resolvers.md"; \
	count=0; \
	while [ $${count} -lt 6 ]; do \
	    printf "\nRetrieving the list of public DNSCrypt resolvers...\n"; \
	    /usr/bin/wget --secure-protocol=TLSv1_2 -T 10 -t 1 "$${resolvers_url}" -O "$${resolvers_md}"; \
	    if [ $${?} -ne 0 ]; then \
		count=`expr $${count} + 1`; \
		if [ $${count} -gt 5 ]; then \
		    err_file="../../error_download_dnscrypt_resolvers"; \
		    ret=$$(date)": DNSCrypt resolvers list download error, check your internet connection! Using recently downloaded file!"; \
		    printf "\n\n"$${ret}"\n\n"; \
		    echo $${ret} >> $${err_file}; \
		    cp rom/etc/dnscrypt-resolvers.csv $${resolvers_file}; \
		    if [ $${?} -ne 0 ]; then \
			ret=$$(date)": No local copy of the dnscrypt-resolvers.csv file has been found, aborting!"; \
			printf "\n\n"$${ret}"\n\n"; \
			echo $${ret} >> $${err_file}; \
			exit 1; \
		    fi; \
		fi; \
		sleep 2; \
	    else \
		# Parse the v2 resolvers file into v1 csv (based on John Bacho script: https://github.com/john9527/asuswrt-merlin/commit/35858017ed807d7f087cd0318d32fe2889eae23a); \
		printf "\nParsing the list of public DNSCrypt resolvers...please wait\n"; \
		sdns_decode="/tmp/sdns_decode.dat"; \
		echo "Name,\"Full name\",\"Description\",\"Location\",\"Coordinates\",URL,Version,DNSSEC validation,No logs,Namecoin,Resolver address,Provider name,Provider public key,Provider public key TXT record" > $${resolvers_file_tmp}; \
\
		oldifs=$${IFS}; \
		IFS=$$'\n'; \
		for sdns in $$(cat $${resolvers_md} | grep -E '^sdns|^##'); do \
		    echo $${sdns} | grep -q "##" >/dev/null 2>&1; \
		    if [ $${?} -eq 0 ]; then \
			desc=$$(echo $${sdns} | sed -r 's/.{3}//'); \
			continue; \
		    fi; \
\
		    sdns=$$(echo $${sdns} | sed -r 's/.{7}//'); \
		    echo $${sdns}== | tr -- "-_ " "+/=" | base64 -d > $${sdns_decode}; \
\
		    type=$$(dd bs=1 count=1 skip=0 <$${sdns_decode} 2>/dev/null | hexdump -e '1/1 "%d"'); \
		    if [ $${type} -ne 1 ]; then continue; fi; \
\
		    props=$$(dd bs=1 count=1 skip=1 <$${sdns_decode} 2>/dev/null | hexdump -e '1/1 "%d"'); \
		    if [ $$(( $${props}&1 )) -eq 1 ]; then dnssec="yes"; else dnssec="no"; fi; \
		    if [ $$(( $${props}&2 )) -eq 2 ]; then logs="yes"; else logs="no"; fi; \
		    if [ $$(( $${props}&4 )) -eq 4 ]; then filter="yes"; else filter="no"; fi; \
\
		    addr_length=$$(dd bs=1 count=1 skip=9 <$${sdns_decode} 2>/dev/null | hexdump -e '1/1 "%d"'); \
		    address=$$(dd bs=1 count=$${addr_length} skip=10 <$${sdns_decode} 2>/dev/null); \
\
		    let pk_length_offset=10+$${addr_length}; \
		    pk_length=$$(dd bs=1 count=1 skip=$${pk_length_offset} <$${sdns_decode} 2>/dev/null | hexdump -e '1/1 "%d"'); \
		    let pk_offset=$${pk_length_offset}+1; \
		    pk=$$(dd bs=1 count=$${pk_length} skip=$${pk_offset} <$${sdns_decode} 2>/dev/null | hexdump -e '1/2 "%04X:"' | sed 's/\(.\)\(.\)\(.\)\(.\)\(.\)/\3\4\1\2\5/g' | sed 's/.$$//'); \
\
		    let name_length_offset=$${pk_offset}+$${pk_length}; \
		    name_length=$$(dd bs=1 count=1 skip=$${name_length_offset} <$${sdns_decode} 2>/dev/null | hexdump -e '1/1 "%d"'); \
		    let name_offset=$${name_length_offset}+1; \
		    name=$$(dd bs=1 count=$${name_length} skip=$${name_offset} <$${sdns_decode} 2>/dev/null); \
\
		    echo $${name} | grep -q "cryptostorm" >/dev/null 2>&1; \
		    if [ $${?} -eq 0 ]; then namecoin="yes"; else namecoin="no"; fi; \
\
		    echo "$${desc},$${desc},,,,,1,$${dnssec},$${logs},$${namecoin},$${address},$${name},$${pk}," >> $${resolvers_file_tmp}; \
		done; \
		IFS=$${oldifs}; \
		mv -f "$${resolvers_file_tmp}" "$${resolvers_file}" > /dev/null 2>&1; \
		rm -rf $${sdns_decode}; \
		rm -f $${resolvers_md}; \
		printf "\nDone\n\n"; \
		count=6; \
	    fi; \
	done
endif

.PHONY: dummy
