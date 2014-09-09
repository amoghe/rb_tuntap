require 'ipaddr'
require 'rb_tuntap_ext'

module RbTunTap

  #
  # The main +Device+ class that holds the bulk of the logic to instantiate and
  # twiddle the interface (mostly implemented in the C extension).
  #
  class Device

    def to_io
      @io ||= IO.new(fd)
    end

    def opened?
      fd > 0
    end

    def closed?
      !opened?
    end

    def detect_tun_device_file
      [ '/dev/net/tun',
        '/dev/tun',
      ].each do |f|
        return f if File.exists?(f) # and File.readable?(f)
      end
    end

    def addr=(address)
      set_addr(validate_address!(address))
    end

    def addr
      get_addr
    end

    def dstaddr=(dst)
      set_dstaddr(validate_address!(dst))
    end

    def dstaddr
      get_dstaddr
    end

    def netmask=(nm)
      set_netmask(validate_netmask!(nm))
    end

    def netmask
      get_netmask
    end

    def hwaddr=(hw)
      unless hw.is_a? String
        raise ArgumentError, 'Argument must be a String'
      end

      unless hw.split(':').count == 6
        raise ArgumentError, 'Invalid address string specified'
      end

      hwintarr = hw.split(':').map { |tok| tok.hex }

      set_hwaddr(hwintarr)
    end

    def hwaddr
      ret = ""
      get_hwaddr.map { |int|
        int.to_s(16)
      }.join(':')
    end

    def mtu=(m)
      set_mtu(m)
    end

    def mtu
      get_mtu
    end

    private

    # Validate that the given string is a valid IP address.
    #
    # @raises ArgumentError, NotImplementedError
    def validate_address!(addr)
      ip = IPAddr.new(addr)

      unless ip.ipv4?
        raise NotImplementedError, 'Only IPv4 is supported by this library'
      end

      if addr.include?('/')
        raise ArgumentError, 'Please specify a host IP address (without mask)'
      end

      addr.to_s
    end

    def validate_netmask!(nm)
      if IPAddr.new(nm).to_i.to_s(2).include?("01")
        raise ArgumentError, "Invalid netmask"
      end

      nm.to_s
    end

  end

  #
  # A derivative of +Device+ that is a TUN device.
  #
  class TunDevice < Device

    def initialize(name)
      super(name, RbTunTap::DEV_TYPE_TUN, detect_tun_device_file)
    end

  end

  #
  # A derivative of +Device+ that is a TAP device.
  #
  class TapDevice < Device

    def initialize(name)
      super(name, RbTunTap::DEV_TYPE_TAP, detect_tun_device_file)
    end

  end

end
