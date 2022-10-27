#!/bin/bash
killall hostapd dhcpd
bash -c "echo 0 > /proc/sys/net/ipv4/ip_forward"
#nmcli d connect wlan0
#nmcli d disconnect wlan0
#nmcli d connect wlan0
