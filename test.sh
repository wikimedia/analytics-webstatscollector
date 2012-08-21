./collector &
#cat example2.log | ./filter -t 
socat -d -d -d -u "EXEC:./filter.sh" UDP-SENDTO:127.0.0.1:3815 
#cat | example2.log

#socat -d -d -d -u -x STDOUT,fork UDP-SENDTO:127.0.0.1:3815
#./filter -t <  ~/Development/analytics/udp-filters/example2.log
