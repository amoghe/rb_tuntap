# RbTunTap

This is a simple library to manipulate (create, configure, persist, delete) tun and tap interfaces (for Linux). Most of the library is implemented as a C extension since it needs to request the changes (creation, deletion, etc.) of the interfaces from the kernel via syscalls. The ruby land wrapper code provides a simpler API for ruby programs to interface with.

## What are tun/tap interfaces?

These interfaces provide packet reception and transmission capabilities for userspace programs. They work with IP and Ethernet frames respectively. [This][1] is a good primer on what these interfaces are and what they are capable of. 

## Platforms

Currently this is only developed (and tested) on Linux (Ubuntu 12.04 and 14.04), however it should work on other (modern) Linux kernels/distributions as well (kernels that ship with most popular distributions support this).

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
tun = RbTunTap::TunDevice.new(DEV_NAME) # DEV_NAME = 'tun0'
tun.open(false)
```

Similarly tap devices are created like this:

```ruby
tap = RbTunTap::TapDevice.new(DEV_NAME) # DEV_NAME = 'tap0'
tap.open(false)
```

The parameter to the ```#open(pkt_info)``` method determines whether the device will return packet info metadata with with each packet (i.e. - sets the ```IFF_NO_PI``` flag accordingly).

If ```pkt_info``` is requested, then each frame format is:
```
  Flags [2 bytes]
  Proto [2 bytes]
  Raw protocol(IP, IPv6, etc) frame.
```

Next, you'll want to configure the (tun) device:

```ruby
tun.addr    = "192.168.168.1"
tun.netmask = "255.255.255.0"
```

And then bring up the interface, maybe persisting it:

```ruby
tun.up
tun.persist(true) # false to undo
```

You may want to also set the hardware address:

```ruby
tap.hwaddr = DEV_HWADDR # e.g. "9a:34:76:31:b5:6a"
```

Reading from the device(s) can be done via the IO object they return (note that you probably want to use ```IO#readpartial``` or ```IO#sysread``` to read at least ```mtu``` bytes from the device thereby ensuring you are reading off entire packets/frames)

```ruby
tio = tun.to_io
tio.sysread(tun.mtu)
```

You can get some really convenient (eth) packet parsing using the [PacketFu][3] gem:

```ruby
irb(main)> require 'packetfu'
=> true

# Here "tio" is attached to a tap interface so we're reading eth frames
irb(main)> raw = tio.sysread(tap.mtu)

# From another terminal, attempt to ping 192.168.168.11
# "raw" will hold the packet we just read off the device.

irb(main)> arp = PacketFu::ARPPacket.new.read(raw)
=> --EthHeader---------------------------------------
  eth_dst       ff:ff:ff:ff:ff:ff PacketFu::EthMac
  eth_src       9a:34:76:31:b5:6a PacketFu::EthMac
  eth_proto     0x0806            StructFu::Int16
--ARPHeader---------------------------------------
  arp_hw        1                 StructFu::Int16
  arp_proto     0x0800            StructFu::Int16
  arp_hw_len    6                 StructFu::Int8
  arp_proto_len 4                 StructFu::Int8
  arp_opcode    1                 StructFu::Int16
  arp_src_mac   9a:34:76:31:b5:6a PacketFu::EthMac
  arp_src_ip    192.168.168.168   PacketFu::Octets
  arp_dst_mac   00:00:00:00:00:00 PacketFu::EthMac
  arp_dst_ip    192.168.168.11    PacketFu::Octets
```

Finally, don't forget to clean up

```ruby
tun.down
tun.close
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
[3]: https://github.com/packetfu/packetfu
