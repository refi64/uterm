#!/bin/bash
set -ex -o pipefail

curl -Lo upspin.tar.gz https://upspin.io/dl/upspin.linux_amd64.tar.gz
sudo tar -C /usr/bin -xvf upspin.tar.gz
rm upspin.tar.gz

git clone https://github.com/glfw/glfw.git
cd glfw

mkdir build
cd build
cmake .. \
  -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=YES \
  -DGLFW_BUILD_EXAMPLES=NO -DGLFW_BUILD_TESTS=NO -DGLFW_BUILD_DOCS=NO
sudo make -j4 install
cd ../..

git clone https://github.com/felix-lang/fbuild.git
cd fbuild
sudo python3 setup.py install
cd ..

upspin keygen -curve p256 -secretseed $UPSPIN_KEY ~/.ssh/nightly@refi64.com
upspin signup -server=upspin.refi64.com nightly@refi64.com ||:
upspin user | upspin user -put

export PKG_CONFIG_PATH=/usr/lib/pkgconfig
# XXX
sed -i '/#include <GLES2\/gl2.h>/d' deps/skia/src/gpu/gl/egl/*.cpp
if ! fbuild --release --cc=gcc --cxx=g++ --ld=gold --no-force-color -j4; then
  set +x
  echo '*******************************'
  echo '********** BUILD LOG **********'
  echo '*******************************'
  echo
  tail -200 build/fbuild.log
  false
fi

curl -Lo functions.sh \
  https://raw.githubusercontent.com/probonopd/AppImages/master/functions.sh

set +e
. ./functions.sh

mkdir uterm.AppDir
cd uterm.AppDir

mkdir -p usr/bin
cp ../build/uterm usr/bin
cp ../uterm.desktop .
curl -Lo uterm.svg 'https://openclipart.org/download/212873/1421942630.svg'

copy_deps
move_lib
delete_blacklisted
get_apprun

cd ..

DATE=`date -u +'%Y-%m-%d-%H:%M'`
COMMIT=`git rev-parse --short HEAD`
APPIMAGE=uterm-$DATE-$COMMIT-x86_64.AppImage
curl -LO https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage
./appimagetool-x86_64.AppImage uterm.AppDir $APPIMAGE

upspin cp $PWD/$APPIMAGE nightly@refi64.com/uterm
