// Copyright © 2016-2021 Thomas Nagler and Thibault Vatter
//
// This file is part of the vinecopulib library and licensed under the terms of
// the MIT license. For a copy, see the LICENSE file in the root directory of
// vinecopulib or https://vinecopulib.github.io/vinecopulib/.

#include <vinecopulib/misc/tools_interpolation.hpp>
#include <vinecopulib/misc/tools_stats.hpp>
#include <wdm/eigen.hpp>

namespace vinecopulib {
inline KernelBicop::KernelBicop()
{
  // construct default grid (equally spaced on Gaussian scale)
  size_t m = 30;
  auto grid_points = this->make_normal_grid(m);

  // move boundary points to 0/1, so we don't have to extrapolate
  grid_points(0) = 0.0;
  grid_points(m - 1) = 1.0;

  interp_grid_ = std::make_shared<tools_interpolation::InterpolationGrid>(
    grid_points, Matrix::Constant(m, m, 1.0) // independence
  );
  npars_ = 0.0;
}

inline Vector
KernelBicop::pdf_raw(const Matrix& u)
{
  auto pdf = interp_grid_->interpolate(u);
  tools_eigen::trim(pdf, 1e-20, DBL_MAX);
  return pdf;
}

inline Vector
KernelBicop::pdf(const Matrix& u)
{
  if (u.cols() == 4) {
    // evaluate jittered density at mid rank for stability
    return pdf_raw((u.leftCols(2) + u.rightCols(2)).array() / 2.0);
  }
  return pdf_raw(u);
}

inline Vector
KernelBicop::cdf(const Matrix& u)
{
  return interp_grid_->integrate_2d(u);
}

inline Vector
KernelBicop::hfunc1_raw(const Matrix& u)
{
  return interp_grid_->integrate_1d(u, 1);
}

inline Vector
KernelBicop::hfunc2_raw(const Matrix& u)
{
  return interp_grid_->integrate_1d(u, 2);
}

inline Vector
KernelBicop::hfunc1(const Matrix& u)
{
  if (u.cols() == 4) {
    auto u_avg = u;
    u_avg.col(0) = (u.col(0) + u.col(2)).array() / 2.0;
    return hfunc1_raw(u_avg.leftCols(2));
  }
  return hfunc1_raw(u);
}

inline Vector
KernelBicop::hfunc2(const Matrix& u)
{
  if (u.cols() == 4) {
    auto u_avg = u;
    u_avg.col(1) = (u.col(1) + u.col(3)).array() / 2.0;
    return hfunc2_raw(u_avg.leftCols(2));
  }
  return hfunc2_raw(u);
}

inline Vector
KernelBicop::hinv1_raw(const Matrix& u)
{
  return hinv1_num(u);
}

inline Vector
KernelBicop::hinv2_raw(const Matrix& u)
{
  return hinv2_num(u);
}

inline double
KernelBicop::parameters_to_tau(const Matrix& parameters)
{
  auto oldpars = this->get_parameters();
  auto old_types = var_types_;
  this->set_parameters(parameters);
  var_types_ = { "c", "c" };

  std::vector<int> seeds = {
    204967043, 733593603, 184618802, 399707801, 290266245
  };
  auto u = tools_stats::ghalton(1000, 2, seeds);
  u.col(1) = hinv1_raw(u);

  this->set_parameters(oldpars);
  var_types_ = old_types;
  return wdm::wdm(u, "tau")(0, 1);
}

inline double
KernelBicop::get_npars() const
{
  return npars_;
}

inline void
KernelBicop::set_npars(const double& npars)
{
  if (npars < 0) {
    throw std::runtime_error("npars must be positive.");
  }
  npars_ = npars;
}

inline Matrix
KernelBicop::get_parameters() const
{
  return interp_grid_->get_values();
}

inline Matrix
KernelBicop::get_parameters_lower_bounds() const
{
  return Matrix::Constant(30, 30, 0.0);
}

inline Matrix
KernelBicop::get_parameters_upper_bounds() const
{
  return Matrix::Constant(30, 30, 1e4);
}

inline void
KernelBicop::set_parameters(const Matrix& parameters)
{
  if (parameters.minCoeff() < 0) {
    std::stringstream message;
    message << "density should be larger than 0. ";
    throw std::runtime_error(message.str().c_str());
  }
  // don't normalize again!
  interp_grid_->set_values(parameters, 0);
}

inline void
KernelBicop::flip()
{
  interp_grid_->flip();
}

inline Matrix
KernelBicop::tau_to_parameters(const double& tau)
{
  return no_tau_to_parameters(tau);
}

// construct default grid (equally spaced on Gaussian scale)
inline Vector
KernelBicop::make_normal_grid(size_t m)
{
  Vector grid_points(m);
  for (size_t i = 0; i < m; ++i)
    grid_points(i) =
      -3.25 + static_cast<double>(i) * (6.5 / static_cast<double>(m - 1));
  grid_points = tools_stats::pnorm(grid_points);

  return grid_points;
}
}
