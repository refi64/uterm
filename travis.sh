#!/bin/bash
set -ex -o pipefail

# yum -y install epel-release centos-release-scl
# yum -y install https://centos7.iuscommunity.org/ius-release.rpm
# yum -y install \
#   file fuse-libs wget \
#   cmake gcc gcc-c++ git2u make \
#   fontconfig-devel freetype-devel libconfuse-devel \
#   libX11-devel libXrandr-devel libXi-devel libXinerama-devel \
#   libXcursor-devel \
#   mesa-libGL-devel mesa-libEGL-devel \
#   python36u python36u-setuptools python-argparse

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
if ! fbuild --release -j4; then
  set +x
  echo '*******************************'
  echo '********** BUILD LOG **********'
  echo '*******************************'
  echo
  tail -200 build/fbuild.log
  false
fi

mkdir -p usr/bin
cp build/uterm usr/bin
curl -Lo functions.sh \
  https://raw.githubusercontent.com/probonopd/AppImages/master/functions.sh

set +e
. ./functions.sh

patch_usr
copy_deps
delete_blacklisted

DATE=`date -u +'%D-%H:%M'`
COMMIT=`git rev-parse --short HEAD`
export APP=uterm
export VERSION=$COMMIT-$DATE
mkdir uterm.AppDir
mv usr uterm.AppDir
cp uterm.desktop uterm.AppDir

cd uterm.AppDir
get_apprun
cd ..

generate_appimage
ls
upspin cp $PWD/out/*.AppImage nightly@refi64.com/uterm
