#ifndef COMMONTESTDEFINITION_HPP
#define COMMONTESTDEFINITION_HPP

#include "gtest/gtest.h"

template<typename T, std::size_t size>
testing::AssertionResult ArraysMatch(const T *expected, const T *actual) {
    for (auto i = 0; i < size; ++i) {
        if(expected[i] != actual[i])
            return testing::AssertionFailure()
                    << "array[" << i << "] (" << actual[i] << ") != expected["
                    << i << "] (" << expected[i] << ")";
    }

    return testing::AssertionSuccess();
}

#endif // COMMONTESTDEFINITION_HPP
