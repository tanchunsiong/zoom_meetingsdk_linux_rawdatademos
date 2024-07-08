
You can use the FFmpeg command-line tool to convert an MP4 file to YUV 420 format with different resolutions. Here's how you can do it for 480p, 720p, and 1080p resolutions:

# Convert to YUV 420 480p
ffmpeg -i Big_Buck_Bunny_720_10s_1MB.mp4 -vf "scale=640:480" -c:v rawvideo -pix_fmt yuv420p videofile_640_480.yuv

# Convert to YUV 420 720p
ffmpeg -i Big_Buck_Bunny_720_10s_1MB.mp4 -vf "scale=1280:720" -c:v rawvideo -pix_fmt yuv420p videofile_1280_720.yuv

# Convert to YUV 420 1080p
ffmpeg -i Big_Buck_Bunny_720_10s_1MB.mp4 -vf "scale=1920:1080" -c:v rawvideo -pix_fmt yuv420p videofile_1920_1080.yuv


# Play YUV 420 480p
ffplay -f rawvideo -pixel_format yuv420p -video_size 640x480 videofile_640_480.yuv

# Play YUV 420 720p
ffplay -f rawvideo -pixel_format yuv420p -video_size 1280x720 videofile_1280_720.yuv

# Play YUV 420 1080p
ffplay -f rawvideo -pixel_format yuv420p -video_size 1920x1080  videofile_1920_1080.yuv