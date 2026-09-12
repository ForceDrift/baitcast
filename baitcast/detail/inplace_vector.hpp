#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202406L
#include <inplace_vector>
#endif

namespace baitcast::detail {

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202406L

  template <typename T, std::size_t Capacity> using inplace_vector = std::inplace_vector<T, Capacity>;

#else

  template <typename T, std::size_t Capacity> class inplace_vector {
    static_assert(Capacity > 0, "baitcast::inplace_vector requires Capacity > 0");
    static_assert(std::is_object_v<T>, "baitcast::inplace_vector requires an object value type");

  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = value_type *;
    using const_iterator = const value_type *;

    constexpr inplace_vector() noexcept = default;

    inplace_vector(const inplace_vector &other) {
      for (const value_type &value : other) {
        push_back(value);
      }
    }

    inplace_vector(inplace_vector &&other) noexcept(std::is_nothrow_move_constructible_v<value_type>) {
      for (value_type &value : other) {
        emplace_back(std::move(value));
      }
      other.clear();
    }

    inplace_vector &operator=(const inplace_vector &other) {
      if (this != &other) {
        clear();
        for (const value_type &value : other) {
          push_back(value);
        }
      }
      return *this;
    }

    inplace_vector &operator=(inplace_vector &&other) noexcept(std::is_nothrow_move_constructible_v<value_type>) {
      if (this != &other) {
        clear();
        for (value_type &value : other) {
          emplace_back(std::move(value));
        }
        other.clear();
      }
      return *this;
    }

    ~inplace_vector() { clear(); }

    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] constexpr bool full() const noexcept { return size_ == Capacity; }
    [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
    [[nodiscard]] static constexpr size_type capacity() noexcept { return Capacity; }
    [[nodiscard]] static constexpr size_type max_size() noexcept { return Capacity; }

    template <typename... Args> reference emplace_back(Args &&...args) {
      static_assert(std::is_constructible_v<value_type, Args &&...>, "emplace_back arguments must construct value_type");
      if (full()) {
        throw std::bad_alloc{};
      }
      pointer slot = reinterpret_cast<pointer>(storage_ + size_ * sizeof(value_type));
      std::construct_at(slot, std::forward<Args>(args)...);
      ++size_;
      return *slot;
    }

    void push_back(const_reference value) { emplace_back(value); }
    void push_back(value_type &&value) { emplace_back(std::move(value)); }

    void pop_back() noexcept {
      std::destroy_at(reinterpret_cast<pointer>(storage_ + (size_ - 1) * sizeof(value_type)));
      --size_;
    }

    void clear() noexcept {
      for (size_type i = 0; i < size_; ++i) {
        std::destroy_at(reinterpret_cast<pointer>(storage_ + i * sizeof(value_type)));
      }
      size_ = 0;
    }

    iterator erase(const_iterator cfirst, const_iterator clast) {
      iterator first = const_cast<iterator>(cfirst);
      iterator last = const_cast<iterator>(clast);
      iterator old_end = end();
      iterator new_end = std::move(last, old_end, first);
      for (iterator it = new_end; it != old_end; ++it) {
        std::destroy_at(std::addressof(*it));
      }
      size_ = static_cast<size_type>(new_end - begin());
      return first;
    }

    [[nodiscard]] reference operator[](size_type i) noexcept { return *reinterpret_cast<pointer>(storage_ + i * sizeof(value_type)); }
    [[nodiscard]] const_reference operator[](size_type i) const noexcept { return *reinterpret_cast<const_pointer>(storage_ + i * sizeof(value_type)); }

    [[nodiscard]] reference at(size_type i) {
      if (i >= size_) {
        throw std::out_of_range{"baitcast::inplace_vector::at"};
      }
      return (*this)[i];
    }
    [[nodiscard]] const_reference at(size_type i) const {
      if (i >= size_) {
        throw std::out_of_range{"baitcast::inplace_vector::at"};
      }
      return (*this)[i];
    }

    [[nodiscard]] reference front() noexcept { return (*this)[0]; }
    [[nodiscard]] const_reference front() const noexcept { return (*this)[0]; }
    [[nodiscard]] reference back() noexcept { return (*this)[size_ - 1]; }
    [[nodiscard]] const_reference back() const noexcept { return (*this)[size_ - 1]; }

    [[nodiscard]] pointer data() noexcept { return reinterpret_cast<pointer>(storage_); }
    [[nodiscard]] const_pointer data() const noexcept { return reinterpret_cast<const_pointer>(storage_); }

    [[nodiscard]] iterator begin() noexcept { return reinterpret_cast<pointer>(storage_); }
    [[nodiscard]] iterator end() noexcept { return reinterpret_cast<pointer>(storage_) + size_; }
    [[nodiscard]] const_iterator begin() const noexcept { return reinterpret_cast<const_pointer>(storage_); }
    [[nodiscard]] const_iterator end() const noexcept { return reinterpret_cast<const_pointer>(storage_) + size_; }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }

  private:
    alignas(value_type) unsigned char storage_[Capacity * sizeof(value_type)];
    size_type size_ = 0;
  };

#endif

} // namespace baitcast::detail