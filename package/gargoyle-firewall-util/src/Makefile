all: make_iptables_rules dump_quotas delete_chain_from_table

delete_chain_from_table: delete_chain_from_table.c
	$(CC) $(CFLAGS) $(LDFLAGS)  $^ -o  $@ -lericstools

make_iptables_rules: make_iptables_rules.c
	$(CC) $(CFLAGS) $(LDFLAGS) make_iptables_rules.c -o make_iptables_rules -lericstools -luci -lm

dump_quotas: dump_quotas.c
	$(CC) $(CFLAGS) $(LDFLAGS) dump_quotas.c -o dump_quotas -lericstools -luci -lm
clean:
	rm -rf make_iptables_rules dump_quotas *.o *~ .*sw*
