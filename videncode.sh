make ntsc-encode
make ntsc-decode
make deinterlace
rate=$(ffprobe -v error -select_streams v:0 -show_entries stream=avg_frame_rate -of default=noprint_wrappers=1:nokey=1 input/$1)
echo $rate
ffmpeg -v quiet -stats -i input/$1 -c:v rawvideo -f rawvideo -vf scale=640:480 -pix_fmt rgb24 - | ./ntsc-encode | ./ntsc-decode -b0 | ./deinterlace | ffmpeg -y -f rawvideo -pix_fmt rgb24 -s:v 800x800 -r $rate -i - -filter:v "crop=647:480:19:26" tempencode/vidcvbs.mp4
ffmpeg -v quiet -stats -y -i input/$1 tempencode/vid.mp3
pacpl --overwrite --to wav tempencode/vid.mp3
sox tempencode/vid.wav tempencode/vidlowpass.wav lowpass 3000
sox tempencode/vidlowpass.wav tempencode/vidcompress.wav compand 0.3,0.8 6:-70,-60,-20 -10 -90 0.5
echo "Run nyquist prompt:"
echo "(setf depth 0.0008)"
echo "(setf speed 120)"
echo "(multichan-expand #'snd-tapv *track* 0 (mult depth (sum 1 (hzosc (/ speed 60.0)))) 2)"
echo
echo "Save as vidcompress.mp3"
/mnt/c/Program\ Files\ \(x86\)/Audacity/audacity.exe c:\\users\\asent\\desktop\\python\\softcvbs\\tempencode\\vidcompress.wav
read -p "Press enter to continue"
ffmpeg -v quiet -stats -y -i tempencode/vidcvbs.mp4 -i tempencode/vidcompress.mp3 -c:v copy -c:a copy output/$1
