Project:

HadesMem

Purpose:

To provide a safe and generic C++-based memory hacking library for Windows based applications.

Rationale:

Previous implementations of similar libraries typically took one of either two paths:
1. A safe explicit API. (i.e. ReadInt32, ReadFloat, ReadString, etc)
2. An unsafe generic API. (i.e. Read<T>, where T is any type, with no restrictions.)

The goal of this library is to combine the best of both worlds, by using templates to provide a generic and extendible API, whilst still retaining type safety.

(Plus, I was bored and wanted a reason to play with the new C++0x type traits.)

Example:

Memory reads are performed using the 'Read' template member function of the 'Memory' class.

The base definition is as follows:

// Read memory (POD types)
template <typename T>
T Read(PVOID Address, typename boost::enable_if<std::is_pod<T>>::type* 
Dummy = 0) const;

As you can see, the library makes use of type traits to ensure that any calls to this function will only succeed if 'T' is a POD type (which basically means it is safe to treat as just a 'blob of data').

Overloads are then provided to facilitate the reading of a couple of common non-POD types:

// Read memory (string types)
template <typename T>
T Read(PVOID Address, typename boost::enable_if<std::is_same<T, 
std::basic_string<typename T::value_type>>>::type* Dummy = 0) const;

// Read memory (vector types)
template <typename T>
T Read(PVOID Address, typename std::vector<typename T::value_type>::
size_type Size, typename boost::enable_if<std::is_same<T, std::vector<
typename T::value_type>>>::type* Dummy = 0) const;

The former template will be chosen if 'T' is a string type (std::string, std::wstring).

The latter template will be chosen if 'T' is a vector type (std::vector<U>, where U is any arbitrary type);

Type safety is still retained even in the latter template by passing the vector's value type to the 'Read' template.

This means that the following code will compile and behave as expected:
auto MyInts = MyMemory.Read<std::vector<int>>(Address, 10); // Read 10 ints from address
auto MyStrings = MyMemory.Read<std::vector<std::string>>(Address, 10); // Read 10 null-terminated strings stored contiguously at address.
struct SomePodType { float Blah; unsigned int Foo; char* Asdf; };
auto MyPodType = MyMemory.Read<SomePodType>(Address); // Read a POD type from address

And the following will fail to compile as expected:
auto MyStreams = MyMemory.Read<std::vector<std::fstream>>(Address, 10); // Read 10 fstreams from address?? This makes no sense, and will not compile.

One important thing to note is that whilst types like 'string' and 'vector' are used, it's assumed that the underlying type you are operating on is their low-level equivalent.

Example, when you call read with a string template parameter, it's assuming you're trying to read a 'CharT*' (e.g. char* or wchar_t*), not an actual string object from the process.

The same applies to vector, as it is assumed you are simply trying to read an array.

This is done because even if you were trying to read a string object or a vector object out of memory, it would not be safe to do it using just Read<T> as non-POD types can not be safely copied in that manner. Hence, there will be no support for such dangerous operations.

If you need to read and write complex objects then you should break them down into their lower level components and read/write those.

The 'Write' collection of functions behave in the same manner. 

Notes:

* HadesMem is currently a header-only library.
* Both the interface and implementation of the library are under heavy development right now, so unfortunately breaking changes in new versions are inevitable.
* There is currently very little documentation. Proper Doxygen based documentation will be provided eventually.
* The current implementation is very basic as this was originally designed as a PoC which I then decided to expand upon. Regular improvements are being made though.
* A sample application is provided, but it is quite messy as it's a heavy WIP. A proper implementation is on the way.
* The only currently supported compiler is MSVC 10. An implementation that works with MSVC 9 is possible, however I currently have no interest in back-porting it.

Release:

HadesMem is released under the GPLv3* and the project is currently hosted at Google Code.

http://code.google.com/p/hadesmem/

* Please note that this means it may NOT be used in any closed-source commercial applications (so if you're a cheat seller, too bad). I will re-license to you it upon request though if you have a good reason.