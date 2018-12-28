// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// limitations under the License.

#include <algorithm>
#include <iostream>
#include <vector>
#include "tafn.hpp"

struct multiply {};
struct add {};

void tafn_customization_point(multiply, tafn::type<int>, int& i, int value) {
	i *= value;
}
int tafn_customization_point(add, tafn::type<int>, const int& i, int value) {
	return i + value;
}

namespace algs {
	struct sort {};
	struct unique {};
	struct copy {};
	template <size_t I>
	struct get {};

	template <typename T>
	decltype(auto) tafn_customization_point(sort, tafn::all_types, T&& t) {
		std::sort(t.begin(), t.end());
		return std::forward<T>(t);
	}
	template <typename T>
	decltype(auto) tafn_customization_point(unique, tafn::all_types, T&& t) {
		auto iter = std::unique(t.begin(), t.end());
		t.erase(iter, t.end());
		return std::forward<T>(t);
	}
	template <typename T, typename I>
	decltype(auto) tafn_customization_point(copy, tafn::all_types, T&& t, I iter) {
		std::copy(t.begin(), t.end(), iter);
		return std::forward<T>(t);
	}
	template <typename T, size_t I>
	decltype(auto) tafn_customization_point(get<I>, tafn::all_types, T&& t) {
		using std::get;
		return get<I>(std::forward<T>(t));
	}

}  // namespace algs

template <typename T>
void sort_unique(T&& t) {
	tafn::wrap(t)._<algs::sort>()._<algs::unique>();
}

#include <system_error>

struct dummy {};

struct operation1 {};
struct operation2 {};

dummy& tafn_customization_point(operation1, tafn::type<dummy>, dummy& d, int i,
	std::error_code& ec) {
	if (i == 2) {
		ec = std::make_error_code(std::errc::function_not_supported);
	}
	return d;
}

void tafn_customization_point(operation2, tafn::type<dummy>, dummy& d, int i,
	int j, std::error_code& ec) {
	if (j == 2) {
		ec = std::make_error_code(std::errc::function_not_supported);
	}
}

template <typename F, typename... Args>
decltype(auto) tafn_customization_point(tafn::all_functions<F>,
	tafn::type<dummy>, Args&&... args) {
	std::error_code ec;

	auto call = [&]() mutable {
		return tafn::call_customization_point<tafn::all_functions<F>>(
			std::forward<Args>(args)..., ec);
	};

	using V = decltype(call());
	if constexpr (!std::is_same_v<void, V>) {
		V r = call();
		if (ec) throw std::system_error(ec);
		return r;
	}
	else {
		call();
		if (ec) throw std::system_error(ec);
	}
}

void test_exception() {
	try {
		dummy d;
		std::error_code ec;
		tafn::wrap(d)._<operation1>(1, ec)._<operation2>(2, 2);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
	}
}

#include <string>
#include <string_view>

struct get_all_lines {};

std::vector<std::string> tafn_customization_point(get_all_lines, tafn::all_types, std::istream& is) {
	std::vector<std::string> result;
	std::string line;
	while (std::getline(is, line)) {
		result.push_back(line);
	}
	return result;
}

struct output {};
template<typename T>
void tafn_customization_point(output, tafn::all_types, T&& t, std::ostream& os, std::string_view delimit = "") {
	os << t << delimit;
}

template<typename F>
struct call_for_each {};
template<typename F, typename C, typename... Args>
void tafn_customization_point(call_for_each<F>, tafn::all_types, C&& c, Args&&... args) {
	for (auto&& v : std::forward<C>(c)) {
		tafn::call_customization_point<F>(std::forward<decltype(v)>(v), std::forward<Args>(args)...);
	}
}


#include <iterator>
#include <tuple>
int main() {
	int i{ 5 };

	tafn::wrap(i)._<multiply>(10);

	std::cout << i;

	int j = 9;
	auto k = tafn::wrap(i)._<add>(2)._<add>(3).unwrapped;
	std::cout << k << " " << tafn::wrap(j)._<add>(4)._<add>(5).unwrapped << " ";

	std::vector<int> v{
		4, 4, 1, 2, 2, 9, 9, 9, 7, 6, 6,
	};
	tafn::wrap(v)._<algs::sort>()._<algs::unique>()._<call_for_each<output>>(std::cout, "\n");
	sort_unique(v);

	std::tuple<int, char, int> t{ 1, 2, 3 };
	std::cout << tafn::wrap(t)._<algs::get<2>>().unwrapped;

	test_exception();


	tafn::wrap(std::cin)._<get_all_lines>()._<algs::sort>()._<algs::unique>()._<call_for_each<output>>(std::cout, "\n");


}