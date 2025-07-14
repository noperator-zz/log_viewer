#!/bin/bash

set -euo pipefail

this_dir=$(realpath $(dirname "$0"))
svg_dir=$(realpath "$this_dir/../svg")
res_dir=$(realpath "$this_dir/../res")

tex_h="$res_dir/tex.h"
tex_data_h="$res_dir/tex_data.h"
tex_raw="$res_dir/tex.raw"

rm -rf "$res_dir"
mkdir -p "$res_dir"
cd "$svg_dir"

echo "// This file is auto-generated from SVG files in the svg directory." > "$tex_h"
echo "#pragma once" >> "$tex_h"
echo "enum class TexID : int{" >> "$tex_h"
echo "    None=0," >> "$tex_h"
num_tex=1
for f in *.svg; do
  bash "$this_dir/svg-to-bitmap.sh" "$f" >> "$tex_raw"
  echo "    ${f%.svg}," >> "$tex_h"
  num_tex=$((num_tex + 1))
done
echo "};" >> "$tex_h"

echo "#pragma once" >> "$tex_data_h"
echo "static constexpr unsigned char tex_raw[] = {" >> "$tex_data_h"
# zeroes for 'None' texture
for i in $(seq 1 64); do
  echo "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0," >> "$tex_data_h"
done
cat "$tex_raw" | xxd -i - >> "$tex_data_h"
echo "};" >> "$tex_data_h"
#echo "static constexpr size_t tex_raw_size = sizeof(tex_raw);" >> "$tex_data_h"
echo "static constexpr size_t num_tex = $num_tex;" >> "$tex_data_h"
echo "static constexpr float tex_step = 1.0f / static_cast<float>(num_tex);" >> "$tex_data_h"
