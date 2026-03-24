# YUV conversion notes

Use FFmpeg to convert a source MP4 into raw YUV420P files for the send-video samples.

## Generate YUV files

```bash
ffmpeg -i Big_Buck_Bunny_720_10s_1MB.mp4 -vf "scale=640:480" -c:v rawvideo -pix_fmt yuv420p videofile_640_480.yuv
ffmpeg -i Big_Buck_Bunny_720_10s_1MB.mp4 -vf "scale=1280:720" -c:v rawvideo -pix_fmt yuv420p videofile_1280_720.yuv
ffmpeg -i Big_Buck_Bunny_720_10s_1MB.mp4 -vf "scale=1920:1080" -c:v rawvideo -pix_fmt yuv420p videofile_1920_1080.yuv
```

## Preview YUV files

```bash
ffplay -f rawvideo -pixel_format yuv420p -video_size 640x480 videofile_640_480.yuv
ffplay -f rawvideo -pixel_format yuv420p -video_size 1280x720 videofile_1280_720.yuv
ffplay -f rawvideo -pixel_format yuv420p -video_size 1920x1080 videofile_1920_1080.yuv
```
