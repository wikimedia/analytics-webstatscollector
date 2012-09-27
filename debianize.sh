#/bin/bash

export DEBFULLNAME="Diederik van Liere"

VERSION=`git describe | awk -F'-g[0-9a-fA-F]+' '{print $1}'`
MAIN_VERSION=`git describe --abbrev=0`

tar -cvf webstatscollector.tar --exclude-from=exclude .
mv webstatscollector.tar ../
cd ..
rm -rf webstatscollector-${MAIN_VERSION}
mkdir webstatscollector-${MAIN_VERSION}
tar -C webstatscollector-${MAIN_VERSION} -xvf webstatscollector.tar


rm webstatscollector-${MAIN_VERSION}.orig.tar.gz

cd webstatscollector-${MAIN_VERSION}
mkdir m4
dh_make -c gpl2 -e dvanliere@wikimedia.org -s --createorig -p webstatscollector_${VERSION}
cd debian
rm *ex *EX
rm README.Debian dirs
#cp ../webstatscollector/debian/control debian/.
#cp ../webstatscollector/debian/rules debian/.
#cp ../webstatscollector/debian/copyright debian/.
#cp ../webstatscollector/debian/changelog debian/.
#cp ../webstatscollector/Makefile debian/.
#cd ../webstatscollector_${VERSION} &&
cd ..
dpkg-buildpackage -v${VERSION}
cd ..
dpkg-deb --contents webstatscollector_${MAIN_VERSION}_amd64.deb
lintian webstatscollector_${MAIN_VERSION}_amd64.deb
mv webstatscollector_${MAIN_VERSION}_amd64.deb webstatscollector_${VERSION}_amd64.db

