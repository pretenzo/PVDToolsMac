# Variables
FRAME_RATE=18
INPUT_FRAMES="%d.ppm"
CONVERTED_FRAMES="%d.bmp"
AUDIO_FILE="input-right2.wav"
VIDEO_OUTPUT="video_output.mp4"
FINAL_OUTPUT="final_output.mp4"

# Step 1: Convert PPM frames to BMP (non-padded filenames)
ffmpeg -i $INPUT_FRAMES $CONVERTED_FRAMES

# Step 2: Create video from BMP frames with specified frame rate
ffmpeg -framerate $FRAME_RATE -i $CONVERTED_FRAMES -pix_fmt yuv420p -c:v libx264 $VIDEO_OUTPUT

# Step 3: Merge video with audio and adjust durations to match
ffmpeg -i $VIDEO_OUTPUT -i $AUDIO_FILE -filter_complex "[0:v:0]setpts=PTS-STARTPTS[v];[1:a:0]aresample=async=1[a]" -map "[v]" -map "[a]" -r $FRAME_RATE -c:v libx264 -c:a aac -shortest $FINAL_OUTPUT

#Step 4: Remove temporary files
rm -rf *.ppm *.bmp

echo "Video processing completed! The final video is saved as $FINAL_OUTPUT."
