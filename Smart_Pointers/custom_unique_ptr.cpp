#include <iostream>

template<typename T>
class unique_ptr
{
    T* ptr_;
    public:
        /* Constructor which should inforce explicit object creation and doesn't throw any exceptions*/
        /* It also allows to create empty unique ptr*/
        explicit unique_ptr(T* ptr = nullptr) noexcept: ptr_(ptr) {}

        /*IMPORTANT: destructor which releases memory when pointer goes out of scope*/
        ~unique_ptr() { delete ptr_; }

        /* Copy constructor is not allowed in unique_ptr */
        unique_ptr(const unique_ptr&) = delete;

        /* Move constructor */
        /* Means: I accept a temporary object whose resources can be stolen. */
        unique_ptr(unique_ptr&& other) noexcept : ptr_(other.ptr_)
        {
            other.ptr_ = nullptr;
        }

        /* Delete Copy Assignment*/
        /* Implicit assignment operator is not allowed */
        unique_ptr& operator=(const unique_ptr&) = delete;
        
        /* Move assignment operator */
        unique_ptr& operator=(unique_ptr&& other) noexcept
        {
            if(this != &other)
            {
                delete ptr_;
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }

        T& operator*() const { return *ptr_; }

        T* operator->() const { return ptr_; }

};

int main()
{
    unique_ptr<int> p(new int(10));
    std::cout << *p << std::endl;

    unique_ptr<int> q = std::move(p);

    std::cout << *q << std::endl;

    std::cout << *p << std::endl;

    return 0;
}