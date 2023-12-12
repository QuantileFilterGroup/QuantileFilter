# QuantileFilter
This repository contains the C++ implementation of QuantileFilter, the first approximate algorithm specifically designed for detecting quantile-outstanding keys. We compare different aspects of performance of QuantileFilter and other quantiles sketches.

## Project Structure

```
├─gen_answer------------------------To generate ground truths
│
├─gen_zipf--------------------------To generate zipf dataset
│
├─parameter_verify------------------To find the best parameters for QuantileFilter
│  │
│  ├─test_block_length--------------To find the best block length
│  │
│  ├─test_memory_proportion---------To find the best memory proportion
│  │
│  ├─test_scaling-------------------To find the best algorithm variant of QuantileFilter
│  │
│  └─test_vague_number--------------To find the best number of hash functions in vague part
│
├─QuantileFilter--------------------source code of our algorithm, QuantileFilter
│
├─related_work----------------------source code of related works and comparisons
│  │
│  ├─HistSketch-master--------------source code of HistSketch
│  │
│  ├─SketchPolymer-code-main--------source code of SketchPolymer
│  │
│  ├─squad--------------------------source code of SQUAD
│  │
│  ├─test_percentage----------------To test algorithms on different percentages
│  │
│  ├─test_throughput----------------To compare the throughput of different algorithms
│  │
│  └─test_zipf----------------------To test algorithms on zipf dataset
│
└─variants--------------------------Possible variants of QuantileFilter
```

## Installation & Run

QuantileFilter can run on both Windows and Linux platforms. Navigate to the QuantileFilter directory using the command `cd QuantileFilter` and execute either `make.bat` or `make.sh` to compile QuantileFilter.

If you want to test different algorithms or different parameters for QuantileFilter, navigate to the respective subdirectory until you find `runall.bat` or `runall.sh` to compile and run it. You may need to prepare test data yourself and modify the default input and output file names in the code to ensure smooth execution. After running `runall`, you can use `painter.py` or `mypainter.py` in the parent directory to generate images.


## Reference

[1] HistSketch: https://github.com/N2-Sys/HistSketch

[2] Rana Shahout, Roy Friedman and Ran Ben Basat. [Together is Better: Heavy Hitters Quantile Estimation. 2023.] (https://dl.acm.org/doi/abs/10.1145/3588937)

[3] SketchPolymer: https://github.com/SketchPolymer/SketchPolymer-code

[4] CAIDA dataset: http://www.caida.org/data/passive/passive_dataset.xml

[5] Yahoo dataset: https://webscope.sandbox.yahoo.com/catalog.php?datatype=g
