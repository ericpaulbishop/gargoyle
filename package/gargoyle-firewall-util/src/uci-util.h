
#include <erics_tools.h>
#include <uci.h>


list* get_all_sections_of_type(struct uci_context *ctx, char* package, char* section_type);
char* get_uci_option(struct uci_context* ctx,char* package_name, char* section_name, char* option_name);
char* get_option_value_string(struct uci_option* uopt);
char* get_groups(struct uci_context* ctx);
