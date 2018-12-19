%% COMPILE_MATCONVNET
%
% Compile MatConvNet
%
% Hyeonseob Nam, 2015 
%

run matconvnet/matlab/vl_setupnn ;
cd matconvnet;
vl_compilenn('enableGpu', true, ...
               'cudaRoot', '/usr/local/cuda-7.5', ...
               'cudaMethod', 'nvcc');
cd ..;
