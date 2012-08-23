#rm ../webstatscollector_0*
#rm -rf ../webstatscollector-0.2
#mkdir ../webstatscollector-0.2
#cd ../webstatscollector-0.2

VERSION=0.2
cd ..
rm webstatscollector-${VERSION}.orig.tar.gz
tar -cvf webstatscollector.tar --exclude-from=webstatscollector/exclude webstatscollector
tar -C webstatscollector-${VERSION} -xvf webstatscollector.tar

#cp -r webstatscollector webstatscollector_${VERSION}
cd webstatscollector-${VERSION}
dh_make -c gpl2 -e dvanliere@wikimedia.org -s --createorig
#cp ../webstatscollector/debian/control debian/.
#cp ../webstatscollector/debian/rules debian/.
#cp ../webstatscollector/debian/copyright debian/.
#cp ../webstatscollector/debian/changelog debian/.
#cp ../webstatscollector/Makefile debian/.
dpkg-buildpackage
cd ..
dpkg-deb --contents webstatscollector_${VERSION}_amd64.deb
