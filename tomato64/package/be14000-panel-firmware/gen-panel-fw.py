#!/usr/bin/env python3
#
# Build the panel-mipi-dbi init blob for the GL.iNet GL-BE14000 front panel.
#
# panel-mipi-dbi does not know any panel: it resets the controller and replays
# a command sequence read from a firmware file, named after the first
# compatible string of the panel node. Our DTS says
# "glinet,gl-be14000-panel", so the file is
#
#     /lib/firmware/glinet,gl-be14000-panel.bin
#
# File format (drivers/gpu/drm/tiny/panel-mipi-dbi.c):
#
#     magic   15 bytes  "MIPI DBI" followed by seven NULs
#     version  1 byte   0x01
#     commands           command, num_parameters, [parameter, ...], ...
#
# A pause is the NOP command (0x00) with one parameter, the delay in ms.
#
# The sequence below is the vendor ST7789P3 init_display() from GL's staging
# fbtft driver, which is what the panel ran on before this port moved it to
# DRM. Derived from package/firmware/glinet-panel-firmware/gen-panel-fw.py in
# JiaY-shi/openwrt branch flint4-support (John Crispin), reduced to the one
# board we build.
#
# MADCTL 0x60 = MV|MX. MV puts the controller in landscape, so the physically
# 240x320 panel presents as 320x240, which is the orientation the DTS
# panel-timing and the UI are written for. The BE10000 carries the same panel
# mounted 180 degrees the other way and uses 0xA0 (MV|MY) instead.

import os

MADCTL = 0x60

# ('cmd', [params]) or ('delay', milliseconds)
SEQUENCE = [
    (0x11, []),                                 # SLPOUT
    ('delay', 120),
    (0x36, [MADCTL]),                           # MADCTL
    (0x3A, [0x05]),                             # COLMOD, 16bpp RGB565
    (0xB2, [0x05, 0x05, 0x00, 0x33, 0x33]),     # PORCTRL
    (0xB7, [0x35]),                             # GCTRL
    (0xBB, [0x21]),                             # VCOMS
    (0xC0, [0x2C]),                             # LCMCTRL
    (0xC2, [0x01]),                             # VDVVRHEN
    (0xC3, [0x0B]),                             # VRHS
    (0xC4, [0x20]),                             # VDVS
    (0xC6, [0x0A]),                             # FRCTRL2
    (0xD0, [0xA7, 0xA1]),                       # PWCTRL1
    (0xD0, [0xA4, 0xA1]),                       # PWCTRL1
    (0x35, [0x00]),                             # TEON
    (0xD6, [0xA1]),                             # vendor magic
    (0xE0, [0xD0, 0x04, 0x08, 0x0A, 0x09, 0x05, 0x2D,
            0x43, 0x49, 0x09, 0x16, 0x15, 0x26, 0x2B]),     # PVGAMCTRL
    (0xE1, [0xD0, 0x03, 0x09, 0x0A, 0x0A, 0x06, 0x2E,
            0x44, 0x40, 0x3A, 0x15, 0x15, 0x26, 0x2A]),     # NVGAMCTRL
    (0x21, []),                                 # INVON
    ('delay', 10),
    (0x29, []),                                 # DISPON
    ('delay', 120),
]

NAME = 'glinet,gl-be14000-panel.bin'


def build():
    out = bytearray(b'MIPI DBI' + b'\x00' * 7 + b'\x01')

    for cmd, params in SEQUENCE:
        if cmd == 'delay':
            out += bytes([0x00, 0x01, params & 0xff])
        else:
            out += bytes([cmd, len(params)] + params)

    return bytes(out)


if __name__ == '__main__':
    blob = build()
    dst = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'files', NAME)
    with open(dst, 'wb') as f:
        f.write(blob)
    print('wrote %s, %d bytes, MADCTL 0x%02x' % (dst, len(blob), MADCTL))
