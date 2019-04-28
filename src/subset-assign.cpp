#include <rray.h>
#include <tools/tools.h>
#include <dispatch.h>
#include <subset-tools.h>

// Required for the assignment step
#include <xtensor/xarray.hpp>

// xtensor can't handle assigning to a requested dimension of 0
// i.e. this crashes) x[0, 1] <- 99
// this should always be a no-op so we just check for it and return x
bool any_zero_length(Rcpp::List x) {
  int n = x.size();

  for (int i = 0; i < n; ++i) {
    if (Rf_length(x[i]) == 0) {
      return true;
    }
  }

  return false;
}

// Ensure that we can actually perform the assignment so we don't segfault
template <class E1, class E2>
void validate_broadcastable_shapes(E1 x, E2 to) {
  auto x_shape = x.shape();
  Rcpp::IntegerVector x_dim(x_shape.begin(), x_shape.end());

  auto to_shape = to.shape();
  Rcpp::IntegerVector to_dim(to_shape.begin(), to_shape.end());

  rray__validate_broadcastable(x_dim, to_dim);
}

template <typename T>
xt::rarray<T> rray__subset_assign_impl(const xt::rarray<T>& x,
                                       Rcpp::List indexer,
                                       Rcpp::RObject value_) {

  // Catch this early on
  if (any_zero_length(indexer)) {
    return x;
  }

  xt::rarray<T> value(value_);

  // Reshape `xt_value` to have the dimensionality of `x`
  const int& x_dims = rray__dims(SEXP(x));
  auto value_view = rray__increase_dims_view(value, x_dims);

  // Request a copy of `x` that we can assign to
  // `x` comes in as a `const&` that we can't modify directly
  // (and we don't want to change this. We will have to copy `x` at
  // some point for the assignment. It makes sense to do it here.)
  xt::rarray<T> out = x;

  if (is_stridable(indexer)) {
    xt::xstrided_slice_vector sv = build_strided_slice_vector(indexer);
    auto x_subset_view = xt::strided_view(out, sv);
    validate_broadcastable_shapes(value_view, x_subset_view);
    x_subset_view = value_view;
  }
  else {
    xt::xdynamic_slice_vector sv = build_dynamic_slice_vector(indexer);
    auto x_subset_view = xt::dynamic_view(out, sv);
    validate_broadcastable_shapes(value_view, x_subset_view);
    x_subset_view = value_view;
  }

  return out;
}

// [[Rcpp::export]]
Rcpp::RObject rray__subset_assign(Rcpp::RObject x,
                                  Rcpp::List indexer,
                                  Rcpp::RObject value) {
  DISPATCH_UNARY_TWO(rray__subset_assign_impl, x, indexer, value);
}
