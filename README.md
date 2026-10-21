# CppMyVector

Refresher project on managing memory in C++, from-scratch implementation of `std::vector`. This is not replacement for `std::vector`

## Project structure

```
src/
  vector.hpp        # class declaration
  vector.tpp        # template implementations
  test_vector.cpp   # tests
Makefile
justfile
```

## Build and run

Requirements: g++, `make`.

```bash
make            # build
./test          # run tests
make clean
```
`just` can also be used for easier running.

```bash
just run        # build and run
just clean
```

## AI Usage

AI used for writing tests, minor reviews and utility

## License

MIT, see [LICENSE](LICENSE).