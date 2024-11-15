% CLICK IN THIS TEXT BOX. USE Ctrl-A TO SELECT ALL.
% USE Ctrl-V TO PASTE TEXT INTO MATLAB SCRIPT. RUN.
% Butter_Cheby_IIR_LPF_HPF_2195.m
 
clearvars; close all; clc

%  Print MATLAB flag
printMATLABFlag = true;
 
fs = 600;  % BPM sampling frequency
ff = fs/2; % BPM folding frequency
 
% NOTE: The Matlab fractional frequency normalized to folding
% NOT sampling frequency. (A bit odd, at least.)
 
w = linspace(0,pi,4096); % freqz() requires 0..pi range
fbpm = (ff/pi)*w;
 
% ************************************************************
% Create Butterworth or Chebyshev filters.
% Comment select filter specification.
 
nOrder = 7;  %  Order of the filter that is the number of poles
cutBPM = 40; % pass/stop edge frequency given that fs = 600 bpm
xDisplayRange = [0 100];
 
%  Comment select the filter type that you want paramters for.  That is
%  Butterworth or Chebyshev
%
%  Select the filter (low pass or highpass) by changing the value
%  of the variable fType to either 'LOW' or 'HIGH'

%-------- Butterworth LPF, HPF -------------------------------
% fType = 'LOW'; % LOW, HIGH
% [b,a] = butter(nOrder,cutBPM/ff,fType);
% comment_txt = sprintf...
%     ('BWRTH %s, order %d, %d BPM\n',...
%     fType,nOrder,cutBPM);


%  Set the value of the ripple of the filter in decibels
%
% %-------- Cehyshev LPF, HPF ----------------------------------
R = 0.5; % dB passband ripple
fType = 'LOW'; % LOW, HIGH
[b,a] = cheby1(nOrder,R,cutBPM/ff,fType);
comment_txt = sprintf...
    ('CHEBY %s, order %d, R = %0.1f, %d BPM\n',...
    fType,nOrder,R,cutBPM);



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
figLoc = [150 150 300 250];
% set(fig1,'position',figLoc);
 
%---------------------------------------------------
% Plot impulse response
 
[hir,smpl] = impz(b,a);
hsr = cumsum(hir);
maxN = find((abs(hir)/max(hir)) > 0.01, 1,'last');
 
fig2 = figure(2);
plot(smpl,hir,'b','linewidth',2); grid on;
% stem(smpl,hir,'b','linewidth',2); grid on;
% xlim([0 maxN]);
xlabel('sample');
title('Impulse Response');
% set(fig2,'position',figLoc);
 
%---------------------------------------------------
% Plot cascade and sos component frequency response
 
[sos,G1] = tf2sos(b,a); % display second order systems
[rsos,~] = size(sos);
Gpole = G1^(1/nOrder);
Gstage = ones(1,rsos)*(Gpole^2);
if (mod(nOrder,2) == 1); Gstage(1) = Gpole; end
 
fig3 = figure(3);
grid on; hold on;
% set(fig3,'position',figLoc);
 
haccum = ones(size(w));
for i = 1:rsos
    hsos = Gstage(i)*abs(freqz(sos(i,1:3),sos(i,4:6),w)); % 
    haccum = haccum.*hsos;
    plot(fbpm,hsos,'LineWidth',2)
end
plot(fbpm,haccum,'k')
hold off;
xlim(xDisplayRange);
yrng = ylim; ylim([0 yrng(2)]);
xlabel('Breaths per Minute');
title(['Frequency Response(s): order ',num2str(nOrder)]);
    
%---------------------------------------------------
% Plot cascade frequency response
 
fig4 = figure(4);
plot(fbpm,abs(freqz(b,a,w)),'b','linewidth',2)
grid on;
xlim(xDisplayRange); ylim([0 1.1]);
xlabel('Breaths per Minute');
title(['Frequency Response: order ',num2str(nOrder)]);
% set(fig4,'position',figLoc);
 
%---------------------------------------------------
% Console output transfer function coefficients in
% C declaration form

fprintf('\n ****  C Code for Direct IIR Filter -- Copy to Arduino IDE ****\n\n');

 
fprintf(['  // ' comment_txt]);
fprintf('  const int MFILT = %d;\n', length(b));
bScale = max(b);
bb = b/bScale;
fprintf('  static float GAIN = %0.6g;\n', bScale);
fprintf('  static float b[] = {%0.7f',  bb(1));
% fprintf('  static float b[13] = {%0.7f',  bb(1));
for i = 2:length(bb)
    fprintf(', %0.7f',bb(i));
end
fprintf('};\n');
 
fprintf('  static float a[] = {%0.7f', a(1));
% fprintf('  static float a[13] = {%0.7f', a(1));
for i = 2:length(a)
    fprintf(', %0.7f', a(i));
end
fprintf('};\n\n');
 
% %---------------------------------------------------
% % Console output sos tf coefficients in C declaration form
%  
% fprintf(['  // ' comment_txt]);
% for i = 1:rsos
%     fprintf('  static float G%d = %0.7f;\n', i, Gstage(i));
%     fprintf('  static float b%d[] = {%0.7f, %0.7f, %0.7f};\n',...
%         i,sos(i,1),sos(i,2),sos(i,3));
%     fprintf('  static float a%d[] = {%0.7f, %0.7f, %0.7f};\n',...
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
 

