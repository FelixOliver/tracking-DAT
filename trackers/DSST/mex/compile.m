%mex -lopencv_core -lopencv_imgproc -L/usr/local/lib -I/usr/local/include/ mexResize.cpp MxArray.cpp
%mex gradientMex.cpp
% modificacion para compilar en windows 10
addpath('E:\SEMESTRE 2018 A\TESIS\drone-tracking-master\mexopencv-2.4');

mex trackers\DSST\mex\mexResize.cpp trackers\DSST\mex\MxArray.cpp 
%mex Resize.cpp
%mex MxArray.cpp
%mex gradientMex.cpp