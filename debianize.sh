rm ../webstatscollector_0*
rm -rf ../webstatscollector-0.2
mkdir ../webstatscollector-0.2
cd ../webstatscollector-0.2
dh_make -c gpl2 -e dvanliere@wikimedia.org -s --createorig
cp ../webstatscollector/debian/control debian/.
cp ../webstatscollector/debian/rules debian/.
cp ../webstatscollector/debian/copyright debian/.
cp ../webstatscollector/debian/changelog debian/.
cp ../webstatscollector/Makefile debian/.
dpkg-buildpackage
