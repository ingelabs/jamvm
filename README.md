# JamVM

JamVM is a compact, open-source Java Virtual Machine. The execution engine supports different levels of optimisation, from a basic switched interpreter to inline-threading with stack-caching (equivalent in performance to a simple JIT).

JamVM supports two class library backends:

- [GNU Classpath](https://github.com/ingelabs/classpath) - lightweight, suitable for embedded targets
- OpenJDK 8 - for environments requiring broader API compatibility

## Supported platforms

The primary targets are Linux embedded systems. The following architectures are supported:

- Linux / 32-bit ARM
- Linux / 64-bit ARM (aarch64)
- Linux / x86_64

macOS with Apple Silicon is also supported for development (not as a target).

Other platforms supported by the build system (including FreeBSD, OpenBSD and PowerPC) may work but are not actively tested.

## Building

See [README](README) and [INSTALL](INSTALL) for build instructions.

## History

This project is a fork of the original JamVM project, which is no longer actively maintained (last release: 2.0.0, July 2014).

The original project site can be found here: https://jamvm.sourceforge.net/

This fork continues development with a focus on embedded Linux targets.
