all: make_nftables_rules delete_chain_from_table backup_quotas restore_quotas print_quotas

print_quotas: print_quotas.c
	        $(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lericstools -luci -lnftbwctl

restore_quotas: restore_quotas.c
	        $(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lericstools -luci -lnftbwctl

backup_quotas: backup_quotas.c
	        $(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lericstools -luci -lnftbwctl

delete_chain_from_table: delete_chain_from_table.c
	$(CC) $(CFLAGS) $(LDFLAGS)  $^ -o  $@ -lericstools

make_nftables_rules: make_nftables_rules.c
	$(CC) $(CFLAGS) $(LDFLAGS) make_nftables_rules.c -o make_nftables_rules -lericstools -luci -lm

clean:
	rm -rf make_nftables_rules delete_chain_from_table print_quotas backup_quotas restore_quotas *.o *~ .*sw*
