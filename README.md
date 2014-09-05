# RbTunTap

This is a simple library to manipulate (create, configure, persist, delete) tun and tap interfaces (for Linux). Most of the library is implemented as a C extension since it needs to request the creation/deletion/configuration of the interfaces from the kernel via syscalls. The ruby land code serves to provide a simpler API for ruby programs to interface with.

## What are tun/tap interfaces?

These interfaces provide packet reception and transmission capabilities for userspace programs. They work with IP and Ethernet frames respectively. [This][1] is a good primer on what these interfaces are and what they are capable of. 

## Installation

Currently this is only developed (and tested) on Linux (Ubuntu 12.04 and 14.04). It should work on other (modern) Linux distributions as well.

Add this line to your application's Gemfile:

```ruby
gem 'rb_tuntap'
```

And then execute:

    $ bundle install

Or install it yourself as:

    $ gem install rb_tuntap

## Usage

See the examples directory for basic usage.

## Contributing

1. Fork it ( [from here][2] )
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create a new Pull Request

## TODOs

1. Enforce rubucop (stylecheck/linting) for rb code
2. Add tests

[1]: https://www.kernel.org/doc/Documentation/networking/tuntap.txt
[2]: https://github.com/[my-github-username]/rb-tuntap/fork
