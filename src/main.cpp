#include <cstdlib>
#include "SharedPtr.h"
#include <cassert>
#include <utility>

struct Test {int value; Test(int v): value(v) {}}; // Test class for makeSharedBasic, makeShared
struct Outer {int x;}; // Test class for aliasing constructor

// TESTS:
int main() {
    SharedPtr<int> empty;  // Default constructor
    assert(!empty);
    assert(empty.get() == nullptr);
    assert(empty.useCount() == 0);

    SharedPtr<int> p(new int(5)); // Underlying pointer constructor
    assert(p); 
    assert(*p == 5); // dereference
    assert(p.get() != nullptr); // get
    assert(p.useCount() == 1); 

    SharedPtr<int> p2(p); // Copy constructor
    assert(p.useCount() == 2);
    assert(p2.useCount() == 2);
    assert(p == p2); // comparison

    SharedPtr<int> p3(std::move(p2)); // Move constructor
    assert(!p2); // loses ownership
    assert(p3.useCount() == 2); // move does not change refcount

    SharedPtr<int> p4;  // Copy assignment
    p4 = p;
    assert(p4 == p); // both store same pointer
    assert(p.useCount() == 3); // another owner added

    SharedPtr<int> p5; // Move assignment
    p5 = std::move(p4);
    assert(!p4); // p4 loses ownership
    assert(p5 == p);
    assert(p.useCount() == 3); // move does not increase refcount

    p3.reset(); // reset()
    assert(!p3); // p3 becomes empty
    assert(p.useCount() == 2); // one owner removed

    p5.reset(new int(10)); // reset(T*)
    assert(*p5 == 10);
    assert(p5.useCount() == 1); // new control block has one owner
    assert(p.useCount() == 1); // p is now only owner of old int

    int* samePtr = p5.get(); // safe self reset
    p5.reset(p5.get());
    assert(p5.get() == samePtr); // pointer stays same
    assert(*p5 == 10);
    assert(p5.useCount() == 1);

    SharedPtr<int> a(new int(1)); // swap()
    SharedPtr<int> b(new int(2));
    a.swap(b);
    assert(*a == 2); // a now owns b's old pointer
    assert(*b == 1); // b now owns a's old pointer

    auto basic = makeSharedBasic<Test>(20); // makeSharedBasic and arrow operator
    assert(basic->value == 20);
    assert(basic.useCount() == 1);

    auto embedded = makeShared<Test>(30); // Embedded makeShared bonus
    assert(embedded->value == 30);
    assert(embedded.useCount() == 1);

    auto outer = makeShared<Outer>(); // Aliasing bonus
    outer->x = 42;
    SharedPtr<int> alias(outer, &outer->x);
    assert(*alias == 42); // alias stores pointer to outer->x
    assert(alias.get() == &outer->x); 
    assert(alias.useCount() == 2); // alias and outer share control block
    assert(outer.useCount() == 2);

    // Copy existing resource
    SharedPtr<int> copyDest(new int(100));
    SharedPtr<int> copySrc(new int(200));
    copyDest = copySrc;
    assert(*copyDest == 200);
    assert(copyDest == copySrc);
    assert(copySrc.useCount() == 2);

    // Move existing resource
    SharedPtr<int> moveDest(new int(300));
    SharedPtr<int> moveSrc(new int(400));
    moveDest = std::move(moveSrc);
    assert(*moveDest == 400);
    assert(!moveSrc);
    assert(moveDest.useCount() == 1);

    // Aliasing and original owner reset
    auto outer2 = makeShared<Outer>();
    outer2->x = 50;
    SharedPtr<int> alias2(outer2, &outer2->x);
    outer2.reset();
    assert(*alias2 == 50);
    assert(alias2.useCount() == 1);
   return EXIT_SUCCESS;
}