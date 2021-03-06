# Copyright (c) 2021 Oakes, Gregory C. <gregcoakes@gmail.com>
# Author: Oakes, Gregory C. <gregcoakes@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

from setuptools import setup, Extension

spdk_mod = Extension(
    name="spdk",
    sources=["pyspdk.c"],
    libraries=[
        # Shared libraries
        "bsd",
        "numa",
        "uuid",
    ],
    # SPDK requires `-Wl,--whole-archive` to be enabled in order to initialize
    # properly. Applying it as an `extra_link_args` by itself prevents libc
    # from linking properly. Thus, they are ommitted from the `libraries`
    # argument and given directly to the linker.
    #
    # TODO: What exactly does --whole-archive do? Presumably it prevents tree
    # shaking?
    #
    # ref: https://github.com/spdk/spdk/issues/1794
    extra_link_args=[
        "-Wl,--whole-archive",
        # SPDK libraries
        "-lspdk_nvme",
        "-lspdk_env_dpdk",
        "-lspdk_vmd",
        "-lspdk_sock",
        "-lspdk_log",
        "-lspdk_util",
        # DPDK libraries
        "-lrte_eal",
        "-lrte_mempool",
        "-lrte_bus_pci",
        "-Wl,--no-whole-archive",
    ],
)

setup(
    name="pyspdk",
    version="0.1.0",
    ext_modules=[spdk_mod],
)
