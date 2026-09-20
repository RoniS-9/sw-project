"""Build the symnmfmodule C extension: python3 setup.py build_ext --inplace"""
from setuptools import setup, Extension

module = Extension("symnmfmodule", sources=["symnmfmodule.c", "symnmf.c"])

setup(
    name="symnmfmodule",
    version="1.0",
    description="SymNMF clustering implemented in C",
    ext_modules=[module],
)
