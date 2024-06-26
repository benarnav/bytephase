from setuptools import setup, Extension
from os.path import join

extension_dir = "bytephase/"

sources = [join(extension_dir, "_bpe.c")]

module = Extension(
    "_bpe",
    sources=sources,
    include_dirs=[extension_dir],
    extra_compile_args=["-O3"],
)

setup(
    name="bytephase",
    version="1.0",
    description="bytephase: Byte Pair Encoder",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    author="Benjamin Arnav",
    author_email="contact_arnav.darkened639@8alias.com",
    url="https://github.com/benarnav/bytephase",
    ext_modules=[module],
    packages=["bytephase"],
    package_dir={"bytephase": extension_dir},
    package_data={"bytephase": ["*.c"]},
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.8",
    install_requires=[
        "regex>=2024.5.15",
    ],
)
