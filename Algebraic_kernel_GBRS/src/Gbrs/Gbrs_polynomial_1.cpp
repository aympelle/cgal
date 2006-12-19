// Copyright (c) 2006 Inria Lorraine (France). All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 2.1 of the License.
// See the file LICENSE.LGPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// 
//
// Author(s)     : Luis Peñaranda <penarand@loria.fr>

#include <CGAL/assertions.h>
#include <CGAL/basic.h>
#include <CGAL/Gmpz.h>
#include <CGAL/Gbrs_polynomial_1.h>
#include <CGAL/Gbrs_algebraic_1.h>
#include <iostream>
#include <gmp.h>

CGAL_BEGIN_NAMESPACE

// copy constructor
Rational_polynomial_1::Rational_polynomial_1(const Rational_polynomial_1 &p){
	degree=p.degree;
	mpz_t *p_coef=p.get_coefs();
	coef=(mpz_t*)malloc(sizeof(mpz_t)*(degree+1));
	// we have to copy the contents, not just the pointer
	for(int i=0;i<degree+1;++i){
		// mpz_init(mpz_ptr);
		mpz_init(coef[i]);
		// mpz_set(mpz_ptr, mpz_srcptr);
		mpz_set(coef[i],p_coef[i]);
	}
	solved=p.get_solved();
	//roots=p.get_roots();
};

// copy assignement operator
Rational_polynomial_1& Rational_polynomial_1::operator= (const Rational_polynomial_1 &p) {
	// destroy the current data
	for (int i=0; i<degree+1; ++i)
		mpz_clear (coef[i]);
	free (coef);
	// copy data from p
	degree = p.degree;
	coef = (mpz_t*)malloc (sizeof(mpz_t)*(degree+1));
	for (int i=0; i<degree+1; ++i)
		mpz_init_set(coef[i],p.coef[i]);
	solved=p.get_solved();
	return *this;
};

// other constructors
Rational_polynomial_1::Rational_polynomial_1():degree(0),solved(false){
	coef=(mpz_t*)malloc(sizeof(mpz_t));
	mpz_init(coef[0]);
	//roots.clear();
};

Rational_polynomial_1::Rational_polynomial_1(unsigned int d):solved(false){
	degree=(int)d;
	coef=(mpz_t*)malloc(sizeof(mpz_t)*(degree+1));
	for(int i=0;i<degree+1;++i)
		mpz_init(coef[i]);
	//roots.clear();
};

Rational_polynomial_1::Rational_polynomial_1(int d):solved(false){
	degree=d<0?0:d;
	coef=(mpz_t*)malloc(sizeof(mpz_t)*(degree+1));
	for(int i=0;i<degree+1;++i)
		mpz_init(coef[i]);
	//roots.clear();
};

// construct a polynomial whose root is the given rational
Rational_polynomial_1::Rational_polynomial_1(const mpq_t &r){
	degree=1;
	coef=(mpz_t*)malloc(sizeof(mpz_t)*2);
	mpz_init(coef[0]);
	mpz_neg(coef[0],mpq_numref(r));
	mpz_init(coef[1]);
	mpz_set(coef[1],mpq_denref(r));
	solved=false;
	//roots.clear();
};

// destructor
Rational_polynomial_1::~Rational_polynomial_1 () {
	for(int i=0;i<degree+1;++i)
		mpz_clear(coef[i]);
	free(coef);
	/*if(solved)
		roots.clear();*/
};

// member functions

void Rational_polynomial_1::set_degree(int d){	// dangerous function!
	for(int i=0;i<degree+1;++i)	// free the old coefficients
		mpz_clear(coef[i]);
	free(coef);
	degree=d;
	coef=(mpz_t*)malloc(sizeof(mpz_t)*(degree+1));
	for(int i=0;i<degree+1;++i)
		mpz_init(coef[i]);
	solved=false;
	return;
};

void Rational_polynomial_1::set_solved(){
	solved=true;
	//roots.clear();
};

void Rational_polynomial_1::clear_solved(){
	/*if(solved)
		roots.clear();*/
	solved=false;
	return;
};

/*CGAL::Algebraic_1 Rational_polynomial_1::eval_alg(const CGAL::Algebraic_1 &x)const{
	Algebraic_1 result(0);
	Algebraic_1 x_pow(1);
	for(int i=0;i<=degree;++i){
		// invariant at this point: x_pow = x^i
		result+=x_pow*coef[i];
		x_pow*=x;
	}
	return result;
};*/

CGAL::Gmpz Rational_polynomial_1::eval(const CGAL::Gmpz &x)const{
	mpz_t result,x_pow;
	mpz_init(result);	// it's 0 now
	mpz_init_set_si(x_pow,1);	// x^0 = 1
	for(int i=0;i<=degree;++i){	// invariant: x_pow = x^i
		mpz_addmul(result,coef[i],x_pow);
		mpz_mul(x_pow,x_pow,x.mpz());	// for the next iteration
	}
	mpz_clear(x_pow);
	CGAL::Gmpz ret(result);
	mpz_clear(result);
	return ret;
};

void Rational_polynomial_1::eval_mpfr
(mpfr_t &result,const mpfr_t &x,mp_prec_t prec)const{
	mpfr_t x_pow,temp;
	mp_prec_t prec_r=mpfr_get_prec(result);
	mpfr_inits2(prec<prec_r?prec_r:prec,x_pow,temp,NULL);
	mpfr_set_ui(x_pow,1,GMP_RNDN);
	mpfr_set_ui(result,0,GMP_RNDN);
	for(int i=0;i<=degree;++i){ // mpfr_fma?
		mpfr_mul_z(temp,x_pow,coef[i],GMP_RNDN);
		mpfr_add(result,temp,result,GMP_RNDN);
		mpfr_mul(x_pow,x_pow,x,GMP_RNDN);
	}
	mpfr_clears(x_pow,temp,NULL);
	return;
};

// I think RS should do this
void Rational_polynomial_1::eval_mpfi(mpfi_ptr result,mpfi_srcptr x)const{
	mpfi_set_z(result,coef[0]);
	if(degree&&((!mpfr_zero_p(&(x->left)))||(!mpfr_zero_p(&(x->right))))){
		mpfi_t x_pow,temp;
		mpfi_init_set(x_pow,x);
		mpfi_init_set_ui(temp,0);
		for(int i=1;i<degree;++i){
			if(mpz_sgn(coef[i])){
				mpfi_mul_z(temp,x_pow,coef[i]);
				mpfi_add(result,temp,result);
			}
			mpfi_mul(x_pow,x_pow,x);
		}
		mpfi_mul_z(temp,x_pow,coef[degree]);
		mpfi_clear(x_pow);
		mpfi_add(result,temp,result);
		mpfi_clear(temp);
	}
	return;
};

Rational_polynomial_1 Rational_polynomial_1::derive()const{
	Rational_polynomial_1 derivative (degree-1);
	mpz_t *coef_d=derivative.get_coefs();
	for(int x=1;x<degree+1;++x)
		mpz_mul_si(coef_d[x-1],coef[x],x);
	return derivative;
};

std::ostream& Rational_polynomial_1::show (std::ostream &s) const {
	bool printed = false;
	if(!degree)
		return(s<<coef[0]);
	for (int i=degree; i>=0; --i) {
		if(mpz_sgn(coef[i])){
			if (printed && (mpz_sgn (coef[i]) == 1))
				s << "+";
			printed = true;
			bool flag = false;
			if((!mpz_cmp_si(coef[i],-1))&&i)
				s << "-";
			else
				if((mpz_cmp_ui(coef[i],1))||(!i)){
					flag = true;
					s << coef[i];
				}
			if(i){
				if (flag)
					s << "*";
				s << "x";
				if(i-1)
					s << "^" << i;
			}
		}
	}
	if (!printed)
		s << "0";
#ifdef CGAL_RS_DEBUG
	s<<" [ d="<<degree<<" ";
	s<<"[ ";
	for (int i=0; i<degree+1; ++i)
		s<<coef[i]<<" ";
	s<<"] ]";
#endif
	return s;
};

Rational_polynomial_1 Rational_polynomial_1::operator-()const{
	Rational_polynomial_1 opposite(degree);
	mpz_t *coef_o=opposite.get_coefs();
	for(int i=0;i<degree+1;++i)
		mpz_neg(coef_o[i],coef[i]);
	return opposite;
};

// XXX: maybe it is better to use operator+= to implement this
Rational_polynomial_1 Rational_polynomial_1::operator+ (const Rational_polynomial_1 &s) const {
	int sd = s.get_degree ();
	int minord, majord;	// the minor and major of both degrees
	bool am_i_bigger;	// is *this' degree bigger than s' degree?
	mpz_t temp1, temp2;

	if (sd < degree) {
		am_i_bigger = true;
		minord = sd;
		majord = degree;
	} else {
		am_i_bigger = false;
		minord = degree;
		majord = sd;
	}
	
	Rational_polynomial_1 sum(majord);

	mpz_init (temp1);
	mpz_init (temp2);

	for (int i=0; i<=minord; ++i) {
		s.get_coef (i, &temp1);
		mpz_add(temp2,temp1,coef[i]);
		sum.set_coef (i, temp2);
	}

	mpz_clear (temp1);

	if (am_i_bigger)
		for (int i=minord+1; i<=majord; ++i)
			sum.set_coef(i,coef[i]);
	else
		for (int i=minord+1; i<=majord; ++i) {
			s.get_coef (i, &temp2);
			sum.set_coef (i, temp2);
		}

	mpz_clear (temp2);
	return sum;
};

Rational_polynomial_1& Rational_polynomial_1::operator+=(const Rational_polynomial_1 &s){
	mpz_t *coef_s=s.get_coefs();
	int sd;
	if(degree<(sd=s.get_degree())){
		mpz_t *coef_sum=(mpz_t*)malloc(sizeof(mpz_t)*(sd+1));
		for(int i=0;i<degree+1;++i){
			mpz_init(coef_sum[i]);
			mpz_add(coef_sum[i],coef_s[i],coef[i]);
			mpz_clear(coef[i]);
		}
		for(int i=degree+1;i<sd+1;++i)
			mpz_init_set(coef_sum[i],coef_s[i]);
		free(coef);
		coef=coef_sum;
		degree=sd;
	}else{
		for(int i=0;i<sd+1;++i)
			mpz_add(coef[i],coef_s[i],coef[i]);
	}
	solved=false;
	return *this;
};

// TODO: this function has to be optimized, because we will use it to intersect
Rational_polynomial_1 Rational_polynomial_1::operator- (const Rational_polynomial_1 &s) const {
	return (*this+(-s));
};

Rational_polynomial_1& Rational_polynomial_1::operator-= (const Rational_polynomial_1 &s) {
	Rational_polynomial_1 aux (*this);
	*this = aux - s;
	solved=false;
	return *this;
};

// multiplies the polynomial by c*x^shiftn
/*Rational_polynomial_1& Rational_polynomial_1::scale_and_shift(const mpz_t &s,int shiftn){
	mpz_t *new_coef=(mpz_t*)malloc(sizeof(mpz_t)*(degree+shiftn+1));
	for (int i=0;i<degree+1;++i){
		mpz_mul(coef[i],coef[i],s);
		mpz_init_set(new_coef[i+shiftn],coef[i]);
		mpz_clear(coef[i]);
	}
	degree+=shiftn;
	free(coef);
	for(int i=0;i<shiftn;++i)
		mpz_init(new_coef[i]);
	coef=new_coef;
	solved=false;
	return *this;
};*/

// TODO: karatsubize this
Rational_polynomial_1 Rational_polynomial_1::operator*(const Rational_polynomial_1 &f)const{
	/*Rational_polynomial_1 product;
	mpz_t *f_coefs;
	f_coefs=f.get_coefs();
	int df=f.get_degree();
	for(int i=0;i<=df;++i){
		Rational_polynomial_1 partial(*this);
		product+=partial.scale_and_shift(f_coefs[i],i);
	}
	return product;*/
	// TODO: test this a bit, and correct it if it doesn't work so we can
	// avoid using the above extra-slow c++ implementation
	mpz_t *coef_f=f.get_coefs();
	int degree_f=f.get_degree();
	int degree_p=degree+degree_f;
	Rational_polynomial_1 product(degree_p);
	mpz_t *coef_p=product.get_coefs();
	for(int c=0;c<degree_p+1;++c){
		int max=(c<degree?c:degree)+1;
		for(int i=0;i<max;++i)
			if(c-i<=degree_f)
				mpz_addmul(coef_p[c],coef[i],coef_f[c-i]);
	}
	return product;
};

Rational_polynomial_1& Rational_polynomial_1::operator*= (const Rational_polynomial_1 &f) {
	Rational_polynomial_1 aux (*this);
	*this = aux * f;
	solved=false;
	return *this;
};

Rational_polynomial_1& Rational_polynomial_1::operator*=(const mpz_t &s){
	for(int i=0;i<=degree;++i)
		mpz_mul(coef[i],coef[i],s);
	return *this;
};

Rational_polynomial_1& Rational_polynomial_1::operator*=(const CGAL::Gmpz &s){
	for(int i=0;i<=degree;++i)
		mpz_mul(coef[i],coef[i],s.mpz());
	return *this;
};

bool Rational_polynomial_1::operator==(const Rational_polynomial_1 &p)const{
	mpz_t *coef_p=p.get_coefs ();
	int degree_p;
	if((degree_p=p.get_degree())<degree){
		for(int i=degree_p+1;i<degree+1;++i)
			if(mpz_sgn(coef[i]))
				return false;
		for(int i=0;i<degree_p+1;++i)
			if(mpz_cmp(coef[i],coef_p[i]))
				return false;
	}else{
		for (int i=degree+1;i<degree_p+1;++i)
			if(mpz_sgn(coef_p[i]))
				return false;
		for(int i=0;i<degree_p+1;++i)
			if(mpz_cmp(coef[i],coef_p[i]))
				return false;
	}
	return true;
};

CGAL_END_NAMESPACE
