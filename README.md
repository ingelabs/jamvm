# JamVM

JamVM is a compact, open-source virtual machine for the Java programming language, originally written by Robert Lougher. The execution engine supports different levels of optimisation, from a basic switched interpreter to inline-threading with stack-caching (equivalent in performance to a simple JIT).

JamVM supports two class library backends:

- [GNU Classpath](https://github.com/ingelabs/classpath) - lightweight, suitable for embedded targets
- OpenJDK 8 - for environments requiring broader API coverage

## Supported platforms

The primary targets are Linux embedded systems. The following architectures are supported:

- Linux / 32-bit ARM
- Linux / 64-bit ARM (aarch64)
- Linux / x86_64

macOS with Apple Silicon is also supported for development (not as a target).

Other platforms supported by the build system (including FreeBSD, OpenBSD and PowerPC) may work but are not actively tested.

## Building

See the original [README](README) and [INSTALL](INSTALL) files for build instructions.

## History

JamVM was created and developed by Robert Lougher, who maintained it until version 2.0.0, released in July 2014. The original project site can be found at https://jamvm.sourceforge.net/.

This project is the maintained continuation of his work, with a focus on embedded Linux targets.
