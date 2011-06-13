// Created 30-May-2011 by David Kirkby (University of California, Irvine) <dkirkby@uci.edu>

#include "likely/FunctionMinimum.h"
#include "likely/Random.h"
#include "likely/RuntimeError.h"

#include "boost/format.hpp"
#include "boost/lambda/lambda.hpp"

#include <iostream>
#include <algorithm>
#include <cmath>

// Declare a binding to this LAPACK Cholesky decomposition routine:
// http://www.netlib.org/lapack/double/dpptrf.f
extern "C" {
    void dpptrf_(char* uplo, int* n, double* ap, int *info);
}

namespace local = likely;

local::FunctionMinimum::FunctionMinimum(double minValue, Parameters const& where)
: _minValue(minValue), _where(where), _random(Random::instance())
{
}

local::FunctionMinimum::FunctionMinimum(double minValue, Parameters const& where,
PackedCovariance const &covar, bool errorsOnly)
: _minValue(minValue), _where(where), _random(Random::instance())
{
    if(!updateCovariance(covar,errorsOnly)) {
        throw RuntimeError("FunctionMinimum: covariance is not positive definite.");
    }
}

local::FunctionMinimum::~FunctionMinimum() { }

void local::FunctionMinimum::updateParameters(Parameters const &params, double fval) {
    _minValue = fval;
    _where = params;
}

bool local::FunctionMinimum::updateCovariance(PackedCovariance const &covar,
bool errorsOnly) {
    int nPar(_where.size()),nCovar(nPar*(nPar+1)/2);
    if(errorsOnly) {
        // We have a vector of errors, instead of a full covariance matrix.
        if(covar.size() != nPar) throw RuntimeError(
            "FunctionMinimum: parameter and error vectors have incompatible sizes.");
        // Check for any errors <= 0.
        if(std::find_if(covar.begin(), covar.end(), boost::lambda::_1 <= 0)
            != covar.end()) return false;
        // Create a diogonal covariance matrix of squared errors.
        _covar.reset(new PackedCovariance(nCovar,0));
        for(int i = 0; i < nPar; ++i) {
            double error(covar[i]);
            (*_covar)[i*(i+3)/2] = error*error;
        }
        // (should probably fill this directly from the input errors instead)
        _cholesky = choleskyDecomposition(*_covar);
    }
    else {
        if(covar.size() != nCovar) throw RuntimeError(
            "FunctionMinimum: parameter and covariance vectors have incompatible sizes.");
        // Use a Cholesky decomposition to test for positive definiteness.
        PackedCovariancePtr cholesky(choleskyDecomposition(covar));
        if(!cholesky) return false;
        _cholesky = cholesky;
        _covar.reset(new PackedCovariance(covar));
    }
}

local::Parameters local::FunctionMinimum::getErrors() const {
    if(!haveCovariance()) {
        throw RuntimeError("FunctionMinimum::getErrors: no covariance matrix available.");
    }
    int nPar(_where.size());
    Parameters errors(nPar);
    for(int i = 0; i < nPar; ++i) {
        double sigsq((*_covar)[i*(i+3)/2]);
        errors[i] = sigsq > 0 ? std::sqrt(sigsq) : 0;
    }
    return errors;
}

local::PackedCovariancePtr local::choleskyDecomposition(PackedCovariance const &covar) {
    // Copy the covariance matrix provided.
    PackedCovariancePtr cholesky(new PackedCovariance(covar));
    // Calculate the number of parameters corresponding to this packed covariance size.
    int nCov(covar.size());
    int nPar(std::floor(0.5*std::sqrt(1+8*nCov)));
    if(nCov != nPar*(nPar+1)/2) {
        throw RuntimeError("choleskyDecomposition: internal error nCov ~ nPar");
    }
    // Use LAPACK to perform the decomposition.
    char uplo('U');
    int info(0);
    dpptrf_(&uplo,&nPar,&(*cholesky)[0],&info);
    if(0 != info) {
        // Reset our return value so that it tests false using, e.g. if(cholesky) ...
        cholesky.reset();
    }
    return cholesky;
}

double local::FunctionMinimum::setRandomParameters(Parameters &params) const {
    if(!haveCovariance()) {
        throw RuntimeError(
            "FunctionMinimum::getRandomParameters: no covariance matrix available.");
    }
    int nPar(_where.size());
    Parameters gauss(nPar);
    double nlWeight(0);
    for(int i = 0; i < nPar; ++i) {
        // Initialize the generated parameters to the function minimum.
        params[i] = _where[i];
        // Fill a vector of random Gaussian variables.
        double r(_random.getNormal());
        gauss[i] = r;
        nlWeight += r*r;
    }
    // Multiply by the Cholesky decomposition matrix.
    PackedCovariance::const_iterator next(_cholesky->begin());
    for(int j = 0; j < nPar; ++j) {
        for(int i = 0; i <= j; ++i) {
            params[j] += (*next++)*gauss[i];
        }
    }
    return nlWeight/2;
}

void local::FunctionMinimum::printToStream(std::ostream &os,
std::string formatSpec) const {
    boost::format formatter(formatSpec);
    os << "F(" << formatter % _where[0];
    int nPar(_where.size());
    for(int i = 1; i < nPar; ++i) {
        os << ',' << formatter % _where[i];
    }
    os << ") = " << formatter % _minValue << std::endl;
    if(haveCovariance()) {
        Parameters errors(getErrors());
        os << "ERRORS:";
        for(int i = 0; i < nPar; ++i) {
            os << ' ' << formatter % errors[i];
        }
        os << std::endl << "COVARIANCE:" << std::endl;
        for(int i = 0; i < nPar; ++i) {
            for(int j = 0; j < nPar; ++j) {
                int index = (i <= j) ? i + j*(j+1)/2 : j + i*(i+1)/2;
                os << ' ' << formatter % (*_covar)[index];
            }
            os << std::endl;
        }
    }
}