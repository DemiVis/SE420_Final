%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% To create the "truth" images for the output from each of the C transforms
% specified below:
%   - Sobel Edge Detection
%   - Hough Lines Detection
%   - Pyramidal Up Conversion
%   - Pyramidal Down Conversion
%
% Author: Matthew Demi Vis (Vis,iting Venusian)
%         vism@my.erau.edu
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% Input Parameters
%Input Image in "..\Test_images\" folder
inputImgFilename = 'mount_xl.pgm';

%Image showing magnification level
imshowMag = 10;     % percent

%Output File type
outFileType = 'pgm';

% Sobel
useAutoThresholdSobel = true;
sobelThreshold = 0.001; 

%Hough
useAutoThresholdHough = true;
houghThreshold = 50;
houghNumLines = 5;


%% Setup for Image transformations
close all

cd ..\Test_images\;
colorImg = imread(inputImgFilename);
cd ..\CreateTruth\;

[height, width, depth] = size(colorImg);
if depth == 3
    grayImg = rgb2gray(colorImg);
elseif depth == 1
    grayImg = colorImg;
else
    error('Input Image has unknown number of color channels. Please use an image with 1 or 3 (RGB) color channels.')
end

imshow(grayImg,'InitialMagnification',imshowMag);
titleStr = sprintf('Input Image. %dx%d', width, height);
title(titleStr);

%% Sobel
if useAutoThresholdSobel
    [sobelImg, thresh] = edge(grayImg, 'sobel');
else
    [sobelImg, thresh] = edge(grayImg, 'sobel', sobelThreshold);
end

figure
imshow(sobelImg,'InitialMagnification',imshowMag);
titleStr = sprintf('Sobel Transformed Image, Thresh:%1.3f', thresh);
title(titleStr);
imwrite(sobelImg, strcat('SobelOut','.',outFileType), outFileType);

%% Hough
[rhoThetaImg,theta,rho] = hough(sobelImg);
if useAutoThresholdHough
    peaks = houghpeaks(rhoThetaImg, houghNumLines);
else
    peaks = houghpeaks(rhoThetaImg, houghNumLines, 'threshold',houghThreshold);
end
lines = houghlines(sobelImg, theta, rho, peaks, 'FillGap',5,'MinLength',7);

figure
imshow(imadjust(mat2gray(rhoThetaImg)),[],'XData',theta,'YData',rho,'InitialMagnification',imshowMag);
titleStr = sprintf('Hough Lines Rho Theta');
title(titleStr);
imwrite(imadjust(mat2gray(rhoThetaImg)), strcat('RhoThetaOut','.',outFileType), outFileType);

figure
imshow(colorImg,'InitialMagnification',imshowMag)
title('Hough Lines on Image')
hold on
for k = 1:length(lines)
   xy = [lines(k).point1; lines(k).point2];
   plot(xy(:,1),xy(:,2),'LineWidth',2,'Color','red');
end
imwrite(sobelImg, strcat('HoughOut','.',outFileType), outFileType);

%% Pyramidal Up
pyrdwnImg = impyramid(grayImg, 'reduce');
pyrupImg = impyramid(grayImg, 'expand');

figure
subplot(1,3,1)
imshow(pyrdwnImg,'InitialMagnification',imshowMag)
[height, width] = size(pyrdwnImg);
titleStr = sprintf('Pyramidal Down Converted %dx%d', width, height);
title(titleStr);
imwrite(pyrdwnImg, strcat('PyrdwnOut','.',outFileType), outFileType)
subplot(1,3,2)
imshow(grayImg,'InitialMagnification',imshowMag)
[height, width] = size(grayImg);
titleStr = sprintf('Original Image %dx%d', width, height);
title(titleStr);
subplot(1,3,3)
imshow(pyrupImg,'InitialMagnification',imshowMag)
[height, width] = size(pyrupImg);
titleStr = sprintf('Pyramidal Up Converted %dx%d', width, height);
title(titleStr);
imwrite(pyrupImg, strcat('PyrupOut','.',outFileType), outFileType)