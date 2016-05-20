# RbTunTap

This gem provides the ability to manipulate (create, configure, persist, delete) tun and tap interfaces (for Linux). Most of the library is implemented as a C extension since it needs to request the changes (creation, deletion, of interfaces) from the kernel via syscalls. Given the relatively complex structures that are involved, it is easier to do this in C than using ffi. The ruby land wrapper code provides a simpler (more ruby-esque) API for ruby programs to interface with.

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

**NOTE** In order to use this gem you probably need superuser (root) privileges on the system you are on.
In order for the following examples to work, you need to launch IRB (or your program which uses this gem)
using `sudo`.

### Creating interfaces

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

### Configuring interfaces

Next, you'll want to configure the device (e.g. tun):

```ruby
tun.addr    = "192.168.168.1"
tun.netmask = "255.255.255.0"
```

And then bring up the interface:

```ruby
tun.up
```

Optionally, persist it:

```ruby
tun.persist(true) # false to undo persistence
```

You may want to also set the hardware address:

```ruby
tap.hwaddr = DEV_HWADDR # e.g. "9a:34:76:31:b5:6a"
```

### Reading from interfaces

Reading from the device(s) can be done via the IO object they return

```ruby
tio = tun.to_io
```

Note that you probably want to use ```IO#readpartial``` or ```IO#sysread``` to read at least ```mtu``` bytes from the device thereby ensuring you are reading off entire packets/frames.

```ruby
raw_ip = tun.to_io.sysread(tun.mtu)
```

You can get some really convenient (eth) packet parsing using tap interfaces and the [PacketFu][3] gem:

```ruby
irb(main)> require 'packetfu'
=> true

# We're reading an ethernet frame off the tap interface
irb(main)> raw = tap.to_io.sysread(tap.mtu)

# From another terminal, attempt to ping 192.168.168.11
# "raw" holds the packet we just read off the device.

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

Similarly, you can use [PacketFu][3] to parse packets from the tun interface (ip packets) as well. You have to do a little more work since PacketFu works with full ethernet frames:

```ruby
irb(main)> require 'packetfu'
=> true

# Assuming the tun interface is up()'ed and assigned valid IP and netmask
# (192.168.168.1/24 in this example).
#
# From another terminal, ensure that the tun interface is serving as the gateway
# via which its network is reachable. For this example, the routing table is:
#
# default via 10.0.2.2 dev eth0
# 10.0.2.0/24 dev eth0  proto kernel  scope link  src 10.0.2.15
# 192.168.168.0/24 via 192.168.168.1 dev tun0
#
# Note that the tun interface is on the 192.168.168.x network and the route indicates
# that the 192.168.168.x network is reachable via the tun interface
#
# Now trying to ping an imaginary machine on the 192 network (e.g. 192.168.168.14)

# Read a packet off the tun interface
irb(main)> ip = tun.to_io.sysread(tun.mtu)

# Create an eth packet (using packetfu)
irb(main)> eth = PacketFu::EthPacket.new

# Set its payload to be the IP packet we just read
irb(main)> eth.payload = ip

# Now create a packetfu ICMPPacket using this eth packet
irb(main)> PacketFu::ICMPPacket.new.read(eth.to_s)
=> --EthHeader-----------------------------------
  eth_dst   00:01:ac:00:00:00 PacketFu::EthMac
  eth_src   00:01:ac:00:00:00 PacketFu::EthMac
  eth_proto 0x0800            StructFu::Int16
--IPHeader------------------------------------
  ip_v      4                 Fixnum
  ip_hl     5                 Fixnum
  ip_tos    0                 StructFu::Int8
  ip_len    84                StructFu::Int16
  ip_id     0xbfba            StructFu::Int16
  ip_frag   16384             StructFu::Int16
  ip_ttl    64                StructFu::Int8
  ip_proto  1                 StructFu::Int8
  ip_sum    0xa98d            StructFu::Int16
  ip_src    192.168.168.1     PacketFu::Octets
  ip_dst    192.168.168.14    PacketFu::Octets
--ICMPHeader----------------------------------
  icmp_type 8                 StructFu::Int8
  icmp_code 0                 StructFu::Int8
  icmp_sum  0xde39            StructFu::Int16

----- 8< OUTPUT SNIPPED 8< -----
```

Finally, don't forget to clean up

```ruby
tun.down
tun.close
```

See the examples directory for a script that demonstrates similar usage of this gem.

## Contributing

0. Create an issue, describe the bugfix/feature you wish to implement
1. Fork the repository ( [from here][2] )
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create a new Pull Request

## TODOs

0. Add multiqueue support (i.e. ```IFF_MULTI_QUEUE```) for parallel access
1. Add documentation for ruby API
2. Add tests

[1]: https://www.kernel.org/doc/Documentation/networking/tuntap.txt
[2]: https://github.com/[my-github-username]/rb-tuntap/fork
[3]: https://github.com/packetfu/packetfu
