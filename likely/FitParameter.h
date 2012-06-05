// Created 27-May-2012 by David Kirkby (University of California, Irvine) <dkirkby@uci.edu>

#ifndef LIKELY_FIT_PARAMETER
#define LIKELY_FIT_PARAMETER

#include "likely/types.h"

#include <string>
#include <vector>

namespace likely {
	class FitParameter {
	// Represents a fit parameter, specified by its name, value and estimated error.
	// Class has no virtual destructor since this is plain-old-data. Do not inherit
	// from this class.
	public:
	    // Creates a new FitParameter or throws a RuntimeError if the optional error
	    // is negative. Zero error implies that the parameter is fixed.
		FitParameter(std::string const &name, double value, double error = 0);
        // Returns a copy of this parameter's name.
        std::string getName() const;
        // Returns this parameter's value.
        double getValue() const;
        // Sets a new value for this parameter.
        void setValue(double value);
        // Returns this parameter's estimated error, or zero if the parameter is fixed.
        double getError() const;
        // Sets a new error for this parameter, or throws a RuntimeError if the specified
        // error is < 0. Set an error of zero to permanently fix a parameter. Use the fix()
        // method to temporarily fix a parameter, so that release() will restore its original
        // estimated error.
        void setError(double error);
        void fix();
        void release();
        // Returns true if this parameter is floating, in which case it will have an error > 0.
        bool isFloating() const;
	private:
        std::string _name;
        double _value, _error;
	}; // FitParameter

    inline std::string FitParameter::getName() const { return _name; }
    inline double FitParameter::getValue() const { return _value; }
    inline void FitParameter::setValue(double value) { _value = value; }
    inline double FitParameter::getError() const { return _error; }
    inline void FitParameter::fix() { if(_error > 0) _error = -_error; }
    inline void FitParameter::release() { if(_error < 0) _error = -_error; }
    inline bool FitParameter::isFloating() const { return (_error > 0); }
    
    // Defines a vector of fit parameters.
    typedef std::vector<FitParameter> FitParameters;
    
    // Returns a vector of parameter values, errors or names. Use the optional onlyFloating
    // parameter to only include floating parameters in the result.
    void getFitParameterValues(FitParameters const &parameters, Parameters &values,
        bool onlyFloating = false);
    void getFitParameterErrors(FitParameters const &parameters, Parameters &values,
        bool onlyFloating = false);
    void getFitParameterNames(FitParameters const &parameters, std::vector<std::string> &names,
        bool onlyFloating = false);
    int countFloatingFitParameters(FitParameters const &parameters);
    int findFitParameterByName(FitParameters const &parameters, std::string const &name);

} // likely

#endif // LIKELY_FIT_PARAMETER
