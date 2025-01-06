# VideoNow Video Decoder for Mac

This is a collection of tools to decode VideoNow PVD discs. This version has been ported to decode VideoNow PVD discs on Macs using the command line. The program expects a VideoNow-formatted disc image in (.bin/.cue) format. Although it theoretically works on B&W/VideoNowXP discs as well, they haven't been tested on the Mac version. The tool currently decodes the Video files to a collection of .ppm video frame files and the corresponding WAV file. A future implementation will pair this tool with ffmpeg to properly encode the outputted files to one final video that's easily watchable.

Original binary and source code is [here](https://sourceforge.net/projects/pvdtools/)  
Technical documentation about black & white disc is [here](https://web.archive.org/web/20161026023116/http://pvdtools.sourceforge.net:80/format.txt)  
Technical documentation about color disc is [here](https://forum.videohelp.com/threads/123262-converting-video-formats-%28For-Hasbro-s-VideoNow%29-I-know-the/page17#post1149694)

## Usage
1. Execute command-line and type below
   ./PVDTools bw [cue file]
   => You can get 1 [wav file](https://en.wikipedia.org/wiki/WAV) and multiple [pgm files](https://en.wikipedia.org/wiki/Netpbm_format) in the current directory.
   ./PVDTools color [cue file]
   => You can get 1 wav file and multiple ppm files in the current directory.
   ./PVDTools xp [cue file]  
   => You can get 1 wav file and multiple ppm files in the current directory.
 
2. Ensure ffmpeg is installed (brew install ffmpeg)
3. Use the convert.sh script to combine ppm and audio files, and export them to .mp4 (script can be changed to any ffmpeg supported format)
4. Profit

