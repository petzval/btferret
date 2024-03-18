#### run with build option:   python3 btfpy.py build
import os
from setuptools import setup, Extension

print("Removing any existing module")
os.system('rm build/lib*/btfpy*.so')
module1 = Extension(name='btfpy',sources=['btlib.c'],extra_compile_args=['-D BTFPYTHON'])
ret = setup(name = 'BtfpyPackage',
             version = '1.0',
             description = 'Bluetooth interface',
             ext_modules = [module1])
print("Copying module to btfpy.so")
os.system('cp build/lib*/btfpy*.so btfpy.so')

  


             
