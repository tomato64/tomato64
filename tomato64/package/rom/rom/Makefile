SHELL := /bin/bash

INSTALLDIR=$(TARGET_DIR)

all:
	rm rom/etc/ld.so.conf

	@echo "" >rom/etc/motd
	@echo "" >>rom/etc/motd
	@echo "Tomato64 `cat $(BUILD_DIR)/libshared-1.0/tomato_version` " >>rom/etc/motd

clean:
	rm -f rom/etc/motd

certs: dummy
	others/mk-ca-bundle.pl -fu rom/etc/ssl/certs/ca-certificates.crt || true

certs-install: certs
	install -d $(INSTALLDIR)/rom/etc/ssl/certs
	install -m 0644 rom/etc/ssl/certs/ca-certificates.crt $(INSTALLDIR)/rom/etc/ssl/certs/ca-certificates.crt
	sed -r '/^\s*(#|$$)/d;/\-BEGIN/{h;d};H;/\-END/{x;/\-BEGIN/p};d' -i $(INSTALLDIR)/rom/etc/ssl/certs/ca-certificates.crt
	ln -sf certs/ca-certificates.crt $(INSTALLDIR)/rom/etc/ssl/cert.pem

install:
#	mkdir -p $(INSTALLDIR)
	( tar --exclude=.svn --exclude=*.bak -cpf - rom ) | ( cd $(INSTALLDIR) && tar -xpf - )

	# remove comments, compress whitespace
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/services > $(INSTALLDIR)/rom/etc/services
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/protocols > $(INSTALLDIR)/rom/etc/protocols

ifneq ($(TCONFIG_STUBBY),y)
	rm -f $(INSTALLDIR)/rom/etc/ssl/certs/ca-certificates.crt
	rm -f $(INSTALLDIR)/rom/etc/getdns-root.key
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
	@resolvers_path="$(INSTALLDIR)/rom/etc"; \
	resolvers_file="$${resolvers_path}/dnscrypt-resolvers.csv"; \
	resolvers_download="$${resolvers_file}.download"; \
	resolvers_signature="$${resolvers_download}.minisig"; \
	resolvers_file_tmp="$${resolvers_file}.tmp"; \
	resolvers_url="https://raw.githubusercontent.com/DNSCrypt/dnscrypt-resolvers/master/v1/dnscrypt-resolvers.csv"; \
	resolvers_signature_url="$${resolvers_url}.minisig"; \
	minisign_key="RWQf6LRCGA9i53mlYecO4IzT51TGPpvWucNSCh1CBM0QTaLn73Y7GFO3"; \
	err_file="../../error_download_dnscrypt_resolvers"; \
	use_local_copy() { \
	    ret="$$(date): $$1 Using bundled dnscrypt-resolvers.csv file."; \
	    printf "\n\n%s\n\n" "$${ret}"; \
	    echo "$${ret}" >> "$${err_file}"; \
	    rm -f "$${resolvers_download}" "$${resolvers_signature}" "$${resolvers_file_tmp}"; \
	    cp rom/etc/dnscrypt-resolvers.csv "$${resolvers_file}"; \
	    if [ $${?} -ne 0 ]; then \
		ret="$$(date): No bundled dnscrypt-resolvers.csv file has been found, aborting!"; \
		printf "\n\n%s\n\n" "$${ret}"; \
		echo "$${ret}" >> "$${err_file}"; \
		exit 1; \
	    fi; \
	}; \
	if ! command -v minisign >/dev/null 2>&1; then \
	    use_local_copy "Minisign is not installed; downloaded resolver list cannot be authenticated."; \
	else \
	    count=0; \
	    while [ $${count} -lt 6 ]; do \
		printf "\nRetrieving the list of public DNSCrypt resolvers and its signature...\n"; \
		if /usr/bin/wget --secure-protocol=TLSv1_2 -T 10 -t 1 "$${resolvers_url}" -O "$${resolvers_download}" && \
		   /usr/bin/wget --secure-protocol=TLSv1_2 -T 10 -t 1 "$${resolvers_signature_url}" -O "$${resolvers_signature}"; then \
		    printf "\nVerifying the DNSCrypt resolvers signature...\n"; \
		    if minisign -Vm "$${resolvers_download}" -x "$${resolvers_signature}" -P "$${minisign_key}"; then \
			printf "\nNormalizing the DNSCrypt resolvers CSV...\n"; \
			if LC_ALL=C awk ' \
			    { \
				line = $$0; \
				sub(/\r$$/, "", line); \
				out = ""; \
				quoted = 0; \
				for (i = 1; i <= length(line); i++) { \
				    c = substr(line, i, 1); \
				    if (c == "\"") { \
					if (quoted && substr(line, i + 1, 1) == "\"") { \
					    out = out "\""; \
					    i++; \
					} \
					else { \
					    quoted = !quoted; \
					} \
				    } \
				    else if (c == "," && quoted) { \
					out = out ";"; \
				    } \
				    else { \
					out = out c; \
				    } \
				} \
				if (quoted) exit 2; \
				print out; \
			    }' "$${resolvers_download}" > "$${resolvers_file_tmp}" && \
			   LC_ALL=C awk -F, ' \
			    NR == 1 { \
				if ($$1 != "Name" || NF != 14) exit 1; \
			    } \
			    NF != 14 { exit 1; } \
			    END { if (NR < 2) exit 1; }' "$${resolvers_file_tmp}"; then \
			    sed -i "s/'/\\\'/g" "$${resolvers_file_tmp}"; \
			    mv -f "$${resolvers_file_tmp}" "$${resolvers_file}"; \
			    rm -f "$${resolvers_download}" "$${resolvers_signature}"; \
			    printf "\nDone\n\n"; \
			    count=6; \
			else \
			    use_local_copy "Authenticated DNSCrypt resolvers list is not a valid 14-column CSV."; \
			    count=6; \
			fi; \
		    else \
			use_local_copy "DNSCrypt resolvers signature verification failed."; \
			count=6; \
		    fi; \
		else \
		    rm -f "$${resolvers_download}" "$${resolvers_signature}"; \
		    count=$$((count + 1)); \
		    if [ $${count} -gt 5 ]; then \
			use_local_copy "DNSCrypt resolvers list or signature download error, check your internet connection!"; \
		    else \
			sleep 2; \
		    fi; \
		fi; \
	    done; \
	fi
endif

ifeq ($(PLATFORM_BPIR3),y)
	mv $(INSTALLDIR)/rom/etc/fw_env.config-bpir3 $(INSTALLDIR)/rom/etc/fw_env.config
else
	rm -f $(INSTALLDIR)/rom/etc/fw_env.config-bpir3
endif

ifeq ($(PLATFORM_BPIR3MINI),y)
	mv $(INSTALLDIR)/rom/etc/fw_env.config-bpir3mini $(INSTALLDIR)/rom/etc/fw_env.config
else
	rm -f $(INSTALLDIR)/rom/etc/fw_env.config-bpir3mini
endif

.PHONY: dummy
