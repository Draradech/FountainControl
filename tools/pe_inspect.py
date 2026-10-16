"""Read-only PE function/field/xref inspection for the installed game binary."""
import argparse
import bisect
import struct
from pathlib import Path

import capstone
import pefile

GAME = Path.home() / '.local/share/Steam/steamapps/common/Alchemy Factory/AlchemyFactory/Binaries/Win64/AlchemyFactory-Win64-Shipping.exe'


class Image:
    def __init__(self, path=GAME):
        self.pe = pefile.PE(str(path), fast_load=True)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.text_section = next(s for s in self.pe.sections if s.Name.rstrip(b'\0') == b'.text')
        self.text_base = self.base + self.text_section.VirtualAddress
        self.text = self.text_section.get_data()
        pdata = next(s for s in self.pe.sections if s.Name.rstrip(b'\0') == b'.pdata').get_data()
        directory = self.pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
        entries = [(a, b, u) for a, b, u in struct.iter_unpack('<III', pdata[:directory.Size]) if a and b]
        # MSVC splits functions into multiple RUNTIME_FUNCTION records. Follow
        # UNW_FLAG_CHAININFO so inspection includes every chained code span.
        unwind = {a: (b, u) for a, b, u in entries}
        def root(start):
            seen = set()
            while start in unwind:
                if start in seen:
                    raise ValueError('Cyclic chained unwind metadata')
                seen.add(start)
                _, rva = unwind[start]
                header = self.pe.get_data(rva, 4)
                if not header[0] & 0x20:
                    break
                chain = rva + 4 + ((header[2] + 1) & ~1) * 2
                start = struct.unpack('<III', self.pe.get_data(chain, 12))[0]
            return start
        groups = {}
        for a, b, _ in entries:
            owner = root(a)
            groups.setdefault(owner, []).append((a, b))
        self.functions = sorted((self.base + min(a for a, _ in spans),
                                 self.base + max(b for _, b in spans)) for spans in groups.values())
        self.starts = [a for a, _ in self.functions]
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
        self.md.detail = True

    def read(self, address, length):
        return self.pe.get_data(address - self.base, length)

    def bounds(self, address):
        i = bisect.bisect_right(self.starts, address) - 1
        if i >= 0 and self.functions[i][0] <= address < self.functions[i][1]:
            return self.functions[i]
        return address, address + 256

    def instructions(self, address, length=None):
        start, end = self.bounds(address) if length is None else (address, address + length)
        return self.md.disasm(self.read(start, end - start), start)

    def disassemble(self, address, length=None):
        start, end = self.bounds(address) if length is None else (address, address + length)
        print(f'FUNCTION {start:#x}..{end:#x}')
        for ins in self.instructions(address, length):
            annotations = []
            for op in ins.operands:
                if op.type == capstone.x86.X86_OP_MEM and op.mem.base == capstone.x86.X86_REG_RIP:
                    target = ins.address + ins.size + op.mem.disp
                    try:
                        data = self.read(target, 8)
                        value = struct.unpack('<d', data)[0]
                        annotations.append(f'{target:#x} f64={value:.12g}')
                    except (struct.error, pefile.PEFormatError):
                        annotations.append(f'{target:#x}')
            print(f'{ins.address:#x}: {ins.mnemonic:10} {ins.op_str}' + (' ; ' + ', '.join(annotations) if annotations else ''))

    def field_candidates(self, offsets, low, high):
        candidates = {}
        for field in offsets:
            needle = struct.pack('<I', field)
            pos = self.text.find(needle)
            while pos != -1:
                addr = self.text_base + pos
                if low <= addr < high:
                    start, end = self.bounds(addr)
                    candidates.setdefault((start, end), set()).add(field)
                pos = self.text.find(needle, pos + 1)
        for (start, end), raw_fields in sorted(candidates.items()):
            if len(raw_fields) < len(offsets):
                continue
            fields = set()
            for ins in self.instructions(start, end - start):
                for op in ins.operands:
                    if op.type == capstone.x86.X86_OP_MEM and op.mem.disp in offsets and op.mem.base != capstone.x86.X86_REG_RIP:
                        fields.add(op.mem.disp)
            if len(fields) == len(offsets):
                print(f'{start:#x}..{end:#x} size={end-start:#x} fields={sorted(fields)}')

    def xrefs(self, target):
        for opcode in (b'\xe8', b'\xe9'):
            pos = self.text.find(opcode)
            while 0 <= pos < len(self.text) - 4:
                addr = self.text_base + pos
                if addr + 5 + struct.unpack_from('<i', self.text, pos + 1)[0] == target:
                    print(f'{addr:#x} -> {target:#x} function={self.bounds(addr)[0]:#x} opcode={opcode.hex()}')
                pos = self.text.find(opcode, pos + 1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, default=GAME, help='Executable to inspect (read only)')
    parser.add_argument('mode', choices=['disasm', 'fields', 'xrefs', 'constants'])
    parser.add_argument('values', nargs='+', type=lambda s: int(s, 0))
    parser.add_argument('--low', type=lambda s: int(s, 0), default=0x144900000)
    parser.add_argument('--high', type=lambda s: int(s, 0), default=0x144f00000)
    parser.add_argument('--length', type=lambda s: int(s, 0))
    args = parser.parse_args()
    image = Image(args.game)
    if args.mode == 'disasm':
        for address in args.values:
            image.disassemble(address, args.length)
    elif args.mode == 'fields':
        image.field_candidates(args.values, args.low, args.high)
    elif args.mode == 'xrefs':
        for address in args.values:
            image.xrefs(address)
    elif args.mode == 'constants':
        for address in args.values:
            print(f'{address:#x}: {struct.unpack("<d", image.read(address, 8))[0]!r}')
