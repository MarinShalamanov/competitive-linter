#!/bin/bash

if [ -d "$pwd/clang-llvm" ]; then

  wget http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-linux-x86_64-ubuntu14.04.tar.xz 
  tar xf clang+llvm-5.0.0-linux-x86_64-ubuntu14.04.tar.xz
  mv clang+llvm-5.0.0-linux-x86_64-ubuntu14.04/ clang-llvm/
  rm clang+llvm-5.0.0-linux-x86_64-ubuntu14.04.tar.xz
else
  echo "Already downloaded" 
fi