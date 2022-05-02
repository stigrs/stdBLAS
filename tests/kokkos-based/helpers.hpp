/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 2.0
//              Copyright (2019) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software. //
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
// Questions? Contact Christian R. Trott (crtrott@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef LINALG_TESTS_KOKKOS_HELPERS_HPP_
#define LINALG_TESTS_KOKKOS_HELPERS_HPP_

#include <experimental/linalg>
#include <experimental/mdspan>
#include <random>

namespace kokkostesting{

template<class T>
auto create_stdvector_and_copy(T sourceView)
{
  static_assert (sourceView.rank() == 1);

  using value_type = typename T::value_type;
  using res_t = std::vector<value_type>;

  res_t result(sourceView.extent(0));
  for (std::size_t i=0; i<sourceView.extent(0); ++i){
    result[i] = sourceView(i);
  }

  return result;
}

template<class T>
auto create_stdvector_and_copy_rowwise(T sourceView)
{
  static_assert (sourceView.rank() == 2);

  using value_type = typename T::value_type;
  using res_t = std::vector<value_type>;

  res_t result(sourceView.extent(0)*sourceView.extent(1));
  std::size_t k=0;
  for (std::size_t i=0; i<sourceView.extent(0); ++i){
    for (std::size_t j=0; j<sourceView.extent(1); ++j){
      result[k++] = sourceView(i,j);
    }
  }

  return result;
}

// create rank-1 mdspan (vector)
template <typename ValueType,
          typename mdspan_t = typename _blas2_signed_fixture<ValueType>::mdspan_r1_t>
mdspan_t make_mdspan(ValueType *data, std::size_t ext) {
  return mdspan_t(data, ext);
}

template <typename ValueType>
auto make_mdspan(std::vector<ValueType> &v) {
  return make_mdspan(v.data(), v.size());
}

template <typename ValueType>
auto make_mdspan(const std::vector<ValueType> &v) {
  return make_mdspan(v.data(), v.size());
}

// create rank-2 mdspan (matrix)
template <typename ValueType,
          typename mdspan_t = typename _blas2_signed_fixture<ValueType>::mdspan_r2_t>
mdspan_t make_mdspan(ValueType *data, std::size_t ext0, std::size_t ext1) {
  return mdspan_t(data, ext0, ext1);
}

template <typename RealValue>
KOKKOS_INLINE_FUNCTION
RealValue scalar_abs_diff(RealValue v1, RealValue v2) {
  return std::abs(v2 - v1);
}

template <typename T>
T scalar_abs_diff(const std::complex<T> &v1, const std::complex<T> &v2) {
  const auto dr = scalar_abs_diff(v1.real(), v2.real());
  const auto di = scalar_abs_diff(v1.imag(), v2.imag());
  return std::max(dr, di);
}

template <typename T>
KOKKOS_INLINE_FUNCTION
T scalar_abs_diff(const Kokkos::complex<T> &v1, const Kokkos::complex<T> &v2) {
  const auto dr = scalar_abs_diff(v1.real(), v2.real());
  const auto di = scalar_abs_diff(v1.imag(), v2.imag());
  return dr > di ? dr : di; // can't use std::max on GPU
}

// no-tolerance (exact) comparison
template <typename ElementType1,
          std::size_t Extent1,
          typename LayoutPolicy1,
          typename AccessorPolicy1,
          typename ElementType2,
          std::size_t Extent2,
          typename LayoutPolicy2,
          typename AccessorPolicy2>
bool is_same_vector(
    mdspan<ElementType1, extents<Extent1>, LayoutPolicy1, AccessorPolicy1> v1,
    mdspan<ElementType2, extents<Extent2>, LayoutPolicy2, AccessorPolicy2> v2)
{
  const auto size = v1.extent(0);
  if (size != v2.extent(0))
    return false;
  const auto v1_view = KokkosKernelsSTD::Impl::mdspan_to_view(v1);
  const auto v2_view = KokkosKernelsSTD::Impl::mdspan_to_view(v2);
  // Note: reducing to `int` because Kokkos can complain on `bool` not being
  //       aligned with int32 and deny it for parallel_reduce()
  using diff_type = int;
  diff_type is_different = false;
  Kokkos::parallel_reduce(size,
    KOKKOS_LAMBDA(const std::size_t i, diff_type &diff){
        diff = v1_view[i] != v2_view[i];
	    }, Kokkos::LOr<diff_type>(is_different));
  return !is_different;
}

template <typename ElementType1,
          std::size_t Extent,
          typename LayoutPolicy,
          typename AccessorPolicy,
          typename ElementType2>
bool is_same_vector(
    mdspan<ElementType1, extents<Extent>, LayoutPolicy, AccessorPolicy> v1,
    const std::vector<ElementType2> &v2)
{
  return is_same_vector(v1, make_mdspan(v2));
}

template <typename ElementType1,
          std::size_t Extent,
          typename LayoutPolicy,
          typename AccessorPolicy,
          typename ElementType2>
bool is_same_vector(
    const std::vector<ElementType1> &v1,
    mdspan<ElementType2, extents<Extent>, LayoutPolicy, AccessorPolicy> v2)
{
  return is_same_vector(v2, v1);
}

template <typename ElementType>
bool is_same_vector(
    const std::vector<ElementType> &v1,
    const std::vector<ElementType> &v2)
{
  return is_same_vector(make_mdspan(v1), make_mdspan(v2));
}

template <typename ElementType1,
          std::size_t Extent1,
          typename LayoutPolicy1,
          typename AccessorPolicy1,
          typename ElementType2,
          std::size_t Extent2,
          typename LayoutPolicy2,
          typename AccessorPolicy2>
auto vector_abs_diff(
    mdspan<ElementType1, extents<Extent1>, LayoutPolicy1, AccessorPolicy1> v1,
    mdspan<ElementType2, extents<Extent2>, LayoutPolicy2, AccessorPolicy2> v2)
{
  using RetType = decltype(scalar_abs_diff(v1[0], v2[0])); // will be same for views
  const auto size = v1.extent(0);
  if (size != v2.extent(0))
    return std::numeric_limits<RetType>::max(); // very, very different
  const auto v1_view = KokkosKernelsSTD::Impl::mdspan_to_view(v1);
  const auto v2_view = KokkosKernelsSTD::Impl::mdspan_to_view(v2);
  RetType diff = static_cast<RetType>(0);
  Kokkos::parallel_reduce(size,
    KOKKOS_LAMBDA(const std::size_t i, RetType &diff){
        const RetType d = scalar_abs_diff(v1_view[i], v2_view[i]);
        if (d > diff) {
          diff = d;
        }
	    }, Kokkos::Max<RetType>(diff));
  return diff;
}

template <typename ElementType1,
          std::size_t Extent,
          typename LayoutPolicy,
          typename AccessorPolicy,
          typename ElementType2>
auto vector_abs_diff(
    mdspan<ElementType1, extents<Extent>, LayoutPolicy, AccessorPolicy> v1,
    const std::vector<ElementType2> &v2)
{
  return vector_abs_diff(v1, make_mdspan(v2));
}

template <typename ElementType1,
          std::size_t Extent,
          typename LayoutPolicy,
          typename AccessorPolicy,
          typename ElementType2>
auto vector_abs_diff(
    const std::vector<ElementType1> &v1,
    mdspan<ElementType2, extents<Extent>, LayoutPolicy, AccessorPolicy> v2)
{
  return vector_abs_diff(v2, v1);
}

template <typename ElementType1, typename ElementType2>
auto vector_abs_diff(
    const std::vector<ElementType1> &v1,
    const std::vector<ElementType2> &v2)
{
  return vector_abs_diff(make_mdspan(v1), make_mdspan(v2));
}

template <typename ElementType1,
          std::size_t Extent1,
          typename LayoutPolicy1,
          typename AccessorPolicy1,
          typename ElementType2,
          std::size_t Extent2,
          typename LayoutPolicy2,
          typename AccessorPolicy2>
auto vector_rel_diff(
    mdspan<ElementType1, extents<Extent1>, LayoutPolicy1, AccessorPolicy1> v1,
    mdspan<ElementType2, extents<Extent2>, LayoutPolicy2, AccessorPolicy2> v2)
{
  using RetType = decltype(scalar_abs_diff(v1[0], v2[0]));
  const auto size = v1.extent(0);
  if (size != v2.extent(0))
    return std::numeric_limits<RetType>::max(); // very, very different

  constexpr auto zero1 = static_cast<ElementType1>(0);
  constexpr auto zero2 = static_cast<ElementType2>(0);
  RetType abs_diff = vector_abs_diff(v1, v2);
  auto v1_norm = scalar_abs_diff(zero1, v1[std::experimental::linalg::idx_abs_max(v1)]);
  auto v2_norm = scalar_abs_diff(zero2, v2[std::experimental::linalg::idx_abs_max(v2)]);
  if (v1_norm == zero1) {
    if (v2_norm == zero2) {
      return static_cast<RetType>(0); // no difference
    } else {
      return abs_diff;
    }
  } else if (v2_norm == static_cast<ElementType2>(0)) {
    return abs_diff;
  } else {
    return abs_diff / std::min(v1_norm, v2_norm); // pick larger relative error
  }
}

template <typename ElementType1,
          std::size_t Extent1,
          typename LayoutPolicy,
          typename AccessorPolicy,
          typename ElementType2>
auto vector_rel_diff(
    mdspan<ElementType1, extents<Extent1>, LayoutPolicy, AccessorPolicy> v1,
    const std::vector<ElementType2> &v2)
{
  return vector_rel_diff(v1, make_mdspan(v2));
}

template <typename ElementType1,
          std::size_t Extent,
          typename LayoutPolicy,
          typename AccessorPolicy,
          typename ElementType2>
auto vector_rel_diff(
    const std::vector<ElementType1> &v1,
    mdspan<ElementType2, extents<Extent>, LayoutPolicy, AccessorPolicy> v2)
{
  return vector_rel_diff(v2, v1);
}

template <typename ElementType1, typename ElementType2>
auto vector_rel_diff(
    const std::vector<ElementType1> &v1,
    const std::vector<ElementType2> &v2)
{
  return vector_rel_diff(make_mdspan(v1), make_mdspan(v2));
}

// no-tolerance (exact) comparison
template <typename ElementType1,
          std::size_t Extent10,
          std::size_t Extent11,
          typename LayoutPolicy1,
          typename AccessorPolicy1,
          typename ElementType2,
          std::size_t Extent20,
          std::size_t Extent21,
          typename LayoutPolicy2,
          typename AccessorPolicy2>
bool is_same_matrix(
    mdspan<ElementType1, extents<Extent10, Extent11>, LayoutPolicy1, AccessorPolicy1> A,
    mdspan<ElementType2, extents<Extent20, Extent21>, LayoutPolicy2, AccessorPolicy2> B)
{
  const auto ext0 = A.extent(0);
  const auto ext1 = A.extent(1);
  if (B.extent(0) != ext0 or B.extent(1) != ext1)
    return false;
  const auto A_view = KokkosKernelsSTD::Impl::mdspan_to_view(A);
  const auto B_view = KokkosKernelsSTD::Impl::mdspan_to_view(B);
  // Note: reducing to `int` because Kokkos can complain on `bool` not being
  //       aligned with int32 and deny it for parallel_reduce()
  using diff_type = int;
  diff_type is_different = false;
  Kokkos::parallel_reduce(ext0,
    KOKKOS_LAMBDA(std::size_t i, diff_type &diff) {
        for (decltype(i) j = 0; j < ext1; ++j) {
          const bool d = A_view(i, j) != B_view(i, j);
          diff = diff || d;
        }
	    }, Kokkos::LOr<diff_type>(is_different));
  return !is_different;
}

template <typename ElementType,
          std::size_t Extent0,
          std::size_t Extent1,
          typename LayoutPolicy1,
          typename AccessorPolicy1>
bool is_same_matrix(
    mdspan<ElementType, extents<Extent0, Extent1>, LayoutPolicy1, AccessorPolicy1> A,
    const std::vector<ElementType> &B)
{
  return is_same_matrix(A, make_mdspan(B.data(), A.extent(0), A.extent(1)));
}

template <typename ElementType,
          std::size_t Extent0,
          std::size_t Extent1,
          typename LayoutPolicy1,
          typename AccessorPolicy1>
bool is_same_matrix(const std::vector<ElementType> &A,
    mdspan<ElementType, extents<Extent0, Extent1>, LayoutPolicy1, AccessorPolicy1> B)
{
  return is_same_matrix(make_mdspan(A.data(), B.extent(0), B.extent(1)), B);
}

template <typename ElementType1,
          std::size_t Extent10,
          std::size_t Extent11,
          typename LayoutPolicy1,
          typename AccessorPolicy1,
          typename ElementType2,
          std::size_t Extent20,
          std::size_t Extent21,
          typename LayoutPolicy2,
          typename AccessorPolicy2>
auto matrix_abs_diff(
    mdspan<ElementType1, extents<Extent10, Extent11>, LayoutPolicy1, AccessorPolicy1> A,
    mdspan<ElementType2, extents<Extent20, Extent21>, LayoutPolicy2, AccessorPolicy2> B)
{
  const auto ext0 = A.extent(0);
  const auto ext1 = A.extent(1);
  using RetType = decltype(scalar_abs_diff(A(0, 0), B(0, 0)));
  if (B.extent(0) != ext0 or B.extent(1) != ext1) {
    return std::numeric_limits<RetType>::max(); // very, very different
  }
  return vector_abs_diff(
      make_mdspan(A.data(), ext0 * ext1),
      make_mdspan(B.data(), ext0 * ext1));
}

template <typename ElementType,
          std::size_t Extent10,
          std::size_t Extent11,
          typename LayoutPolicy1,
          typename AccessorPolicy1,
          std::size_t Extent20,
          std::size_t Extent21,
          typename LayoutPolicy2,
          typename AccessorPolicy2>
auto matrix_rel_diff(
    mdspan<ElementType, extents<Extent10, Extent11>, LayoutPolicy1, AccessorPolicy1> A,
    mdspan<ElementType, extents<Extent20, Extent21>, LayoutPolicy2, AccessorPolicy2> B)
{
  const auto ext0 = A.extent(0);
  const auto ext1 = A.extent(1);
  using RetType = decltype(scalar_abs_diff(A(0, 0), B(0, 0)));
  if (B.extent(0) != ext0 or B.extent(1) != ext1) {
    return std::numeric_limits<RetType>::max(); // very, very different
  }
  return vector_rel_diff(
      make_mdspan(A.data(), ext0 * ext1),
      make_mdspan(B.data(), ext0 * ext1));
}

namespace Impl { // internal to test helpers

template <typename T, typename Enabled=void> struct _tolerance_out { using type = T; };
template <typename T> struct _tolerance_out<std::complex<T>> { using type = T; };

}

// uses T to select single or double precision value
template <typename T>
Impl::_tolerance_out<T>::type tolerance(double double_tol, float float_tol);

template <> double tolerance<double>(double double_tol, float float_tol) { return double_tol; }
template <> float  tolerance<float>( double double_tol, float float_tol) { return float_tol; }
template <> double tolerance<std::complex<double>>(double double_tol, float float_tol) { return double_tol; }
template <> float  tolerance<std::complex<float>>( double double_tol, float float_tol) { return float_tol; }

// checks if std::complex<T> and Kokkos::complex<T> are aligned
// (they can get misalligned when Kokkos is build with Kokkos_ENABLE_COMPLEX_ALIGN=ON)
template <typename ValueType, typename Enabled = void>
struct check_complex_alignment: public std::true_type {};

template <typename T>
struct check_complex_alignment<std::complex<T>> {
  static constexpr bool value = alignof(std::complex<T>) == alignof(Kokkos::complex<T>);
};

template <typename ValueType>
constexpr auto check_complex_alignment_v = check_complex_alignment<ValueType>::value;

// skips test execution (giving a warning instead) if type checks fail
template <typename ValueType, typename cb_type>
void run_checked_tests(const std::string_view test_prefix, const std::string_view method_name,
                       const std::string_view test_postfix, const std::string_view type_spec,
                       const cb_type cb) {
  if constexpr (check_complex_alignment_v<ValueType>) { // add more checks if needed
    cb();
  } else {
    std::cout << "***\n"
              << "***  Warning: " << test_prefix << method_name << test_postfix << " skipped for "
              << type_spec << " (type check failed)\n"
              << "***" << std::endl;
    /* avoid dispatcher check failure if all cases are skipped this way */
    KokkosKernelsSTD::Impl::signal_kokkos_impl_called(method_name);
  }
}

// drives A = F(A, x, ...) operation test
template<class x_t, class A_t, class AToleranceType, class GoldType, class ActionType>
void test_op_Ax(x_t x, A_t A, AToleranceType A_tol, GoldType get_gold, ActionType action)
{
  // backup x to verify it is not changed after kernel
  auto x_preKernel = create_stdvector_and_copy(x);

  // compute gold
  auto A_copy = create_stdvector_and_copy_rowwise(A);
  auto A_gold = make_mdspan(A_copy.data(), A.extent(0), A.extent(1));
  get_gold(A_gold);

  // run tested routine
  action();

  // compare results with gold
  EXPECT_LE(matrix_rel_diff(A_gold, A), A_tol);

  // x should not change after kernel
  EXPECT_TRUE(is_same_vector(x, x_preKernel));
}

// drives A = F(A, x, y, ...) operation test
template<class x_t, class y_t, class A_t, class AToleranceType, class GoldType, class ActionType>
void test_op_Axy(x_t x, y_t y, A_t A, AToleranceType A_tol, GoldType get_gold, ActionType action)
{
  auto y_preKernel = create_stdvector_and_copy(y);
  test_op_Ax(x, A, A_tol, get_gold, action);
  EXPECT_TRUE(is_same_vector(y, y_preKernel));
}

}
#endif
