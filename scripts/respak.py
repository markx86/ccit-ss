#!/usr/bin/env python3

import sys
import os
import struct


class Pak:
    def __init__(self):
        self._res = []
        self._running_size = 0

    def _add_res(self, res_path: str):
        file_size = os.path.getsize(os.path.join(self._root, res_path))
        file_size = (file_size + 15) & ~15
        self._res.append((res_path, self._running_size, file_size))
        self._running_size += file_size

    def _detect_root(self, resources: list[str]) -> str:
        root = os.path.commonprefix(resources)
        if os.path.isfile(root):
            root = os.path.dirname(root)
        if root[-1] == "/":
            return root
        else:
            return root + "/"

    def add_res(self, resources: list[str]):
        self._root = self._detect_root(resources)
        for res_path in resources:
            res_subpath = res_path.strip(self._root)
            self._add_res(res_subpath)

    def _compute_path_hash(self, path: str) -> int:
        hash = 0
        for c in path.encode():
            hash = (c + (hash << 6) + (hash << 16) - hash) & 0xFFFFFFFFFFFFFFFF
        return hash

    def _compute_header_size(self) -> tuple[int, dict[str, int]]:
        header_size = 0
        hashes = {}

        def get_resources_by_hash(hash) -> list[str]:
            return list(
                map(lambda x: x[0], filter(lambda x: x[1] == hash, hashes.items()))
            )

        for res_path, _, _ in self._res:
            header_size += 4 * 2  # offset + size
            header_size += 8  # hash
            hashes[res_path] = self._compute_path_hash(res_path)

        unique_hashes = set(hashes.values())
        if len(unique_hashes) != len(hashes.items()):
            for hash in unique_hashes:
                assert len(resources := get_resources_by_hash(hash)) > 1
                for res_path in resources:
                    header_size += len(res_path) + 1
                    hashes[res_path] = 0

        return (header_size, hashes)

    def save(self, path: str):
        header_size, hashes = self._compute_header_size()
        padded_header_size = (header_size + 8 + 15) & ~15
        with open(path, "wb") as file:
            # write header
            header = 0xDEFEC8ED.to_bytes(4, "little")
            header += header_size.to_bytes(4, "little")
            for res_path, res_offset, res_size in self._res:
                hash = hashes[res_path]
                header += struct.pack(
                    "<QII", hash, res_offset + padded_header_size, res_size
                )
                if hash == 0:
                    header += res_path.encode() + b"\x00"
            header = header.ljust(padded_header_size, b"\x00")  # add header padding
            file.write(header)
            # write file data
            for res_path, _, res_size in self._res:
                with open(os.path.join(self._root, res_path), "rb") as res_file:
                    bytes_written = file.write(res_file.read())
                    file.write(b"\x00" * (res_size - bytes_written))


def main():
    if len(sys.argv) < 3:
        print(f"USAGE: {sys.argv[0]} OUTPUT_FILE RESOURCE_FILES...", file=sys.stderr)
        exit(-1)

    pak = Pak()
    pak.add_res(sys.argv[2:])
    pak.save(sys.argv[1])


if __name__ == "__main__":
    main()
