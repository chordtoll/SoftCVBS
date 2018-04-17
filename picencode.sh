make ntsc-decode
echo convert $1 -resize 640x480\! pic.png
convert $1 -resize 640x480\! pic.png
python ntsc-encode.py pic.png pic.cvbs
cat pic.cvbs | ./ntsc-decode > pic.rgb
convert -size "800x800" -depth 8 pic.rgb picint.png
convert -crop 800x400 picint.png picint%d.png
./interleave picint{1,0}.png picdeint.png
convert picdeint.png -crop 647x480+19+26 piccrop.png