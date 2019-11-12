# libaad

## Step 1: Build on Linux

Step zero is to be in the right directory:
    
    cd <wherever-you-cloned-libaad>

First, install dependencies (e.g., clang, cmake) using:

    scripts/linux/install-deps.sh

Second, install pairing (`ate-pairing`), finite field (`libff`) and FFT libraries (`libfqfft`):

    scripts/linux/install-libs.sh

Then, go back to the `libaad` repo directory and set up the environment. 
    
    cd <wherever-libaad-was-cloned>
    source scripts/linux/set-env.sh release

...you can also use `debug`, `relwithdebug` or `trace` as an argument for `set-env.sh`.

To build:

    make.sh

(This **will store the built code in ~/builds/aad/**)

Alternatively, you could build manually in your own `build/` directory:

    mkdir -p build/
    cd build/
    cmake -DCMAKE_BUILD_TYPE=Release <path-to-libaad-repository>
    make

## Step 2: Tests

To run all tests, you can just invoke:
    
    test.sh

Tests, benchmarks and any other binaries are automatically added to `PATH` but can also be found in:

    cd ~/builds/aad/master/release/libaad/bin/

To run all test, you can also just do:
    
    cd ~/builds/aad/master/release
    ctest

(or replace `release` with `debug` or whatever you used in `set-env.sh`)

## Step 3: Install

You can install easily by just running:

    scripts/linux/install.sh

Alternatively, you can install manually:

    cd ~/builds/aad/master/<build-type>/
    sudo make install

## Git submodules

This is just for reference purposes. 
No need to execute these.
To fetch the submodules, just do:

    git submodule init
    git submodule update

For historical purposes, (i.e., don't execute these), when I set up the submodules, I did:
    
    cd depends/
    git submodule add git://github.com/herumi/ate-pairing.git
    git submodule add git://github.com/herumi/xbyak.git
    git submodule add git://github.com/scipr-lab/libff.git
    git submodule add https://github.com/scipr-lab/libfqfft.git

To update your submodules with changes from their upstream github repos, do:

    git submodule foreach git pull origin master

## Some documentation notes

See:

    cat depends/libff/libff/common/default_types/ec_pp.hpp

To see the interface for `libff`'s finite field type (i.e., libaad's `Fr` type) go to:

    cd depends/libff
    cat libff/algebra/fields/fp.hpp

For group elements, see:
    
    cd depends/libff
    cat libff/algebra/curves/public_params.hpp
    cat libff/algebra/curves/bn128/bn128_pp.hpp

For FFT polynomial operations, see:

    cd depends/libfqfft/
    cat libfqfft/polynomial_arithmetic/basic_operations.hpp
    cat libfqfft/polynomial_arithmetic/basic_operations.tcc

We use `-DCURVE_BN128`.
What is the `ALT_BN128` curve?

`-DBN_SUPPORT_SNARK` in `libff` and `ate-pairing` enables (what I think is) a curve with an $n$th root of unity for some $n = 2^i$. They say: _"BN curve over a 254-bit prime p such that n := p + 1 - t has high 2-adicity."_ (However the pairing on this curve is slower.)

_WARNING:_ Looking at `libff/algebra/curves/bn128/bn128_init.cpp` BN128 initialization code and the `ate-pairing` README, it seems that libff _ONLY_ supports the SNARK curve as a fast curve!
