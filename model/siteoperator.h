/*****************************************************************************
*
* ALPS Project: Algorithms and Libraries for Physics Simulations
*
* ALPS Libraries
*
* Copyright (C) 2003-2005 by Matthias Troyer <troyer@comp-phys.org>,
*                            Synge Todo <wistaria@comp-phys.org>
*
* This software is part of the ALPS libraries, published under the ALPS
* Library License; you can use, redistribute it and/or modify it under
* the terms of the license, either version 1 or (at your option) any later
* version.
* 
* You should have received a copy of the ALPS Library License along with
* the ALPS Libraries; see the file LICENSE.txt. If not, the license is also
* available from http://alps.comp-phys.org/.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
* DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

/* $Id$ */

#ifndef ALPS_MODEL_SITEOPERATOR_H
#define ALPS_MODEL_SITEOPERATOR_H

#include <alps/model/operator.h>
#include <alps/model/sitestate.h>
#include <alps/model/sitebasisstates.h>
#include <alps/expression.h>
#include <alps/multi_array.hpp>
#include <alps/parameters.h>

namespace alps {

template <class I, class T=std::complex<double>, class STATE = site_state<I> > class SiteOperatorEvaluator;
class ModelLibrary;

template <class I, class T=std::complex<double> >
class SiteOperatorSplitter : public OperatorEvaluator<T>
{
private:
  typedef OperatorEvaluator<T> super_type;
  typedef SiteOperatorSplitter<I,T> SELF_;

public:
  SiteOperatorSplitter(const SiteBasisDescriptor<I>& b,
                       const std::string& site,
                       const Parameters& p)
    : super_type(p), basis_(b), site_(site) {}

  bool can_evaluate_function(const std::string& name, const expression::Expression<T>& argument,bool=false) const;
  expression::Expression<T> partial_evaluate_function(const std::string& name, const expression::Expression<T>& argument,bool=false) const;
  const expression::Term<T>& site_operators() const { return site_ops_; }
  bool has_operator(const std::string& name, const expression::Expression<T>& arg) const
  { 
    return (arg==site_ && basis_.has_operator(name)); 
  }
  
private:
  const SiteBasisDescriptor<I>& basis_;
  mutable expression::Term<T> site_ops_;
  std::string site_;
};

class SiteOperator
{
public:
  SiteOperator() {}
  SiteOperator(const std::string& t, const std::string& s) : term_(t), site_(s) {}
  template <class T>
  SiteOperator(const T& t, const std::string& s)
    : term_(boost::lexical_cast<std::string>(t)), site_(s) {}
  SiteOperator(const std::string& t)
    : term_(t+"(i)"), site_("i") {}
  SiteOperator(const XMLTag& tag, std::istream& is) { read_xml(tag,is);}

  void read_xml(const XMLTag& tag, std::istream& is);
  void write_xml(oxstream&) const;

  const std::string& site() const { return site_;}
  const std::string& term() const { return term_;}
  const std::string& name() const { return name_;}
  template <class T, class I>
  boost::multi_array<T,2> matrix(const SiteBasisDescriptor<I>&,
                                          const Parameters& p=Parameters()) const;

  void substitute_operators(const ModelLibrary& m, const Parameters& p=Parameters());
  std::set<std::string> operator_names() const;

template <class T>
  std::vector<boost::tuple<expression::Term<T>,SiteOperator> > templated_split(const Parameters& = Parameters()) const;
  std::vector<boost::tuple<Term,SiteOperator> > split(const Parameters& p= Parameters()) const 
  { return templated_split<std::complex<double> >(p);}
private:
  std::string term_;
  std::string site_;
  std::string name_;
};


template <class I, class T>
inline boost::multi_array<T,2> get_matrix(T,const SiteOperator& m, const SiteBasisDescriptor<I>& basis1,  const Parameters& p=Parameters())
{
  return m.template matrix<T,I>(basis1,p);
}


template <class I, class T>
bool SiteOperatorSplitter<I,T>::can_evaluate_function(const std::string&name , const expression::Expression<T>& arg,bool isarg) const
{
  return (arg==site_ || expression::ParameterEvaluator<T>::can_evaluate_function(name,arg,isarg));
}


template <class I, class T>
expression::Expression<T> SiteOperatorSplitter<I,T>::partial_evaluate_function(const std::string& name, const expression::Expression<T>& arg, bool isarg) const
{
  if (arg==site_) {
    site_ops_ *= expression::Function<T>(name,arg);
    return  1.;
  }
  return expression::ParameterEvaluator<T>(*this).partial_evaluate_function(name,arg,isarg);
}


template <class T, class I> boost::multi_array<T,2>
SiteOperator::matrix(const SiteBasisDescriptor<I>& b,  const Parameters& p) const
{
  typedef typename expression_value_type_traits<T>::value_type value_type;

  SiteBasisDescriptor<I> basis(b);
  basis.set_parameters(p);
  Parameters parms(p);
  parms.copy_undefined(basis.get_parameters());
  std::size_t dim=basis.num_states();
  boost::multi_array<T,2> mat(boost::extents[dim][dim]);
  // parse expression and store it as sum of terms
  expression::Expression<value_type> ex(term());
  ex.flatten();
  ex.simplify();

  // fill the matrix
    site_basis<I> states(basis);
    for (int i=0;i<states.size();++i) {
    //calculate expression applied to state *it and store it into matrix
      for (typename expression::Expression<value_type>::term_iterator tit = ex.terms().first; tit !=ex.terms().second; ++tit) {
            SiteOperatorEvaluator<I,value_type> evaluator(states[i],basis,parms,site());
        expression::Term<value_type> term(*tit);
        term.partial_evaluate(evaluator);
        unsigned int j = states.index(evaluator.state());
        if (is_nonzero(term)) {
          if (evaluator.fermionic())
            boost::throw_exception(std::runtime_error("Fermionic number changing site operator is unphysical."));
          if (boost::is_arithmetic<T>::value || type_traits<T>::is_complex)
            if (!can_evaluate(boost::lexical_cast<std::string>(term)))
              boost::throw_exception(std::runtime_error("Cannot evaluate expression " + boost::lexical_cast<std::string>(term)));
          mat[i][j] += evaluate<T>(term);
          simplify(mat[i][j]);
        }
      }
    }
  return mat;
}


template <class T>
std::vector<boost::tuple<expression::Term<T>,SiteOperator> > alps::SiteOperator::templated_split(const Parameters& p) const
{
  std::vector<boost::tuple<expression::Term<T>,SiteOperator> > terms;
  Expression ex(term());
  ex.flatten();
  ex.simplify();
  SiteBasisDescriptor<short> b;
  for (typename Expression::term_iterator tit = ex.terms().first; tit !=ex.terms().second; ++tit) {
    SiteOperatorSplitter<short> evaluator(b,site(),p);
    expression::Term<T> term(*tit);
    term.partial_evaluate(evaluator);
    term.simplify();
    terms.push_back(boost::make_tuple(term,
        SiteOperator(boost::lexical_cast<std::string>(evaluator.site_operators()),site())));
  }
  return terms;
}



} // namespace alps

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
namespace alps {
#endif

inline alps::oxstream& operator<<(alps::oxstream& out, const alps::SiteOperator& q)
{
  q.write_xml(out);
  return out;
}

inline std::ostream& operator<<(std::ostream& out, const alps::SiteOperator& q)
{
  alps::oxstream xml(out);
  xml << q;
  return out;
}

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
} // namespace alps
#endif

#endif
