==============================================================================

 About makeheader.py

==============================================================================

This helper script helps handle created .c source files.
This script collects existing .c files exported from SpriteStudio in a folder and performs include definitions.
You must have Python installed to use it.


- Usage
makeheader.py [name of folder containing .c files]


When run, two files named ssdatas.cpp/.h are generated.

Add the .cpp file to the project and include the .h file in the locations where animation is used for simple animation playback.

