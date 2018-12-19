startup;
% add all tracker path
cur_dir = fileparts(mfilename('fullpath'));
addpath(fullfile(cur_dir, 'experiments', 'trackers'));

% config the dataset path
data_path = './data/DTB70';
%data_path = './data/VATOLIVER';
seq_name = 'Car8'
%seq_name = '1car2';
data_path = fullfile(data_path, seq_name);
img_path = fullfile(data_path, 'img');
img_list = dir(fullfile(img_path, '*.jpg'));
img_list

rects = importdata(fullfile(data_path, 'groundtruth_rect.txt'));
rects
seq.init_rect = rects(1,:);
seq.path = img_path;
seq.ext = 'jpg';
seq.startFrame = 1;
seq.endFrame = size(img_list, 1);
seq.s_frames = cell(size(img_list, 1), 1);
for i = 1:size(img_list, 1)
	seq.s_frames{i} = fullfile(img_path, img_list(i).name);
end

% config the tracker name
results = run_DAT(seq, '', false);
%results = run_DSST(seq, '', false);
results_ransac = run_DAT_RANSAC(seq, '', false);

fin = "FIN"
