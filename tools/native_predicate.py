"""Execute the game's native reach predicate offline; never runs or edits the game.

Only the world-transform helper and failure-message FName constructor are stubbed.
The distance arithmetic, pattern dispatch, speed limits and arrival test are the
original x64 instructions from the installed executable.
"""
import hashlib
import os
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import UC_X86_REG_RAX, UC_X86_REG_RCX, UC_X86_REG_RDX, UC_X86_REG_RIP, UC_X86_REG_RSP

import pefile

class Image:
    def __init__(self):
        default_path = Path.home() / '.local/share/Steam/steamapps/common/Alchemy Factory/AlchemyFactory/Binaries/Win64/AlchemyFactory-Win64-Shipping.exe'
        path = Path(os.environ.get('FOUNTAINCONTROL_GAME_EXE', default_path))
        if hashlib.sha256(path.read_bytes()).hexdigest()!='66618d029654d125723c95e8e62919f267973ad881e24abff3b80e0b355a52f8':
            raise RuntimeError('Native comparison requires the supported 1.0.4963 executable')
        self.pe=pefile.PE(str(path),fast_load=True)
        self.base=self.pe.OPTIONAL_HEADER.ImageBase
    def read(self,address,length):
        return self.pe.get_data(address-self.base,length)

ENTRY = 0x144A1E1D0
TRANSFORM = 0x144A1C6C0
FNAME = 0x141330710
FACILITY, INDICATOR, TARGET = 0x200000, 0x201000, 0x202000
STACK, STOP = 0x400000, 0x300000


class NativePredicate:
    def __init__(self, pattern=2, min_speed=0, max_speed=700, speed=500):
        self.image = Image()
        self.uc = Uc(UC_ARCH_X86, UC_MODE_64)
        self.uc.mem_map(FACILITY, 0x10000)
        self.uc.mem_map(STOP, 0x1000)
        self.uc.mem_map(STACK, 0x10000)
        self.uc.hook_add(UC_HOOK_MEM_UNMAPPED, self.map_page)
        self.uc.hook_add(UC_HOOK_CODE, self.stub)
        self.uc.mem_write(FACILITY + 0x2a0, struct.pack('<Q', INDICATOR))
        self.uc.mem_write(INDICATOR + 0xc8, struct.pack('<I', pattern))
        self.uc.mem_write(INDICATOR + 0xd0, struct.pack('<ddd', min_speed, max_speed, speed))

    def map_page(self, uc, access, address, size, value, data):
        if not self.image.base <= address < self.image.base + self.image.pe.OPTIONAL_HEADER.SizeOfImage:
            raise RuntimeError(f'Unexpected memory access: {address:#x}')
        page = address & ~0xfff
        uc.mem_map(page, 0x1000)
        uc.mem_write(page, self.image.read(page, 0x1000))
        return True

    def stub(self, uc, address, size, data):
        if address not in (TRANSFORM, FNAME):
            return
        out = uc.reg_read(UC_X86_REG_RDX if address == TRANSFORM else UC_X86_REG_RCX)
        uc.mem_write(out, struct.pack('<ddd', *self.emitter) if address == TRANSFORM else bytes(8))
        uc.reg_write(UC_X86_REG_RAX, out)
        rsp = uc.reg_read(UC_X86_REG_RSP)
        ret = struct.unpack('<Q', uc.mem_read(rsp, 8))[0]
        uc.reg_write(UC_X86_REG_RSP, rsp + 8)
        uc.reg_write(UC_X86_REG_RIP, ret)

    def accepts(self, x, y, z, max_distance=500, emitter=(0, 0, 0)):
        self.emitter = emitter
        self.uc.mem_write(TARGET, struct.pack('<ddd', x, y, z))
        self.uc.mem_write(FACILITY + 0x2c0, struct.pack('<d', max_distance))
        rsp = STACK + 0xff00 - 8
        self.uc.mem_write(rsp, struct.pack('<Q', STOP))
        self.uc.reg_write(UC_X86_REG_RSP, rsp)
        self.uc.reg_write(UC_X86_REG_RCX, FACILITY)
        self.uc.reg_write(UC_X86_REG_RDX, TARGET)
        self.uc.emu_start(ENTRY, STOP, count=2000)
        assert self.uc.reg_read(UC_X86_REG_RIP) == STOP
        return bool(self.uc.reg_read(UC_X86_REG_RAX) & 0xff)

