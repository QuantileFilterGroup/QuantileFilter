import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import math

names = [
	"raw_cms",
	"raw_cs",
	"raw_balanced",
	"raw_imbalanced",
	"optimized_sketch_direct_cms",
	"optimized_sketch_direct_cs",
	"optimized_sketch_larger_cms",
	"optimized_sketch_larger_cms_compressed",
	"optimized_sketch_larger_cs",
	"optimized_sketch_larger_cs_compressed",
	"optimized_sketch_probability_cms",
	"optimized_sketch_probability_cs",
	"SketchPolymer",
	"SketchPolymer_delta_fixed",
	"HistSketch_server",
	"squad"
]

full_paths = ["./" + name + "/summary.csv" for name in names]
dfs = [pd.read_csv (name) for name in full_paths]
for i, name in enumerate (names):
	(dfs [i])['name'] = name

df = pd.concat (dfs, axis = 0)
df ['Log_Memory'] = df ['Memory'].apply (lambda x: math.log (x, 2))

sns.set (style = "darkgrid")

# F1 Score
fig = plt.figure (figsize = (10, 6))
gs = gridspec.GridSpec(1, 2, width_ratios=[3, 1])
ax1 = plt.subplot(gs[0])
sns.lineplot (x = 'Log_Memory', y = 'F1_Score', hue = 'name', style = 'name', markers = True, data = df)
ax1.set_title('F1 Score vs. Log Memory')
ax1.set_xlabel ('Log Memory (Bytes, Base 2 Logarithm)')
ax1.set_ylabel ('F1 Score')
ax1.legend(loc='upper right', title = 'Algorithm Name', bbox_to_anchor=(1.6, 1))
ax2 = plt.subplot(gs[1])
ax2.axis('off')
plt.savefig('F1_score.png')
plt.show ()

# Precision
fig = plt.figure (figsize = (10, 6))
gs = gridspec.GridSpec(1, 2, width_ratios=[3, 1])
ax1 = plt.subplot(gs[0])
sns.lineplot (x = 'Log_Memory', y = 'Precision', hue = 'name', style = 'name', markers = True, data = df)
ax1.set_title('Precision vs. Log Memory')
ax1.set_xlabel ('Log Memory (Bytes, Base 2 Logarithm)')
ax1.set_ylabel ('Precision')
ax1.legend(loc='upper right', title = 'Algorithm Name', bbox_to_anchor=(1.6, 1))
ax2 = plt.subplot(gs[1])
ax2.axis('off')
plt.savefig('Precision.png')
plt.show ()


# Recall
fig = plt.figure (figsize = (10, 6))
gs = gridspec.GridSpec(1, 2, width_ratios=[3, 1])
ax1 = plt.subplot(gs[0])
sns.lineplot (x = 'Log_Memory', y = 'Recall', hue = 'name', style = 'name', markers = True, data = df)
ax1.set_title('Recall vs. Log Memory')
ax1.set_xlabel ('Log Memory (Bytes, Base 2 Logarithm)')
ax1.set_ylabel ('Recall')
ax1.legend(loc='upper right', title = 'Algorithm Name', bbox_to_anchor=(1.6, 1))
ax2 = plt.subplot(gs[1])
ax2.axis('off')
plt.savefig('Recall.png')
plt.show ()
