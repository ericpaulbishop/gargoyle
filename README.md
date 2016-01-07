# gargoyle
Known Devices

The idea is to be able to set policy (Quota, QoS, Restrictions etc) over a
Group of Known Devices (owned by an individual for example). To achieve this,
the Known Devices need to be identified by their MAC address' and then arranged
into Groups. Both Devices and Groups can be named by the Gargoyle Administrator.

Up until now, the best way for a Gargoyle admin to apply policy to users with
a number of devices has been to assign a static IP address to each device such
that each users' devices are in an IP range. Then, the policy is applied to the
IP range, and the IP range is displayed throughout the Gargoyle GUI. Known Devices
and Groups enable the Gargoyle admin to work with meaningful names rather than IP
ranges, and relieves the need to assign static IP addresses.

Status
- Gargoyle-Connection-Devices has a Section for Known Devices and another for Device Groups.
- Device Groups may be used:
 - Gargoyle-Firewall-Quotas-BandwidthQuotas-AppliesTo
 - Gargoyle-Firewall-Restrictions-AccessRestrictions-RuleAppliesTo
 - Gargoyle-Firewall-Restrictions-Exceptions(Whitelist)-RuleAppliesTo
 - Gargoyle-Forewall-QoS(Upload)-ClassificationRules
 - Gargoyle-Forewall-QoS(Download)-ClassificationRules

Technical
- Approx 17k in size
- Known Devices are stored as a host in uci /etc/config/dhcp
- Each host has a supplementary Group section
- Each Device Group is represented by an ipset (each set = 550 bytes min)
- IP addresses in each ipset are dynamically adjusted when dnsmasq issues a dhcp lease
- Existing iptables quota rules can check against the membership of an ipset
