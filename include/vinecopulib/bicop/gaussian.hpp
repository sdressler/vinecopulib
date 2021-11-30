// Copyright © 2016-2021 Thomas Nagler and Thibault Vatter
//
// This file is part of the vinecopulib library and licensed under the terms of
// the MIT license. For a copy, see the LICENSE file in the root directory of
// vinecopulib or https://vinecopulib.github.io/vinecopulib/.

#pragma once

#include <vinecopulib/bicop/elliptical.hpp>

namespace vinecopulib {
//! @brief The Gaussian copula.
//!
//! This class is used in the implementation underlying the Bicop class.
//! Users should not use AbstractBicop or derived classes directly, but
//! always work with the Bicop interface.
//!
//! @literature
//! Joe, Harry. Dependence modeling with copulas. CRC Press, 2014.
class GaussianBicop : public EllipticalBicop
{
public:
  // constructor
  GaussianBicop();

private:
  // PDF
  Vector pdf_raw(const Matrix& u);

  // CDF
  Vector cdf(const Matrix& u);

  // hfunction
  Vector hfunc1_raw(const Matrix& u);

  // inverse hfunction
  Vector hinv1_raw(const Matrix& u);

  Matrix tau_to_parameters(const double& tau);

  Vector get_start_parameters(const double tau);
};
}

#include <vinecopulib/bicop/implementation/gaussian.ipp>
