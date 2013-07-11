# This pattern matches DNS requests for .onion TLD domains
#
# Based on second version of patterns specified dns.pat
#

oniondns

# This way assumes that TLDs are any alpha string 2-6 characters long.
# If TLDs are added, this is a good fallback.

# this is original pattern, from dns.pat:
#^.?.?.?.?[\x01\x02].?.?.?.?.?.?[\x01-?][a-z0-9][\x01-?a-z]*[\x02-\x06][a-z][a-z][a-z]?[a-z]?[a-z]?[a-z]?[\x01-\x10][\x01\x03\x04\xFF]

# matches .onion domains only:
^.?.?.?.?[\x01\x02].?.?.?.?.?.?[\x01-?][a-z0-9][\x01-?a-z]*[\x02-\x06]onion[\x01-\x10][\x01\x03\x04\xFF]

