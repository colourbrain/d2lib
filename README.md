# d2lib
`d2lib` is a C++ library of discrete distribution (d2) based 
__large-scale__ data processing framework. It supports the data analysis
of distributions at scale, such as nearest neighbors, clustering, and
some other machine learning capability. `d2lib` uses templates and C++11 features 
a lot, aiming to maximize its extensibility for different types of data.

`d2lib` also contains a collection of computing tools supporting the analysis 
of typical d2 data, such as images, sequences, documents.

*[under construction]*

Dependencies
 - BLAS
 - [rabit](https://github.com/dmlc/rabit): the use of generic parallel infrastructure
 - [mosek](https://www.mosek.com): fast LP/QP solvers, academic license available.

Make sure you have those pre-compiled libraries installed and
configured in the [d2lib/make.inc](d2lib/make.inc).
```bash
cd d2lib && make && make test
```

## Introduction
### Data Format Specifications
 - discrete distribution over Euclidean space
 - discrete distribution with finite possible supports in Euclidean space (e.g., bag-of-word-vectors and sparsified histograms)
 - n-gram data with cross-term distance
 - dense histogram

### Basic Functions
 - distributed/serial IO 
 - compute distance between a pair of D2: [Wasserstein distance](http://en.wikipedia.org/wiki/Wasserstein_metric) (or EMD), [Sinkhorn Distances](http://www.iip.ist.i.kyoto-u.ac.jp/member/cuturi/SI.html) (entropic regularized optimal transport).


### Learnings
 - nearest neighbors [TBA]
 - D2-clustering [TBA]
 - Dirichlet process [TBA]

## Other Tools
 - document analysis: from bag-of-words to .d2s format [TBA]

