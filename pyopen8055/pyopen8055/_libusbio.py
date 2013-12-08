import ctypes
import math
import pyopen8055
import pylibusb as usb
import re

def _debug(*args):
    print 'DEBUG:', args
    pass

usb.init()

class _usbio:
    """
    pylibusb based IO module for pyopen8055.
    """

    ##########
    # _usbio()
    ##########
    def __init__(self, card_address):
        """
        Open a local 8055 type card via pylibusb.
        """
        self.card_address = card_address
        _debug('_libusb_io.__init__("%s")' % card_address)
        m = re.match('^card([0-9]+)$', card_address)
        if not m:
            raise ValueError("invalid card address '%s'" % card_address)
        self.card_number = int(m.groups()[0])
        _debug('  card_number = %d' % self.card_number)

        # ----
        # Find the actual device
        # ----
        busses = usb.get_busses()
        if not busses:
            usb.find_busses()
            usb.find_devices()
            busses = usb.get_busses()
        found = False
        for bus in busses:
            for dev in bus.devices:
                _debug('  idVendor: 0x%04x idProduct 0x%04x' % (dev.descriptor.idVendor, dev.descriptor.idProduct))
                if (dev.descriptor.idVendor == 0x10cf and
                    dev.descriptor.idProduct == 0x5500 + self.card_number):
                    found = True
                    break
            if found:
                break
        if not found:
            raise RuntimeError("'%s' not found" % card_address)
        _debug("  found device '%s'" % dev)

        # ----
        # Open the device and detach an eventually existing kernel driver.
        # ----
        self.handle = usb.open(dev)
        if hasattr(usb, 'get_driver_np'):
            name = usb.get_driver_np(self.handle, 0)
            if name != '':
                _debug("  attached to kernel driver '%s' - detaching" % name)
                usb.detach_kernel_driver_np(self.handle, 0)

        # ----
        # Set the active configuration. The K8055/K8055N has only one, so
        # the existence of multiple configurations identifies the Open8055.
        # ----
        _debug("  bNumConfigurations = %d" % dev.descriptor.bNumConfigurations)
        if False and dev.descriptor.bNumConfigurations > 1:
            config = dev.config[1]
            self.card_type = pyopen8055.OPEN8055
        else:
            config = dev.config[0]
            self.card_type = pyopen8055.K8055
        _debug("  bConfigurationValue = %d" % config.bConfigurationValue)
        usb.set_configuration(self.handle, config.bConfigurationValue)

        _debug("  %s: card_type = %s" % (card_address, self.card_type))

    def close(self):
        """
        Close the connection to this card.
        """
        usb.release_interface(self.handle, 0)
        usb.close(self.handle)
        self.handle = None
        
    def send_pkt(self, buffer):
        rc = usb.interrupt_write(self.handle, 0x01, buffer, 0)
        if rc != len(buffer):
            raise RuntimeError(
                    'interrupt_write returned %d - expected %d' % 
                    (rc, len(buffer)))

    def recv_pkt(self, length):
        buffer = ctypes.create_string_buffer(length)
        rc = usb.interrupt_read(self.handle, 0x81, buffer, 0)
        if rc != length:
            raise RuntimeError(
                    'interrupt_write returned %d - expected %d' % 
                    (rc, length))
        return buffer