#ifndef __compile_assert_H
#define __compile_assert_H

template < bool >
struct compile_assert;

template <>
struct compile_assert< true > {
	compile_assert() {}
};

#endif // __compile_assert_H
