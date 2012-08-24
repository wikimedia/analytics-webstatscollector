#rm ../webstatscollector_0*
#rm -rf ../webstatscollector-0.2
#mkdir ../webstatscollector-0.2
#cd ../webstatscollector-0.2

VERSION=0.2

tar -cvf webstatscollector.tar --exclude-from=exclude .
mv webstatscollector.tar ../
cd ..
rm -rf webstatscollector-${VERSION}
#cd .. && 
mkdir webstatscollector-${VERSION}
#cd .. && 
tar -C webstatscollector-${VERSION} -xvf webstatscollector.tar


#webstatscollector_0.2.orig.tar.gz
#cd .. && 
rm webstatscollector-${VERSION}.orig.tar.gz

#cp -r webstatscollector webstatscollector_${VERSION}
cd webstatscollector-${VERSION} 
dh_make -c gpl2 -e dvanliere@wikimedia.org -s --createorig
#cd ../webstatscollector-${VERSION}/debian && 
cd debian
rm *ex *EX
#cd ../webstatscollector-${VERSION}/debian && 
rm README.Debian dirs
#cp ../webstatscollector/debian/control debian/.
#cp ../webstatscollector/debian/rules debian/.
#cp ../webstatscollector/debian/copyright debian/.
#cp ../webstatscollector/debian/changelog debian/.
#cp ../webstatscollector/Makefile debian/.
#cd ../webstatscollector_${VERSION} && 
cd ..
dpkg-buildpackage
cd ..
dpkg-deb --contents webstatscollector_${VERSION}_amd64.deb
lintian webstatscollector_0.2_amd64.deb
