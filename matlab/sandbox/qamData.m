function rawData = qamData(m, T)

% m is the number of points in constellation
% Test BPSK Signal
fc = 2048;
rb = 2048;
fs = 8192;

if m == 4
  scale = 1;
elseif m == 16
  scale = 2;
elseif m == 64
  scale = 4;
end


t = linspace(0,T-1/fs, T*fs);  % This makes it 4 samples per symbol exactly
carrier1 = sin(2*pi*fc*t);
carrier2 = cos(2*pi*fc*t);

sampPerSym = fs/fc;
input1 = [];
input2 = [];
% There has to be a better way to do this but whatever
while (length(input1) < length(carrier1))
  input1 = [input1 ceil(scale*rand)*(2*round(rand)-1)*ones(1, sampPerSym)];
  input2 = [input2 ceil(scale*rand)*(2*round(rand)-1)*ones(1, sampPerSym)];
endwhile

rawData = input1.*carrier1 + input2.*carrier2;