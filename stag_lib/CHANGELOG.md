# Changelog
All notable changes to the library are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Changed
- [Issue #5](https://github.com/pmacg/stag/issues/5): Rename the `Graph::volume()` method to `Graph::total_volume()`
- [Issue #5](https://github.com/pmacg/stag/issues/5): Attempting to construct a graph with an assymetric adjacency matrix
throws an exception

### Added
- [Issue #5](https://github.com/pmacg/stag/issues/5): `Graph::degree_matrix()` method
- [Issue #5](https://github.com/pmacg/stag/issues/5): `Graph::normalised_laplacian()` method
- [Issue #5](https://github.com/pmacg/stag/issues/5): `Graph::number_of_vertices()` method
- [Issue #5](https://github.com/pmacg/stag/issues/5): `Graph::number_of_edges()` method
- [Issue #4](https://github.com/pmacg/stag/issues/4): Add `load_edgelist()` method to read graphs from edgelist files
- [Issue #4](https://github.com/pmacg/stag/issues/4): Add `save_edgelist()` method to save graphs to edgelist files
- Add graph equality operators `==` and `!=`
- [Issue #6](https://github.com/pmacg/stag/issues/6): Add `LocalGraph` abstract class for providing local graph access

## [0.1.6] - 2022-10-10
### Added
- Graph class with a few basic methods
- `adjacency()` and `laplacian()` methods
- `cycle_graph(n)` and `complete_graph(n)` graph constructors