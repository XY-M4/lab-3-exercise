#ifndef SHARED_PTR_HEADER
#define SHARED_PTR_HEADER

// Lab3-exercise
#include <cassert> // for assert statements
#include <utility> // for std::forward

class ControlBlockBase {
public:
    ControlBlockBase(): mRefCount {1} {} // DONE: implement the default constructor.

    // dtor is virtual, so that we can call derived class's dtor from a ptr to this base class.
    virtual ~ControlBlockBase() = default; // DONE: implement the destructor.

    // pure virtual function; must be overriden by derived classes
    virtual void* managedAddress() = 0;

    // Delete copies, which also implicitly deletes moves.
    ControlBlockBase(const ControlBlockBase&) = delete;
    ControlBlockBase& operator=(const ControlBlockBase&) = delete;

    // Example usage: controlBlock->increment();
    long increment() {
        // DONE: increment refcount by 1 and return result.
        assert(mRefCount > 0);
        ++mRefCount;
        return mRefCount;
    } // add one owner and return new refcount

    // Example usage: controlBlock->decrement();
    long decrement() {
        // DONE: decrement refcount by 1 and return result.
        assert(mRefCount > 0);
        --mRefCount;
        return mRefCount;
    } // remove one owner and return new refcount

    // Example usage: long count = controlBlock->refCount();
    long refCount() const {
        // DONE: just return the refcount.
        return mRefCount;
    } // return number of current owners
private:
    // DONE: add field(s) which both control block types need to have
    long mRefCount; // No. of SharedPtrs that currently own the resource
};

template <typename T>
class ControlBlock: public ControlBlockBase {
public:
    ControlBlock(T* ptr): mPtr {ptr} {} // Constructor

    ~ControlBlock() override { 
        delete mPtr; } // Destructor

    void* managedAddress() override { // override base class
        return mPtr; } // Return address of managed object
private: 
    T* mPtr; // store a (private) T*, pointing to the managed object
};

// DONE Bonus: Embedded Control Block (+15%) 
template <typename T>
class ControlBlockEmbedded: public ControlBlockBase {
public:
    // Constructor, Example usage: ControlBlockEmbedded<T>(args...);
    template <typename... Args>
    ControlBlockEmbedded(Args&&... args): mValue(std::forward<Args>(args)...) { 
    } // create T embedded inside control block, forward args

    ~ControlBlockEmbedded() override = default; // Destructor
    // T is destroyed automatically when control block is destroyed

    void* managedAddress() override { // override base class
        return &mValue;
    } // return address of embedded T
private:
    T mValue; // embedded T
};

/////////////// check code below
template <typename T>
class SharedPtr {
public:
    SharedPtr(): mPtr {nullptr}, mControlBlock {nullptr} { // Default constructor
    } // Example usage: SharedPtr<int> ptr;, empty SharedPtr owns nothing

    SharedPtr(T* ptr): mPtr {ptr}, mControlBlock {new ControlBlock<T>(ptr)} { // Constructor
    } // Example usage: SharedPtr<int> ptr(new int(5)); store ptr and create control block with refcount 1

    ~SharedPtr() { // Destructor
        if (mControlBlock != nullptr) { // if Non-empty
            if (mControlBlock->decrement() == 0) { // Delete one owner, if empty
                delete mControlBlock; // Delete SharedPtr object
            }
        }
    } // delete control block when owners=0

    // Copy constructor, Example usage: SharedPtr<int> ptr2(ptr1);
    SharedPtr(const SharedPtr& other): mPtr {other.mPtr}, mControlBlock {other.mControlBlock} {
        if (mControlBlock != nullptr) { // if Non-empty
            mControlBlock->increment(); // Add owner
        }
    } // share same pointer and increase refcount by 1

    // Move constructor, Example usage: SharedPtr<int> ptr2(std::move(ptr1));
    SharedPtr(SharedPtr&& other): mPtr {other.mPtr}, mControlBlock {other.mControlBlock} {
        other.mPtr = nullptr;
        other.mControlBlock = nullptr;
    } // steal both resources without changing refcount

    // Copy assignment operator, Example usage: ptr2 = ptr1;
    SharedPtr& operator=(const SharedPtr& other) {
        SharedPtr temp(other);
        swap(temp);
        return *this;
    } // temp deletes old resource when leaves scope

    // Move assignment operator, Example usage: ptr2 = std::move(ptr1);
    SharedPtr& operator=(SharedPtr&& other) {
        SharedPtr temp(std::move(other));
        swap(temp);
        return *this;
    } // steal other's resource, temp deletes old resource when leaves scope

    // Dereference operator, Example usage: int value = *ptr;
    T& operator*() const {
        assert(mPtr != nullptr);
        return *mPtr;
    } // allows for dereference of SharedPtrs.

    // Arrow operator, Example usage: sPtr->method();
    T* operator->() const {
        assert(mPtr != nullptr);
        return mPtr;
    } // returns the underlying pointer, allowing member access (methods)

    // Get underlying stored pointer, Example usage: int* value = sPtr.get();
    T* get() const {return mPtr;} 

    // Conversion operator, Example usage: if (ptr1 == ptr2)
    bool operator==(const SharedPtr<T>& other) const {
        return mPtr == other.mPtr;
    } // return true, if both mPtr values are equal, else false

    // Bool conversion, Example usage: if (ptr)
    explicit operator bool() const {
        return mPtr != nullptr;
    } // return false if empty, else true 

    // Swap SharedPtrs, Example usage: ptr1.swap(ptr2);
    void swap(SharedPtr<T>& other) {
        std::swap(mPtr, other.mPtr);
        std::swap(mControlBlock, other.mControlBlock);
    } // swap owners, refcounts do not change

    // Reset to empty, Example usage: ptr.reset();
    void reset() {
        SharedPtr().swap(*this);
    } // temporary SharedPtr() takes current resource and dies out of scope

    // Reset to a new value, Example usage: ptr.reset(new int(10));
    void reset(T* other) {
        if (other == mPtr && other != nullptr) {return;} // sPtr must be different
        SharedPtr(other).swap(*this); // self-assignment, make control block
    } // delete old value out of scope and left w new value

    // Get No. of owners, Example usage: long count = ptr.useCount();
    long useCount() const {
        if (mControlBlock == nullptr) {return 0;} // if empty, 0 owners
        return mControlBlock->refCount(); // refcount = No. of owners
    } // return 0 if there is no control block, else return refcount

    // Bonus: Aliasing constructor (+5%)
    template <typename U> //declare type U
    SharedPtr(const SharedPtr<U>& other, T* storedPtr): mPtr(storedPtr), mControlBlock(other.mControlBlock) { // copy contents
        if (mControlBlock != nullptr) { // share control block
            mControlBlock->increment(); } // increase refcount
    } // share same control block, but refer storedPtr

private:
    template <typename U> friend class SharedPtr; // Permission to access other sPtr's private attributes
    T* mPtr;   // store a (private) T*, pointing to the managed object
    ControlBlockBase* mControlBlock; // control block shared between owners
    
    // DONE Bonus: Constructor for Embedded Control Block (+15%)
    template <typename U, typename... Args> // Permit makeShared to use sPtr's private constructor
    friend SharedPtr<U> makeShared(Args&&... args);
    explicit SharedPtr(T* storedPtr, ControlBlockBase* controlBlock)
        : mPtr {storedPtr}, mControlBlock {controlBlock} { // pass in pointer and control block
    } 
};

// DONE: Create a Shared Pointer, Example usage: auto sPtr = makeSharedBasic<int>(5);
template <typename T, typename... Args> 
SharedPtr<T> makeSharedBasic(Args&&... args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...)); //pass args using forward
} // returns new SharedPtr<T> which manages new T

// DONE Bonus: Create a Shared Pointer using Embedded Control Block (+15%)
// Example usage: auto sPtr = makeShared<int>(5);
template <typename T, typename... Args> SharedPtr<T> makeShared(Args&&... args) {
    // create control block containing T embedded inside it (not T*)
    ControlBlockEmbedded<T>* controlBlock = new ControlBlockEmbedded<T>(std::forward<Args>(args)...); 
    T* storedPtr = static_cast<T*>(controlBlock->managedAddress()); // get address of embedded T and convert void* to T*
    return SharedPtr<T>(storedPtr, controlBlock); // return new SharedPtr
}

#endif