# gargoyle
Known Devices

The idea is to be able to set policy (Quota, QoS, Restrictions etc) over a 
Group of Known Devices (owned by an individual for example). To achieve this 
the Known Devices need to be identified by their MAC address' and then arranged
into Groups. Both Devices and Groups can be named by the Gargoyle Administrator.

Currently, the user has to work with an IP range for the devices of each user throughout 
the Gargoyle GUI. MAC Groups will releive the Gargoyle administrator of needing to asign 
a static IP address to each device requiring management.

Status
- Gargoyle-Connection-Devices has a Section for Known Devices and another for Device Groups.
- Known Device and Group data is stored in uci /etc/config/known
