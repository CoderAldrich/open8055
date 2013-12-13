"""
pyopen8055.py - IO module for the whole 8055 card family.
"""

import ctypes
import math
import re
import struct
import sys

def _debug(*args):
    print 'DEBUG:', args
    pass

if sys.platform.startswith('freebsd'):
    import _libusbio as usbio
    import _netio as netio
elif sys.platform.startswith('linux'):
    import _libusbio as usbio
    import _netio as netio
elif sys.platform.startswith('win'):
    import _winusbio as usbio
    import _netio as netio
else:
    raise RuntimeError('platform %s not supported yet' % sys.platform)

##########
# Supported card types
##########
K8055 = 'K8055'
K8055N = 'K8055N'
OPEN8055 = 'OPEN8055'

K8055_VID = 0x10cf
K8055_PID = 0x5500
OPEN8055_PID = 0x55f0

##########
# K8055 message types
##########
TAG_K8055_SET_DEBOUNCE_1 = 1
TAG_K8055_SET_DEBOUNCE_2 = 2
TAG_K8055_RESET_COUNTER_1 = 3
TAG_K8055_RESET_COUNTER_2 = 4
TAG_K8055_SET_OUTPUT = 5

##########
# K8055N message types
##########
TAG_K8055N_SET_PROTO = 6
TAG_K8055N_SET_DEBOUNCE_1 = 11
TAG_K8055N_SET_DEBOUNCE_2 = 12
TAG_K8055N_RESET_COUNTER_1 = 13
TAG_K8055N_RESET_COUNTER_2 = 14
TAG_K8055N_SET_OUTPUT = 15
TAG_K8055N_GET_DIGITAL_IN = 16
TAG_K8055N_GET_ANALOG_IN = 17
TAG_K8055N_GET_COUNTER_1 = 18
TAG_K8055N_GET_COUNTER_2 = 19
TAG_K8055N_GET_COUNTER16 = 20      # 21 and 21
TAG_K8055N_GET_DIGITAL_OUT = 24
TAG_K8055N_GET_ANALOG_OUT = 25

##########
# Open8055 message types
##########
TAG_OPEN8055_OUTPUT = 0x01
TAG_OPEN8055_GETINPUT = 0x02
TAG_OPEN8055_SETCONFIG1 = 0x03
TAG_OPEN8055_GETCONFIG = 0x04
TAG_OPEN8055_SAVECONFIG = 0x05
TAG_OPEN8055_SAVEALL = 0x06
TAG_OPEN8055_RESET = 0x7f
TAG_OPEN8055_INPUT = 0x81

##########
# K8055 HID report
##########
class k8055_hid_report:
    def __init__(self):
        self.digital_in = 0
        self.card_address = 0
        self.analog_in = [0, 0]
        self.counter = [0, 0]
    
    def get_binary_data(self):
        return struct.pack("BBBBHH", self.digital_in, self.card_address,
                self.analog_in[0], self.analog_in[1],
                self.counter[0], self.counter[1])
        
    def set_binary_data(self, data):
        (self.digital_in, self.card_address,
                self.analog_in[0], self.analog_in[1],
                self.counter[0], self.counter[1]
            ) = struct.unpack("BBBBHH", data)

##########
# K8055 HID command
##########
class k8055_hid_command:
    def __init__(self):
        self.command_tag = 0
        self.digital_out = 0
        self.analog_out = [0, 0]
        self.debounce = [0, 0]

    def get_binary_data(self):
        return struct.pack("BBBBxxBB", self.command_tag, self.digital_out,
                self.analog_out[0], self.analog_out[1],
                self.debounce[0], self.debounce[1])

    def set_binary_data(self, data):
        (self.command_tag, self.digital_out,
                self.analog_out[0], self.analog_out[1],
                self.debounce[0], self.debounce[1]
            ) = struct.unpack("BBBBxxBB", data)

##########
# Open8055 input report
##########
class open8055_hid_input:
    def __init__(self):
        self.msg_type = 0
        self.input_bits = 0
        self.input_counter = [0, 0, 0, 0, 0]
        self.input_adc = [0, 0]

    def get_binary_data(self):
        return struct.pack("!BB5H2H16x",
                self.msg_type, self.input_bits,
                self.input_counter[0], self.input_counter[1],
                self.input_counter[2], self.input_counter[3],
                self.input_counter[4],
                self.input_adc[0], self.input_adc[1])

    def set_binary_data(self, data):
        (self.msg_type, self.input_bits,
                self.input_counter[0], self.input_counter[1],
                self.input_counter[2], self.input_counter[3],
                self.input_counter[4],
                self.input_adc[0], self.input_adc[1]
            ) = struct.unpack("!BB5H2H16x", data)

##########
# Open8055 output command
##########
class open8055_hid_output:
    def __init__(self):
        self.msg_type = 0
        self.output_bits = 0
        self.output_value = [0, 0, 0, 0, 0, 0, 0, 0]
        self.output_pwm = [0, 0]
        self.reset_counter = 0

    def get_binary_data(self):
        return struct.pack("!BB8H2HB9x",
                self.msg_type, self.output_bits,
                self.output_value[0], self.output_value[1],
                self.output_value[2], self.output_value[3],
                self.output_value[4], self.output_value[5],
                self.output_value[7], self.output_value[7],
                self.output_pwm[0], self.output_pwm[1],
                self.reset_counter)

    def set_binary_data(self, data):
        (self.msg_type, self.output_bits,
                self.output_value[0], self.output_value[1],
                self.output_value[2], self.output_value[3],
                self.output_value[4], self.output_value[5],
                self.output_value[7], self.output_value[7],
                self.output_pwm[0], self.output_pwm[1],
                self.reset_counter
            ) = struct.unpack("!BB8H2HB9x", data)

##########
# Open8055 config1 record
##########
class open8055_hid_config1:
    def __init__(self):
        self.msg_type = 0
        self.mode_adc = [0, 0]
        self.mode_input = [0, 0, 0, 0, 0]
        self.mode_output = [0, 0, 0, 0, 0, 0, 0, 0]
        self.mode_pwm = [0, 0]
        self.debounce = [0, 0, 0, 0, 0]
        self.card_address = 0

    def get_binary_data(self):
        return struct.pack("!B2B5B8B2B5HB3x",
                self.msg_type,
                self.mode_adc[0], self.mode_adc[1],
                self.mode_input[0], self.mode_input[1],
                self.mode_input[2], self.mode_input[3],
                self.mode_input[4],
                self.mode_output[0], self.mode_output[1],
                self.mode_output[2], self.mode_output[3],
                self.mode_output[4], self.mode_output[5],
                self.mode_output[6], self.mode_output[7],
                self.mode_pwm[0], self.mode_pwm[1],
                self.debounce[0], self.debounce[1],
                self.debounce[2], self.debounce[3],
                self.debounce[4],
                self.card_address)

    def set_binary_data(self, data):
        (self.msg_type,
                self.mode_adc[0], self.mode_adc[1],
                self.mode_input[0], self.mode_input[1],
                self.mode_input[2], self.mode_input[3],
                self.mode_input[4],
                self.mode_output[0], self.mode_output[1],
                self.mode_output[2], self.mode_output[3],
                self.mode_output[4], self.mode_output[5],
                self.mode_output[6], self.mode_output[7],
                self.mode_pwm[0], self.mode_pwm[1],
                self.debounce[0], self.debounce[1],
                self.debounce[2], self.debounce[3],
                self.debounce[4],
                self.card_address
            ) = struct.unpack("!B2B5B8B2B5HB3x", data)
        pass

############################################################
# pyopen8055
############################################################
class pyopen8055:
    """
    Implements a connection to a physically attached K8055(N)/VM110(N)
    Experimental USB Interface Card. 
    """

    ##########
    # pyopen8055 - Creating a new card instance.
    ##########
    def __init__(self, card_address, autosend = True, autorecv = True):
        """
        Open a K8055, K8055N or Open8055 board.
        """
        _debug('pyopen8055.__init__("%s")' % card_address)
        self.io = None
        if re.match('^card[0-9+]$', card_address):
            self.io = usbio._usbio(card_address)
        else:
            self.io = netio._netio(card_address)

        self.autosend = autosend
        self.autorecv = autorecv

        self.card_type = self.io.card_type
        if self.card_type == OPEN8055:
            self.config1 = open8055_hid_config1()
            self.send_buffer = open8055_hid_output()
            self.recv_buffer = open8055_hid_input()
            self.readback_all()
        elif self.card_type == K8055N:
            self.recv_buffer = k8055_hid_report()
            self.send_buffer = k8055_hid_command()
            self.counter = [0, 0]
            self.readback_digital_all()
            self.readback_analog_all()
        elif self.card_type == K8055:
            self.recv_buffer = k8055_hid_report()
            self.send_buffer = k8055_hid_command()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # __del__()
    ##########
    def __del__(self):
        """
        Cleanup on object destruction.
        """
        self.close()

    ##########
    # close()
    ##########
    def close(self):
        """
        Close the connection to this K8055(N) and restore the kernel
        device driver (if there was one on open).
        """
        if self.io is not None:
            self.io.close()
        self.io = None

    ##########
    # set_digital_all()
    ##########
    def set_digital_all(self, value):
        if self.card_type == OPEN8055:
            self.send_buffer.msg_type = TAG_OPEN8055_OUTPUT
            self.send_buffer.output_bits = value & 0xff
            self._autosend()
        elif self.card_type == K8055N:
            self.send_buffer.command_tag = TAG_K8055N_SET_OUTPUT
            self.send_buffer.digital_out = value & 0xff
            self._autosend()
        elif self.card_type == K8055:
            self.send_buffer.command_tag = TAG_K8055_SET_OUTPUT
            self.send_buffer.digital_out = value & 0xff
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # set_digital_port()
    ##########
    def set_digital_port(self, port, value):
        if self.card_type == OPEN8055:
            if port < 0 or port > 7:
                raise ValueError('invalid port number %d' % port)
            self.send_buffer.command_tag = TAG_OPEN8055_OUTPUT
            if bool(value):
                self.send_buffer.output_bits |= (1 << port)
            else:
                self.send_buffer.output_bits &= ~(1 << port)
            self._autosend()
        elif self.card_type == K8055N:
            if port < 0 or port > 7:
                raise ValueError('invalid port number %d' % port)
            self.send_buffer.command_tag = TAG_K8055N_SET_OUTPUT
            if bool(value):
                self.send_buffer.digital_out |= (1 << port)
            else:
                self.send_buffer.digital_out &= ~(1 << port)
            self._autosend()
        elif self.card_type == K8055:
            if port < 0 or port > 7:
                raise ValueError('invalid port number %d' % port)
            self.send_buffer.command_tag = TAG_K8055_SET_OUTPUT
            if bool(value):
                self.send_buffer.digital_out |= (1 << port)
            else:
                self.send_buffer.digital_out &= ~(1 << port)
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # set_analog_all()
    ##########
    def set_analog_all(self, value1, value2):
        if self.card_type == OPEN8055:
            self.send_buffer.msg_type = TAG_OPEN8055_OUTPUT
            self.send_buffer.output_pwm[0] = value1
            self.send_buffer.output_pwm[1] = value2
            self._autosend()
        elif self.card_type == K8055N:
            self.send_buffer.command_tag = TAG_K8055N_SET_OUTPUT
            self.send_buffer.analog_out[0] = value1
            self.send_buffer.analog_out[1] = value2
            self._autosend()
        elif self.card_type == K8055:
            self.send_buffer.command_tag = TAG_K8055_SET_OUTPUT
            self.send_buffer.analog_out[0] = value1
            self.send_buffer.analog_out[1] = value2
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # set_analog_port()
    ##########
    def set_analog_port(self, port, value):
        if self.card_type == OPEN8055:
            if port == 0:
                self.set_analog_all(value, self.send_buffer.output_pwm[1])
            elif port == 1:
                self.set_analog_all(self.send_buffer.output_pwm[0], value)
            else:
                raise ValueError('invalid port number %d' % port)
        elif self.card_type == K8055N or self.card_type == K8055:
            if port == 0:
                self.set_analog_all(value, self.send_buffer.analog_out[1])
            elif port == 1:
                self.set_analog_all(self.send_buffer.analog_out[0], value)
            else:
                raise ValueError('invalid port number %d' % port)
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # set_counter_debounce_time()
    ##########
    def set_counter_debounce_time(self, port, value):
        if self.card_type == OPEN8055:
            if port < 0 or port > 4:
                raise ValueError('invalid port number %d' % port)
            if value < 1.0 or value > 5000.0:
                raise ValueError('invalid debounce time %f' % value)
            self.config1.msg_type = TAG_OPEN8055_SETCONFIG1
            self.config1.debounce[port] = float(value) * 10.0
            self.io.send_pkt(self.config1.get_binary_data())
        elif self.card_type == K8055N:
            if port < 0 or port > 1:
                raise ValueError('invalid port number %d' % port)
            if value < 1.0 or value > 5000.0:
                raise ValueError('invalid debounce time %f' % value)
            binval = int(round(2.20869 * math.pow(value, 0.5328)))
            if binval < 1:
                binval = 1
            if binval > 255:
                binval = 255

            if port == 0:
                self.send_buffer.command_tag = TAG_K8055N_SET_DEBOUNCE_1
                self.send_buffer.debounce[0] = binval
            else:
                self.send_buffer.command_tag = TAG_K8055N_SET_DEBOUNCE_2
                self.send_buffer.debounce[1] = binval
            self._autosend()
        elif self.card_type == K8055:
            if port < 0 or port > 1:
                raise ValueError('invalid port number %d' % port)
            if value < 1.0 or value > 5000.0:
                raise ValueError('invalid debounce time %f' % value)
            binval = int(round(2.20869 * math.pow(value, 0.5328)))
            if binval < 1:
                binval = 1
            if binval > 255:
                binval = 255

            if port == 0:
                self.send_buffer.command_tag = TAG_K8055_SET_DEBOUNCE_1
                self.send_buffer.debounce[0] = binval
            else:
                self.send_buffer.command_tag = TAG_K8055_SET_DEBOUNCE_2
                self.send_buffer.debounce[1] = binval
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # reset_counter()
    ##########
    def reset_counter(self, port):
        if self.card_type == OPEN8055:
            if port < 0 or port > 4:
                raise ValueError('invalid port number %d' % port)
            self.send_buffer.msg_type = TAG_OPEN8055_OUTPUT
            self.send_buffer.reset_counter |= (1 << port)
            self._autosend()
        elif self.card_type == K8055N:
            if port == 0:
                self.send_buffer.command_tag = TAG_K8055N_RESET_COUNTER_1
            elif port == 1:
                self.send_buffer.command_tag = TAG_K8055N_RESET_COUNTER_2
            else:
                raise ValueError('invalid port number %d' % port)
            self.io.send_pkt(self.send_buffer.get_binary_data())
        elif self.card_type == K8055:
            if port == 0:
                self.send_buffer.command_tag = TAG_K8055_RESET_COUNTER_1
            elif port == 1:
                self.send_buffer.command_tag = TAG_K8055_RESET_COUNTER_2
            else:
                raise ValueError('invalid port number %d' % port)
            self.io.send_pkt(self.send_buffer.get_binary_data())
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # read_digital_all()
    ##########
    def read_digital_all(self):
        if self.card_type == OPEN8055:
            self._autorecv(tag = TAG_OPEN8055_GETINPUT)
            return self.recv_buffer.input_bits
        elif self.card_type == K8055N:
            self._autorecv(tag = TAG_K8055N_GET_DIGITAL_IN)
            value = self.recv_buffer.digital_in
            return (((value & 0x10) >> 4) | ((value & 0x20) >> 4) |
                    ((value & 0x01) << 2) | ((value & 0x40) >> 3) |
                    ((value & 0x80) >> 3))
        elif self.card_type == K8055:
            self._autorecv()
            value = self.recv_buffer.digital_in
            return (((value & 0x10) >> 4) | ((value & 0x20) >> 4) |
                    ((value & 0x01) << 2) | ((value & 0x40) >> 3) |
                    ((value & 0x80) >> 3))
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # read_digital_port()
    ##########
    def read_digital_port(self, port):
        if self.card_type == OPEN8055:
            if port < 0 or port > 5:
                raise ValueError('invalid port number %d' % port)
            return bool(self.read_digital_all() & (1 << port))
        elif self.card_type == K8055N:
            if port < 0 or port > 5:
                raise ValueError('invalid port number %d' % port)
            return bool(self.read_digital_all() & (1 << port))
        elif self.card_type == K8055:
            if port < 0 or port > 5:
                raise ValueError('invalid port number %d' % port)
            return bool(self.read_digital_all() & (1 << port))
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # read_counter()
    ##########
    def read_counter(self, port):
        if self.card_type == OPEN8055:
            if port < 0 or port > 4:
                raise ValueError('invalid port number %d' % port)
            self._autorecv(tag = TAG_OPEN8055_GETINPUT)
            return self.recv_buffer.input_counter[port]
        elif self.card_type == K8055N:
            if port == 0:
                self._autorecv(tag = TAG_K8055N_GET_COUNTER_1)
                return self.counter[0]
            elif port == 1:
                self._autorecv(tag = TAG_K8055N_GET_COUNTER_2)
                return self.counter[1]
            else:
                raise ValueError('invalid port number %d' % port)
        elif self.card_type == K8055:
            if port == 0:
                self._autorecv()
                return self.recv_buffer.counter[0]
            elif port == 1:
                self._autorecv()
                return self.recv_buffer.counter[1]
            else:
                raise ValueError('invalid port number %d' % port)
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # read_counter16()
    ##########
    def read_counter16(self, port):
        return self.read_counter(port) & 0xffff

    ##########
    # read_analog_all()
    ##########
    def read_analog_all(self):
        if self.card_type == OPEN8055:
            self._autorecv(tag = TAG_OPEN8055_GETINPUT)
            return (self.recv_buffer.input_adc[0],
                    self.recv_buffer.input_adc[1])
        elif self.card_type == K8055N:
            self._autorecv(tag = TAG_K8055N_GET_ANALOG_IN)
            return (self.recv_buffer.analog_in[0],
                    self.recv_buffer.analog_in[1])
        elif self.card_type == K8055:
            self._autorecv()
            return (self.recv_buffer.analog_in[0],
                    self.recv_buffer.analog_in[1])
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # read_analog_port()
    ##########
    def read_analog_port(self, port):
        if self.card_type == OPEN8055:
            if port == 0:
                self._autorecv(tag = TAG_OPEN8055_GETINPUT)
                return self.recv_buffer.input_adc[0]
            elif port == 1:
                self._autorecv(tag = TAG_OPEN8055_GETINPUT)
                return self.recv_buffer.input_adc[1]
            else:
                raise ValueError('invalid port number %d' % port)
        elif self.card_type == K8055N:
            if port == 0:
                self._autorecv(tag = TAG_K8055N_GET_ANALOG_IN)
                return self.recv_buffer.analog_in[0]
            elif port == 1:
                self._autorecv(tag = TAG_K8055N_GET_ANALOG_IN)
                return self.recv_buffer.analog_in[1]
            else:
                raise ValueError('invalid port number %d' % port)
        elif self.card_type == K8055:
            if port == 0:
                self._autorecv()
                return self.recv_buffer.analog_in[0]
            elif port == 1:
                self._autorecv()
                return self.recv_buffer.analog_in[1]
            else:
                raise ValueError('invalid port number %d' % port)
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # readback_digital_all()
    ##########
    def readback_digital_all(self):
        if self.card_type == OPEN8055:
            return self.send_buffer.output_bits
        elif self.card_type == K8055N:
            if self.autorecv:
                self.send_buffer.command_tag = TAG_K8055N_GET_DIGITAL_OUT
                self.io.send_pkt(self.send_buffer.get_binary_data())
                buf = self.io.recv_pkt(8)
                self.send_buffer.digital_out = ord(buf[4])
            return self.send_buffer.digital_out
        elif self.card_type == K8055:
            return self.send_buffer.digital_out
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # readback_analog_all()
    ##########
    def readback_analog_all(self):
        if self.card_type == OPEN8055:
            return self.send_buffer.output_pwm
        elif self.card_type == K8055N:
            if self.autorecv:
                self.send_buffer.command_tag = TAG_K8055N_GET_ANALOG_OUT
                self.io.send_pkt(self.send_buffer.get_binary_data())
                buf = self.io.recv_pkt(8)
                self.send_buffer.analog_out[0] = ord(buf[5])
                self.send_buffer.analog_out[1] = ord(buf[6])
            return (self.send_buffer.analog_out[0], 
                    self.send_buffer.analog_out[1])
        elif self.card_type == K8055:
            return (self.send_buffer.analog_out[0], 
                    self.send_buffer.analog_out[1])
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # readback_all()
    ##########
    def readback_all(self):
        if self.card_type == OPEN8055:
            self.send_buffer.msg_type = TAG_OPEN8055_GETCONFIG
            self.io.send_pkt(self.send_buffer.get_binary_data())
            have_config1 = False
            have_output = False
            have_input = False
            while not have_config1 or not have_output or not have_input:
                buf = self.io.recv_pkt(32)
                tag = ord(buf[0])
                if tag == TAG_OPEN8055_SETCONFIG1:
                    self.config1.set_binary_data(buf)
                    have_config1 = True
                elif tag == TAG_OPEN8055_OUTPUT:
                    self.send_buffer.set_binary_data(buf)
                    have_output = True
                elif tag == TAG_OPEN8055_INPUT:
                    self.recv_buffer.set_binary_data(buf)
                    have_input = True
                else:
                    raise RuntimeError("unexpected HID message type 0x%02x"
                            % tag)
        elif self.card_type == K8055 or self.card_type == K8055N:
            self.readback_digital_all()
            self.readback_analog_all()

    ##########
    # recv()
    ##########
    def recv(self):
        if self.card_type == K8055:
            self.recv_buffer.set_binary_data(self.io.recv_pkt(8))

    ##########
    # send()
    ##########
    def send(self):
        if self.card_type == K8055:
            self.io.send_pkt(self.send_buffer.get_binary_data())
        
    ############################################################
    # Private functions
    ############################################################

    def _autorecv(self, tag = None):
        if self.autorecv:
            if self.card_type == OPEN8055:
                self.send_buffer.msg_type = tag
                self.io.send_pkt(self.send_buffer.get_binary_data())
                self.recv_buffer.set_binary_data(self.io.recv_pkt(32))
            elif self.card_type == K8055N:
                self.send_buffer.command_tag = tag
                self.io.send_pkt(self.send_buffer.get_binary_data())
                buf = self.io.recv_pkt(8)
                if tag == TAG_K8055N_GET_DIGITAL_IN:
                    self.recv_buffer.digital_in = ord(buf[0])
                elif tag == TAG_K8055N_GET_ANALOG_IN:
                    self.recv_buffer.analog_in[0] = ord(buf[2])
                    self.recv_buffer.analog_in[1] = ord(buf[3])
                elif tag == TAG_K8055N_GET_COUNTER_1:
                    self.counter[0] = (
                            (ord(buf[4])) |
                            (ord(buf[5])) << 8 |
                            (ord(buf[6])) << 16 |
                            (ord(buf[7])) << 24)
                elif tag == TAG_K8055N_GET_COUNTER_2:
                    self.counter[1] = (
                            (ord(buf[4])) |
                            (ord(buf[5])) << 8 |
                            (ord(buf[6])) << 16 |
                            (ord(buf[7])) << 24)
            elif self.card_type == K8055:
                self.recv_buffer.set_binary_data(self.io.recv_pkt(8))

    def _autosend(self):
        if self.autosend:
            if self.card_type == OPEN8055:
                self.io.send_pkt(self.send_buffer.get_binary_data())
                self.send_buffer.reset_counter = 0
            elif self.card_type == K8055N or self.card_type == K8055:
                self.io.send_pkt(self.send_buffer.get_binary_data())

    def _dump_pkt(self, pkt):
        res = []
        for i in range(0, len(pkt)):
            res.append('{0:02x}'.format(ord(pkt[i])))
        return ' '.join(res)

