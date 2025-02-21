override define MARIADB_INSTALL_TARGET_CMDS
	$(INSTALL) -d $(TARGET_DIR)/usr/bin
	$(INSTALL) -d $(TARGET_DIR)/usr/share/mysql

	$(INSTALL) -D $(@D)/extra/my_print_defaults		$(TARGET_DIR)/usr/bin
	$(INSTALL) -D $(@D)/extra/resolveip			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D $(@D)/storage/myisam/myisamchk		$(TARGET_DIR)/usr/bin

	$(INSTALL) -D $(@D)/client/mariadb			$(TARGET_DIR)/usr/bin
	ln -sf mariadb						$(TARGET_DIR)/usr/bin/mysql

	$(INSTALL) -D $(@D)/scripts/mariadb-install-db		$(TARGET_DIR)/usr/bin
	ln -sf mariadb-install-db				$(TARGET_DIR)/usr/bin/mysql_install_db

	$(INSTALL) -D $(@D)/client/mariadb-admin		$(TARGET_DIR)/usr/bin
	ln -sf mariadb-admin					$(TARGET_DIR)/usr/bin/mysqladmin

	$(INSTALL) -D $(@D)/client/mariadb-dump			$(TARGET_DIR)/usr/bin
	ln -sf mariadb-dump					$(TARGET_DIR)/usr/bin/mysldump

	$(INSTALL) -D $(@D)/sql/mariadbd			$(TARGET_DIR)/usr/sbin
	ln -sf /usr/sbin/mariadbd				$(TARGET_DIR)/usr/sbin/mysqld
	ln -sf /usr/sbin/mariadbd				$(TARGET_DIR)/usr/bin/mysqld

	cd $(TARGET_DIR)/usr/share/mysql && \
	cp -arfpu $(@D)/sql/share/english . && \
	cp -arfpu $(@D)/scripts/fill_help_tables.sql . && \
	cp -arfpu $(@D)/scripts/maria_add_gis_sp_bootstrap.sql . && \
	cp -arfpu $(@D)/scripts/mysql_performance_tables.sql . && \
	cp -arfpu $(@D)/scripts/mysql_sys_schema.sql . && \
	cp -arfpu $(@D)/scripts/mysql_system_tables.sql . && \
	cp -arfpu $(@D)/scripts/mysql_system_tables_data.sql . && \
	cp -arfpu $(@D)/scripts/mysql_test_db.sql .
endef

override undefine MARIADB_POST_INSTALL_TARGET_HOOKS
