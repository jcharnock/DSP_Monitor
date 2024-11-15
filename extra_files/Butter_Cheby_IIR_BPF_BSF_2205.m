% CLICK IN THIS TEXT BOX. USE Ctrl-A TO SELECT ALL.
% PASTE TEXT INTO MATLAB SCRIPT. RUN.
% Butter_Cheby_IIR_BPF_BSF_2195.m
 
clearvars; close all; clc

%  Print MATLAB flag
printMATLABFlag = true;
 
fs = 600;  % BPM sampling frequency
ff = fs/2; % BPM folding frequency
 
% NOTE: The Matlab fractional frequency normalized to folding
% NOT sampling frequency. (A bit odd, at least.)
 
%  Create an angular frequency variable from 0 to pi with 4096 values
w = linspace(0,pi,4096); % freqz() requires 0..pi range

%  Normalize the frequency to BPM
fbpm = (ff/pi)*w;
 
% ************************************************************
% Create filter transfer function.
% Comment select filter type.
 
nOrder = 4;
cutBPM = [4.5 75]; % band pass/stop edge frequencies in BPM
 
xDisplayRange = [0 200];
 
%-------- Butterworth BPF ----------------------------------
% [b,a] = butter(nOrder,cutBPM/ff,'bandpass');
% comment_txt = sprintf...
%     ('BWRTH BANDPASS, order %d, [%d %d] BPM\n',...
%     nOrder,cutBPM(1),cutBPM(2));
 
% %-------- Butterworth BSF ----------------------------------
% [b,a] = butter(nOrder,cutBPM/ff,'stop');
% comment_txt = sprintf...
%     ('BWRTH BANDSTOP, order %d, [%d %d] BPM\n',...
%     nOrder,cutBPM(1),cutBPM(2));
 
% %-------- Cehyshev BPF -------------------------------------
R = 0.5; % dB passband ripple
[b,a] = cheby1(nOrder,R,cutBPM/ff,'bandpass');
comment_txt = sprintf...
    ('CHEBY BANDPASS, order %d, R = %0.2f, [%d %d] BPM\n',...
    nOrder,R,cutBPM(1),cutBPM(2));
 
% %-------- Cehyshev BSF -------------------------------------
% R = 0.25; % dB passband ripple
% [b,a] = cheby1(nOrder,R,cutBPM/ff,'stop');
% comment_txt = sprintf...
%     ('CHEBY BANDSTOP, order %d, R = %0.2f, [%d %d] BPM\n',...
%     nOrder,R,cutBPM(1),cutBPM(2));
 
% ************************************************************
% Console output transfer function
Tsmpl = 0.1;
Tfilt = tf(b,a,Tsmpl,'variable','z^-1')
 
%---------------------------------------------------
% Console output pole and zero locations
 
zeros_b = roots(b)
poles_a = roots(a)
pole_wn = abs(poles_a) % display pole magnitudes
 
%---------------------------------------------------
% Console output pole-zero plot for nPole pole filter
fig1 = figure(1);
zplane(b,a);
title('Filter Pole-Zero Plot');
 
%---------------------------------------------------
% Plot impulse response
 
[hir,smpl] = impz(b,a);
maxN = find((abs(hir)/max(hir)) > 0.01, 1,'last');
 
fig2 = figure(2);
plot(smpl,hir,'b','linewidth',2); grid on;
% stem(smpl,hir,'b','linewidth',2); grid on;
if ~isempty(maxN)
    xlim([0 maxN]);
else
    xlim(xDisplayRange);
end
xlabel('sample');
title('Impulse Response');
 
%---------------------------------------------------
% Plot cascade and sos component frequency response
 
[sos,G1] = tf2sos(b,a); % display second order systems
[rsos,~] = size(sos);
 
fig3 = figure(3);
grid on;
xlim(xDisplayRange); yrng = ylim; ylim([0 yrng(2)]);
xlabel('Breaths per Minute');
title('Partial BPF Frequency Responses');
haccum = ones(size(w));
for i = 1:nOrder
    hsos = abs(freqz(sos(i,1:3),sos(i,4:6),w));
    haccum = haccum.*hsos;
    plot(fbpm,hsos); grid on;
    xlim(xDisplayRange);
end
 
plot(fbpm,G1*haccum,'linewidth',2);
grid on;
xlim(xDisplayRange); yrng = ylim; ylim([0 1.1]);
xlabel('Breaths per Minute');
title(['BPF Frequency Response: order ', num2str(nOrder)]);
 
%---------------------------------------------------
% Console output transfer function coefficients in
% C declaration form

fprintf('\n ****  C Code for Direct IIR Filter -- Copy to Arduino IDE ****\n\n');

 
fprintf(['// ' comment_txt]);
fprintf('const int MFILT = %d;\n', length(b));
bScale = max(b);
bb = b/bScale;
fprintf('static float GAIN = %0.6g;\n', bScale);
fprintf('static float b[] = {%0.7f',  bb(1));
for i = 2:length(bb)
    fprintf(', %0.7f',bb(i));
end
fprintf('};\n');
 
fprintf('static float a[] = {%0.7f', a(1));
for i = 2:length(a)
    fprintf(', %0.7f', a(i));
end
fprintf('};\n\n');
 
% %---------------------------------------------------
% % Console output sos tf coefficients in C declaration form
%  
% fprintf(['// ' comment_txt]);
% sSize = size(sos);
% stageG = G1^(1/sSize(1));
% for i = 1:rsos
%     fprintf('static float G%d = %0.7f;\n', i, G1^(1/rsos));
%     fprintf('static float b%d[] = {%0.7f, %0.7f, %0.7f};\n',...
%         i,sos(i,1),sos(i,2),sos(i,3));
%     fprintf('static float a%d[] = {%0.7f, %0.7f, %0.7f};\n',...
%         i,sos(i,4),sos(i,5),sos(i,6));
% end

%---------------------------------------------------
% Console output sos tf coefficients in C declaration form
 
% fprintf(['// ' comment_txt]);
sSize = size(sos);
stageG = G1^(1/sSize(1));

fprintf('\n ****  C Code for SOS Filter -- Copy to Arduino IDE ****\n');

%  Variable declarations
fprintf('\n//Filter specific variable declarations\n')
fprintf('const int numStages = %d;\n', rsos);
fprintf('static float G[numStages];\n');
fprintf('static float b[numStages][3];\n');
fprintf('static float a[numStages][3];\n');

%  Variable initialization
fprintf(['\n\n// ' comment_txt]);
for i = 1:rsos
    fprintf('G[%d] = %0.7f;\n', i-1, G1^(1/rsos));

    %     fprintf('b[%d][] = {%0.7f, %0.7f, %0.7f};\n',...
%         i-1,sos(i,1),sos(i,2),sos(i,3));
    
     fprintf('b[%d][0] = %0.7f; b[%d][1] = %0.7f; b[%d][2]= %0.7f;\n',...
        i-1,sos(i,1),i-1,sos(i,2),i-1,sos(i,3));
    
%     fprintf('a[%d][] = {%0.7f, %0.7f, %0.7f};\n',...
%         i-1,sos(i,4),sos(i,5),sos(i,6));
 
fprintf('a[%d][0] = %0.7f; a[%d][1] =  %0.7f; a[%d][2] =  %0.7f;\n',...
        i-1,sos(i,4),i-1, sos(i,5),i-1, sos(i,6));
end

%%  Print the coefficients so that they can easily be copied into MATLAB

if printMATLABFlag
    
    %  If the printMATLABFlag is true then output the impulse response so
    %  that it can easily be copied into MATLAB
    
 fprintf('\n\nDirect IIR Filter Coefficients for easy copy to MATLAB\n\n');    
    
   
    numColumns = 14;  %  Number of columns in each row
    
   
%  Print the numerator b coefficients for easy copy to MATLAB
    
    for iCoeff = 1:length(b)-1
        
               
        if iCoeff == 1
            fprintf('b = [');
        end
        
        fprintf( '%3.7f', b(iCoeff) );
        
        if mod(iCoeff,numColumns) == 0
            fprintf( ',...\n');
        else
            fprintf(', ');
        end
    end
   
    % Print the last value
    
    fprintf( '%3.7f];\n', b(end) )
    
    
%  Print the numerator a coefficients for easy copy to MATLAB
    
    for iCoeff = 1:length(a)-1
        
               
        if iCoeff == 1
            fprintf('a = [');
        end
        
        fprintf( '%3.7f', a(iCoeff) );
        
        if mod(iCoeff,numColumns) == 0
            fprintf( ',...\n');
        else
            fprintf(', ');
        end
    end
   
    % Print the last value
    
    fprintf( '%3.7f];\n', a(end) )    
end

return
 

