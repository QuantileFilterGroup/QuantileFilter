import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import math
import numpy as np

names = [
    "optimized_sketch_larger_cs_compressed",
    "SketchPolymer_delta_fixed",
    "HistSketch_server",
    "squad"
]

real_names = [
    "Ours",
    "SketchPolymer",
    "HistSketch",
    "SQUAD"
]

full_paths = ["./" + name + "/summary.csv" for name in names]
dfs = [pd.read_csv (name) for name in full_paths]
for i, name in enumerate (names):
    (dfs [i])['name'] = name
for i, name in enumerate (real_names):
    (dfs [i])['real_name'] = name

df = pd.concat (dfs, axis = 0)
df ['Log_Memory'] = df ['Memory'].apply (lambda x: math.log (x, 2))


sns.set_theme (style = "ticks",
                rc = {"grid.linestyle" : "--",
                    "axes.grid.axis": "y",
                    "axes.grid": True}
            )

def fig (x, y, xlabel, ylabel, style, data, name):
    plt.figure (figsize = (6, 5))
    sns.lineplot (x = x, y = y, style = style, hue = style,
                    markers = True, dashes=False, data = data,
                    linewidth=2.5, markersize=12)
    plt.ylabel(ylabel, fontweight='bold', fontsize = 22, labelpad = -5)
    plt.xlabel(xlabel, fontweight='bold', fontsize = 22, labelpad = 5)
    plt.subplots_adjust (bottom = 0.18, right = 0.95, left = 0.17, top = 0.77)
    legend = plt.legend(loc='upper center', bbox_to_anchor=(0.42, 1.4), ncol=2, prop={'weight': 'bold', 'size' : 20})
    for line in legend.get_lines():
        line.set_linewidth(2.5)
        line.set_markersize(12)
    # legend.get_ ().set_fontweight (1000)
    plt.tick_params(axis='both', which='major', labelsize = 18, pad = 8)
    plt.tick_params(axis='x', which='major', labelsize = 22, pad = 8)
    origin = 14
    xticks = np.linspace(15, 30, 6)
    plt.xticks (xticks, [f'$2^{{{int (i)}}}$' for i in xticks])
    plt.xlim (origin, 31)
    plt.ylim (-3, None)
    plt.savefig(f'{name}.pdf')
    plt.show()

# F1 Score
fig ('Log_Memory', 'F1_Score', 'Memory (Bytes)',
        'F1 Score', 'real_name', df, 'F1_score_yahoo_0')

# Precision
fig ('Log_Memory', 'Precision', 'Memory (Bytes)',
        'Precision', 'real_name', df, 'Precision_yahoo_0')

# Recall
fig ('Log_Memory', 'Recall', 'Memory (Bytes)',
        'Recall', 'real_name', df, 'Recall_yahoo_0')
