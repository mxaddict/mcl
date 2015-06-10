#pragma once
/**
	@file
	@brief power window method
	@author MITSUNARI Shigeo(@herumi)
	@note
	Copyright (c) 2014, National Institute of Advanced Industrial
	Science and Technology All rights reserved.
	This source file is subject to BSD 3-Clause license.
*/
#include <vector>
#include <mcl/fp.hpp>

namespace mcl { namespace fp {

/*
	get w-bit size from x[0, bitLen)
	@param x [in] data
	@param bitLen [in] data size
	@param w [in] split size < UnitBitN
*/
template<class T>
struct ArrayIterator {
	static const size_t TBitN = sizeof(T) * 8;
	ArrayIterator(const T *x, size_t bitLen, size_t w)
		: x(x)
		, bitLen(bitLen)
		, w(w)
		, pos(0)
		, mask((w == TBitN ? 0 : (T(1) << w)) - 1)
	{
		assert(w <= TBitN);
	}
	bool hasNext() const { return bitLen > 0; }
	T getNext()
	{
		if (w == TBitN) {
			bitLen -= w;
			return *x++;
		}
		if (pos + w < TBitN) {
			T v = (*x >> pos) & mask;
			pos += w;
			if (bitLen < w) {
				bitLen = 0;
			} else {
				bitLen -= w;
			}
			return v;
		}
		if (pos + bitLen <= TBitN) {
			assert(bitLen <= w);
			T v = *x >> pos;
			assert((v >> bitLen) == 0);
			bitLen = 0;
			return v & mask;
		}
		assert(pos > 0);
		T v = (x[0] >> pos) | (x[1] << (TBitN - pos));
		v &= mask;
		pos = (pos + w) - TBitN;
		bitLen -= w;
		x++;
		return v;
	}
	const T *x;
	size_t bitLen;
	size_t w;
	size_t pos;
	T mask;
};

template<class Ec>
class PowerWindow {
public:
	typedef std::vector<Ec> EcV;
	size_t bitLen_;
	size_t winSize_;
	std::vector<EcV> tbl_;
	PowerWindow(const Ec& x, size_t bitLen, size_t winSize)
	{
		init(x, bitLen, winSize);
	}
	PowerWindow()
		: bitLen_(0)
		, winSize_(0)
	{
	}
	/*
		@param x [in] base index
		@param bitLen [in] exponent bit length
		@param winSize [in] window size
	*/
	void init(const Ec& x, size_t bitLen, size_t winSize)
	{
		bitLen_ = bitLen;
		winSize_ = winSize;
		const size_t tblNum = (bitLen + winSize) / winSize;
		const size_t r = size_t(1) << winSize;
		// alloc table
		tbl_.resize(tblNum);
		Ec t(x);
		for (size_t i = 0; i < tblNum; i++) {
			tbl_[i].resize(r);
			EcV& w = tbl_[i];
			for (size_t d = 1; d < r; d *= 2) {
				for (size_t j = 0; j < d; j++) {
					Ec::add(w[j + d], w[j], t);
				}
				Ec::dbl(t, t);
			}
		}
	}
	/*
		@param z [out] x^y
		@param y [in] exponent
	*/
	template<class tag2, size_t maxBitN2>
	void power(Ec& z, const FpT<tag2, maxBitN2>& y) const
	{
		fp::Block b;
		y.getBlock(b);
		powerArray(z, b.p, b.n * UnitBitN, false);
	}
	void power(Ec& z, int y) const
	{
		if (y == 0) {
			z.clear();
			return;
		}
		Unit u = std::abs(y);
		powerArray(z, &u, cybozu::bsr<Unit>(y) + 1, y < 0);
	}
	void power(Ec& z, const mpz_class& y) const
	{
		powerArray(z, Gmp::getBlock(y), abs(y.get_mpz_t()->_mp_size) * UnitBitN, y < 0);
	}
	void powerArray(Ec& z, const Unit* y, size_t bitLen, bool isNegative) const
	{
		z.clear();
		if (bitLen == 0) return;
		size_t i = 0;
		ArrayIterator<Unit> ai(y, bitLen, winSize_);
		do {
			Unit v = ai.getNext();
			if (i >= tbl_.size()) throw cybozu::Exception("mcl:PowerWindow:power:bad value") << i << tbl_.size() << bitLen << winSize_;
			if (v) {
				Ec::add(z, z, tbl_[i][v]);
			}
			i++;
		} while (ai.hasNext());
		if (isNegative) {
			Ec::neg(z, z);
		}
	}
};

} } // mcl::fp

