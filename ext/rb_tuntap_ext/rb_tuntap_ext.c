#include "ruby.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>

/**
 * Helpers (private)
 */

static int socket_ioctl(int cmd, struct ifreq* req) {

  int ret  = 0;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);

  if (sock < 0) {
    rb_raise(rb_eRuntimeError, "Failed to open socket for ioctl");
  }

  ret = ioctl(sock, cmd, req);
  close(sock);
  return ret;
}

static const char *ivar_to_cstr(VALUE self, const char *ivar_name) {
  VALUE ivar = rb_iv_get(self, ivar_name);
  return StringValueCStr(ivar);
}

/**
 * Methods
 */

static VALUE device_initialize(VALUE self,
			       VALUE name,
			       VALUE type,
			       VALUE dev) {

  int iftype = NUM2INT(type);

  Check_Type(name, T_STRING);
  Check_Type(dev , T_STRING);

  /* Validate name */
  if(RSTRING_LEN(name) >= IFNAMSIZ) {
    rb_raise(rb_eArgError, "Interface name too long");
  }

  /* Validate type */
  if((iftype != IFF_TUN) && (iftype != IFF_TAP)) {
    rb_raise(rb_eArgError, "Type must be either IFF_TUN or IFF_TAP");
  }

  /* Validate dev */
  /* Easier to do in Ruby e.g. File.exists?(dev) */

  rb_iv_set(self, "@name" , name);
  rb_iv_set(self, "@type" , type);
  rb_iv_set(self, "@dev"  , dev );
  rb_iv_set(self, "@fd"   , INT2NUM(-1));

  return self;
}

static VALUE device_open(VALUE self, VALUE no_pi) {

  int fd = -1;
  struct ifreq req;

  memset(&req, 0, sizeof(req));

  fd = open(ivar_to_cstr(self, "@dev"), O_RDWR);

  if(fd < 0) {
    rb_sys_fail("Failed to open device");
  }

  /* Setup the device */
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name")); /* set name */
  req.ifr_flags = NUM2INT(rb_iv_get(self, "@type")); /* set type */

  if(RTEST(no_pi)) {
    req.ifr_flags |= IFF_NO_PI;
  }

  if(ioctl(fd, TUNSETIFF, &req) < 0) {
    rb_sys_fail("ioctl failed on device");
  }

  /* Copy req.ifr_name into instance var, in case kernel overrode it */
  rb_iv_set(self, "@name", rb_str_new2(req.ifr_name));
  rb_iv_set(self, "@fd"  , INT2NUM(fd));

  return self;
}

static VALUE device_set_addr(VALUE self, VALUE addr) {

  struct ifreq req;
  struct sockaddr_in *sin = NULL;

  Check_Type(addr, T_STRING);

  memset(&req, 0, sizeof(req));
  sin = (struct sockaddr_in *) &req.ifr_addr;

  /* Populate the req object */
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));
  sin->sin_family = AF_INET;

  if(inet_aton(StringValueCStr(addr), &sin->sin_addr) == 0) {
    rb_raise(rb_eArgError, "Bad IP address");
  }

  if (socket_ioctl(SIOCSIFADDR, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  return addr;
}

static VALUE device_get_addr(VALUE self)
{
  const  char *caddr = NULL;
  struct ifreq req;

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));

  if (socket_ioctl(SIOCGIFADDR, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  caddr = inet_ntoa(((struct sockaddr_in*)&req.ifr_addr)->sin_addr);
  if (caddr == NULL) {
    rb_raise(rb_eRuntimeError, "Failed to retrieve address");
  }

  return rb_str_new2(caddr);
}

static VALUE device_set_dstaddr(VALUE self, VALUE dstaddr)
{
  struct ifreq req;
  struct sockaddr_in *sin      = NULL;
  const  char        *cdstaddr = RSTRING_PTR(dstaddr);

  Check_Type(dstaddr, T_STRING);

  memset(&req, 0, sizeof(req));
  sin = (struct sockaddr_in *) &req.ifr_dstaddr;

  /* Populate the req object */
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));
  sin->sin_family = AF_INET;

  if(inet_aton(cdstaddr, &sin->sin_addr) == 0) {
    rb_raise(rb_eArgError, "Bad IP address");
  }

  if (socket_ioctl(SIOCSIFDSTADDR, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  return dstaddr;
}

static VALUE device_get_dstaddr(VALUE self)
{
  const  char  *cdstaddr = NULL;
  struct ifreq req;

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));

  if (socket_ioctl(SIOCGIFDSTADDR, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  cdstaddr = inet_ntoa(((struct sockaddr_in*)&req.ifr_dstaddr)->sin_addr);
  if (cdstaddr == NULL) {
    rb_raise(rb_eRuntimeError, "Failed to retrieve address");
  }

  return rb_str_new2(cdstaddr);
}

static VALUE device_set_hwaddr(VALUE self, VALUE hwaddr) {
  struct ifreq req;

  Check_Type(hwaddr, T_STRING);

  if (RSTRING_LEN(hwaddr) != ETH_ALEN) {
    rb_raise(rb_eArgError, "Invalid MAC address");
  }

  memset(&req, 0, sizeof(req));
  req.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  strcpy(req.ifr_name, RSTRING_PTR(rb_iv_get(self, "@name")));
  memcpy(req.ifr_hwaddr.sa_data, RSTRING_PTR(hwaddr), RSTRING_LEN(hwaddr));

  if (socket_ioctl(SIOCSIFHWADDR, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  return hwaddr;
}

static VALUE device_get_hwaddr(VALUE self)
{
  struct ifreq req;

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));

  if (socket_ioctl(SIOCGIFHWADDR, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  return rb_str_new2(req.ifr_hwaddr.sa_data);
}

static VALUE device_set_netmask(VALUE self, VALUE netmask) {

  struct ifreq req;
  struct sockaddr_in* sin;

  Check_Type(netmask, T_STRING);

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));
  sin = (struct sockaddr_in*)&req.ifr_netmask;
  sin->sin_family = AF_INET;

  if (inet_aton(StringValueCStr(netmask), &sin->sin_addr) == 0) {
    rb_raise(rb_eArgError, "Bad netmask IP address");
  }

  if (socket_ioctl(SIOCSIFNETMASK, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  return netmask;
}

static VALUE device_get_netmask(VALUE self) {

  struct ifreq req;
  const  char *cnetmask = NULL;

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));

  if (socket_ioctl(SIOCGIFNETMASK, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  cnetmask = inet_ntoa(((struct sockaddr_in*) &req.ifr_netmask)->sin_addr);
  if (cnetmask == NULL) {
    rb_raise(rb_eRuntimeError, "Failed to retrieve netmask");
  }

  return rb_str_new2(cnetmask);
}

static VALUE device_close(VALUE self) {
  int fd = NUM2INT(rb_iv_get(self, "@fd"));

  if(fd > 0) {
    close(fd);
  }

  rb_iv_set(self, "@fd", INT2NUM(-1));

  return Qtrue;
}

static VALUE device_set_mtu(VALUE self, VALUE mtu) {

  struct ifreq req;
  int          cmtu;

  Check_Type(mtu, T_FIXNUM);

  cmtu = FIX2INT(mtu);
  if (mtu <= 0) {
    rb_raise(rb_eArgError, "Bad MTU, should be > 0");
  }

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));
  req.ifr_mtu = cmtu;

  if (socket_ioctl(SIOCSIFMTU, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  return mtu;
}

static VALUE device_get_mtu(VALUE self) {
  struct ifreq req;

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));

  if (socket_ioctl(SIOCGIFMTU, &req) < 0) {
    rb_sys_fail("Failed to call ioctl on socket");
  }

  return INT2NUM(req.ifr_mtu);
}

static VALUE device_up(VALUE self) {
  struct ifreq req;

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));

  if (socket_ioctl(SIOCGIFFLAGS, &req) < 0) {
    rb_sys_fail("Failed to call ioctl to get status");
  }

  if (!(req.ifr_flags & IFF_UP)) {
    req.ifr_flags |= IFF_UP;
    if (socket_ioctl(SIOCSIFFLAGS, &req) < 0) {
      rb_sys_fail("Failed to call ioctl to set device state");
    }
  }

  return Qtrue;
}

static VALUE device_down(VALUE self) {
  struct ifreq req;

  memset(&req, 0, sizeof(req));
  strcpy(req.ifr_name, ivar_to_cstr(self, "@name"));

  if (socket_ioctl(SIOCGIFFLAGS, &req) < 0) {
    rb_sys_fail("Failed to call ioctl to get status");
  }

  if (req.ifr_flags & IFF_UP) {
    req.ifr_flags &= ~IFF_UP;
    if (socket_ioctl(SIOCSIFFLAGS, &req) < 0) {
      rb_sys_fail("Failed to call ioctl to set device state");
    }
  }

  return Qtrue;
}

static VALUE device_persist(VALUE self, VALUE persist) {

  int cpersist = 0;
  int ret      = 0;
  int fd       = 0;

  fd = NUM2INT(rb_iv_get(self, "@fd"));
  if (fd < 0) {
    rb_raise(rb_eRuntimeError, "Device has not been opened yet");
  }

  switch(TYPE(persist)) {
  case T_TRUE : cpersist = 1 ; break ;
  case T_FALSE: cpersist = 0 ; break ;
  case T_NIL  : cpersist = 0 ; break ;
  default     : cpersist = 0 ; break ;
  }

  ret = ioctl(fd, TUNSETPERSIST, cpersist);
  if (ret < 0) {
    rb_sys_fail("Failed to call ioctl to set persistenc value");
  }

  return Qtrue;
}

/**
 * Ruby
 */

void Init_rb_tuntap_ext() {
  /* module TunTap */
  VALUE c_tuntap = rb_define_module("RbTunTap");

  /* Constants */
  rb_define_const(c_tuntap, "DEV_TYPE_TUN", INT2NUM(IFF_TUN));
  rb_define_const(c_tuntap, "DEV_TYPE_TAP", INT2NUM(IFF_TAP));

  /* class TunTap::Device */
  VALUE c_device = rb_define_class_under(c_tuntap, "Device", rb_cObject);

  rb_define_method(c_device, "initialize"  , device_initialize  , 3);
  rb_define_method(c_device, "open"        , device_open        , 1);
  rb_define_method(c_device, "close"       , device_close       , 0);

  rb_define_method(c_device, "set_addr"    , device_set_addr    , 1);
  rb_define_method(c_device, "get_addr"    , device_get_addr    , 0);

  rb_define_method(c_device, "set_dstaddr" , device_set_dstaddr , 1);
  rb_define_method(c_device, "get_dstaddr" , device_get_dstaddr , 0);

  rb_define_method(c_device, "set_netmask" , device_set_netmask , 1);
  rb_define_method(c_device, "get_netmask" , device_get_netmask , 0);

  rb_define_method(c_device, "set_hwaddr"  , device_set_hwaddr  , 1);
  rb_define_method(c_device, "get_hwaddr"  , device_get_hwaddr  , 0);

  rb_define_method(c_device, "set_mtu"     , device_set_mtu     , 1);
  rb_define_method(c_device, "get_mtu"     , device_get_mtu     , 0);

  rb_define_method(c_device, "set_netmask" , device_set_netmask , 1);
  rb_define_method(c_device, "get_netmask" , device_get_netmask , 0);

  rb_define_method(c_device, "up"          , device_up          , 0);
  rb_define_method(c_device, "down"        , device_down        , 0);

  rb_define_method(c_device, "persist"     , device_persist     , 0);

  rb_define_attr(c_device, "fd"  , 1, 0);
  rb_define_attr(c_device, "name", 1, 0);
}
