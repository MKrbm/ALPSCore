/***************************************************************************
* ALPS++ library
*
* alps/random/random.h     a few extensions to boost/random.hpp
*
* $Id$
*
* Copyright (C) 1994-2003 by Matthias Troyer <troyer@itp.phys.ethz.ch>
*
* Permission is hereby granted, free of charge, to any person or organization 
* obtaining a copy of the software covered by this license (the "Software") 
* to use, reproduce, display, distribute, execute, and transmit the Software, 
* and to prepare derivative works of the Software, and to permit others
* to do so for non-commerical academic use, all subject to the following:
*
* The copyright notice in the Software and this entire statement, including 
* the above license grant, this restriction and the following disclaimer, 
* must be included in all copies of the Software, in whole or in part, and 
* all derivative works of the Software, unless such copies or derivative 
* works are solely in the form of machine-executable object code generated by 
* a source language processor.

* In any scientific publication based in part or wholly on the Software, the
* use of the Software has to be acknowledged and the publications quoted
* on the web page http://www.alps.org/license/ have to be referenced.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
* DEALINGS IN THE SOFTWARE.
*
**************************************************************************/

#ifndef ALPS_RANDOM_H
#define ALPS_RANDOM_H

#include <alps/config.h>
#include <alps/random/pseudo_des.h>

#include <boost/random.hpp>
#include <boost/utility.hpp>
#include <boost/throw_exception.hpp>
#include <boost/generator_iterator.hpp>

#include <iostream>

namespace alps {

template <class RNG>
void seed_with_sequence(RNG& rng, uint32_t seed)
{
  pseudo_des start(seed);
  pseudo_des end(seed);
  start(); // make start!=end
  typedef boost::generator_iterator_generator<pseudo_des>::type iterator_type;
  iterator_type start_it(boost::make_generator_iterator(start));
  iterator_type end_it(boost::make_generator_iterator(end));
  rng.seed(start_it,end_it);
}

template <class T=double>
class BufferedRandomNumberGeneratorBase
{
public:
  typedef T result_type;
  BufferedRandomNumberGeneratorBase(std::size_t b=10240) 
   : buf_(b), ptr_(buf_.end()) {}

  T operator()() {
    if(ptr_==buf_.end()) {
      fill_buffer();
      ptr_=buf_.begin();
    }
    return *ptr_++;
  }
  virtual void seed(uint32_t) = 0;
  virtual void seed() =0;
  virtual void write(std::ostream&) const =0;
  virtual void read(std::istream&)=0;
protected:
  std::vector<T> buf_;
  typename std::vector<T>::iterator ptr_;
private:
  virtual void fill_buffer() = 0;
};

template <class RNG> class BufferedRandomNumberGeneratorAdaptor
 : public BufferedRandomNumberGeneratorAdaptor<typename RNG::result_type>
{
public:
  typedef typename RNG::result_type result_type;

  BufferedRandomNumberGeneratorAdaptor(RNG& rng) : rng_(rng) {}

  void fill_buffer();
  template <class IT>
  void seed(IT start, IT end) { rng_.seed(start,end);}
  void seed(uint32_t);
  void seed();
  virtual void write(std::ostream&) const;
  virtual void read(std::istream&);
protected:
  const RNG& rng_;
};



template <class RNG>
void BufferedRandomNumberGeneratorAdaptor<RNG>::seed(uint32_t s)
{
  seed_with_sequence(rng_,s);
}

template <class RNG>
void BufferedRandomNumberGeneratorAdaptor<RNG>::read(std::istream& is)
{
  is >> rng_;
}

template <class RNG>
void BufferedRandomNumberGeneratorAdaptor<RNG>::write(std::ostream& os) const
{
  os << rng_;
}

template <class RNG>
void BufferedRandomNumberGeneratorAdaptor<RNG>::fill_buffer()
{
  std::fill(buf_.begin(),buf_.end(),rng_);
}

template <class RNG> class BufferedRandomNumberGenerator
 : public BufferedRandomNumberGeneratorAdaptor<typename RNG::result_type>
{
public:
  typedef typename RNG::result_type result_type;

  BufferedRandomNumberGenerator() 
   : BufferedRandomNumberGeneratorAdaptor<RNG>(rngstore_) {}
   
  BufferedRandomNumberGenerator(const RNG& r) 
   : BufferedRandomNumberGeneratorAdaptor<RNG>(r),
     rngstore_(r) {}

 private:
  RNG rngstore_;
};

} // end namespace

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
namespace alps {
#endif

template <class T>
inline std::ostream& operator<<(std::ostream& os, const BufferedRandomNumberGeneratorBase<T>& r) {
  r.write(os);
  return os;
}

template <class T>
std::istream& operator>>(std::istream& is, BufferedRandomNumberGeneratorBase<T>& r) {
  r.read(is);
  return is;
}

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
} // end namespace alps
#endif

#endif // ALPS_RANDOM_H
