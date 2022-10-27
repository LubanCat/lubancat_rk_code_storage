#!/bin/bash
bash -c "echo 1 > /proc/sys/net/ipv4/ip_forward"
iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
nmcli d disconnect wlan0
ifconfig wlan0 192.168.0.1 netmask 255.255.255.0
rfkill unblock wlan
sleep 6s
nohup hostapd /etc/hostapd/hostapd.conf >/home/cat/hostapd.log 2>&1 &
dhcpd wlan0 -pf /var/run/dhcpd.pid
ps -ef|head -n1 && ps -ef|egrep "dhcpd|hostapd"
