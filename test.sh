./collector &
#cat example2.log | ./filter -t 


#Test: adding this comment to be merged to gerrit, for learning purposes
#Test: Now it is fixed because of this new comment(also for learning purposes)

socat -d -d -d -u "EXEC:./filter.sh" UDP-SENDTO:127.0.0.1:3815 
#cat | example2.log

#socat -d -d -d -u -x STDOUT,fork UDP-SENDTO:127.0.0.1:3815
#./filter -t <  ~/Development/analytics/udp-filters/example2.log
