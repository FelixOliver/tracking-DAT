function startup()
% startup function

cur_dir = fileparts(mfilename('fullpath'));
addpath(fullfile(cur_dir, 'experiments'));
addpath(fullfile(cur_dir, 'experiments', 'rstEval'));
addpath(fullfile(cur_dir, 'experiments', 'util'));
addpath(fullfile(cur_dir, 'util'));

% add mexopencv path
addpath('/home/oliver/TRACKING/mexopencv');

%addpath('E:\SEMESTRE 2018 A\TESIS\drone-tracking-master\mexopencv-2.4');
fprintf('startup config finished.\n');