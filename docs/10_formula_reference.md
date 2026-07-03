# 10. Formula Reference

## 1. Sampling

\[
T_s = 5ms = 0.005s
\]

\[
F_s=\frac{1}{T_s}=200Hz
\]

## 2. ADC conversion

\[
V_{in}[n]=\frac{ADC_{raw}[n]}{4095}V_{REF}
\]

## 3. HPF

\[
y_{HPF}[n]=\alpha(y_{HPF}[n-1]+x[n]-x[n-1])
\]

where:

\[
\alpha=0.995
\]

## 4. LPF

\[
y_{LPF}[n]=y_{LPF}[n-1]+\beta(x[n]-y_{LPF}[n-1])
\]

where:

\[
\beta=0.075
\]

## 5. Biquad DF2T

\[
y[n]=b_0x[n]+s_1[n-1]
\]

\[
s_1[n]=b_1x[n]+s_2[n-1]-a_1y[n]
\]

\[
s_2[n]=b_2x[n]-a_2y[n]
\]

## 6. Envelope amplitude

\[
A[n]=
\begin{cases}
\gamma_a |y[n]|+(1-\gamma_a)A[n-1], & |y[n]|>A[n-1] \\
\gamma_d |y[n]|+(1-\gamma_d)A[n-1], & |y[n]|\le A[n-1]
\end{cases}
\]

where:

\[
\gamma_a=0.40, \quad \gamma_d=0.02
\]

## 7. Adaptive threshold

\[
thr[n]=K A[n]
\]

where:

\[
K=0.10
\]

## 8. Derivatives

\[
d_1[n]=y[n]-y[n-1]
\]

\[
d_2[n]=d_1[n]-d_1[n-1]
\]

## 9. Refractory

\[
t_{now}-t_{lastpeak}>300ms
\]

## 10. IBI

\[
IBI_{ms}=t_{peak,k}-t_{peak,k-1}
\]

valid range:

\[
250ms \le IBI_{ms} \le 2000ms
\]

## 11. BPM

\[
BPM=\frac{60000}{IBI_{ms}}
\]

## 12. EAR

\[
EAR=\frac{\lVert p_2-p_6\rVert+\lVert p_3-p_5\rVert}{2\lVert p_1-p_4\rVert}
\]

## 13. Drowsiness decision

\[
Drowsy=(EAR<0.22)\land(t_{closed}\ge 2000ms)
\]

## 14. LCD BPM bar

`client.c` maps BPM range 50~120 to 16 LCD columns.

\[
norm=\frac{BPM-50}{120-50}
\]

\[
bars=round(norm\cdot16)
\]

## 15. TCP packet

\[
Packet = SerialNumber + ',' + BPM + ',' + Status
\]

Example:

```text
SN-RPI-001,82,1
```
