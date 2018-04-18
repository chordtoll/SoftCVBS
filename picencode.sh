make ntsc-decode
make interleave
convert input/$1 -resize 640x480\! tempencode/pic.png
python ntsc-encode.py tempencode/pic.png tempencode/pic.cvbs
cat tempencode/pic.cvbs | ./ntsc-decode -a0 -d0 > tempencode/pic.rgb
convert -size "800x800" -depth 8 tempencode/pic.rgb tempencode/picint.png
convert -crop 800x400 tempencode/picint.png tempencode/picint%d.png
./interleave tempencode/picint{1,0}.png tempencode/picdeint.png
convert tempencode/picdeint.png -crop 647x480+19+26 output/$1