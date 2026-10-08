"""Plot the saved C++ Monte Carlo results. Optional dependency: matplotlib.
Core C++ labs do not require Python. No simulated result is fabricated here.
"""
import csv
import math
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties

ROOT = Path(__file__).resolve().parent.parent
font_path = Path('/System/Library/Fonts/STHeiti Medium.ttc')
font = FontProperties(fname=str(font_path)) if font_path.exists() else FontProperties()
plt.rcParams.update({'font.family': 'DejaVu Sans', 'font.size': 12,
                     'axes.spines.top': False, 'axes.spines.right': False})
fig, axes = plt.subplots(1, 2, figsize=(13.3, 5.4), layout='constrained')
colors = ['#215d8c', '#147c75']
for index, (filename, label) in enumerate([('qpsk.csv', 'QPSK'), ('qam16.csv', '16QAM')]):
    with (ROOT/'cpp/results'/filename).open() as file:
        rows = list(csv.DictReader(file))
    x = [float(r['snr_data_esn0_db']) for r in rows]
    nonzero = [r for r in rows if int(r['errors']) > 0]
    zero = [r for r in rows if int(r['errors']) == 0]
    axes[0].semilogy([float(r['snr_data_esn0_db']) for r in nonzero],
                     [float(r['ber']) for r in nonzero], '-o', color=colors[index], label=label)
    axes[0].scatter([float(r['snr_data_esn0_db']) for r in zero],
                    [3/int(r['bits']) for r in zero], marker='v', s=65,
                    facecolors='none', edgecolors=colors[index],
                    label=label+' zero errors: 95% upper bound')
    axes[1].plot(x, [float(r['evm_rms']) for r in rows], '-o', color=colors[index], label=label)
fine = [i/20 for i in range(401)]
qpsk = [.5*math.erfc(math.sqrt(10**(s/10))/math.sqrt(2)) for s in fine]
axes[0].semilogy(fine, qpsk, '--', color='#748397', label='QPSK theoretical BER')
axes[0].set_ylim(1e-6, 1)
axes[1].plot(fine, [10**(-s/20) for s in fine], '--', color='#748397', label='Theoretical RMS EVM')
for ax in axes:
    ax.set_xlabel('Data-symbol Es/N0 (dB)')
    ax.set_xlim(-.5, 20.5)
    ax.grid(True, which='both', alpha=.2)
    ax.legend(fontsize=9)
axes[0].set_ylabel('Uncoded bit error rate')
axes[1].set_ylabel('RMS EVM (linear ratio)')
fig.suptitle('未编码调制基线：真实 C++ 运行计数与理论对照', fontproperties=font, fontsize=17)
fig.text(.5, -.03, '100 batches per point | 1380 symbols per batch | h=1, perfect synchronization | no LDPC/HARQ',
         ha='center', fontsize=10, color='#617285')
fig.savefig(ROOT/'assets/developer_ber.png', dpi=150, bbox_inches='tight')
fig.savefig(ROOT/'assets/developer_ber.svg', bbox_inches='tight')
plt.close(fig)
print(ROOT/'assets/developer_ber.png')
