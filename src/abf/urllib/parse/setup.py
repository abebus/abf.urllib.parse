from setuptools import setup, Extension

setup(
    name='abfparse',
    version='0.1',
    ext_modules=[
        Extension(
            'abfparse',
            sources=['abfparsemodule.c', 'parse.c'],
            include_dirs=['.'],
        ),
    ],
)
