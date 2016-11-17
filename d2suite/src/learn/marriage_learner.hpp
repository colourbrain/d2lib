#ifndef _MARRIAGE_LEARNER_H_
#define _MARRIAGE_LEARNER_H_

#include <rabit/rabit.h>
#include "../common/common.hpp"
#include "../common/d2.hpp"
#include "../common/cblas.h"
#include "../common/d2_badmm.hpp"

namespace d2 {
  /*!
   * \brief The predicting utility of marriage learning using the winner-take-all method
   * \param data the data block to be predicted
   * \param learner the set of classifiers learnt via marriage learning
   * \param write_label if false, compute the accuracy, otherwise overwrite labels to data
   */
  template <typename ElemType1, typename FuncType, size_t dim>
  real_t ML_Predict_ByWinnerTakeAll(Block<ElemType1> &data,
				    const Elem<def::Function<FuncType>, dim> &learner,
				    bool write_label = false,
				    std::vector<real_t> *scores = NULL) {
    using namespace rabit;
    real_t *y;
    y = new real_t[data.get_size()];
    for (size_t i=0; i<data.get_size(); ++i) y[i] = data[i].label[0];

    real_t *C = new real_t [data.get_col() * learner.len];
    real_t *emds;
    if (write_label && scores) {
      scores->resize(data.get_col() * learner.len);
      emds = &(*scores)[0];
    } else {
      assert((write_label && scores) || !write_label);
      emds = new real_t [data.get_size() * FuncType::NUMBER_OF_CLASSES];
    }
    for (size_t i=0; i<FuncType::NUMBER_OF_CLASSES; ++i) {
      _pdist2_label(learner.supp, learner.len,
		    data.get_support_ptr(), i, data.get_col(),
		    data.meta, C);
      EMD(learner, data, emds + data.get_size() * i, C, NULL, NULL, true);
    }


    real_t accuracy = 0.0;    
    if (false) {
      
    } else {
      for (size_t i=0; i<data.get_size(); ++i) {
	real_t *emd = emds + i;
	size_t label = 1;
	real_t min_emd = emd[data.get_size()];
	for (size_t j=2; j<FuncType::NUMBER_OF_CLASSES; ++j) {
	  if (emd[j*data.get_size()] < min_emd) {
	    min_emd = emd[j*data.get_size()];
	    label = j;
	  }
	}
	accuracy += label == (size_t) y[i];
      }
      size_t global_size = data.get_size();
      Allreduce<op::Sum>(&global_size, 1);
      Allreduce<op::Sum>(&accuracy, 1);
      accuracy /= global_size;      
    }

    
    delete [] y;
    delete [] C;
    if (!(write_label && scores))
      delete [] emds;
    
    return accuracy;    
  }

  /*!
   * \brief The predicting utility of marriage learning using the (multimarginal) voting method
   * \param data the data block to be predicted
   * \param learner the set of classifiers learnt via marriage learning
   * \param write_label if false, compute the accuracy, otherwise overwrite labels to data
   */
  template <typename ElemType1, typename FuncType, size_t dim>
  real_t ML_Predict_ByVoting(Block<ElemType1> &data,
			     const Elem<def::Function<FuncType>, dim> &learner,
			     bool write_label = false,
			     std::vector<real_t> *class_proportion = NULL) {
    using namespace rabit;
    if (write_label && class_proportion) {
      class_proportion->resize(data.get_size() * FuncType::NUMBER_OF_CLASSES);
    } else {
      assert((write_label && class_proportion) || !write_label);
    }
    real_t *y, *label_cache;
    y = new real_t[data.get_size()];
    label_cache = new real_t[data.get_col()];
    memcpy(label_cache, data.get_label_ptr(), sizeof(real_t) * data.get_col());
    for (size_t i=0; i<data.get_size(); ++i) y[i] = data[i].label[0];

    const size_t mat_size = data.get_col() * learner.len;
    real_t *C    = new real_t [mat_size * FuncType::NUMBER_OF_CLASSES];
    real_t *minC = new real_t [mat_size];
    real_t *Pi   = new real_t [mat_size];
    size_t *index= new size_t [mat_size];

    _pdist2_alllabel(learner.supp, learner.len,
		     data.get_support_ptr(), data.get_col(),
		     data.meta, C);

    for (size_t i=0; i<mat_size; ++i) {
      real_t minC_value = std::numeric_limits<real_t>::max();
      size_t minC_index = -1;
      for (size_t j=1; j<FuncType::NUMBER_OF_CLASSES; ++j) {
	if (minC_value > C[i+j*mat_size]) {
	  minC_value = C[i+j*mat_size];
	  minC_index = j;
	}
      }
      minC[i] = minC_value - C[i];
      index[i] = minC_index;
    }
    EMD(learner, data, NULL, minC, Pi, NULL, true);        

    real_t accuracy = 0.0;    
    if (false) {
      
    } else {
      real_t *Pi_ptr = Pi;
      size_t *index_ptr = index;
      for (size_t i=0; i<data.get_size(); ++i) {
	const size_t ms = learner.len * data[i].len;
	real_t thislabel[FuncType::NUMBER_OF_CLASSES] ={};
	for (size_t j=0; j<ms; ++j) {
	  thislabel[index_ptr[j]] += Pi_ptr[j];	  
	}
	index_ptr += ms;
	Pi_ptr    += ms;

	const real_t w = thislabel[(size_t) y[i]];
	size_t label = std::max_element(thislabel+1, thislabel+FuncType::NUMBER_OF_CLASSES) - thislabel;
	if (write_label && class_proportion) {
	  memcpy(&class_proportion[i*FuncType::NUMBER_OF_CLASSES], thislabel, sizeof(real_t) * FuncType::NUMBER_OF_CLASSES);
	}
	accuracy += label == (size_t) y[i];
      }
      size_t global_size = data.get_size();

      Allreduce<op::Sum>(&global_size, 1);
      Allreduce<op::Sum>(&accuracy, 1);

      accuracy /= global_size;
      //printf("accuracy: %.3lf\n", accuracy);
      memcpy(data.get_label_ptr(), label_cache, sizeof(real_t) * data.get_col());
      
    }

    
    delete [] y;
    delete [] label_cache;
    delete [] C;
    delete [] minC;
    delete [] index;
    delete [] Pi;
    
    return accuracy;    
  }

  
  static real_t beta; /* an important parameter */

#ifdef _USE_SPARSE_ACCELERATE_
  const static bool sparse = true; /* only possible for def::WordVec */
#endif

  namespace internal {
    template <typename ElemType>
    void _get_sample_weight(const Block<ElemType> &data,
			    const real_t *Pi, size_t leading,
			    real_t *sample_weight, size_t sample_size) {
      for (size_t ii=0; ii<data.get_col(); ++ii) sample_weight[ii] = 0;
      _D2_CBLAS_FUNC(axpy)(data.get_col(),
			   - beta,
			   Pi, leading,
			   sample_weight, 1);
      _D2_CBLAS_FUNC(axpy)(data.get_col(),
			   beta,
			   data.get_weight_ptr(), 1,
			   sample_weight, 1);
      _D2_CBLAS_FUNC(copy)(data.get_col(),
			   Pi, leading,
			   sample_weight + data.get_col(), 1);    
    }

#ifdef _USE_SPARSE_ACCELERATE_
    template <size_t D>
    void _get_sample_weight(const Block<Elem<def::WordVec, D> > &data,
			    const real_t *Pi, size_t leading,
			    real_t *sample_weight, size_t sample_size) {
      using namespace rabit;
      for (size_t ii=0; ii<sample_size; ++ii) sample_weight[ii] = 0;
      for (size_t ii=0; ii<data.get_col(); ++ii) {
	real_t cur_w = Pi[ii*leading];
	sample_weight[data.get_support_ptr()[ii] +
		      data.meta.size * (size_t) data.get_label_ptr()[ii]] += cur_w;
	sample_weight[data.get_support_ptr()[ii]] += beta * (data.get_weight_ptr()[ii] - cur_w);
      }
      Allreduce<op::Sum>(sample_weight, sample_size);
    }
#endif

    template <typename ElemType>
    size_t _get_sample_size(const Block<ElemType> &data, size_t num_of_copies)
    {return data.get_col() * 2;}

#ifdef _USE_SPARSE_ACCELERATE_
    template <size_t D>
    size_t _get_sample_size(const Block<Elem<def::WordVec, D> > &data, size_t num_of_copies)
    {return data.meta.size * num_of_copies;}
#endif
  }


  namespace def {
    /*! \brief all hyper parameters of ML_BADMM
     */
    struct ML_BADMM_PARAM {
      size_t max_iter = 50; ///< the maximum number of iterations
      size_t badmm_iter = 50;///< the number of iterations used in badmm per update
      real_t rho = 10.; ///< the BADMM parameter
      real_t beta = 1.; ///< the relative weight of non-above class
      size_t restart = -1; ///< the number of iterations fulfilled to restart BADMM; -1 means disabled
      bool   bootstrap = false; ///< whether using bootstrap samples to initialize classifers
    };
  }
  /*!
   * \brief the marriage learning algorithm enabled by BADMM 
   * \param data a block of elements to train
   * \param learner a d2 element which has several classifiers of type FuncType
   * \param rho the BADMM parameter
   * \param val_data a vector of validation data
   */
  template <typename ElemType1, typename FuncType, size_t dim>
  void ML_BADMM (Block<ElemType1> &data,
		 Elem<def::Function<FuncType>, dim> &learner,
		 const def::ML_BADMM_PARAM &param,
		 std::vector<Block<ElemType1>* > &val_data) {    
    using namespace rabit;
    // basic initialization
    for (size_t i=0; i<learner.len; ++i) {
      learner.w[i] = 1. / learner.len;
      learner.supp[i].init();
      learner.supp[i].sync(i % rabit::GetWorldSize() );
    }

    size_t global_col = data.get_col();
    size_t global_size= data.get_size();    

    Allreduce<op::Sum>(&global_col, 1);
    Allreduce<op::Sum>(&global_size, 1);


#ifdef _USE_SPARSE_ACCELERATE_
    for (size_t i=0; i<learner.len; ++i)
      learner.supp[i].set_communicate(false);
#endif
    
    internal::BADMMCache badmm_cache_arr;
    real_t rho = param.rho;
    beta = param.beta / (learner.len - 1);
    assert(learner.len > 1);
    allocate_badmm_cache(data, learner, badmm_cache_arr);
    
    // initialization
    for (size_t j=0; j<data.get_col() * learner.len; ++j)
      badmm_cache_arr.Lambda[j] = 0;

    for (size_t k=0, l=0; k<data.get_col(); ++k)
      for (size_t j=0; j<learner.len; ++j, ++l) {
	badmm_cache_arr.Pi2[l] = data.get_weight_ptr()[k] * learner.w[j];
	badmm_cache_arr.Pi1[l] = badmm_cache_arr.Pi2[l];
      }

    real_t prim_res = 1., dual_res = 1., totalC = 0.;
    real_t old_prim_res, old_dual_res, old_totalC;
    real_t *X, *y;

#ifdef _USE_SPARSE_ACCELERATE_    
    internal::get_dense_if_need_mapped(data, &X, &y, FuncType::NUMBER_OF_CLASSES);
#else
    internal::get_dense_if_need_ec(data, &X, &y);
#endif


    // initialize classifers using bootstrap samples
    if (param.bootstrap) {
      if (GetRank() == 0) {
	std::cout << "Initializing parameters using bootstrap samples ... " << std::endl;
      }  
      const size_t sample_size = internal::_get_sample_size(data, FuncType::NUMBER_OF_CLASSES);
      real_t *bootstrap_weight = new real_t[sample_size];
      real_t *sample_weight = new real_t[sample_size];
      internal::_get_sample_weight(data, badmm_cache_arr.Pi2, learner.len, sample_weight, sample_size);
      for (size_t j=0, old_j=0; j<learner.len; ++j) {
	if (j % rabit::GetWorldSize() == rabit::GetRank()) {
	  std::random_device rd;
	  std::uniform_real_distribution<real_t>  unif(0., 1.);
	  std::mt19937 rnd_gen(rd());
	  for (size_t i=0; i<sample_size; ++i) {
	    bootstrap_weight[i] = unif(rnd_gen) * sample_weight[i];
	  }
#ifdef _USE_SPARSE_ACCELERATE_	  
	  learner.supp[j].fit(X, y, bootstrap_weight, sample_size, sparse);
#else
	  learner.supp[j].fit(X, y, bootstrap_weight, sample_size);	  
#endif
	}
	if ((j+1) % rabit::GetWorldSize() == 0 || j+1 == learner.len) {
	  for (size_t jj=old_j; jj <= j; ++jj) {
	    learner.supp[jj].sync(jj % rabit::GetWorldSize() );
	  }
	  old_j = j+1;
	}
      }
      delete [] sample_weight;
      delete [] bootstrap_weight;
    }
    
    if (GetRank() == 0) {
      std::cout << "\t"
		<< "iter    " << "\t"
		<< "loss    " << "\t"
		<< "rho     " << "\t" 
		<< "prim_res" << "\t"
		<< "dual_res" << "\t"
		<< "tr_acc  " << "\t"
		<< "va_acc  " << std::endl;
    }    

    real_t loss;
    real_t train_accuracy_1, train_accuracy_2, validate_accuracy_1, validate_accuracy_2;
    for (size_t iter=0; iter < param.max_iter; ++iter) {
      /* ************************************************
       * compute cost matrix
       */
      _pdist2(learner.supp, learner.len,
	      data.get_support_ptr(), data.get_col(),
	      data.meta, badmm_cache_arr.C);

      _pdist2_label(learner.supp, learner.len,
		    data.get_support_ptr(), (real_t) 0, data.get_col(),
		    data.meta, badmm_cache_arr.Ctmp);

      for (size_t i=0; i<data.get_col() * learner.len; ++i)
	badmm_cache_arr.C[i] -= beta * badmm_cache_arr.Ctmp[i];      

      
      /* ************************************************
       * rescale badmm parameters
       */
      if (iter == 0 || true) {
	old_totalC = totalC * rho;
	if (prim_res < 0.5 *dual_res) { rho /=2;}
	if (dual_res < 0.5 *prim_res) { rho *=2;}
	totalC = _D2_CBLAS_FUNC(asum)(data.get_col() * learner.len, badmm_cache_arr.C, 1);
	Allreduce<op::Sum>(&totalC, 1);
	totalC /= global_col * learner.len;
      }
      if (param.restart > 0 && iter % param.restart == 0) {
	// restart badmm
	old_totalC = 0;
      }
      
      _D2_CBLAS_FUNC(scal)(data.get_col() * learner.len, 1./ (rho*totalC), badmm_cache_arr.C, 1);
      _D2_CBLAS_FUNC(scal)(data.get_col() * learner.len, old_totalC / (totalC * rho), badmm_cache_arr.Lambda, 1);

      /* ************************************************
       * compute current loss
       */
      loss = _D2_CBLAS_FUNC(dot)(data.get_col() * learner.len,
				 badmm_cache_arr.C, 1,
				 badmm_cache_arr.Pi2, 1);
      Allreduce<op::Sum>(&loss, 1);
      loss = loss / global_size * totalC * rho;


      /* ************************************************
       * print status informations 
       */
      if (iter > 0) {
	if (GetRank() == 0) {
	  printf("\t%zd", iter);
	  printf("\t\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t", loss, totalC * rho, prim_res, dual_res);
	  printf("%.3lf/", train_accuracy_1);
	  printf("%.3lf\t", train_accuracy_2);
	}
	if (val_data.size() > 0) {
	  for (size_t i=0; i<val_data.size(); ++i) {
	    validate_accuracy_1 = ML_Predict_ByWinnerTakeAll(*val_data[i], learner);
	    validate_accuracy_2 = ML_Predict_ByVoting(*val_data[i], learner);
	  }	
	}	
	if (GetRank() == 0) {
	  printf("%.3lf/", validate_accuracy_1);
	  printf("%.3lf", validate_accuracy_2);
	  std::cout << std::endl;
	}
      }

      /* ************************************************
       * start badmm iterations
       */
      old_prim_res = prim_res;
      old_dual_res = dual_res;
      //      while (prim_res >= old_prim_res || dual_res >= old_dual_res) {
      prim_res = 0;
      dual_res = 0;
      internal::BADMMCache badmm_cache_ptr = badmm_cache_arr;
      for (size_t i=0; i<data.get_size();++i) {
	const size_t matsize = data[i].len * learner.len;
	real_t p_res, d_res;
	EMD_BADMM(learner, data[i], badmm_cache_ptr, param.badmm_iter, &p_res, &d_res);

	badmm_cache_ptr.C += matsize;
	badmm_cache_ptr.Ctmp += matsize;
	badmm_cache_ptr.Pi1 += matsize;
	badmm_cache_ptr.Pi2 += matsize;
	badmm_cache_ptr.Lambda += matsize;
	badmm_cache_ptr.Ltmp += matsize;
	badmm_cache_ptr.buffer += matsize;
	badmm_cache_ptr.Pi_buffer += matsize;

	prim_res += p_res;
	dual_res += d_res;
      }

      Allreduce<op::Sum>(&prim_res, 1);
      Allreduce<op::Sum>(&dual_res, 1);

      prim_res /= global_size;
      dual_res /= global_size;
      //      }

      /* ************************************************
       * re-fit classifiers
       */
      const size_t sample_size = internal::_get_sample_size(data, FuncType::NUMBER_OF_CLASSES);
#ifdef _USE_SPARSE_ACCELERATE_      
      real_t *sample_weight_local = new real_t[sample_size];
#endif
      for (size_t i=0, old_i=0; i<learner.len; ++i)
      {
	real_t *sample_weight = new real_t[sample_size];
	internal::_get_sample_weight(data, badmm_cache_arr.Pi2 + i, learner.len, sample_weight, sample_size);
	//learner.supp[i].init();
#ifdef _USE_SPARSE_ACCELERATE_      
	if (i % GetWorldSize() == GetRank())
	{
	  std::memcpy(sample_weight_local, sample_weight, sizeof(real_t) * sample_size);
	}
#endif

#ifdef _USE_SPARSE_ACCELERATE_	
	if ((i+1) % GetWorldSize() == 0 || i+1==learner.len) 
	{
	  for (size_t ii=old_i; ii<=i; ++ii) {
	    if (ii % GetWorldSize() == GetRank())
	    {
	      int err_code = 0;
	      err_code = learner.supp[ii].fit(X, y, sample_weight_local, sample_size, sparse);
	      assert(err_code >= 0);
	    }
	  }

	  for (size_t ii=old_i; ii<=i; ++ii) {
	    learner.supp[ii].sync(ii % GetWorldSize());
	  }
	  old_i = i+1;	  
	}
#else
	{
	  int err_code = 0;
	  err_code = learner.supp[i].fit(X, y, sample_weight, sample_size);
	  assert(err_code >= 0);
	}	
#endif	

	if (GetRank() == 0)	  
	{
	  printf("\b\b\b\b\b\b\b%3zd/%3zd", (i+1), learner.len);
	  fflush(stdout);
	}	  	
	delete [] sample_weight;

      }
#ifdef _USE_SPARSE_ACCELERATE_      
      delete [] sample_weight_local;
#endif
      Barrier();
      
      
      train_accuracy_1 = ML_Predict_ByWinnerTakeAll(data, learner);
      train_accuracy_2 = ML_Predict_ByVoting(data, learner);

    }

    
#ifdef _USE_SPARSE_ACCELERATE_    
    internal::release_dense_if_need_mapped(data, &X, &y);
#else
    internal::release_dense_if_need_ec(data, &X, &y);
#endif
    
    deallocate_badmm_cache(badmm_cache_arr);
  }
  
}

#endif /* _MARRIAGE_LEARNER_H_ */
