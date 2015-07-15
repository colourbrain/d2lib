#ifndef _D2_H_
#define _D2_H_

/*!
 * Main header file for apps
 */

#include "common.hpp"

namespace d2 {

  namespace def {    
    /*!
     * The type of discrete distribution decides how the
     * ground distance is computed. For example, d2::def::Euclidean
     * means the ground distance is computed from the Euclidean 
     * distance between two arbitray vectors. Also, the type
     * of discrete distribution also decides how the data is 
     * stored. For example, d2::def::WordVec means the raw 
     * data is stored with index, aka, the word and the meta part 
     * stores the actual vector w.r.t. individual indexed word. 
     * Thus, different types corresponds to different meta data.
     * In the case of d2::def::Euclidean, the meta data is empty.
     */
    struct Euclidean;

    struct WordVec;
    
    struct NGram;
    
    struct Histogram;

    struct SparseHistogram;
  }

  /**********************************************************************/
  /* Core data structures in template: 
   *     Elem, ElemMultiPhase, Meta, Block, BlockMultiPhase
   *
   * and distributed data structure: 
   *     DistributedBlockMultiPhase
   */

  /*!
   * Elem defines an element type has two parameters to specify, one is the
   * type of ground metric space, and the other is the dimension
   * of the ground metric space. 
   */
  template <typename D2Type, size_t dim>
  struct Elem;

  /*!
   * ElemMultiPhase defines a multi-phase element type
   * param @Ts: struct Elem<>
   */
  template <typename... Ts>
  struct ElemMultiPhase;

  /*!
   * A Meta class is for storing the meta data associated with
   * an Elem type. It is an empty class for def::Euclidean. 
   */
  template <typename ElemType>
  class Meta;  

  /*!
   * A Block class is for storing a block data of Elem in a 
   * local node. 
   */
  template <typename ElemType>
  class Block;

  /*!
   * BlockMultiPhase defines a block of elements with multiple phases
   * param @Ts: struct Elem<>
   */
  template <typename... Ts> // a sequence of Elem types
  class BlockMultiPhase;

#ifdef RABIT_RABIT_H_
  template <typename... Ts>
  class DistributedBlockMultiPhase;
#endif 

  /**********************************************************************/
  /* Core functions over previous data structures:
   *     pdist2, EMD, LowerThanEMD, KNearestNeighbors_Linear
   *
   */

  template <typename D2Type1, typename D2Type2, size_t dim>
  inline void pdist2 (const typename D2Type1::type *s1, const size_t n1,
		      const typename D2Type2::type *s2, const size_t n2,
		      const Meta<Elem<D2Type2, dim> > &meta,
		      __OUT__ real_t* mat);


  /*!
   * compute EMD between two discrete distributions
   */
  template <typename ElemType1, typename ElemType2, typename MetaType2>
  inline real_t EMD (const ElemType1 &e1, const ElemType2 &e2, const MetaType2 &meta,
		     __IN__ real_t* cache_mat = NULL,
		     __OUT__ real_t* cache_primal = NULL, 
		     __OUT__ real_t* cache_dual = NULL);

  /*!
   * compute EMD between a discrete distribution and a block of discrete distributions
   */
  template <typename ElemType1, typename ElemType2>
  void EMD (const ElemType1 &e, const Block<ElemType2> &b,
	    __OUT__ real_t* emds,
	    __IN__ real_t* cache_mat = NULL,
	    __OUT__ real_t* cache_primal = NULL, 
	    __OUT__ real_t* cache_dual = NULL);

  template <template<typename...> class D1, template<typename... > class D2, 
	    typename... Ts1, typename... Ts2>
  void EMD(const D1<Ts1...> &e, const D2<Ts2...> &b,
	   __OUT__ real_t* emds);
  

  /*!
   * compute lower bound of EMD 
   * version 0: extremely cheap, non-iterative
   */
  template <typename ElemType1, typename ElemType2, typename MetaType2>
  inline real_t LowerThanEMD_v0(const ElemType1 &e1, const ElemType2 &e2, const MetaType2 &meta);

  template <typename ElemType1, typename ElemType2>
  void LowerThanEMD_v0(const ElemType1 &e, const Block<ElemType2> &b,
		       __OUT__ real_t* emds);

  /*!
   * compute lower bound of EMD 
   * version 1: fast, non-iterative and simple
   */
  template <typename ElemType1, typename ElemType2, typename MetaType2>
  inline real_t LowerThanEMD_v1(const ElemType1 &e1, const ElemType2 &e2, const MetaType2 &meta,
				__IN__ real_t* cache_mat = NULL);

  template <typename ElemType1, typename ElemType2>
  void LowerThanEMD_v1(const ElemType1 &e, const Block<ElemType2> &b,
		       __OUT__ real_t* emds,
		       __IN__ real_t* cache_mat);


  /*!
   * simple linear approach without any prefetching or pruning
   */
  template <typename ElemType1, typename ElemType2>
  void KNearestNeighbors_Linear(size_t k,
				const ElemType1 &e, const Block<ElemType2> &b,
				__OUT__ real_t* emds_approx,
				__OUT__ index_t* rank);

  template <template<typename...> class D1, template<typename...> class D2,
	    typename... Ts1, typename... Ts2>
  void KNearestNeighbors_Linear(size_t k,
				const D1<Ts1...> &e,
				const D2<Ts2...> &b,
				__OUT__ real_t* emds_approx,
				__OUT__ index_t* rank);


  /*!
   * prefetching and pruning with lowerbounds
   */
  template <typename ElemType1, typename ElemType2>
  void KNearestNeighbors_Simple(size_t k,
				const ElemType1 &e, const Block<ElemType2> &b,
				__OUT__ real_t* emds_approx,
				__OUT__ index_t* rank,
				size_t n = 0);

  template <template<typename...> class D1, template<typename...> class D2,
	    typename... Ts1, typename... Ts2>
  void KNearestNeighbors_Simple(size_t k,
				const D1<Ts1...> &e, 
				const D2<Ts2...> &b,
				__OUT__ real_t* emds_approx,
				__OUT__ index_t* rank,
				size_t n);


  inline void Init(int argc, char*argv[]);
  inline void Finalize();

}

#include "d2_data.hpp"
#include "d2_io_impl.hpp"

#ifdef RABIT_RABIT_H_
#include "d2_io_impl_rabit.hpp"
#endif

#include "d2_server.hpp"

#endif /* _D2_H_ */