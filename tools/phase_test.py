import sys
import wave
import numpy as np


if len(sys.argv) != 2:
    print('Usage: {} multi.wav'.format(sys.argv[0]))
    sys.exit(1)


multi = wave.open(sys.argv[1], 'rb')
rate = multi.getframerate()
channels = multi.getnchannels()

if channels <= 1:
    sys.exit(1)

N = rate 

window = np.hanning(N)

interp = 4*8
max_offset = int(rate * 0.1 / 340 * interp)

def gcc_phat(sig, refsig, fs=1, max_tau=None, interp=16):
    '''
    This function computes the offset between the signal sig and the reference signal refsig
    using the Generalized Cross Correlation - Phase Transform (GCC-PHAT)method.
    '''
    
    # make sure the length for the FFT is larger or equal than len(sig) + len(refsig)
    n = sig.shape[0] + refsig.shape[0]

    # Generalized Cross Correlation Phase Transform
    SIG = np.fft.rfft(sig, n=n)
    REFSIG = np.fft.rfft(refsig, n=n)
    R = SIG * np.conj(REFSIG)
    #R /= np.abs(R)

    cc = np.fft.irfft(R, n=(interp * n))

    max_shift = int(interp * n / 2)
    if max_tau:
        max_shift = np.minimum(int(interp * fs * max_tau), max_shift)

    cc = np.concatenate((cc[-max_shift:], cc[:max_shift+1]))

    # find max cross correlation index
    shift = np.argmax(np.abs(cc)) - max_shift

    tau = shift / float(interp * fs)
    
    return tau, cc


print(multi.getsampwidth())

while True:
    data = multi.readframes(N)

    if len(data) != multi.getsampwidth() * N * channels:
        print("done")
        break

    if multi.getsampwidth() == 2:
        data = np.fromstring(data, dtype='int16')
    else:
        data = np.fromstring(data, dtype='int32')
    ref_buf = data[0::channels]

    offsets = []
    for ch in range(1, channels):
        sig_buf = data[ch::channels]
        tau, _ = gcc_phat(sig_buf * window, ref_buf * window, fs=1, max_tau=max_offset, interp=interp)
        # tau, _ = gcc_phat(sig_buf, ref_buf, fs=rate, max_tau=1)

        offsets.append(tau)

    print(offsets)

print(multi.getframerate())

multi.close()
