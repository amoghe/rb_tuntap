#!/usr/bin/env ruby

require 'rb_tuntap'
require 'timeout'

DEV_NAME = 'tun0'
DEV_ADDR = '192.168.192.168'

STDOUT.puts("** Opening tun device as #{DEV_NAME}")
tun = RbTunTap::TunDevice.new(DEV_NAME)
tun.open(true)

STDOUT.puts("** Assigning ip #{DEV_ADDR} to device")
tun.addr = DEV_ADDR
tun.up

STDOUT.puts("** Interface stats (as seen by ifconfig)")
STDOUT.puts(`ifconfig #{DEV_NAME}`)

STDOUT.puts("** Reading 4 bytes from the tun device (waiting 5s)")
begin
  Timeout::timeout(5) {
    io = tun.to_io
    bytes = io.sysread(4)
  }
rescue Timeout::Error
  STDOUT.puts("Nothing to read")
else
  STDOUT.puts("Read: #{bytes}")
end

STDOUT.puts("** Bringing down and closing device")
tun.down
tun.close
