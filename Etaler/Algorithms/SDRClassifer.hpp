#pragma once

#include <Etaler/Core/Tensor.hpp>
#include <Etaler/Core/Serialize.hpp>

#include "Etaler_export.h"

#include <vector>

namespace et
{

struct ETALER_EXPORT SDRClassifer
{
	SDRClassifer() = default;
	SDRClassifer(Shape input_shape, size_t num_classes)
		: input_shape_(input_shape), reference_(zeros(Shape{(intmax_t)num_classes}+input_shape
			, DType::Int32))
		, num_patterns_(num_classes)
	{
	}

	size_t numCategories() const
	{
		return num_patterns_.size();
	}

	void addPattern(const Tensor& sdr, size_t class_id)
	{
		et_assert(sdr.shape() == input_shape_);
		et_assert(sdr.dtype() == DType::Bool);
		et_assert(class_id < num_patterns_.size());

		reference_.view({(intmax_t)class_id}) = reference_.view({(intmax_t)class_id}) + sdr;
		num_patterns_[class_id]++;
	}

	size_t compute(const Tensor& x, float min_common_frac=0.75) const
	{
		const size_t num_classes = num_patterns_.size();
		std::vector<float> threshold(num_classes);
		for(size_t i=0;i<num_classes;i++)
			threshold[i] = num_patterns_[i]*min_common_frac;
		Tensor thr = Tensor(Shape{(intmax_t)num_classes, 1}, threshold.data(), reference_.backend());
		const auto match = sum((reference_ > thr) && x.reshape(Shape{1}+x.shape()), 1).toHost<int>();

		assert(match.size() == num_classes);
		size_t best_match_id = 0;
		int best_match = 0;
		for(size_t i=0;i<num_classes;i++) {
			if(best_match < match[i])
				std::tie(best_match, best_match_id) = std::pair(match[i], i);
		}
		return best_match_id;
	}

	StateDict states() const
	{
		return {{"input_shape", input_shape_}, {"reference", reference_}, {"num_patterns", num_patterns_}};
	}

	void loadState(const StateDict& states)
	{
		input_shape_ = std::any_cast<Shape>(states.at("input_shape"));
		reference_ = std::any_cast<Tensor>(states.at("reference"));
		num_patterns_ = std::any_cast<std::vector<int>>(states.at("num_patterns"));
	}

	SDRClassifer to(Backend* b) const
	{
		SDRClassifer c = *this;
		assert(c.reference_.size() == reference_.size());
		c.reference_ = reference_.to(b);
		return c;
	}

	SDRClassifer copy() const
	{
		if(reference_.size() == 0)
			return *this;
		return to(reference_.backend());
	}

	Shape input_shape_;
	Tensor reference_;
	std::vector<int> num_patterns_;
};

// SDRClassifer in Etaler is CLAClassifer in NuPIC
// SDRClassifer from NuPIC is not implemented
using CLAClassifer = SDRClassifer;

}
