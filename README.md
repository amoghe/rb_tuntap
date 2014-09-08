# RbTunTap

This is a simple library to manipulate (create, configure, persist, delete) tun and tap interfaces (for Linux). Most of the library is implemented as a C extension since it needs to request the changes (creation, deletion, etc.) of the interfaces from the kernel via syscalls. The ruby land wrapper code provides a simpler API for ruby programs to interface with.

## What are tun/tap interfaces?

These interfaces provide packet reception and transmission capabilities for userspace programs. They work with IP and Ethernet frames respectively. [This][1] is a good primer on what these interfaces are and what they are capable of. 

## Platforms

Currently this is only developed (and tested) on Linux (Ubuntu 12.04 and 14.04). It should work on other (modern) Linux kernels/distributions as well (kernels that ship with most popular distributions support this).

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'rb_tuntap'
```

And then execute:

    $ bundle install

Or install it yourself as:

    $ gem install rb_tuntap

## Usage

Creating a tun device is as easy as:

```ruby
tun = RbTunTap::TunDevice.new(DEV_NAME)
tun.open(true)
```

Similarly tap devices are created like this:

```ruby
tap = RbTunTap::TapDevice.new(DEV_NAME)
tap.open(true)
```

The parameter to the ```open()``` method determines whether the tun device will return packets with additional metadata from the kernel (i.e. - sets the ```IFF_NO_PI``` flag accordingly).

Next, you'll want to configure the (tun) device:

```ruby
tun.addr = "192.168.1.4"
tun.netmask = "255.255.255.0"
```

And then bring up the interface, maybe persisting it:

```ruby
tun.up
tun.persist(true) # pass false to undo
```

For tap devices, you may want to also set the hardware address:

```ruby
tap.hwaddr = DEV_HWADDR
```

Reading from the device(s) can be done via the IO object they return

```ruby
tio = tun.to_io
tio.read(4)
```

See the examples directory for a script that demonstrates similar usage of this gem.

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
