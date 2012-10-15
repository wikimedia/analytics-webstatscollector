import socket

s=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
host = 'localhost'
port = 3815

filename = '/Users/diederik/Development/analytics/udp-filters/example2.log'
fh = open(filename, 'r')

#for line in fh:
#	line = line.strip()
#	print line
#	s.sendto(line, (host, port))

for x in xrange(10000):
	line = '%s %s %s %s %s' % (x, 'wiki', 1, 1024, 'Foo Bar')
	s.sendto(line, (host, port))

fh.close()
#s.shutdown(socket.SHUT_WR)
s.close()
