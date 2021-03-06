//*LB*
// Copyright (c) 2010, University of Bonn, Institute for Computer Science VI
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//  * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//  * Neither the name of the University of Bonn 
//    nor the names of its contributors may be used to endorse or promote
//    products derived from this software without specific prior written
//    permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//*LE*





#ifndef __DENSEDENSE_TO_SPARSE_HPP__
#define __DENSEDENSE_TO_SPARSE_HPP__


#include <cuv/basics/dia_matrix.hpp>

#define SPARSE_DIA_BLOCK_SIZE 16
#define SPARSE_DIA_BLOCK_SIZE_LEN (2*SPARSE_DIA_BLOCK_SIZE+2)

namespace cuv{

	/**
	 * Dummy Block descriptor on host.
	 * this class is needed ON DEVICE for 
	 *   DIA_Mat := Dense_Mat * Dense_Mat
	 * the dummy host descriptor does nothing and exists such that you can use
	 * the same interface for both device and host matrices.
	 */
	template<class __value_type, class __index_type=unsigned int>
	class host_block_descriptor{
		public:
			typedef __value_type value_type;  ///< type of matrix elements
			typedef __index_type index_type;  ///< type of matrix indices
			typedef dia_matrix<value_type,host_memory_space,index_type> diamat_type; ///< matrix-type associated with blockdescriptor
			/** constructor: does nothing
			 *  @param d unused
			 */
			host_block_descriptor(const diamat_type& d){}
	};

	/**
	 * Block descriptors on device
	 * this class is needed for DIA_Mat = Dense_Mat * Dense_Mat
	 * it stores all blocks of size SPARSE_DIA_BLOCK_SIZE x SPARSE_DIA_BLOCK_SIZE
	 * of a regluar grid where at least one diagonal crosses the block.
	 *
	 * Creating Block-Descriptors can take some time, but this pays off when calculating densedense_to_dia (see below)
	 */
	template<class __value_type, class __index_type=unsigned int>
	class dev_block_descriptor{
		public:
			typedef __value_type value_type; ///< type of matrix elements
			typedef __index_type index_type; ///< type of matrix indices
			typedef dia_matrix<value_type,dev_memory_space,index_type> diamat_type; ///< matrix-type associated with blockdescriptor
		protected:
			/**
			 * One block consists of the index of its upper-left corner and the offsets of all diagonals crossing this block, a Block has Size SPARSE_DIA_BLOCK_SIZE*SPARSE_DIA_BLOCK_SIZE.
			 */
			struct block{
				int              startx;  ///< upper left corner of block
				int				 starty;  ///< upper left corner of block
				int              diag[2*SPARSE_DIA_BLOCK_SIZE];  ///< the offsets of all diagonals crossing the block
			};
			struct block_array{  ///< memory for storing multiple blocks
				int*    ptr;     ///< data (on device)
				int     len;     ///< number of blocks stored in ptr
			} m_blocks;          ///< structure holding the actual data stored in the descriptor
		public:
			/** Create a block descriptor for a DIA matrix.
			 *
			 * @param  d a dia-matrix for which to create the block-descriptor.
			 */
			dev_block_descriptor(const diamat_type& d);
			/// destroy the block descriptor
			~dev_block_descriptor();

			/// @return the internal block structure
			const block_array& blocks()const{return m_blocks;}

			///  @return the number of blocks
			inline int len()const{ return m_blocks.len; }
	};

	/**
	 * DIA_Mat <- Dense_Mat * Dense_Mat_transposed.
	 *
	 * This is one special case for matrix multiplication, where the second
	 * matrix is transposed and only the elements on the diagonals of a DIA matrix
	 * must be computed. The function is needed for the backwards-pass of a
	 * neural network.
	 *
	 * @param C    target matrix
	 * @param Cbd  block descriptor (is not changed, so you can re-use the bd for all matrices with same layout)
	 * @param A    A as in C=A*B'
	 * @param B    B as in C=A*B'
	 * @param factAB the result of A*B is multiplied with this factAB and then added to factC*C
	 * @param factC the result of A*B is multiplied with this factAB and then added to factC*C
	 */
        // dev_block_descriptor and host_block_descriptor explicit... UGLY...
	template<class __value_type, class __memory_layout_type, class __index_type >
	void densedense_to_dia(
		   dia_matrix<__value_type,dev_memory_space,__index_type>&           C,
		   const dev_block_descriptor<__value_type, __index_type>&      Cbd,
		   const tensor<__value_type, dev_memory_space, __memory_layout_type>&   A,
		   const tensor<__value_type, dev_memory_space, __memory_layout_type>&   B,
		   const __value_type& factAB=1.f,
		   const __value_type& factC =0.f);

	template<class __value_type, class __memory_layout_type, class __index_type >
	void densedense_to_dia(
		   dia_matrix<__value_type,host_memory_space,__index_type>&           C,
		   const host_block_descriptor<__value_type, __index_type>&      Cbd,
		   const tensor<__value_type, host_memory_space, __memory_layout_type>&   A,
		   const tensor<__value_type, host_memory_space, __memory_layout_type>&   B,
		   const __value_type& factAB=1.f,
		   const __value_type& factC =0.f);

	
} // cuv


#endif
