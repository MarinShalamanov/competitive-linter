#!/bin/bash

clang-llvm/bin/clang++ \
  -fsyntax-only \
  -Xclang -load -Xclang build/plugin_floating_point_comp.so \
  -Xclang -load -Xclang build/plugin_goto.so \
  -Xclang -load -Xclang build/plugin_consecutive_newlines.so \
  -Xclang -load -Xclang build/plugin_no_float.so \
  -Xclang -load -Xclang build/plugin_debug_macro.so \
  -Xclang -load -Xclang build/plugin_fast_cin.so \
  -Xclang -add-plugin -Xclang debug_macro \
  -Xclang -add-plugin -Xclang floating_point_comp \
  -Xclang -add-plugin -Xclang consecutive_newlines \
  -Xclang -add-plugin -Xclang no_float \
  -Xclang -add-plugin -Xclang goto \
  -Xclang -add-plugin -Xclang fast_cin \
  "$1"