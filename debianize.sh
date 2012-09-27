#/bin/bash

export DEBFULLNAME="Diederik van Liere"

VERSION=`git describe | awk -F'-g[0-9a-fA-F]+' '{print $1}' | sed -e 's/\-/./g' `
MAIN_VERSION=`git describe --abbrev=0`

tar -cvf webstatscollector.tar --exclude-from=exclude .
mv webstatscollector.tar ../
cd ..
rm -rf webstatscollector-${MAIN_VERSION}
mkdir webstatscollector-${MAIN_VERSION}
tar -C webstatscollector-${MAIN_VERSION} -xvf webstatscollector.tar


rm webstatscollector-${MAIN_VERSION}.orig.tar.gz

cd webstatscollector-${MAIN_VERSION}

VERSION=$VERSION perl -pi -e 's/VERSION=".*";/VERSION="$ENV{VERSION}";/' src/filter.c
VERSION=$VERSION perl -pi -e 's/VERSION=".*";/VERSION="$ENV{VERSION}";/' src/collector.c

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


ARCHITECTURE=`uname -m`
ARCH_i686="i386"
ARCH_x86_64="amd64"
ARCH_SYS=""

if [ $ARCHITECTURE == "i686" ]; then
  ARCH_SYS=$ARCH_i686
elif [ $ARCHITECTURE == "x86_64" ]; then
  ARCH_SYS="amd64"
else
  echo -e  "Sorry, only i686 and x86_64 architectures are supported.\n"
  exit 1
fi


PACKAGE_NAME_VERSION=webstatscollector_${VERSION}_$ARCH_SYS.deb
PACKAGE_NAME_MAIN_VERSION=webstatscollector_${MAIN_VERSION}_${ARCH_SYS}.deb


dpkg-deb --contents ${PACKAGE_NAME_MAIN_VERSION}

echo -e "Linting package ${PACKAGE_NAME_MAIN_VERSION} ...\n"
lintian ${PACKAGE_NAME_MAIN_VERSION}
mv ${PACKAGE_NAME_MAIN_VERSION} ${PACKAGE_NAME_VERSION}




