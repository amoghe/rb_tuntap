require 'mkmf'

abort "Missing if_tun.h" unless have_header "linux/if_tun.h"

modulename = "rb_tuntap_ext"

dir_config      modulename
create_makefile modulename
