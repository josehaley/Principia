# Principia

Principia is a mod for Kerbal Space Program (KSP) which implements N-body and extended body gravitation.  Instead of being within the sphere of influence of a single celestial body at any point in time, your vessels are influenced by all the celestials.  This makes it possible to implement missions that are more complex and more realistic than in the stock game, especially if used in conjunction with a mod like RealSolarSystem which has real-life celestials.

N-body gravitation is more complex than the toy physics of the stock game.  Therefore, before using the mod we recommend that you read the [concepts](https://github.com/mockingbirdnest/Principia/wiki/Concepts) document which explains the most important parts of Principia.  In particular, you should learn about the [plotting frame](https://github.com/mockingbirdnest/Principia/wiki/Concepts#plotting-frame) and [flight planning](https://github.com/mockingbirdnest/Principia/wiki/Concepts#flight-planning).

You might also want to go through our
[tutorial](https://github.com/mockingbirdnest/Principia/wiki/A-guide-to-going-to-the-Mun-with-Principia) which shows how 
to go to the Mun with Principia in an energy-efficient manner.

The [FAQ](https://github.com/mockingbirdnest/Principia/wiki/Installing,-reporting-bugs,-and-frequently-asked-questions) explain how to install, how to report bugs and documents the known issues and limitations.

The [change log](https://github.com/mockingbirdnest/Principia/wiki/Change-Log) gives a fairly detailed description of the new features in each release.

Download the binary (Ubuntu and Windows) [here](https://goo.gl/eZbr4e) or, if you don't trust our binary, build the mod from the [Cantor](https://github.com/mockingbirdnest/Principia/releases/tag/2016081314-Cantor) release.

## Technical overview
Most KSP mods consist of some CLR DLLs that get loaded by KSP. Principia has most of its logic (in particular, all the numerical analysis) in a native library (`ksp_plugin/`) compiled from C++14, which is called from the C♯ layer (`ksp_plugin_adapter/`) via a P/Invoke interface. That interface is, for the most part, generated by a custom code generator (`tools/`) from protocol buffer descriptors (`serialization/journal.proto`). Those descriptors allow for journalling of anything crossing the interface; this is used for profile-guided optimization as well as debugging.

For serialization (saves), the C++ objects are serialized using protocol buffers (see the `.proto` files in `serialization/`), and the protocol buffer binary format is then written as a sequence of hexadecimal strings to the KSP save.

The rest of the codebase is mostly a series of C++14 template libraries, which come with their tests. Roughly in topological order,
- `base/` contains general utilities.
- `quantities/` is a strongly-typed library for physical quantities. The implementation uses `binary64` (`double`) floating-point in SI units, but this is not exposed to the user, who explicitly multiplies or divides by units. Angles are dimensionful to obviate confusion between degrees and radians. Strongly-typed elementary functions are provided.
- `geometry/` deals with geometrical concepts useful for classical physics: strongly-typed `Vector`s of some `Quantity` with a strongly-typed dependency on the reference frame, a distinction between `Vector`s and `Bivector`s (also known as pseudovectors), a distinction between `Vector`s and `Point`s (which live an affine space). It provides abstractions for changes of coordinates between reference frames (`Rotation`, `OrthogonalMap`, `Permutation`).  Vector quantities useful for classical physics are named, e.g., [`Position`, `Velocity`, `AngularVelocity`](https://github.com/mockingbirdnest/Principia/blob/master/geometry/named_quantities.hpp)...
- `numerics/` provides various numerical utilities, with enough templatization to work with the aforementioned types. This contains some code with precomputed matrices generated by a Mathematica notebook.
- `integrators/` focuses specifically on numerical integration.
- `physics/` provides libraries to deal with a physical system under the influence of gravity: abstractions for bodies, trajectories, ephemerides, and reference frames defined from the bodies in the physical system, as well as some support for Kepler orbits and Jacobi coordinates.
- `astronomy/` contains libraries and data that have to do with the real world: our solar system, our timescales (support for date literals in TT, TAI, UT1, and UTC, where the type `Instant` counts TT). Time scale handling involves code generated from IERS tables by an awk script. Moreover, `astronomy/` contains high-level tests of the above libraries for real-life astronomical scenarii: precession of the perihelion of Mercury, lunar eclipses...

In addition,
- `benchmarks/` contains benchmarks using (a fork of) https://github.com/google/benchmark/,
- `benchmark_automation/` is a C♯ utility used to run the benchmarks and parse their output,
- `coverage_analyser/` is a C♯ utility that interprets the output of the instrumented tests to yield coverage statistics,
- `stacktrace_decoder/` is a C♯ utility used to turn logs containing stack trace addresses and the DLL base address into [something usable](https://github.com/mockingbirdnest/Principia/issues/896#issue-135074672).