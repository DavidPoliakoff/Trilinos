/*
// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
// @HEADER
*/

#ifndef TPETRA_DETAILS_COMPUTEOFFSETS_HPP
#define TPETRA_DETAILS_COMPUTEOFFSETS_HPP

/// \file Tpetra_Details_computeOffsets.hpp
/// \brief Declare and define the function
///   Tpetra::Details::computeOffsetsFromCounts, an implementation
///   detail of Tpetra (in particular, of FixedHashTable, CrsGraph,
///   and CrsMatrix).

#include "TpetraCore_config.h"
#include "Tpetra_Details_getEntryOnHost.hpp"
#include <limits>
#include <type_traits>

namespace Tpetra {
namespace Details {

//
// Implementation details for computeOffsetsFromCounts (see below).
// Users should skip over this anonymous namespace.
//
namespace { // (anonymous)

/// \brief Parallel scan functor for computing offsets from counts.
///
/// \warning This is NOT for users.  It is an implementation detail of
///   the computeOffsetsFromCounts function (see below), which you
///   should call instead.
///
/// \tparam OffsetsViewType Type of the Kokkos::View specialization
///   used to store the offsets; the output array of this functor.
/// \tparam CountsViewType Type of the Kokkos::View specialization
///   used to store the counts; the input array of this functor.
/// \tparam SizeType The parallel loop index type; a built-in integer
///   type.  Defaults to the type of the input View's dimension.  You
///   may use a shorter type to improve performance.
///
/// The type of each entry of the \c ptr array must be able to store
/// the sum of all the entries of \c counts.  This functor makes no
/// attempt to check for overflow in this sum.
template<class OffsetsViewType,
         class CountsViewType,
         class SizeType = typename OffsetsViewType::size_type>
class ComputeOffsetsFromCounts {
public:
  static_assert (Kokkos::Impl::is_view<OffsetsViewType>::value,
                 "OffsetsViewType (the type of ptr) must be a Kokkos::View.");
  static_assert (Kokkos::Impl::is_view<CountsViewType>::value,
                 "CountsViewType (the type of counts) must be a Kokkos::View.");
  static_assert (std::is_same<typename OffsetsViewType::value_type,
                   typename OffsetsViewType::non_const_value_type>::value,
                 "OffsetsViewType (the type of ptr) must be a nonconst Kokkos::View.");
  static_assert (static_cast<int> (OffsetsViewType::rank) == 1,
                 "OffsetsViewType (the type of ptr) must be a rank-1 Kokkos::View.");
  static_assert (static_cast<int> (CountsViewType::rank) == 1,
                 "CountsViewType (the type of counts) must be a rank-1 Kokkos::View.");
  static_assert (std::is_integral<typename OffsetsViewType::non_const_value_type>::value,
                 "The entries of ptr must be built-in integers.");
  static_assert (std::is_integral<typename CountsViewType::non_const_value_type>::value,
                 "The entries of counts must be built-in integers.");
  static_assert (std::is_integral<SizeType>::value,
                 "SizeType must be a built-in integer type.");

  using offsets_view_type = OffsetsViewType;
  using counts_view_type = typename CountsViewType::const_type;
  using size_type = SizeType;
  using value_type = typename OffsetsViewType::non_const_value_type;

  /// \brief Constructor
  ///
  /// \param offsets [out] (Preallocated) offsets; one entry longer
  ///   than \c counts
  /// \param counts [in] View of bucket counts
  ComputeOffsetsFromCounts (const offsets_view_type& offsets,
                            const counts_view_type& counts) :
    offsets_ (offsets),
    counts_ (counts),
    size_ (counts.extent (0))
  {}

  //! Set the initial value of the reduction result.
  KOKKOS_INLINE_FUNCTION void init (value_type& dst) const
  {
    dst = 0;
  }

  //! Combine intermedate reduction results across threads.
  KOKKOS_INLINE_FUNCTION void
  join (volatile value_type& dst,
        const volatile value_type& src) const
  {
    dst += src;
  }

  //! Reduction operator.
  KOKKOS_INLINE_FUNCTION void
  operator () (const size_type& i, value_type& update, const bool final) const
  {
    const auto curVal = (i < size_) ? counts_[i] : value_type ();
    if (final) {
      offsets_[i] = update;
    }
    update += (i < size_) ? curVal : value_type ();
  }

private:
  //! Offsets (output argument)
  offsets_view_type offsets_;
  //! Bucket counts (input argument).
  counts_view_type counts_;
  //! Number of entries in counts_.
  size_type size_;
};

/// \brief Parallel scan functor for computing offsets from a constant count.
///
/// \warning This is NOT for users.  It is an implementation detail of
///   the computeOffsetsFromConstantCount function (see below), which
///   you should call instead.
///
/// \tparam OffsetsViewType Type of the Kokkos::View specialization
///   used to store the offsets; the output array of this functor.
/// \tparam CountType Type of the count; must be a built-in integer
///   type.
/// \tparam SizeType The parallel loop index type; a built-in integer
///   type.  Defaults to the type of the input View's dimension.  You
///   may use a shorter type to improve performance.
///
/// The type of each entry of the \c ptr array must be able to store
/// <tt>ptr.extent (0) * count</tt>.  This functor makes no
/// attempt to check for overflow in this sum.
template<class OffsetsViewType,
         class CountType,
         class SizeType = typename OffsetsViewType::size_type>
class ComputeOffsetsFromConstantCount {
public:
  static_assert (Kokkos::Impl::is_view<OffsetsViewType>::value,
                 "OffsetsViewType (the type of ptr) must be a Kokkos::View.");
  static_assert (std::is_same<typename OffsetsViewType::value_type,
                   typename OffsetsViewType::non_const_value_type>::value,
                 "OffsetsViewType (the type of ptr) must be a nonconst Kokkos::View.");
  static_assert (static_cast<int> (OffsetsViewType::rank) == 1,
                 "OffsetsViewType (the type of ptr) must be a rank-1 Kokkos::View.");
  static_assert (std::is_integral<typename OffsetsViewType::non_const_value_type>::value,
                 "The entries of ptr must be built-in integers.");
  static_assert (std::is_integral<CountType>::value,
                 "CountType must be a built-in integer type.");
  static_assert (std::is_integral<SizeType>::value,
                 "SizeType must be a built-in integer type.");

  using offsets_view_type = OffsetsViewType;
  using count_type = CountType;
  using size_type = SizeType;
  using value_type = typename offsets_view_type::non_const_value_type;

  /// \brief Constructor
  ///
  /// \param offsets [out] (Preallocated) offsets; one entry longer
  ///   than \c counts
  /// \param count [in] The constant count
  ComputeOffsetsFromConstantCount (const offsets_view_type& offsets,
                                   const count_type count) :
    offsets_ (offsets),
    count_ (count),
    size_ (offsets_.extent (0) == 0 ?
           size_type (0) :
           size_type (offsets_.extent (0) - 1))
  {}

  //! Set the initial value of the reduction result.
  KOKKOS_INLINE_FUNCTION void init (value_type& dst) const
  {
    dst = 0;
  }

  //! Combine intermedate reduction results across threads.
  KOKKOS_INLINE_FUNCTION void
  join (volatile value_type& dst,
        const volatile value_type& src) const
  {
    dst += src;
  }

  //! Reduction operator.
  KOKKOS_INLINE_FUNCTION void
  operator () (const size_type i, value_type& update, const bool final) const
  {
    if (final) {
      offsets_[i] = update;
    }
    if (i < size_) {
      update += count_;
    }
  }

private:
  //! Offsets (output argument)
  offsets_view_type offsets_;
  //! "Count" input argument
  count_type count_;
  //! Number of entries in offsets_, minus 1.
  size_type size_;
};

} // namespace (anonymous)

/// \brief Compute offsets from counts
///
/// Compute offsets from counts via prefix sum:
///
/// ptr[i+1] = \sum_{j=0}^{i} counts[j]
///
/// Thus, ptr[i+1] - ptr[i] = counts[i], so that ptr[i+1] = ptr[i] +
/// counts[i].  If we stored counts[i] in ptr[i+1] on input, then the
/// formula is ptr[i+1] += ptr[i].
///
/// \return Sum of all counts; last entry of \c ptr.
///
/// \tparam OffsetsViewType Type of the Kokkos::View specialization
///   used to store the offsets; the output array of this function.
/// \tparam CountsViewType Type of the Kokkos::View specialization
///   used to store the counts; the input array of this function.
/// \tparam SizeType The parallel loop index type; a built-in integer
///   type.  Defaults to the type of the input View's dimension.  You
///   may use a shorter type to improve performance.
///
/// The type of each entry of the \c ptr array must be able to store
/// the sum of all the entries of \c counts.  This functor makes no
/// attempt to check for overflow in this sum.
template<class OffsetsViewType,
         class CountsViewType,
         class SizeType = typename OffsetsViewType::size_type>
typename OffsetsViewType::non_const_value_type
computeOffsetsFromCounts (const OffsetsViewType& ptr,
                          const CountsViewType& counts)
{
  static_assert (Kokkos::Impl::is_view<OffsetsViewType>::value,
                 "OffsetsViewType (the type of ptr) must be a Kokkos::View.");
  static_assert (Kokkos::Impl::is_view<CountsViewType>::value,
                 "CountsViewType (the type of counts) must be a Kokkos::View.");
  static_assert (std::is_same<typename OffsetsViewType::value_type,
                   typename OffsetsViewType::non_const_value_type>::value,
                 "OffsetsViewType (the type of ptr) must be a nonconst Kokkos::View.");
  static_assert (static_cast<int> (OffsetsViewType::rank) == 1,
                 "OffsetsViewType (the type of ptr) must be a rank-1 Kokkos::View.");
  static_assert (static_cast<int> (CountsViewType::rank) == 1,
                 "CountsViewType (the type of counts) must be a rank-1 Kokkos::View.");
  static_assert (std::is_integral<typename OffsetsViewType::non_const_value_type>::value,
                 "The entries of ptr must be built-in integers.");
  static_assert (std::is_integral<typename CountsViewType::non_const_value_type>::value,
                 "The entries of counts must be built-in integers.");
  static_assert (std::is_integral<SizeType>::value,
                 "SizeType must be a built-in integer type.");

  using CVT = typename CountsViewType::const_type;
  using OVT = OffsetsViewType;
  using count_type = typename CVT::non_const_value_type;
  using offset_type = typename OVT::non_const_value_type;
  using device_type = typename OVT::device_type;
  const char funcName[] = "Tpetra::Details::computeOffsetsFromCounts";

  const auto numOffsets = ptr.size ();
  const auto numCounts = counts.size ();
  offset_type total (0);

  if (numOffsets != 0) {
    TEUCHOS_TEST_FOR_EXCEPTION
      (numCounts >= numOffsets, std::invalid_argument, funcName <<
       ": counts.size() = " << numCounts << " >= ptr.size() = " <<
       numOffsets << ".");

    using execution_space = typename device_type::execution_space;
    using range_type = Kokkos::RangePolicy<execution_space, SizeType>;
    range_type range (0, numCounts+1);
    try {
      // We always want to run in the offsets' execution space, since
      // that is the output argument.  (This gives us first touch, if
      // applicable, and in general improves locality.)  However, we
      // need to make sure that we can access counts from this
      // execution space.  If we can't, we need to make a temporary
      // "device" copy of counts in offsets' memory space.

// Workaround to issue #5179, discovered by jhu.
// Workaround allows application progress while root cause of issue is 
// investigated.  KDD 8/17/19
#define Tpetra_Workaround5179
#ifndef Tpetra_Workaround5179

      using memory_space = typename device_type::memory_space;
      // The first template parameter needs to be a memory space.
      
      constexpr bool countsAccessibleFromOffsetsExecSpace =
        Kokkos::Impl::VerifyExecutionCanAccessMemorySpace<memory_space,
        typename CVT::memory_space>::value;
      if (countsAccessibleFromOffsetsExecSpace) {
#ifdef KOKKOS_ENABLE_CUDA
        // If 'counts' is a UVM allocation, then conservatively fence
        // first, in case it was last accessed on host.  We're about
        // to access it on device.
        using CMS = typename CVT::memory_space;
        if (std::is_same<CMS, Kokkos::CudaUVMSpace>::value) {
          using counts_exec_space = typename CMS::execution_space;
          counts_exec_space ().fence (); // for UVM's sake.
        }
#endif // KOKKOS_ENABLE_CUDA
        using functor_type = ComputeOffsetsFromCounts<OVT, CVT, SizeType>;
        functor_type functor (ptr, counts);
        Kokkos::parallel_scan (range, functor, total, funcName);
      }
      else { // make tmp copy of counts accessible from offsets' space
#endif // Tpetra_Workaround5179
        using dev_counts_type = Kokkos::View<count_type*,
          typename CVT::array_layout, device_type>;
        dev_counts_type counts_d ("counts_d", numCounts);
        Kokkos::deep_copy (counts_d, counts);

        using functor_type =
          ComputeOffsetsFromCounts<OVT, dev_counts_type, SizeType>;
        functor_type functor (ptr, counts_d);
        Kokkos::parallel_scan (range, functor, total, funcName);
#ifndef Tpetra_Workaround5179
      }
#endif // Tpetra_Workaround5179
    }
    catch (std::exception& e) {
      TEUCHOS_TEST_FOR_EXCEPTION
        (true, std::runtime_error, funcName << ": Kokkos::parallel_scan "
         "with device_type " << typeid (device_type).name () <<
         " threw an exception: " << e.what ());
    }
    catch (...) {
      TEUCHOS_TEST_FOR_EXCEPTION
        (true, std::runtime_error, funcName << ": Kokkos::parallel_scan "
         "threw an exception not a subclass of std::exception");
    }
  }

  return total;
}

/// \brief Compute offsets from a constant count
///
/// Compute offsets from a constant count via prefix sum:
///
/// ptr[i+1] = \sum_{j=0}^{i} count
///
/// Thus, ptr[i+1] - ptr[i] = count, so that ptr[i+1] = ptr[i] +
/// count.
///
/// \return Sum of all counts; last entry of \c ptr.
///
/// \tparam OffsetsViewType Type of the Kokkos::View specialization
///   used to store the offsets; the output array of this function.
/// \tparam CountType Type of the constant count; the input argument
///   of this function.
/// \tparam SizeType The parallel loop index type; a built-in integer
///   type.  Defaults to the type of the output View's dimension.  You
///   may use a shorter type to improve performance.
///
/// The type of each entry of the \c ptr array must be able to store
/// <tt>ptr.extent (0) * count</tt>.  This functor makes no
/// attempt to check for overflow in this sum.
template<class OffsetsViewType,
         class CountType,
         class SizeType = typename OffsetsViewType::size_type>
typename OffsetsViewType::non_const_value_type
computeOffsetsFromConstantCount (const OffsetsViewType& ptr,
                                 const CountType count)
{
  static_assert (Kokkos::Impl::is_view<OffsetsViewType>::value,
                 "OffsetsViewType (the type of ptr) must be a Kokkos::View.");
  static_assert (std::is_same<typename OffsetsViewType::value_type,
                   typename OffsetsViewType::non_const_value_type>::value,
                 "OffsetsViewType (the type of ptr) must be a nonconst Kokkos::View.");
  static_assert (static_cast<int> (OffsetsViewType::rank) == 1,
                 "OffsetsViewType (the type of ptr) must be a rank-1 Kokkos::View.");
  static_assert (std::is_integral<typename OffsetsViewType::non_const_value_type>::value,
                 "The entries of ptr must be built-in integers.");
  static_assert (std::is_integral<CountType>::value,
                 "CountType must be a built-in integer type.");
  static_assert (std::is_integral<SizeType>::value,
                 "SizeType must be a built-in integer type.");

  using offset_type = typename OffsetsViewType::non_const_value_type;
  using device_type = typename OffsetsViewType::device_type;
  const char funcName[] = "Tpetra::Details::computeOffsetsFromConstantCount";

  const auto numOffsets = ptr.size ();
  offset_type total (0);

  if (numOffsets != 0) {
    using CT = typename std::decay<CountType>::type;
    using OVT = OffsetsViewType;
    using functor_type =
      ComputeOffsetsFromConstantCount<OVT, CT, SizeType>;
    functor_type functor (ptr, count);

    using execution_space = typename device_type::execution_space;
    using range_type = Kokkos::RangePolicy<execution_space, SizeType>;
    range_type range (0, numOffsets);
    try {
      Kokkos::parallel_scan (range, functor, total, funcName);
    }
    catch (std::exception& e) {
      TEUCHOS_TEST_FOR_EXCEPTION
        (true, std::runtime_error, funcName << ": Kokkos::parallel_scan "
         "(with device_type " << typeid (device_type).name () <<
         ">) threw an exception: " << e.what ());
    }
    catch (...) {
      TEUCHOS_TEST_FOR_EXCEPTION
        (true, std::runtime_error, funcName << ": Kokkos::parallel_scan "
         "(with device_type " << typeid (device_type).name () <<
         ">) threw an exception not a subclass of std::exception");
    }
  }
  return total;
}

} // namespace Details
} // namespace Tpetra

#endif // TPETRA_DETAILS_COMPUTEOFFSETS_HPP
