# Real-time-convolutional-reverb-
This project aimed to implement a convolutional reverb on real time using c++ and RTaudio an audio API .

To execute duplex.cpp type :

./duplex 'your_channel' 'sampling_frequency' 'type_conv' 'impulsion_length' 'buffer_size' .

with type_conv=1 for a convolution in frequency domain and 0 for temporal domain .

I imported the impulsional response in imp.txt file , you can replace it with yours :) . and change the file path in lune 231 .


once you execute the code , the input stream buffer will be listening and you can hear your reverbed voice as the same time as you're talking
with a better performance on frequency convolution than with temporal one .

