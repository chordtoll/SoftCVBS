make ntsc-encode
make ntsc-decode
make deinterlace
ffmpeg -i $1 -c:v rawvideo -f rawvideo -vf scale=640:480 -pix_fmt rgb24 - | ./ntsc-encode | ./ntsc-decode | ./deinterlace | ffmpeg -y -f rawvideo -pix_fmt rgb24 -s:v 800x800 -r 30 -i - -filter:v "crop=647:480:19:26" vidcvbs.mp4
ffmpeg -i $1 vid.mp3
pacpl --overwrite --to wav vid.mp3
sox vid.wav vidlowpass.wav lowpass 3000
sox vidlowpass.wav vidcompress.wav compand 0.3,0.8 6:-70,-60,-20 -10 -90 0.5
echo "Run nyquist prompt:"
echo "(setf depth 0.0008)"
echo "(setf speed 120)"
echo "(multichan-expand #'snd-tapv *track* 0 (mult depth (sum 1 (hzosc (/ speed 60.0)))) 2)"
echo
echo "Save as vidcompress.mp3"
/mnt/c/Program\ Files\ \(x86\)/Audacity/audacity.exe c:\\users\\asent\\desktop\\python\\ntsc\\sdr-examples\\ntsc\\vidcompress.wav
read -p "Press enter to continue"
ffmpeg -i vidcvbs.mp4 -i vidcompress.mp3 -c:v copy -c:a copy ${1%.*}merged.mp4