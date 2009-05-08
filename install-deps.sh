#!/bin/bash

opt_update="false"
opt_components_all="true"
opt_components_raknet="false"
opt_components_sst="false"
opt_components_sirikata="false"

# parameters
until [ -z "$1" ]
do
  if [ "$1" == "update" ]; then
    opt_update="true"
    echo "Performing update"
  elif [ "$1" == "raknet" ]; then
    opt_components_all="false"
    opt_components_raknet="true"
  elif [ "$1" == "sst" ]; then
    opt_components_all="false"
    opt_components_sst="true"
  elif [ "$1" == "sirikata" ]; then
    opt_components_all="false"
    opt_components_sirikata="true"
  else
    echo "Unknown option: $1"
  fi
  shift
done

# if opt_components_all is still marked, mark all for processing
if [ ${opt_components_all} == "true" ]; then
  opt_components_raknet="true"
  opt_components_sst="true"
  opt_components_sirikata="true"
fi


# make sure we have the basic directory layout
if [ ! -e dependencies ]; then
    mkdir dependencies
fi
cd dependencies

deps_dir=`pwd`


# raknet
if [ ${opt_components_raknet} == "true" ]; then

  if [ ${opt_update} != "true" ]; then
    if [ -e raknet ]; then
      rm -rf raknet
    fi
    if [ -e installed-raknet ]; then
      rm -rf installed-raknet
    fi
    mkdir raknet
  fi

  cd raknet

  if [ ${opt_update} != "true" ]; then
    wget http://www.jenkinssoftware.com/raknet/downloads/RakNet-3.51.zip
    unzip RakNet-3.51.zip
    patch -p1 < ../raknet_gcc_4_3.patch
  fi

  sh bootstrap
  ./configure --prefix=${deps_dir}/installed-raknet
  make
  make install
  cd ..

fi # opt_components_raknet


# sst
if [ ${opt_components_sst} == "true" ]; then

  if [ ${opt_update} != "true" ]; then
    if [ -e sst ]; then
      rm -rf sst
    fi
    if [ -e installed-sst ]; then
      rm -rf installed-sst
    fi
    #svn co svn://svn.pdos.csail.mit.edu/uia/trunk/uia/sst
    git clone git@ahoy:sst.git
    cd sst
    git branch stanford origin/stanford
    git checkout stanford
  else
    cd sst
    git pull origin stanford:stanford
  fi

  misc/setup
  ./configure --prefix=${deps_dir}/installed-sst
  make
  make install
  cd ..

fi # opt_components_sst



# sirikata
if [ ${opt_components_sirikata} == "true" ]; then

  if [ ${opt_update} != "true" ]; then
    if [ -e sirikata ]; then
      rm -rf sirikata
    fi
    if [ -e installed-sirikata ]; then
      rm -rf installed-sirikata
    fi
    git clone git://github.com/sirikata/sirikata.git sirikata
    cd sirikata
    make depends
  else
    cd sirikata
    git pull origin
  fi
  git submodule init
  git submodule update
  cd build/cmake
  # debug
  cmake -DCMAKE_INSTALL_PREFIX=${deps_dir}/installed-sirikata -DCMAKE_BUILD_TYPE=Debug .
  make clean
  make -j2
  make install
  # release
  cmake -DCMAKE_INSTALL_PREFIX=${deps_dir}/installed-sirikata -DCMAKE_BUILD_TYPE=Release .
  make clean
  make -j2
  make install
  # we need to manually copy over the protobuf libs because sirikata doesn't handle this properly yet
  cp ${deps_dir}/sirikata/dependencies/lib/libproto* ${deps_dir}/installed-sirikata/lib/
  cd ../../..

fi # opt_components_sirikata
