/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATBUFFERS_H_
#define FLATBUFFERS_H_

#include <cstdbool>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace flatbuffers {
template <typename T>
struct Offset {
    Offset() {
        data = std::make_shared<T>();
    }

    T* operator->() {
        return data.get();
    }

    const T* operator->() const {
        return data.get();
    }

    operator T*() {
        return data.get();
    }

    operator const T*() const {
        return data.get();
    }

    std::shared_ptr<T> data;
};

template <typename T>
class Vector {
public:
    Vector<T>() : vector_() {
    }

    Vector<T>(std::vector<T> vector) : vector_(vector) {
    }

    T Get(std::size_t f_index) const {
        return vector_[f_index];
    }

    void push_back(T f_param) {
        vector_.push_back(f_param);
    }

    T& operator[](std::size_t f_index) {
        return vector_[f_index];
    }
    std::size_t size() {
        return vector_.size();
    }

    bool empty() {
        return vector_.empty();
    }

    typename std::vector<T>::iterator begin() {
        return vector_.begin();
    }

    typename std::vector<T>::iterator end() {
        return vector_.end();
    }

    void clear() {
        vector_.clear();
    }

    typename std::vector<T>::value_type* data() {
        return vector_.data();
    }

    void resize(size_t n, const typename std::vector<T>::value_type& val) {
        vector_.resize(n, val);
    }

private:
    std::vector<T> vector_;
};

class String {
public:
    String(const char* str) : string_(str) {
    }

    String(std::string& str) : string_(str) {
    }

    const std::string& str() const {
        return string_;
    }

    const char* c_str() const {
        return string_.c_str();
    }

    std::size_t Length() const {
        return string_.length();
    }

    std::size_t size() const {
        return string_.length();
    }

private:
    std::string string_;
};

}  // namespace flatbuffers

#endif  // FLATBUFFERS_H_
